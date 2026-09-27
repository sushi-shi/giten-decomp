// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// item-roll span until link-order evidence names its owner.

#include <rva.h>

#include <Game/Attack.h>
#include <Game/BattleEffect.h>
#include <Game/Clock.h>
#include <Game/Field.h>
#include <Game/FieldSight.h>
#include <Game/Growth.h>
#include <Game/ItemEffect.h>
#include <Game/ItemRecord.h>
#include <Game/PartyAction.h>
#include <Game/Skill.h>
#include <Util/Range.h>

#include <math.h>

RVA(0x0000b0f0, 0x22c)
i16 ResolveItemAttack(Character* attacker, Character* target, i16 sameSide) {
    i16 attribute;
    i32 accuracy;
    i32 defense;
    i32 itemValue;
    i16 value;
    i16 roll;
    g_attackResistance = GetItemResistance(target, attacker->pickTarget, 1, sameSide, &attribute);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, attribute);
    if (g_attackResistance == -6) {
        SetResistanceResult(attacker, -6, 10);
        return 0;
    }
    SetActionResult(attacker, 3);
    if (g_attackResistance <= -4) {
        return 1;
    }
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        return 1;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == 2) {
        return 1;
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
    itemValue = GetLoadedRecord(attacker->pickTarget)->params[0xc];
    itemValue *= 100;
    itemValue += accuracy;
    value = WearSkillValue(itemValue);
    if (accuracy >= defense) {
        accuracy = ScaleActionValue(value + defense * 4, g_attackResistance, 4);
        roll = RandomAverage(0, 14, 1);
    } else {
        accuracy = ScaleActionValue(value * 4, g_attackResistance, 4);
        roll = RandomAverage(0, 15, 0);
    }
    if (accuracy > defense * roll) {
        return 1;
    }
    SetActionResult(attacker, 0);
    return 0;
}

RVA(0x0000b320, 0x1a7)
i32 ComputeItemDamage(Character* attacker, Character* target, i16 hit) {
    i16 power;
    i16 defense;
    double amount;
    i32 facing;
    i32 damage;
    if (!hit) {
        return 0;
    }
    power = GetItemDamagePower(GetLoadedRecord(attacker->pickTarget))
            + GetBattleStatShown(attacker, BATTLE_STAT_MAGIC_POWER);
    power = WearSkillValue(power);
    amount = power;
    defense = GetBattleStatShown(target, BATTLE_STAT_MAGIC_DEFENSE);
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
    damage = ScaleActionValue(damage, g_attackResistance, 2);
    damage = ScaleByMoonValue(damage, attacker->moonRow, 2);
    damage = RandomPercent(damage, -20, 20);
    damage = ClampInt(damage / 100, 0, 0x7fffffff);
    if (damage == 0) {
        SetActionResult(attacker, 1);
    }
    return damage;
}

RVA(0x0000b4d0, 0x14d)
i16 RollItemCondition(Character* attacker, Character* target, i16 resistance, i16 condition) {
    i16 roll;
    i16 luck;
    i16 defense;
    i32 value;
    i16 power;
    g_statusCondition = 0;
    if (!condition) {
        return 0;
    }
    if (attacker->lastChange < GetConditionDamageThreshold(target)) {
        return 0;
    }
    if (g_actionResult >= 7) {
        return 0;
    }
    if (g_targetId >= 0 && IsFieldModeAtLeast(0) && IsFieldConditionRestricted(condition)) {
        return 0;
    }
    roll = RandomAverage(0, 20, 0);
    luck = GetStatTotal(attacker, STAT_FORTUNE);
    luck += roll;
    if (luck <= GetStatTotal(target, STAT_FORTUNE)) {
        return 0;
    }
    roll = RandomAverage(0, 30, 0);
    defense = GetBattleStatShown(target, 5);
    defense *= roll;
    value = GetItemHitPower(GetLoadedRecord(attacker->pickTarget));
    value += GetRecordValue();
    power = WearSkillValue(value);
    if (ScaleActionValue(power * 10, resistance, 2) - defense <= 0) {
        return 0;
    }
    if (IsConditionResisted(target, condition)) {
        return 0;
    }
    g_statusCondition = condition;
    InflictCondition(condition, target);
    return 1;
}

RVA(0x0000b620, 0xaa)
i16 RunItemAttack(Character* attacker, Character* target) {
    i16 hit;
    i32 damage;
    hit = ResolveItemAttack(attacker, target, 0);
    if (hit) {
        AddTrainingPoints(attacker, 2, 3);
    }
    damage = ComputeItemDamage(attacker, target, hit);
    if (hit && !damage) {
        g_actionResult = 0;
        hit = 0;
    }
    attacker->resultFlag = hit;
    attacker->result = g_actionResult;
    ApplySkillResistanceOutcome(attacker, damage);
    return RollItemCondition(
        attacker,
        target,
        g_attackResistance,
        GetItemInflictedCondition(GetLoadedRecord(attacker->pickTarget))
    );
}
