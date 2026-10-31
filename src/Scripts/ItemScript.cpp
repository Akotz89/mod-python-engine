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

class PythonItemScript : public AllItemScript
{
public:
    PythonItemScript() : AllItemScript("PythonItemScript") {}

    bool OnItemQuestAccept(Player* player, Item* item, Quest const* quest) override
    {
        return TRIGGER_ITEM_HOOK_RESULT(ON_QUEST_ACCEPT, item->GetEntry(), false, player, item, quest);
    }

    bool CanItemUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        return TRIGGER_ITEM_HOOK_RESULT(ON_USE, item->GetEntry(), false, player, item, targets);
    }

    bool OnItemExpire(Player* player, ItemTemplate const* proto) override
    {
        return TRIGGER_ITEM_HOOK_RESULT(ON_EXPIRE, proto->ItemId, false, player, proto);
    }

    bool OnItemRemove(Player* player, Item* item) override
    {
        bool handled = TRIGGER_ITEM_HOOK_RESULT(ON_REMOVE, item->GetEntry(), false, player, item);
        if (Py_IsInitialized())
        {
            PyEng::Bridge::GILGuard gil;
            PyEng::Bridge::InvalidateBorrowedPointer(item);
        }
        return handled;
    }

    bool CanItemCastCombatSpell(Player* player, Unit* victim, SpellInfo const* spellInfo, Item* item) override
    {
        return TRIGGER_ITEM_HOOK_RESULT(ON_CAST_COMBAT_SPELL, item->GetEntry(), true, player, victim, spellInfo, item);
    }

    void OnItemGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        TRIGGER_ITEM_HOOK(ON_GOSSIPE_SELECT, item->GetEntry(), player, item, sender, action);
    }

    void OnItemGossipSelectCode(Player* player, Item* item, uint32 sender, uint32 action, char const* code) override
    {
        TRIGGER_ITEM_HOOK(ON_GOSSIPE_SELECT_CODE, item->GetEntry(), player, item, sender, action, code);
    }
};

void AddSC_PythonItemScripts()
{
    new PythonItemScript();
}
