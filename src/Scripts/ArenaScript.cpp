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

class PythonArenaScript : public ArenaScript
{
public:
    PythonArenaScript() : ArenaScript("PythonArenaScript") {}

    bool CanAddMember(ArenaTeam* team, ObjectGuid PlayerGuid) override
    {
        return TRIGGER_ARENA_HOOK_RESULT(CAN_ADD_MEMBER, true, team, PlayerGuid);
    }

    void OnGetPoints(ArenaTeam* team, uint32 memberRating, float& points) override
    {
        TRIGGER_ARENA_HOOK(ON_GET_POINTS, team, memberRating, std::ref(points));
    }

    bool CanSaveToDB(ArenaTeam* team) override
    {
        return TRIGGER_ARENA_HOOK_RESULT(CAN_SAVE_TO_DB, true, team);
    }

    bool OnBeforeArenaCheckWinConditions(Battleground* const bg ) override
    {
        return TRIGGER_ARENA_HOOK_RESULT(ON_BEFORE_CHECK_WIN_CONDITION, true, bg);
    }

    void OnArenaStart(Battleground* bg ) override
    {
        TRIGGER_ARENA_HOOK(ON_ARENA_START, bg);
    }

    bool OnBeforeArenaTeamMemberUpdate(ArenaTeam* team, Player* player, bool won, uint32 opponentMatchmakerRating, int32 matchmakerChange) override
    {
        return TRIGGER_ARENA_HOOK_RESULT(ON_BEFORE_TEAM_MEMBER_UPDATE, true, team, player, won, opponentMatchmakerRating, matchmakerChange);
    }

    bool CanSaveArenaStatsForMember(ArenaTeam* team, ObjectGuid playerGuid) override
    {
        return TRIGGER_ARENA_HOOK_RESULT(CAN_SAVE_ARENA_STATS_FOR_MEMBER, true, team, playerGuid);
    }

    void OnGetStartPersonalRating(ArenaTeam* team, ObjectGuid playerGuid, uint32& personalRating) override
    {
        TRIGGER_ARENA_HOOK(ON_GET_START_PERSONAL_RATING, team, playerGuid, std::ref(personalRating));
    }
};

void AddSC_PythonArenaScripts()
{
    new PythonArenaScript();
}
