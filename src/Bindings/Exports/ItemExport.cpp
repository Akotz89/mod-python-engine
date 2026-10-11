#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Adapters/ItemAdapter.h"
#include "ItemTemplate.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief ItemTemplate struct - Contains static item database data
 */
void export_item_template_struct()
{
    bp::class_<ItemTemplate> item_template_struct("ItemTemplate", bp::no_init);
    RegisterBorrowedType<ItemTemplate>(item_template_struct);

    // data value
    item_template_struct.add_property("Id", bp::make_getter(&ItemTemplate::ItemId, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("Class", bp::make_getter(&ItemTemplate::Class, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("SubClass", bp::make_getter(&ItemTemplate::SubClass, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("Name", bp::make_getter(&ItemTemplate::Name1, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("Quality", bp::make_getter(&ItemTemplate::Quality, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("BuyCount", bp::make_getter(&ItemTemplate::BuyCount, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("BuyPrice", bp::make_getter(&ItemTemplate::BuyPrice, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("SellPrice", bp::make_getter(&ItemTemplate::SellPrice, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("InventoryType", bp::make_getter(&ItemTemplate::InventoryType, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("AllowableClass", bp::make_getter(&ItemTemplate::AllowableClass, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("AllowableRace", bp::make_getter(&ItemTemplate::AllowableRace, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("ItemLevel", bp::make_getter(&ItemTemplate::ItemLevel, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("MaxCount", bp::make_getter(&ItemTemplate::MaxCount, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
    item_template_struct.add_property("Stackable", bp::make_getter(&ItemTemplate::Stackable, BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
}

/**
 * @brief Item Class - Represents game items (armor, weapons, consumables, etc.)
 */
void export_item_class()
{
    bp::class_<Item, boost::noncopyable> item_class("Item", bp::no_init);
    RegisterBorrowedType<Item>(item_class);

    // data ref
    item_class.def("GetTemplate", &Item::GetTemplate, BorrowedPolicy<bp::return_internal_reference<>>());

    // common
    item_class.def("GetEntry", &Item::GetEntry, BorrowedPolicy<>());
    item_class.def("GetCount", &Item::GetCount, BorrowedPolicy<>());
    item_class.def("GetSlot", &Item::GetSlot, BorrowedPolicy<>());
    item_class.def("GetBagSlot", &Item::GetBagSlot, BorrowedPolicy<>());
}
