// @identity-TODO: the owning TU is unproven; this unit holds the window
// text-writing span until link-order evidence names it.

#include <rva.h>

#include <Script/TextState.h>
#include <Text/Font.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>

#include <mbstring.h>

// @early-stop: retail assigns window/choosing to ebx/ebp and uses the opposite
// style-bit scratch registers; declaration-order and TU-state controls are flat.
RVA(0x00045340, 0xc7)
void PrintWindowText(i16 window, const char* text, u16 attr, i16 style, i16 noKinsoku) {
    TextState state;
    u16 ch;
    u16 savedAttr;
    u16 activeAttr;
    i16 pos;

    if (window == TEXT_PLANE_NONE) {
        return;
    }
    activeAttr = attr;
    if (!activeAttr) {
        activeAttr = GetTextPlaneAttr(window);
    }
    InitTextStateFlags(&state, style, 0);
    state.messageHookEnabled = false;
    state.delayRamp = 0;
    state.scrollEnabled = false;
    state.timedWait = false;
    state.inputWait = 0;
    savedAttr = SetTextPlaneAttr(window, activeAttr);
    for (pos = 0; text[pos];) {
        pos = ReadTextChar(&ch, text, pos);
        if (ch == '\n') {
            AdvanceWindowLine(window);
        } else {
            PutTextChar(window, ch, &state, noKinsoku);
        }
    }
    SetTextPlaneAttr(window, savedAttr);
}

RVA(0x00045410, 0x139)
i16 PutTextChar(i16 plane, u16 ch, TextState* state, i16 choosing) {
    i16 width = 1;
    i16 slack;
    i16 result;
    i16 x;
    i16 y;
    u16 attr;
    u8* text;
    TextAttr* attrs;

    if (IsTwoByteTextChar(ch)) {
        width = 2;
    }
    slack = 0;
    if (!choosing) {
        slack = -2;
        if (_mbschr(
                "\201\146\201\150\201\152\201\154\201\156\201\160\201\162\201\164\201\166\201\170"
                "\201\172\201\101\201\103\201\102\201\104\201\111\201\110",
                ch
            )) {
            slack = 0;
        } else if (_mbschr(
                       "\201\145\201\147\201\151\201\153\201\155\201\157\201\161\201\163\201\165"
                       "\201\167\201\171",
                       ch
                   )) {
            slack = -4;
        }
    }
    if (ReserveTextPlaneCells(plane, width, slack)) {
        SetWindowDeferredChar(plane, ch);
        return -2;
    }
    x = GetTextPlaneCursorX(plane);
    y = GetTextPlaneCursorY(plane);
    attr = GetTextPlaneAttr(plane);
    DrawTextCell(plane, ch, attr, x * 8, y);
    text = GetTextPlaneRowText(plane, y);
    attrs = GetTextPlaneAttrRow(plane, y);
    if (width > 1) {
        StoreTextCell(text, attrs, x, ch >> 8, attr);
        x++;
    }
    StoreTextCell(text, attrs, x, ch, attr);
    MoveTextPlaneCursorX(plane, width);
    result = state->delayOn ? width : 0;
    return result;
}
