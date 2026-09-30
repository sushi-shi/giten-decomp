#ifndef GITEN_GAME_BATTLEEFFECT_H
#define GITEN_GAME_BATTLEEFFECT_H

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Ints.h>

// The battle message selected by the low action-result code. The result word
// also carries restore outcomes, effect codes and drain tags in other contexts.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(BattleActionResult, i16)
    BATTLE_ACTION_MISSED = 0,
    BATTLE_ACTION_NO_EFFECT = 1,
    BATTLE_ACTION_GRAZED = 2,
    BATTLE_ACTION_SUCCESS = 3,
    BATTLE_ACTION_CRITICAL = 4,
    BATTLE_ACTION_LETHAL = 5,
    BATTLE_ACTION_IMMUNE = 6,
    BATTLE_ACTION_REFLECTED = 7,
    BATTLE_ACTION_HP_ABSORBED = 8,
    BATTLE_ACTION_MP_ABSORBED = 9,
    BATTLE_ACTION_PROTECTED = 10
GZ_ENUM_END_SPLIT(BattleActionResult)
// clang-format on

// The HP and MP change and the cured condition the last applied effect
// reports to the battle messages (0x41fe90 writes them).
extern i16 g_hpChange;
extern i16 g_mpChange;
extern i16 g_effectCondition;

// The shared result word also holds restoration results, effect codes and
// the draining skills' extra tags. The drain amount is reported separately.
extern i16 g_actionResult;
extern i16 g_drainAmount;

static __inline void ResetPoolChanges(void) {
    g_hpChange = 0;
    g_mpChange = 0;
}

static __inline void ResetActionOutcome(void) {
    g_statusCondition = INFLICT_NONE;
    g_actionResult = BATTLE_ACTION_MISSED;
}

static __inline void SetResistanceResult(
    Character* actor,
    i16 result,
    GZ_ENUM_PARAM(BattleActionResult, i16) actionResult
) {
    actor->result = result;
    g_actionResult = actionResult;
}

static __inline void
SetFlaggedActionResult(Character* actor, GZ_ENUM_PARAM(BattleActionResult, i16) result) {
    g_actionResult = result;
    SetCharacterResult(actor, result, 1);
}

static __inline i32 GetConditionDamageThreshold(Character* target) {
    return target->level / 2 + 1;
}

static __inline GZ_ENUM_RETURN(BattleActionResult, i16) SetActionResult(
    Character* actor,
    GZ_ENUM_PARAM(BattleActionResult, i16) result
) {
    actor->result = result;
    g_actionResult = result;
    return result;
}

// The skill or item id of the action being resolved (the actor's
// `pickTarget`).
extern i16 g_actionId;

// @identity-TODO: the outcome the script reads (OpGetBattleOutcome): 1 for a
// skill, 0 otherwise; and the number of targets the action struck
// (OpIfBattleResult).
extern i16 g_battleOutcome;
extern i16 g_targetCount;

#endif // GITEN_GAME_BATTLEEFFECT_H
