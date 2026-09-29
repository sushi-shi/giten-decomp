#ifndef GITEN_GAME_RESTOREEFFECT_H
#define GITEN_GAME_RESTOREEFFECT_H

#include <EnumDomain.h>

// What a restoration does, by its effect code (an item's params[9] or a
// skill's effect code): codes 1..34 cure the condition of that number (1 and 2
// also revive), the rest are named after what ApplyRestoreEffect does.
GZ_ENUM_BEGIN_SPLIT(RestoreEffect, i16)
    RESTORE_EFFECT_POOLS = 0,
    RESTORE_EFFECT_CURE_FIRST = 1,
    RESTORE_EFFECT_CURE_LAST = 34,
    RESTORE_EFFECT_HP_PAST_MAX = 49,
    RESTORE_EFFECT_MP = 50,
    RESTORE_EFFECT_HP_QUARTER_MP = 51,
    RESTORE_EFFECT_MP_QUARTER_HP = 52,
    RESTORE_EFFECT_MENTAL = 53,
    RESTORE_EFFECT_EXTENDED_MENTAL = 54,
    RESTORE_EFFECT_POISON_PARALYSIS = 55,
    RESTORE_EFFECT_EXTENDED_POISON_PARALYSIS = 56,
    RESTORE_EFFECT_PHYSICAL = 57,
    RESTORE_EFFECT_FAINT = 58,
    RESTORE_EFFECT_DEATH = 59,
    RESTORE_EFFECT_DRAIN_MP = 60,
    RESTORE_EFFECT_DRAIN_HP = 61,
    RESTORE_EFFECT_GENERAL = 62,
    RESTORE_EFFECT_HEAL_IF_ALIGNMENT_B_ABOVE_NEUTRAL = 63,
    RESTORE_EFFECT_SPECIAL = 64,
    RESTORE_EFFECT_HEAL_IF_ALIGNMENT_B_BELOW_NEUTRAL = 65
GZ_ENUM_END_SPLIT(RestoreEffect)

// ComputeRestoreAmount's amount codes that restore a fixed share of the pool;
// other codes add to a roll on the user's magic.
GZ_ENUM_CONST_BEGIN(RestoreAmount)
    RESTORE_AMOUNT_QUARTER = 253,
    RESTORE_AMOUNT_HALF = 254,
    RESTORE_AMOUNT_FULL = 255
GZ_ENUM_CONST_END(RestoreAmount)

#endif // GITEN_GAME_RESTOREEFFECT_H
