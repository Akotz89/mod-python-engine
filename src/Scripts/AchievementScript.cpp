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

class PythonAchievementScript : public AchievementScript
{
public:
    PythonAchievementScript() : AchievementScript("PythonAchievementScript") {}

    void SetRealmCompleted(AchievementEntry const* achievement) override
    {
        TRIGGER_ACHIEVEMENT_HOOK(SET_REALM_COMPLETED, achievement);
    }

    bool IsCompletedCriteria(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria, AchievementEntry const* achievement, CriteriaProgress const* progress) override
    {
        return TRIGGER_ACHIEVEMENT_HOOK_RESULT(IS_COMPLETED_CRITERIA, true, mgr, achievementCriteria, achievement, progress);
    }

    bool IsRealmCompleted(AchievementGlobalMgr const* globalmgr, AchievementEntry const* achievement, SystemTimePoint completionTime) override
    {
        return TRIGGER_ACHIEVEMENT_HOOK_RESULT(IS_REALM_COMPLETED, true, globalmgr, achievement, completionTime);
    }

    void OnBeforeCheckCriteria(AchievementMgr* mgr, std::list<AchievementCriteriaEntry const*> const* achievementCriteriaList) override
    {
        TRIGGER_ACHIEVEMENT_HOOK(ON_BEFORE_CHECK_CRITERIA, mgr, achievementCriteriaList);
    }

    bool CanCheckCriteria(AchievementMgr* mgr, AchievementCriteriaEntry const* achievementCriteria) override
    {
        return TRIGGER_ACHIEVEMENT_HOOK_RESULT(CAN_CHECK_CRITERIA, true, mgr, achievementCriteria);
    }
};

void AddSC_PythonAchievementScripts()
{
    new PythonAchievementScript();
}
