// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// combat-roll helpers until link-order evidence names their owner.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/Attack.h>
#include <Game/BattleEffect.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/Field.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/FieldView.h>
#include <Game/GameState.h>
#include <Game/Growth.h>
#include <Game/ItemEffect.h>
#include <Game/ItemRecord.h>
#include <Game/PartyAction.h>
#include <Game/Skill.h>
#include <Game/SkillUse.h>
#include <Mem/Handle.h>
#include <Util/Range.h>

#include <math.h>
#include <string.h>

DATA(0x00078488)
static i16 s_gunPower[16];
DATA(0x000784a8)
static i16 s_gunRounds[16];
DATA(0x000784d8)
i16 g_attackResistance;
DATA(0x000784dc)
i16 g_attackAttribute;
DATA(0x000784e0)
i16 g_attackCondition;
// @identity-TODO: the penalties use total stats 8 and 6 respectively;
// the stat names are not yet recovered.
DATA(0x000784e4)
static i16 s_gunPenaltyA;
DATA(0x000784e8)
static i16 s_gunPenaltyB;
DATA(0x000784ec)
static i16 s_gunRoundPower;
DATA(0x000784f0)
static i16 s_gunBasePower;
DATA(0x0007851c)
static i32 s_gunDistribution;

RVA(0x00008450, 0x2f)
i16 GetCombatantSideRelation(void) {
    if (g_actorId < 0 && g_targetId < 0) {
        return -1;
    }
    if (g_actorId >= 0 && g_targetId >= 0) {
        return 1;
    }
    return 0;
}

static __inline void AddArmorSlotHitModifier(ItemSlot* slot, i16* modifier) {
    if (slot->item != -1) {
        *modifier += GetArmorHitModifier(GetLoadedRecord(slot->item));
    }
}

RVA(0x00008480, 0x11b)
i16 GetEquipmentHitModifier(Character* attacker, Character* target) {
    i16 modifier = 0;
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[0], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[1], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[2], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[3], &modifier);
    AddArmorSlotHitModifier(&GetCharacterEquipment(target)[4], &modifier);
    modifier = -modifier;
    if (attacker->pickTarget >= 1) {
        modifier += GetLoadedRecord(attacker->pickTarget)->params[0x1c];
    }
    return modifier;
}

static __inline i16 GetCombatantFacing(i16 id) {
    if (id < 0) {
        return g_field.pos.direction;
    }
    return GetFieldActor(id)->facing;
}

RVA(0x000085a0, 0x65)
i16 GetCombatantFacingDifference(i16 first, i16 second) {
    i16 direction;
    i16 difference;
    if (first < 0 && second < 0) {
        return 0;
    }
    direction = GetCombatantFacing(first);
    difference = GetCombatantFacing(second) - direction - 2;
    return difference & 3;
}

RVA(0x00008610, 0x59)
i16 GetCombatantDistance(i16 first, i16 second) {
    MapCoord from;
    MapCoord to;
    from = GetFieldTargetCoord(first);
    to = GetFieldTargetCoord(second);
    return GridDistance(from.x, from.y, to.x, to.y);
}

RVA(0x00008670, 0x51)
i16 GetCombatantAttackRange(i16 id) {
    Character* actor = GetCombatant(id);
    if (actor->pickTarget < 1) {
        return 1;
    }
    if (!actor->pickTargetHigh) {
        return GetItemAttackRange(GetLoadedRecord(actor->pickTarget));
    }
    return GetSkillAttackRange(actor->pickTarget);
}

RVA(0x000086d0, 0x27)
i16 GetAttackRangeExcess(i16 first, i16 second) {
    i16 distance = GetCombatantDistance(first, second);
    return distance - GetCombatantAttackRange(first);
}

RVA(0x00008700, 0x209)
i16 RollExceptionalAttack(Character* attacker, Character* target, i16 mode, i16 resistance) {
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
                SetActionResult(attacker, 5);
                return 5;
            }
        }
        attack = GetExceptionalAttackBase(attacker);
        attack += RandomUpTo(7);
        defense = GetExceptionalAttackBase(target);
        defense += RandomUpTo(31);
        if (modifier + attack > defense) {
            SetActionResult(attacker, 4);
            return 4;
        }
        SetActionResult(attacker, 0);
    }
    return 0;
}

RVA(0x00008910, 0x207)
i16 RollGunHit(Character* attacker, Character* target, i16 resistance) {
    i32 accuracy;
    i32 evasion;
    i32 attack;
    i32 defense;
    i32 roll;
    if (GetPickBlockingCondition(GetCharacterConditions(target))) {
        SetActionResult(attacker, 3);
        return 1;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) == 2) {
        SetActionResult(attacker, 3);
        return 1;
    }
    accuracy = GetBattleStatShown(attacker, BATTLE_STAT_GUN_ACCURACY);
    ApplyAttackAccuracyConditions(attacker, accuracy);
    evasion = GetBattleStatShown(target, BATTLE_STAT_GUN_EVASION);
    if (HasCondition(GetCharacterConditions(target), 19)) {
        evasion *= 2;
    }
    if (GetAttackRangeExcess(g_actorId, g_targetId) < 0) {
        attack = accuracy * 50;
    } else {
        attack = accuracy * 100;
    }
    if (GetCombatantDistance(g_actorId, g_targetId) == 0) {
        attack *= 2;
    }
    if (GetCombatantFacingDifference(g_actorId, g_targetId) != 0) {
        defense = evasion * 75;
    } else {
        defense = evasion * 100;
    }
    if (attack >= defense) {
        roll = defense * RandomAverage(-2, 12, 1);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, 3);
            return 1;
        }
    } else {
        roll = defense * RandomAverage(0, 15, 0);
        attack *= 8;
        if (attack >= roll) {
            SetActionResult(attacker, 3);
            return 1;
        }
    }
    roll = defense * RandomAverage(0, 7, 0);
    if (attack >= roll) {
        SetActionResult(attacker, 2);
        return 1;
    }
    SetActionResult(attacker, 0);
    return 0;
}

RVA(0x00008b20, 0x17)
i16 GetGunAttackPower(Character* attacker) {
    if (s_gunPower[0]) {
        return s_gunPower[0];
    }
    return GetBattleStatShown(attacker, BATTLE_STAT_GUN_POWER);
}

RVA(0x00008b40, 0x1e9)
i32 ComputeGunDamage(Character* attacker, Character* target, i16 result) {
    i16 power;
    i16 defense;
    double ratio;
    double amount;
    i32 facing;
    i32 damage;
    if (!result) {
        return 0;
    }
    power = GetGunAttackPower(attacker);
    defense = GetBattleStatShown(target, BATTLE_STAT_GUN_DEFENSE);
    amount = defense;
    amount *= 0.2;
    amount = -amount;
    amount += power;
    if (amount < 0.0) {
        amount = 0.0;
    }
    ratio = power;
    if (defense) {
        ratio /= defense;
    }
    if (power >= defense) {
        ratio += 2.2;
    } else {
        ratio += 1.0;
    }
    amount = sqrt(amount) * ratio;
    if (result == 4) {
        amount += attacker->level + 5;
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
    if (result == 2) {
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
    if (damage == 0) {
        SetActionResult(attacker, 1);
    }
    return damage;
}

RVA(0x00008d30, 0x124)
i16 RollGunCondition(Character* attacker, Character* target, i16 resistance, i16 condition) {
    i16 luck;
    i16 roll;
    i16 defense;
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
    roll = RandomAverage(0, 40, 0);
    defense = GetBattleStatShown(target, 11);
    defense *= roll;
    if (ScaleActionValue(GetBattleStatShown(attacker, 9) * 10, resistance, 2) - defense <= 0) {
        return 0;
    }
    if (IsConditionResisted(target, g_attackCondition)) {
        return 0;
    }
    g_statusCondition = g_attackCondition;
    InflictCondition(g_attackCondition, target);
    return 1;
}

RVA(0x00008e60, 0x176)
i16 ResolveGunAttack(Character* attacker, Character* target, i16 mode) {
    i16 result;
    i32 amount = 0;
    g_statusCondition = 0;
    g_actionResult = 0;
    g_hpChange = 0;
    g_attackAttribute = GetPickedAttackAttribute(attacker, &g_attackCondition);
    g_attackResistance = GetActionResistance(target, g_attackAttribute, 2, 1, 0);
    g_attackResistance = ScaleDamageByEquipment(attacker, g_attackResistance, g_attackAttribute);
    result = RollExceptionalAttack(attacker, target, mode, g_attackResistance);
    if (result != 0) {
        AddTrainingPoints(attacker, 1, 1);
    }
    if (result < 5) {
        if (result == 0) {
            result = RollGunHit(attacker, target, g_attackResistance);
            attacker->resultFlag = result;
            if (result != 0) {
                AddTrainingPoints(attacker, 1, 1);
            }
        } else {
            SetCharacterResult(attacker, result, 1);
        }
        amount = ComputeGunDamage(attacker, target, attacker->result);
        SetCharacterChanges(attacker, amount, 0);
    } else if (result == 5) {
        amount = 0x7fff;
        SetCharacterChanges(attacker, amount, 0);
        SetFlaggedActionResult(attacker, 5);
        AddTrainingPoints(attacker, 1, 1);
    }
    ApplyResistanceOutcome(attacker, g_attackResistance, amount);
    return RollGunCondition(attacker, target, g_attackResistance, g_attackCondition);
}

RVA(0x00008fe0, 0x2a)
void LoadGunDistributionTable(void) {
    FILE* fp = OpenDataFile(18, 12, 0);
    s_gunDistribution = ReadRawHandle(fp);
    CloseDataFile(fp);
}

RVA(0x00009010, 0x38)
i16 FilterGunTargets(Character* attacker, i16 count) {
    i16 rounds = GetGunBurstRounds(attacker);
    if (rounds < 1) {
        return 0;
    }
    return DistributeGunRounds(rounds, PrepareGunBurst(attacker, count));
}

RVA(0x00009050, 0x62)
i16 GetGunBurstRounds(Character* attacker) {
    i16 rounds;
    i16 limit;
    if (GetCharacterEquipment(attacker)[6].item < 1) {
        return 0;
    }
    if (GetCharacterEquipment(attacker)[7].item < 1) {
        return 0;
    }
    rounds = GetCharacterEquipment(attacker)[7].quantity;
    if (g_actorId >= 0) {
        rounds = 255;
    }
    limit = GetGunBurstLimit(GetLoadedRecord(GetCharacterEquipment(attacker)[6].item));
    if (limit > rounds) {
        limit = rounds;
    }
    return limit;
}

RVA(0x000090c0, 0xe7)
i16 PrepareGunBurst(Character* attacker, i16 count) {
    ItemRecord* record = GetLoadedRecord(GetCharacterEquipment(attacker)[6].item);
    u8 limits;
    i16 minimum;
    i16 maximum;
    s_gunPenaltyA = GetGunRequirementPenalty(
        GetStatTotal(attacker, STAT_DEXTERITY),
        GetItemRequiredDexterity(record)
    );
    s_gunPenaltyB = GetGunRequirementPenalty(
        GetStatTotal(attacker, STAT_VITALITY),
        GetItemRequiredVitality(record)
    );
    s_gunBasePower = GetItemAttackPower(record);
    limits = GetGunTargetLimits(record);
    s_gunRoundPower = GetItemAttackPower(GetLoadedRecord(GetCharacterEquipment(attacker)[7].item));
    maximum = limits & 15;
    minimum = limits >> 4;
    if (minimum < 1) {
        minimum = 1;
    } else if (minimum > 15) {
        minimum = 15;
    }
    if (maximum < minimum) {
        maximum = minimum;
    } else if (maximum > 15) {
        maximum = 15;
    }
    if (count < minimum) {
        return minimum;
    }
    if (count > maximum) {
        return maximum;
    }
    return count;
}

RVA(0x000091b0, 0x32)
i16 GetGunRequirementPenalty(i16 stat, i16 requirement) {
    if (requirement < 1) {
        requirement = 1;
    }
    return ClampShort(5 - stat / requirement, 1, 0x7fff);
}

static __inline u8 GetGunRoundPercent(u8 (*table)[15], i16 count, i16 index) {
    return table[count - 1][index];
}

RVA(0x000091f0, 0xe2)
i16 DistributeGunRounds(i16 rounds, i16 count) {
    u8(*table)[15];
    i16 index;
    i16 remaining;
    i16 share;
    memset(s_gunRounds, 0, sizeof(s_gunRounds));
    memset(s_gunPower, 0, sizeof(s_gunPower));
    table = HandleReadPtr(s_gunDistribution);
    remaining = rounds;
    for (index = 0; index < count; index++) {
        if (rounds * GetGunRoundPercent(table, count, index) == 0 || remaining < 1) {
            break;
        }
        share = rounds * GetGunRoundPercent(table, count, index);
        share /= 100;
        if (share < 1) {
            share = 1;
        }
        if (share > remaining) {
            share = remaining;
        }
        s_gunRounds[index] = share;
        s_gunPower[index] = ComputeGunBurstPower(share);
        remaining -= share;
    }
    return index;
}

RVA(0x000092e0, 0x3e)
i16 ComputeGunBurstPower(i16 rounds) {
    i16 penalty = s_gunPenaltyB;
    i16 power = s_gunRoundPower;
    i16 total = 0;
    i16 index;
    penalty += s_gunPenaltyA;
    for (index = 0; index < rounds; index++) {
        total += power;
        power -= penalty;
        if (power < 1) {
            power = 1;
        }
    }
    return total + s_gunBasePower;
}

RVA(0x00009320, 0x85)
void SpendGunRounds(Character* attacker) {
    i16 rounds;
    i16 index;
    if (g_actorId < 0) {
        rounds = s_gunRounds[0];
        if (rounds > GetCharacterEquipment(attacker)[7].quantity) {
            rounds = GetCharacterEquipment(attacker)[7].quantity;
        }
        GetCharacterEquipment(attacker)[7].quantity -= rounds;
        if (GetCharacterEquipment(attacker)[7].quantity <= 0) {
            ClearItemSlot(&GetCharacterEquipment(attacker)[7]);
        }
    }
    for (index = 0; index < 15; index++) {
        s_gunRounds[index] = s_gunRounds[index + 1];
        s_gunPower[index] = s_gunPower[index + 1];
    }
    s_gunPower[15] = 0;
    s_gunRounds[15] = 0;
}

RVA(0x000093b0, 0x28)
void SpendAllGunRounds(Character* attacker) {
    if (attacker) {
        while (s_gunRounds[0]) {
            SpendGunRounds(attacker);
        }
    }
}
