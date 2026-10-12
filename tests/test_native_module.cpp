#include "PythonEngine.h"
#include "Config.h"
#include "Spell.h"
#include "Hooks/HookDefines.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

// No module/export stubs: this executable links the production module archive.
int main(int argc, char** argv)
{
    assert(argc == 2);
    auto config = std::filesystem::path(argv[1]) / "native-module.conf";
    std::ofstream(config) << "[worldserver]\nPython.Enabled = 1\nPython.Population.Enabled = 0\n";
    sConfigMgr->Configure(config.string(), {});
    assert(sConfigMgr->LoadAppConfigs());
    auto engine = sPythonEngine;
    engine->Initialize();
    assert(engine->IsEnabled());
    auto result = engine->ExecuteCode(R"PY(
import azerothcore as ac
for name in '''ObjectGuid Object Position WorldLocation WorldObject Unit Creature
GameObject Player ItemTemplate Item Map SpellInfo SpellCastTargets Quest Corpse
Spell Aura AuraApplication WorldSession Group Guild ByteBuffer WorldPacket
AuraEffect ArenaTeam Battleground Channel InstanceScript AchievementEntry
AchievementCriteriaEntry CriteriaProgress AchievementMgr AchievementGlobalMgr
CreatureTemplate FactionTemplateEntry MapEntry MapDifficulty InstanceTemplate
DungeonProgressionRequirements ProgressionRequirement AuctionEntry LootItem Loot
VendorItem Roll MovementInfo SkillLineAbilityEntry TalentEntry ItemSetEffect
ScalingStatValuesEntry KillRewarder'''.split():
    assert isinstance(getattr(ac, name), type), name
assert issubclass(ac.Unit, ac.WorldObject)
assert issubclass(ac.Player, ac.Unit)
assert callable(ac.Player.GiveXP)
assert callable(ac.SpellInfo.GetMaxRange)
assert callable(ac.SpellInfo.CalcCastTime)
assert callable(ac.Unit.GetRace)
assert 'None' in ac.Player.GiveXP.__doc__
assert 'None' in ac.SpellInfo.GetMaxRange.__doc__
assert 'None' in ac.SpellInfo.CalcCastTime.__doc__
assert callable(ac.GetPlayer) and callable(ac.GetMap)
seen = []
ac.Register('WORLD_ON_UPDATE', lambda diff: seen.append(diff))
ac.Schedule(lambda: seen.append('scheduled'))
)PY");
    if (!result)
        std::cerr << result.error << '\n';
    assert(result);
    {
        GILGuard gil;
        API::Object none;
        assert(boost::python::extract<Unit*>(none)() == nullptr);
        assert(boost::python::extract<Spell*>(none)() == nullptr);
        assert(!PyErr_Occurred());
    }
    engine->RunScheduledCallbacks();
    engine->Trigger(PyEng::Hooks::World::ON_UPDATE, 0, uint32(42));
    result = engine->ExecuteCode("assert seen == ['scheduled', 42]\n");
    assert(result);
    engine->Shutdown();
    assert(!Py_IsInitialized());
    std::cout << "PASS: real production azerothcore module import, all export initialization/default registration, native inheritance/methods, Register, Schedule and shutdown\n";
}
