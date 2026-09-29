#ifndef GITEN_GAME_ACTIONOUTCOME_H
#define GITEN_GAME_ACTIONOUTCOME_H

#include <EnumDomain.h>

// What ResolveCombatAction reports after an action: the default message, an
// inflicted condition, or the battle tally (SetActionOutcome).
GZ_ENUM_BEGIN_SPLIT(ActionOutcome, i16)
    ACTION_OUTCOME_DEFAULT = 0,
    ACTION_OUTCOME_CONDITION = 1,
    ACTION_OUTCOME_BATTLE_TALLY = 2
GZ_ENUM_END_SPLIT(ActionOutcome)

#endif // GITEN_GAME_ACTIONOUTCOME_H
