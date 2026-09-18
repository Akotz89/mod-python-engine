#ifndef MOD_PYTHON_ENGINE_POPULATION_BRIDGE_H
#define MOD_PYTHON_ENGINE_POPULATION_BRIDGE_H

#include "Define.h"
#include <string>
#include <unordered_map>
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
    uint32 observedQuestId{0};
    std::string observedQuestStatus{"none"};
    uint8 level{0};
    uint32 xp{0};
    std::vector<StrategicQuest> quests;
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
    virtual StrategicSnapshot ReadQuestSnapshot(std::string const& identityKey,
                                                uint32 observedQuestId) = 0;
    virtual StrategicGoalResult SetQuestGoal(std::string const& identityKey, uint32 questId) = 0;
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
