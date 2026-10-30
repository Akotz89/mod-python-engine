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

class PythonAccountScript : public AccountScript
{
public:
    PythonAccountScript() : AccountScript("PythonAccountScript") {}

    void OnAccountLogin(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_ACCOUNT_LOGIN, accountId);
    }

    void OnBeforeAccountDelete(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_BEFORE_ACCOUNT_DELETE, accountId);
    }

    void OnLastIpUpdate(uint32 accountId, std::string ip) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_LAST_IP_UPDATE, accountId, ip);
    }

    void OnFailedAccountLogin(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_FAILED_ACCOUNT_LOGIN, accountId);
    }

    void OnEmailChange(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_EMAIL_CHANGE, accountId);
    }

    void OnFailedEmailChange(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_FAILED_EMAIL_CHANGE, accountId);
    }

    void OnPasswordChange(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_PASSWORD_CHANGE, accountId);
    }

    void OnFailedPasswordChange(uint32 accountId) override
    {
        TRIGGER_ACCOUNT_HOOK(ON_FAILED_PASSWORD_CHANGE, accountId);
    }

    bool CanAccountCreateCharacter(uint32 accountId, uint8 charRace, uint8 charClass) override
    {
        return TRIGGER_ACCOUNT_HOOK_RESULT(CAN_ACCOUNT_CREATE_CHARACTER, true, accountId, charRace, charClass);
    }
};

void AddSC_PythonAccountScripts()
{
    new PythonAccountScript();
}
