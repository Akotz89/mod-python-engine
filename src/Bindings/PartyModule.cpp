#include "AutoRegistryMgr.h"
#include "PartyBridge.h"
#include "PythonEngine.h"
#include "PythonHeaders.h"
#include <stdexcept>
#include <string>
#include <vector>

using namespace PyEng::Party;

namespace
{
Bridge& Provider()
{
    if (!sPythonEngine->IsNativeCapabilityCallAllowed())
        throw std::runtime_error("party bridge calls are allowed only inside a fenced world-thread callback");

    Bridge* provider = GetBridge();
    if (!provider)
        throw std::runtime_error("the required party provider is not registered");
    return *provider;
}

boost::python::dict Member(MemberSnapshot const& value)
{
    boost::python::dict result;
    result["character_guid"] = value.characterGuid;
    result["human"] = value.human;
    result["current_master"] = value.currentMaster;
    result["controlled"] = value.controlled;
    result["alive"] = value.alive;
    result["in_combat"] = value.inCombat;
    result["map_id"] = value.mapId;
    result["zone_id"] = value.zoneId;
    return result;
}

boost::python::dict Item(InventoryItem const& value)
{
    boost::python::dict result;
    result["owner_guid"] = value.ownerGuid;
    result["item_guid"] = value.itemGuid;
    result["entry"] = value.entry;
    result["count"] = value.count;
    result["bag"] = value.bag;
    result["slot"] = value.slot;
    result["quality"] = value.quality;
    result["inventory_type"] = value.inventoryType;
    result["item_level"] = value.itemLevel;
    result["soulbound"] = value.soulbound;
    result["quest_item"] = value.questItem;
    result["key_item"] = value.keyItem;
    result["conjured"] = value.conjured;
    result["usable"] = value.usable;
    return result;
}

boost::python::dict Result(OperationResult const& value)
{
    boost::python::dict result;
    result["accepted"] = value.accepted;
    result["retryable"] = value.retryable;
    result["item_guid"] = value.itemGuid;
    result["processed_count"] = value.processedCount;
    result["code"] = value.code;
    return result;
}

std::vector<uint64> ExtractGuids(boost::python::list const& values)
{
    Py_ssize_t count = boost::python::len(values);
    if (count < 0 || count > 12)
        throw std::runtime_error("party mail batch must contain 0..12 items");

    std::vector<uint64> result;
    result.reserve(static_cast<std::size_t>(count));
    for (Py_ssize_t index = 0; index < count; ++index)
        result.push_back(boost::python::extract<uint64>(values[index]));
    return result;
}

boost::python::dict ReadCurrentParty()
{
    PartySnapshot value = Provider().ReadCurrentParty();
    boost::python::dict result;
    result["generation"] = value.generation;
    result["master_guid"] = value.masterGuid;
    result["valid"] = value.valid;

    boost::python::list members;
    for (MemberSnapshot const& member : value.members)
        members.append(Member(member));
    result["members"] = members;
    return result;
}

boost::python::list ReadInventory(uint64 memberGuid)
{
    boost::python::list result;
    for (InventoryItem const& item : Provider().ReadInventory(memberGuid))
        result.append(Item(item));
    return result;
}

boost::python::dict Equip(uint64 memberGuid, uint64 itemGuid)
{
    return Result(Provider().Equip(memberGuid, itemGuid));
}

boost::python::dict Trade(uint64 sourceGuid, uint64 targetGuid, uint64 itemGuid, uint32 count)
{
    return Result(Provider().Trade(sourceGuid, targetGuid, itemGuid, count));
}

boost::python::dict SellVendor(uint64 memberGuid, uint64 vendorGuid, uint64 itemGuid, uint32 count)
{
    return Result(Provider().SellVendor(memberGuid, vendorGuid, itemGuid, count));
}

boost::python::dict RequestItemUse(uint64 memberGuid, uint64 itemGuid)
{
    return Result(Provider().RequestItemUse(memberGuid, itemGuid));
}

boost::python::dict SendMailBatch(uint64 senderGuid, uint64 recipientGuid,
                                  boost::python::list const& itemGuids, std::string const& subject)
{
    if (subject.size() > 255)
        throw std::runtime_error("party mail subject exceeds 255 bytes");
    return Result(Provider().SendMailBatch(senderGuid, recipientGuid, ExtractGuids(itemGuids), subject));
}
} // namespace

void export_party_api()
{
    boost::python::def("party_read_current", ReadCurrentParty);
    boost::python::def("party_read_inventory", ReadInventory);
    boost::python::def("party_equip", Equip);
    boost::python::def("party_trade", Trade);
    boost::python::def("party_sell_vendor", SellVendor);
    boost::python::def("party_request_item_use", RequestItemUse);
    boost::python::def("party_send_mail_batch", SendMailBatch);
}
