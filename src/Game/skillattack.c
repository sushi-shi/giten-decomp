// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// skill-roll span until link-order evidence names its owner.

#include <rva.h>

#include <Game/Attack.h>
#include <Game/BattleEffect.h>
#include <Game/Clock.h>
#include <Game/Field.h>
#include <Game/FieldSight.h>
#include <Game/Growth.h>
#include <Game/ItemEffect.h>
#include <Game/PartyAction.h>
#include <Game/Skill.h>
#include <Util/Range.h>

#include <math.h>

RVA(0x0000aa20, 0x22c)
b16 RollSkillHit(Character* attacker, Character* target, i16 sameSide) {
    i16 attribute;
    i32 accuracy;
    i32 defense;
    i32 skillValue;
    i16 value;
    i32 roll;
    g_attackResistance = GetSkillResistance(target, attacker->pickTarget, 1, sameSide, &attribute);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, attribute);
    if (g_attackResistance == -6) {
        SetResistanceResult(attacker, -6, 10);
        return false;
    }
    SetActionResult(attacker, 3);
    if (g_attackResistance <= -4) {
        return true;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        return true;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == 2) {
        return true;
    }
    accuracy = GetRecordValue();
    ApplyAttackAccuracyConditions(attacker, accuracy);
    defense = GetBattleStatShown(target, BATTLE_STAT_MAGIC_EVASION);
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        accuracy *= 200;
    } else {
        accuracy *= 100;
    }
    defense *= 100;
    if (GetCombatantFacingDifference(g_actorId, g_targetId) != 0) {
        accuracy = accuracy * 150 / 100;
    }
    skillValue = GetSkillValueA(GetCachedSkill(attacker->pickTarget));
    skillValue *= 100;
    value = WearSkillValue(skillValue + accuracy);
    skillValue = value;
    if (accuracy >= defense) {
        accuracy = ScaleActionValue(skillValue + defense * 4, g_attackResistance, 4);
        roll = defense * RandomAverage(0, 14, 1);
    } else {
        accuracy = ScaleActionValue(skillValue * 4, g_attackResistance, 4);
        roll = defense * RandomAverage(0, 15, 0);
    }
    if (accuracy > roll) {
        return true;
    }
    SetActionResult(attacker, 0);
    return false;
}

RVA(0x0000ac50, 0x1ce)
i32 ComputeSkillDamage(Character* attacker, Character* target, i16 hit) {
    SkillHeader* skill;
    i16 power;
    i16 defense;
    double amount;
    i32 facing;
    i32 damage;
    if (!hit) {
        return 0;
    }
    skill = GetCachedSkill(attacker->pickTarget);
    power = WearSkillValue(GetSkillValueB(skill) + GetBattleStatShown(attacker, 15));
    defense = GetBattleStatShown(target, 17);
    amount = power;
    if (power < defense) {
        amount *= 0.8;
    }
    amount = power * (amount - sqrt(defense));
    if (defense) {
        amount /= defense;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        amount *= 1.2;
    }
    facing = GetCombatantFacingDifference(g_actorId, g_targetId);
    if (facing == 2) {
        amount *= 1.5;
    } else if (facing != 0) {
        amount *= 1.2;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        amount *= 1.5;
    }
    damage = RoundToInt(amount * 100.0);
    if (CountElementGuards(target, GetSkillAttackAttribute(skill))) {
        damage /= 2;
    }
    damage = ScaleActionValue(damage, g_attackResistance, 2);
    damage = ScaleByMoonValue(damage, attacker->moonRow, 2);
    damage = RandomPercent(damage, -20, 20);
    damage = ClampInt(damage / 100, 0, 0x7fffffff);
    if (damage == 0) {
        SetActionResult(attacker, 1);
    }
    return damage;
}

RVA(0x0000ae20, 0x14d)
b16 RollSkillCondition(Character* attacker, Character* target, i16 resistance, i16 condition) {
    i16 roll;
    i16 luck;
    i16 defense;
    i32 value;
    i16 power;
    g_statusCondition = 0;
    if (!condition) {
        return false;
    }
    if (attacker->lastChange < GetConditionDamageThreshold(target)) {
        return false;
    }
    if (g_actionResult >= 7) {
        return false;
    }
    if (g_targetId >= 0 && IsFieldModeAtLeast(false) && IsFieldConditionRestricted(condition)) {
        return false;
    }
    roll = RandomAverage(0, 20, 0);
    luck = GetStatTotal(attacker, STAT_FORTUNE);
    luck += roll;
    if (luck <= GetStatTotal(target, STAT_FORTUNE)) {
        return false;
    }
    roll = RandomAverage(0, 30, 0);
    defense = GetBattleStatShown(target, 5);
    defense *= roll;
    value = GetSkillValueA(GetCachedSkill(attacker->pickTarget));
    value += GetRecordValue();
    power = WearSkillValue(value);
    if (ScaleActionValue(power * 10, resistance, 2) - defense <= 0) {
        return false;
    }
    if (IsConditionResisted(target, condition)) {
        return false;
    }
    g_statusCondition = condition;
    InflictCondition(condition, target);
    return true;
}

RVA(0x0000af70, 0x38)
i16 ApplySkillResistanceOutcome(Character* attacker, i32 amount) {
    ApplyResistanceOutcome(attacker, g_attackResistance, amount);
    if (g_actionResult == 7) {
        return -1;
    }
    return g_actionResult < 8;
}

RVA(0x0000afb0, 0x13e)
b16 ResolveSkillAttack(Character* attacker, Character* target) {
    u16 mode = GetSkillMode(attacker->pickTarget);
    i16 hit;
    i32 damage;
    if (mode == 1) {
        if (g_targetId >= 0) {
            return ResolveWeaponAttack(attacker, target, IsFieldModeAtLeast(false));
        } else {
            return ResolveWeaponAttack(attacker, target, 0);
        }
    }
    if (mode == 2) {
        if (g_targetId >= 0) {
            return ResolveGunAttack(attacker, target, IsFieldModeAtLeast(false));
        } else {
            return ResolveGunAttack(attacker, target, 0);
        }
    }
    hit = RollSkillHit(attacker, target, 0);
    if (hit) {
        AddTrainingPoints(attacker, 2, 3);
    }
    damage = ComputeSkillDamage(attacker, target, hit);
    if (hit && !damage) {
        g_actionResult = 0;
        hit = 0;
    }
    attacker->resultFlag = hit;
    attacker->result = g_actionResult;
    ApplySkillResistanceOutcome(attacker, damage);
    return RollSkillCondition(
        attacker,
        target,
        g_attackResistance,
        GetSkillInflictedCondition(GetCachedSkill(attacker->pickTarget))
    );
}
