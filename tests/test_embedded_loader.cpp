#include "PythonEngine.h"
#include "Config.h"
#include "Hooks/Defines/WorldHook.h"
#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace bp = boost::python;
namespace fs = std::filesystem;
void export_hook_api();

// The module entrypoint is test plumbing; interpreter, config, loader, exec and queue are production code.
BOOST_PYTHON_MODULE(azerothcore)
{
    export_hook_api();
}

void InitAzerothCoreModule()
{
    if (PyImport_AppendInittab("azerothcore", PyInit_azerothcore) < 0)
        throw std::runtime_error("failed to register test module");
}

struct PythonEngineTestAccess
{
    static void SetRoot(PythonEngine* engine, std::string const& root)
    {
        GILGuard gil;
        engine->main_namespace["__test_root__"] = root;
    }
};

std::string Read(fs::path const& path)
{
    std::ifstream stream(path, std::ios::binary);
    return {std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>()};
}

void Write(fs::path const& path, std::string const& text)
{
    std::ofstream stream(path);
    stream << text;
    assert(stream.good());
}

void Exec(PythonEngine* engine, std::string const& code)
{
    auto result = engine->ExecuteCode(code);
    if (!result)
        std::cerr << result.error << '\n';
    assert(result);
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    fs::path run(argv[1]);
    fs::path root = run / "python";
    fs::create_directories(root);
    Write(run / "loader.conf", "[worldserver]\nPython.Enabled = 1\nPython.Population.Enabled = 0\nPython.ScriptsPath = \"" + root.string() + "\"\n");
    sConfigMgr->Configure((run / "loader.conf").string(), {});
    assert(sConfigMgr->LoadAppConfigs());
    Write(root / "loader_dependency.py", "GENERATION = 'v1'\n");
    Write(root / "a_main.py", R"PY(
import azerothcore as ac
import sys, pathlib, hashlib
import loader_dependency
seen = []
def on_update(diff):
    seen.append(loader_dependency.GENERATION)
ac.Register('WORLD_ON_UPDATE', on_update)
)PY");
    auto engine = sPythonEngine;
    engine->Initialize();
    assert(engine->IsEnabled());
    PythonEngineTestAccess::SetRoot(engine, root.string());
    Exec(engine, R"PY(
import sys, pathlib, hashlib
sys.path.insert(0, __test_root__)
import loader_dependency
assert loader_dependency.GENERATION == 'v1'
original_dependency = loader_dependency
cache_file = pathlib.Path(loader_dependency.__cached__)
assert cache_file.is_file()
cache_checksum = hashlib.sha256(cache_file.read_bytes()).hexdigest()
)PY");
    API::Object originalDependency;
    fs::path cache;
    {
        GILGuard gil;
        originalDependency = API::Import("loader_dependency");
        cache = boost::python::extract<std::string>(originalDependency.attr("__cached__"))();
    }
    auto cacheBytes = Read(cache);
    // Preserve the bytecode's source timestamp and size while changing the actual source.
    auto timestamp = fs::last_write_time(root / "loader_dependency.py");
    Write(root / "loader_dependency.py", "GENERATION = 'v2'\n");
    fs::last_write_time(root / "loader_dependency.py", timestamp);
    for (auto const& entry : fs::recursive_directory_iterator(root))
        fs::permissions(entry.path(), entry.is_directory() ? fs::perms::owner_read | fs::perms::owner_exec : fs::perms::owner_read);
    fs::permissions(root, fs::perms::owner_read | fs::perms::owner_exec);
    // Actual LoadScripts exercises every Boost proxy/object/list preparation expression.
    assert(engine->LoadScripts());
    assert(Read(cache) == cacheBytes);
    {
        GILGuard gil;
        assert(boost::python::extract<std::string>(originalDependency.attr("GENERATION"))() == "v1");
        assert(API::Import("loader_dependency").ptr() != originalDependency.ptr());
        originalDependency = API::Object();
    }
    engine->Trigger(PyEng::Hooks::World::ON_UPDATE, 0, uint32(1));
    Exec(engine, R"PY(
assert seen == ['v2']
assert loader_dependency.GENERATION == 'v2'
assert sum(bool(getattr(f, '_python_engine_source_finder', False)) for f in sys.meta_path) == 1
)PY");
    // Prepare tuple path input and a pending accepted queue callback before a rejected candidate.
    Exec(engine, "sys.path = tuple(sys.path)\nac.Schedule(lambda: seen.append('accepted_queue'))\n");
    fs::permissions(root, fs::perms::owner_all);
    Write(root / "z_reject.py", "ac.Schedule(lambda: seen.append('rejected_queue'))\nraise RuntimeError('intentional rejected load')\n");
    assert(!engine->LoadScripts());
    assert(!engine->IsReloading());
    engine->RunScheduledCallbacks();
    engine->Trigger(PyEng::Hooks::World::ON_UPDATE, 0, uint32(1));
    Exec(engine, "assert seen == ['v2', 'accepted_queue', 'v2']\nassert isinstance(sys.path, tuple)\nassert loader_dependency.GENERATION == 'v2'\n");
    fs::remove(root / "z_reject.py");
    assert(engine->LoadScripts());
    Exec(engine, "assert isinstance(sys.path, list)\nassert loader_dependency.GENERATION == 'v2'\n");
    engine->Shutdown();
    assert(!Py_IsInitialized());
    std::cout << "PASS: actual PythonEngine.cpp Initialize/LoadScripts/ExecuteCode/queue; readonly stale-cache source refresh, tuple-path preparation, rejected-load registry/import/queue retention, subsequent successful reload and shutdown\n";
}
