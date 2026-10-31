#include "HookMacros.h"
#include "ScriptMgr.h"
#include "Player.h"
#include "Creature.h"
#include "GameObject.h"
#include "Spell.h"
#include "Item.h"
#include "ArenaTeam.h"
#include "Battleground.h"
#include "AchievementMgr.h"
#include "WorldSession.h"
#include "WorldSocket.h"
#include "Map.h"
#include "Guild.h"
#include "Group.h"
#include "Channel.h"
#include "Pet.h"
#include "ObjectMgr.h"
#include "LootMgr.h"
#include "SpellAuraEffects.h"

class PythonUnitScript : public UnitScript
{
public:
    PythonUnitScript() : UnitScript("PythonUnitScript") {}

    void OnHeal(Unit* healer, Unit* reciever, uint32& gain) override
    {
        TRIGGER_UNIT_HOOK(ON_HEAL, healer, reciever, std::ref(gain));
    }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        TRIGGER_UNIT_HOOK(ON_DAMAGE, attacker, victim, std::ref(damage));
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        TRIGGER_UNIT_HOOK(MODIFY_PERIODIC_DAMAGE_AURAS_TICK, target, attacker, std::ref(damage), spellInfo);
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        TRIGGER_UNIT_HOOK(MODIFY_MELEE_DAMAGE, target, attacker, std::ref(damage));
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        TRIGGER_UNIT_HOOK(MODIFY_SPELL_DAMAGE_TAKEN, target, attacker, std::ref(damage), spellInfo);
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        TRIGGER_UNIT_HOOK(MODIFY_HEAL_RECEIVED, target, healer, std::ref(heal), spellInfo);
    }

    void OnBeforeRollMeleeOutcomeAgainst(Unit const* attacker, Unit const* victim, WeaponAttackType attType, int32& attackerMaxSkillValueForLevel, int32& victimMaxSkillValueForLevel, int32& attackerWeaponSkill, int32& victimDefenseSkill, int32& crit_chance, int32& miss_chance, int32& dodge_chance, int32& parry_chance, int32& block_chance ) override
    {
        TRIGGER_UNIT_HOOK(ON_BEFORE_ROLL_MELEE_OUTCOME_AGAINST, attacker, victim, attType, std::ref(attackerMaxSkillValueForLevel), std::ref(victimMaxSkillValueForLevel), std::ref(attackerWeaponSkill), std::ref(victimDefenseSkill), std::ref(crit_chance), std::ref(miss_chance), std::ref(dodge_chance), std::ref(parry_chance), std::ref(block_chance));
    }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        TRIGGER_UNIT_HOOK(ON_AURA_APPLY, unit, aura);
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        TRIGGER_UNIT_HOOK(ON_AURA_REMOVE, unit, aurApp, mode);
    }

    bool IfNormalReaction(Unit const* unit, Unit const* target, ReputationRank& repRank) override
    {
        return TRIGGER_UNIT_HOOK_RESULT(IF_NORMAL_REACTION, true, unit, target, std::ref(repRank));
    }

    bool CanSetPhaseMask(Unit const* unit, uint32 newPhaseMask, bool update) override
    {
        return TRIGGER_UNIT_HOOK_RESULT(CAN_SET_PHASE_MASK, true, unit, newPhaseMask, update);
    }

    bool IsCustomBuildValuesUpdate(Unit const* unit, uint8 updateType, ByteBuffer& fieldBuffer, Player const* target, uint16 index) override
    {
        return TRIGGER_UNIT_HOOK_RESULT(IS_CUSTOM_BUILD_VALUES_UPDATE, false, unit, updateType, std::ref(fieldBuffer), target, index);
    }

    bool ShouldTrackValuesUpdatePosByIndex(Unit const* unit, uint8 updateType, uint16 index) override
    {
        return TRIGGER_UNIT_HOOK_RESULT(SHOULD_TRACK_VALUES_UPDATE_POS_BY_INDEX, false, unit, updateType, index);
    }

    void OnPatchValuesUpdate(Unit const* unit, ByteBuffer& valuesUpdateBuf, BuildValuesCachePosPointers& posPointers, Player* target) override
    {
        TRIGGER_UNIT_HOOK(ON_PATCH_VALUES_UPDATE, unit, std::ref(valuesUpdateBuf), std::ref(posPointers), target);
    }

    void OnUnitUpdate(Unit* unit, uint32 diff) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_UPDATE, unit, diff);
    }

    void OnDisplayIdChange(Unit* unit, uint32 displayId) override
    {
        TRIGGER_UNIT_HOOK(ON_DISPLAYID_CHANGE, unit, displayId);
    }

    void OnUnitEnterEvadeMode(Unit* unit, uint8 evadeReason) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_ENTER_EVADE_MODE, unit, evadeReason);
    }

    void OnUnitEnterCombat(Unit* unit, Unit* victim) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_ENTER_COMBAT, unit, victim);
    }

    void OnUnitDeath(Unit* unit, Unit* killer) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_DEATH, unit, killer);
    }

    void OnUnitSetShapeshiftForm(Unit* unit, uint8 form) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_SET_SHAPESHIFT_FORM, unit, form);
    }

    void OnUnitExitCombat(Unit* unit) override
    {
        TRIGGER_UNIT_HOOK(ON_UNIT_EXIT_COMBAT, unit);
    }
    uint32 DealDamage(Unit* attacker, Unit* victim, uint32 damage, DamageEffectType damageType) override
    {
        return TRIGGER_UNIT_HOOK_RESULT(DEAL_DAMAGE, damage, attacker, victim, damage, damageType);
    }

};

void AddSC_PythonUnitScripts()
{
    new PythonUnitScript();
}
