#ifndef GITEN_GAME_WAITLOOP_H
#define GITEN_GAME_WAITLOOP_H

#include <rva.h>

#include <Ints.h>

// What a caller that runs the wait state itself drives each step. Codegen
// constraint: kept out of <Game/WaitState.h>, whose declaration count the
// field TU's RunFieldEncounter depends on.

// One step of the wait state (game state 1): pops it when its input or time
// condition is met.
b16 StepWaitState(void);
b16 RunScreenFadeState(void);
b16 RunMessageBoxState(void);

// @identity-TODO: empty in the retail build (returns 0); the message opcode's
// wait loop calls it with (1, 0x18) each step. It lies in the motion TU's span.
RVA_DECL(0x00004990)
b16 PollIdle(i16 mode, i16 frames);

#endif // GITEN_GAME_WAITLOOP_H
