#include "PythonConverter.h"
#include "PythonGIL.h"
#include "PythonException.h"
#include <cassert>
#include <set>
#include <map>
#include <thread>

namespace bp = boost::python;
using namespace PyEng::Bridge;
void export_mutable_types();

struct NativeTestObject
{
    int value = 7;
    int Read() const { return value; }
    NativeTestObject* Child() { return this; }
};
struct UnboundTestObject { int value = 3; };

BOOST_PYTHON_MODULE(binding_test)
{
    export_mutable_types();
    bp::class_<NativeTestObject, boost::noncopyable> native("Native", bp::no_init);
    RegisterBorrowedType<NativeTestObject>(native);
    native.def("read", &NativeTestObject::Read, BorrowedPolicy<>());
    native.def("child", &NativeTestObject::Child,
        BorrowedPolicy<bp::return_value_policy<bp::reference_existing_object>>());
    native.add_property("value", bp::make_getter(&NativeTestObject::value,
        BorrowedPolicy<bp::return_value_policy<bp::return_by_value>>()));
}

void Exec(char const* script, bp::dict& globals)
{
    bp::exec(script, globals, globals);
}

int main()
{
    assert(PyImport_AppendInittab("binding_test", PyInit_binding_test) == 0);
    Py_Initialize();
    try
    {
        {
            bp::dict globals;
            globals["__builtins__"] = bp::import("builtins");
            globals["__name__"] = "__main__";
            bp::import("binding_test");
            NativeTestObject native;
            UnboundTestObject opaque;
            int32 number = 1;
            std::string text = "old";
            std::set<uint32> values{1, 2};
            std::map<uint32, uint32> mapping{{1, 2}};
            UnboundTestObject* output = &opaque;
            {
                CallbackScope scope;
                globals["native"] = Converter::ToPython(&native);
                globals["const_native"] = Converter::ToPython(static_cast<NativeTestObject const*>(&native));
                globals["number"] = Converter::ToPython(std::ref(number));
                globals["text"] = Converter::ToPython(std::ref(text));
                globals["values"] = Converter::ToPython(std::ref(values));
                globals["opaque"] = Converter::ToPython(&opaque);
                globals["output"] = Converter::ToPython(std::ref(output));
                globals["mapping"] = Converter::ToPython(std::ref(mapping));
                Exec("assert native.read() == 7\nassert native.value == 7\nassert const_native.read() == 7\n"
                     "child = native.child()\nassert child.read() == 7\n"
                     "number.set(42)\ntext.set('new')\nvalues.set([3, 4])\n"
                     "assert opaque.IsValid()\noutput.set(None)\nmapping.set({3: 4})\n", globals);
                {
                    CallbackScope nested;
                    Exec("assert native.read() == 7\n", globals);
                }
                try
                {
                    Exec("class BrokenMessage(Exception):\n def __str__(self):\n  raise ValueError('format failure')\nraise BrokenMessage()\n", globals);
                    assert(false);
                }
                catch (bp::error_already_set const&)
                {
                    auto error = ExceptionHandler::FormatException();
                    assert(error.find("BrokenMessage") != std::string::npos);
                    assert(!PyErr_Occurred());
                }
                Exec("assert native.read() == 7\n", globals);
                // Background Python may run while a callback yields, but cannot touch its native objects.
                PyThreadState* threadState = PyEval_SaveThread();
                std::thread worker([&globals]
                {
                    GILGuard gil;
                    Exec("try:\n native.read()\nexcept RuntimeError:\n pass\nelse:\n raise AssertionError('cross-thread reference accepted')\n", globals);
                });
                worker.join();
                PyEval_RestoreThread(threadState);
                InvalidateBorrowedPointer(&native);
                Exec("try:\n child.read()\nexcept ReferenceError:\n pass\nelse:\n raise AssertionError('removed native reference accepted')\n", globals);
            }
            assert(number == 42 && text == "new" && values == std::set<uint32>({3, 4}));
            assert(output == nullptr && mapping.at(3) == 4 && mapping.size() == 1);
            Exec("for value in (native, child):\n try:\n  value.read()\n except ReferenceError:\n  pass\n else:\n  raise AssertionError('expired native accepted')\n"
                 "for value in (number, text, values):\n try:\n  value.get()\n except ReferenceError:\n  pass\n else:\n  raise AssertionError('expired mutable reference accepted')\n"
                 "assert not opaque.IsValid()\ntry:\n opaque.GetTypeName()\nexcept ReferenceError:\n pass\nelse:\n raise AssertionError('expired opaque reference accepted')\n", globals);
            // Enum refs use a typed copied value instead of aliasing enum storage as an integer.
            enum class TestEnum : uint8 { A = 1, B = 2 };
            TestEnum enumeration = TestEnum::A;
            {
                CallbackScope scope;
                globals["enumeration"] = Converter::ToPython(std::ref(enumeration));
                Exec("enumeration.set(2)\n", globals);
            }
            assert(enumeration == TestEnum::B);
            assert(API::IsNone(Converter::ToPython(static_cast<char const*>(nullptr))));
        }
        ClearBorrowedReferences();
    }
    catch (bp::error_already_set const&)
    {
        PyErr_Print();
        return 1;
    }
    return Py_FinalizeEx() < 0 ? 1 : 0;
}
