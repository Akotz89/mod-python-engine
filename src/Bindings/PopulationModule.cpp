#include "AutoRegistryMgr.h"
#include "PopulationBridge.h"
#include "PythonEngine.h"
#include "Hooks/HookRegistry.h"
#include "PythonHeaders.h"
#include "Cryptography/CryptoHash.h"
#include <stdexcept>
#include <string>
#include <vector>
#include <limits>
#include <initializer_list>
#include <sstream>
#include <locale>
#include <unordered_set>
#include <set>

using namespace PyEng::Population;

namespace
{
Bridge& Provider()
{
    if (!sPythonEngine->IsPopulationCallAllowed())
        throw std::runtime_error("population bridge calls are allowed only inside the fenced world-thread tick");
    Bridge* provider = GetBridge();
    if (!provider)
        throw std::runtime_error("the required population provider is not registered");
    return *provider;
}

std::vector<std::string> Strings(boost::python::list const& values)
{
    Py_ssize_t count = boost::python::len(values);
    if (count < 0 || count > 256)
        throw std::runtime_error("population bridge batch is outside 0..256");
    std::vector<std::string> result;
    result.reserve(static_cast<std::size_t>(count));
    for (Py_ssize_t index = 0; index < count; ++index)
        result.push_back(boost::python::extract<std::string>(values[index]));
    return result;
}

boost::python::dict ReadQuestSnapshot(std::string const& identityKey, uint32 observedQuestId)
{
    StrategicSnapshot value = Provider().ReadQuestSnapshot(identityKey, observedQuestId);
    boost::python::dict result;
    result["identity_key"] = value.identityKey;
    result["account_id"] = value.accountId;
    result["character_guid"] = value.characterGuid;
    result["online"] = value.online;
    result["control_allowed"] = value.controlAllowed;
    result["reason"] = value.reason;
    result["current_goal_quest_id"] = value.currentGoalQuestId;
    result["current_goal_acquiring"] = value.currentGoalAcquiring;
    result["observed_quest_id"] = value.observedQuestId;
    result["observed_quest_status"] = value.observedQuestStatus;
    result["level"] = value.level;
    result["xp"] = value.xp;
    boost::python::list quests;
    for (StrategicQuest const& quest : value.quests)
    {
        boost::python::dict row;
        row["quest_id"] = quest.questId;
        row["status"] = quest.status;
        row["eligible"] = quest.eligible;
        row["rejection_reason"] = quest.rejectionReason;
        quests.append(row);
    }
    result["quests"] = quests;
    return result;
}

boost::python::dict GearItemResult(GearItem const& value)
{
    boost::python::dict result;
    result["entry"] = value.entry;
    result["instance_guid"] = value.instanceGuid;
    result["count"] = value.count;
    result["random_property_id"] = value.randomPropertyId;
    result["random_property_known"] = value.randomPropertyKnown;
    return result;
}

boost::python::dict GearRewardResult(GearReward const& reward)
{
    boost::python::dict item;
    item["kind"] = reward.kind;
    item["index"] = reward.index;
    item["item"] = GearItemResult(reward.item);
    item["requirement_result"] = reward.requirementResult;
    item["requirement_reason"] = reward.requirementReason;
    item["classification_allowed"] = reward.classificationAllowed;
    item["classification_reason"] = reward.classificationReason;
    item["reason"] = reward.reason;
    boost::python::list slots;
    for (GearSlotComparison const& comparison : reward.slots)
    {
        boost::python::dict slot;
        slot["slot"] = comparison.slot;
        slot["current"] = GearItemResult(comparison.current);
        slot["candidate_score_valid"] = comparison.candidateScoreValid;
        slot["candidate_score"] = comparison.candidateScore;
        slot["candidate_score_reason"] = comparison.candidateScoreReason;
        slot["current_score_valid"] = comparison.currentScoreValid;
        slot["current_score"] = comparison.currentScore;
        slot["current_score_reason"] = comparison.currentScoreReason;
        slots.append(slot);
    }
    item["slots"] = slots;
    return item;
}

boost::python::dict ReadGearSnapshot(std::string const& identityKey, uint32 observedQuestId)
{
    GearSnapshot value = Provider().ReadGearSnapshot(identityKey, observedQuestId);
    boost::python::dict result;
    result["identity_key"] = value.identityKey;
    result["account_id"] = value.accountId;
    result["character_guid"] = value.characterGuid;
    result["online"] = value.online;
    result["control_allowed"] = value.controlAllowed;
    result["reason"] = value.reason;
    result["level"] = value.level;
    result["class_id"] = value.classId;
    result["spec_tab"] = value.specTab;
    result["session_incarnation"] = value.sessionIncarnation;
    result["strategic_generation"] = value.strategicGeneration;
    result["role"] = value.role;
    result["map_id"] = value.mapId;
    result["instance_id"] = value.instanceId;
    result["score_mode"] = value.scoreMode;
    result["requirement_scope"] = value.requirementScope;
    result["slot_scope"] = value.slotScope;
    result["script_acceptance_evaluated"] = value.scriptAcceptanceEvaluated;
    result["final_equip_evaluated"] = value.finalEquipEvaluated;
    boost::python::list equipment;
    for (GearEquipment const& equipped : value.equipment)
    {
        boost::python::dict row;
        row["slot"] = equipped.slot;
        row["item"] = GearItemResult(equipped.item);
        equipment.append(row);
    }
    result["equipment"] = equipment;
    boost::python::list quests;
    for (GearQuest const& quest : value.quests)
    {
        boost::python::dict row;
        row["giver_entry"] = quest.giverEntry;
        row["giver_map"] = quest.giverMap;
        row["giver_x"] = quest.giverX;
        row["giver_y"] = quest.giverY;
        row["giver_z"] = quest.giverZ;
        row["quest_id"] = quest.questId;
        row["status"] = quest.status;
        row["reason"] = quest.reason;
        boost::python::list rewards;
        for (GearReward const& reward : quest.rewards)
        {
            boost::python::dict item = GearRewardResult(reward);
            rewards.append(item);
        }
        row["rewards"] = rewards;
        quests.append(row);
    }
    result["quests"] = quests;
    return result;
}

std::vector<uint32> ResourceIds(boost::python::list const& values, Py_ssize_t maximum)
{
    Py_ssize_t count = boost::python::len(values);
    if (count < 0 || count > maximum)
        throw std::runtime_error("gear resource query exceeds its ID bound");
    std::vector<uint32> result;
    result.reserve(static_cast<std::size_t>(count));
    std::unordered_set<uint32> unique;
    for (Py_ssize_t index = 0; index < count; ++index)
    {
        boost::python::object value = values[index];
        if (!PyLong_Check(value.ptr()) || PyBool_Check(value.ptr()))
            throw std::runtime_error("gear resource IDs must be positive uint32 integers");
        unsigned long long parsed = PyLong_AsUnsignedLongLong(value.ptr());
        if (PyErr_Occurred())
            boost::python::throw_error_already_set();
        if (!parsed || parsed > std::numeric_limits<uint32>::max() ||
            !unique.insert(static_cast<uint32>(parsed)).second)
            throw std::runtime_error("invalid or duplicate gear resource ID");
        result.push_back(static_cast<uint32>(parsed));
    }
    return result;
}

boost::python::list ReadVendorOffers(std::string const& identityKey, boost::python::list const& itemIds)
{
    auto values = Provider().ReadVendorOffers(identityKey, ResourceIds(itemIds, 64));
    boost::python::list result;
    for (auto const& value : values)
    {
        boost::python::dict row;
        row["request_id"] = value.requestId; row["source_id"] = value.sourceId;
        row["vendor_guid"] = value.vendorGuid; row["vendor_entry"] = value.vendorEntry;
        row["vendor_slot"] = value.vendorSlot; row["price_copper"] = value.priceCopper;
        row["stock"] = value.stock; row["money_copper"] = value.moneyCopper;
        row["reason"] = value.reason; row["reward"] = GearRewardResult(value.reward);
        result.append(row);
    }
    return result;
}

boost::python::dict BuyVendorItem(std::string const& identityKey, std::string const& requestId,
    std::string const& sourceId, uint32 item, uint8 equipmentSlot)
{
    VendorPurchaseResult value = Provider().BuyVendorItem({identityKey, requestId, sourceId, item, equipmentSlot});
    boost::python::dict row;
    row["request_id"] = value.requestId; row["status"] = value.status;
    row["reason"] = value.reason; row["stage"] = value.stage; row["item"] = value.item;
    row["price_copper"] = value.priceCopper; row["money_before"] = value.moneyBefore;
    row["money_after"] = value.moneyAfter; row["item_before"] = value.itemBefore;
    row["item_after"] = value.itemAfter; row["item_guid"] = value.itemGuid;
    row["observed_money_after"] = value.observedMoneyAfter; row["observed_item_after"] = value.observedItemAfter;
    row["observation_valid"] = value.observationValid;
    return row;
}

boost::python::dict ReadQuestAcquisitionSnapshot(std::string const& identityKey, uint32 questId,
    std::string const& kind, uint32 index, uint32 item)
{
    auto value = Provider().ReadQuestAcquisitionSnapshot(identityKey, questId, kind, index, item);
    boost::python::dict result;
    result["schema"] = "core1-native-quest-acquisition-v1";
    result["identity_key"] = value.identityKey;
    result["account_id"] = value.accountId;
    result["character_guid"] = value.characterGuid;
    result["strategic_generation"] = value.strategicGeneration;
    result["phase_digest"] = value.phaseDigest;
    result["valid"] = value.valid;
    result["reason"] = value.reason;
    boost::python::list attempts;
    for (auto const& attempt : value.attempts)
    {
        boost::python::dict row;
        row["record_id"] = attempt.recordId;
        row["identity_key"] = attempt.identityKey;
        row["account_id"] = attempt.accountId;
        row["character_guid"] = attempt.characterGuid;
        row["session_incarnation"] = attempt.sessionIncarnation;
        row["strategic_generation"] = attempt.strategicGeneration;
        row["goal_revision"] = attempt.goalRevision;
        row["phase_digest"] = attempt.phaseDigest;
        row["source_key"] = attempt.sourceKey;
        row["quest_id"] = attempt.questId;
        row["reward_kind"] = attempt.rewardKind;
        row["reward_index"] = attempt.rewardIndex;
        row["item_entry"] = attempt.itemEntry;
        row["reward_count"] = attempt.rewardCount;
        row["monotonic_frequency"] = attempt.monotonicFrequency;
        row["valid_window"] = attempt.validWindow;
        row["terminal"] = attempt.terminal;
        row["action_terminal"] = attempt.actionTerminal;
        row["pending_deliveries"] = attempt.pendingDeliveries;
        row["current_context_matches"] = attempt.currentContextMatches;
        row["record_sha256"] = attempt.recordSha256;
        row["record_digest_version"] = "core1-native-acquisition-record-digest-v2";
        row["observer_failure"] = attempt.observerFailure;
        row["post_action_context_lost"] = attempt.postActionContextLost;
        row["turn_in_flight"] = attempt.turnInFlight; row["complete_quest_observed"] = attempt.completeQuestObserved;
        row["moving"] = attempt.moving;
        row["valid_attempt"] = attempt.validAttempt;
        row["terminal_outcome"] = attempt.terminalOutcome;
        row["movement_coverage"] = attempt.movementCoverage;
        row["death_coverage"] = attempt.deathCoverage;
        row["failure_terminal_coverage"] = attempt.failureTerminalCoverage;
        row["route_coverage"] = attempt.routeCoverage;
        row["complete_destination_attribution"] = attempt.completeDestinationAttribution;
        row["movement_scope"] = "native_OnPlayerUpdate_isMoving_transition_intervals";
        row["route_scope"] = "ordinary_quest_MoveFarTo_and_MoveRandomNear_final_decisions_and_indexed_giver_fullness_eligibility";
        row["predicate_scope"] = "actor_session_goal_phase_only;revalidate_before_action;not_universal";
        row["reward_receipt_scope"] = "ordinary_exact_reward_slot_inventory_destination_pre_post_readback;mail_requires_linked_committed_receipt";
        boost::python::list events, predicates;
        for (auto const& event : attempt.events)
        {
            boost::python::dict e;
            e["event_sequence"] = event.eventSequence;
            e["monotonic_ticks"] = event.monotonicTicks;
            e["event_kind"] = event.eventKind;
            e["reason"] = event.reason;
            e["map_id"] = event.mapId;
            e["target_map"] = event.targetMap;
            e["target_x"] = event.targetX;
            e["target_y"] = event.targetY;
            e["target_z"] = event.targetZ;
            e["outcome"] = event.outcome;
            e["item_guid"] = event.itemGuid;
            e["item_entry"] = event.itemEntry;
            e["count"] = event.count;
            e["operation_id"] = event.operationId; e["measurement_scope"] = event.measurementScope;
            events.append(e);
        }
        for (auto const& predicate : attempt.predicates)
        {
            boost::python::dict p;
            p["name"] = predicate.name;
            p["scope"] = predicate.scope;
            p["reason"] = predicate.reason;
            p["event_sequence"] = predicate.eventSequence;
            p["monotonic_ticks"] = predicate.monotonicTicks;
            p["evaluated"] = predicate.evaluated;
            p["result"] = predicate.result;
            predicates.append(p);
        }
        boost::python::list receipts;
        for (auto const& receipt : attempt.rewardReceipts)
        {
            boost::python::dict value;
            value["event_sequence"] = receipt.eventSequence;
            value["monotonic_ticks"] = receipt.monotonicTicks;
            value["quest_id"] = receipt.questId;
            value["reward_kind"] = receipt.rewardKind;
            value["reward_index"] = receipt.rewardIndex;
            value["item_entry"] = receipt.itemEntry;
            value["count"] = receipt.count;
            value["count_sum"] = receipt.countSum;
            value["complete"] = receipt.complete;
            value["delivery_reason"] = receipt.deliveryReason;
            value["record_id"] = receipt.recordId; value["operation_id"] = receipt.operationId;
            value["delivery_pending"] = receipt.deliveryPending; value["delivery_known"] = receipt.deliveryKnown;
            value["transaction_committed"] = receipt.transactionCommitted; value["queued_complete"] = receipt.queuedComplete;
            boost::python::list destinations;
            for (auto const& destination : receipt.destinations)
            {
                boost::python::dict d;
                d["bag_slot"] = destination.bagSlot;
                d["planned_count"] = destination.plannedCount;
                d["before_guid"] = destination.beforeGuid;
                d["before_entry"] = destination.beforeEntry;
                d["before_count"] = destination.beforeCount;
                d["item_guid"] = destination.itemGuid;
                d["item_entry"] = destination.itemEntry;
                d["after_count"] = destination.afterCount;
                d["delta"] = destination.delta;
                d["matched"] = destination.matched;
                destinations.append(d);
            }
            value["destinations"] = destinations;
            boost::python::list envelopes;
            for (auto const& envelope : receipt.mailEnvelopes)
            {
                boost::python::dict m; boost::python::list attachments;
                m["mail_id"] = envelope.mailId; m["receiver_guid"] = envelope.receiverGuid;
                m["deliver_time"] = envelope.deliverTime; m["queued"] = envelope.queued;
                for (auto const& item : envelope.attachments)
                {
                    boost::python::dict a;
                    a["item_guid"] = item.itemGuid; a["item_entry"] = item.itemEntry; a["count"] = item.count;
                    a["saved_entry"] = item.savedEntry; a["saved_count"] = item.savedCount;
                    a["saved_value_matched"] = item.savedValueMatched; attachments.append(a);
                }
                m["attachments"] = attachments; envelopes.append(m);
            }
            value["mail_envelopes"] = envelopes; receipts.append(value);
        }
        row["reward_receipts"] = receipts;
        row["events"] = events; row["predicates"] = predicates; attempts.append(row);
    }
    result["attempts"] = attempts;
    return result;
}

boost::python::list AcknowledgeQuestAcquisition(std::string const& identityKey, boost::python::list const& rows)
{
    std::vector<AcquisitionAcknowledgement> proofs;
    for (Py_ssize_t index = 0; index < boost::python::len(rows); ++index)
    {
        boost::python::dict row = boost::python::extract<boost::python::dict>(rows[index]);
        if (boost::python::len(row) != 10) throw std::runtime_error("acquisition ACK requires exact original custody fields");
        for (auto const& key : {"record_id", "account_id", "character_guid", "session_incarnation", "strategic_generation", "goal_revision"})
        {
            boost::python::object value = row[key];
            if (!PyLong_Check(value.ptr()) || PyBool_Check(value.ptr()))
                throw std::runtime_error("acquisition ACK original binding IDs must be unsigned integers");
        }
        AcquisitionAcknowledgement proof;
        proof.recordId = boost::python::extract<uint64>(row["record_id"]);
        proof.accountId = boost::python::extract<uint32>(row["account_id"]);
        proof.characterGuid = boost::python::extract<uint64>(row["character_guid"]);
        proof.sessionIncarnation = boost::python::extract<uint64>(row["session_incarnation"]);
        proof.strategicGeneration = boost::python::extract<uint64>(row["strategic_generation"]);
        proof.goalRevision = boost::python::extract<uint64>(row["goal_revision"]);
        proof.phaseDigest = boost::python::extract<std::string>(row["phase_digest"]);
        proof.sourceKey = boost::python::extract<std::string>(row["source_key"]);
        proof.recordSha256 = boost::python::extract<std::string>(row["record_sha256"]);
        proof.durableSha256 = boost::python::extract<std::string>(row["durable_sha256"]);
        proofs.push_back(std::move(proof));
    }
    boost::python::list result;
    for (auto const& value : Provider().AcknowledgeQuestAcquisition(identityKey, proofs))
    {
        boost::python::dict row;
        row["record_id"] = value.recordId; row["status"] = value.status; row["reason"] = value.reason;
        row["record_sha256"] = value.recordSha256; row["durable_sha256"] = value.durableSha256;
        row["recovered_closed_context"] = value.recoveredClosedContext; result.append(row);
    }
    return result;
}


boost::python::dict ExactPredicateObject(boost::python::object const& value, std::initializer_list<char const*> keys)
{
    if (!PyDict_Check(value.ptr())) throw std::runtime_error("source predicate requires an object");
    boost::python::dict row = boost::python::extract<boost::python::dict>(value);
    if (boost::python::len(row) != Py_ssize_t(keys.size())) throw std::runtime_error("source predicate object has extra or missing keys");
    for (auto key : keys) if (!row.has_key(key)) throw std::runtime_error("source predicate object has missing key");
    return row;
}
std::string PredicateString(boost::python::object const& value)
{
    if (!PyUnicode_Check(value.ptr())) throw std::runtime_error("source predicate requires string");
    std::string result = boost::python::extract<std::string>(value);
    if (result.empty()) throw std::runtime_error("source predicate string is empty");
    return result;
}
template<class T> T PredicateInteger(boost::python::object const& value)
{
    if (!PyLong_Check(value.ptr()) || PyBool_Check(value.ptr())) throw std::runtime_error("source predicate requires integer");
    return boost::python::extract<T>(value);
}
boost::python::dict SetQuestAcquisitionCapture(std::string const& identityKey, boost::python::dict const& input)
{
    auto row = ExactPredicateObject(input, {"enabled", "capture_id", "account_id", "character_guid", "session_incarnation",
        "strategic_generation", "phase_digest", "quest_id", "reward_kind", "reward_index", "item_entry", "reward_count"});
    boost::python::object enabled = row["enabled"];
    if (!PyBool_Check(enabled.ptr())) throw std::runtime_error("capture enabled must be boolean");
    QuestAcquisitionCaptureRequest request;
    request.enabled = boost::python::extract<bool>(enabled);
    request.captureId = PredicateString(row["capture_id"]); request.phaseDigest = PredicateString(row["phase_digest"]);
    request.rewardKind = PredicateString(row["reward_kind"]);
    request.accountId = PredicateInteger<uint32>(row["account_id"]); request.characterGuid = PredicateInteger<uint64>(row["character_guid"]);
    request.sessionIncarnation = PredicateInteger<uint64>(row["session_incarnation"]); request.strategicGeneration = PredicateInteger<uint64>(row["strategic_generation"]);
    request.questId = PredicateInteger<uint32>(row["quest_id"]); request.rewardIndex = PredicateInteger<uint32>(row["reward_index"]);
    request.itemEntry = PredicateInteger<uint32>(row["item_entry"]); request.rewardCount = PredicateInteger<uint32>(row["reward_count"]);
    auto value = Provider().SetQuestAcquisitionCapture(identityKey, request);
    boost::python::dict result;
    result["status"] = value.status; result["reason"] = value.reason; result["capture_id"] = value.captureId;
    result["record_id"] = value.recordId; result["session_incarnation"] = value.sessionIncarnation;
    result["strategic_generation"] = value.strategicGeneration;
    return result;
}
boost::python::dict PredicateReference(std::string const& kind, uint32 id)
{
    boost::python::dict result; result["kind"] = kind; result["id"] = id; return result;
}
boost::python::object PredicateReward(bool present, std::string const& kind, uint32 index, uint32 item)
{
    if (!present) return boost::python::object();
    boost::python::dict result; result["kind"] = kind; result["index"] = index; result["item_id"] = item; return result;
}
boost::python::dict ReadSourcePredicates(std::string const& identityKey, boost::python::dict const& input)
{
    auto request = ExactPredicateObject(input, {"schema", "source_binding", "actor", "phase", "evaluator_id", "predicates", "query_digest"});
    if (PredicateString(request["schema"]) != "cpi211-source-predicate-query-v1" || PredicateString(request["evaluator_id"]) != "core1-source-predicates-v1")
        throw std::runtime_error("unsupported source predicate schema/evaluator");
    SourcePredicateQuery query;
    auto binding = ExactPredicateObject(request["source_binding"], {"candidate_id", "generation", "graph_digest", "manifest_digest", "projection_digest"});
    query.candidateId = PredicateString(binding["candidate_id"]); query.generation = PredicateString(binding["generation"]);
    query.graphDigest = PredicateString(binding["graph_digest"]); query.manifestDigest = PredicateString(binding["manifest_digest"]); query.projectionDigest = PredicateString(binding["projection_digest"]);
    auto actor = ExactPredicateObject(request["actor"], {"identity_key", "account_id", "character_guid", "session_incarnation", "strategic_generation"});
    query.identityKey = PredicateString(actor["identity_key"]); query.accountId = PredicateInteger<uint32>(actor["account_id"]);
    query.characterGuid = PredicateInteger<uint64>(actor["character_guid"]); query.sessionIncarnation = PredicateInteger<uint64>(actor["session_incarnation"]); query.strategicGeneration = PredicateInteger<uint64>(actor["strategic_generation"]);
    auto phase = ExactPredicateObject(request["phase"], {"phase_id", "policy_digest", "planner_phase"});
    query.phaseId = PredicateString(phase["phase_id"]); query.policyDigest = PredicateString(phase["policy_digest"]); query.plannerPhase = PredicateInteger<uint32>(phase["planner_phase"]);
    query.queryDigest = PredicateString(request["query_digest"]);
    boost::python::object rows = request["predicates"];
    if (!PyList_Check(rows.ptr())) throw std::runtime_error("source predicates must be an array");
    for (Py_ssize_t i = 0; i < boost::python::len(rows); ++i)
    {
        auto row = ExactPredicateObject(rows[i], {"predicate_id", "source_id", "source_ref", "item_id", "reward", "locus", "condition_id", "ast_path", "definition_sha256", "definition"});
        SourcePredicateRow value;
        value.predicateId = PredicateString(row["predicate_id"]); value.sourceId = PredicateString(row["source_id"]); value.itemId = PredicateInteger<uint32>(row["item_id"]);
        value.conditionId = PredicateString(row["condition_id"]); value.astPath = PredicateString(row["ast_path"]); value.definitionSha256 = PredicateString(row["definition_sha256"]);
        auto source = ExactPredicateObject(row["source_ref"], {"kind", "id"}); value.sourceKind = PredicateString(source["kind"]); value.sourceEntry = PredicateInteger<uint32>(source["id"]);
        auto locus = ExactPredicateObject(row["locus"], {"kind", "owner_ref"}); value.locusKind = PredicateString(locus["kind"]);
        auto owner = ExactPredicateObject(locus["owner_ref"], {"kind", "id"}); value.ownerKind = PredicateString(owner["kind"]); value.ownerEntry = PredicateInteger<uint32>(owner["id"]);
        boost::python::object rewardObject = row["reward"];
        if (!rewardObject.is_none())
        {
            auto reward = ExactPredicateObject(rewardObject, {"kind", "index", "item_id"}); value.hasReward = true;
            value.rewardKind = PredicateString(reward["kind"]); value.rewardIndex = PredicateInteger<uint32>(reward["index"]); value.rewardItem = PredicateInteger<uint32>(reward["item_id"]);
        }
        auto definition = ExactPredicateObject(row["definition"], {"op", "condition_type", "values"});
        if (PredicateString(definition["op"]) != "leaf") throw std::runtime_error("only exact native leaf definitions are supported");
        value.definition.type = PredicateInteger<uint32>(definition["condition_type"]);
        boost::python::object values = definition["values"];
        if (!PyList_Check(values.ptr()) || boost::python::len(values) != 3) throw std::runtime_error("predicate values must be exact signed64 triple");
        for (uint32 index = 0; index < 3; ++index) value.definition.values[index] = PredicateInteger<int64>(values[index]);
        query.predicates.push_back(std::move(value));
    }
    auto value = Provider().ReadSourcePredicates(identityKey, query);
    // Passive registry facts are part of the semantic witness; no Python callback is invoked.
    std::ostringstream registry; registry.imbue(std::locale::classic());
    auto fact = [&](auto const& input) { std::ostringstream text; text.imbue(std::locale::classic()); text << input; auto bytes = text.str(); registry << bytes.size() << ':' << bytes; };
    fact("core1-source-predicates-passive-registry-v1"); fact(value.observedStateGeneration);
    fact(sPythonEngine->IsEnabled()); fact(sPythonEngine->IsReloading());
    std::set<uint32> quests;
    for (auto const& predicate : query.predicates) if (predicate.sourceKind == "quest") quests.insert(predicate.sourceEntry);
    for (auto const& name : {"PLAYER_CAN_ACCEPT_QUEST", "PLAYER_CAN_PROGRESS_QUEST_OBJECTIVE", "PLAYER_CAN_REWARD_QUEST", "PLAYER_ON_BEFORE_QUEST_COMPLETE"})
    {
        auto hook = PyEng::Hooks::GetHookByName(name); fact(name); fact(bool(hook));
        fact(hook && sPythonEngine->HasRegisteredCallbacks(*hook, 0));
        for (auto quest : quests) { fact(quest); fact(hook && sPythonEngine->HasRegisteredCallbacks(*hook, quest)); }
    }
    if (!sPythonEngine->IsEnabled() || sPythonEngine->IsReloading())
    {
        value.valid = false; value.reason = "callback_registry_context_changed";
        for (auto& verdict : value.verdicts) { verdict.verdict = "unknown"; verdict.reason = value.reason; }
    }
    fact(value.valid);
    auto digest = Acore::Crypto::SHA256::GetDigestOf(registry.str());
    value.observedStateGeneration.clear();
    for (auto byte : digest) { value.observedStateGeneration += "0123456789abcdef"[byte >> 4]; value.observedStateGeneration += "0123456789abcdef"[byte & 15]; }
    boost::python::dict result, resultBinding, sourceBinding, resultActor, resultPhase;
    auto const& current = value.binding;
    sourceBinding["candidate_id"] = current.candidateId; sourceBinding["generation"] = current.generation; sourceBinding["graph_digest"] = current.graphDigest;
    sourceBinding["manifest_digest"] = current.manifestDigest; sourceBinding["projection_digest"] = current.projectionDigest;
    resultActor["identity_key"] = current.identityKey; resultActor["account_id"] = current.accountId; resultActor["character_guid"] = current.characterGuid;
    resultActor["session_incarnation"] = current.sessionIncarnation; resultActor["strategic_generation"] = current.strategicGeneration;
    resultPhase["phase_id"] = current.phaseId; resultPhase["policy_digest"] = current.policyDigest; resultPhase["planner_phase"] = current.plannerPhase;
    resultBinding["source_binding"] = sourceBinding; resultBinding["actor"] = resultActor; resultBinding["phase"] = resultPhase; resultBinding["evaluator_id"] = "core1-source-predicates-v1";
    result["schema"] = "cpi211-source-predicate-context-v1"; result["valid"] = value.valid; result["reason"] = value.reason;
    result["binding"] = resultBinding; result["observed_state_generation"] = value.observedStateGeneration; result["query_digest"] = current.queryDigest;
    boost::python::list verdicts;
    for (auto const& verdict : value.verdicts)
    {
        boost::python::dict row; row["predicate_id"] = verdict.predicateId; row["definition_sha256"] = verdict.definitionSha256;
        row["verdict"] = verdict.verdict; row["reason"] = verdict.reason;
        if (verdict.verdict != "unknown")
        {
            boost::python::dict definition, source; boost::python::list values;
            definition["op"] = "leaf"; definition["condition_type"] = verdict.matchedDefinition.type;
            for (auto number : verdict.matchedDefinition.values) values.append(number);
            definition["values"] = values; row["matched_definition"] = definition;
            source["source_ref"] = PredicateReference(verdict.sourceKind, verdict.sourceEntry); source["item_id"] = verdict.itemId;
            source["reward"] = PredicateReward(verdict.hasReward, verdict.rewardKind, verdict.rewardIndex, verdict.rewardItem);
            source["locus_owner_ref"] = PredicateReference(verdict.ownerKind, verdict.ownerEntry); row["matched_source"] = source;
        }
        verdicts.append(row);
    }
    result["verdicts"] = verdicts; return result;
}

boost::python::dict ReadAcquisitionCatalog()
{
    // Public templates need the existing native world-thread fence, not an actor tick/admission.
    if (!sPythonEngine->IsNativeCapabilityCallAllowed() || sPythonEngine->IsReloading())
        throw std::runtime_error("acquisition catalogue requires the enabled native world thread outside reload");
    Bridge* provider = GetBridge();
    if (!provider)
        throw std::runtime_error("the required population provider is not registered");
    AcquisitionCatalog catalog = provider->ReadAcquisitionCatalog();
    boost::python::dict result, phase;
    result["schema"] = "core1-loaded-acquisition-catalog-v1";
    result["valid"] = catalog.valid; result["reason"] = catalog.reason;
    result["observation_generation"] = catalog.observationGeneration;
    phase["enabled"] = catalog.phase.enabled; phase["policy_valid"] = catalog.phase.policyValid;
    phase["phase_id"] = catalog.phase.phaseId; phase["level_ceiling"] = catalog.phase.levelCeiling;
    phase["policy_digest"] = catalog.phase.policyDigest; result["phase"] = phase;
    boost::python::list items, quests;
    for (auto const& value : catalog.items)
    {
        boost::python::dict row;
        row["entry"] = value.entry; row["class"] = value.itemClass; row["subclass"] = value.subclass;
        row["inventory_type"] = value.inventoryType; row["item_level"] = value.itemLevel;
        row["required_level"] = value.requiredLevel; row["allowable_class"] = value.allowableClass;
        row["allowable_race"] = value.allowableRace; row["required_skill"] = value.requiredSkill;
        row["required_skill_rank"] = value.requiredSkillRank; row["name"] = value.name;
        row["random_property"] = value.randomProperty; row["random_suffix"] = value.randomSuffix;
        items.append(row);
    }
    for (auto const& value : catalog.quests)
    {
        boost::python::dict row;
        row["entry"] = value.entry; row["minimum_level"] = value.minimumLevel;
        row["maximum_level"] = value.maximumLevel; row["required_classes"] = value.requiredClasses;
        row["allowable_races"] = value.allowableRaces; row["flags"] = value.flags; row["type"] = value.type;
        row["required_skill"] = value.requiredSkill;
        row["required_skill_rank"] = value.requiredSkillRank;
        row["previous_quest_id"] = value.previousQuestId;
        row["next_quest_id"] = value.nextQuestId;
        row["next_quest_in_chain"] = value.nextQuestInChain;
        row["exclusive_group"] = value.exclusiveGroup;
        row["signed_money"] = value.signedMoney;
        row["rep_objective_faction"] = value.repObjectiveFaction; row["rep_objective_value"] = value.repObjectiveValue;
        row["rep_objective_faction2"] = value.repObjectiveFaction2; row["rep_objective_value2"] = value.repObjectiveValue2;
        row["required_player_kills"] = value.requiredPlayerKills;
        row["timed"] = value.timed; row["exploration_or_event"] = value.explorationOrEvent; row["cast_objective"] = value.castObjective;
        boost::python::list breadcrumbs; for (auto id : value.breadcrumbDependents) breadcrumbs.append(id); row["breadcrumb_dependents"] = breadcrumbs;
        row["minimum_rep_faction"] = value.minimumRepFaction;
        row["minimum_rep_value"] = value.minimumRepValue;
        row["maximum_rep_faction"] = value.maximumRepFaction;
        row["maximum_rep_value"] = value.maximumRepValue;
        row["source_item"] = value.sourceItem;
        row["source_item_count"] = value.sourceItemCount;
        row["source_spell"] = value.sourceSpell;
        row["time_allowed"] = value.timeAllowed;
        row["event_id"] = value.eventId;
        row["breadcrumb_quest"] = value.breadcrumbQuest;
        row["reward_mail_template"] = value.rewardMailTemplate;
        row["repeatable"] = value.repeatable;
        row["daily"] = value.daily;
        row["weekly"] = value.weekly;
        row["monthly"] = value.monthly;
        row["seasonal"] = value.seasonal;
        row["dungeon_finder"] = value.dungeonFinder;
        row["predicate_coverage"] = value.predicateCoverage;
        row["source_rule_version"] = value.sourceRuleVersion;
        row["quest_disabled"] = value.questDisabled;
        boost::python::dict phasePolicy;
        phasePolicy["known"] = value.phaseTemplateKnown; phasePolicy["configured"] = value.phaseGateConfigured;
        phasePolicy["ready"] = value.phaseGateReady; phasePolicy["template_allowed"] = value.phaseTemplateAllowed;
        phasePolicy["scope"] = "actual_shared_phase_gate_static_template_predicate;catalogue_phase_digest_and_generation;revalidate_at_native_action;not_actor_CanTake_truth";
        row["phase_template_policy"] = phasePolicy;
        row["native_quest_level"] = value.nativeQuestLevel; row["native_suggested_players"] = value.nativeSuggestedPlayers;
        row["native_low_level_hide_diff"] = value.nativeLowLevelHideDiff;
        row["giver_source_complete"] = value.giverSourceComplete;
        boost::python::list missing, entries, itemGivers, questPools, questEvents, spawns, requiredItems, sourceItems, targets;
        for (auto const& reason : value.giverSourceMissing) missing.append(reason);
        for (auto entry : value.giverEntries) entries.append(entry);
        for (auto id : value.itemGivers) itemGivers.append(id);
        for (auto id : value.questPoolIds) questPools.append(id);
        for (auto id : value.questEventIds) questEvents.append(id);
        for (auto const& spawn : value.giverSpawns)
        {
            boost::python::dict g; boost::python::list eventIds, poolIds;
            g["entry"] = spawn.entry; g["guid"] = spawn.guid; g["map_id"] = spawn.mapId;
            g["phase_mask"] = spawn.phaseMask; g["spawn_mask"] = spawn.spawnMask;
            g["spawn_group_id"] = spawn.spawnGroupId; g["spawn_group_flags"] = spawn.spawnGroupFlags;
            g["script_id"] = spawn.scriptId; g["multispawn"] = spawn.multispawn;
            for (auto id : spawn.eventIds) eventIds.append(id);
            for (auto id : spawn.poolIds) poolIds.append(id);
            g["event_ids"] = eventIds; g["pool_ids"] = poolIds; spawns.append(g);
        }
        auto objectiveItems = [](auto const& rows, boost::python::list& output)
        {
            for (auto const& item : rows) { boost::python::dict d; d["index"] = item.index; d["item"] = item.item; d["count"] = item.count; output.append(d); }
        };
        objectiveItems(value.requiredItems, requiredItems); objectiveItems(value.requiredSourceItems, sourceItems);
        for (auto const& target : value.requiredTargets) { boost::python::dict d; d["index"] = target.index; d["entry"] = target.entry; d["count"] = target.count; targets.append(d); }
        row["giver_source_missing"] = missing; row["giver_entries"] = entries; row["item_givers"] = itemGivers;
        row["quest_pool_ids"] = questPools; row["quest_event_ids"] = questEvents; row["giver_spawns"] = spawns;
        row["required_items"] = requiredItems; row["required_source_items"] = sourceItems; row["required_targets"] = targets;
        boost::python::list previousQuests; for (auto id : value.previousQuests) previousQuests.append(id); row["previous_quests"] = previousQuests;
        boost::python::list previousChainQuests; for (auto id : value.previousChainQuests) previousChainQuests.append(id); row["previous_chain_quests"] = previousChainQuests;
        boost::python::list exclusiveQuests; for (auto id : value.exclusiveQuests) exclusiveQuests.append(id); row["exclusive_quests"] = exclusiveQuests;
        boost::python::list conditions;
        for (auto const& c : value.conditions)
        {
            boost::python::dict condition;
            condition["source_type"] = c.sourceType;
            condition["source_group"] = c.sourceGroup;
            condition["source_entry"] = c.sourceEntry;
            condition["source_id"] = c.sourceId;
            condition["else_group"] = c.elseGroup;
            condition["condition_type"] = c.type;
            condition["value1"] = c.value1;
            condition["value2"] = c.value2;
            condition["value3"] = c.value3;
            condition["reference_id"] = c.referenceId;
            condition["script_id"] = c.scriptId;
            condition["target"] = c.target;
            condition["negative"] = c.negative;
            conditions.append(condition);
        }
        row["conditions"] = conditions;
        boost::python::list giverScripts;
        for (auto const& giver : value.giverScripts)
        {
            boost::python::dict g;
            g["entry"] = giver.entry; g["relation"] = giver.relation; g["script_id"] = giver.scriptId;
            g["ai_name"] = giver.aiName; g["registered_script_name"] = giver.registeredScriptName;
            g["template_known"] = giver.templateKnown; g["registry_resolved"] = giver.registryResolved; giverScripts.append(g);
        }
        row["giver_scripts"] = giverScripts;
        boost::python::list callbackFacts;
        for (auto const& name : {"PLAYER_CAN_ACCEPT_QUEST", "PLAYER_CAN_PROGRESS_QUEST_OBJECTIVE", "PLAYER_CAN_REWARD_QUEST", "PLAYER_ON_BEFORE_QUEST_COMPLETE"})
        {
            boost::python::dict fact;
            auto hook = PyEng::Hooks::GetHookByName(name);
            fact["hook_name"] = name; fact["quest_id"] = value.entry; fact["native_dispatch_entry"] = 0;
            fact["registry_known"] = bool(hook); fact["engine_enabled"] = sPythonEngine->IsEnabled();
            fact["reloading"] = sPythonEngine->IsReloading();
            fact["has_global"] = hook && sPythonEngine->HasRegisteredCallbacks(*hook, 0);
            fact["has_global_or_quest_entry"] = hook && sPythonEngine->HasRegisteredCallbacks(*hook, value.entry);
            fact["scope"] = "passive_global0_and_exact_entry_registry_observation;current_player_macro_dispatches_global0;revalidate_at_action";
            callbackFacts.append(fact);
        }
        row["python_predicate_callbacks"] = callbackFacts;
        boost::python::list rewards;
        for (auto const& rewardValue : value.rewards)
        {
            boost::python::dict reward;
            reward["kind"] = rewardValue.kind; reward["index"] = rewardValue.index;
            reward["item"] = rewardValue.item; reward["count"] = rewardValue.count; rewards.append(reward);
        }
        row["rewards"] = rewards; quests.append(row);
    }
    boost::python::list dispatches;
    for (auto const& dispatch : catalog.playerHookDispatch)
    {
        boost::python::dict d; boost::python::list names;
        d["hook_id"] = dispatch.hookId; d["hook_name"] = dispatch.hookName;
        for (auto const& name : dispatch.scriptNames) names.append(name);
        d["script_names"] = names;
        d["scope"] = "actual_enabled_dispatch;may_inherit_default_body;not_actor_result_or_universal_predicate";
        dispatches.append(d);
    }
    result["player_hook_dispatch"] = dispatches;
    result["player_hook_registry_initialized"] = catalog.playerHookRegistryInitialized;
    if (!sPythonEngine->IsEnabled() || sPythonEngine->IsReloading())
    {
        result["valid"] = false; result["reason"] = "callback_observation_engine_context_changed";
    }
    result["items"] = items; result["quests"] = quests;
    result["item_count"] = catalog.items.size(); result["quest_count"] = catalog.quests.size();
    std::size_t rewardCount = 0;
    for (auto const& quest : catalog.quests) rewardCount += quest.rewards.size();
    result["reward_count"] = rewardCount;
    return result;
}

boost::python::dict ReadGearResources(std::string const& identityKey,
    boost::python::list const& skillIds, boost::python::list const& itemIds,
    boost::python::list const& questIds)
{
    // Validate all bounded requests before Provider() or any native identity query.
    auto skills = ResourceIds(skillIds, 64);
    auto items = ResourceIds(itemIds, 256);
    auto quests = ResourceIds(questIds, 256);
    GearResources value = Provider().ReadGearResources(identityKey, skills, items, quests);
    boost::python::dict result;
    result["identity_key"] = value.identityKey;
    result["account_id"] = value.accountId;
    result["character_guid"] = value.characterGuid;
    result["online"] = value.online;
    result["control_allowed"] = value.controlAllowed;
    result["reason"] = value.reason;
    result["map_id"] = value.mapId;
    result["zone_id"] = value.zoneId;
    result["level"] = value.level;
    result["money_copper"] = value.moneyCopper;
    boost::python::dict professions, counts;
    for (auto const& entry : value.professionSkillValues)
        professions[entry.first] = entry.second;
    for (auto const& entry : value.itemCounts)
        counts[entry.first] = entry.second;
    result["profession_skill_values"] = professions;
    result["item_counts"] = counts;
    boost::python::list rewarded;
    for (uint32 id : value.rewardedQuestIds)
        rewarded.append(id);
    result["rewarded_quest_ids"] = rewarded;
    return result;
}

boost::python::dict SetQuestGoal(std::string const& identityKey, uint32 questId,
    std::string const& rewardKind, uint32 rewardIndex, uint32 rewardItem)
{
    StrategicGoalResult value = Provider().SetQuestGoal(identityKey, questId, rewardKind, rewardIndex, rewardItem);
    boost::python::dict result;
    result["status"] = value.status;
    result["reason"] = value.reason;
    result["quest_id"] = value.questId;
    return result;
}

boost::python::list RenewQuestGoals(boost::python::list const& rows)
{
    Py_ssize_t count = boost::python::len(rows);
    if (count < 0 || count > 256)
        throw std::runtime_error("quest renewal batch is outside 0..256");
    std::vector<StrategicGoalRenewal> goals;
    goals.reserve(static_cast<std::size_t>(count));
    for (Py_ssize_t index = 0; index < count; ++index)
    {
        boost::python::dict row = boost::python::extract<boost::python::dict>(rows[index]);
        boost::python::object quest = row["quest_id"];
        if (boost::python::len(row) != 2 || !PyLong_Check(quest.ptr()) || PyBool_Check(quest.ptr()))
            throw std::runtime_error("invalid quest renewal shape");
        goals.push_back({boost::python::extract<std::string>(row["identity_key"]),
                         boost::python::extract<uint32>(quest)});
    }
    boost::python::list result;
    for (StrategicGoalResult const& value : Provider().RenewQuestGoals(goals))
    {
        boost::python::dict row;
        row["status"] = value.status;
        row["reason"] = value.reason;
        row["quest_id"] = value.questId;
        result.append(row);
    }
    return result;
}

boost::python::dict ReadOwnerHighWater()
{
    OwnerHighWater value = Provider().ReadOwnerHighWater();
    boost::python::dict result;
    result["character_guid"] = value.characterGuid;
    result["level"] = value.level;
    result["authenticated"] = value.authenticated;
    result["observed_epoch"] = value.observedEpoch;
    return result;
}

boost::python::list ReadEarnedSnapshot(boost::python::list const& identityKeys)
{
    boost::python::list result;
    for (EarnedProgress const& value : Provider().ReadEarnedSnapshot(Strings(identityKeys)))
    {
        boost::python::dict row;
        row["identity_key"] = value.identityKey;
        row["character_guid"] = value.characterGuid;
        row["level"] = value.level;
        row["xp"] = value.xp;
        row["played_seconds"] = value.playedSeconds;
        row["map_id"] = value.mapId;
        row["zone_id"] = value.zoneId;
        row["session_requested"] = value.sessionRequested;
        row["online"] = value.online;
        result.append(row);
    }
    return result;
}

bool ReserveIdentity(std::string const& identityKey, std::string const& accountKey,
                     std::string const& guidAllocationKey, std::string const& waveKey)
{
    return Provider().ReserveIdentity(identityKey, accountKey, guidAllocationKey, waveKey);
}

boost::python::dict CreationResult(uint32 id)
{
    boost::python::dict result;
    result["id"] = id;
    result["pending"] = false;
    return result;
}

boost::python::dict EnsureAccount(std::string const& accountKey, std::string const& accountName,
                                  uint8 expectedManifestSlots)
{
    return CreationResult(Provider().EnsureAccount(accountKey, accountName, expectedManifestSlots));
}

boost::python::dict EnsureCharacter(std::string const& identityKey, uint32 accountId,
                                    boost::python::dict const& raw)
{
    boost::python::dict appearance = boost::python::extract<boost::python::dict>(raw["appearance"]);
    CharacterCreationFields fields;
    fields.characterName = boost::python::extract<std::string>(raw["character_name"]);
    fields.raceId = boost::python::extract<uint8>(raw["race_id"]);
    fields.classId = boost::python::extract<uint8>(raw["class_id"]);
    fields.initialLevel = boost::python::extract<uint8>(raw["initial_level"]);
    fields.gender = boost::python::extract<uint8>(appearance["gender"]);
    fields.skin = boost::python::extract<uint8>(appearance["skin"]);
    fields.face = boost::python::extract<uint8>(appearance["face"]);
    fields.hairStyle = boost::python::extract<uint8>(appearance["hair_style"]);
    fields.hairColor = boost::python::extract<uint8>(appearance["hair_color"]);
    fields.facialHair = boost::python::extract<uint8>(appearance["facial_hair"]);
    return CreationResult(Provider().EnsureCharacter(identityKey, accountId, fields));
}

boost::python::dict BindIdentityOnce(std::string const& identityKey, uint32 accountId,
                                    uint32 characterGuid, std::string const& manifestDigest)
{
    IdentityBinding value = Provider().BindIdentityOnce(identityKey, accountId, characterGuid,
                                                         manifestDigest);
    boost::python::dict result;
    result["identity_key"] = value.identityKey;
    result["account_id"] = value.accountId;
    result["character_guid"] = value.characterGuid;
    return result;
}

bool SetAdmission(std::string const& identityKey, std::string const& waveKey,
                  std::string const& manifestDigest)
{
    return Provider().SetAdmission(identityKey, waveKey, manifestDigest);
}

bool ApplyProgressionPolicy(boost::python::list const& rows)
{
    Py_ssize_t count = boost::python::len(rows);
    if (count < 0 || count > 256)
        throw std::runtime_error("progression policy batch is outside 0..256");
    std::vector<ProgressionPolicy> policies;
    policies.reserve(static_cast<std::size_t>(count));
    for (Py_ssize_t index = 0; index < count; ++index)
    {
        boost::python::dict row = boost::python::extract<boost::python::dict>(rows[index]);
        boost::python::object held = row["held"];
        boost::python::object ceiling = row["effective_ceiling"];
        bool const hasScheduler = row.has_key("scheduler_eligible");
        bool schedulerEligible = false;
        if (hasScheduler)
        {
            boost::python::object scheduler = row["scheduler_eligible"];
            if (!PyBool_Check(scheduler.ptr()))
                throw std::runtime_error("invalid scheduler eligibility");
            schedulerEligible = boost::python::extract<bool>(scheduler);
        }
        if (boost::python::len(row) != (hasScheduler ? 4 : 3) || !PyBool_Check(held.ptr()) ||
            !PyLong_Check(ceiling.ptr()) || PyBool_Check(ceiling.ptr()))
            throw std::runtime_error("invalid progression policy shape");
        int rawCeiling = boost::python::extract<int>(ceiling);
        if (rawCeiling < 1 || rawCeiling > 80)
            throw std::runtime_error("progression ceiling is outside 1..80");
        policies.push_back({boost::python::extract<std::string>(row["identity_key"]),
                            static_cast<uint8>(rawCeiling),
                            boost::python::extract<bool>(held), schedulerEligible});
    }
    return Provider().ApplyProgressionPolicy(policies);
}

bool RequestSession(std::string const& identityKey, uint64 capacityGeneration)
{
    return Provider().RequestSession(identityKey, capacityGeneration);
}

bool ReleaseSession(std::string const& identityKey)
{
    return Provider().ReleaseSession(identityKey);
}

boost::python::dict ReadCapacitySnapshot()
{
    CapacitySnapshot value = Provider().ReadCapacitySnapshot();
    boost::python::dict result;
    result["generation"] = value.generation;
    result["online_limit"] = value.onlineLimit;
    result["available_sessions"] = value.availableSessions;
    result["valid"] = value.valid;
    result["observed_epoch"] = value.observedEpoch;
    return result;
}

boost::python::dict ReadPhaseSnapshot()
{
    PhaseSnapshot value = Provider().ReadPhaseSnapshot();
    boost::python::dict result;
    result["enabled"] = value.enabled;
    result["policy_valid"] = value.policyValid;
    result["phase_id"] = value.phaseId;
    result["level_ceiling"] = value.levelCeiling;
    result["policy_digest"] = value.policyDigest;
    return result;
}

bool FailClosedBinding() { return Provider().FailClosed(); }
} // namespace

void export_population_api()
{
    boost::python::def("strategic_set_quest_acquisition_capture", SetQuestAcquisitionCapture,
        (boost::python::arg("identity_key"), boost::python::arg("request")));
    boost::python::def("strategic_read_quest_acquisition_snapshot", ReadQuestAcquisitionSnapshot,
        (boost::python::arg("identity_key"), boost::python::arg("quest_id"), boost::python::arg("reward_kind"),
         boost::python::arg("reward_index"), boost::python::arg("item_entry")));
    boost::python::def("strategic_ack_quest_acquisition_records", AcknowledgeQuestAcquisition,
        (boost::python::arg("identity_key"), boost::python::arg("acknowledgements")));
    boost::python::def("strategic_read_acquisition_catalog", ReadAcquisitionCatalog);
    boost::python::def("strategic_read_vendor_offers", ReadVendorOffers);
    boost::python::def("strategic_buy_vendor_item", BuyVendorItem);
    boost::python::def("strategic_read_quest_snapshot", ReadQuestSnapshot,
                       (boost::python::arg("identity_key"),
                        boost::python::arg("observed_quest_id") = 0));
    boost::python::def("strategic_read_source_predicates", ReadSourcePredicates, (boost::python::arg("identity_key"), boost::python::arg("query")));
    boost::python::def("strategic_read_gear_snapshot", ReadGearSnapshot,
                       (boost::python::arg("identity_key"),
                        boost::python::arg("observed_quest_id") = 0));
    boost::python::def("strategic_read_gear_resources", ReadGearResources,
        (boost::python::arg("identity_key"), boost::python::arg("skill_ids"),
         boost::python::arg("item_ids"), boost::python::arg("quest_ids")));
    boost::python::def("strategic_set_quest_goal", SetQuestGoal,
        (boost::python::arg("identity_key"), boost::python::arg("quest_id"),
         boost::python::arg("reward_kind") = "", boost::python::arg("reward_index") = 0,
         boost::python::arg("reward_item") = 0));
    boost::python::def("strategic_renew_quest_goals", RenewQuestGoals);
    boost::python::def("population_read_owner_high_water", ReadOwnerHighWater);
    boost::python::def("population_read_earned_snapshot", ReadEarnedSnapshot);
    boost::python::def("population_reserve_identity", ReserveIdentity);
    boost::python::def("population_ensure_account", EnsureAccount);
    boost::python::def("population_ensure_character", EnsureCharacter);
    boost::python::def("population_bind_identity_once", BindIdentityOnce);
    boost::python::def("population_set_admission", SetAdmission);
    boost::python::def("population_apply_progression_policy", ApplyProgressionPolicy);
    boost::python::def("population_request_session", RequestSession);
    boost::python::def("population_release_session", ReleaseSession);
    boost::python::def("population_read_capacity_snapshot", ReadCapacitySnapshot);
    boost::python::def("population_read_phase_snapshot", ReadPhaseSnapshot);
    boost::python::def("population_fail_closed", FailClosedBinding);
}
