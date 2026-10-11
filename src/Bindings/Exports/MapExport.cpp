#include "PythonHeaders.h"
#include "PythonBorrowed.h"
#include "Map.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "MapMgr.h"
#include "DBCStores.h"
#include "PythonEngine.h"

namespace bp = boost::python;
using PyEng::Bridge::BorrowedPolicy;
using PyEng::Bridge::RegisterBorrowedType;

namespace
{
    Map* GetMap(uint32 mapId)
    {
        if (!sPythonEngine->IsNativeCapabilityCallAllowed())
            throw std::runtime_error("map access requires the world thread");
        if (!sMapStore.LookupEntry(mapId))
            throw std::invalid_argument("unknown map ID");
        return sMapMgr->CreateBaseMap(mapId);
    }

    void LoadGrid(Map* map, float x, float y)
    {
        if (!sPythonEngine->IsNativeCapabilityCallAllowed())
            throw std::runtime_error("grid loading requires the world thread");
        if (!Acore::IsValidMapCoord(x, y))
            throw std::invalid_argument("invalid map coordinates");
        map->LoadGrid(x, y);
    }
}

/**
 * @brief Map Class - Represents a game map (world, dungeon, battleground, etc.)
 */
void export_map_class()
{
    bp::class_<Map, boost::noncopyable> map_class("Map", bp::no_init);
    RegisterBorrowedType<Map>(map_class);

    // common
    map_class.def("GetId", &Map::GetId, BorrowedPolicy<>());
    map_class.def("GetMapName", &Map::GetMapName, BorrowedPolicy<>());

    // type checks
    map_class.def("Instanceable", &Map::Instanceable, BorrowedPolicy<>());
    map_class.def("IsDungeon", &Map::IsDungeon, BorrowedPolicy<>());
    map_class.def("IsNonRaidDungeon", &Map::IsNonRaidDungeon, BorrowedPolicy<>());
    map_class.def("IsRaid", &Map::IsRaid, BorrowedPolicy<>());
    map_class.def("IsRaidOrHeroicDungeon", &Map::IsRaidOrHeroicDungeon, BorrowedPolicy<>());
    map_class.def("IsHeroic", &Map::IsHeroic, BorrowedPolicy<>());
    map_class.def("Is25ManRaid", &Map::Is25ManRaid, BorrowedPolicy<>());
    map_class.def("IsBattleground", &Map::IsBattleground, BorrowedPolicy<>());
    map_class.def("IsBattleArena", &Map::IsBattleArena, BorrowedPolicy<>());
    map_class.def("IsBattlegroundOrArena", &Map::IsBattlegroundOrArena, BorrowedPolicy<>());
    map_class.def("IsWorldMap", &Map::IsWorldMap, BorrowedPolicy<>());

    // entity
    map_class.def("GetCreature", &Map::GetCreature, BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
    map_class.def("GetGameObject", &Map::GetGameObject, BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());

    map_class.def("LoadGrid", &LoadGrid, BorrowedPolicy<>());
    bp::def("GetMap", &GetMap, BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());

    // location
    map_class.def("GetHeight", static_cast<float (Map::*)(float, float, float, bool, float) const>(&Map::GetHeight), BorrowedPolicy<>(),
                  (bp::arg("x"), bp::arg("y"), bp::arg("z"), bp::arg("checkVMap") = true,
                   bp::arg("maxSearchDist") = DEFAULT_HEIGHT_SEARCH));
    map_class.def("GetAreaId", &Map::GetAreaId, BorrowedPolicy<>());
    map_class.def("GetZoneId", &Map::GetZoneId, BorrowedPolicy<>());
}
