// Real dispatcher templates and CPython; only engine construction is test plumbing.
#include "PythonEngine.h"
#include <cassert>
#include <iostream>
#include <optional>

namespace bp = boost::python;
using PyEng::Hooks::Category;

// No realm interpreter lifecycle is initialized by this isolated test.
PythonEngine::~PythonEngine() = default;

class PythonEngineTestAccess
{
public:
    static PythonEngine* Create()
    {
        auto* engine = new PythonEngine;
        engine->enabled.store(true);
        return engine;
    }
    static void Destroy(PythonEngine* engine) { delete engine; }
    static void Clear(PythonEngine* engine) { engine->hookMap.clear(); }
    static void Add(PythonEngine* engine, HookInfo hook, uint32 entry, bp::object callback)
    {
        std::unique_lock<std::shared_mutex> lock(engine->hookMutex);
        engine->hookMap[hook][entry].push_back(callback);
    }
};

static PythonEngine* testEngine;
static constexpr HookInfo testHook{Category::WORLD, 7};
static void AddDuringCallback(bp::object callback)
{
    PythonEngineTestAccess::Add(testEngine, testHook, 0, callback);
}

void export_mutable_types();

BOOST_PYTHON_MODULE(dispatch_test)
{
    export_mutable_types();
    bp::def("register_dynamic", AddDuringCallback);
}

int main()
{
    assert(PyImport_AppendInittab("dispatch_test", PyInit_dispatch_test) == 0);
    Py_Initialize();
    try
    {
        testEngine = PythonEngineTestAccess::Create();
        bp::dict globals;
        globals["__builtins__"] = bp::import("builtins");
        bp::exec(R"PY(
from dispatch_test import register_dynamic
seen = []
registered = False
saved_ref = None
saved_string_ref = None
def first(value):
    global registered
    seen.append(('global', value))
    if not registered:
        registered = True
        register_dynamic(dynamic)
def dynamic(value): seen.append(('dynamic', value))
def broken(value): raise RuntimeError('intentional dispatcher test')
class BrokenStringException(Exception):
    def __str__(self): raise RuntimeError('intentional malformed exception string')
def malformed(value): raise BrokenStringException()
def specific(value): seen.append(('entry', value))
def yes(): return True
def no(): return False
def unchanged(): return None
def wrong_type(): return 'invalid boolean'
def one(): return 1
def seven(): return 7
def mutable_add(value):
    global saved_ref
    saved_ref = value
    value.set(value.get() + 2)
def mutable_multiply(value): value.set(value.get() * 3)
def mutable_string_first(value):
    global saved_string_ref
    saved_string_ref = value
    value.set('first')
def mutable_string_second(value): value.set(value.get() + '_second')
)PY", globals, globals);
        auto add = [&](char const* name, uint32 entry = 0)
        {
            PythonEngineTestAccess::Add(testEngine, testHook, entry, globals[name]);
        };
        add("first"); add("broken"); add("malformed"); add("specific", 42);
        testEngine->Trigger(testHook, 42, 11);
        assert(bp::len(globals["seen"]) == 2); // registration applies to the next dispatch snapshot
        testEngine->Trigger(testHook, 42, 12);
        bp::exec("assert seen == [('global', 11), ('entry', 11), ('global', 12), ('dynamic', 12), ('entry', 12)]", globals, globals);
        assert(!PyErr_Occurred());
        PythonEngineTestAccess::Clear(testEngine);
        add("yes"); add("unchanged"); add("no"); add("yes");
        assert(!testEngine->TriggerResult(testHook, 0, true));
        assert(testEngine->TriggerResult(testHook, 0, false));
        PythonEngineTestAccess::Clear(testEngine);
        add("one"); add("unchanged"); add("seven");
        assert(testEngine->TriggerResult(testHook, 0, uint32(99)) == 7);
        PythonEngineTestAccess::Clear(testEngine);
        add("unchanged");
        assert(!testEngine->TriggerResult(testHook, 0, std::optional<bool>{}).has_value());
        add("no");
        assert(testEngine->TriggerResult(testHook, 0, std::optional<bool>{true}) == false);
        PythonEngineTestAccess::Clear(testEngine);
        add("wrong_type"); add("yes");
        assert(testEngine->TriggerResult(testHook, 0, true));
        assert(!PyErr_Occurred());
        PythonEngineTestAccess::Clear(testEngine);
        add("mutable_add"); add("mutable_multiply");
        uint32 nativeValue = 8;
        testEngine->Trigger(testHook, 0, std::ref(nativeValue));
        assert(nativeValue == 30);
        bp::exec(R"PY(
try:
    saved_ref.get()
except ReferenceError:
    pass
else:
    raise AssertionError('retained mutable reference did not expire')
)PY", globals, globals);
        PythonEngineTestAccess::Clear(testEngine);
        add("mutable_string_first"); add("mutable_string_second");
        std::string nativeString = "original";
        testEngine->Trigger(testHook, 0, std::ref(nativeString));
        assert(nativeString == "first_second");
        bp::exec(R"PY(
try:
    saved_string_ref.set('too late')
except ReferenceError:
    pass
else:
    raise AssertionError('retained string reference did not expire')
)PY", globals, globals);
        assert(testEngine->BeginReload());
        assert(testEngine->TriggerResult(testHook, 0, uint32(88)) == 88);
        testEngine->EndReload();
        PythonEngineTestAccess::Destroy(testEngine);
        testEngine = nullptr;
        PyEng::Bridge::ClearBorrowedReferences();
    }
    catch (bp::error_already_set const&)
    {
        PyErr_Print();
        return 1;
    }
    Py_FinalizeEx();
    std::cout << "PASS: real CPython dispatcher global/entry order, dynamic snapshot, exception containment, boolean/optional/numeric returns, sequential scalar/string mutable commits, expired refs, reload fallback\n";
}
