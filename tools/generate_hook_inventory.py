#!/usr/bin/env python3
"""Generate the mechanically derived mod-python-engine hook inventory.

This tool parses the hook declarations and the native forwarding adapters.  It
does not import or execute the embedded module and is safe to run in the
source/static verification lane.
"""

from __future__ import annotations

import argparse
import csv
import io
import re
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path


DECLARATION_RE = re.compile(
    r"DEFINE_GAME_HOOK\(\s*(?P<category>[A-Z]+)\s*,\s*"
    r"(?P<name>[A-Z0-9_]+)\s*,\s*(?P<identifier>\d+)\s*\)"
)
FORWARD_RE = re.compile(
    r"TRIGGER_(?P<category>[A-Z]+)_HOOK(?:_RESULT)?\(\s*(?P<name>[A-Z0-9_]+)"
)

AUTHORITATIVE_BOOLEAN_HOOKS = {
    "CREATURE_ON_GOSSIP_HELLO",
    "CREATURE_ON_GOSSIP_SELECT",
    "CREATURE_ON_GOSSIP_SELECT_CODE",
    "CREATURE_ON_QUEST_ACCEPT",
    "CREATURE_ON_QUEST_COMPLETE",
    "CREATURE_ON_QUEST_REWARD",
    "CREATURE_ON_QUEST_SELECT",
    "GAMEOBJECT_ON_GOSSIP_HELLO",
    "GAMEOBJECT_ON_GOSSIP_SELECT",
    "ITEM_ON_CAST_COMBAT_SPELL",
    "ITEM_ON_EXPIRE",
    "ITEM_ON_QUEST_ACCEPT",
    "ITEM_ON_REMOVE",
    "ITEM_ON_USE",
    "UNIT_CAN_SET_PHASE_MASK",
    "UNIT_IF_NORMAL_REACTION",
    "UNIT_IS_CUSTOM_BUILD_VALUES_UPDATE",
    "UNIT_SHOULD_TRACK_VALUES_UPDATE_POS_BY_INDEX",
}

MUTABLE_AUTHORITY_HOOKS = {
    "PLAYER_ON_GIVE_EXP",
    "PLAYER_ON_MONEY_CHANGED",
    "UNIT_MODIFY_HEAL_RECEIVED",
    "UNIT_MODIFY_MELEE_DAMAGE",
    "UNIT_MODIFY_SPELL_DAMAGE_TAKEN",
    "UNIT_ON_BEFORE_ROLL_MELEE_OUTCOME_AGAINST",
    "UNIT_ON_PATCH_VALUES_UPDATE",
}

SESSION_HOOKS = {
    "ACCOUNT_ON_ACCOUNT_LOGIN",
    "ACCOUNT_ON_FAILED_ACCOUNT_LOGIN",
    "PLAYER_ON_LOGIN",
    "PLAYER_ON_LOGOUT",
}


@dataclass(frozen=True)
class HookRecord:
    category: str
    name: str
    identifier: int
    declaration: str
    forwarders: tuple[str, ...]
    signatures: tuple[str, ...] = ()
    return_semantics: tuple[str, ...] = ()

    @property
    def full_name(self) -> str:
        return f"{self.category}_{self.name}"

    @property
    def forwarded(self) -> bool:
        return bool(self.forwarders)

    @property
    def risk_tags(self) -> str:
        if not self.forwarded:
            return "unwired"

        tags: set[str] = set()
        if self.category != "ACCOUNT":
            tags.add("borrowed-native-object")
        if self.full_name in AUTHORITATIVE_BOOLEAN_HOOKS:
            tags.add("authoritative-boolean")
        if self.full_name in MUTABLE_AUTHORITY_HOOKS:
            tags.add("mutable-authority")
        if self.full_name in SESSION_HOOKS:
            tags.add("session-path")
        if self.category == "UNIT":
            tags.add("combat-or-hot-path")
        if self.full_name == "PLAYER_ON_LOOT_ITEM":
            tags.add("economy-path")
        return ";".join(sorted(tags))


def _line_number(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def collect_inventory(repo_root: Path) -> list[HookRecord]:
    declarations: dict[tuple[str, str], tuple[int, str]] = {}
    identifiers: dict[tuple[str, int], str] = {}

    for path in sorted((repo_root / "src/Hooks/Defines").glob("*Hook.h")):
        text = path.read_text(encoding="utf-8")
        for match in DECLARATION_RE.finditer(text):
            category = match.group("category")
            name = match.group("name")
            identifier = int(match.group("identifier"))
            location = f"{path.relative_to(repo_root)}:{_line_number(text, match.start())}"
            key = (category, name)
            if key in declarations:
                raise ValueError(f"duplicate hook declaration: {category}_{name}")
            id_key = (category, identifier)
            if id_key in identifiers:
                raise ValueError(
                    f"duplicate hook id {category}:{identifier}: "
                    f"{identifiers[id_key]} and {name}"
                )
            declarations[key] = (identifier, location)
            identifiers[id_key] = name

    forwarded: dict[tuple[str, str], list[str]] = defaultdict(list)
    signatures: dict[tuple[str, str], set[str]] = defaultdict(set)
    semantics: dict[tuple[str, str], set[str]] = defaultdict(set)
    for path in sorted((repo_root / "src/Scripts").glob("*Script.cpp")):
        text = path.read_text(encoding="utf-8")
        for match in FORWARD_RE.finditer(text):
            key = (match.group("category"), match.group("name"))
            location = f"{path.relative_to(repo_root)}:{_line_number(text, match.start())}"
            forwarded[key].append(location)
            methods = list(re.finditer(r"^    (.+?) override\n    \{", text[:match.start()], re.MULTILINE))
            if methods:
                signature = methods[-1].group(1)
                signatures[key].add(signature[signature.index("("):signature.rindex(")") + 1])
            else:
                signature = ""
            call_end = text.find(";", match.start())
            call = text[match.start():call_end]
            if "_HOOK_RESULT(" not in call:
                policy = "notification; return ignored"
            else:
                arguments = call[call.index("(") + 1:].split(",")
                default_index = 2 if key[0] in {"CREATURE", "GAMEOBJECT", "ITEM"} else 1
                default = arguments[default_index].strip().rstrip(")")
                policy = {
                    "true": "permit; AND; default True",
                    "false": "handled/override; OR; default False",
                    "Optional<bool>{}": "optional decision; default None",
                }.get(default, "last valid return; default " + default)
            semantics[key].add(policy)

    missing = sorted(set(forwarded) - set(declarations))
    if missing:
        names = ", ".join(f"{category}_{name}" for category, name in missing)
        raise ValueError(f"forwarded hooks without declarations: {names}")

    records = [
        HookRecord(
            category=category,
            name=name,
            identifier=identifier,
            declaration=location,
            forwarders=tuple(sorted(forwarded.get((category, name), []))),
            signatures=tuple(sorted(signatures.get((category, name), []))),
            return_semantics=tuple(sorted(semantics.get((category, name), []))),
        )
        for (category, name), (identifier, location) in declarations.items()
    ]
    return sorted(records, key=lambda record: (record.category, record.identifier, record.name))


def render_csv(records: list[HookRecord]) -> str:
    output = io.StringIO(newline="")
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(
        (
            "hook_name",
            "category",
            "hook_id",
            "declared",
            "forwarded",
            "availability",
            "risk_tags",
            "declaration",
            "forwarders",
            "callback_signature",
            "return_semantics",
        )
    )
    for record in records:
        writer.writerow(
            (
                record.full_name,
                record.category,
                record.identifier,
                "yes",
                "yes" if record.forwarded else "no",
                "available" if record.forwarded else "missing-native-forwarder",
                record.risk_tags,
                record.declaration,
                ";".join(record.forwarders),
                ";".join(record.signatures),
                ";".join(record.return_semantics),
            )
        )
    return output.getvalue()


def render_markdown(records: list[HookRecord]) -> str:
    categories: dict[str, list[HookRecord]] = defaultdict(list)
    for record in records:
        categories[record.category].append(record)

    forwarded_total = sum(record.forwarded for record in records)
    lines = [
        "# Generated hook inventory",
        "",
        "This file is generated by `tools/generate_hook_inventory.py`. Do not edit it by hand.",
        "The declaration headers and native `TRIGGER_*_HOOK` call sites are the evidence inputs.",
        "",
        f"- Declared hook names: {len(records)}",
        f"- Actually forwarded hook names/IDs: {forwarded_total}",
        f"- Missing native forwarders: {len(records) - forwarded_total}",
        "",
        "These are source forwarding counts, not runtime/gameplay acceptance claims.",
        "Every name is available to ordinary scripts; there is no capability profile gate.",
        "",
        "| Category | Declared | Native forwarder | Missing |",
        "| --- | ---: | ---: | ---: |",
    ]
    for category in sorted(categories):
        rows = categories[category]
        forwarded = sum(record.forwarded for record in rows)
        lines.append(
            f"| {category} | {len(rows)} | {forwarded} | {len(rows) - forwarded} |"
        )
    lines.extend(
        [
            "",
            "Per-hook source locations are in `docs/hook-inventory.csv`.",
            "Callback arguments and returns are documented in [hook-signatures.md](hook-signatures.md).",
            "A callback returning None preserves native behavior. Permit hooks combine boolean",
            "results with AND; handled/override hooks combine results with OR. Non-const reference",
            "parameters are mutable callback-scoped objects. They must not be used after callback return.",
            "",
            "The old `PLAYER_GET_CUSTOM_GET_ARENA_TEAM_ID` and",
            "`PLAYER_GET_CUSTOM_ARENA_PERSONAL_RATING` names are compatibility aliases for the",
            "current arena getters, with `(player, slot, result)` mutable-result arguments.",
            "The misspelled upstream `ITEM_ON_GOSSIPE_SELECT` and `_CODE` names remain supported.",
            "",
        ]
    )
    return "\n".join(lines)


def render_signatures(records: list[HookRecord]) -> str:
    lines = [
        "# Native Python hook signatures",
        "",
        "Generated from the native adapters by `tools/generate_hook_inventory.py`.",
        "Arguments appear in callback order; C++ type names identify the native object or value.",
        "Non-const `&` arguments are mutable wrappers: use `.get()` and `.set(value)`.",
        "Native objects are borrowed handles; mutable references are callback-scoped and expire after return.",
        "Null native pointers become `None`. `MAP_ON_BEFORE_CREATE_INSTANCE_SCRIPT` receives a mutable",
        "`InstanceScript*` output reference for its second argument, or `None` when no output location exists.",
        "",
        "`None` preserves the native default. Permit callbacks combine boolean results with AND;",
        "handled/override callbacks combine them with OR. This table records native dispatch semantics,",
        "not proof that every event has been observed in the deployed realm.",
        "",
        "Map registry entries are published after native data initialization. `MAP_ON_CREATE_MAP` runs",
        "outside the creation mutex exactly once. Existing lookups may return while that notification",
        "is still running; nested map creation and same-map lookup remain available. Map iteration",
        "workers run over pointer snapshots, so newly created maps cannot invalidate active iterators.",
        "Shutdown retains the core's quiesced raw-pointer lifetime model. Finite maps created during",
        "destruction callbacks are drained; trusted scripts that continually recreate maps can prolong",
        "shutdown, just as any unbounded in-process callback can. Callbacks are not forcibly preempted.",
        "",
        "## Compatibility and native callsite behavior",
        "",
        "- The two `PLAYER_GET_CUSTOM_*ARENA*` names forward on the current arena getters, with",
        "  `(player, slot, result)` arguments; mutate `result` rather than return a number.",
        "- The upstream misspelling `ITEM_ON_GOSSIPE_SELECT` and `_CODE` remains supported.",
        "- Item remove/expiry/quest-accept and creature/gameobject quest-accept results handle the",
        "  script fallback. Their native gameplay callers ignore the result; returning True does not",
        "  prevent item destruction or undo quest acceptance. Use the corresponding `PLAYER_CAN_*`",
        "  permission hooks for actual pre-action decisions.",
        "- Creature/gameobject quest-reward runs after reward application; True handles follow-up dialogue/AI.",
        "- Creature quest-select and quest-complete now dispatch from the validated questgiver requests.",
        "- `PLAYER_ON_BEFORE_INIT_TALENT_FOR_LEVEL` can change the pending talent-point count; its level",
        "  parameter is a snapshot in the current core and does not change the character's level.",
        "- The legacy weapon-dependent aura permit hook runs when the current core selects active damage",
        "  aura contributions for an equipped attack slot (`apply=True`). Removal recomputes the remaining",
        "  active contributions rather than replaying the removed aura with `apply=False`.",
        "- World before-config, after-config, and custom-table callbacks on initial boot occur before scripts",
        "  are loaded. Registered callbacks receive subsequent native events/reloads; initial startup is",
        "  `WORLD_ON_STARTUP` after script activation.",
        "",
        "| Hook | Callback arguments | Return semantics |",
        "| --- | --- | --- |",
    ]
    for record in records:
        arguments = " / ".join(record.signatures).replace("|", "\\|")
        policies = "; ".join(record.return_semantics)
        lines.append(f"| `{record.full_name}` | `{arguments}` | {policies} |")
    lines.append("")
    return "\n".join(lines)


def render_header(records: list[HookRecord]) -> str:
    rows = "\n".join(
        "        GeneratedHookRecord{"
        f'"{record.full_name}", Category::{record.category}, {record.identifier}, '
        f"{'true' if record.forwarded else 'false'}"
        "},"
        for record in records
    )
    return f"""// Generated by tools/generate_hook_inventory.py. Do not edit.
#ifndef MOD_PYTHON_ENGINE_GENERATED_HOOK_INVENTORY_H
#define MOD_PYTHON_ENGINE_GENERATED_HOOK_INVENTORY_H

#include "HookInfo.h"
#include <array>
#include <string_view>

namespace PyEng::Hooks
{{
    struct GeneratedHookRecord
    {{
        std::string_view name;
        Category category;
        uint8 identifier;
        bool forwarded;
    }};

    inline constexpr std::array<GeneratedHookRecord, {len(records)}> GeneratedHookInventory = {{{{
{rows}
    }}}};

    [[nodiscard]] inline constexpr GeneratedHookRecord const* FindGeneratedHook(std::string_view name) noexcept
    {{
        for (GeneratedHookRecord const& record : GeneratedHookInventory)
            if (record.name == name)
                return &record;
        return nullptr;
    }}
}} // namespace PyEng::Hooks

#endif // MOD_PYTHON_ENGINE_GENERATED_HOOK_INVENTORY_H
"""


def generated_outputs(repo_root: Path) -> dict[Path, str]:
    records = collect_inventory(repo_root)
    return {
        repo_root / "docs/hook-inventory.csv": render_csv(records),
        repo_root / "docs/hook-inventory.md": render_markdown(records),
        repo_root / "docs/hook-signatures.md": render_signatures(records),
        repo_root / "src/Hooks/GeneratedHookInventory.h": render_header(records),
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--check", action="store_true", help="fail if generated files differ")
    parser.add_argument("--repo-root", type=Path, default=Path(__file__).resolve().parents[1])
    args = parser.parse_args()

    mismatches: list[str] = []
    for path, expected in generated_outputs(args.repo_root.resolve()).items():
        if args.check:
            actual = path.read_text(encoding="utf-8") if path.exists() else None
            if actual != expected:
                mismatches.append(str(path.relative_to(args.repo_root)))
        else:
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text(expected, encoding="utf-8")

    if mismatches:
        print("generated hook inventory is stale: " + ", ".join(mismatches))
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
