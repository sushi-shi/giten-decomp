#ifndef GITEN_GAME_INFLICTCODE_H
#define GITEN_GAME_INFLICTCODE_H

#include <Enums.h>

// What an item or skill inflicts (ResolveInflictedCondition): nothing, the
// condition of that number (1..34), or a condition picked by chance, the
// target's alignment or its demon class.
GZ_ENUM_BEGIN(InflictCode)
    INFLICT_NONE = 0,
    INFLICT_CONDITION_FIRST = 1,
    INFLICT_CONDITION_LAST = 34,
    INFLICT_CHARM_UNLESS_ALIGNED_B_POSITIVE = 57,
    INFLICT_TIPSY_BY_HALF = 58,
    INFLICT_PARALYSIS_OR_TIPSY = 59,
    INFLICT_HIGH_HALLUCINATION_OR_BERSERK = 60,
    INFLICT_DRUNK_OR_TIPSY = 61,
    INFLICT_DYING_OR_DEAD = 62,
    INFLICT_HIGH_OR_ASH = 63,
    INFLICT_DEAD_BY_HALF = 64,
    INFLICT_PANIC_UNLESS_ALIGNED_B_NEGATIVE = 65
GZ_ENUM_END(InflictCode)

#endif // GITEN_GAME_INFLICTCODE_H
