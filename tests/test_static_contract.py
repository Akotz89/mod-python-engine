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


class HookInventoryTests(unittest.TestCase):
    def test_inventory_is_truthful_and_generated_files_are_current(self) -> None:
        records = collect_inventory(REPO_ROOT)
        self.assertGreaterEqual(len(records), 271)
        self.assertTrue(all(record.forwarded for record in records))
        self.assertEqual(len(records), len({record.full_name for record in records}))
        parsed = list(csv.DictReader(io.StringIO(render_csv(records))))
        self.assertEqual(len(records), len(parsed))
        self.assertEqual({"available"}, {row["availability"] for row in parsed})
        for path, expected in generated_outputs(REPO_ROOT).items():
            self.assertEqual(expected, path.read_text(encoding="utf-8"), path.name)

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


if __name__ == "__main__":
    unittest.main()
