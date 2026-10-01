#ifndef GITEN_GAME_STATS_H
#define GITEN_GAME_STATS_H

#include <rva.h>

#include <Game/Character.h>
#include <Game/PoolFillMode.h>
#include <Ints.h>
#include <Util/CurMax.h>

i32 ClampTo999(i16 value);
i32 ClampTo100(i16 value);
i32 ClampSum100(i16 a, i16 b, i16 c);

void DrainPool(CurMax* pool, i32 amount);

i16 FillPool(CurMax* pool, i32 amount, GZ_ENUM_PARAM(PoolFillMode, i16) mode);
i16 EmptyPoolMask(CharacterPools* pools);

void RecalcDerivedStats(Character* character);
i16 SumArmorDefenseBonus(Character* character);

i16 RecalcStatTotals(StatBlock* stats);

b16 ApplyItemStatBonuses(StatBlock* stats, ItemSlot* slots);

static __inline void RecalcEquippedStatTotals(StatBlock* stats, ItemSlot* slots) {
    ApplyItemStatBonuses(stats, slots);
    RecalcStatTotals(stats);
}

#endif // GITEN_GAME_STATS_H
