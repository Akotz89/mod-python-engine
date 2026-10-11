"""Check native routing seams that previously advertised unavailable callbacks."""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
CORE = ROOT.parent.parent / 'src/server/game/Scripting/ScriptDefines'
sys.path.insert(0, str(ROOT / 'tools'))
from generate_hook_inventory import collect_inventory


class NativeForwardingTests(unittest.TestCase):
    def test_every_declared_hook_has_a_native_forwarder(self):
        rows = collect_inventory(ROOT)
        self.assertFalse([row.full_name for row in rows if not row.forwarded])
        self.assertEqual(
            {'ACCOUNT', 'ACHIEVEMENT', 'ARENA', 'ARENATEAM', 'CREATURE',
             'GAMEOBJECT', 'ITEM', 'MAP', 'PLAYER', 'SERVER', 'UNIT', 'WORLD'},
            {row.category for row in rows},
        )

    def test_entity_callbacks_use_global_routing(self):
        for entity in ('Creature', 'GameObject', 'Item'):
            text = (ROOT / f'src/Scripts/{entity}Script.cpp').read_text()
            self.assertIn(f'class Python{entity}Script : public All{entity}Script', text)
            self.assertNotIn('IsDatabaseBound()', text)

    def test_new_handled_seams_preserve_entity_fallbacks(self):
        for header, dispatcher, hooks in (
            ('AllCreatureScript.h', 'CreatureScript.cpp',
             ('OnCreatureQuestSelect', 'OnCreatureQuestComplete', 'OnCreatureQuestReward')),
            ('AllGameObjectScript.h', 'GameObjectScript.cpp', ('OnGameObjectQuestReward',)),
            ('AllItemScript.h', 'AllItemScript.cpp',
             ('OnItemQuestAccept', 'OnItemExpire', 'OnItemRemove', 'CanItemCastCombatSpell')),
        ):
            declarations = (CORE / header).read_text()
            dispatch = (CORE / dispatcher).read_text()
            for hook in hooks:
                self.assertIn(f'virtual bool {hook}(', declarations)
                self.assertIn(f'script->{hook}(', dispatch)
            self.assertIn('GetScriptById(', dispatch)

    def test_heal_damage_and_periodic_damage_are_live_mutable_references(self):
        text = (ROOT / 'src/Scripts/UnitScript.cpp').read_text()
        for hook, reference in (('ON_HEAL', 'gain'), ('ON_DAMAGE', 'damage'),
                                ('MODIFY_PERIODIC_DAMAGE_AURAS_TICK', 'damage')):
            match = re.search(r'TRIGGER_UNIT_HOOK\(' + hook + r', ([^;]+)', text)
            self.assertIsNotNone(match)
            self.assertIn(f'std::ref({reference})', match[1])
        self.assertIn('TRIGGER_UNIT_HOOK_RESULT(CAN_SET_PHASE_MASK, true,', text)

    def test_numeric_damage_hook_and_map_pointer_output(self):
        unit = (ROOT / 'src/Scripts/UnitScript.cpp').read_text()
        self.assertIn('TRIGGER_UNIT_HOOK_RESULT(DEAL_DAMAGE, damage,', unit)
        map_script = (ROOT / 'src/Scripts/MapScript.cpp').read_text()
        self.assertIn('std::ref(*instanceData)', map_script)
        self.assertIn('instanceMap, nullptr, load, data, completedEncounterMask', map_script)

    def test_world_shutdown_delivers_callback_before_finalizing_python(self):
        text = (ROOT / 'src/Scripts/WorldScript.cpp').read_text()
        self.assertLess(text.index('TRIGGER_WORLD_HOOK(ON_AFTER_UNLOAD_ALL_MAPS)'),
                        text.index('sPythonEngine->Shutdown()'))

    def test_current_native_arena_getters_forward_legacy_aliases(self):
        text = (ROOT / 'src/Scripts/PlayerScript.cpp').read_text()
        for hook in ('GET_CUSTOM_GET_ARENA_TEAM_ID', 'GET_CUSTOM_ARENA_PERSONAL_RATING'):
            self.assertIn(f'TRIGGER_PLAYER_HOOK({hook}, player, slot, std::ref(result))', text)


if __name__ == '__main__':
    unittest.main()
