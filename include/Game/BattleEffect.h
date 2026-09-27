#ifndef GITEN_GAME_BATTLEEFFECT_H
#define GITEN_GAME_BATTLEEFFECT_H

#include <Game/Character.h>
#include <Ints.h>

// The HP and MP change and the cured condition the last applied effect
// reports to the battle messages (0x41fe90 writes them).
// @identity-TODO: their owner TU is unclaimed.
extern i16 g_hpChange;
extern i16 g_mpChange;
extern i16 g_effectCondition;

// @identity-TODO: the outcome code of the last action (1 no effect, 5 an
// overkill, bits 0x50/0x70/0x80 the draining skills), and the amount the
// experience-draining skill took.
extern i16 g_actionResult;
extern i16 g_drainAmount;

static __inline void SetResistanceResult(Character* actor, i16 result, i16 actionResult) {
    actor->result = result;
    g_actionResult = actionResult;
}

static __inline void SetFlaggedActionResult(Character* actor, i16 result) {
    g_actionResult = result;
    SetCharacterResult(actor, result, 1);
}

static __inline i32 GetConditionDamageThreshold(Character* target) {
    return target->level / 2 + 1;
}

static __inline i16 SetActionResult(Character* actor, i16 result) {
    actor->result = result;
    g_actionResult = result;
    return result;
}

// The skill or item id of the action being resolved (the actor's
// `pickTarget`).
// @identity-TODO: its owner TU is unclaimed.
extern i16 g_actionId;

// @identity-TODO: the outcome the script reads (OpGetBattleOutcome): 1 for a
// skill, 0 otherwise; and the number of targets the action struck
// (OpIfBattleResult).
extern i16 g_battleOutcome;
extern i16 g_targetCount;

// @identity-TODO: the party position of the member whose command is resolved
// (RunPartyCommandInput copies it); the summoning skill swaps the summoned
// demon into it.
extern i16 g_commandPosition;

#endif // GITEN_GAME_BATTLEEFFECT_H
