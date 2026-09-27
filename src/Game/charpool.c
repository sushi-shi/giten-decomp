// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/CharInfo.h>
#include <Game/EquipMagicDefense.h>
#include <Game/ItemBonus.h>
#include <Game/Stats.h>
#include <Game/StatUpdate.h>
#include <Util/BitSet.h>
#include <Util/CurMax.h>
#include <Util/Range.h>

// Codegen constraint: the operand order of CalcWeaponPowerStat, CalcMagicAccuracyStat,
// CalcMagicEvasionStat and ScalePercent999 shifts with the number of declarations
// this unit sees; memset comes from <memory.h> (<string.h> breaks
// CalcWeaponPowerStat), and a declaration added to an included header needs a
// recheck of all four.
#include <math.h>
#include <memory.h>
#include <stdlib.h>

// Bit 0: the first pool is below half; bit 1: the second one is.
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref`); retail keeps it because the link had no /OPT:REF.
RVA(0x0003d000, 0x25)
i16 LowPoolMask(CharacterPools* pools) {
    i16 mask = pools->hp.cur < pools->hp.max / 2;
    mask |= pools->mp.cur < pools->mp.max / 2 ? POOL_MASK_MP : POOL_MASK_NONE;
    return mask;
}

// 2: full; 1: partly spent; 0: empty.
RVA(0x0003d030, 0x1d)
i16 PoolState(CurMax* pool) {
    if (pool->max <= pool->cur) {
        return 2;
    }
    return pool->cur > 0;
}

// Bit 0: the first pool is empty; bit 1: the second one is.
RVA(0x0003d050, 0x1d)
i16 EmptyPoolMask(CharacterPools* pools) {
    i16 mask = pools->hp.cur == 0;
    mask |= pools->mp.cur == 0 ? POOL_MASK_MP : POOL_MASK_NONE;
    return mask;
}

RVA(0x0003d070, 0x1e)
void DrainPool(CurMax* pool, i32 amount) {
    SubCapped(&pool->cur, ClampToUShort(amount), 0);
}

// Refills a pool: mode 0 up to its maximum, 1 without a cap, 2 up to twice
// its maximum; other modes do nothing and return 0.
RVA(0x0003d090, 0x97)
i16 FillPool(CurMax* pool, i32 amount, GZ_ENUM_STORAGE(PoolFillMode, i16) mode) {
    u16 limit;
    switch (mode) {
        case POOL_FILL_TO_MAX:
            AddCapped(&pool->cur, ClampToUShort(amount), pool->max);
            break;
        case POOL_FILL_TO_USHORT_MAX:
            AddCapped(&pool->cur, ClampToUShort(amount), 0xffff);
            break;
        case POOL_FILL_TO_DOUBLE_MAX:
            limit = pool->max;
            AddCapped(&limit, pool->max, 0xffff);
            AddCapped(&pool->cur, ClampToUShort(amount), limit);
            break;
        default:
            return 0;
    }
}

// Drains a negative amount, refills a positive one up to the maximum.
RVA(0x0003d130, 0x2a)
void ChangePool(CurMax* pool, i32 amount) {
    if (amount < 0) {
        DrainPool(pool, -amount);
    } else {
        FillPool(pool, amount, POOL_FILL_TO_MAX);
    }
}

RVA(0x0003d160, 0x30)
i32 ClampTo999(i16 value) {
    return max(min(value, 999), 1);
}

// Derived values built step by step from the eleven stats, rounded and
// clamped to 1..999. Codegen constraint: each is accumulated in one double through
// separate compound statements; single expressions schedule differently.
RVA(0x0003d190, 0x54)
i32 StatBlend80(i16* stats, i16 level) {
    double value = stats[STAT_DEXTERITY] * 0.5;
    value -= stats[STAT_INTUITION] * -0.25;
    value *= sqrt(level);
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d1f0, 0x4f)
i32 StatBlend57(i16* stats, i16 level) {
    double value = stats[STAT_STRENGTH] * 0.25;
    value *= sqrt(level);
    value += stats[STAT_AGILITY];
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d240, 0x75)
i32 CalcPhysicalDefenseStat(i16* stats, i16 bonus) {
    double value = stats[STAT_PROTECTION] * 0.2;
    value -= stats[STAT_STRENGTH] * -0.25;
    value -= stats[STAT_MENTAL_STRENGTH] * -0.1;
    value += bonus;
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d2c0, 0x7b)
i32 CalcGunAccuracyStat(i16* stats, i16 bonus, i16 extra) {
    double value = stats[STAT_DEXTERITY];
    value -= stats[STAT_INTUITION] * -0.2;
    value += bonus;
    if (extra > 0) {
        value += extra * 2 + 10;
    }
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d340, 0x41)
i32 CalcGunEvasionStat(i16* stats) {
    double value = stats[STAT_PROTECTION] + stats[STAT_INTUITION];
    value *= 0.4;
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d390, 0x5b)
i32 CalcWeaponAccuracyStat(i16* stats, i16 bonus) {
    double value = stats[STAT_AGILITY];
    value *= 0.5;
    value += bonus;
    value += stats[STAT_STRENGTH];
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d3f0, 0x80)
i32 CalcWeaponPowerStat(i16* stats, i16 bonus, i16 level, i16 penalty) {
    double value = stats[5];
    value += stats[6];
    value *= 0.2;
    value += bonus;
    if (level >= 0x20) {
        value -= penalty * -0.15;
    }
    return ClampTo999(RoundToShort(value));
}

RVA(0x0003d470, 0x5a)
i32 CalcWeaponEvasionStat(i16* stats, i16 bonus) {
    double value = stats[STAT_INTUITION];
    value *= 0.4;
    value += stats[STAT_AGILITY];
    value += bonus;
    return ClampTo999(RoundToShort(value));
}

// Derived values from pairs of an eleven-stat array, clamped to 1..999. CalcMagicAccuracyStat
// and CalcMagicEvasionStat ignore the item bonus RecalcDerivedStats passes them.
// @early-stop TU state: the same body alone in a probe TU loads stats[STAT_INTELLIGENCE] first
// as retail does; in this unit cl loads stats[STAT_MAGIC] first (commutative order).
RVA(0x0003d4d0, 0x1e)
i32 CalcMagicAccuracyStat(i16* stats, i16 bonus) {
    i16 sum = stats[3] + stats[2];
    return ClampTo999(sum / 2);
}

RVA(0x0003d4f0, 0x17)
i32 CalcMagicPowerStat(i16* stats, i16 amount) {
    return ClampTo999(stats[STAT_MAGIC] + amount);
}

RVA(0x0003d510, 0x1e)
i32 CalcMagicEvasionStat(i16* stats, i16 bonus) {
    i16 sum = stats[4] + stats[3];
    return ClampTo999(sum / 2);
}

RVA(0x0003d530, 0x24)
i32 CalcMagicDefenseStat(i16* stats, i32 amount) {
    i16 sum = stats[STAT_MENTAL_STRENGTH] + stats[STAT_PROTECTION];
    i32 average = sum / 2;
    return ClampTo999(average + amount);
}

// Recomputes a character's 24 battle stats from its total stats and the
// bonuses of its eight item slots (each slot's item, then its indexed gem
// item); stats 0, 6, 12 and 18 are kept, 13 and 19..23 are set to 1.
// @identity-TODO: which values the indices name is unrecovered.
RVA(0x0003d560, 0x3e0)
void RecalcDerivedStats(Character* character) {
    i16 kept12 = GetBattleStatBase(character, 12);
    i16 kept18 = GetBattleStatBase(character, 18);
    i16 level = GetBattleStatBase(character, 0);
    i16 kept6 = GetBattleStatBase(character, 6);
    i16 bonuses[24];
    i16* stats;

    memset(bonuses, 0, sizeof(bonuses));
    AddItemStatBonuses(GetCharacterEquipment(character)[0].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[5].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[6].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[7].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[0].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[5].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[6].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[7].attachment, bonuses, 1);

    stats = character->stats.total;
    character->battleStats[0] = level;
    character->battleStats[1] = StatBlend57(stats, level);
    character->battleStats[BATTLE_STAT_WEAPON_ACCURACY] =
        CalcWeaponAccuracyStat(stats, bonuses[BATTLE_STAT_WEAPON_ACCURACY]);
    character->battleStats[BATTLE_STAT_WEAPON_POWER] =
        CalcWeaponPowerStat(stats, bonuses[BATTLE_STAT_WEAPON_POWER], character->id, level);
    character->battleStats[BATTLE_STAT_WEAPON_EVASION] =
        CalcWeaponEvasionStat(stats, bonuses[BATTLE_STAT_WEAPON_EVASION]);
    character->battleStats[BATTLE_STAT_WEAPON_DEFENSE] =
        CalcPhysicalDefenseStat(stats, bonuses[BATTLE_STAT_WEAPON_DEFENSE]);
    character->battleStats[6] = kept6;
    character->battleStats[7] = StatBlend80(stats, kept6);
    character->battleStats[BATTLE_STAT_GUN_ACCURACY] = CalcGunAccuracyStat(
        stats,
        bonuses[BATTLE_STAT_GUN_ACCURACY],
        GetBattleStatBase(character, 6)
    );
    character->battleStats[BATTLE_STAT_GUN_POWER] = ClampTo999(bonuses[BATTLE_STAT_GUN_POWER]);
    character->battleStats[BATTLE_STAT_GUN_EVASION] = CalcGunEvasionStat(stats);
    character->battleStats[BATTLE_STAT_GUN_DEFENSE] =
        GetBattleStatBase(character, BATTLE_STAT_WEAPON_DEFENSE);
    character->battleStats[12] = kept12;
    character->battleStats[13] = 1;
    character->battleStats[BATTLE_STAT_MAGIC_POWER] =
        CalcMagicPowerStat(stats, GetItemMagicPowerBonus(GetCharacterEquipment(character)[5].item));
    character->battleStats[BATTLE_STAT_MAGIC_ACCURACY] = CalcMagicAccuracyStat(
        stats,
        GetItemMagicAccuracyBonus(GetCharacterEquipment(character)[5].item)
    );
    character->battleStats[BATTLE_STAT_MAGIC_EVASION] = CalcMagicEvasionStat(
        stats,
        GetItemPhysicalEvasionBonus(GetCharacterEquipment(character)[4].item)
    );
    character->battleStats[BATTLE_STAT_MAGIC_DEFENSE] =
        CalcMagicDefenseStat(stats, SumEquippedMagicDefenseBonus(character, 3));
    character->battleStats[18] = kept18;
    character->battleStats[19] = 1;
    character->battleStats[21] = 1;
    character->battleStats[20] = 1;
    character->battleStats[22] = 1;
    character->battleStats[23] = 1;
}

// The physical-defense bonuses of armor slots 0..4 and their attached gems.
RVA(0x0003d940, 0x150)
i16 SumArmorDefenseBonus(Character* character) {
    i16 bonuses[24];

    memset(bonuses, 0, sizeof(bonuses));
    AddItemStatBonuses(GetCharacterEquipment(character)[0].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].item, bonuses, 0);
    AddItemStatBonuses(GetCharacterEquipment(character)[0].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[1].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[2].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[3].attachment, bonuses, 1);
    AddItemStatBonuses(GetCharacterEquipment(character)[4].attachment, bonuses, 1);
    return bonuses[BATTLE_STAT_WEAPON_DEFENSE];
}

// Scales the shown battle stats by the member's personal flags: flag 36
// halves stats 2, 4, 8 and 10, flag 35 raises them by half, flag 37 raises
// stats 3 and 5 by half.
// @identity-TODO: which stats the indices name and what the flags stand for
// are unrecovered.
RVA(0x0003da90, 0x14b)
void ApplyStatFlags(Character* character) {
    if (TestCharacterFlag(character, 0x24) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_ACCURACY), 50);
        character->battleStatsShown[BATTLE_STAT_WEAPON_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_EVASION), 50);
        character->battleStatsShown[BATTLE_STAT_GUN_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_ACCURACY), 50);
        character->battleStatsShown[BATTLE_STAT_GUN_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_EVASION), 50);
    }
    if (TestCharacterFlag(character, 0x23) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_ACCURACY), 150);
        character->battleStatsShown[BATTLE_STAT_WEAPON_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_EVASION), 150);
        character->battleStatsShown[BATTLE_STAT_GUN_ACCURACY] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_ACCURACY), 150);
        character->battleStatsShown[BATTLE_STAT_GUN_EVASION] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_GUN_EVASION), 150);
    }
    if (TestCharacterFlag(character, 0x25) == 1) {
        character->battleStatsShown[BATTLE_STAT_WEAPON_POWER] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_POWER), 150);
        character->battleStatsShown[BATTLE_STAT_WEAPON_DEFENSE] =
            ScalePercent999(GetBattleStatShown(character, BATTLE_STAT_WEAPON_DEFENSE), 150);
    }
}

RVA(0x0003dbe0, 0x28)
i32 ScalePercent999(i16 value, i16 percent) {
    return ClampTo999(value * percent / 100);
}

RVA(0x0003dc10, 0x2e)
i32 ClampTo100(i16 value) {
    return max(min(value, 100), 1);
}

RVA(0x0003dc40, 0x1a)
i32 ClampSum100(i16 a, i16 b, i16 c) {
    return ClampTo100(c + b + a);
}

// Recomputes each stat's total (base, equipment and bonus, modifiers; 1..100);
// returns how many changed.
RVA(0x0003dc60, 0x43)
i16 RecalcStatTotals(StatBlock* stats) {
    i16 changed = 0;
    i16 i;
    i16 total;
    for (i = 0; i < 11; i++) {
        total =
            ClampSum100(stats->base[i], stats->equipment[i] + stats->bonus[i], stats->modifiers[i]);
        if (total != stats->total[i]) {
            changed++;
        }
        stats->total[i] = total;
    }
    return changed;
}

// Clears the modifiers and returns how many were set.
RVA(0x0003dcb0, 0x21)
i16 ClearStatModifiers(StatBlock* stats) {
    i16 count = 0;
    i16 i;
    for (i = 0; i < 11; i++) {
        if (stats->modifiers[i] != 0) {
            count++;
        }
        stats->modifiers[i] = 0;
    }
    return count;
}

// Clears `stats`' bonus and equipment arrays, then adds the stat points of
// each slot's item to the bonus array and those of each slot's indexed gem
// item to the equipment array.
RVA(0x0003dce0, 0x1c0)
i16 ApplyItemStatBonuses(StatBlock* stats, ItemSlot* slots) {
    i16 i;

    for (i = 0; i < 11; i++) {
        stats->bonus[i] = 0;
        stats->equipment[i] = 0;
    }
    AddItemStatPoints(slots[0].item, stats->bonus);
    AddItemStatPoints(slots[1].item, stats->bonus);
    AddItemStatPoints(slots[2].item, stats->bonus);
    AddItemStatPoints(slots[3].item, stats->bonus);
    AddItemStatPoints(slots[4].item, stats->bonus);
    AddItemStatPoints(slots[5].item, stats->bonus);
    AddItemStatPoints(slots[6].item, stats->bonus);
    AddItemStatPoints(slots[7].item, stats->bonus);
    AddItemStatPoints(GemItemId(slots[0].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[1].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[2].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[3].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[4].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[5].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[6].attachment), stats->equipment);
    AddItemStatPoints(GemItemId(slots[7].attachment), stats->equipment);
    return 0;
}

RVA(0x0003dea0, 0x20)
i16 GemItemId(i16 index) {
    if (index == -1) {
        return -1;
    }
    return index + GetGemItemBase();
}
