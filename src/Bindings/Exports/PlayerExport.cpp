#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Adapters/PlayerAdapter.h"
#include "Player.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief Player Class - Represents a player character
 * @note Inherits from Unit
 */
void export_player_class()
{
    using namespace PyEng::PlayerAdapter;

    bp::class_<Player, bp::bases<Unit>, boost::noncopyable> player_class("Player", bp::no_init);
    RegisterBorrowedType<Player>(player_class);

    // common
    player_class.def("GetGUID", &GetGUIDLow, BorrowedPolicy<>());
    player_class.def("GetAccountId", &GetAccountId, BorrowedPolicy<>());

    // level
    player_class.def("GiveXP", &GiveXP, BorrowedPolicy<>(),
                     (bp::arg("xp"), bp::arg("victim") = static_cast<Unit*>(nullptr),
                      bp::arg("group_rate") = 1.0f, bp::arg("isLFGReward") = false));
    player_class.def("GiveLevel", &Player::GiveLevel, BorrowedPolicy<>());
    player_class.def("SetXP", &SetXP, BorrowedPolicy<>());

    // chat
    player_class.def("Say", &Say, BorrowedPolicy<>());
    player_class.def("Yell", &Yell, BorrowedPolicy<>());
    player_class.def("SendNotification", &SendNotification, BorrowedPolicy<>());
    player_class.def("SendSysMessage", &SendSysMessage, BorrowedPolicy<>());

    // item
    player_class.def("HasItem", &HasItem, BorrowedPolicy<>());
    player_class.def("AddItem", &AddItem, BorrowedPolicy<>());
    player_class.def("DestroyItem", &DestroyItem, BorrowedPolicy<>());
    player_class.def("RemoveItem", &RemoveItem, BorrowedPolicy<>());

    // money
    player_class.def("GetMoney", &Player::GetMoney, BorrowedPolicy<>());
    player_class.def("SetMoney", &Player::SetMoney, BorrowedPolicy<>());
    player_class.def("ModifyMoney", &ModifyMoney, BorrowedPolicy<>());

    // teleport
    player_class.def("TeleportTo", &TeleportTo, BorrowedPolicy<>());

    // quest
    player_class.def("HasQuest", &Player::HasQuest, BorrowedPolicy<>());
    player_class.def("CompleteQuest", &Player::CompleteQuest, BorrowedPolicy<>());
}
