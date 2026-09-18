#include "PythonEngine.h"
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
    constexpr char SafeProfile[] = "safe-static-v1";
    constexpr char PopulationProfile[] = "population-v1";
    constexpr char ComposedProfile[] = "safe-composed-v1";
    constexpr char UnsafeProfile[] = "unsafe-development-v1";
    constexpr char UnsafeAcknowledgement[] = "CPI-223-UNSAFE-DEVELOPMENT-ONLY";

    bool ContainsParentTraversal(fs::path const& path)
    {
        return std::find(path.begin(), path.end(), fs::path("..")) != path.end();
    }

    bool RejectSymlinkComponents(fs::path const& absolutePath, std::string& error)
    {
        fs::path current;
        for (fs::path const& component : absolutePath)
        {
            current /= component;
            std::error_code ec;
            fs::file_status status = fs::symlink_status(current, ec);
            if (ec)
            {
                error = "cannot inspect path component '" + current.string() + "': " + ec.message();
                return false;
            }
            if (fs::is_symlink(status))
            {
                error = "symlink path component is forbidden: " + current.string();
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
        if (!configuredRoot.is_absolute())
        {
            error = "Python.ScriptsPath must be absolute; current-working-directory resolution is forbidden";
            return false;
        }
        if (ContainsParentTraversal(configuredRoot))
        {
            error = "Python.ScriptsPath must not contain parent traversal components";
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

    std::string configuredProfile = sConfigMgr->GetOption<std::string>("Python.CapabilityProfile",
                                                                        std::string(SafeProfile));
    CapabilityProfile selectedProfile = CapabilityProfile::SafeStaticV1;
    if (configuredProfile == UnsafeProfile)
    {
        bool allowUnsafe = sConfigMgr->GetOption<bool>("Python.AllowUnsafeDevelopmentProfile", false);
        std::string acknowledgement = sConfigMgr->GetOption<std::string>(
            "Python.UnsafeDevelopmentAcknowledgement", "");
        if (!allowUnsafe || acknowledgement != UnsafeAcknowledgement)
        {
            LOG_ERROR("module.python", "Unsafe Python profile requested without the explicit CPI-223 development "
                      "gate and acknowledgement; refusing initialization.");
            return;
        }
        selectedProfile = CapabilityProfile::UnsafeDevelopmentV1;
    }
    else if (configuredProfile == PopulationProfile)
    {
        selectedProfile = CapabilityProfile::PopulationV1;
    }
    else if (configuredProfile == ComposedProfile)
    {
        selectedProfile = CapabilityProfile::SafeComposedV1;
    }
    else if (configuredProfile != SafeProfile)
    {
        LOG_ERROR("module.python", "Unknown Python.CapabilityProfile '{}'; refusing initialization.",
                  configuredProfile);
        return;
    }

    capabilityProfile.store(selectedProfile, std::memory_order_release);
    bool execAllowed = selectedProfile == CapabilityProfile::UnsafeDevelopmentV1 &&
                       sConfigMgr->GetOption<bool>("Python.AllowExecCommand", false);
    allowExecCommand.store(execAllowed, std::memory_order_release);

    LOG_INFO("module.python", "Initializing Python Engine...");

    try
    {
        InitAzerothCoreModule();

        // Initialize Python - main thread now HOLDS the GIL
        Py_Initialize();
        initializationThread = std::this_thread::get_id();

        // Establish an isolated initial namespace. LoadScripts builds another
        // candidate namespace and publishes it only after every script succeeds.
        API::Dict initialNamespace;
        initialNamespace["__builtins__"] = API::Import("builtins");
        initialNamespace["__name__"] = "__main__";
        main_namespace = initialNamespace;
        populationCallback = API::Object();

        // CRITICAL: After Py_Initialize(), the main thread holds the GIL. We must
        // release it to allow other threads to use PyGILState_Ensure (GILGuard).
        mainThreadState = PyEval_SaveThread();

        // Release: all initialization writes are visible when enabled is set
        reloading.store(false, std::memory_order_release);
        enabled.store(true, std::memory_order_release);

        LOG_INFO("module.python", "Python Engine initialized with capability profile '{}'.",
                 GetCapabilityProfileName());
    }
    catch (...)
    {
        enabled.store(false, std::memory_order_relaxed);
        allowExecCommand.store(false, std::memory_order_relaxed);

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
            safeEventMap.clear();
            main_namespace = API::Object();
            if (Py_FinalizeEx() < 0)
                LOG_ERROR("module.python", "Python cleanup after initialization failure reported an error.");
            initializationThread = std::thread::id();
        }

        LOG_ERROR("module.python", "Python Engine Initialization Failed!");
    }
}

void PythonEngine::Shutdown()
{
    if (!Py_IsInitialized())
        return;

    LOG_INFO("module.python", "Shutting down Python Engine...");

    // Set disabled with release semantics
    enabled.store(false, std::memory_order_release);
    reloading.store(true, std::memory_order_release);
    allowExecCommand.store(false, std::memory_order_release);
    bool populationWasHealthy = populationHealthy.exchange(false, std::memory_order_acq_rel);
    safeEventsHealthy.store(false, std::memory_order_release);
    if (populationWasHealthy && !PyEng::Population::FailClosed())
        LOG_ERROR("module.python", "Population provider did not acknowledge shutdown fail-close.");

    // Block new registrations/callbacks before restoring the initialization
    // thread state. Restoring first could deadlock with a callback that holds a
    // shared hook lock while waiting for the GIL.
    std::unique_lock<std::shared_mutex> lock(hookMutex);
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
    mainThreadState = nullptr;
    hookMap.clear();
    safeEventMap.clear();
    populationCallback = API::Object();
    main_namespace = API::Object(); // Release main module ref while holding the GIL

    // WARNING: This should only be called if no other C++ static objects hold
    // Python references, as accessing them after finalization causes crashes.
    // Py_FinalizeEx destroys the GIL, so no RAII GIL guard may span this call.
    if (Py_FinalizeEx() < 0)
        LOG_ERROR("module.python", "Python Engine finalization reported an error.");

    capabilityProfile.store(CapabilityProfile::SafeStaticV1, std::memory_order_release);
    reloading.store(false, std::memory_order_release);
    initializationThread = std::thread::id();

    LOG_INFO("module.python", "Python Engine shutdown complete.");
}

bool PythonEngine::LoadScripts()
{
    // Acquire: see initialization writes
    if (!enabled.load(std::memory_order_acquire))
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

    fs::path populationEntry;
    bool hasPopulationEntry = false;
    if (HasPopulationCapability())
    {
        populationEntry = fs::path(sConfigMgr->GetOption<std::string>("Python.Population.EntryScript", ""));
        hasPopulationEntry = !populationEntry.empty();
        if (IsPopulationProfile() && !hasPopulationEntry)
        {
            LOG_ERROR("module.python", "Python.Population.EntryScript must be one relative non-traversing path.");
            return false;
        }
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
            if (IsPopulationProfile())
                scriptFiles.assign(1, *found);
        }
    }

    // Lock ordering is always hook lock before GIL. Existing callbacks finish
    // first; candidate Python objects are created and destroyed while the GIL
    // is held. The active registry/namespace are not changed until publication.
    std::unique_lock<std::shared_mutex> lock(hookMutex);
    GILGuard gil;
    HookRegistry candidateHookMap;
    SafeEventRegistry candidateSafeEventMap;
    API::Object candidateNamespace;
    API::Object candidatePopulationCallback;
    int count = 0;

    try
    {
        API::Dict namespaceDictionary;
        namespaceDictionary["__builtins__"] = API::Import("builtins");
        namespaceDictionary["__name__"] = "__main__";
        namespaceDictionary["__script_root__"] = canonicalRoot.string();
        candidateNamespace = namespaceDictionary;

        API::Object sysPath = API::Import("sys").attr("path");
        API::Object scriptRoot = API::FromString(canonicalRoot.string());
        if (!boost::python::extract<bool>(sysPath.attr("__contains__")(scriptRoot)))
            sysPath.attr("insert")(0, scriptRoot);

        candidateRegistrationTarget = &candidateHookMap;
        candidateSafeEventRegistrationTarget = &candidateSafeEventMap;

        for (fs::path const& scriptFile : scriptFiles)
        {
            LOG_DEBUG("module.python", "Executing script: {}...", scriptFile.string());

            try
            {
                API::ExecFile(scriptFile.string(), candidateNamespace, candidateNamespace);
                count++;
            }
            catch (...)
            {
                candidateRegistrationTarget = nullptr;
                candidateSafeEventRegistrationTarget = nullptr;
                LOG_ERROR("module.python", "Failed to execute script: {}", scriptFile.string());
                LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
                LOG_ERROR("module.python", "Candidate reload rejected; the last accepted registry and namespace "
                          "were preserved. Python or external side effects outside those staged objects cannot be "
                          "rolled back.");
                return false;
            }
        }
        candidateRegistrationTarget = nullptr;
        candidateSafeEventRegistrationTarget = nullptr;

        if (HasPopulationCapability())
        {
            API::Dict namespaceDictionary = boost::python::extract<API::Dict>(candidateNamespace);
            if (namespaceDictionary.has_key("population_tick"))
            {
                candidatePopulationCallback = namespaceDictionary["population_tick"];
                if (!API::IsCallable(candidatePopulationCallback))
                    throw std::runtime_error("population_tick is not callable");
            }
            else if (IsPopulationProfile() || hasPopulationEntry)
            {
                throw std::runtime_error("configured population entry did not publish population_tick");
            }
        }

        hookMap.swap(candidateHookMap);
        safeEventMap.swap(candidateSafeEventMap);
        std::swap(main_namespace, candidateNamespace);
        if (HasPopulationCapability())
        {
            std::swap(populationCallback, candidatePopulationCallback);
            populationHealthy.store(API::IsCallable(populationCallback), std::memory_order_release);
        }
        safeEventsHealthy.store(HasSafeEventCapability(), std::memory_order_release);
        LOG_INFO("module.python", ">> Published {} Python scripts from canonical root '{}' in {} ms.", count,
                 canonicalRoot.string(), GetMSTimeDiffToNow(oldMSTime));
        return true;
    }
    catch (...)
    {
        candidateRegistrationTarget = nullptr;
        candidateSafeEventRegistrationTarget = nullptr;
        LOG_ERROR("module.python", "Candidate reload failed before publication: {}", ExceptionHelper::Format());
        LOG_ERROR("module.python", "The last accepted registry and namespace were preserved; external Python "
                  "side effects are outside the transactional contract.");
        return false;
    }
}

bool PythonEngine::RunPopulationTick(uint32 diff)
{
    if (!enabled.load(std::memory_order_acquire) || !HasPopulationCapability() ||
        reloading.load(std::memory_order_acquire))
        return false;

    std::shared_lock<std::shared_mutex> lock(hookMutex);
    if (!populationCallback.ptr())
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
    GILGuard gil;
    struct TickGuard
    {
        TickGuard() { populationTickActive = true; triggerDepth++; }
        ~TickGuard() { triggerDepth--; populationTickActive = false; }
    } guard;

    bool accepted = false;
    try
    {
        accepted = boost::python::extract<bool>(populationCallback(diff));
    }
    catch (...)
    {
        LOG_ERROR("module.python", "population-v1 tick raised an exception: {}", ExceptionHelper::Format());
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
    if (!enabled.load(std::memory_order_acquire) || !HasSafeEventCapability() ||
        !IsSafeEventName(eventName) || !API::IsCallable(callback) || !candidateSafeEventRegistrationTarget)
        return false;

    try
    {
        (*candidateSafeEventRegistrationTarget)[eventName].push_back(callback);
        LOG_DEBUG("module.python", "Staged safe event '{}'", eventName);
        return true;
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Failed to stage safe event '{}'", eventName);
        LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
        return false;
    }
}

bool PythonEngine::DispatchSafeEvent(std::string const& eventName,
                                     PyEng::Events::Fields const& fields)
{
    if (!enabled.load(std::memory_order_acquire) || !HasSafeEventCapability() ||
        !IsSafeEventName(eventName) ||
        !safeEventsHealthy.load(std::memory_order_acquire) || reloading.load(std::memory_order_acquire) ||
        initializationThread != std::this_thread::get_id() || safeEventDispatchActive)
        return false;

    std::shared_lock<std::shared_mutex> lock(hookMutex);
    auto callbacks = safeEventMap.find(eventName);
    if (callbacks == safeEventMap.end())
        return true;

    GILGuard gil;
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

        for (API::Object const& callback : callbacks->second)
        {
            API::Object result = callback(payload);
            if (!API::IsNone(result))
            {
                LOG_ERROR("module.python", "Safe event '{}' returned a value; callbacks must return None.",
                          eventName);
                accepted = false;
                break;
            }
        }
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Safe event '{}' raised an exception: {}", eventName,
                  ExceptionHelper::Format());
        accepted = false;
    }

    uint32 elapsed = GetMSTimeDiffToNow(started);
    uint32 maximum = sConfigMgr->GetOption<uint32>("Python.SafeEvent.MaxMilliseconds", 25);

    if (!accepted || elapsed > maximum)
    {
        safeEventsHealthy.store(false, std::memory_order_release);
        LOG_ERROR("module.python", "Safe event '{}' disabled after result={} elapsed={}ms maximum={}ms. "
                  "In-process Python cannot be preempted while hung.",
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

    if (!IsUnsafeDevelopmentProfile())
    {
        LOG_ERROR("module.python", "Hook registration '{}' is disabled by safe-static-v1 policy.", eventName);
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

    // Registrations are accepted only into the thread-local candidate registry
    // while LoadScripts holds the exclusive hook lock. Runtime/dynamic mutation
    // of the published registry is forbidden.
    if (!candidateRegistrationTarget)
    {
        LOG_ERROR("module.python", "Hook '{}' registration attempted outside a staged script load; rejected.",
                  eventName);
        return false;
    }

    try
    {
        (*candidateRegistrationTarget)[hookId][entryId].push_back(callback);
        LOG_DEBUG("module.python", "Staged hook '{}' (entry {})", eventName, entryId);
        return true;
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Failed to register hook '{}' (entry {})", eventName, entryId);
        LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
        return false;
    }
}

ExecutionResult PythonEngine::ExecuteCode(std::string const& code)
{
    // Acquire: see initialization
    if (!enabled.load(std::memory_order_acquire))
        return ExecutionResult::Error("Python Engine is disabled");

    if (!IsUnsafeDevelopmentProfile() || !IsExecCommandAllowed())
        return ExecutionResult::Error("Python exec is available only in the explicitly acknowledged unsafe "
                                      "development profile");

    if (triggerDepth > 0)
        return ExecutionResult::Error("Cannot execute while hooks are running");

    // Try to acquire exclusive lock
    std::unique_lock<std::shared_mutex> lock(hookMutex, std::defer_lock);
    if (!lock.try_lock())
        return ExecutionResult::Error("Python engine is busy");

    GILGuard gil;

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
