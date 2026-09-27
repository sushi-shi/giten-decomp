#ifndef GITEN_GAME_STATESTACK_H
#define GITEN_GAME_STATESTACK_H

#include <rva.h>

#include <Ints.h>

// The game's state machine: the current state, the phase within it, the step
// within the phase and a sub-step. Changing a level resets the levels below.
// A state change can push the current record to return to.
// @identity-TODO: the meaning of the state numbers (0x19 from the script, 0xb
// from the field entry) is unrecovered.
typedef struct GameState {
    i16 state;
    i16 phase;
    i16 step;
    i16 sub;
} GameState;

#define GAME_STATE_STACK_DEPTH 32

void ClearGameStateStack(void);
void SaveGameState(void);
void PopGameState(void);
i16 __fastcall SetGameState(i16 state);
i16 SetGamePhase(i16 phase);
i16 NextGamePhase(void);
i16 PrevGamePhase(void);
i16 SetGamePhaseKeepStep(i16 phase);
i16 PrevGamePhaseKeepStep(void);
i16 __fastcall SetGameStep(u16 step);
i16 NextGameStep(void);
i16 PrevGameStep(void);
i16 __fastcall SetGameSub(i16 sub);
i16 NextGameSub(void);
i16 PrevGameSub(void);
i16 GetGameState(void);
u16 GetGamePhase(void);
u16 GetGameStep(void);
u16 GetGameSub(void);
void __fastcall PushGameState(i16 state);

// @identity-TODO: a one-jump wrapper around PopGameState; the state handlers
// call it to return to the state that pushed them.
void ReturnFromGameState(void);

i16 DispatchGameState(void);

#endif // GITEN_GAME_STATESTACK_H
