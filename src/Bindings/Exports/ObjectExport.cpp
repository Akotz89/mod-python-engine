#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Object.h"
#include "Adapters/ObjectAdapter.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief Object Class - Base class for all game objects
 */
void export_object_class()
{
    bp::class_<Object, boost::noncopyable> object_class("Object", bp::no_init);
    RegisterBorrowedType<Object>(object_class, true);

    // common
    object_class.def("GetObjectGUID", static_cast<ObjectGuid (Object::*)() const>(&Object::GetGUID), BorrowedPolicy<>());
    object_class.def("GetEntry", &Object::GetEntry, BorrowedPolicy<>());
    object_class.def("GetTypeId", &Object::GetTypeId, BorrowedPolicy<>());
    object_class.def("isType", &Object::isType, BorrowedPolicy<>());

    // type checks
    object_class.def("IsInWorld", &Object::IsInWorld, BorrowedPolicy<>());
    object_class.def("IsItem", &Object::IsItem, BorrowedPolicy<>());
    object_class.def("IsUnit", &Object::IsUnit, BorrowedPolicy<>());
    object_class.def("IsPlayer", &Object::IsPlayer, BorrowedPolicy<>());
    object_class.def("IsCreature", &Object::IsCreature, BorrowedPolicy<>());
    object_class.def("IsGameObject", &Object::IsGameObject, BorrowedPolicy<>());
    object_class.def("IsDynamicObject", &Object::IsDynamicObject, BorrowedPolicy<>());
    object_class.def("IsCorpse", &Object::IsCorpse, BorrowedPolicy<>());

    // field access
    object_class.def("GetUInt32Value", &PyEng::ObjectAdapter::GetUInt32Value, BorrowedPolicy<>());
    object_class.def("GetFloatValue", &PyEng::ObjectAdapter::GetFloatValue, BorrowedPolicy<>());
    object_class.def("SetUInt32Value", &PyEng::ObjectAdapter::SetUInt32Value, BorrowedPolicy<>());
    object_class.def("SetFloatValue", &PyEng::ObjectAdapter::SetFloatValue, BorrowedPolicy<>());

    // flags
    object_class.def("HasFlag", &PyEng::ObjectAdapter::HasFlag, BorrowedPolicy<>());
    object_class.def("SetFlag", &PyEng::ObjectAdapter::SetFlag, BorrowedPolicy<>());
    object_class.def("RemoveFlag", &PyEng::ObjectAdapter::RemoveFlag, BorrowedPolicy<>());
}

/**
 * @brief WorldObject Class - Base class for objects in the game world
 * @note Inherits from Object, WorldLocation
 */
void export_worldobject_class()
{
    bp::class_<WorldObject, bp::bases<Object, WorldLocation>, boost::noncopyable> worldobject_class("WorldObject",
                                                                                                    bp::no_init);
    RegisterBorrowedType<WorldObject>(worldobject_class);

    // common
    worldobject_class.def("GetName", &WorldObject::GetName, BorrowedPolicy<bp::return_value_policy<bp::copy_const_reference>>());

    // location
    worldobject_class.def("GetInstanceId", &WorldObject::GetInstanceId, BorrowedPolicy<>());
    worldobject_class.def("GetAreaId", &WorldObject::GetAreaId, BorrowedPolicy<>());
    worldobject_class.def("GetZoneId", &WorldObject::GetZoneId, BorrowedPolicy<>());
    worldobject_class.def("GetMap", &WorldObject::GetMap, BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
}
