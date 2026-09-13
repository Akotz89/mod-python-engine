#ifndef MOD_PYTHON_ENGINE_PARTY_BRIDGE_H
#define MOD_PYTHON_ENGINE_PARTY_BRIDGE_H

#include "Define.h"
#include <atomic>
#include <string>
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

class Bridge
{
public:
    virtual ~Bridge() = default;

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

bool RegisterBridge(Bridge* provider) noexcept;
void UnregisterBridge(Bridge* provider) noexcept;
Bridge* GetBridge() noexcept;
} // namespace PyEng::Party

#endif // MOD_PYTHON_ENGINE_PARTY_BRIDGE_H
