#include "PopulationBridge.h"
#include <atomic>

namespace PyEng::Population
{
namespace
{
std::atomic<Bridge*> Provider{nullptr};
}

bool RegisterBridge(Bridge* provider) noexcept
{
    if (!provider)
        return false;
    Bridge* expected = nullptr;
    return Provider.compare_exchange_strong(expected, provider, std::memory_order_release,
                                            std::memory_order_relaxed);
}

void UnregisterBridge(Bridge* provider) noexcept
{
    Bridge* expected = provider;
    Provider.compare_exchange_strong(expected, nullptr, std::memory_order_acq_rel,
                                     std::memory_order_relaxed);
}

Bridge* GetBridge() noexcept { return Provider.load(std::memory_order_acquire); }

bool FailClosed() noexcept
{
    if (Bridge* provider = GetBridge())
        return provider->FailClosed();
    return false;
}
} // namespace PyEng::Population
