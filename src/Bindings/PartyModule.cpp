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
        throw std::runtime_error("party bridge calls require the enabled native world thread");

    Bridge* provider = GetBridge();
    if (!provider)
        throw std::runtime_error("the required party provider is not registered");
    return *provider;
}

void RejectDecisionWrite(DecisionHandle const&, boost::python::object const&, boost::python::object const&)
{
    PyErr_SetString(PyExc_AttributeError, "native decision handles are immutable");
    boost::python::throw_error_already_set();
}
void RejectDecisionDelete(DecisionHandle const&, boost::python::object const&)
{
    PyErr_SetString(PyExc_AttributeError, "native decision handles are immutable");
    boost::python::throw_error_already_set();
}
uint64 DecisionGuid(boost::python::object const& value)
{
    if (PyBool_Check(value.ptr()) || !PyLong_Check(value.ptr()))
        throw std::runtime_error("decision GUID must be an unsigned 64-bit integer");
    auto result = PyLong_AsUnsignedLongLong(value.ptr());
    if (PyErr_Occurred()) boost::python::throw_error_already_set();
    if (!result) throw std::runtime_error("decision GUID must be nonzero");
    return result;
}
std::string DecisionWord(boost::python::object const& value)
{
    if (!PyUnicode_Check(value.ptr())) throw std::runtime_error("decision operation/choice must be a string");
    return boost::python::extract<std::string>(value);
}
boost::python::dict ObserveDecision(boost::python::object bot, boost::python::object operation,
                                    boost::python::object target)
{
    auto word = DecisionWord(operation);
    if (word != "attack" && word != "heal2050") throw std::runtime_error("unknown decision operation");
    auto value = Provider().ObserveDecision(DecisionGuid(bot), word == "attack" ? DecisionOperation::Attack :
                                            DecisionOperation::Heal2050, DecisionGuid(target));
    boost::python::dict result;
    result["handle"] = value.handle; result["bot_guid"] = value.bot; result["target_guid"] = value.target;
    result["operation"] = word; result["ttl_ms"] = 500;
    result["automatic"] = value.automatic; result["eligible"] = value.eligible; result["heal2050_learned"] = value.heal2050Learned;
    result["caster_health"] = value.casterHealth; result["caster_max_health"] = value.casterMaxHealth;
    result["target_health"] = value.targetHealth; result["target_max_health"] = value.targetMaxHealth;
    result["mana"] = value.mana; result["max_mana"] = value.maxMana;
    return result;
}
boost::python::dict SubmitDecision(DecisionHandle const& handle, boost::python::object choice)
{
    auto word = DecisionWord(choice);
    if (word != "wait" && word != "execute") throw std::runtime_error("unknown decision choice");
    auto value = Provider().SubmitDecision(handle, word == "wait" ? DecisionChoice::Wait : DecisionChoice::Execute);
    boost::python::dict result;
    result["admitted"] = value.admitted; result["native"] = value.native; result["reason"] = value.reason;
    result["effect"] = "unknown";
    result["cast_result"] = value.hasCastResult ? boost::python::object(value.castResult) : boost::python::object();
    return result;
}

boost::python::dict SnapshotDecisionContext(DecisionHandle const& handle)
{
    auto value = Provider().SnapshotDecisionContext(handle);
    boost::python::dict result;
    result["valid"] = value.valid;
    result["recipient_guid"] = value.valid ? boost::python::object(value.recipientGuid) : boost::python::object();
    result["health"] = value.valid ? boost::python::object(value.health) : boost::python::object();
    result["max_health"] = value.valid ? boost::python::object(value.maxHealth) : boost::python::object();
    return result;
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
    result["container_guid"] = value.containerGuid;
    result["random_property_id"] = value.randomPropertyId;
    result["suffix_factor"] = value.suffixFactor;
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

boost::python::object WithContext(OwnerContext const& context, boost::python::object callback)
{
    if (!PyCallable_Check(callback.ptr()))
        throw std::runtime_error("party context callback must be callable");
    ContextScope scope(context);
    return callback();
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
    if (subject.size() > 255 || subject.find('\0') != std::string::npos)
        throw std::runtime_error("party mail subject exceeds 255 bytes or contains NUL");
    return Result(Provider().SendMailBatch(senderGuid, recipientGuid, ExtractGuids(itemGuids), subject));
}
} // namespace

void export_party_api()
{
    boost::python::class_<OwnerContext>("PartyOwnerContext", boost::python::no_init);
    boost::python::class_<DecisionHandle>("PartyDecisionHandle", boost::python::no_init)
        .def("__setattr__", RejectDecisionWrite)
        .def("__delattr__", RejectDecisionDelete);
    boost::python::def("party_decision_observe", ObserveDecision);
    boost::python::def("party_decision_submit", SubmitDecision);
    boost::python::def("party_decision_context_snapshot", SnapshotDecisionContext);
    boost::python::def("party_capture_context", CaptureContext);
    boost::python::def("party_with_context", WithContext);
    boost::python::def("party_read_current", ReadCurrentParty);
    boost::python::def("party_read_inventory", ReadInventory);
    boost::python::def("party_equip", Equip);
    boost::python::def("party_trade", Trade);
    boost::python::def("party_sell_vendor", SellVendor);
    boost::python::def("party_request_item_use", RequestItemUse);
    boost::python::def("party_send_mail_batch", SendMailBatch);
}
