#include "PythonEngine.h"
#include "PartyBridge.h"
#include "PythonOutput.h"
#include "HookRegistry.h"
#include "PopulationBridge.h"
#include "Config.h"
#include "Timer.h"
#include <algorithm>
#include <filesystem>
#include <system_error>
#include <vector>

namespace fs = std::filesystem;

namespace
{
    bool ContainsParentTraversal(fs::path const& path)
    {
        return std::find(path.begin(), path.end(), fs::path("..")) != path.end();
    }

    bool RejectSymlinkComponents(fs::path const& path, std::string& error)
    {
        fs::path current;
        for (fs::path const& component : path)
        {
            current /= component;
            std::error_code ec;
            fs::file_status status = fs::symlink_status(current, ec);
            if (ec || fs::is_symlink(status))
            {
                error = "cannot safely inspect path component: " + current.string();
                return false;
            }
        }
        return true;
    }

    bool IsWithinRoot(fs::path const& root, fs::path const& candidate)
    {
        fs::path relative = candidate.lexically_relative(root);
        return !relative.empty() && !relative.is_absolute() &&
               std::find(relative.begin(), relative.end(), fs::path("..")) == relative.end();
    }

    bool CollectScriptFiles(fs::path const& configuredRoot, fs::path& canonicalRoot,
                            std::vector<fs::path>& scriptFiles, std::string& error)
    {
        if (configuredRoot.empty())
        {
            error = "Python.ScriptsPath is empty";
            return false;
        }
        if (!configuredRoot.is_absolute() || ContainsParentTraversal(configuredRoot))
        {
            error = "Python.ScriptsPath must be absolute and must not contain parent traversal";
            return false;
        }
        if (!RejectSymlinkComponents(configuredRoot, error))
            return false;
        std::error_code ec;
        canonicalRoot = fs::canonical(configuredRoot, ec);
        if (ec)
        {
            error = "cannot canonicalize Python.ScriptsPath: " + ec.message();
            return false;
        }
        if (!fs::is_directory(canonicalRoot, ec) || ec)
        {
            error = "Python.ScriptsPath is not a readable directory: " + canonicalRoot.string();
            return false;
        }

        fs::recursive_directory_iterator iterator(canonicalRoot, fs::directory_options::none, ec);
        fs::recursive_directory_iterator end;
        if (ec)
        {
            error = "cannot enumerate Python.ScriptsPath: " + ec.message();
            return false;
        }

        while (iterator != end)
        {
            fs::directory_entry const& entry = *iterator;
            fs::file_status status = entry.symlink_status(ec);
            if (ec)
            {
                error = "cannot inspect script entry '" + entry.path().string() + "': " + ec.message();
                return false;
            }
            if (fs::is_symlink(status))
            {
                error = "symlink entries are forbidden in Python.ScriptsPath: " + entry.path().string();
                return false;
            }

            if (entry.path().extension() == ".py")
            {
                if (!fs::is_regular_file(status))
                {
                    error = "Python script entry is not a regular file: " + entry.path().string();
                    return false;
                }

                fs::path canonicalFile = fs::canonical(entry.path(), ec);
                if (ec)
                {
                    error = "cannot canonicalize Python script '" + entry.path().string() + "': " + ec.message();
                    return false;
                }
                if (!IsWithinRoot(canonicalRoot, canonicalFile))
                {
                    error = "Python script escaped the configured root: " + entry.path().string();
                    return false;
                }
                scriptFiles.push_back(std::move(canonicalFile));
            }

            iterator.increment(ec);
            if (ec)
            {
                error = "error while enumerating Python.ScriptsPath: " + ec.message();
                return false;
            }
        }

        std::sort(scriptFiles.begin(), scriptFiles.end(), [](fs::path const& left, fs::path const& right)
        {
            return left.generic_string() < right.generic_string();
        });
        return true;
    }

    bool IsSafeEventName(std::string const& name)
    {
        if (name.empty() || name.size() > 96)
            return false;

        return std::all_of(name.begin(), name.end(), [](unsigned char value)
        {
            return (value >= 'a' && value <= 'z') ||
                   (value >= '0' && value <= '9') || value == '.' || value == '_' || value == '-';
        });
    }
}

// Singleton Instance
PythonEngine* PythonEngine::instance()
{
    static PythonEngine instance;
    return &instance;
}

PythonEngine::~PythonEngine()
{
    // NOTE: In practice, this should rarely execute actual cleanup since the
    // server's shutdown sequence should call Shutdown() explicitly. However,
    // this guarantees cleanup even in abnormal termination scenarios like
    // crashes during shutdown or unexpected destructor ordering.
    Shutdown();
}

void PythonEngine::Initialize()
{
    if (Py_IsInitialized())
        return;

    bool isEnabled = sConfigMgr->GetOption<bool>("Python.Enabled", false);
    if (!isEnabled)
    {
        LOG_INFO("module.python", "Python Engine is disabled in config.");
        return;
    }

    LOG_INFO("module.python", "Initializing Python Engine...");

    try
    {
        InitAzerothCoreModule();

        // Initialize Python - main thread now HOLDS the GIL
        Py_Initialize();
        initializationThread = std::this_thread::get_id();

        // Establish an isolated initial namespace. LoadScripts builds another
        // candidate namespace and publishes it only after every script succeeds.
        {
            API::Dict initialNamespace;
            initialNamespace["__builtins__"] = API::Import("builtins");
            initialNamespace["__name__"] = "__main__";
            main_namespace = initialNamespace;
            populationCallback = API::Object();
        }

        // CRITICAL: After Py_Initialize(), the main thread holds the GIL. We must
        // release it to allow other threads to use PyGILState_Ensure (GILGuard).
        mainThreadState = PyEval_SaveThread();

        // Release: all initialization writes are visible when enabled is set
        reloading.store(false, std::memory_order_release);
        enabled.store(true, std::memory_order_release);

        LOG_INFO("module.python", "General-purpose Python Engine initialized.");
    }
    catch (...)
    {
        enabled.store(false, std::memory_order_relaxed);

        // Initialization and finalization stay on the initialization thread.
        // Python-owned references are released while that thread owns the GIL.
        if (Py_IsInitialized())
        {
            if (mainThreadState)
            {
                PyEval_RestoreThread(mainThreadState);
                mainThreadState = nullptr;
            }
            hookMap.clear();
            scheduledCallbacks.clear();
            safeEventMap.clear();
            main_namespace = API::Object();
            ClearBorrowedReferences();
            if (Py_FinalizeEx() < 0)
                LOG_ERROR("module.python", "Python cleanup after initialization failure reported an error.");
            initializationThread = std::thread::id();
        }

        LOG_ERROR("module.python", "Python Engine Initialization Failed!");
    }
}

void PythonEngine::Shutdown()
{
    PyEng::Party::InvalidateContexts();
    if (!Py_IsInitialized())
        return;

    LOG_INFO("module.python", "Shutting down Python Engine...");

    // Set disabled with release semantics
    enabled.store(false, std::memory_order_release);
    reloading.store(true, std::memory_order_release);
    bool populationWasHealthy = populationHealthy.exchange(false, std::memory_order_acq_rel);
    safeEventsHealthy.store(false, std::memory_order_release);
    if (populationWasHealthy && !PyEng::Population::FailClosed())
        LOG_ERROR("module.python", "Population provider did not acknowledge shutdown fail-close.");

    // Native dispatch is quiesced before shutdown; all registry operations take the GIL first.
    if (!mainThreadState)
    {
        LOG_ERROR("module.python", "Python Engine main thread state is unavailable; refusing unsafe finalization.");
        return;
    }
    if (initializationThread != std::this_thread::get_id())
    {
        LOG_ERROR("module.python", "Python shutdown is not running on the initialization thread; refusing unsafe "
                  "thread-state restoration and finalization.");
        return;
    }

    PyEval_RestoreThread(mainThreadState);
    std::unique_lock<std::shared_mutex> lock(hookMutex);
    mainThreadState = nullptr;
    hookMap.clear();
    scheduledCallbacks.clear();
    safeEventMap.clear();
    populationCallback = API::Object();
    main_namespace = API::Object(); // Release main module ref while holding the GIL

    // WARNING: This should only be called if no other C++ static objects hold
    // Python references, as accessing them after finalization causes crashes.
    // Py_FinalizeEx destroys the GIL, so no RAII GIL guard may span this call.
    ClearBorrowedReferences();
    if (Py_FinalizeEx() < 0)
        LOG_ERROR("module.python", "Python Engine finalization reported an error.");

    reloading.store(false, std::memory_order_release);
    initializationThread = std::thread::id();

    LOG_INFO("module.python", "Python Engine shutdown complete.");
}

bool PythonEngine::LoadScripts()
{
    // Acquire: see initialization writes
    if (!enabled.load(std::memory_order_acquire))
        return false;

    if (triggerDepth > 0)
        return false;
    struct ReloadGuard
    {
        PythonEngine& engine;
        bool owns;
        ~ReloadGuard() { if (owns) engine.EndReload(); }
    } reloadGuard{*this, BeginReload()};
    if (!reloadGuard.owns)
        return false;
    uint32 oldMSTime = getMSTime();

    LOG_INFO("module.python", "Loading Python scripts...");

    fs::path configuredRoot(sConfigMgr->GetOption<std::string>("Python.ScriptsPath", ""));
    fs::path canonicalRoot;
    std::vector<fs::path> scriptFiles;
    std::string pathError;
    if (!CollectScriptFiles(configuredRoot, canonicalRoot, scriptFiles, pathError))
    {
        LOG_ERROR("module.python", "Python script root rejected: {}", pathError);
        return false;
    }

    bool const populationEnabled = sConfigMgr->GetOption<bool>("Python.Population.Enabled", false);
    fs::path populationEntry;
    bool hasPopulationEntry = false;
    if (populationEnabled)
    {
        populationEntry = fs::path(sConfigMgr->GetOption<std::string>("Python.Population.EntryScript", ""));
        hasPopulationEntry = !populationEntry.empty();
        if (hasPopulationEntry &&
            (populationEntry.is_absolute() || ContainsParentTraversal(populationEntry)))
        {
            LOG_ERROR("module.python", "Python.Population.EntryScript must be one relative non-traversing path.");
            return false;
        }
        if (hasPopulationEntry)
        {
            fs::path requested = (canonicalRoot / populationEntry).lexically_normal();
            auto found = std::find(scriptFiles.begin(), scriptFiles.end(), requested);
            if (found == scriptFiles.end())
            {
                LOG_ERROR("module.python", "Python population entry script '{}' is absent or rejected.",
                          requested.string());
                return false;
            }
        }
    }

    if (triggerDepth > 0)
        return false;
    GILGuard gil;
    CallbackScope scope;
    HookRegistry candidateHookMap;
    SafeEventRegistry candidateSafeEventMap;
    CallbackList candidateScheduledCallbacks;
    API::Object candidateNamespace;
    API::Object candidatePopulationCallback;
    API::Object candidateCommit;
    int count = 0;
    try
    {
        API::Object sys = API::Import("sys");
        API::Object modulesObject = sys.attr("modules");
        API::Object pathObject = sys.attr("path");
        API::Object metaPathObject = sys.attr("meta_path");
        API::Dict modules = boost::python::extract<API::Dict>(modulesObject);
        API::Dict previousModules = boost::python::extract<API::Dict>(modules.attr("copy")());
        // Python call arguments must be concrete objects, not Boost attribute proxies.
        API::Object previousPath = API::Import("copy").attr("copy")(pathObject);
        API::Object previousMetaPath = API::List(metaPathObject);
        sys.attr("path") = API::List(pathObject);
        bool published = false;
        struct ImportRollback
        {
            API::Dict& modules;
            API::Dict& previous;
            API::Object& sys;
            API::Object& previousPath;
            API::Object& previousMetaPath;
            bool& published;
            ~ImportRollback()
            {
                if (!published)
                {
                    try
                    {
                        modules.clear();
                        modules.update(previous);
                        sys.attr("path") = previousPath;
                        sys.attr("meta_path") = previousMetaPath;
                    }
                    catch (...)
                    {
                        auto error = ExceptionHelper::Format();
                        LOG_ERROR("module.python", "Import registry rollback failed: {}", error);
                    }
                }
            }
        } importRollback{modules, previousModules, sys, previousPath, previousMetaPath, published};

        API::Dict namespaceDictionary;
        namespaceDictionary["__builtins__"] = API::Import("builtins");
        namespaceDictionary["__name__"] = "__main__";
        namespaceDictionary["__script_root__"] = canonicalRoot.string();
        candidateNamespace = namespaceDictionary;
        // Reload local imports too; rejected candidates restore the prior module registry.
        API::Exec(R"PY(
def _engine_refresh_sources(script_root):
    import sys, pathlib, importlib, importlib.machinery as machinery
    root = pathlib.Path(script_root).resolve()
    for name, module in list(sys.modules.items()):
        filename = getattr(module, '__file__', None)
        if filename and pathlib.Path(filename).resolve().is_relative_to(root):
            del sys.modules[name]
    class FreshSourceLoader(machinery.SourceFileLoader):
        def get_code(self, fullname):
            filename = self.get_filename(fullname)
            return self.source_to_code(self.get_data(filename), filename)
    class RootSourceFinder:
        _python_engine_source_finder = True
        def find_spec(self, fullname, path=None, target=None):
            spec = machinery.PathFinder.find_spec(fullname, path, target)
            if (spec and spec.origin and isinstance(spec.loader, machinery.SourceFileLoader)
                    and pathlib.Path(spec.origin).resolve().is_relative_to(root)):
                spec.loader = FreshSourceLoader(fullname, spec.origin)
                return spec
            return None
    finders = [finder for finder in sys.meta_path
               if not getattr(finder, '_python_engine_source_finder', False)]
    index = next((i for i, finder in enumerate(finders) if finder is machinery.PathFinder), len(finders))
    finders.insert(index, RootSourceFinder())
    sys.meta_path = finders
    importlib.invalidate_caches()
_engine_refresh_sources(__script_root__)
del _engine_refresh_sources
)PY", candidateNamespace, candidateNamespace);

        API::Object sysPath = API::Import("sys").attr("path");
        API::Object scriptRoot = API::FromString(canonicalRoot.string());
        if (!boost::python::extract<bool>(sysPath.attr("__contains__")(scriptRoot)))
            sysPath.attr("insert")(0, scriptRoot);

        candidateRegistrationTarget = &candidateHookMap;
        candidateScheduledTarget = &candidateScheduledCallbacks;
        candidateSafeEventRegistrationTarget = &candidateSafeEventMap;

        for (fs::path const& scriptFile : scriptFiles)
        {
            LOG_DEBUG("module.python", "Executing script: {}...", scriptFile.string());

            try
            {
                bool const isPopulationEntry = hasPopulationEntry &&
                    scriptFile == (canonicalRoot / populationEntry).lexically_normal();
                if (isPopulationEntry)
                {
                    namespaceDictionary.attr("pop")("__previous_population_namespace__", API::Object());
                    namespaceDictionary.attr("pop")("__commit_reload__", API::Object());
                    if (populationHealthy.load(std::memory_order_acquire))
                        namespaceDictionary["__previous_population_namespace__"] = main_namespace;
                }
                candidateNamespace["__file__"] = scriptFile.string();
                API::ExecFile(scriptFile.string(), candidateNamespace, candidateNamespace);
                if (isPopulationEntry)
                {
                    // Capture only this entry's transaction; later scripts cannot replace it.
                    namespaceDictionary.attr("pop")("__previous_population_namespace__", API::Object());
                    if (namespaceDictionary.has_key("__commit_reload__"))
                    {
                        candidateCommit = namespaceDictionary.attr("pop")("__commit_reload__");
                        if (!API::IsCallable(candidateCommit))
                            throw std::runtime_error("__commit_reload__ must be callable");
                    }
                }
                count++;
            }
            catch (...)
            {
                candidateRegistrationTarget = nullptr;
                candidateScheduledTarget = nullptr;
                candidateSafeEventRegistrationTarget = nullptr;
                LOG_ERROR("module.python", "Failed to execute script: {}", scriptFile.string());
                auto error = ExceptionHelper::Format();
                LOG_ERROR("module.python", "{}", error);
                LOG_ERROR("module.python", "Candidate reload rejected; the last accepted registry and namespace "
                          "were preserved. Python or external side effects outside those staged objects cannot be "
                          "rolled back.");
                return false;
            }
        }
        candidateRegistrationTarget = nullptr;
        candidateScheduledTarget = nullptr;
        candidateSafeEventRegistrationTarget = nullptr;

        if (populationEnabled)
        {
            API::Dict namespaceDictionary = boost::python::extract<API::Dict>(candidateNamespace);
            if (namespaceDictionary.has_key("population_tick"))
            {
                candidatePopulationCallback = namespaceDictionary["population_tick"];
                if (!API::IsCallable(candidatePopulationCallback))
                    throw std::runtime_error("population_tick is not callable");
            }
            else if (hasPopulationEntry)
            {
                throw std::runtime_error("configured population entry did not publish population_tick");
            }
        }

        namespaceDictionary.attr("pop")("__previous_population_namespace__", API::Object());
        namespaceDictionary.attr("pop")("__commit_reload__", API::Object());
        bool const candidatePopulationHealthy = API::IsCallable(candidatePopulationCallback);
        std::string const publishedRoot = canonicalRoot.string();
        std::unique_lock<std::shared_mutex> lock(hookMutex);
        if (!API::IsNone(candidateCommit))
            candidateCommit();
        // These swaps/atomic stores only transfer ownership; no Python callbacks/config reads.
        auto publishCandidate = [&]() noexcept
        {
            hookMap.swap(candidateHookMap);
            scheduledCallbacks.swap(candidateScheduledCallbacks);
            safeEventMap.swap(candidateSafeEventMap);
            std::swap(main_namespace, candidateNamespace);
            if (populationEnabled)
            {
                std::swap(populationCallback, candidatePopulationCallback);
                populationHealthy.store(candidatePopulationHealthy, std::memory_order_release);
            }
            safeEventsHealthy.store(true, std::memory_order_release);
            PyEng::Party::InvalidateContexts();
            published = true;
        };
        publishCandidate();
        // Diagnostics cannot turn an already committed publication into a rejection.
        try
        {
            LOG_INFO("module.python", ">> Published {} Python scripts from canonical root '{}' in {} ms.", count,
                     publishedRoot, GetMSTimeDiffToNow(oldMSTime));
        }
        catch (...) { }
        return true;
    }
    catch (...)
    {
        candidateRegistrationTarget = nullptr;
        candidateScheduledTarget = nullptr;
        candidateSafeEventRegistrationTarget = nullptr;
        auto error = ExceptionHelper::Format();
        LOG_ERROR("module.python", "Candidate reload failed before publication: {}", error);
        LOG_ERROR("module.python", "The last accepted registry and namespace were preserved; external Python "
                  "side effects are outside the transactional contract.");
        return false;
    }
}

bool PythonEngine::RunPopulationTick(uint32 diff)
{
    if (!enabled.load(std::memory_order_acquire) || !sConfigMgr->GetOption<bool>("Python.Population.Enabled", false) ||
        reloading.load(std::memory_order_acquire))
        return false;

    GILGuard gil;
    API::Object callback;
    {
        std::shared_lock<std::shared_mutex> lock(hookMutex);
        callback = populationCallback;
    }
    if (API::IsNone(callback))
        return true;
    if (!populationHealthy.load(std::memory_order_acquire))
        return false;
    if (initializationThread != std::this_thread::get_id() || populationTickActive || safeEventDispatchActive)
    {
        populationHealthy.store(false, std::memory_order_release);
        PyEng::Population::FailClosed();
        LOG_ERROR("module.python", "population-v1 dispatch violated its world-thread/non-reentrancy fence.");
        return false;
    }

    uint32 started = getMSTime();
    CallbackScope scope;
    struct TickGuard
    {
        TickGuard() { populationTickActive = true; triggerDepth++; }
        ~TickGuard() { triggerDepth--; populationTickActive = false; }
    } guard;

    bool accepted = false;
    try
    {
        accepted = boost::python::extract<bool>(callback(diff));
    }
    catch (...)
    {
        auto error = ExceptionHelper::Format();
        LOG_ERROR("module.python", "population-v1 tick raised an exception: {}", error);
    }
    uint32 elapsed = GetMSTimeDiffToNow(started);
    uint32 maximum = sConfigMgr->GetOption<uint32>("Python.Population.MaxTickMilliseconds", 25);
    if (!accepted || elapsed > maximum)
    {
        populationHealthy.store(false, std::memory_order_release);
        if (!PyEng::Population::FailClosed())
            LOG_ERROR("module.python", "Population provider did not acknowledge population-v1 fail-close.");
        LOG_ERROR("module.python", "population-v1 disabled after tick result={} elapsed={}ms maximum={}ms. "
                  "In-process Python cannot be preempted while hung; runtime acceptance must prove this bound.",
                  accepted, elapsed, maximum);
        return false;
    }
    return true;
}

bool PythonEngine::RegisterSafeEvent(std::string const& eventName, API::Object callback)
{
    if (!enabled.load(std::memory_order_acquire) ||
        !IsSafeEventName(eventName) || !API::IsCallable(callback) ||
        (IsReloading() && !candidateSafeEventRegistrationTarget))
        return false;

    try
    {
        if (candidateSafeEventRegistrationTarget)
            (*candidateSafeEventRegistrationTarget)[eventName].push_back(callback);
        else
        {
            std::unique_lock<std::shared_mutex> lock(hookMutex);
            safeEventMap[eventName].push_back(callback);
        }
        LOG_DEBUG("module.python", "Staged safe event '{}'", eventName);
        return true;
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Failed to stage safe event '{}'", eventName);
        auto error = ExceptionHelper::Format();
        LOG_ERROR("module.python", "{}", error);
        return false;
    }
}

bool PythonEngine::DispatchSafeEvent(std::string const& eventName,
                                     PyEng::Events::Fields const& fields)
{
    if (!enabled.load(std::memory_order_acquire) ||
        !IsSafeEventName(eventName) ||
        reloading.load(std::memory_order_acquire) ||
        initializationThread != std::this_thread::get_id() || safeEventDispatchActive)
        return false;

    GILGuard gil;
    CallbackList callbacks;
    {
        std::shared_lock<std::shared_mutex> lock(hookMutex);
        auto found = safeEventMap.find(eventName);
        if (found == safeEventMap.end()) return true;
        callbacks = found->second;
    }
    CallbackScope scope;
    struct EventGuard
    {
        EventGuard()
        {
            safeEventDispatchActive = true;
            triggerDepth++;
        }
        ~EventGuard()
        {
            triggerDepth--;
            safeEventDispatchActive = false;
        }
    } guard;

    bool accepted = true;
    uint32 started = getMSTime();
    try
    {
        API::Dict payload;
        for (PyEng::Events::Field const& field : fields)
        {
            std::visit([&](auto const& value)
            {
                payload[field.name] = value;
            }, field.value);
        }

        for (API::Object const& callback : callbacks)
        {
            try { callback(payload); }
            catch (...)
            {
                auto error = ExceptionHelper::Format();
                LOG_ERROR("module.python", "Event '{}' raised an exception: {}", eventName, error);
                accepted = false;
            }
        }
    }
    catch (...)
    {
        auto error = ExceptionHelper::Format();
        LOG_ERROR("module.python", "Event '{}' raised an exception: {}", eventName, error);
        accepted = false;
    }

    uint32 elapsed = GetMSTimeDiffToNow(started);
    uint32 maximum = sConfigMgr->GetOption<uint32>("Python.SafeEvent.MaxMilliseconds", 25);

    if (!accepted || elapsed > maximum)
    {
        LOG_WARN("module.python", "Event '{}' returned result={} after {}ms (observation threshold {}ms).",
                 eventName, accepted, elapsed, maximum);
        return false;
    }
    return true;
}

bool PythonEngine::RegisterHook(std::string const& eventName, API::Object callback, uint32 entryId)
{
    // Acquire: see initialization
    if (!enabled.load(std::memory_order_acquire) || eventName.empty())
        return false;

    if (IsReloading() && !candidateRegistrationTarget)
    {
        LOG_ERROR("module.python", "Hook '{}' registration raced a reload; retry after publication.", eventName);
        return false;
    }

    // Validate that the provided object is actually a function or callable object.
    // This prevents runtime crashes later when the event attempts to trigger.
    if (!PyCallable_Check(callback.ptr()))
    {
        LOG_ERROR("module.python", "Attempted to register a non-callable object for hook '{}' (entry {}).",
                  eventName, entryId);
        return false;
    }

    using PyEng::Hooks::FindGeneratedHook;
    using PyEng::Hooks::GeneratedHookRecord;
    GeneratedHookRecord const* generated = FindGeneratedHook(eventName);
    if (!generated)
    {
        LOG_ERROR("module.python", "Attempted to register unknown event hook '{}'.", eventName);
        return false;
    }
    if (!generated->forwarded)
    {
        LOG_ERROR("module.python", "Hook '{}' is declared but unsupported because no native forwarder exists.",
                  eventName);
        return false;
    }

    using PyEng::Hooks::GetHookByName;
    auto hookIdOpt = GetHookByName(eventName);

    if (!hookIdOpt.has_value())
    {
        LOG_ERROR("module.python", "Generated hook inventory mismatch for '{}'; refusing registration.", eventName);
        return false;
    }

    HookInfo hookId = hookIdOpt.value();

    try
    {
        if (candidateRegistrationTarget)
            (*candidateRegistrationTarget)[hookId][entryId].push_back(callback);
        else
        {
            std::unique_lock<std::shared_mutex> lock(hookMutex);
            hookMap[hookId][entryId].push_back(callback);
        }
        LOG_DEBUG("module.python", "Staged hook '{}' (entry {})", eventName, entryId);
        return true;
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Failed to register hook '{}' (entry {})", eventName, entryId);
        auto error = ExceptionHelper::Format();
        LOG_ERROR("module.python", "{}", error);
        return false;
    }
}

ExecutionResult PythonEngine::ExecuteCode(std::string const& code)
{
    // Acquire: see initialization
    if (!enabled.load(std::memory_order_acquire))
        return ExecutionResult::Error("Python Engine is disabled");

    if (triggerDepth > 0)
        return ExecutionResult::Error("Cannot execute while hooks are running");

    if (IsReloading())
        return ExecutionResult::Error("Python engine is reloading");
    GILGuard gil;
    CallbackScope scope;

    try
    {
        OutputCapture capture;
        API::Exec(code, main_namespace, main_namespace);
        std::string output = capture.GetOutput();

        return ExecutionResult::Success(
            output.empty() ? "Execution completed successfully" : output
        );
    }
    catch (...)
    {
        return ExecutionResult::Error(ExceptionHelper::Format());
    }
}
