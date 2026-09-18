from __future__ import annotations

import csv
import io
import shutil
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from generate_hook_inventory import collect_inventory, generated_outputs, render_csv  # noqa: E402
from static_contract import (  # noqa: E402
    UNSAFE_ACKNOWLEDGEMENT,
    ScriptRootError,
    StagedReloadModel,
    resolve_script_files,
    select_capability_surface,
)


class HookInventoryTests(unittest.TestCase):
    def test_inventory_is_truthful_and_generated_files_are_current(self) -> None:
        records = collect_inventory(REPO_ROOT)
        self.assertEqual(271, len(records))
        self.assertEqual(49, sum(record.forwarded for record in records))
        self.assertEqual(178, sum(record.category == "PLAYER" for record in records))
        self.assertEqual(
            7,
            sum(record.category == "PLAYER" and record.forwarded for record in records),
        )

        parsed = list(csv.DictReader(io.StringIO(render_csv(records))))
        self.assertEqual(271, len(parsed))
        self.assertEqual(
            {"disabled-by-policy"},
            {row["safe_static_v1"] for row in parsed if row["forwarded"] == "yes"},
        )
        self.assertEqual(
            {"unsupported"},
            {row["safe_static_v1"] for row in parsed if row["forwarded"] == "no"},
        )

        for path, expected in generated_outputs(REPO_ROOT).items():
            self.assertEqual(expected, path.read_text(encoding="utf-8"), path.name)

    def test_unsafe_profile_still_rejects_unwired_declarations(self) -> None:
        rows = csv.DictReader(io.StringIO(render_csv(collect_inventory(REPO_ROOT))))
        for row in rows:
            expected = "available" if row["forwarded"] == "yes" else "unsupported"
            self.assertEqual(expected, row["unsafe_development_v1"])

    def test_unknown_forwarder_and_duplicate_id_are_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            mutated = Path(temporary)
            shutil.copytree(REPO_ROOT / "src/Hooks", mutated / "src/Hooks")
            shutil.copytree(REPO_ROOT / "src/Scripts", mutated / "src/Scripts")
            player_script = mutated / "src/Scripts/PlayerScript.cpp"
            player_script.write_text(
                player_script.read_text(encoding="utf-8")
                + "\nTRIGGER_PLAYER_HOOK(NOT_DECLARED, nullptr);\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "without declarations"):
                collect_inventory(mutated)

        with tempfile.TemporaryDirectory() as temporary:
            mutated = Path(temporary)
            shutil.copytree(REPO_ROOT / "src/Hooks", mutated / "src/Hooks")
            shutil.copytree(REPO_ROOT / "src/Scripts", mutated / "src/Scripts")
            account_hooks = mutated / "src/Hooks/Defines/AccountHook.h"
            account_hooks.write_text(
                account_hooks.read_text(encoding="utf-8")
                + "\nDEFINE_GAME_HOOK(ACCOUNT, DUPLICATE_ID, 0)\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "duplicate hook id"):
                collect_inventory(mutated)


class ScriptRootTests(unittest.TestCase):
    def test_requires_an_absolute_root_and_rejects_parent_traversal(self) -> None:
        with self.assertRaises(ScriptRootError):
            resolve_script_files("python_scripts")
        with self.assertRaises(ScriptRootError):
            resolve_script_files("/tmp/allowed/../escaped")

    def test_rejects_root_and_entry_symlinks(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            base = Path(temporary)
            real_root = base / "real"
            real_root.mkdir()
            (real_root / "ok.py").write_text("pass\n", encoding="utf-8")
            root_link = base / "root-link"
            root_link.symlink_to(real_root, target_is_directory=True)
            with self.assertRaises(ScriptRootError):
                resolve_script_files(str(root_link))

            outside = base / "outside.py"
            outside.write_text("pass\n", encoding="utf-8")
            (real_root / "linked.py").symlink_to(outside)
            with self.assertRaises(ScriptRootError):
                resolve_script_files(str(real_root))

            (real_root / "linked.py").unlink()
            outside_directory = base / "outside-directory"
            outside_directory.mkdir()
            (real_root / "linked-directory").symlink_to(
                outside_directory, target_is_directory=True
            )
            with self.assertRaises(ScriptRootError):
                resolve_script_files(str(real_root))

    def test_returns_only_deterministic_regular_python_files(self) -> None:
        with tempfile.TemporaryDirectory() as temporary:
            root = Path(temporary)
            (root / "b.py").write_text("pass\n", encoding="utf-8")
            (root / "a.py").write_text("pass\n", encoding="utf-8")
            (root / "ignored.txt").write_text("ignored\n", encoding="utf-8")
            nested = root / "nested"
            nested.mkdir()
            (nested / "c.py").write_text("pass\n", encoding="utf-8")

            files = resolve_script_files(str(root))
            self.assertEqual(
                ["a.py", "b.py", "nested/c.py"],
                [path.relative_to(root).as_posix() for path in files],
            )

    def test_cpp_source_has_no_current_working_directory_fallback(self) -> None:
        engine = (REPO_ROOT / "src/PythonEngine.cpp").read_text(encoding="utf-8")
        self.assertNotIn("fs::current_path", engine)
        self.assertIn("fs::canonical(configuredRoot", engine)
        self.assertIn("fs::is_symlink(status)", engine)
        self.assertIn("Python.ScriptsPath must be absolute", engine)


class ReloadContractTests(unittest.TestCase):
    def test_success_publishes_complete_candidate(self) -> None:
        model = StagedReloadModel(registry={"old": ["callback"]}, namespace={"old": 1})

        def first(registry: dict[str, list[str]], namespace: dict[str, object]) -> None:
            registry["first"] = ["callback"]
            namespace["first"] = 1

        def second(registry: dict[str, list[str]], namespace: dict[str, object]) -> None:
            registry["second"] = ["callback"]
            namespace["second"] = 2

        self.assertTrue(model.reload({"02-second.py": second, "01-first.py": first}))
        self.assertEqual({"first": ["callback"], "second": ["callback"]}, model.registry)
        self.assertEqual({"first": 1, "second": 2}, model.namespace)

    def test_failure_preserves_last_accepted_registry_and_namespace(self) -> None:
        original_registry = {"old": ["callback"]}
        original_namespace = {"old": 1}
        model = StagedReloadModel(
            registry=original_registry.copy(), namespace=original_namespace.copy()
        )

        def partial(registry: dict[str, list[str]], namespace: dict[str, object]) -> None:
            registry["partial"] = ["callback"]
            namespace["partial"] = True

        def fail(_registry: dict[str, list[str]], _namespace: dict[str, object]) -> None:
            raise RuntimeError("candidate failure")

        self.assertFalse(model.reload({"01-partial.py": partial, "02-fail.py": fail}))
        self.assertEqual(original_registry, model.registry)
        self.assertEqual(original_namespace, model.namespace)

    def test_cpp_source_contains_staging_and_no_pre_reload_clear(self) -> None:
        engine = (REPO_ROOT / "src/PythonEngine.cpp").read_text(encoding="utf-8")
        command = (REPO_ROOT / "src/Scripts/CommandScript.cpp").read_text(encoding="utf-8")
        self.assertIn("HookRegistry candidateHookMap", engine)
        self.assertIn("API::Object candidateNamespace", engine)
        self.assertIn(
            "API::ExecFile(scriptFile.string(), candidateNamespace, candidateNamespace)",
            engine,
        )
        self.assertIn("hookMap.swap(candidateHookMap)", engine)
        self.assertIn("std::swap(main_namespace, candidateNamespace)", engine)
        self.assertIn("initializationThread != std::this_thread::get_id()", engine)
        self.assertIn("bp::throw_error_already_set()", (
            REPO_ROOT / "src/Bindings/Functions/HookAPI.cpp"
        ).read_text(encoding="utf-8"))
        self.assertIn("last accepted registry and namespace were preserved", engine)
        reload_body = command.split("HandlePythonReloadCommand", 1)[1].split(
            "HandlePythonExecCommand", 1
        )[0]
        self.assertNotIn("ClearHooks", reload_body)


class CapabilityProfileTests(unittest.TestCase):
    def test_safe_surface_filters_every_native_authority_even_if_exec_is_set(self) -> None:
        surface = select_capability_surface(allow_exec=True)
        self.assertEqual("safe-static-v1", surface.profile)
        self.assertFalse(surface.hook_registration)
        self.assertFalse(surface.native_wrappers)
        self.assertFalse(surface.mutable_bindings)
        self.assertFalse(surface.exec_command)

    def test_safe_composed_surface_is_typed_and_non_authoritative(self) -> None:
        for profile in ("population-v1", "safe-composed-v1"):
            surface = select_capability_surface(profile, allow_exec=True)
            self.assertFalse(surface.hook_registration)
            self.assertFalse(surface.native_wrappers)
            self.assertFalse(surface.mutable_bindings)
            self.assertFalse(surface.exec_command)
            self.assertTrue(surface.safe_event_dispatch)
            self.assertTrue(surface.population_bindings)
            self.assertTrue(surface.party_bindings)

    def test_unsafe_surface_requires_every_acknowledgement_gate(self) -> None:
        for allow_unsafe, acknowledgement in (
            (False, ""),
            (True, ""),
            (False, UNSAFE_ACKNOWLEDGEMENT),
            (True, "almost-correct"),
        ):
            with self.assertRaises(ValueError):
                select_capability_surface(
                    "unsafe-development-v1",
                    allow_unsafe=allow_unsafe,
                    acknowledgement=acknowledgement,
                    allow_exec=True,
                )

        surface = select_capability_surface(
            "unsafe-development-v1",
            allow_unsafe=True,
            acknowledgement=UNSAFE_ACKNOWLEDGEMENT,
            allow_exec=True,
        )
        self.assertTrue(surface.hook_registration)
        self.assertTrue(surface.native_wrappers)
        self.assertTrue(surface.mutable_bindings)
        self.assertTrue(surface.exec_command)

        with self.assertRaises(ValueError):
            select_capability_surface("unsafe-development")

    def test_disabled_defaults_and_unsafe_triple_gate(self) -> None:
        config = (REPO_ROOT / "conf/mod_python_engine.conf.dist").read_text(encoding="utf-8")
        engine = (REPO_ROOT / "src/PythonEngine.cpp").read_text(encoding="utf-8")
        self.assertIn("Python.Enabled = 0", config)
        self.assertIn('Python.CapabilityProfile = "safe-static-v1"', config)
        self.assertIn('safe-composed-v1', config)
        self.assertIn('Python.SafeEvent.MaxMilliseconds = 25', config)
        self.assertIn("Python.AllowUnsafeDevelopmentProfile = 0", config)
        self.assertIn('Python.UnsafeDevelopmentAcknowledgement = ""', config)
        self.assertIn("Python.AllowExecCommand = 0", config)
        self.assertIn('GetOption<bool>("Python.Enabled", false)', engine)
        self.assertIn(
            'GetOption<std::string>("Python.CapabilityProfile",',
            engine,
        )
        self.assertIn('"unsafe-development-v1"', engine)
        self.assertIn('"CPI-223-UNSAFE-DEVELOPMENT-ONLY"', engine)
        self.assertIn('"Python.AllowUnsafeDevelopmentProfile", false', engine)

    def test_safe_module_exports_only_immutable_constants(self) -> None:
        module = (REPO_ROOT / "src/Bindings/PythonModule.cpp").read_text(encoding="utf-8")
        safe_scope = module.split("CREATE_REGISTRY_SCOPE(SafeExports)", 1)[1].split(
            "CREATE_REGISTRY_SCOPE(UnsafeExports)", 1
        )[0]
        self.assertIn("export_game_constants", safe_scope)
        self.assertNotIn("export_hook_api", safe_scope)
        self.assertNotIn("export_mutable_types", safe_scope)
        self.assertNotIn("export_player_class", safe_scope)
        self.assertNotIn("export_unit_class", safe_scope)
        self.assertIn("IsUnsafeDevelopmentProfile", module)
        self.assertIn("export_safe_event_api", module)
        self.assertIn("export_party_api", module)

        party = (REPO_ROOT / "src/Bindings/PartyModule.cpp").read_text(encoding="utf-8")
        for name in (
            "party_read_current",
            "party_read_inventory",
            "party_equip",
            "party_trade",
            "party_sell_vendor",
            "party_request_item_use",
            "party_send_mail_batch",
        ):
            self.assertIn(f'def("{name}"', party)
        events = (REPO_ROOT / "src/Events/SafeEvent.h").read_text(encoding="utf-8")
        self.assertIn('"party.vendor_open"', events)
        self.assertIn('"party.mailbox_open"', events)

        bridge = (REPO_ROOT / "src/Party/PartyBridge.cpp").read_text(encoding="utf-8")
        self.assertIn("std::atomic<Bridge*> Provider", bridge)
        self.assertIn("RegisterBridge", bridge)
        self.assertIn("UnregisterBridge", bridge)
        self.assertIn("compare_exchange_strong", bridge)
        self.assertIn("the required party provider is not registered", party)
        engine = (REPO_ROOT / "src/PythonEngine.cpp").read_text(encoding="utf-8")
        dispatch = engine.split("bool PythonEngine::DispatchSafeEvent", 1)[1].split(
            "bool PythonEngine::RegisterHook", 1
        )[0]
        self.assertIn("!IsSafeEventName(eventName)", dispatch)

        command = (REPO_ROOT / "src/Scripts/CommandScript.cpp").read_text(encoding="utf-8")
        self.assertIn("IsUnsafeDevelopmentProfile() && sPythonEngine->IsExecCommandAllowed()", command)
        self.assertIn('commands.push_back({ "exec"', command)

    def test_forbidden_authority_dependencies_are_absent(self) -> None:
        forbidden = (
            "PopulationDirector",
            "Playerbot",
            "RandomPlayerbotMgr",
            "PlayerbotsDatabase",
            "WorldSession",
            "LoginDatabase",
            "CharacterDatabase",
            "WorldDatabase",
            "mod-game-state",
            "MultiBot",
        )
        sources = "\n".join(
            path.read_text(encoding="utf-8")
            for path in sorted((REPO_ROOT / "src").rglob("*"))
            if path.is_file()
        )
        for token in forbidden:
            self.assertNotIn(token, sources)


if __name__ == "__main__":
    unittest.main()
