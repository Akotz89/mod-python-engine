#include "HookMacros.h"
#include "PythonBorrowed.h"
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

class PythonCreatureScript : public AllCreatureScript
{
public:
    PythonCreatureScript() : AllCreatureScript("PythonCreatureScript") {}

    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_GOSSIP_HELLO, creature->GetEntry(), false, player, creature);
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_GOSSIP_SELECT, creature->GetEntry(), false, player, creature, sender, action);
    }

    bool CanCreatureGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, char const* code) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_GOSSIP_SELECT_CODE, creature->GetEntry(), false, player, creature, sender, action, code);
    }

    bool CanCreatureQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_QUEST_ACCEPT, creature->GetEntry(), false, player, creature, quest);
    }

    bool OnCreatureQuestSelect(Player* player, Creature* creature, Quest const* quest) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_QUEST_SELECT, creature->GetEntry(), false, player, creature, quest);
    }

    bool OnCreatureQuestComplete(Player* player, Creature* creature, Quest const* quest) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_QUEST_COMPLETE, creature->GetEntry(), false, player, creature, quest);
    }

    bool OnCreatureQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt) override
    {
        return TRIGGER_CREATURE_HOOK_RESULT(ON_QUEST_REWARD, creature->GetEntry(), false, player, creature, quest, opt);
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        TRIGGER_CREATURE_HOOK(ON_ADD_WORLD, creature->GetEntry(), creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        TRIGGER_CREATURE_HOOK(ON_REMOVE_WORLD, creature->GetEntry(), creature);
        if (Py_IsInitialized())
        {
            PyEng::Bridge::GILGuard gil;
            PyEng::Bridge::InvalidateBorrowedPointer(creature);
        }
    }
};

void AddSC_PythonCreatureScripts()
{
    new PythonCreatureScript();
}
