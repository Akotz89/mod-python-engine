#include "PythonEngine.h"
#include "PythonAPI.h"

namespace bp = boost::python;
using namespace PyEng::Bridge;

namespace
{
void RegisterEvent(std::string const& eventName, API::Object callback)
{
    if (!sPythonEngine->IsEnabled())
        return;

    if (!sPythonEngine->RegisterSafeEvent(eventName, callback))
    {
        PyErr_SetString(PyExc_RuntimeError, "safe event registration rejected by capability/load policy");
        bp::throw_error_already_set();
    }
}
} // namespace

void export_safe_event_api()
{
    bp::def("RegisterEvent", &RegisterEvent);
}
