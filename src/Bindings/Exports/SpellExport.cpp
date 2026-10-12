#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "SpellInfo.h"
#include "Spell.h"
#include "Corpse.h"
#include "Item.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief SpellInfo Class - Contains static spell database information
 */
void export_spellinfo_class()
{
    bp::class_<SpellInfo> spellinfo_class("SpellInfo", bp::no_init);
    RegisterBorrowedType<SpellInfo>(spellinfo_class);

    // data value
    spellinfo_class.add_property("Id", bp::make_getter(&SpellInfo::Id, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("SpellName", bp::make_function(+[](SpellInfo const& info)
    {
        bp::list values;
        for (char const* text : info.SpellName)
            values.append(text ? text : "");
        return bp::tuple(values);
    }, BorrowedPolicy<>()));
    spellinfo_class.add_property("Rank", bp::make_function(+[](SpellInfo const& info)
    {
        bp::list values;
        for (char const* text : info.Rank)
            values.append(text ? text : "");
        return bp::tuple(values);
    }, BorrowedPolicy<>()));
    spellinfo_class.add_property("SchoolMask", bp::make_getter(&SpellInfo::SchoolMask, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("DmgClass", bp::make_getter(&SpellInfo::DmgClass, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("Dispel", bp::make_getter(&SpellInfo::Dispel, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("Mechanic", bp::make_getter(&SpellInfo::Mechanic, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("ManaCost", bp::make_getter(&SpellInfo::ManaCost, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("ManaCostPercentage", bp::make_getter(&SpellInfo::ManaCostPercentage, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("PowerType", bp::make_getter(&SpellInfo::PowerType, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("RecoveryTime", bp::make_getter(&SpellInfo::RecoveryTime, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("CategoryRecoveryTime", bp::make_getter(&SpellInfo::CategoryRecoveryTime, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    spellinfo_class.add_property("StackAmount", bp::make_getter(&SpellInfo::StackAmount, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));

    // type checks
    spellinfo_class.def("IsPassive", &SpellInfo::IsPassive, BorrowedPolicy<>());
    spellinfo_class.def("IsChanneled", &SpellInfo::IsChanneled, BorrowedPolicy<>());
    spellinfo_class.def("IsPositive", &SpellInfo::IsPositive, BorrowedPolicy<>());

    // helpers
    spellinfo_class.def("GetDuration", &SpellInfo::GetDuration, BorrowedPolicy<>());
    spellinfo_class.def("GetMaxRange", &SpellInfo::GetMaxRange, BorrowedPolicy<>(),
                        (bp::arg("positive") = false, bp::arg("caster") = static_cast<Unit*>(nullptr),
                         bp::arg("spell") = static_cast<Spell*>(nullptr)));
    spellinfo_class.def("CalcCastTime", &SpellInfo::CalcCastTime, BorrowedPolicy<>(),
                        (bp::arg("caster") = static_cast<Unit*>(nullptr), bp::arg("spell") = static_cast<Spell*>(nullptr)));
}

/**
 * @brief SpellCastTargets Class - Represents targeting information for spell casts
 */
void export_spellcast_targets_class()
{
    bp::class_<SpellCastTargets> spellcast_targets_class("SpellCastTargets", bp::no_init);
    RegisterBorrowedType<SpellCastTargets>(spellcast_targets_class, true);

    // target ref
    spellcast_targets_class.def("GetUnitTarget", &SpellCastTargets::GetUnitTarget,
                                BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
    spellcast_targets_class.def("GetObjectTarget", &SpellCastTargets::GetObjectTarget,
                                BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
    spellcast_targets_class.def("GetCorpseTarget", &SpellCastTargets::GetCorpseTarget,
                                BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
    spellcast_targets_class.def("GetItemTarget", &SpellCastTargets::GetItemTarget,
                                BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());

    // target guid
    spellcast_targets_class.def("GetUnitTargetGUID", &SpellCastTargets::GetUnitTargetGUID, BorrowedPolicy<>());
    spellcast_targets_class.def("GetObjectTargetGUID", &SpellCastTargets::GetObjectTargetGUID, BorrowedPolicy<>());
    spellcast_targets_class.def("GetCorpseTargetGUID", &SpellCastTargets::GetCorpseTargetGUID, BorrowedPolicy<>());
    spellcast_targets_class.def("GetItemTargetGUID", &SpellCastTargets::GetItemTargetGUID, BorrowedPolicy<>());
}
