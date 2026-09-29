#ifndef GITEN_GAME_CONDITION_H
#define GITEN_GAME_CONDITION_H

#include <rva.h>

#include <Ints.h>
#include <Game/ConditionId.h>
#include <Util/BitSet.h>

// Status conditions are bits 0..34, followed by their individual ages.
typedef struct ConditionSet {
    u8 bits[5];
    u8 ages[35];
} ConditionSet;

static __inline void ClearCondition(ConditionSet* conditions, i16 condition) {
    ClearBit(conditions->bits, condition);
}

#define AccumulateConditionBits(conditions, found)                                                 \
    do {                                                                                           \
        i16 conditionByte;                                                                         \
        for (conditionByte = 0; conditionByte < 5; conditionByte++) {                              \
            (found) |= (conditions)->bits[conditionByte];                                          \
        }                                                                                          \
    } while (0)

// @identity-TODO: a condition id the battle code keeps (cleared by
// 0x424b20/0x424b60 and the attack routines 0x408d30/0x424950, set from byte
// +0xd of the cached skill by 0x42db90); its role is unrecovered.
extern i16 g_statusCondition;

const char* GetConditionName(i16 bit);
b16 HasCondition(ConditionSet* conditions, i16 condition);
i16 ConditionKindApplies(i16 kind, ConditionSet* conditions);

// The physical ailments selected by restoration kind 57.
extern const i16 g_physicalRecoveryConditions[8];
i16 AddCondition(ConditionSet* conditions, i16 condition);
i16 LastConditionIn(ConditionSet* conditions, const i16* list);
void ClearConditionList(ConditionSet* conditions, const i16* list);
i16 GetDisablingCondition(ConditionSet* conditions);
void ClearBattleConditions(ConditionSet* conditions);
void ClearLeaveConditions(ConditionSet* conditions);
void ClearAllConditions(ConditionSet* conditions);

// The last of the fatal conditions (ash, dead, dying) the set holds, else 0:
// LastConditionIn over the list {0, 1, 2}.
i16 GetFatalCondition(ConditionSet* conditions);

i16 EaseSleep(ConditionSet* conditions);
const char* GetFirstConditionName(ConditionSet* conditions);

i16 GetPickBlockingCondition(ConditionSet* conditions);

#endif // GITEN_GAME_CONDITION_H
