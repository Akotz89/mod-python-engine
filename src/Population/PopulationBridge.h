#ifndef MOD_PYTHON_ENGINE_POPULATION_BRIDGE_H
#define MOD_PYTHON_ENGINE_POPULATION_BRIDGE_H

#include "Define.h"
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace PyEng::Population
{
struct OwnerHighWater
{
    uint32 characterGuid{0};
    uint8 level{0};
    bool authenticated{false};
    uint64 observedEpoch{0};
};

struct EarnedProgress
{
    std::string identityKey;
    uint32 characterGuid{0};
    uint8 level{0};
    uint32 xp{0};
    uint32 playedSeconds{0};
    uint32 mapId{0};
    uint32 zoneId{0};
    bool sessionRequested{false};
    bool online{false};
};

struct ProgressionPolicy
{
    std::string identityKey;
    uint8 effectiveCeiling{0};
    bool held{true};
    bool schedulerEligible{false};
};

struct IdentityBinding
{
    std::string identityKey;
    uint32 accountId{0};
    uint32 characterGuid{0};
};

struct CharacterCreationFields
{
    std::string characterName;
    uint8 raceId{0};
    uint8 classId{0};
    uint8 initialLevel{0};
    uint8 gender{0};
    uint8 skin{0};
    uint8 face{0};
    uint8 hairStyle{0};
    uint8 hairColor{0};
    uint8 facialHair{0};
};

struct CapacitySnapshot
{
    uint64 generation{0};
    uint32 onlineLimit{0};
    uint32 availableSessions{0};
    bool valid{false};
    uint64 observedEpoch{0};
};

struct PhaseSnapshot
{
    bool enabled{false};
    bool policyValid{false};
    std::string phaseId;
    uint8 levelCeiling{0};
    std::string policyDigest;
};

struct StrategicQuest
{
    uint32 questId{0};
    std::string status;
    bool eligible{false};
    std::string rejectionReason;
};

struct StrategicSnapshot
{
    std::string identityKey;
    uint32 accountId{0};
    uint32 characterGuid{0};
    bool online{false};
    bool controlAllowed{false};
    std::string reason;
    uint32 currentGoalQuestId{0};
    bool currentGoalAcquiring{false};
    uint32 observedQuestId{0};
    std::string observedQuestStatus{"none"};
    uint8 level{0};
    uint32 xp{0};
    std::vector<StrategicQuest> quests;
};

// All observation rows own their values; native objects never cross this boundary.
struct GearItem
{
    uint32 entry{0};
    uint64 instanceGuid{0};
    uint32 count{0};
    int32 randomPropertyId{0};
    bool randomPropertyKnown{false};
};

struct GearEquipment
{
    uint8 slot{0};
    GearItem item;
};

struct GearSlotComparison
{
    uint8 slot{0};
    GearItem current;
    bool candidateScoreValid{false};
    float candidateScore{0};
    bool currentScoreValid{false};
    float currentScore{0};
    std::string candidateScoreReason;
    std::string currentScoreReason;
};

struct GearReward
{
    std::string kind;
    uint8 index{0};
    GearItem item;
    uint32 requirementResult{0};
    std::string requirementReason;
    bool classificationAllowed{false};
    std::string classificationReason;
    std::string reason;
    std::vector<GearSlotComparison> slots;
};

struct GearQuest
{
    uint32 questId{0};
    int32 giverEntry{0};
    uint32 giverMap{0};
    float giverX{0}, giverY{0}, giverZ{0};
    std::string status;
    std::string reason;
    std::vector<GearReward> rewards;
};

struct GearSnapshot
{
    uint64 sessionIncarnation{0}, strategicGeneration{0}, goalRevision{0};
    std::string identityKey;
    uint32 accountId{0};
    uint32 characterGuid{0};
    bool online{false};
    bool controlAllowed{false};
    std::string reason;
    uint8 level{0};
    uint8 classId{0};
    int32 specTab{0};
    std::string role;
    uint32 mapId{0};
    uint32 instanceId{0};
    std::string scoreMode{"native_slot_aware_template_random_property_socket_approximation"};
    std::string requirementScope{"template_requirements_without_script"};
    std::string slotScope{"native_base_class_without_script"};
    bool scriptAcceptanceEvaluated{false};
    bool finalEquipEvaluated{false};
    std::vector<GearEquipment> equipment;
    std::vector<GearQuest> quests;
};

// Copied resource values for the requested native skill/item/quest IDs only.
struct GearResources
{
    std::string identityKey;
    uint32 accountId{0};
    uint32 characterGuid{0};
    bool online{false};
    bool controlAllowed{false};
    std::string reason;
    uint32 mapId{0};
    uint32 zoneId{0};
    uint8 level{0};
    uint32 moneyCopper{0};
    std::unordered_map<uint32, uint32> professionSkillValues;
    std::unordered_map<uint32, uint32> itemCounts;
    std::vector<uint32> rewardedQuestIds;
};

struct GearVendorOffer
{
    std::string requestId, sourceId, reason, phaseDigest, witnessId;
    uint64 vendorGuid{0}, session{0}, generation{0}, goalRevision{0};
    // vendorSlot is the compacted loaded-list index used by the dynamic quote;
    // sourceSlot is the original npc_vendor DB slot and is never an index.
    uint32 vendorEntry{0}, vendorSlot{0}, sourceSlot{0}, priceCopper{0}, stock{0}, moneyCopper{0};
    bool sourceSlotKnown{false};
    uint32 accountId{0}, characterGuid{0}, maxCount{0}, incrTime{0};
    uint64 vendorListToken{0}, itemTemplateToken{0};
    uint8 level{0}, classId{0};
    int32 specTab{0};
    GearReward reward;
    std::vector<GearEquipment> equipment;
};
struct VendorPurchaseRequest
{
    std::string identityKey, requestId, sourceId;
    uint32 item{0};
    uint8 equipmentSlot{0};
};
struct VendorPurchaseResult
{
    std::string requestId, status{"refused_before_effect"}, reason, stage;
    uint32 item{0}, priceCopper{0}, moneyBefore{0}, moneyAfter{0}, itemBefore{0}, itemAfter{0};
    uint64 itemGuid{0};
    uint32 observedMoneyAfter{0}, observedItemAfter{0};
    bool observationValid{false};
};

// Public loaded templates only; no actor eligibility or acquisition-rate inference.
struct AcquisitionItem
{
    uint32 entry, itemClass, subclass, inventoryType, itemLevel, requiredLevel;
    uint32 allowableClass, allowableRace, requiredSkill, requiredSkillRank;
    std::string name;
    int32 randomProperty{0}, randomSuffix{0};
};
struct AcquisitionReward
{
    std::string kind;
    uint8 index;
    uint32 item, count;
};
// Native source facts are separate from actor-scoped script-capable evaluations.
struct AcquisitionCondition
{
    uint32 sourceType{0}, sourceGroup{0}, sourceId{0}, elseGroup{0}, type{0};
    int32 sourceEntry{0};
    uint32 value1{0}, value2{0}, value3{0}, referenceId{0}, scriptId{0}, target{0};
    bool negative{false};
};
struct AcquisitionHookDispatch
{
    uint32 hookId{0};
    std::string hookName;
    std::vector<std::string> scriptNames;
};
struct AcquisitionGiverScript
{
    int32 entry{0};
    uint32 relation{0}, scriptId{0};
    std::string aiName, registeredScriptName;
    bool templateKnown{false}, registryResolved{false};
};
struct AcquisitionGiverSpawn
{
    int32 entry{0};
    uint32 guid{0}, mapId{0}, phaseMask{0}, spawnMask{0}, spawnGroupId{0}, spawnGroupFlags{0}, scriptId{0};
    bool multispawn{false};
    std::vector<int32> eventIds;
    std::vector<uint32> poolIds;
};
struct AcquisitionObjectiveItem { uint32 index{0}, item{0}, count{0}; };
struct AcquisitionObjectiveTarget { uint32 index{0}, count{0}; int32 entry{0}; };
struct AcquisitionQuest
{
    uint32 entry, minimumLevel, maximumLevel, requiredClasses, allowableRaces, flags, type;
    std::vector<AcquisitionReward> rewards;
    uint32 requiredSkill{0}, requiredSkillRank{0}, nextQuestId{0}, nextQuestInChain{0};
    int32 previousQuestId{0}, exclusiveGroup{0}, signedMoney{0};
    uint32 minimumRepFaction{0}, maximumRepFaction{0}, repObjectiveFaction{0}, repObjectiveFaction2{0}, requiredPlayerKills{0};
    int32 repObjectiveValue{0}, repObjectiveValue2{0};
    std::vector<uint32> breadcrumbDependents;
    bool timed{false}, explorationOrEvent{false}, castObjective{false};
    int32 minimumRepValue{0}, maximumRepValue{0};
    uint32 sourceItem{0}, sourceItemCount{0}, sourceSpell{0}, timeAllowed{0}, eventId{0};
    uint32 breadcrumbQuest{0}, rewardMailTemplate{0};
    bool repeatable{false}, daily{false}, weekly{false}, monthly{false}, seasonal{false}, dungeonFinder{false};
    std::vector<int32> previousQuests;
    std::vector<uint32> previousChainQuests;
    std::vector<AcquisitionCondition> conditions;
    std::vector<uint32> exclusiveQuests;
    std::vector<AcquisitionGiverScript> giverScripts;
    std::vector<AcquisitionGiverSpawn> giverSpawns;
    std::vector<int32> giverEntries;
    std::vector<uint32> itemGivers, questPoolIds, questEventIds;
    std::vector<std::string> giverSourceMissing;
    bool giverSourceComplete{false};
    std::vector<AcquisitionObjectiveItem> requiredItems, requiredSourceItems;
    std::vector<AcquisitionObjectiveTarget> requiredTargets;
    int32 nativeQuestLevel{0}; uint32 nativeLowLevelHideDiff{0}, nativeSuggestedPlayers{0};
    bool questDisabled{false}, phaseTemplateKnown{false}, phaseGateConfigured{false}, phaseGateReady{false}, phaseTemplateAllowed{false};
    std::string sourceRuleVersion{"core1-ordinary-quest-source-rules-v1"};
    std::string predicateCoverage{"native_template_condition_rows_and_enabled_player_hook_dispatch;handler_bodies_not_inferred"};
};
struct AcquisitionPredicate
{
    std::string name, scope, reason;
    uint64 eventSequence{0}, monotonicTicks{0};
    bool evaluated{false}, result{false};
};
struct QuestAcquisitionEvent
{
    uint64 eventSequence{0}, monotonicTicks{0}, operationId{0};
    std::string eventKind, reason, measurementScope{"action_window"};
    uint32 mapId{0}, targetMap{0}, itemEntry{0}, count{0};
    float targetX{0}, targetY{0}, targetZ{0};
    uint64 itemGuid{0};
    bool outcome{false};
};
struct AcquisitionDestination
{
    uint16 bagSlot{0};
    uint32 plannedCount{0}, beforeEntry{0}, itemEntry{0}, beforeCount{0}, afterCount{0}, delta{0};
    uint64 beforeGuid{0}, itemGuid{0};
    bool matched{false};
};
struct AcquisitionMailAttachment
{
    uint64 itemGuid{0};
    uint32 itemEntry{0}, count{0}, savedEntry{0}, savedCount{0};
    bool savedValueMatched{false};
};
struct AcquisitionMailEnvelope
{
    uint32 mailId{0}, receiverGuid{0};
    uint64 deliverTime{0};
    bool queued{false};
    std::vector<AcquisitionMailAttachment> attachments;
};
struct AcquisitionRewardReceipt
{
    uint64 eventSequence{0}, monotonicTicks{0}, countSum{0}, recordId{0}, operationId{0};
    uint32 questId{0}, rewardIndex{0}, itemEntry{0}, count{0};
    std::string rewardKind, deliveryReason;
    bool complete{false}, deliveryPending{false}, deliveryKnown{false}, transactionCommitted{false}, queuedComplete{false};
    std::vector<AcquisitionDestination> destinations;
    std::vector<AcquisitionMailEnvelope> mailEnvelopes;
};
struct QuestAcquisitionAttempt
{
    std::string identityKey, phaseDigest, sourceKey, rewardKind, terminalOutcome{"pending"};
    uint32 accountId{0}, questId{0}, rewardIndex{0}, itemEntry{0}, rewardCount{0};
    uint64 recordId{0}, characterGuid{0}, sessionIncarnation{0}, strategicGeneration{0}, goalRevision{0};
    uint64 monotonicFrequency{1000000000};
    bool validWindow{true}, terminal{false}, validAttempt{false}, actionTerminal{false};
    bool currentContextMatches{true}, observerFailure{false}, postActionContextLost{false};
    uint64 pendingDeliveries{0};
    std::string recordSha256;
    bool movementCoverage{true}, deathCoverage{true}, routeCoverage{true};
    bool failureTerminalCoverage{false};
    bool completeDestinationAttribution{false}, turnInFlight{false}, completeQuestObserved{false};
    bool moving{false};
    std::vector<QuestAcquisitionEvent> events;
    std::vector<AcquisitionPredicate> predicates;
    std::vector<AcquisitionRewardReceipt> rewardReceipts;
};
struct QuestAcquisitionCaptureRequest
{
    bool enabled{false};
    std::string captureId, phaseDigest, rewardKind;
    uint32 accountId{0}, questId{0}, rewardIndex{0}, itemEntry{0}, rewardCount{0};
    uint64 characterGuid{0}, sessionIncarnation{0}, strategicGeneration{0};
};
struct QuestAcquisitionCaptureResult
{
    std::string status{"refused"}, reason, captureId;
    uint64 recordId{0}, sessionIncarnation{0}, strategicGeneration{0};
};
struct AcquisitionAcknowledgement
{
    uint64 recordId{0}, characterGuid{0}, sessionIncarnation{0}, strategicGeneration{0}, goalRevision{0};
    uint32 accountId{0};
    std::string phaseDigest, sourceKey, recordSha256, durableSha256;
};
struct AcquisitionAcknowledgementResult
{
    uint64 recordId{0};
    std::string status{"refused"}, reason, recordSha256, durableSha256;
    bool recoveredClosedContext{false};
};
struct QuestAcquisitionSnapshot
{
    std::string identityKey, reason, phaseDigest;
    uint32 accountId{0};
    uint64 characterGuid{0}, strategicGeneration{0};
    bool valid{false};
    std::vector<QuestAcquisitionAttempt> attempts;
};
struct AcquisitionCatalog
{
    uint64 observationGeneration{0};
    bool valid{false};
    std::string reason;
    PhaseSnapshot phase;
    std::vector<AcquisitionItem> items;
    std::vector<AcquisitionQuest> quests;
    std::vector<AcquisitionHookDispatch> playerHookDispatch;
    bool playerHookRegistryInitialized{false};
};

// A caller supplies only a source table and its complete natural key. Native
// code resolves that key against the currently loaded source store and copies
// values out; it never accepts sealed ordinals or digests from the caller.
struct LoadedSourceNaturalKey
{
    std::string sourceTable;
    uint32 entry{0}, slot{0};
    int32 item{0}; // npc_vendor stores negative reference rows in this signed column
};
struct LoadedVendorSourceObservation
{
    std::string family{"vendor"}, sourceTable{"npc_vendor"}, stableJoinKey, reason;
    uint32 entry{0}, slot{0};
    int32 item{0};
    bool lookupEvaluated{false}, matched{false}, itemTemplateKnown{false};
    uint32 maxCount{0}, incrementTime{0}, extendedCost{0};
    uint32 buyPrice{0}, buyCount{0}, itemRequiredLevel{0}, allowableClass{0}, allowableRace{0};
    uint32 requiredSkill{0}, requiredSkillRank{0};
    // Current Core1 vendor projection fields unavailable from these loaded
    // native rows, and all five metric classes, remain explicit unknowns.
    std::vector<std::string> unknownSemantics;
    std::vector<std::string> unknownMetrics;
};
struct LoadedSourceSnapshot
{
    bool valid{false};
    std::string reason, sourceSnapshotId, processIncarnation;
    uint64 providerConstructedMonotonicNs{0}, captureMonotonicNs{0}, copyOutCompletedMonotonicNs{0}, copyOutDurationNs{0};
    bool reloadGenerationAvailable{false};
    PhaseSnapshot phase;
    std::vector<LoadedVendorSourceObservation> rows;
};

struct SourcePredicateDefinition
{
    uint32 type{0}; int64 values[3]{0, 0, 0};
};
struct SourcePredicateRow
{
    std::string predicateId, sourceId, sourceKind, rewardKind, locusKind, ownerKind, conditionId, astPath, definitionSha256;
    uint32 sourceEntry{0}, itemId{0}, rewardIndex{0}, rewardItem{0}, ownerEntry{0};
    bool hasReward{false}; SourcePredicateDefinition definition;
};
struct SourcePredicateQuery
{
    std::string candidateId, generation, graphDigest, manifestDigest, projectionDigest, identityKey, phaseId, policyDigest, queryDigest;
    uint32 accountId{0}, plannerPhase{0}; uint64 characterGuid{0}, sessionIncarnation{0}, strategicGeneration{0};
    std::vector<SourcePredicateRow> predicates;
};
struct SourcePredicateVerdict
{
    std::string predicateId, definitionSha256, verdict{"unknown"}, reason{"unsupported_native_source_definition_association"};
    SourcePredicateDefinition matchedDefinition;
    std::string sourceKind, rewardKind, ownerKind;
    uint32 sourceEntry{0}, itemId{0}, rewardIndex{0}, rewardItem{0}, ownerEntry{0}; bool hasReward{false};
};
struct SourcePredicateContext
{
    bool valid{false}; std::string reason{"unsupported"}, observedStateGeneration;
    SourcePredicateQuery binding; std::vector<SourcePredicateVerdict> verdicts;
};
struct StrategicGoalRenewal
{
    std::string identityKey;
    uint32 questId{0};
};

struct StrategicGoalResult
{
    std::string status;
    std::string reason;
    uint32 questId{0};
};

class Bridge
{
public:
    virtual ~Bridge() = default;
    virtual GearSnapshot ReadGearSnapshot(std::string const& identityKey, uint32 /*observedQuestId*/)
    {
        GearSnapshot result;
        result.identityKey = identityKey;
        result.reason = "unsupported";
        return result;
    }
    virtual GearResources ReadGearResources(std::string const& identityKey,
        std::vector<uint32> const& /*skillIds*/, std::vector<uint32> const& /*itemIds*/,
        std::vector<uint32> const& /*questIds*/)
    {
        GearResources result;
        result.identityKey = identityKey;
        result.reason = "unsupported";
        return result;
    }
    virtual QuestAcquisitionCaptureResult SetQuestAcquisitionCapture(std::string const&,
        QuestAcquisitionCaptureRequest const& request)
    {
        QuestAcquisitionCaptureResult result; result.reason = "unsupported"; result.captureId = request.captureId;
        result.sessionIncarnation = request.sessionIncarnation; result.strategicGeneration = request.strategicGeneration;
        return result;
    }
    virtual QuestAcquisitionSnapshot ReadQuestAcquisitionSnapshot(std::string const& identityKey,
        uint32, std::string const&, uint32, uint32)
    {
        QuestAcquisitionSnapshot result; result.identityKey = identityKey; result.reason = "unsupported"; return result;
    }
    virtual std::vector<AcquisitionAcknowledgementResult> AcknowledgeQuestAcquisition(
        std::string const&, std::vector<AcquisitionAcknowledgement> const& acknowledgements)
    {
        std::vector<AcquisitionAcknowledgementResult> results;
        for (auto const& proof : acknowledgements)
        {
            AcquisitionAcknowledgementResult result; result.recordId = proof.recordId; result.reason = "unsupported";
            result.recordSha256 = proof.recordSha256; result.durableSha256 = proof.durableSha256; results.push_back(result);
        }
        return results;
    }
    virtual SourcePredicateContext ReadSourcePredicates(std::string const&, SourcePredicateQuery const& query)
    {
        SourcePredicateContext result; result.binding = query; return result;
    }
    virtual AcquisitionCatalog ReadAcquisitionCatalog()
    {
        AcquisitionCatalog result;
        result.reason = "unsupported";
        return result;
    }
    virtual LoadedSourceSnapshot ReadLoadedSourceSnapshot(
        std::vector<LoadedSourceNaturalKey> const& keys)
    {
        LoadedSourceSnapshot result;
        result.reason = "unsupported";
        result.rows.reserve(keys.size());
        for (LoadedSourceNaturalKey const& key : keys)
        {
            LoadedVendorSourceObservation row;
            row.sourceTable = key.sourceTable; row.entry = key.entry; row.slot = key.slot; row.item = key.item;
            row.family = key.sourceTable == "npc_vendor" ? "vendor" : "unknown";
            row.reason = "unsupported"; result.rows.push_back(std::move(row));
        }
        return result;
    }
    virtual std::vector<GearVendorOffer> ReadVendorOffers(std::string const&, std::vector<uint32> const&) { return {}; }
    virtual VendorPurchaseResult BuyVendorItem(VendorPurchaseRequest const& request)
    {
        VendorPurchaseResult result; result.requestId = request.requestId; result.reason = "unsupported"; return result;
    }
    virtual StrategicSnapshot ReadQuestSnapshot(std::string const& identityKey,
                                                uint32 observedQuestId) = 0;
    virtual StrategicGoalResult SetQuestGoal(std::string const& identityKey, uint32 questId,
        std::string const& rewardKind = "", uint32 rewardIndex = 0, uint32 rewardItem = 0) = 0;
    virtual std::vector<StrategicGoalResult> RenewQuestGoals(
        std::vector<StrategicGoalRenewal> const& goals) = 0;
    virtual OwnerHighWater ReadOwnerHighWater() = 0;
    virtual std::vector<EarnedProgress> ReadEarnedSnapshot(std::vector<std::string> const& identityKeys) = 0;
    virtual bool ReserveIdentity(std::string const& identityKey, std::string const& accountKey,
                                 std::string const& guidAllocationKey, std::string const& waveKey) = 0;
    virtual uint32 EnsureAccount(std::string const& accountKey, std::string const& accountName,
                                 uint8 expectedManifestSlots) = 0;
    virtual uint32 EnsureCharacter(std::string const& identityKey, uint32 accountId,
                                   CharacterCreationFields const& fields) = 0;
    virtual IdentityBinding BindIdentityOnce(std::string const& identityKey, uint32 accountId,
                                             uint32 characterGuid, std::string const& manifestDigest) = 0;
    virtual bool SetAdmission(std::string const& identityKey, std::string const& waveKey,
                              std::string const& manifestDigest) = 0;
    virtual bool ApplyProgressionPolicy(std::vector<ProgressionPolicy> const& policies) = 0;
    virtual bool RequestSession(std::string const& identityKey, uint64 capacityGeneration) = 0;
    virtual bool ReleaseSession(std::string const& identityKey) = 0;
    virtual CapacitySnapshot ReadCapacitySnapshot() = 0;
    virtual PhaseSnapshot ReadPhaseSnapshot() = 0;
    virtual bool FailClosed() noexcept = 0;
};

bool RegisterBridge(Bridge* provider) noexcept;
void UnregisterBridge(Bridge* provider) noexcept;
Bridge* GetBridge() noexcept;
bool FailClosed() noexcept;
} // namespace PyEng::Population

#endif
