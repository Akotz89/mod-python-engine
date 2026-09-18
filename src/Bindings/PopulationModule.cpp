#include "AutoRegistryMgr.h"
#include "PopulationBridge.h"
#include "PythonEngine.h"
#include "PythonHeaders.h"
#include <stdexcept>
#include <string>
#include <vector>

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

boost::python::dict SetQuestGoal(std::string const& identityKey, uint32 questId)
{
    StrategicGoalResult value = Provider().SetQuestGoal(identityKey, questId);
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
        if (boost::python::len(row) != 3 || !PyBool_Check(held.ptr()) ||
            !PyLong_Check(ceiling.ptr()) || PyBool_Check(ceiling.ptr()))
            throw std::runtime_error("invalid progression policy shape");
        int rawCeiling = boost::python::extract<int>(ceiling);
        if (rawCeiling < 1 || rawCeiling > 80)
            throw std::runtime_error("progression ceiling is outside 1..80");
        policies.push_back({boost::python::extract<std::string>(row["identity_key"]),
                            static_cast<uint8>(rawCeiling),
                            boost::python::extract<bool>(held)});
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
    boost::python::def("strategic_read_quest_snapshot", ReadQuestSnapshot,
                       (boost::python::arg("identity_key"),
                        boost::python::arg("observed_quest_id") = 0));
    boost::python::def("strategic_set_quest_goal", SetQuestGoal);
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
