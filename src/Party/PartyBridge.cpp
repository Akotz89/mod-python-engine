#include "PartyBridge.h"
#include "PythonEngine.h"
#include <stdexcept>

namespace PyEng::Party
{
namespace
{
std::atomic<Bridge*> Provider{nullptr};
std::atomic<uint64> Generation{1};
thread_local OwnerContext Ambient;
OwnerContext ActorContext(Player const* actor)
{
    if (!sPythonEngine->IsNativeCapabilityCallAllowed())
        return {};
    Bridge* bridge = GetBridge();
    return bridge ? bridge->CaptureActor(actor) : OwnerContext{};
}
}

bool CanCallNative() noexcept { return sPythonEngine->IsNativeCapabilityCallAllowed() && !sPythonEngine->IsReloading(); }
uint64 ContextGeneration() noexcept { return Generation.load(std::memory_order_acquire); }
void InvalidateContexts() noexcept { Generation.fetch_add(1, std::memory_order_acq_rel); }
OwnerContext const& CurrentContext() noexcept { return Ambient; }
OwnerContext CaptureContext()
{
    Bridge* bridge = GetBridge();
    if (!CanCallNative() || !bridge || !bridge->ValidateContext(Ambient))
        throw std::runtime_error("no valid native party owner context");
    return Ambient;
}
ContextScope::ContextScope() : previous(Ambient) { Ambient = {}; }
ContextScope::ContextScope(OwnerContext const& context) : previous(Ambient)
{
    Bridge* bridge = GetBridge();
    if (!CanCallNative() || !bridge || !bridge->ValidateContext(context))
        throw std::runtime_error("expired or invalid native party owner context");
    Ambient = context;
}
ContextScope::~ContextScope() { Ambient = previous; }
NativeActorScope::NativeActorScope(Player const* actor) : scope()
{
    try { Ambient = ActorContext(actor); }
    catch (...) { Ambient = {}; }
}
void ObserveSession(Player const* actor, bool loggedIn)
{
    if (sPythonEngine->IsWorldThread())
        if (Bridge* bridge = GetBridge())
            try { bridge->ObserveSession(actor, loggedIn); }
            catch (...) { InvalidateContexts(); }
}

bool RegisterBridge(Bridge* provider) noexcept
{
    if (!provider)
        return false;

    Bridge* expected = nullptr;
    if (!Provider.compare_exchange_strong(expected, provider, std::memory_order_release,
                                          std::memory_order_relaxed)) return false;
    InvalidateContexts();
    return true;
}

void UnregisterBridge(Bridge* provider) noexcept
{
    Bridge* expected = provider;
    if (Provider.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel,
                                        std::memory_order_relaxed)) InvalidateContexts();
}

Bridge* GetBridge() noexcept { return Provider.load(std::memory_order_acquire); }
} // namespace PyEng::Party
