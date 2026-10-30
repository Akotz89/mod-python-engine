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

class PythonServerScript : public ServerScript
{
public:
    PythonServerScript() : ServerScript("PythonServerScript") {}

    void OnNetworkStart() override
    {
        TRIGGER_SERVER_HOOK(ON_NETWORK_START);
    }

    void OnNetworkStop() override
    {
        TRIGGER_SERVER_HOOK(ON_NETWORK_STOP);
    }

    void OnSocketOpen(std::shared_ptr<WorldSocket> const& socket) override
    {
        TRIGGER_SERVER_HOOK(ON_SOCKET_OPEN, socket);
    }

    void OnSocketClose(std::shared_ptr<WorldSocket> const& socket) override
    {
        TRIGGER_SERVER_HOOK(ON_SOCKET_CLOSE, socket);
    }

    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        return TRIGGER_SERVER_HOOK_RESULT(CAN_PACKET_SEND, true, session, packet);
    }

    bool CanPacketReceive(WorldSession* session, WorldPacket& packet) override
    {
        return TRIGGER_SERVER_HOOK_RESULT(CAN_PACKET_RECEIVE, true, session, std::ref(packet));
    }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        return TRIGGER_SERVER_HOOK_RESULT(CAN_PACKET_RECEIVE, true, session, packet);
    }
    void OnPacketReceived(WorldSession* session, WorldPacket const& packet) override
    {
        TRIGGER_SERVER_HOOK(ON_PACKET_RECEIVED, session, packet);
    }

};

void AddSC_PythonServerScripts()
{
    new PythonServerScript();
}
