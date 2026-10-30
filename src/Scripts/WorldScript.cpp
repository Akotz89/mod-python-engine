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

class PythonWorldScript : public WorldScript
{
public:
    PythonWorldScript() : WorldScript("PythonWorldScript") {}

    void OnOpenStateChange(bool open) override
    {
        TRIGGER_WORLD_HOOK(ON_OPEN_STATE_CHANGE, open);
    }

    void OnAfterConfigLoad(bool reload) override
    {
        TRIGGER_WORLD_HOOK(ON_AFTER_CONFIG_LOAD, reload);
    }

    void OnLoadCustomDatabaseTable() override
    {
        TRIGGER_WORLD_HOOK(ON_LOAD_CUSTOM_DATABASE_TABLE);
    }

    void OnBeforeConfigLoad(bool reload) override
    {
        TRIGGER_WORLD_HOOK(ON_BEFORE_CONFIG_LOAD, reload);
    }

    void OnMotdChange(std::string& newMotd, LocaleConstant& locale) override
    {
        TRIGGER_WORLD_HOOK(ON_MOTD_CHANGE, std::ref(newMotd), std::ref(locale));
    }

    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask) override
    {
        TRIGGER_WORLD_HOOK(ON_SHUTDOWN_INITIATE, code, mask);
    }

    void OnShutdownCancel() override
    {
        TRIGGER_WORLD_HOOK(ON_SHUTDOWN_CANCEL);
    }

    void OnUpdate(uint32 diff) override
    {
        TRIGGER_WORLD_HOOK(ON_UPDATE, diff);
    }

    void OnStartup() override
    {
        TRIGGER_WORLD_HOOK(ON_STARTUP);
    }

    void OnShutdown() override
    {
        TRIGGER_WORLD_HOOK(ON_SHUTDOWN);
    }

    void OnAfterUnloadAllMaps() override
    {
        TRIGGER_WORLD_HOOK(ON_AFTER_UNLOAD_ALL_MAPS);
        sPythonEngine->Shutdown();
    }

    void OnBeforeFinalizePlayerWorldSession(uint32& cacheVersion) override
    {
        TRIGGER_WORLD_HOOK(ON_BEFORE_FINALIZE_PLAYER_WORLD_SESSION, std::ref(cacheVersion));
    }

    void OnBeforeWorldInitialized() override
    {
        TRIGGER_WORLD_HOOK(ON_BEFORE_WORLD_INITIALIZED);
    }
};

void AddSC_PythonWorldScripts()
{
    new PythonWorldScript();
}
