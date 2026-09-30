// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// weapon-roll span until link-order evidence names its owner.

#include <rva.h>

#include <Game/Attack.h>
#include <Game/BattleEffect.h>
#include <Game/Clock.h>
#include <Game/FieldSight.h>
#include <Game/Growth.h>
#include <Game/ItemEffect.h>
#include <Game/PartyAction.h>
#include <Util/Range.h>

#include <math.h>

RVA(0x0000a120, 0x142)
b16 RollWeaponCondition(
    Character* attacker,
    Character* target,
    i16 resistance,
    i16 condition,
    i16 mode
) {
    i16 luck;
    i16 roll;
    i16 defense;
    i32 power;
    g_statusCondition = INFLICT_NONE;
    if (!condition) {
        return false;
    }
    if (attacker->lastChange < GetConditionDamageThreshold(target)) {
        return false;
    }
    if (g_actionResult >= BATTLE_ACTION_REFLECTED) {
        return false;
    }
    if (mode && IsFieldConditionRestricted(condition)) {
        return false;
    }
    roll = RandomAverage(0, 20, 0);
    luck = GetStatTotal(attacker, STAT_FORTUNE);
    luck += roll;
    if (luck <= GetStatTotal(target, STAT_FORTUNE)) {
        return false;
    }
    roll = RandomAverage(0, 40, 0);
    defense = GetBattleStatShown(target, BATTLE_STAT_WEAPON_DEFENSE);
    defense *= roll;
    power = GetBattleStatShown(attacker, BATTLE_STAT_WEAPON_POWER);
    ApplyWeaponPowerConditions(attacker, power);
    if (ScaleActionValue(power * 10, resistance, 2) - defense <= 0) {
        return false;
    }
    if (IsConditionResisted(target, g_attackCondition)) {
        return false;
    }
    g_statusCondition = g_attackCondition;
    InflictCondition(g_attackCondition, target);
    return true;
}

RVA(0x0000a270, 0x21c)
i32 ComputeWeaponDamage(Character* attacker, Character* target, i16 result) {
    double power;
    i16 defense;
    double amount;
    i32 facing;
    i32 damage;
    if (!result) {
        return 0;
    }
    power = GetBattleStatShown(attacker, BATTLE_STAT_WEAPON_POWER);
    ApplyWeaponPowerConditions(attacker, power);
    defense = GetBattleStatShown(target, BATTLE_STAT_WEAPON_DEFENSE);
    amount = -(defense * 0.2);
    amount += power;
    if (amount < 0.0) {
        amount = 0.0;
    }
    if (defense) {
        power /= defense;
    }
    if (GetBattleStatShown(attacker, BATTLE_STAT_WEAPON_POWER) >= defense) {
        power += 2.2;
    } else {
        power += 1.0;
    }
    amount = sqrt(amount) * power;
    if (result == BATTLE_ACTION_CRITICAL) {
        amount += attacker->level + 5;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        amount *= 1.2;
    }
    facing = GetCombatantFacingDifference(g_actorId, g_targetId);
    if (facing == FACING_FROM_BEHIND) {
        amount *= 1.5;
    } else if (facing != FACING_FACE_TO_FACE) {
        amount *= 1.2;
    }
    if (result == BATTLE_ACTION_GRAZED) {
        amount *= 0.25;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        amount *= 1.5;
    }
    damage = RoundToInt(amount * 100.0);
    damage = ScaleActionValue(damage, g_attackResistance, 2);
    damage = ScaleByMoonValue(damage, attacker->moonRow, 2);
    damage = RandomPercent(damage, -20, 20);
    damage = ClampInt(damage / 100, 0, 0x7fffffff);
    if (damage <= 0) {
        SetActionResult(attacker, BATTLE_ACTION_NO_EFFECT);
    }
    return damage;
}

RVA(0x0000a490, 0x1f5)
b16 RollWeaponHit(Character* attacker, Character* target, i16 resistance) {
    i32 accuracy;
    i32 evasion;
    i32 attack;
    i32 defense;
    i32 roll;
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
        return true;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == FACING_FROM_BEHIND) {
        SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
        return true;
    }
    accuracy = GetBattleStatShown(attacker, BATTLE_STAT_WEAPON_ACCURACY);
    ApplyAttackAccuracyConditions(attacker, accuracy);
    evasion = GetBattleStatShown(target, BATTLE_STAT_WEAPON_EVASION);
    if (GetAttackRangeExcess(g_actorId, g_targetId) < 0) {
        attack = accuracy * 50;
    } else {
        attack = accuracy * 100;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        attack *= 2;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) != FACING_FACE_TO_FACE) {
        defense = evasion * 75;
    } else {
        defense = evasion * 100;
    }
    if (attack >= defense) {
        roll = defense * RandomAverage(-2, 12, 1);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
            return true;
        }
    } else {
        roll = defense * RandomAverage(0, 15, 0);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, BATTLE_ACTION_SUCCESS);
            return true;
        }
    }
    roll = defense * RandomAverage(0, 7, 0);
    if (attack >= roll) {
        SetActionResult(attacker, BATTLE_ACTION_GRAZED);
        return true;
    }
    SetActionResult(attacker, BATTLE_ACTION_MISSED);
    return false;
}

RVA(0x0000a690, 0x209)
i16 RollExceptionalWeaponAttack(Character* attacker, Character* target, i16 mode, i16 resistance) {
    i32 phase = (g_clock.moonPhase + 13) % 14 + 1;
    i16 modifier;
    i32 attack;
    i32 defense;
    double value;
    if (phase * phase / 4 > RandomUpTo(255) && resistance != 0 && resistance != -6) {
        modifier = GetEquipmentHitModifier(attacker, target);
        if (!mode) {
            value = GetExceptionalAttackLuck(attacker);
            value *= RandomAverage(80, 120, 0);
            value *= 0.01;
            value += modifier;
            attack = RoundToInt(value);
            value = GetExceptionalAttackLuck(target);
            defense = RoundToInt(value * RandomAverage(100, 200, 0) * 0.01);
            if (attack > defense) {
                return SetActionResult(attacker, BATTLE_ACTION_LETHAL);
            }
        }
        attack = GetExceptionalAttackBase(attacker);
        attack += RandomUpTo(7);
        defense = GetExceptionalAttackBase(target);
        defense += RandomUpTo(31);
        if (modifier + attack > defense) {
            return SetActionResult(attacker, BATTLE_ACTION_CRITICAL);
        }
        SetActionResult(attacker, BATTLE_ACTION_MISSED);
    }
    return 0;
}

RVA(0x0000a8a0, 0x179)
b16 ResolveWeaponAttack(Character* attacker, Character* target, i16 mode) {
    i16 result;
    i32 amount = 0;
    ResetActionOutcome();
    g_hpChange = 0;
    g_attackAttribute = GetPickedAttackAttribute(attacker, &g_attackCondition);
    g_attackResistance = GetActionResistance(target, g_attackAttribute, ATTACK_WEAPON, true, false);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, g_attackAttribute);
    result = RollExceptionalWeaponAttack(attacker, target, mode, g_attackResistance);
    if (result != 0) {
        AddTrainingPoints(attacker, BATTLE_GROUP_WEAPON, 1);
    }
    if (result < 5) {
        if (result == 0) {
            result = RollWeaponHit(attacker, target, g_attackResistance);
            attacker->resultFlag = result;
            if (result != 0) {
                AddTrainingPoints(attacker, BATTLE_GROUP_WEAPON, 1);
            }
        } else {
            SetCharacterResult(attacker, result, 1);
        }
        amount = ComputeWeaponDamage(attacker, target, attacker->result);
        SetCharacterChanges(attacker, amount, 0);
    } else if (result == BATTLE_ACTION_LETHAL) {
        amount = 0x7fff;
        SetCharacterChanges(attacker, amount, 0);
        SetFlaggedActionResult(attacker, BATTLE_ACTION_LETHAL);
        AddTrainingPoints(attacker, BATTLE_GROUP_WEAPON, 1);
    }
    ApplyResistanceOutcome(attacker, g_attackResistance, amount);
    return RollWeaponCondition(attacker, target, g_attackResistance, g_attackCondition, mode);
}
