#include "AutoRegistryMgr.h"
#include "PopulationBridge.h"
#include "PythonEngine.h"
#include "ScriptMgr.h"
#include "Config.h"

// Create Scripts scope for the forwards
CREATE_REGISTRY_SCOPE(Scripts);

// Forward declarations into the Scripts scope
REGISTER_TO_SCOPE(Scripts, AddSC_PythonAccountScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonCommandScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonCreatureScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonGameObjectScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonItemScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonPlayerScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonUnitScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonAchievementScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonArenaScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonArenaTeamScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonMapScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonServerScripts);
REGISTER_TO_SCOPE(Scripts, AddSC_PythonWorldScripts);


class PythonLoader : public WorldScript
{
public:
    PythonLoader() : WorldScript("PythonLoader") {}

    void OnBeforeConfigLoad(bool reload) override
    {
        if (!reload)
        {
            // Initialize the Engine (creates the Python VM, registers bindings)
            // @todo create separate method for reload(?)
            sPythonEngine->Initialize();
        }
    }

    void OnBeforeWorldInitialized() override
    {
        if (!sPythonEngine->IsEnabled())
            return;

        // Immediately load all scripts from Python.ScriptsPath
        if (!sPythonEngine->LoadScripts())
        {
            LOG_ERROR("module.python", "Python scripts were not activated during world initialization.");
            PyEng::Population::FailClosed();
        }
    }

    void OnUpdate(uint32 diff) override
    {
        sPythonEngine->RunScheduledCallbacks();
        if (sConfigMgr->GetOption<bool>("Python.Population.Enabled", false))
            sPythonEngine->RunPopulationTick(diff);
    }


};

void Addmod_python_engineScripts()
{
    new PythonLoader();

    EXECUTE_REGISTRY_SCOPE(Scripts);
}
