#include "PythonEngine.h"
#include "PartyBridge.h"

bool PythonEngine::Schedule(API::Object callback)
{
    if (!IsEnabled() || !API::IsCallable(callback) || (IsReloading() && !candidateScheduledTarget))
        return false;
    if (candidateScheduledTarget)
        candidateScheduledTarget->push_back(callback);
    else
    {
        std::unique_lock<std::shared_mutex> lock(hookMutex);
        scheduledCallbacks.push_back(callback);
    }
    return true;
}

void PythonEngine::RunScheduledCallbacks()
{
    PyEng::Party::ContextScope partyScope;
    if (!IsEnabled() || IsReloading() || !IsWorldThread()) return;
    GILGuard gil;
    CallbackList callbacks;
    {
        std::unique_lock<std::shared_mutex> lock(hookMutex);
        callbacks.swap(scheduledCallbacks);
    }
    TriggerDepthGuard depth;
    for (API::Object const& callback : callbacks)
    {
        try
        {
            CallbackScope scope;
            callback();
        }
        catch (...)
        {
            auto error = ExceptionHelper::Format();
            LOG_ERROR("module.python", "Scheduled callback raised: {}", error);
        }
    }
}
