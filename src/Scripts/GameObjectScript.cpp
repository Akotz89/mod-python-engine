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

class PythonGameObjectScript : public AllGameObjectScript
{
public:
    PythonGameObjectScript() : AllGameObjectScript("PythonGameObjectScript") {}

    bool CanGameObjectGossipHello(Player* player, GameObject* go) override
    {
        return TRIGGER_GAMEOBJECT_HOOK_RESULT(ON_GOSSIP_HELLO, go->GetEntry(), false, player, go);
    }

    bool CanGameObjectGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        return TRIGGER_GAMEOBJECT_HOOK_RESULT(ON_GOSSIP_SELECT, go->GetEntry(), false, player, go, sender, action);
    }

    bool CanGameObjectGossipSelectCode(Player* player, GameObject* go, uint32 sender, uint32 action, char const* code) override
    {
        return TRIGGER_GAMEOBJECT_HOOK_RESULT(ON_GOSSIP_SELECT_CODE, go->GetEntry(), false, player, go, sender, action, code);
    }

    bool CanGameObjectQuestAccept(Player* player, GameObject* go, Quest const* quest) override
    {
        return TRIGGER_GAMEOBJECT_HOOK_RESULT(ON_QUEST_ACCEPT, go->GetEntry(), false, player, go, quest);
    }

    bool OnGameObjectQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 opt) override
    {
        return TRIGGER_GAMEOBJECT_HOOK_RESULT(ON_QUEST_REWARD, go->GetEntry(), false, player, go, quest, opt);
    }

    void OnGameObjectDestroyed(GameObject* go, Player* player) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_DESTROYED, go->GetEntry(), go, player);
    }

    void OnGameObjectDamaged(GameObject* go, Player* player) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_DAMAGED, go->GetEntry(), go, player);
    }

    void OnGameObjectModifyHealth(GameObject* go, Unit* attackerOrHealer, int32& change, SpellInfo const* spellInfo) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_MODIFY_HEALTH, go->GetEntry(), go, attackerOrHealer, std::ref(change), spellInfo);
    }

    void OnGameObjectLootStateChanged(GameObject* go, uint32 state, Unit* unit) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_LOOT_STATE_CHANGED, go->GetEntry(), go, state, unit);
    }

    void OnGameObjectUpdate(GameObject* go, uint32 diff) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_UPDATE, go->GetEntry(), go, diff);
    }

    void OnGameObjectAddWorld(GameObject* go) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_ADD_WORLD, go->GetEntry(), go);
    }

    void OnGameObjectRemoveWorld(GameObject* go) override
    {
        TRIGGER_GAMEOBJECT_HOOK(ON_REMOVE_WORLD, go->GetEntry(), go);
        if (Py_IsInitialized())
        {
            PyEng::Bridge::GILGuard gil;
            PyEng::Bridge::InvalidateBorrowedPointer(go);
        }
    }
};

void AddSC_PythonGameObjectScripts()
{
    new PythonGameObjectScript();
}
