// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldScreen.h>
#include <Game/ModeFlags.h>
#include <Game/StateStack.h>
#include <Script/Script.h>
#include <Script/ScriptVars.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/Message.h>
#include <Ui/MessageScript.h>

// The shared message window: its handle (-1 while closed), the ticks left
// before it closes by itself, and whether it is held open (no countdown).
DATA(0x00068300)
static i16 s_messageWindow = -1;

DATA(0x000716f0)
static i16 s_textStateRefreshPending = 0;

DATA(0x000716f4)
static i16 s_messageLifetime = 0;

DATA(0x000716f8)
static i16 s_messageHold = 0;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002500, 0x50)
void PushTextWindowState(const char* text) {
    i16 plane;
    PushGameState(GAME_STATE_TEXT_WINDOW);
    plane = CreateTextPlane(15, 0x4000);
    PrintWindowText(plane, text, 0, 0, 1);
    SetGamePhase(plane);
    s_textStateRefreshPending = 0;
}

RVA(0x00002550, 0x70)
b16 RunTextWindowState(void) {
    i16 plane;
    switch (GetGameSub()) {
        case 0:
            plane = GetGamePhase();
            RepaintTextPlane(plane, 1);
            NextGameSub();
            TestFeatureMask(1);
            break;
        case 1:
            CloseTextWindow(GetGamePhase());
            ReturnFromGameState();
            break;
    }
    if (s_textStateRefreshPending) {
        s_textStateRefreshPending = 0;
        return UpdateFieldScreen(1);
    }
    return false;
}

RVA(0x000025c0, 0x40)
i16 OpenMessageWindow(void) {
    if (s_messageWindow == -1) {
        s_messageWindow = CreateTextPlane(15, 0);
    } else {
        ClearTextPlane(s_messageWindow);
    }
    ClearTextPlane(s_messageWindow);
    return s_messageWindow;
}

RVA(0x00002600, 0x30)
i16 OpenMessageText(void) {
    if (s_messageWindow == -1) {
        s_messageWindow = CreateTextPlane(15, 0);
    }
    ClearTextPlaneText(s_messageWindow);
    return s_messageWindow;
}

RVA(0x00002630, 0x16)
void SetMessageLifetime(i16 ticks) {
    if (ticks < 1) {
        ticks = 15;
    }
    s_messageLifetime = ticks;
}

// Returns the previous hold state; -1 only queries it. With no window open
// the message is held.
RVA(0x00002650, 0x35)
i16 SetMessageHold(i16 hold) {
    i16 old;
    if (hold == -1) {
        return s_messageHold;
    }
    if (s_messageWindow == -1) {
        s_messageHold = 1;
        return 1;
    }
    old = s_messageHold;
    s_messageHold = hold;
    return old;
}

RVA(0x00002690, 0x30)
i16 RefreshMessageWindow(void) {
    if (s_messageWindow == -1) {
        return s_messageWindow;
    }
    RepaintTextPlane(s_messageWindow, -2);
    s_messageHold = 1;
    return s_messageWindow;
}

RVA(0x000026c0, 0x30)
i16 StartMessageTimer(i16 ticks, i16 hold) {
    RefreshMessageWindow();
    SetMessageLifetime(ticks);
    SetMessageHold(hold);
    return ticks;
}

RVA(0x000026f0, 0x30)
void ShowMessage(const char* text, i16 ticks) {
    OpenMessageWindow();
    PrintWindowText(s_messageWindow, text, 0, 1, 1);
    StartMessageTimer(ticks, 0);
}

RVA(0x00002720, 0x20)
i16 CloseMessageWindow(void) {
    if (s_messageWindow != -1) {
        CloseTextWindow(s_messageWindow);
        s_messageWindow = -1;
    }
    return s_messageWindow;
}

RVA(0x00002740, 0x40)
void TickMessageWindow(void) {
    if (s_messageWindow != -1 && s_messageHold == 0 && s_messageLifetime != 0) {
        if (--s_messageLifetime <= 0) {
            CloseMessageWindow();
        }
    }
}

RVA(0x00002780, 0x50)
void RunMessageScene(i16 scene, i16 entry, i16 ticks) {
    PushGameState(GAME_STATE_MESSAGE_SCENE_END);
    SetGamePhase(ticks);
    OpenMessageWindow();
    RefreshMessageWindow();
    SetMessageHold(1);
    StartDebugScene(scene, entry, s_messageWindow);
}

RVA(0x000027d0, 0x30)
b16 FinishMessageScene(void) {
    SetMessageHold(0);
    SetMessageLifetime(GetGamePhase());
    ReturnFromGameState();
    return false;
}

RVA(0x00002800, 0x50)
void RunMessageScript(i16 script, i16 entry, i16 ticks) {
    i16 hold;
    OpenMessageWindow();
    hold = SetHold(1);
    RunScript(script, entry, s_messageWindow);
    SetHold(hold);
    StartMessageTimer(ticks, 0);
}

RVA(0x00002850, 0x50)
void RunMessageTextScript(i16 script, i16 entry, i16 ticks) {
    i16 hold;
    OpenMessageText();
    hold = SetHold(1);
    RunScript(script, entry, s_messageWindow);
    SetHold(hold);
    StartMessageTimer(ticks, 0);
}
