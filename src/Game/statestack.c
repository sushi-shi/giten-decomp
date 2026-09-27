// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/StateStack.h>

DATA(0x0007bb70)
static GameState s_gameState;

DATA(0x0007bc38)
static GameState s_stateStack[GAME_STATE_STACK_DEPTH];

DATA(0x0007be48)
static i16 s_stateDepth;

RVA(0x000169e0, 0xa)
void ClearGameStateStack(void) {
    s_stateDepth = 0;
}

RVA(0x000169f0, 0x32)
void SaveGameState(void) {
    if (s_stateDepth < GAME_STATE_STACK_DEPTH) {
        s_stateStack[s_stateDepth++] = s_gameState;
    }
}

RVA(0x00016a30, 0x31)
void PopGameState(void) {
    if (s_stateDepth != 0) {
        s_gameState = s_stateStack[--s_stateDepth];
    }
}

RVA(0x00016a70, 0x24)
i16 __fastcall SetGameState(i16 state) {
    i16 old = s_gameState.state;
    s_gameState.state = state;
    s_gameState.phase = 0;
    s_gameState.step = 0;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016aa0, 0x23)
i16 SetGamePhase(i16 phase) {
    i16 old = s_gameState.phase;
    s_gameState.phase = phase;
    s_gameState.step = 0;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016ad0, 0x12)
i16 NextGamePhase(void) {
    return SetGamePhase(s_gameState.phase + 1);
}

RVA(0x00016af0, 0x12)
i16 PrevGamePhase(void) {
    return SetGamePhase(s_gameState.phase - 1);
}

RVA(0x00016b10, 0x1c)
i16 SetGamePhaseKeepStep(i16 phase) {
    i16 old = s_gameState.phase;
    s_gameState.phase = phase;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016b30, 0x12)
i16 PrevGamePhaseKeepStep(void) {
    return SetGamePhaseKeepStep(s_gameState.phase - 1);
}

RVA(0x00016b50, 0x16)
i16 __fastcall SetGameStep(u16 step) {
    i16 old = s_gameState.step;
    s_gameState.step = step;
    s_gameState.sub = 0;
    return old;
}

RVA(0x00016b70, 0xd)
i16 NextGameStep(void) {
    return SetGameStep(s_gameState.step + 1);
}

RVA(0x00016b80, 0xd)
i16 PrevGameStep(void) {
    return SetGameStep(s_gameState.step - 1);
}

RVA(0x00016b90, 0xe)
i16 __fastcall SetGameSub(i16 sub) {
    i16 old = s_gameState.sub;
    s_gameState.sub = sub;
    return old;
}

RVA(0x00016ba0, 0xe)
i16 NextGameSub(void) {
    return SetGameSub(s_gameState.sub + 1);
}

RVA(0x00016bb0, 0xe)
i16 PrevGameSub(void) {
    return SetGameSub(s_gameState.sub - 1);
}

RVA(0x00016bc0, 0x7)
i16 GetGameState(void) {
    return s_gameState.state;
}

RVA(0x00016bd0, 0x7)
u16 GetGamePhase(void) {
    return s_gameState.phase;
}

RVA(0x00016be0, 0x7)
u16 GetGameStep(void) {
    return s_gameState.step;
}

RVA(0x00016bf0, 0x7)
u16 GetGameSub(void) {
    return s_gameState.sub;
}

RVA(0x00016c00, 0x16)
void __fastcall PushGameState(i16 state) {
    SaveGameState();
    SetGameState(state);
}

// @identity-TODO: a one-line wrapper placed after PushGameState in this TU,
// compiled to a tail jump into PopGameState.
RVA(0x00016c20, 0x5)
void ReturnFromGameState(void) {
    PopGameState();
}
