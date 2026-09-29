#ifndef GITEN_GAME_ACTIONMARK_H
#define GITEN_GAME_ACTIONMARK_H

#include <EnumDomain.h>

// An action wait's ready mark (ActionWait.ready): set when the actor is queued
// to act, cleared once it has.
GZ_ENUM_BEGIN_SPLIT(ActionMark, i8)
    ACTION_UNMARKED = 0,
    ACTION_MARKED = 1
GZ_ENUM_END_SPLIT(ActionMark)

// Remaining-tick values of an action wait: ready, queued for the next turn,
// the reset delay, and the alert clamp OpSetActorAlert extends a wait to.
GZ_ENUM_CONST_BEGIN(ActionWaitTicks)
    ACTION_WAIT_READY = 0,
    ACTION_WAIT_QUEUED = 1,
    ACTION_WAIT_RESET = 0xff,
    ACTION_WAIT_EXTENDED = 0x200
GZ_ENUM_CONST_END(ActionWaitTicks)

#endif // GITEN_GAME_ACTIONMARK_H
