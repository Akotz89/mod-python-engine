#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "QuestDef.h"
#include "Corpse.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "WorldSession.h"
#include "Group.h"
#include "Guild.h"

namespace bp = boost::python;
using namespace PyEng::Bridge;
using BorrowedReturn = BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>;
using CopiedString = BorrowedPolicy<bp::return_value_policy<bp::copy_const_reference>>;

void export_callback_data_classes()
{
    bp::class_<Quest, boost::noncopyable> quest("Quest", bp::no_init);
    RegisterBorrowedType<Quest>(quest);
    quest.def("GetQuestId", &Quest::GetQuestId, BorrowedPolicy<>());
    quest.def("GetQuestLevel", &Quest::GetQuestLevel, BorrowedPolicy<>());
    quest.def("GetTitle", &Quest::GetTitle, CopiedString());

    bp::class_<Corpse, bp::bases<WorldObject>, boost::noncopyable> corpse("Corpse", bp::no_init);
    RegisterBorrowedType<Corpse>(corpse);
    corpse.def("GetOwnerGUID", &Corpse::GetOwnerGUID, BorrowedPolicy<>());

    bp::class_<Spell, boost::noncopyable> spell("Spell", bp::no_init);
    RegisterBorrowedType<Spell>(spell);
    spell.def("GetCaster", &Spell::GetCaster, BorrowedReturn());
    spell.def("GetSpellInfo", &Spell::GetSpellInfo, BorrowedReturn());

    bp::class_<Aura, boost::noncopyable> aura("Aura", bp::no_init);
    RegisterBorrowedType<Aura>(aura);
    aura.def("GetId", &Aura::GetId, BorrowedPolicy<>());
    aura.def("GetCaster", &Aura::GetCaster, BorrowedReturn());
    aura.def("GetSpellInfo", &Aura::GetSpellInfo, BorrowedReturn());

    bp::class_<AuraApplication, boost::noncopyable> auraApplication("AuraApplication", bp::no_init);
    RegisterBorrowedType<AuraApplication>(auraApplication);
    auraApplication.def("GetTarget", &AuraApplication::GetTarget, BorrowedReturn());

    bp::class_<WorldSession, boost::noncopyable> session("WorldSession", bp::no_init);
    RegisterBorrowedType<WorldSession>(session);
    session.def("GetAccountId", &WorldSession::GetAccountId, BorrowedPolicy<>());
    session.def("GetSecurity", +[](WorldSession const& value) { return static_cast<uint32>(value.GetSecurity()); }, BorrowedPolicy<>());
    session.def("GetPlayer", &WorldSession::GetPlayer, BorrowedReturn());

    bp::class_<Group, boost::noncopyable> group("Group", bp::no_init);
    RegisterBorrowedType<Group>(group);
    group.def("GetLeaderGUID", &Group::GetLeaderGUID, BorrowedPolicy<>());
    group.def("GetMembersCount", &Group::GetMembersCount, BorrowedPolicy<>());

    bp::class_<Guild, boost::noncopyable> guild("Guild", bp::no_init);
    RegisterBorrowedType<Guild>(guild);
    guild.def("GetId", &Guild::GetId, BorrowedPolicy<>());
    guild.def("GetName", &Guild::GetName, CopiedString());
    guild.def("GetLeaderGUID", &Guild::GetLeaderGUID, BorrowedPolicy<>());
}
