// @identity-TODO: the owning TU is unproven; this unit holds the status
// screen entry and exit span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldScreen.h>
#include <Game/Party.h>
#include <Game/StateStack.h>
#include <Game/StatusScreen.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/Vram.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>

// @identity-TODO: the menu plane is only read; its creator is unrecovered.
DATA(0x00068a50)
static i16 s_dismissMenuPlane = -1;

DATA(0x0007bea4)
static PaletteState* s_statusPaletteState;

DATA(0x0007bec8)
Character* g_rosterPendingMember;

DATA(0x0007becc)
i16 g_rosterReturnState;

DATA(0x0007bed0)
u16 g_rosterReturnPhase;

DATA(0x0007bed4)
u16 g_rosterReturnStep;

DATA(0x0007bed8)
static i16 s_rosterSavedColumn;

RVA(0x00016fc0, 0x2d)
void EnterStatusScreen(i16 nested) {
    ClearStatusPicture();
    if (!nested) {
        SetSubscreenActive(1);
        s_statusPaletteState = SavePaletteState(s_statusPaletteState, 2);
    }
}

RVA(0x00016ff0, 0x31)
void LeaveStatusScreen(i16 nested) {
    ResetStatusMenu();
    if (!nested) {
        if (s_statusPaletteState) {
            s_statusPaletteState = RestorePaletteState(s_statusPaletteState, 1);
        }
        SetSubscreenActive(0);
    }
}

RVA(0x00017030, 0x60)
b16 RunDismissMenuState(void) {
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            ResetTextPlaneHighlight(s_dismissMenuPlane);
        case 1:
            if (PollMenuInput(s_dismissMenuPlane)) {
                NextGamePhase();
            }
            break;
        case 2:
            CloseTextWindow(s_dismissMenuPlane);
            ReturnFromGameState();
            break;
    }
    return false;
}

// @early-stop load width: retail loads the saved state and step into ecx
// with dword reads for fastcall arguments; this build uses cx. Their sole
// writer stores words, so widening the saved globals would misstate storage.
RVA(0x00017090, 0xc1)
b16 ReplaceRosterMember(void) {
    i16 selected;
    switch (GetGamePhase()) {
        case 0:
            s_rosterSavedColumn = SetStatusColumn(2);
            NextGamePhase();
        case 1:
            selected = RunStatusListPicker(0);
            if (selected == -1 || selected == -2) {
                break;
            }
            NextGamePhase();
        case 2:
            // Retail leaves selected uninitialized on direct entry to this phase.
            RemoveFromRoster(selected);
            SetStatusColumn(s_rosterSavedColumn);
            RunStatusListPicker(1);
            if (AddToRoster(g_rosterPendingMember) < 0) {
                SetGamePhase(0);
            }
            RestoreRosterReturnState();
            break;
    }
    return false;
}
