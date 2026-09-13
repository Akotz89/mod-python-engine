#ifndef MOD_PYTHON_ENGINE_H
#define MOD_PYTHON_ENGINE_H

#include "PythonAPI.h"
#include "PythonConverter.h"
#include "PythonGIL.h"
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
public:
    enum class CapabilityProfile : uint8
    {
        SafeStaticV1,
        PopulationV1,
        SafeComposedV1,
        UnsafeDevelopmentV1,
    };

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

    [[nodiscard]] bool IsExecCommandAllowed() const noexcept
    {
        return allowExecCommand.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsUnsafeDevelopmentProfile() const noexcept
    {
        return capabilityProfile.load(std::memory_order_acquire) == CapabilityProfile::UnsafeDevelopmentV1;
    }

    [[nodiscard]] bool IsPopulationProfile() const noexcept
    {
        return capabilityProfile.load(std::memory_order_acquire) == CapabilityProfile::PopulationV1;
    }

    [[nodiscard]] bool HasPopulationCapability() const noexcept
    {
        CapabilityProfile profile = capabilityProfile.load(std::memory_order_acquire);
        return profile == CapabilityProfile::PopulationV1 || profile == CapabilityProfile::SafeComposedV1;
    }

    [[nodiscard]] bool HasSafeEventCapability() const noexcept
    {
        CapabilityProfile profile = capabilityProfile.load(std::memory_order_acquire);
        return profile == CapabilityProfile::PopulationV1 || profile == CapabilityProfile::SafeComposedV1;
    }

    [[nodiscard]] bool HasPartyCapability() const noexcept
    {
        CapabilityProfile profile = capabilityProfile.load(std::memory_order_acquire);
        return profile == CapabilityProfile::PopulationV1 || profile == CapabilityProfile::SafeComposedV1;
    }

    [[nodiscard]] bool IsSafeCompositionProfile() const noexcept
    {
        return HasSafeEventCapability();
    }

    [[nodiscard]] bool IsPopulationCallAllowed() const noexcept
    {
        return HasPopulationCapability() && populationTickActive &&
               initializationThread == std::this_thread::get_id() &&
               !reloading.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool IsNativeCapabilityCallAllowed() const noexcept
    {
        return IsSafeCompositionProfile() &&
               (populationTickActive || safeEventDispatchActive) &&
               initializationThread == std::this_thread::get_id() &&
               !reloading.load(std::memory_order_acquire);
    }

    [[nodiscard]] char const* GetCapabilityProfileName() const noexcept
    {
        if (IsUnsafeDevelopmentProfile())
            return "unsafe-development-v1";
        if (capabilityProfile.load(std::memory_order_acquire) == CapabilityProfile::SafeComposedV1)
            return "safe-composed-v1";
        return IsPopulationProfile() ? "population-v1" : "safe-static-v1";
    }

    [[nodiscard]] bool RunPopulationTick(uint32 diff);

    [[nodiscard]] bool RegisterSafeEvent(std::string const& eventName, API::Object callback);
    [[nodiscard]] bool DispatchSafeEvent(std::string const& eventName,
                                         PyEng::Events::Fields const& fields);

    /**
     * @brief Main bridge between C++ game logic and Python scripts
     *
     * When a specific game event occurs (ex., player login, creature death),
     * this method looks up all registered Python callbacks and executes them.
     *
     * @param hinfo Hook identifier to trigger
     * @param entryId Specific entry ID (creature, item, etc.; 0 = global)
     * @param args Variadic arguments to pass to Python callbacks
     */
    template<typename... Args>
    void Trigger(HookInfo hinfo, uint32 entryId, Args&&... args)
    {
        // Acquire reloading flag to ensure we see hook modifications.
        // NOTE: Could use relaxed here since hookMutex provides synchronization,
        // but acquire is safer and the cost is negligible.
        // The safe profile has no callback surface. This check is defense in
        // depth in addition to omitting Register and native wrappers from the
        // safe azerothcore module.
        if (!enabled.load(std::memory_order_acquire) || !IsUnsafeDevelopmentProfile() ||
            reloading.load(std::memory_order_acquire))
            return;

        std::shared_lock<std::shared_mutex> lock(hookMutex);

        // CRITICAL: Double-check reloading and bindings after acquiring lock.
        // Prevent race conditions with reload scripts operations.
        if (reloading.load(std::memory_order_acquire) || !HasBindingsFor(hinfo, entryId))
            return;

        TriggerDepthGuard depthGuard;

        auto itr = hookMap.find(hinfo);
        if (itr == hookMap.end())
            return;

        TriggerCallbacks(itr->second, 0, std::forward<Args>(args)...);
        if (entryId > 0)
            TriggerCallbacks(itr->second, entryId, std::forward<Args>(args)...);
    }

    static void TriggerHook(HookInfo* hinfo, uint32 entryId);

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

    /**
     * @brief Checks if any callbacks are registered for specific hook
     *
     * @param hinfo Hook identifier to check
     * @param entryId Specific entry ID (creature, item, etc.; 0 = global)
     * @return True if callbacks registered, false otherwise
     */
    [[nodiscard]] bool HasBindingsFor(HookInfo hinfo, uint32 entryId = 0) const noexcept
    {
        auto itr = hookMap.find(hinfo);
        if (itr == hookMap.end())
            return false;

        auto callbacks = [&](uint32 id)
        {
            auto it = itr->second.find(id);
            return it != itr->second.end() && !it->second.empty();
        };

        return callbacks(0) || (entryId > 0 && callbacks(entryId));
    }

    /**
     * @brief Executes all registered Python callbacks for specific hook
     *
     * @param entryMap Map of entity IDs to callback vectors
     * @param entryId Specific entry ID (creature, item, etc.; 0 = global)
     * @param args Variadic arguments to pass to Python callbacks
     */
    template<typename... Args>
    void TriggerCallbacks(HookEntryMap const& entryMap, uint32 entryId, Args&&... args)
    {
        auto itr = entryMap.find(entryId);
        if (itr == entryMap.end())
            return;

        // Callback dispatch owns the GIL explicitly. Native wrappers are
        // borrowed for the synchronous call only; this path exists solely in
        // unsafe-development-v1 and retaining wrappers is unsupported. A
        // Python exception is logged and converted to no native decision.
        // There is no safe way to preempt a hung in-process callback, which is
        // why every callback (especially hot paths) is forbidden in the safe
        // profile and remains outside production acceptance.
        GILGuard gil;
        for (auto const& callback : itr->second)
        {
            try
            {
                callback(Converter::ToPython(args)...);
            }
            catch (...)
            {
                LOG_ERROR("module.python", "{}", ExceptionHelper::Format());
            }
        }
    }

    std::atomic<bool> enabled{false};
    std::atomic<bool> reloading{false};
    std::atomic<bool> allowExecCommand{false};
    std::atomic<CapabilityProfile> capabilityProfile{CapabilityProfile::SafeStaticV1};
    mutable std::shared_mutex hookMutex;
    inline static thread_local int triggerDepth = 0;

    HookRegistry hookMap;
    inline static thread_local HookRegistry* candidateRegistrationTarget = nullptr;
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
