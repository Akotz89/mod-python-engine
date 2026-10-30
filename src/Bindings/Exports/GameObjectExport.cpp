#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Adapters/GameObjectAdapter.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

/**
 * @brief GameObject Class - Represents game objects (chests, doors, buttons, etc.)
 */
void export_gameobject_class()
{
    using namespace PyEng::GameObjectAdapter;

    bp::class_<GameObject, bp::bases<WorldObject>, boost::noncopyable> gameobject_class("GameObject", bp::no_init);
    RegisterBorrowedType<GameObject>(gameobject_class);

    gameobject_class.def("GetEntry", &GameObject::GetEntry, BorrowedPolicy<>());
    gameobject_class.def("GetName", &GameObject::GetName, BorrowedPolicy<bp::return_value_policy<bp::copy_const_reference>>());
    gameobject_class.def("Respawn", &Respawn, BorrowedPolicy<>());
    gameobject_class.def("Delete", &GameObject::Delete, BorrowedPolicy<>());
    gameobject_class.def("Use", &Use, BorrowedPolicy<>());
    gameobject_class.def("GetGoState", &GameObject::GetGoState, BorrowedPolicy<>());
    gameobject_class.def("SetGoState", &GameObject::SetGoState, BorrowedPolicy<>());
    gameobject_class.def("IsSpawned", &GameObject::isSpawned, BorrowedPolicy<>());
    gameobject_class.def("GetGUID", &GetGUIDLow, BorrowedPolicy<>());
}
