#ifndef MOD_PYTHON_ENGINE_H
#define MOD_PYTHON_ENGINE_H

#include "PythonAPI.h"
#include "PythonConverter.h"
#include "PythonGIL.h"
#include "PythonBorrowed.h"
#include <optional>
#include <type_traits>
#include "ExceptionHelper.h"
#include "ExecutionResult.h"
#include "Events/SafeEvent.h"
#include "HookInfo.h"
#include "Define.h"
#include "Log.h"
#include <atomic>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

// Forward declaration
void InitAzerothCoreModule();

using namespace PyEng::Bridge;
using namespace PyEng::Utilities;
using HookInfo = PyEng::Hooks::HookInfo;

class PythonEngine
{
    friend class PythonEngineTestAccess;
public:
    [[nodiscard]] static PythonEngine* instance();

    PythonEngine(PythonEngine const&) = delete;
    PythonEngine& operator=(PythonEngine const&) = delete;

    /**
     * @brief Initializes Python interpreter and prepares for script execution
     */
    void Initialize();

    /**
     * @brief Safely shuts down Python engine and cleans up all resources
     */
    void Shutdown();

    /**
     * @brief Loads and executes all Python scripts from the scripts directory
     */
    [[nodiscard]] bool LoadScripts();

    /**
     * @brief Registers Python callback for the game event hook
     *
     * @param eventName Hook event name (ex., "PLAYER_ON_LOGIN")
     * @param callback Python callable object
     * @param entryId Specific entry ID (creature, item, etc.; 0 = global)
     */
    [[nodiscard]] bool RegisterHook(std::string const& eventName, API::Object callback, uint32 entryId = 0);

    /**
     * @brief Executes arbitrary Python code string
     *
     * @param code Python code to execute
     * @return ExecutionResult with success status and output/error message
     */
    [[nodiscard]] ExecutionResult ExecuteCode(std::string const& code);

    /**
     * @brief Begin reload operation (thread-safe with acquire semantics)
     *
     * Uses acquire ordering to ensure all subsequent operations see consistent
     * state after the flag is set.
     *
     * @return True if reload started, false if already reloading
     */
    [[nodiscard]] bool BeginReload() noexcept
    {
        // compare_exchange_strong with acquire-release semantics:
        // - success: acquire (we see all previous writes)
        // - failure: relaxed (we're not proceeding anyway)
        bool expected = false;
        return reloading.compare_exchange_strong(expected, true,
                                                 std::memory_order_acquire, // Success: acquire semantics
                                                 std::memory_order_relaxed  // Failure: no synchronization needed
        );
    }

    /**
     * @brief End reload operation (thread-safe with release semantics)
     *
     * Uses release ordering to ensure all hook modifications are visible to
     * other threads when the flag is cleared.
     */
    void EndReload() noexcept
    {
        // Release: all previous writes (hook modifications) are visible
        // to threads that acquire this flag
        reloading.store(false, std::memory_order_release);
    }

    /**
     * @brief Check if currently reloading (with acquire semantics)
     *
     * Uses acquire ordering to ensure we see all modifications
     * that happened before reloading was set.
     *
     * @return True if reload in progress
     */
    [[nodiscard]] bool IsReloading() const noexcept
    {
        return reloading.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if engine is busy (hooks running or reloading)
     *
     * @return True if engine is busy, false otherwise
     */
    [[nodiscard]] bool IsBusy() const noexcept
    {
        return triggerDepth > 0 || reloading.load(std::memory_order_acquire);
    }

    /**
     * @brief Checks if engine is enabled (with acquire semantics)
     *
     * @return True if enabled
     */
    [[nodiscard]] bool IsEnabled() const noexcept
    {
        return enabled.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsPopulationCallAllowed() const noexcept
    {
        return populationTickActive && initializationThread == std::this_thread::get_id() && !IsReloading();
    }

    [[nodiscard]] bool IsNativeCapabilityCallAllowed() const noexcept
    {
        return IsEnabled() && initializationThread == std::this_thread::get_id();
    }

    [[nodiscard]] bool IsWorldThread() const noexcept
    {
        return initializationThread == std::this_thread::get_id();
    }
    [[nodiscard]] bool Schedule(API::Object callback);
    void RunScheduledCallbacks();

    [[nodiscard]] bool RunPopulationTick(uint32 diff);

    [[nodiscard]] bool RegisterSafeEvent(std::string const& eventName, API::Object callback);
    [[nodiscard]] bool DispatchSafeEvent(std::string const& eventName,
                                         PyEng::Events::Fields const& fields);

    template<typename... Args>
    void Trigger(HookInfo hook, uint32 entryId, Args&&... args)
    {
        if (!IsEnabled() || IsReloading() || !HasCallbacks(hook, entryId))
            return;
        GILGuard gil;
        auto callbacks = SnapshotCallbacks(hook, entryId);
        TriggerDepthGuard depth;
        for (auto const& callback : callbacks)
        {
            try
            {
                CallbackScope scope(hook.category != PyEng::Hooks::Category::SERVER || IsWorldThread());
                Call(callback, args...);
            }
            catch (...)
            {
                auto error = ExceptionHelper::Format();
                LOG_ERROR("module.python", "{}", error);
            }
        }
    }

    template<typename Result, typename... Args>
    Result TriggerResult(HookInfo hook, uint32 entryId, Result fallback, Args&&... args)
    {
        if (!IsEnabled() || IsReloading() || !HasCallbacks(hook, entryId))
            return fallback;
        GILGuard gil;
        auto callbacks = SnapshotCallbacks(hook, entryId);
        TriggerDepthGuard depth;
        Result decision = fallback;
        for (auto const& callback : callbacks)
        {
            try
            {
                CallbackScope scope(hook.category != PyEng::Hooks::Category::SERVER || IsWorldThread());
                API::Object result = Call(callback, args...);
                if (API::IsNone(result))
                    continue;
                if constexpr (std::is_same_v<Result, std::optional<bool>>)
                    decision = Converter::FromPython<bool>(result);
                else if constexpr (std::is_same_v<Result, bool>)
                {
                    bool value = Converter::FromPython<bool>(result);
                    decision = fallback ? decision && value : decision || value;
                }
                else
                    decision = Converter::FromPython<Result>(result);
            }
            catch (...)
            {
                auto error = ExceptionHelper::Format();
                LOG_ERROR("module.python", "{}", error);
            }
        }
        return decision;
    }

    static void TriggerHook(HookInfo* hinfo, uint32 entryId);

    // Passive registry observation only; the existing shared lock guards global0 plus exact entry.
    [[nodiscard]] bool HasRegisteredCallbacks(HookInfo hook, uint32 entryId) const
    {
        return HasCallbacks(hook, entryId);
    }

private:
    using CallbackList = std::vector<API::Object>;
    using HookEntryMap = std::unordered_map<uint32, CallbackList>;
    using HookRegistry = std::unordered_map<HookInfo, HookEntryMap>;
    using SafeEventRegistry = std::unordered_map<std::string, CallbackList>;

    PythonEngine() = default;
    ~PythonEngine();

    // RAII guard for tracking Trigger depth
    struct TriggerDepthGuard
    {
        TriggerDepthGuard() { PythonEngine::triggerDepth++; }
        ~TriggerDepthGuard() { PythonEngine::triggerDepth--; }
        TriggerDepthGuard(TriggerDepthGuard const&) = delete;
        TriggerDepthGuard& operator=(TriggerDepthGuard const&) = delete;
    };

    bool HasCallbacks(HookInfo hook, uint32 entryId) const
    {
        std::shared_lock<std::shared_mutex> lock(hookMutex);
        auto found = hookMap.find(hook);
        if (found == hookMap.end()) return false;
        auto has = [&](uint32 id)
        {
            auto entries = found->second.find(id);
            return entries != found->second.end() && !entries->second.empty();
        };
        return has(0) || (entryId && has(entryId));
    }

    // Called with the GIL held; release the registry lock before invoking Python.
    CallbackList SnapshotCallbacks(HookInfo hook, uint32 entryId)
    {
        std::shared_lock<std::shared_mutex> lock(hookMutex);
        CallbackList callbacks;
        if (!IsEnabled() || IsReloading())
            return callbacks;
        auto found = hookMap.find(hook);
        if (found == hookMap.end())
            return callbacks;
        auto append = [&](uint32 id)
        {
            auto entries = found->second.find(id);
            if (entries != found->second.end())
                callbacks.insert(callbacks.end(), entries->second.begin(), entries->second.end());
        };
        append(0);
        if (entryId) append(entryId);
        return callbacks;
    }

    template<typename... Args>
    static API::Object Call(API::Object const& callback, Args&&... args)
    {
        API::List values;
        (values.append(Converter::ToPython(args)), ...);
        API::Tuple tuple(values);
        PyObject* result = PyObject_CallObject(callback.ptr(), tuple.ptr());
        if (!result) boost::python::throw_error_already_set();
        return API::Object(boost::python::handle<>(result));
    }

    std::atomic<bool> enabled{false};
    std::atomic<bool> reloading{false};
    mutable std::shared_mutex hookMutex;
    inline static thread_local int triggerDepth = 0;

    HookRegistry hookMap;
    inline static thread_local HookRegistry* candidateRegistrationTarget = nullptr;
    CallbackList scheduledCallbacks;
    inline static thread_local CallbackList* candidateScheduledTarget = nullptr;
    SafeEventRegistry safeEventMap;
    inline static thread_local SafeEventRegistry* candidateSafeEventRegistrationTarget = nullptr;
    API::Object main_namespace;
    API::Object populationCallback;
    PyThreadState* mainThreadState{nullptr};
    std::thread::id initializationThread;
    std::atomic<bool> populationHealthy{false};
    std::atomic<bool> safeEventsHealthy{false};
    inline static thread_local bool populationTickActive = false;
    inline static thread_local bool safeEventDispatchActive = false;
};

extern PythonEngine* sPythonEngine;

#define sPythonEngine PythonEngine::instance()

#endif // MOD_PYTHON_ENGINE_H
