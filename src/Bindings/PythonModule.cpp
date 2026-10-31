#include "AutoRegistryMgr.h"
#include "PythonEngine.h"
#include "PythonHeaders.h"

// Expose the full scripting API; native references enforce callback lifetimes.
CREATE_REGISTRY_SCOPE(SafeExports);
REGISTER_TO_SCOPE(SafeExports, export_game_constants, 0);

CREATE_REGISTRY_SCOPE(SafeEventExports);
REGISTER_TO_SCOPE(SafeEventExports, export_safe_event_api, 0);

CREATE_REGISTRY_SCOPE(PopulationExports);
REGISTER_TO_SCOPE(PopulationExports, export_population_api, 0);

CREATE_REGISTRY_SCOPE(PartyExports);
REGISTER_TO_SCOPE(PartyExports, export_party_api, 0);

// Native classes preserve the upstream API and its inheritance.
CREATE_REGISTRY_SCOPE(NativeExports);
REGISTER_TO_SCOPE(NativeExports, export_hook_api, 0);
REGISTER_TO_SCOPE(NativeExports, export_mutable_types, 0);
// object
REGISTER_TO_SCOPE(NativeExports, export_objectguid_class, 5);
REGISTER_TO_SCOPE(NativeExports, export_object_class, 5);
REGISTER_TO_SCOPE(NativeExports, export_position_struct, 5);
REGISTER_TO_SCOPE(NativeExports, export_worldlocation_class, 10);
REGISTER_TO_SCOPE(NativeExports, export_worldobject_class, 15);
REGISTER_TO_SCOPE(NativeExports, export_unit_class, 20);
REGISTER_TO_SCOPE(NativeExports, export_creature_class, 25);
REGISTER_TO_SCOPE(NativeExports, export_gameobject_class, 25);
REGISTER_TO_SCOPE(NativeExports, export_player_class, 25);
// item
REGISTER_TO_SCOPE(NativeExports, export_item_template_struct, 30);
REGISTER_TO_SCOPE(NativeExports, export_item_class, 35);
// map
REGISTER_TO_SCOPE(NativeExports, export_map_class, 40);
// spell
REGISTER_TO_SCOPE(NativeExports, export_spellinfo_class, 50);
REGISTER_TO_SCOPE(NativeExports, export_spellcast_targets_class, 55);
REGISTER_TO_SCOPE(NativeExports, export_callback_data_classes, 60);
REGISTER_TO_SCOPE(NativeExports, export_event_data_classes, 65);

// Register module name with Python before Py_Initialize()
extern "C" PyObject* PyInit_azerothcore();

/**
 * @brief Registers the C++ azerothcore module into Python's internal table of
 * modules before the interpreter starts. This allows Python scripts to import
 * and use C++ functionality via import azerothcore.
 */
void InitAzerothCoreModule()
{
    // NOTE: Must match BOOST_PYTHON_MODULE name below
    if (PyImport_AppendInittab("azerothcore", PyInit_azerothcore) == -1)
        throw std::runtime_error("Failed to append azerothcore module to inittab");
}

/**
 * @brief Construct a new boost python module object
 */
BOOST_PYTHON_MODULE(azerothcore)
{
    EXECUTE_REGISTRY_SCOPE(SafeExports);
    EXECUTE_REGISTRY_SCOPE(SafeEventExports);
    EXECUTE_REGISTRY_SCOPE(PopulationExports);
    EXECUTE_REGISTRY_SCOPE(PartyExports);
    EXECUTE_REGISTRY_SCOPE(NativeExports);
}
