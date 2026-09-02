#include "PythonEngine.h"
#include "PythonOutput.h"
#include "HookRegistry.h"
#include "Config.h"
#include "Timer.h"
#include <algorithm>
#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

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

    allowExecCommand.store(sConfigMgr->GetOption<bool>("Python.AllowExecCommand", false),
                           std::memory_order_release);

    LOG_INFO("module.python", "Initializing Python Engine...");

    try
    {
        InitAzerothCoreModule();

        // Initialize Python - main thread now HOLDS the GIL
        Py_Initialize();

        // Import modules and setup namespace while we have the GIL
        API::Object main_module = API::Import("__main__");
        main_namespace = main_module.attr("__dict__");

        // CRITICAL: After Py_Initialize(), the main thread holds the GIL. We must
        // release it to allow other threads to use PyGILState_Ensure (GILGuard).
        mainThreadState = PyEval_SaveThread();

        // Release: all initialization writes are visible when enabled is set
        enabled.store(true, std::memory_order_release);

        LOG_INFO("module.python", "Python Engine Initialized Successfully.");
    }
    catch (...)
    {
        // Relaxed: we're in error path, no synchronization needed
        enabled.store(false, std::memory_order_relaxed);

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
    allowExecCommand.store(false, std::memory_order_release);

    // Block new registrations/callbacks before restoring the initialization
    // thread state. Restoring first could deadlock with a callback that holds a
    // shared hook lock while waiting for the GIL.
    std::unique_lock<std::shared_mutex> lock(hookMutex);
    if (!mainThreadState)
    {
        LOG_ERROR("module.python", "Python Engine main thread state is unavailable; refusing unsafe finalization.");
        return;
    }

    PyEval_RestoreThread(mainThreadState);
    mainThreadState = nullptr;
    hookMap.clear();
    main_namespace = API::Object(); // Release main module ref while holding the GIL

    // WARNING: This should only be called if no other C++ static objects hold
    // Python references, as accessing them after finalization causes crashes.
    // Py_FinalizeEx destroys the GIL, so no RAII GIL guard may span this call.
    if (Py_FinalizeEx() < 0)
        LOG_ERROR("module.python", "Python Engine finalization reported an error.");

    LOG_INFO("module.python", "Python Engine shutdown complete.");
}

bool PythonEngine::LoadScripts()
{
    // Acquire: see initialization writes
    if (!enabled.load(std::memory_order_acquire))
        return false;

    uint32 oldMSTime = getMSTime();

    LOG_INFO("module.python", "Loading Python scripts...");

    std::string pathStr = sConfigMgr->GetOption<std::string>("Python.ScriptsPath", "python_scripts");
    fs::path scriptsPath(pathStr);

    if (!scriptsPath.is_absolute())
        scriptsPath = fs::current_path() / scriptsPath;

    if (!fs::exists(scriptsPath))
    {
        LOG_ERROR("module.python", "Python scripts directory not found: {}", scriptsPath.string());
        return false;
    }

    int count = 0;
    bool success = true;
    std::vector<fs::path> scriptFiles;

    for (auto const& entry : fs::recursive_directory_iterator(scriptsPath))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".py")
            scriptFiles.push_back(entry.path());
    }
    std::sort(scriptFiles.begin(), scriptFiles.end());

    {
        GILGuard gil;
        for (fs::path const& scriptFile : scriptFiles)
        {
            LOG_DEBUG("module.python", "Executing script: {}...", scriptFile.string());

            try
            {
                API::ExecFile(scriptFile.string(), main_namespace, main_namespace);
                count++;
            }
            catch (...)
            {
                success = false;
                LOG_ERROR("module.python", "Failed to execute script: {}", scriptFile.string());
                LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
            }
        }
    }

    if (!success)
    {
        ClearHooks();
        LOG_ERROR("module.python", "Python script load failed; all Python hooks were disabled.");
        return false;
    }

    LOG_INFO("module.python", ">> Loaded {} Python scripts in {} ms.", count, GetMSTimeDiffToNow(oldMSTime));
    return true;
}

void PythonEngine::ClearHooks()
{
    // Acquire exclusive lock and GIL, then clear hooks
    std::unique_lock<std::shared_mutex> lock(hookMutex);

    LOG_DEBUG("module.python", "All event handlers finished. Clearing hooks...");

    GILGuard gil;
    hookMap.clear();
}

void PythonEngine::RegisterHook(std::string const& eventName, API::Object callback, uint32 entryId)
{
    // Acquire: see initialization
    if (!enabled.load(std::memory_order_acquire) || eventName.empty())
        return;

    // Validate that the provided object is actually a function or callable object.
    // This prevents runtime crashes later when the event attempts to trigger.
    if (!PyCallable_Check(callback.ptr()))
    {
        LOG_ERROR("module.python", "Attempted to register a non-callable object for hook '{}' (entry {}).",
                  eventName, entryId);
        return;
    }

    using PyEng::Hooks::GetHookByName;
    auto hookIdOpt = GetHookByName(eventName);

    if (!hookIdOpt.has_value())
    {
        LOG_ERROR("module.python", "Attempted to register unknown event hook '{}'.", eventName);
        return;
    }

    HookInfo hookId = hookIdOpt.value();

    // If triggerDepth > 0, we are currently inside a Trigger (holding shared lock).
    // Attempting to acquire unique lock now would cause a deadlock.
    if (triggerDepth > 0)
    {
        LOG_ERROR("module.python", "CRITICAL: Attempted to register hook '{}' inside an event handler! "
                  "Dynamic registration causes deadlocks and is forbidden.", eventName);
        return;
    }

    std::unique_lock<std::shared_mutex> lock(hookMutex);

    try
    {
        hookMap[hookId][entryId].push_back(callback);
        LOG_DEBUG("module.python", "Registered hook '{}' (entry {})", eventName, entryId);
    }
    catch (...)
    {
        LOG_ERROR("module.python", "Failed to register hook '{}' (entry {})\n{}", eventName, entryId);
        LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
    }
}

ExecutionResult PythonEngine::ExecuteCode(std::string const& code)
{
    // Acquire: see initialization
    if (!enabled.load(std::memory_order_acquire))
        return ExecutionResult::Error("Python Engine is disabled");

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
