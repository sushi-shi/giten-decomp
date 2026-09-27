#ifndef GITEN_GAME_WAITSTATE_H
#define GITEN_GAME_WAITSTATE_H

#include <rva.h>
#include <Platform/ScreenFade.h>

#include <EnumDomain.h>

GZ_ENUM_BEGIN_SPLIT(WaitMode, i16)
WAIT_FRAMES = 0, WAIT_INPUT = 1, WAIT_INPUT_OR_FRAMES = 2, WAIT_FADE = 3,
                 GZ_ENUM_END_SPLIT(WaitMode)

    // The wait and fade states pushed on the game-state stack.

    // @identity-TODO: script waits pass a text window as the fourth argument;
    // other callers pass -1. The Windows body never reads it.
    void PushWaitState(GZ_ENUM_STORAGE(WaitMode, i16) mode, u16 inputMask, u16 frames, i16 unused);

static __inline void StartScreenFadeAndWait(i16 kind, i16 speed) {
    StartScreenFade(kind, speed);
    PushWaitState(WAIT_FADE, 0, 0, -1);
}

void PushScreenFade(i16 kind, i16 speed);

void FadeScreenAndWait(i16 kind, i16 speed);

// Opens a text plane and pushes the state that waits for input, then closes it.
i16 PushMessageBox(i16 window, const char* text);

#endif // GITEN_GAME_WAITSTATE_H
