#ifndef GITEN_GAME_WAITSTATE_H
#define GITEN_GAME_WAITSTATE_H

#include <rva.h>
#include <Platform/ScreenFade.h>

#include <EnumDomain.h>

GZ_ENUM_BEGIN_SPLIT(WaitMode, i16)
    WAIT_FRAMES = 0,
    WAIT_INPUT = 1,
    WAIT_INPUT_OR_FRAMES = 2,
    WAIT_FADE = 3,
GZ_ENUM_END_SPLIT(WaitMode)

// The inputs that end an input wait; a right click always does.
GZ_ENUM_FLAGS_BEGIN(WaitInputMask, u16)
    WAIT_ON_LEFT_DOWN = 1,
    WAIT_ON_LEFT_PRESSED = 2,
    WAIT_ON_ANY_INPUT = 0xffff
GZ_ENUM_FLAGS_END(WaitInputMask)

// The wait and fade states pushed on the game-state stack.

// @identity-TODO: script waits pass a text window as the fourth argument;
// other callers pass -1. The Windows body never reads it.
void PushWaitState(
    GZ_ENUM_STORAGE(WaitMode, i16) mode,
    GZ_ENUM_PARAM(WaitInputMask, u16) inputMask,
    u16 frames,
    i16 unused
);

static __inline void StartScreenFadeAndWait(GZ_ENUM_PARAM(ScreenFadeMode, i16) kind, i16 speed) {
    StartScreenFade(kind, speed);
    PushWaitState(WAIT_FADE, 0, 0, -1);
}

void PushScreenFade(GZ_ENUM_PARAM(ScreenFadeMode, i16) kind, i16 speed);

void FadeScreenAndWait(GZ_ENUM_PARAM(ScreenFadeMode, i16) kind, i16 speed);

// Opens a text plane and pushes the state that waits for input, then closes it.
i16 PushMessageBox(i16 window, const char* text);

#endif // GITEN_GAME_WAITSTATE_H
