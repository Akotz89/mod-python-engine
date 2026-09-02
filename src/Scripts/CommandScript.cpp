#include "StringHelper.h"
#include "PythonEngine.h"
#include "ScriptMgr.h"
#include "Chat.h"

using namespace Acore::ChatCommands;
using namespace PyEng::Utilities;

class PythonCommandScript : public CommandScript
{
public:
    PythonCommandScript() : CommandScript("PythonCommandScript") {}

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable pythonCommandTable = {
            { "status", HandlePythonStatusCommand, SEC_ADMINISTRATOR, Console::Yes },
            { "reload", HandlePythonReloadCommand, SEC_ADMINISTRATOR, Console::Yes },
            { "exec",   HandlePythonExecCommand,   SEC_ADMINISTRATOR, Console::Yes }
        };

        static ChatCommandTable commandTable = {
            { "python", pythonCommandTable }
        };

        return commandTable;
    }

    static bool HandlePythonStatusCommand(ChatHandler* handler, char const* /*args*/)
    {
        bool enabled = sPythonEngine->IsEnabled();
        if (!enabled)
        {
            handler->SendErrorMessage("Python Engine is disabled");
            return false;
        }

        handler->PSendSysMessage("Python Engine: {}", sPythonEngine->IsBusy() ? "Busy" : "Ready");
        handler->PSendSysMessage("Reloading: {}", sPythonEngine->IsReloading() ? "Yes" : "No");
        handler->PSendSysMessage("Scripts Path: {}", sConfigMgr->GetOption<std::string>("Python.ScriptsPath", "python_scripts"));

        return true;
    }

    static bool HandlePythonReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        if (!sPythonEngine->IsEnabled())
        {
            handler->SendErrorMessage("Error: Python Engine is disabled");
            return false;
        }

        // Begin reload (atomically set reloading flag)
        if (!sPythonEngine->BeginReload())
        {
            handler->SendErrorMessage("Error: Reload already in progress");
            return false;
        }

        handler->SendSysMessage("Reloading Python scripts...");

        sPythonEngine->ClearHooks();
        bool success = sPythonEngine->LoadScripts();

        // End reload (atomically clear reloading flag)
        sPythonEngine->EndReload();

        if (!success)
        {
            handler->SendErrorMessage("Python script reload failed; all Python hooks are disabled");
            return false;
        }

        handler->SendSysMessage("Python scripts reloaded successfully");

        return true;
    }

    static bool HandlePythonExecCommand(ChatHandler* handler, char const* args)
    {
        if (!sPythonEngine->IsExecCommandAllowed())
        {
            handler->SendErrorMessage("Error: Python exec command is disabled");
            return false;
        }

        if (!sPythonEngine->IsEnabled())
        {
            handler->SendErrorMessage("Error: Python Engine is disabled");
            return false;
        }

        if (!*args)
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        std::string code = std::string(StringHelper::Unquote(StringHelper::Trim(args)));
        if (code.empty())
        {
            handler->SendErrorMessage(LANG_CMD_SYNTAX);
            return false;
        }

        auto result = sPythonEngine->ExecuteCode(code);

        handler->SendSysMessage(result ? "Output:" : "Error:");

        if (result.HasMessage())
        {
            auto lines = StringHelper::SplitLines(result.GetMessage(), 250);

            constexpr size_t MAX_OUTPUT_LINES = 10;
            size_t linesToShow = std::min(lines.size(), MAX_OUTPUT_LINES);

            for (size_t i = 0; i < linesToShow; ++i)
                handler->SendSysMessage(lines[i]);

            if (lines.size() > MAX_OUTPUT_LINES)
                handler->PSendSysMessage("... ({} lines truncated)", lines.size() - MAX_OUTPUT_LINES);
        }
        else
            handler->SendSysMessage("No output...");

        handler->SetSentErrorMessage(!result);
        return static_cast<bool>(result);
    }
};

void AddSC_PythonCommandScripts()
{
    new PythonCommandScript();
}
