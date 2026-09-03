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

uint32 EnsureAccount(std::string const& accountKey, std::string const& accountName,
                     uint8 expectedManifestSlots)
{
    return Provider().EnsureAccount(accountKey, accountName, expectedManifestSlots);
}

uint32 EnsureCharacter(std::string const& identityKey, uint32 accountId,
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
    return Provider().EnsureCharacter(identityKey, accountId, fields);
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

bool FailClosed() { return Provider().FailClosed(); }
} // namespace

void export_population_api()
{
    boost::python::def("population_read_owner_high_water", ReadOwnerHighWater);
    boost::python::def("population_read_earned_snapshot", ReadEarnedSnapshot);
    boost::python::def("population_reserve_identity", ReserveIdentity);
    boost::python::def("population_ensure_account", EnsureAccount);
    boost::python::def("population_ensure_character", EnsureCharacter);
    boost::python::def("population_bind_identity_once", BindIdentityOnce);
    boost::python::def("population_set_admission", SetAdmission);
    boost::python::def("population_request_session", RequestSession);
    boost::python::def("population_release_session", ReleaseSession);
    boost::python::def("population_read_capacity_snapshot", ReadCapacitySnapshot);
    boost::python::def("population_read_phase_snapshot", ReadPhaseSnapshot);
    boost::python::def("population_fail_closed", FailClosed);
}
