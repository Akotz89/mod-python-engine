"""Check native routing seams that previously advertised unavailable callbacks."""
from pathlib import Path
import re
import sys
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT.parent.parent
if not (SOURCE / 'src/server/game').is_dir():
    SOURCE = ROOT.parent / 'azerothcore-wotlk'
CORE = SOURCE / 'src/server/game/Scripting/ScriptDefines'
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

    def test_global_handled_hooks_keep_fallback_and_cast_permission_is_and(self):
        cases = (
            ('CreatureScript.cpp', 'bool ScriptMgr::OnQuestSelect(Player* player, Creature* creature', 'OnCreatureQuestSelect'),
            ('CreatureScript.cpp', 'bool ScriptMgr::OnQuestComplete(Player* player, Creature* creature', 'OnCreatureQuestComplete'),
            ('CreatureScript.cpp', 'bool ScriptMgr::OnQuestReward(Player* player, Creature* creature', 'OnCreatureQuestReward'),
            ('GameObjectScript.cpp', 'bool ScriptMgr::OnQuestReward(Player* player, GameObject* go', 'OnGameObjectQuestReward'),
            ('AllItemScript.cpp', 'bool ScriptMgr::OnQuestAccept(Player* player, Item* item', 'OnItemQuestAccept'),
            ('AllItemScript.cpp', 'bool ScriptMgr::OnItemExpire(Player* player', 'OnItemExpire'),
            ('AllItemScript.cpp', 'bool ScriptMgr::OnItemRemove(Player* player', 'OnItemRemove'),
        )
        for dispatcher, signature, hook in cases:
            text = (CORE / dispatcher).read_text()
            body = self._function_body(text, signature)
            self.assertIn(f'script->{hook}(', body)
            handled = re.search(r'if \((?:ret|handled) && \*(?:ret|handled)\)\s*return true;', body)
            self.assertIsNotNone(handled, hook)
            fallback = body.find('GetScriptById(')
            self.assertGreaterEqual(fallback, 0, hook)
            self.assertLess(body.index(f'script->{hook}('), handled.start(), hook)
            self.assertLess(handled.end(), fallback, hook)
            self.assertIn('return tempScript ?', body[fallback:], hook)

        for header, hooks in (
            ('AllCreatureScript.h', ('OnCreatureQuestSelect', 'OnCreatureQuestComplete', 'OnCreatureQuestReward')),
            ('AllGameObjectScript.h', ('OnGameObjectQuestReward',)),
            ('AllItemScript.h', ('OnItemQuestAccept', 'OnItemExpire', 'OnItemRemove')),
        ):
            declarations = (CORE / header).read_text()
            for hook in hooks:
                declaration = re.search(rf'virtual bool {hook}\([^;]+\{{ return false; \}}', declarations)
                self.assertIsNotNone(declaration, hook)

        item_dispatch = (CORE / 'AllItemScript.cpp').read_text()
        cast_body = self._function_body(item_dispatch, 'bool ScriptMgr::OnCastItemCombatSpell(')
        self.assertIn('bool canCast = true;', cast_body)
        self.assertIn('ExecuteScript<AllItemScript>', cast_body)
        self.assertIn('script->CanItemCastCombatSpell(', cast_body)
        self.assertIn('canCast = script->CanItemCastCombatSpell(player, victim, spellInfo, item) && canCast;', cast_body)
        self.assertLess(cast_body.index('if (!canCast)'), cast_body.index('GetScriptById('))
        self.assertIn('return tempScript ? tempScript->OnCastItemCombatSpell', cast_body)
        self.assertRegex((CORE / 'AllItemScript.h').read_text(),
                         r'virtual bool CanItemCastCombatSpell\([^;]+\{ return true; \}')

    def test_quest_lifecycle_callers_follow_validation_and_preserve_chicken_reset(self):
        handler = (SOURCE / 'src/server/game/Handlers/QuestHandler.cpp').read_text()
        select = self._function_body(handler, 'void WorldSession::HandleQuestgiverQueryQuestOpcode(')
        self.assertLess(select.index('if (!_player->CanTakeQuest(quest, true))'),
                        select.index('sScriptMgr->OnQuestSelect('))
        self.assertLess(select.index('if (!_player->CanInteractWithQuestGiver(object))'),
                        select.index('sScriptMgr->OnQuestSelect('))
        self.assertLess(select.index('sScriptMgr->OnQuestSelect('),
                        select.index('if (quest->IsAutoAccept()'))
        self.assertLess(select.index('sScriptMgr->OnQuestSelect('),
                        select.index('SendQuestGiverQuestDetails('))

        complete = self._function_body(handler, 'void WorldSession::HandleQuestgiverCompleteQuest(')
        for validation in ('!object || !object->hasInvolvedQuest(questId)',
                           '!_player->CanInteractWithQuestGiver(object)',
                           '!_player->CanSeeStartQuest(quest)'):
            self.assertLess(complete.index(validation), complete.index('sScriptMgr->OnQuestComplete('))
        self.assertLess(complete.index('sScriptMgr->OnQuestComplete('),
                        complete.index('SendQuestGiverRequestItems('))
        self.assertLess(complete.index('sScriptMgr->OnQuestComplete('),
                        complete.index('SendQuestGiverOfferReward('))

        reward = self._function_body(handler, 'void WorldSession::HandleQuestgiverChooseRewardOpcode(')
        for validation in ('!object || !object->hasInvolvedQuest(questId)',
                           '!_player->CanInteractWithQuestGiver(object)',
                           '_player->CanRewardQuest(quest, reward, true)'):
            self.assertLess(reward.index(validation), reward.index('_player->RewardQuest('))
        self.assertLess(reward.index('_player->RewardQuest('), reward.index('sScriptMgr->OnQuestReward('))

        special = (SOURCE / 'src/server/scripts/World/npcs_special.cpp').read_text()
        chicken_class = special[special.index('class npc_chicken_cluck :'):
                                special.index('/*######\n## npc_dancing_flames')]
        self.assertNotIn('OnQuestComplete(', chicken_class)
        reward = self._function_body(chicken_class,
            'bool OnQuestReward(Player* /*player*/, Creature* creature, Quest const* quest, uint32 /*opt*/)')
        self.assertIn('quest->GetQuestId() == QUEST_CLUCK', reward)
        self.assertIn('->Reset();', reward)
        self.assertRegex(reward, r'return false;\s*}$')

    @staticmethod
    def _function_body(text, signature):
        start = text.index(signature)
        opening = text.index('{', start)
        depth = 0
        for index in range(opening, len(text)):
            if text[index] == '{':
                depth += 1
            elif text[index] == '}':
                depth -= 1
                if depth == 0:
                    return text[start:index + 1]
        raise AssertionError(f'unterminated function: {signature}')

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
