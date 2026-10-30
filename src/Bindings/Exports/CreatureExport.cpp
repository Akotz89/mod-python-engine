#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Adapters/CreatureAdapter.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief Creature Class - Represents NPCs, monsters, pets, etc.
 * @note Inherits from Unit
 */
void export_creature_class()
{
    using namespace PyEng::CreatureAdapter;

    bp::class_<Creature, bp::bases<Unit>, boost::noncopyable> creature_class("Creature", bp::no_init);
    RegisterBorrowedType<Creature>(creature_class);

    creature_class.def("GetEntry", &Creature::GetEntry, BorrowedPolicy<>());
    creature_class.def("Despawn", &Despawn, BorrowedPolicy<>(),
                       (bp::arg("delayMs") = 0, bp::arg("respawnSeconds") = 0));
    creature_class.def("Respawn", &Respawn, BorrowedPolicy<>());
    creature_class.def("Say", &Say, BorrowedPolicy<>());
    creature_class.def("Yell", &Yell, BorrowedPolicy<>());
    creature_class.def("GetGUID", &GetGUIDLow, BorrowedPolicy<>());
}
