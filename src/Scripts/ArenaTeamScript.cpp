#include "HookMacros.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Spell.h"
#include "Item.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "AchievementMgr.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "Map.h"
#include "Guild.h"
#include "Group.h"
#include "Channel.h"
#include "Pet.h"
#include "ObjectMgr.h"
#include "LootMgr.h"
#include "SpellAuraEffects.h"

class PythonArenaTeamScript : public ArenaTeamScript
{
public:
    PythonArenaTeamScript() : ArenaTeamScript("PythonArenaTeamScript") {}

    void OnGetSlotByType(const uint32 type, uint8& slot) override
    {
        TRIGGER_ARENATEAM_HOOK(ON_GET_SLOT_BY_TYPE, type, std::ref(slot));
    }

    void OnGetArenaPoints(ArenaTeam* team, float& points) override
    {
        TRIGGER_ARENATEAM_HOOK(ON_GET_ARENA_POINTS, team, std::ref(points));
    }

    void OnTypeIDToQueueID(const BattlegroundTypeId bgTypeId, const uint8 arenaType, uint32& queueTypeID) override
    {
        TRIGGER_ARENATEAM_HOOK(ON_TYPEID_TO_QUEUEID, bgTypeId, arenaType, std::ref(queueTypeID));
    }

    void OnQueueIdToArenaType(const BattlegroundQueueTypeId bgQueueTypeId, uint8& ArenaType) override
    {
        TRIGGER_ARENATEAM_HOOK(ON_QUEUEID_TO_ARENA_TYPE, bgQueueTypeId, std::ref(ArenaType));
    }

    void OnSetArenaMaxPlayersPerTeam(const uint8 arenaType, uint32& maxPlayerPerTeam) override
    {
        TRIGGER_ARENATEAM_HOOK(ON_SET_ARENA_MAX_PLAYERS_PER_TEAM, arenaType, std::ref(maxPlayerPerTeam));
    }
};

void AddSC_PythonArenaTeamScripts()
{
    new PythonArenaTeamScript();
}
