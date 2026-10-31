#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Adapters/UnitAdapter.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief Unit Class - Base class for all entities (players, creatures, pets)
 * @note Inherits from WorldObject
 */
void export_unit_class()
{
    using namespace PyEng::UnitAdapter;

    bp::class_<Unit, bp::bases<WorldObject>, boost::noncopyable> unit_class("Unit", bp::no_init);
    RegisterBorrowedType<Unit>(unit_class);

    // common
    unit_class.def("GetGUID", &GetGUIDLow, BorrowedPolicy<>());
    unit_class.def("GetName", &Unit::GetName, BorrowedPolicy<bp::return_value_policy<bp::copy_const_reference>>());
    unit_class.def("GetGender", &Unit::getGender, BorrowedPolicy<>());
    unit_class.def("GetRace", &Unit::getRace, bp::arg("original") = false, BorrowedPolicy<>());
    unit_class.def("GetClass", &Unit::getClass, BorrowedPolicy<>());
    unit_class.def("GetLevel", &Unit::GetLevel, BorrowedPolicy<>());

    // health/power
    unit_class.def("GetHealth", &Unit::GetHealth, BorrowedPolicy<>());
    unit_class.def("GetMaxHealth", &Unit::GetMaxHealth, BorrowedPolicy<>());
    unit_class.def("SetHealth", &Unit::SetHealth, bp::args("val"), BorrowedPolicy<>());
    unit_class.def("GetPower", &PyEng::UnitAdapter::GetPower, bp::args("power"), BorrowedPolicy<>());
    unit_class.def("GetMaxPower", &PyEng::UnitAdapter::GetMaxPower, bp::args("power"), BorrowedPolicy<>());

    // movement
    unit_class.def("IsInCombat", &Unit::IsInCombat, BorrowedPolicy<>());
    unit_class.def("IsPolymorphed", &Unit::IsPolymorphed, BorrowedPolicy<>());
    unit_class.def("isFrozen", &Unit::isFrozen, BorrowedPolicy<>());
    unit_class.def("IsInFlight", &Unit::IsInFlight, BorrowedPolicy<>());
    unit_class.def("IsLevitating", &Unit::IsLevitating, BorrowedPolicy<>());
    unit_class.def("IsWalking", &Unit::IsWalking, BorrowedPolicy<>());
    unit_class.def("IsMoving", &Unit::isMoving, BorrowedPolicy<>());
    unit_class.def("IsSwimming", &Unit::isSwimming, BorrowedPolicy<>());
    unit_class.def("IsFlying", &Unit::IsFlying, BorrowedPolicy<>());
    unit_class.def("IsFalling", &Unit::IsFalling, BorrowedPolicy<>());
    unit_class.def("IsRooted", &Unit::IsRooted, BorrowedPolicy<>());
    unit_class.def("IsStopped", &Unit::IsStopped, BorrowedPolicy<>());

    // misc
    unit_class.def("IsAlive", &Unit::IsAlive, BorrowedPolicy<>());
    unit_class.def("IsDead", &Unit::isDead, BorrowedPolicy<>());
}
