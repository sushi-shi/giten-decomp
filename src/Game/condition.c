// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/Stats.h>
#include <Util/BitSet.h>
#include <Util/Range.h>

// Each condition's base chance (of 256) to wear off per roll; 0 never does.
DATA(0x00069f08)
static i16 s_recoveryChance[35] = {
    0,  0,  0,   0,  0,  0,  0,  0,  0,  0,  15, 40, 30, 40, 40, 0, 60, 10,
    50, 50, 128, 60, 50, 50, 10, 80, 10, 70, 80, 50, 10, 0,  0,  0, 0,
};

// A status condition's bit index and its display name (Shift-JIS).
typedef struct ConditionName {
    u8 bit;
    char name[7];
} ConditionName;

// The fatal conditions (ash, dead, dying).
DATA(0x000646e8)
static const i16 s_fatalConditions[] = {CONDITION_ASH, CONDITION_DEAD, CONDITION_DYING, -1};

// @identity-TODO: the conditions GetPickBlockingCondition reports (the last one
// set wins); named from its caller in the party picker.
DATA(0x000646f0)
static const i16 s_pickBlockingConditions[] = {
    21,
    6,
    CONDITION_DOZE,
    CONDITION_SLEEP,
    10,
    3,
    CONDITION_DYING,
    CONDITION_DEAD,
    CONDITION_ASH,
    20,
    12,
    5,
    4,
    19,
    -1
};

// Conditions suppressed when an attack targets a field actor in field mode.
DATA(0x00064710)
static const i16 s_fieldRestrictedConditions[] = {
    CONDITION_ASH,
    CONDITION_DEAD,
    CONDITION_DYING,
    3,
    4,
    5,
    6,
    7,
    CONDITION_ZOMBIE,
    9,
    10,
    11,
    12,
    CONDITION_SLEEP,
    14,
    CONDITION_POISON,
    17,
    19,
    21,
    30,
    31,
    CONDITION_SEVERE_POISON,
    33,
    -1,
};

// @identity-TODO: the conditions GetDisablingCondition reports (the last one
// set wins), those cleared after a battle, those cleared when a member leaves
// the party, and every condition in display order.
DATA(0x00064740)
static const i16 s_disablingConditions[] =
    {CONDITION_DYING, CONDITION_DEAD, CONDITION_ASH, 3, 4, 5, 6, -1};

DATA(0x00064750)
static const i16 s_battleConditions[] = {17, 20, 21, 22, CONDITION_DOZE, 26, 27, 28, -1};

DATA(0x00064768)
static const i16 s_leaveConditions[] = {
    10,
    11,
    CONDITION_SLEEP,
    14,
    16,
    18,
    19,
    20,
    21,
    22,
    23,
    CONDITION_DOZE,
    26,
    27,
    28,
    29,
    31,
    -1,
};

DATA(0x00064790)
static const i16 s_allConditions[] = {
    CONDITION_ASH,
    CONDITION_DEAD,
    CONDITION_DYING,
    3,
    4,
    5,
    6,
    7,
    CONDITION_ZOMBIE,
    9,
    10,
    11,
    12,
    CONDITION_SLEEP,
    14,
    CONDITION_SEVERE_POISON,
    CONDITION_POISON,
    16,
    17,
    18,
    19,
    20,
    21,
    22,
    23,
    24,
    CONDITION_DOZE,
    26,
    27,
    30,
    29,
    28,
    31,
    33,
    34,
    -1,
};

// Shown when no condition name applies: three full-width spaces.
DATA(0x000647d8)
static char* const s_noConditionName = "\201@\201@\201@";

DATA(0x000647e0)
static const ConditionName s_conditionNames[] = {
    {CONDITION_ASH, "\212D"},                      // 灰
    {CONDITION_DEAD, "\216\200"},                  // 死
    {CONDITION_DYING, "\225m\216\200"},            // 瀕死
    {3, "\215\250\223|"},                          // 昏倒
    {4, "\220\316\211\273"},                       // 石化
    {5, "\226\203\341\203"},                       // 麻痺
    {6, "\223\200\214\213"},                       // 凍結
    {7, "\234\337\210\313"},                       // 憑依
    {CONDITION_ZOMBIE, "\203]\203\223\203r"},      // ゾンビ
    {9, "\216\364\202\242"},                       // 呪い
    {10, "\213C\220\342"},                         // 気絶
    {11, "\222\202\221\247"},                      // 窒息
    {12, "\213\326\224\233"},                      // 禁縛
    {CONDITION_SLEEP, "\226\260\202\350"},         // 眠り
    {14, "\213\260\215Q"},                         // 恐慌
    {CONDITION_SEVERE_POISON, "\226\322\223\305"}, // 猛毒
    {CONDITION_POISON, "\223\305"},                // 毒
    {16, "\214\266\212o"},                         // 幻覚
    {17, "\226\243\227\271"},                      // 魅了
    {18, "\215\254\227\220"},                      // 混乱
    {19, "\225\221\223\245"},                      // 舞踏
    {20, "\212\264\223d"},                         // 感電
    {21, "\225X\214\213"},                         // 氷結
    {22, "\211\212\217\343"},                      // 炎上
    {23, "\226\323\226\332"},                      // 盲目
    {24, "\225\225\226\202"},                      // 封魔
    {CONDITION_DOZE, "\213\217\226\260\202\350"},  // 居眠り
    {26, "\213\266\220\355\216m"},                 // 狂戦士
    {27, "\203n\203C"},                            // ハイ
    {30, "\223D\220\214"},                         // 泥酔
    {29, "\202\331\202\353\220\214"},              // ほろ酔
    {28, "\215K\225\237"},                         // 幸福
    {31, "\275\327\262\321"},                      // ｽﾗｲﾑ
    {33, "\213z\214\214"},                         // 吸血
    {34, "\212O\217\235"},                         // 外傷
};

RVA(0x0003e6b0, 0x13)
b16 HasCondition(ConditionSet* conditions, i16 condition) {
    return TestBit(conditions->bits, condition);
}

#define AccumulateCollapseOrPetrification(blocked, conditions)                                     \
    do {                                                                                           \
        (blocked) |= HasCondition((conditions), 3);                                                \
        (blocked) |= HasCondition((conditions), 4);                                                \
    } while (0)

// The last condition of `list` that is set, or 0.
// Adds `condition` to a condition set under the conditions' precedence rules:
// returns 1 when added (its byte reset), -1 when already held, 0 when a
// fatal or overriding condition blocks it, and 2 when frozen and burning only
// cancel each other. Doze escalates to sleep, poison to severe poison,
// ice-bound to frozen and tipsy to drunk when already held.
// @early-stop tail merge: the eight `return 0` exits of the four escalating
// cases (15/21/25/29) jump to the shared return of cases 33/34 in retail but to
// case 5's here; instructions, calls and branch counts are identical. Loop
// form (for/goto), nested-if returns, case 33/34 spelling and order were tried.
RVA(0x0003e6d0, 0x700)
i16 AddCondition(ConditionSet* conditions, i16 condition) {
    i16 blocked;
    i16 i;

    HasCondition(conditions, CONDITION_ZOMBIE);
    for (;;) {
        blocked = 0;
        switch (condition) {
            case CONDITION_POISON:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                blocked |= HasCondition(conditions, CONDITION_SEVERE_POISON);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, CONDITION_POISON)) {
                    break;
                }
                condition = CONDITION_SEVERE_POISON;
                continue;
            case 21:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                blocked |= HasCondition(conditions, 6);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, 21)) {
                    break;
                }
                condition = 6;
                continue;
            case CONDITION_DOZE:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, CONDITION_SLEEP);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, CONDITION_DOZE)) {
                    break;
                }
                condition = CONDITION_SLEEP;
                continue;
            case 29:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 30);
                if (blocked) {
                    return 0;
                }
                if (!HasCondition(conditions, 29)) {
                    break;
                }
                condition = 30;
                continue;
            case CONDITION_ASH:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                for (i = 0; i < 35; i++) {
                    ClearCondition(conditions, i);
                }
                break;
            case CONDITION_DEAD:
            case CONDITION_DYING:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                for (i = 0; i < 35; i++) {
                    ClearCondition(conditions, i);
                }
                break;
            case 3:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                if (HasCondition(conditions, 26)) {
                    return 0;
                }
                ClearCondition(conditions, 10);
                ClearCondition(conditions, CONDITION_SLEEP);
                ClearCondition(conditions, 18);
                ClearCondition(conditions, 20);
                ClearCondition(conditions, CONDITION_DOZE);
                ClearCondition(conditions, 27);
                ClearCondition(conditions, 28);
                ClearCondition(conditions, 29);
                ClearCondition(conditions, 30);
                break;
            case 4:
            case 9:
            case 24:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                break;
            case 5:
            case 12:
            case 17:
            case 19:
            case 20:
            case 28:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                break;
            case 26:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 5);
                blocked |= HasCondition(conditions, 7);
                blocked |= HasCondition(conditions, CONDITION_ZOMBIE);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, CONDITION_SLEEP);
                blocked |= HasCondition(conditions, CONDITION_DOZE);
                if (blocked) {
                    return 0;
                }
                break;
            case 30:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, 29);
                break;
            case CONDITION_SEVERE_POISON:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_POISON);
                break;
            case 33:
            case 34:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                break;
            case 6:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                if (HasCondition(conditions, 22)) {
                    ClearCondition(conditions, 22);
                    return 2;
                }
                ClearCondition(conditions, 21);
                break;
            case 7:
            case CONDITION_ZOMBIE:
            case 11:
            case 31:
                if (HasCondition(conditions, condition)) {
                    return -1;
                }
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                if (HasCondition(conditions, 4)) {
                    return 0;
                }
                break;
            case 10:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_SLEEP);
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case CONDITION_SLEEP:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 10);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case 14:
            case 16:
            case 18:
            case 27:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                AccumulateCollapseOrPetrification(blocked, conditions);
                blocked |= HasCondition(conditions, 26);
                if (blocked) {
                    return 0;
                }
                ClearCondition(conditions, CONDITION_DOZE);
                break;
            case 22:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                if (HasCondition(conditions, 6)) {
                    ClearCondition(conditions, 6);
                    return 2;
                }
                break;
            case 23:
                if (GetFatalCondition(conditions)) {
                    return 0;
                }
                blocked |= HasCondition(conditions, 4);
                if (blocked) {
                    return 0;
                }
                break;
        }
        break;
    }
    SetBit(conditions->bits, condition);
    SetConditionAge(conditions, condition, 0);
    return 1;
}

RVA(0x0003edd0, 0x13)
i16 GetFatalCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_fatalConditions);
}

RVA(0x0003edf0, 0x43)
i16 LastConditionIn(ConditionSet* conditions, const i16* list) {
    i16 found = 0;
    i16 i;
    for (i = 0; list[i] != -1; i++) {
        if (HasCondition(conditions, list[i])) {
            found = list[i];
        }
    }
    return found;
}

RVA(0x0003ee40, 0x32)
void ClearConditionList(ConditionSet* conditions, const i16* list) {
    i16 i;
    for (i = 0; list[i] != -1; i++) {
        ClearCondition(conditions, list[i]);
    }
}

RVA(0x0003ee80, 0x13)
i16 GetPickBlockingCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_pickBlockingConditions);
}

RVA(0x0003eea0, 0x36)
b16 IsFieldConditionRestricted(i16 condition) {
    i16 i;
    for (i = 0; s_fieldRestrictedConditions[i] != -1; i++) {
        if (s_fieldRestrictedConditions[i] == condition) {
            return true;
        }
    }
    return false;
}

RVA(0x0003eee0, 0x13)
i16 GetDisablingCondition(ConditionSet* conditions) {
    return LastConditionIn(conditions, s_disablingConditions);
}

RVA(0x0003ef00, 0x13)
void ClearBattleConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_battleConditions);
}

RVA(0x0003ef20, 0x13)
void ClearLeaveConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_leaveConditions);
}

RVA(0x0003ef40, 0x13)
void ClearAllConditions(ConditionSet* conditions) {
    ClearConditionList(conditions, s_allConditions);
}

// Ages every held condition by `amount`; nonzero when any aged.
RVA(0x0003ef60, 0x37)
i16 AgeConditions(ConditionSet* conditions, i16 amount) {
    i16 aged = 0;
    i16 i;
    if (!amount) {
        return 0;
    }
    for (i = 0; i < 35; i++) {
        aged |= AgeCondition(amount, conditions, i);
    }
    return aged;
}

// Ages `condition` by `amount` (kept in 0..255) when it is held and can wear
// off; 1 when aged.
RVA(0x0003efa0, 0x57)
b16 AgeCondition(i16 amount, ConditionSet* conditions, i16 condition) {
    if (!HasCondition(conditions, condition)) {
        return false;
    }
    if (s_recoveryChance[condition] == 0) {
        return false;
    }
    SetConditionAge(
        conditions,
        condition,
        ClampUShort(GetConditionAge(conditions, condition) + amount, 0, 0xff)
    );
    return true;
}

// Rolls every held condition of `character` for recovery; nonzero when any
// wore off.
RVA(0x0003f000, 0x25)
i16 RecoverConditions(Character* character) {
    i16 recovered = 0;
    i16 i;
    for (i = 0; i < 35; i++) {
        recovered |= RecoverCondition(character, i);
    }
    return recovered;
}

// Rolls `condition` for recovery: its chance plus an eighth of its age against
// 0..255. A condition that stays can hurt: dancing (19) drains 1..5 HP,
// suffocation (11) 1..33.
RVA(0x0003f030, 0xb6)
b16 RecoverCondition(Character* character, i16 condition) {
    i16 chance;
    if (!HasCondition(GetCharacterConditions(character), condition)) {
        return false;
    }
    if (s_recoveryChance[condition] == 0) {
        return false;
    }
    chance = (GetConditionAge(GetCharacterConditions(character), condition) >> 3)
             + s_recoveryChance[condition];
    if (chance <= RandomUpTo(0xff)) {
        if (condition == 19) {
            DrainPool(&character->pools.hp, RandomUpTo(4) + 1);
        } else if (condition == 11) {
            DrainPool(&character->pools.hp, RandomUpTo(0x20) + 1);
        }
        return false;
    }
    ClearCondition(GetCharacterConditions(character), condition);
    return true;
}

// Applies the conditions an empty HP or MP pool causes: 1 when HP ran out
// (dying, unless already dying (2) or the condition 8 blocks it), 3 when a
// pool ran out and a condition was added, 4 when both are out while already
// dying, 2 when HP is out while dying, else 0.
// @identity-TODO: condition 8's role here is unrecovered.
RVA(0x0003f0f0, 0xd0)
i16 ApplyEmptyPools(Character* character) {
    switch (EmptyPoolMask(&character->pools)) {
        case POOL_MASK_NONE:
            break;
        case POOL_MASK_BOTH:
            if (!HasCondition(GetCharacterConditions(character), CONDITION_DYING)) {
                AddCondition(GetCharacterConditions(character), CONDITION_DYING);
                return 3;
            }
            return 4;
        case POOL_MASK_MP:
            if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                AddCondition(GetCharacterConditions(character), CONDITION_DEAD);
                return 3;
            }
            break;
        case POOL_MASK_HP:
            if (!HasCondition(GetCharacterConditions(character), CONDITION_DYING)) {
                if (HasCondition(GetCharacterConditions(character), CONDITION_ZOMBIE)) {
                    break;
                }
                AddCondition(GetCharacterConditions(character), CONDITION_DYING);
                return 1;
            }
            return 2;
    }
    return 0;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
// Escalates `mild` to `severe`: 0 when `severe` is held, 1 when `mild` was
// added, 2 when `mild` became `severe`.
RVA(0x0003f1c0, 0x60)
i16 EscalateCondition(ConditionSet* conditions, i16 mild, i16 severe) {
    if (HasCondition(conditions, severe)) {
        return 0;
    }
    if (!HasCondition(conditions, mild)) {
        AddCondition(conditions, mild);
        return 1;
    }
    ClearCondition(conditions, mild);
    AddCondition(conditions, severe);
    return 2;
}

// Eases `severe` to `mild` (2), or clears `mild` (1); 0 when neither is held.
RVA(0x0003f220, 0x6c)
i16 EaseCondition(ConditionSet* conditions, i16 mild, i16 severe) {
    if (HasCondition(conditions, severe)) {
        ClearCondition(conditions, severe);
        ClearCondition(conditions, mild);
        AddCondition(conditions, mild);
        return 2;
    }
    if (HasCondition(conditions, mild)) {
        ClearCondition(conditions, mild);
        return 1;
    }
    return 0;
}

// Eases sleep (13) to doze (25).
RVA(0x0003f290, 0x12)
i16 EaseSleep(ConditionSet* conditions) {
    return EaseCondition(conditions, CONDITION_DOZE, CONDITION_SLEEP);
}

// The display name of the first held condition in display order.
RVA(0x0003f2b0, 0x3e)
const char* GetFirstConditionName(ConditionSet* conditions) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (TestBit(conditions->bits, s_conditionNames[i].bit)) {
            return s_conditionNames[i].name;
        }
    }
    return s_noConditionName;
}

// The name of the next held condition from `*cursor` on (display order),
// leaving `*cursor` on it; -1 and the blank name after the last.
RVA(0x0003f2f0, 0x5a)
const char* NextConditionName(ConditionSet* conditions, i16* cursor) {
    for (; *cursor < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); (*cursor)++) {
        if (TestBit(conditions->bits, s_conditionNames[*cursor].bit)) {
            return s_conditionNames[*cursor].name;
        }
    }
    *cursor = -1;
    return s_noConditionName;
}

RVA(0x0003f350, 0x30)
const char* GetConditionName(i16 bit) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (s_conditionNames[i].bit == bit) {
            return s_conditionNames[i].name;
        }
    }
    return s_noConditionName;
}

// The display-order index of the first condition `character` holds; -1 when
// none.
RVA(0x0003f380, 0x3c)
i16 GetFirstConditionIndex(Character* character) {
    i16 i;
    for (i = 0; i < sizeof(s_conditionNames) / sizeof(s_conditionNames[0]); i++) {
        if (TestBit(GetCharacterConditions(character)->bits, s_conditionNames[i].bit)) {
            return i;
        }
    }
    return -1;
}
