#ifndef GITEN_GAME_CONDITIONAGE_H
#define GITEN_GAME_CONDITIONAGE_H

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Ints.h>

static __inline u8 GetConditionAge(ConditionSet* conditions, i16 condition) {
    return conditions->ages[condition];
}

static __inline void SetConditionAge(ConditionSet* conditions, i16 condition, u8 age) {
    conditions->ages[condition] = age;
}

// The condition set's per-condition ages: aging, recovery rolls and the
// conditions an empty pool brings, plus easing and the name walk.
i16 AgeConditions(ConditionSet* conditions, i16 amount);
b16 AgeCondition(i16 amount, ConditionSet* conditions, i16 condition);
i16 RecoverConditions(Character* character);
b16 RecoverCondition(Character* character, i16 condition);
i16 ApplyEmptyPools(Character* character);
i16 EaseCondition(
    ConditionSet* conditions,
    GZ_ENUM_PARAM(ConditionId, i16) mild,
    GZ_ENUM_PARAM(ConditionId, i16) severe
);
const char* NextConditionName(ConditionSet* conditions, i16* cursor);
i16 GetFirstConditionIndex(Character* character);

#endif // GITEN_GAME_CONDITIONAGE_H
