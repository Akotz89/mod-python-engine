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
        static ChatCommandTable pythonCommandTable = []
        {
            ChatCommandTable commands = {
                { "status", HandlePythonStatusCommand, SEC_ADMINISTRATOR, Console::Yes },
                { "reload", HandlePythonReloadCommand, SEC_ADMINISTRATOR, Console::Yes }
            };

            // Do not expose administrative exec at all in the default profile.
            // The command appears only when both unsafe development and the
            // separate exec switch passed their initialization gates.
            if (sPythonEngine->IsUnsafeDevelopmentProfile() && sPythonEngine->IsExecCommandAllowed())
                commands.push_back({ "exec", HandlePythonExecCommand, SEC_ADMINISTRATOR, Console::Yes });
            return commands;
        }();

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
        handler->PSendSysMessage("Capability Profile: {}", sPythonEngine->GetCapabilityProfileName());
        handler->PSendSysMessage("Reloading: {}", sPythonEngine->IsReloading() ? "Yes" : "No");
        handler->PSendSysMessage("Configured Scripts Path: {}", sConfigMgr->GetOption<std::string>("Python.ScriptsPath", ""));

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

        bool success = sPythonEngine->LoadScripts();

        // End reload (atomically clear reloading flag)
        sPythonEngine->EndReload();

        if (!success)
        {
            handler->SendErrorMessage("Python script reload failed; the last accepted registry remains active");
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
