#ifndef GITEN_GAME_CONDITIONAGE_H
#define GITEN_GAME_CONDITIONAGE_H

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Ints.h>

static __inline u8
GetConditionAge(ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition) {
    return conditions->ages[condition];
}

static __inline void
SetConditionAge(ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition, u8 age) {
    conditions->ages[condition] = age;
}

// The result of applying depleted HP or MP to a character's conditions.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(EmptyPoolOutcome, i16)
    EMPTY_POOL_UNCHANGED = 0,
    EMPTY_POOL_HP_DYING_ADDED = 1,
    EMPTY_POOL_HP_ALREADY_DYING = 2,
    EMPTY_POOL_CONDITION_ADDED = 3,
    EMPTY_POOL_BOTH_ALREADY_DYING = 4
GZ_ENUM_END_SPLIT(EmptyPoolOutcome);
// clang-format on

// The condition set's per-condition ages: aging, recovery rolls and the
// conditions an empty pool brings, plus easing and the name walk.
i16 AgeConditions(ConditionSet* conditions, i16 amount);
b16 AgeCondition(i16 amount, ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition);
i16 RecoverConditions(CharacterCore* character);
b16 RecoverCondition(CharacterCore* character, GZ_ENUM_PARAM(ConditionId, i16) condition);
GZ_ENUM_RETURN(EmptyPoolOutcome, i16) ApplyEmptyPools(CharacterCore* character);
GZ_ENUM_RETURN(ConditionChangeResult, i16) EscalateCondition(
    ConditionSet* conditions,
    GZ_ENUM_PARAM(ConditionId, i16) mild,
    GZ_ENUM_PARAM(ConditionId, i16) severe
);
GZ_ENUM_RETURN(ConditionChangeResult, i16) EaseCondition(
    ConditionSet* conditions,
    GZ_ENUM_PARAM(ConditionId, i16) mild,
    GZ_ENUM_PARAM(ConditionId, i16) severe
);
const char* NextConditionName(ConditionSet* conditions, i16* cursor);
i16 GetFirstConditionIndex(CharacterCore* character);

#endif // GITEN_GAME_CONDITIONAGE_H
