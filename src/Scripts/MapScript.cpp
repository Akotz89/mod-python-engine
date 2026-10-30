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

class PythonMapScript : public AllMapScript
{
public:
    PythonMapScript() : AllMapScript("PythonMapScript") {}

    void OnCreateMap(Map* map) override
    {
        TRIGGER_MAP_HOOK(ON_CREATE_MAP, map);
    }

    void OnDestroyMap(Map* map) override
    {
        TRIGGER_MAP_HOOK(ON_DESTROY_MAP, map);
        if (Py_IsInitialized())
        {
            PyEng::Bridge::GILGuard gil;
            PyEng::Bridge::InvalidateBorrowedPointer(map);
        }
    }

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        TRIGGER_MAP_HOOK(ON_PLAYER_ENTER_ALL, map, player);
    }

    void OnPlayerLeaveAll(Map* map, Player* player) override
    {
        TRIGGER_MAP_HOOK(ON_PLAYER_LEAVE_ALL, map, player);
    }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        TRIGGER_MAP_HOOK(ON_MAP_UPDATE, map, diff);
    }

    void OnBeforeCreateInstanceScript(InstanceMap* instanceMap, InstanceScript** instanceData, bool load, std::string data, uint32 completedEncounterMask) override
    {
        if (instanceData)
            TRIGGER_MAP_HOOK(ON_BEFORE_CREATE_INSTANCE_SCRIPT, instanceMap, std::ref(*instanceData), load, data, completedEncounterMask);
        else
            TRIGGER_MAP_HOOK(ON_BEFORE_CREATE_INSTANCE_SCRIPT, instanceMap, nullptr, load, data, completedEncounterMask);
    }

    void OnDestroyInstance(MapInstanced* mapInstanced, Map* map) override
    {
        TRIGGER_MAP_HOOK(ON_DESTROY_INSTANCE, mapInstanced, map);
    }
};

void AddSC_PythonMapScripts()
{
    new PythonMapScript();
}
