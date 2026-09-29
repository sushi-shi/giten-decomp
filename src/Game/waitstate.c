// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/StateStack.h>
#include <Game/WaitLoop.h>
#include <Game/WaitState.h>
#include <Input/Mouse.h>
#include <Platform/PlatformApi.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>

RVA(0x0001a870, 0xc0)
b16 StepWaitState(void) {
    u16 inputMask;
    switch (GetGamePhase()) {
        case WAIT_FRAMES:
            if (PrevGameSub() == 1) {
                ReturnFromGameState();
            }
            break;
        case WAIT_INPUT_OR_FRAMES:
            if (PrevGameSub() == 1) {
                HideBusyCursor();
                ReturnFromGameState();
                break;
            }
            // Fall through to the input wait while the timer is running.
        case WAIT_INPUT:
            ShowBusyCursor();
            inputMask = GetGameStep();
            if ((inputMask & 1) && (g_mousePosition.buttons & MOUSE_LEFT_DOWN)) {
                HideBusyCursor();
                ReturnFromGameState();
                break;
            }
            if ((inputMask & 2) && (g_mousePosition.buttons & MOUSE_LEFT_PRESSED)) {
                HideBusyCursor();
                ReturnFromGameState();
                break;
            }
            if (g_mousePosition.buttons & MOUSE_RIGHT_DOWN) {
                HideBusyCursor();
                ReturnFromGameState();
            }
            break;
        case WAIT_FADE:
            if (GetScreenFade() <= SCREEN_FADE_NONE) {
                ReturnFromGameState();
            }
            break;
    }
    return false;
}

RVA(0x0001a930, 0x29)
void PushWaitState(GZ_ENUM_STORAGE(WaitMode, i16) mode, u16 inputMask, u16 frames, i16 unused) {
    PushGameState(GAME_STATE_WAIT);
    SetGamePhase(mode);
    SetGameStep(inputMask);
    SetGameSub(frames);
}

RVA(0x0001a960, 0x25)
b16 RunScreenFadeState(void) {
    GZ_ENUM_LOCAL(ScreenFadeMode, i16) kind = GetGamePhase();
    i16 speed = GetGameStep();
    ReturnFromGameState();
    StartScreenFade(kind, speed);
    return false;
}

RVA(0x0001a990, 0x30)
void PushScreenFade(GZ_ENUM_PARAM(ScreenFadeMode, i16) kind, i16 speed) {
    PushWaitState(WAIT_FADE, 0, 0, -1);
    PushGameState(GAME_STATE_SCREEN_FADE);
    SetGamePhase(kind);
    SetGameStep(speed);
}

RVA(0x0001a9c0, 0x23)
void FadeScreenAndWait(GZ_ENUM_PARAM(ScreenFadeMode, i16) kind, i16 speed) {
    PushWaitState(WAIT_FADE, 0, 0, -1);
    StartScreenFade(kind, speed);
}

RVA(0x0001a9f0, 0x41)
i16 PushMessageBox(i16 window, const char* text) {
    i16 plane;
    PushGameState(GAME_STATE_MESSAGE_BOX);
    plane = CreateTextPlane(window, 0x4000);
    PrintWindowText(plane, text, 0, 0, 1);
    SetGamePhase(plane);
    return plane;
}

RVA(0x0001aa40, 0x54)
b16 RunMessageBoxState(void) {
    i16 plane = GetGamePhase();
    switch (GetGameStep()) {
        case 1:
            CloseTextWindow(plane);
            ReturnFromGameState();
            break;
        case 0:
            NextGameStep();
            RepaintTextPlane(plane, 1);
            PushWaitState(WAIT_INPUT, 10, 0xffff, plane);
            break;
    }
    return false;
}
