#ifndef GITEN_GAME_CONDITION_H
#define GITEN_GAME_CONDITION_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/ConditionId.h>
#include <Game/InflictCode.h>
#include <Game/RestoreEffect.h>
#include <Ints.h>
#include <Util/BitSet.h>

// Status conditions are bits 0..34, followed by their individual ages.
typedef struct ConditionSet {
    u8 bits[5];
    u8 ages[CONDITION_COUNT];
} ConditionSet;

static __inline void
ClearCondition(ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition) {
    ClearBit(conditions->bits, condition);
}

#define AccumulateConditionBits(conditions, found)                                                 \
    do {                                                                                           \
        i16 conditionByte;                                                                         \
        for (conditionByte = 0; conditionByte < 5; conditionByte++) {                              \
            (found) |= (conditions)->bits[conditionByte];                                          \
        }                                                                                          \
    } while (0)

// The inflict code of the action being resolved (the skill's inflicted
// condition, or the one an attack inflicted); INFLICT_NONE when none.
extern GZ_ENUM_STORAGE(InflictCode, i16) g_statusCondition;

const char* GetConditionName(GZ_ENUM_PARAM(ConditionId, i16) bit);
b16 HasCondition(ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition);
i16 ConditionKindApplies(GZ_ENUM_PARAM(RestoreEffect, i16) kind, ConditionSet* conditions);

// What AddCondition did: the condition was already held, was blocked (a
// fatal or overriding condition), was added, or instead cleared the opposing
// condition it cancels.
GZ_ENUM_BEGIN(ConditionAddResult)
    CONDITION_ADD_ALREADY_HELD = -1,
    CONDITION_ADD_BLOCKED = 0,
    CONDITION_ADD_ADDED = 1,
    CONDITION_ADD_CANCELLED_OPPOSITE = 2
GZ_ENUM_END(ConditionAddResult)

GZ_ENUM_RETURN(ConditionAddResult, i16) AddCondition(ConditionSet* conditions, GZ_ENUM_PARAM(ConditionId, i16) condition);
GZ_ENUM_RETURN(ConditionId, i16) LastConditionIn(ConditionSet* conditions, const GZ_ENUM_STORAGE(ConditionId, i16) * list);
void ClearConditionList(ConditionSet* conditions, const GZ_ENUM_STORAGE(ConditionId, i16) * list);
GZ_ENUM_RETURN(ConditionId, i16) GetDisablingCondition(ConditionSet* conditions);
void ClearBattleConditions(ConditionSet* conditions);
void ClearLeaveConditions(ConditionSet* conditions);
void ClearAllConditions(ConditionSet* conditions);

// The last of the fatal conditions (ash, dead, dying) the set holds, else 0:
// LastConditionIn over the list {0, 1, 2}.
GZ_ENUM_RETURN(ConditionId, i16) GetFatalCondition(ConditionSet* conditions);

// Escalating or easing a condition changes nothing, changes the mild state,
// or changes the severe state. The direction depends on the operation.
GZ_ENUM_BEGIN_SPLIT(ConditionChangeResult, i16)
    CONDITION_CHANGE_NONE = 0,
    CONDITION_CHANGE_MILD = 1,
    CONDITION_CHANGE_SEVERE = 2
GZ_ENUM_END_SPLIT(ConditionChangeResult)

GZ_ENUM_RETURN(ConditionChangeResult, i16) EaseSleep(ConditionSet* conditions);
const char* GetFirstConditionName(ConditionSet* conditions);

GZ_ENUM_RETURN(ConditionId, i16) GetPickBlockingCondition(ConditionSet* conditions);

#endif // GITEN_GAME_CONDITION_H
