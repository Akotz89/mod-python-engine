#include "PythonEngine.h"
#include "PythonAPI.h"

namespace bp = boost::python;
using namespace PyEng::Bridge;

namespace
{
    void RejectRegistration(char const* eventName)
    {
        std::string message = "Hook registration rejected (unknown event or non-callable callback): ";
        message += eventName ? eventName : "<null>";
        PyErr_SetString(PyExc_RuntimeError, message.c_str());
        bp::throw_error_already_set();
    }

    /**
     * @brief Register global hook (no entry ID)
     *
     * @param eventName Hook name string (ex., "PLAYER_ON_LOGIN")
     * @param callback Python callable object
     */
    void RegisterGlobal(char const* eventName, API::Object callback)
    {
        if (!sPythonEngine->IsEnabled())
            return;

        if (!sPythonEngine->RegisterHook(std::string(eventName), callback, 0))
            RejectRegistration(eventName);
    }

    /**
     * @brief Register entry-specific hook
     *
     * @param eventName Hook name string (ex., "CREATURE_ON_GOSSIP_HELLO")
     * @param callback Python callable object
     * @param entryId Specific entry ID (creature/item/spell/etc.)
     */
    void RegisterEntry(char const* eventName, API::Object callback, uint32 entryId)
    {
        if (!sPythonEngine->IsEnabled())
            return;

        if (!sPythonEngine->RegisterHook(std::string(eventName), callback, entryId))
            RejectRegistration(eventName);
    }

    void Schedule(API::Object callback)
    {
        if (!sPythonEngine->Schedule(callback))
        {
            PyErr_SetString(PyExc_RuntimeError, "Schedule requires an enabled engine, a callable, and no concurrent reload");
            bp::throw_error_already_set();
        }
    }

} // anonymous namespace

/**
 * @brief Export Hook API Functions to Python
 */
void export_hook_api()
{
    bp::def("Schedule", &Schedule);

    // Usage: azerothcore.Register("PLAYER_ON_LOGIN", on_login)
    bp::def("Register", &RegisterGlobal);

    // Usage: azerothcore.Register("CREATURE_ON_GOSSIP_HELLO", on_gossip_hello, 12345)
    bp::def("Register", &RegisterEntry);
}
