#ifndef MOD_PYTHON_ENGINE_PARTY_BRIDGE_H
#define MOD_PYTHON_ENGINE_PARTY_BRIDGE_H

#include "Define.h"
#include <atomic>
#include <string>
#include <memory>
#include <thread>

class Player;
#include <vector>

namespace PyEng::Party
{
struct MemberSnapshot
{
    uint64 characterGuid{0};
    bool human{false};
    bool currentMaster{false};
    bool controlled{false};
    bool alive{false};
    bool inCombat{false};
    uint32 mapId{0};
    uint32 zoneId{0};
};

struct PartySnapshot
{
    uint64 generation{0};
    uint64 masterGuid{0};
    bool valid{false};
    std::vector<MemberSnapshot> members;
};

struct InventoryItem
{
    uint64 containerGuid{0};
    int32 randomPropertyId{0};
    uint32 suffixFactor{0};
    uint64 ownerGuid{0};
    uint64 itemGuid{0};
    uint32 entry{0};
    uint32 count{0};
    uint8 bag{0};
    uint8 slot{0};
    uint8 quality{0};
    uint8 inventoryType{0};
    uint32 itemLevel{0};
    bool soulbound{false};
    bool questItem{false};
    bool keyItem{false};
    bool conjured{false};
    bool usable{false};
};

struct OperationResult
{
    bool accepted{false};
    bool retryable{false};
    uint64 itemGuid{0};
    uint32 processedCount{0};
    std::string code;
};

// Copied native identity; Python exposes neither construction nor writable fields.
class OwnerContext
{
    struct Identity
    {
        uint64 actor, owner, group, actorEpoch, ownerEpoch, generation, dispatch;
        uint32 actorAccount, ownerAccount;
        std::thread::id thread;
    };
    std::shared_ptr<Identity const> identity;
    explicit OwnerContext(Identity value) : identity(std::make_shared<Identity const>(value)) {}
    friend class Bridge;
public:
    OwnerContext() = default;
};

enum class DecisionOperation { Attack, Heal2050 };
enum class DecisionChoice { Wait, Execute };

// The payload owns only copied native ticket data and authenticated owner identity.
class DecisionHandle
{
    struct Identity
    {
        OwnerContext owner;
        uint64 bot, target;
        DecisionOperation operation;
        std::shared_ptr<void const> ticket;
        std::shared_ptr<void const> issuer;
    };
    std::shared_ptr<Identity const> identity;
    explicit DecisionHandle(Identity value) : identity(std::make_shared<Identity const>(std::move(value))) {}
    friend class Bridge;
public:
    DecisionHandle() = default;
};
struct DecisionObservation
{
    DecisionHandle handle;
    uint64 bot = 0, target = 0;
    DecisionOperation operation = DecisionOperation::Attack;
    bool automatic = false, eligible = false, heal2050Learned = false;
    uint32 casterHealth = 0, casterMaxHealth = 0, targetHealth = 0, targetMaxHealth = 0;
    uint32 mana = 0, maxMana = 0;
};
struct DecisionContextSnapshot
{
    bool valid = false;
    uint64 recipientGuid = 0;
    uint32 health = 0, maxHealth = 0;
};
struct DecisionResult
{
    bool admitted = false, native = false;
    bool hasCastResult = false;
    uint32 castResult = 0;
    std::string reason;
};

class Bridge
{
public:
    virtual ~Bridge() = default;
    virtual OwnerContext CaptureActor(Player const* actor) = 0;
    virtual bool ValidateContext(OwnerContext const& context) = 0;
    virtual void ObserveSession(Player const* actor, bool loggedIn) = 0;

protected:
    using Identity = OwnerContext::Identity;
    static OwnerContext MakeContext(Identity value) { return OwnerContext(value); }
    static Identity const* IdentityOf(OwnerContext const& value) { return value.identity.get(); }
    static bool SameContext(OwnerContext const& left, OwnerContext const& right)
    { return left.identity && left.identity == right.identity; }
    using DecisionIdentity = DecisionHandle::Identity;
    static DecisionHandle MakeDecisionHandle(DecisionIdentity value) { return DecisionHandle(std::move(value)); }
    static DecisionIdentity const* IdentityOf(DecisionHandle const& value) { return value.identity.get(); }
public:

    virtual DecisionObservation ObserveDecision(uint64 botGuid, DecisionOperation operation, uint64 targetGuid) = 0;
    virtual DecisionResult SubmitDecision(DecisionHandle const& handle, DecisionChoice choice) = 0;
    virtual DecisionContextSnapshot SnapshotDecisionContext(DecisionHandle const& handle) = 0;
    virtual PartySnapshot ReadCurrentParty() = 0;
    virtual std::vector<InventoryItem> ReadInventory(uint64 memberGuid) = 0;
    virtual OperationResult Equip(uint64 memberGuid, uint64 itemGuid) = 0;
    virtual OperationResult Trade(uint64 sourceGuid, uint64 targetGuid, uint64 itemGuid,
                                 uint32 count) = 0;
    virtual OperationResult SellVendor(uint64 memberGuid, uint64 vendorGuid, uint64 itemGuid,
                                       uint32 count) = 0;
    virtual OperationResult RequestItemUse(uint64 memberGuid, uint64 itemGuid) = 0;
    virtual OperationResult SendMailBatch(uint64 senderGuid, uint64 recipientGuid,
                                          std::vector<uint64> const& itemGuids,
                                          std::string const& subject) = 0;
};

bool CanCallNative() noexcept;
uint64 ContextGeneration() noexcept;
void InvalidateContexts() noexcept;
OwnerContext CaptureContext();
OwnerContext const& CurrentContext() noexcept;
void ObserveSession(Player const* actor, bool loggedIn);
class ContextScope
{
    OwnerContext previous;
public:
    explicit ContextScope(OwnerContext const& context);
    ContextScope(); // Clear ambient authority at actorless native entry points.
    ~ContextScope();
    ContextScope(ContextScope const&) = delete;
    ContextScope& operator=(ContextScope const&) = delete;
};
class NativeActorScope
{
    ContextScope scope;
public:
    explicit NativeActorScope(Player const* actor);
};
bool RegisterBridge(Bridge* provider) noexcept;
void UnregisterBridge(Bridge* provider) noexcept;
Bridge* GetBridge() noexcept;
} // namespace PyEng::Party

#endif // MOD_PYTHON_ENGINE_PARTY_BRIDGE_H
