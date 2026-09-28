// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/BagItems.h>
#include <Game/FieldHud.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSupport.h>
#include <Game/MenuCursor.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Game/WorldMap.h>
#include <Gfx/ScreenLayer.h>
#include <Input/Mouse.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Text/TextPlane.h>
#include <Ui/Message.h>

// A four-level cursor: setting a level resets every level below it.
RVA(0x00016c30, 0x12)
i16 SetCursorLevel3(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[3];
    cursor->level[3] = value;
    return old;
}

RVA(0x00016c50, 0x22)
i16 SetCursorLevel2(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[2];
    cursor->level[2] = value;
    SetCursorLevel3(cursor, 0);
    return old;
}

RVA(0x00016c80, 0x22)
i16 SetCursorLevel1(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[1];
    cursor->level[1] = value;
    SetCursorLevel2(cursor, 0);
    return old;
}

RVA(0x00016cb0, 0x20)
i16 SetCursorLevel0(MenuCursor* cursor, i16 value) {
    i16 old = cursor->level[0];
    cursor->level[0] = value;
    SetCursorLevel1(cursor, 0);
    return old;
}

RVA(0x00016cd0, 0x14)
i16 NextCursorLevel0(MenuCursor* cursor) {
    return SetCursorLevel0(cursor, cursor->level[0] + 1);
}

RVA(0x00016cf0, 0x14)
i16 PrevCursorLevel0(MenuCursor* cursor) {
    return SetCursorLevel0(cursor, cursor->level[0] - 1);
}

RVA(0x00016d10, 0x8)
u16 GetCursorLevel0(MenuCursor* cursor) {
    return cursor->level[0];
}

RVA(0x00016d20, 0x9)
i16 GetCursorLevel1(MenuCursor* cursor) {
    return cursor->level[1];
}

RVA(0x00016d30, 0x11c)
i16 PickStatusMember(void) {
    i16 selected;
    switch (GetGameStep()) {
        case 2:
            selected = PollStatusMenu();
            if (selected == 2 || selected == -2) {
                CheckStatusMenuItem(2);
                SetGameStep(0xffff);
            } else {
                selected = RunStatusListPicker(0);
                if (selected >= 0) {
                    PrevGameStep();
                    SetGameSub(selected);
                }
            }
            break;
        case 0:
            SetGameStep(2);
            TakeMouseCancelSound();
            SetStatusMenuItemsHidden(1);
            SetStatusMenuItemFlag(3, 0x800, !CountBagEntries());
            RedrawPartyStatus();
            RepaintTextPlane(g_infoPlane, 1);
            ClearStatusMenu();
            SetStatusColumn(0);
            RunStatusListPicker(0);
            break;
        case 1:
        case 0xffff:
            SetStatusMenuItemsHidden(0);
            RunStatusListPicker(1);
            if (GetGameStep() == 0xffff) {
                return -3;
            }
            return GetGameSub();
    }
    return -1;
}

// @identity-TODO: the status screen's analyze mode flag.
DATA(0x0007be44)
static i16 s_statusAnalyzeMode;

DATA(0x0007be3c)
i16 g_statusMember;

DATA(0x0007be40)
i16 g_statusFixedMember;

RVA(0x00016e50, 0xc)
void SetStatusAnalyzeMode(i16 on) {
    s_statusAnalyzeMode = on;
}

RVA(0x00016e60, 0x7)
i16 GetStatusAnalyzeMode(void) {
    return s_statusAnalyzeMode;
}

RVA(0x00016e70, 0x144)
b16 RunStatusScreen(void) {
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            SetPictureRenderMode();
            HideScreenLayer(1);
            CloseMessageWindow();
            SetGamePhase(2);
            SetStatusMenuItemsHidden(0);
            EnterStatusScreen(0);
            if (s_statusAnalyzeMode) {
                NextGamePhase();
                g_statusMember = 15;
                g_statusFixedMember = 1;
            }
            break;
        case 1:
            ReturnFromGameState();
            RunStatusListPicker(1);
            RequestFieldRefresh();
            LeaveStatusScreen(0);
            g_statusFixedMember = 0;
            SetFieldPanelRowChecked(1, 0);
            g_statusMember = 0;
            ErasePictureSurface(0x36);
            ClearStatusPicture();
            break;
        case 2:
            g_statusMember = PickStatusMember();
            if (g_statusMember == -3) {
                PrevGamePhase();
            } else if (g_statusMember != -1) {
                NextGamePhase();
            }
            break;
        case 3:
            result = RunStatusCommands();
            if (result == -3) {
                ErasePictureSurface(0x36);
                SetGamePhase(1);
            } else if (result == 7 || result == -2) {
                PrevGamePhase();
                ClearStatusPicture();
                ErasePictureSurface(0x36);
            }
            break;
    }
    return false;
}
