// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Input/Mouse.h>
#include <Input/MouseClickState.h>
#include <Sound/Sound.h>

DATA(0x00091540)
GZ_ENUM_STORAGE(MouseClickState, i16) g_mouseLeftClick;

DATA(0x00091982)
i16 g_mouseLeftClickX;

DATA(0x00091984)
i16 g_mouseLeftClickY;

DATA(0x0009199c)
i16 g_mouseRightClickY;

DATA(0x0009199e)
i16 g_mouseRightClickX;

DATA(0x000919e0)
i16 g_hoveredObjectId;

DATA(0x000919e8)
MousePosition g_mousePosition;

DATA(0x000919ee)
i16 g_selectedObjectId;

DATA(0x000919fa)
GZ_ENUM_STORAGE(MouseClickState, i16) g_mouseRightClick;

RVA(0x00002a00, 0x5d)
void LatchMouseClicks(void) {
    if ((g_mousePosition.buttons & MOUSE_LEFT_PRESSED) && g_mouseLeftClick == MOUSE_CLICK_NONE) {
        g_mouseLeftClick = MOUSE_CLICK_PRESENT;
        g_mouseLeftClickX = g_mousePosition.x;
        g_mouseLeftClickY = g_mousePosition.y;
    }
    if ((g_mousePosition.buttons & MOUSE_RIGHT_PRESSED) && g_mouseRightClick == MOUSE_CLICK_NONE) {
        g_mouseRightClick = MOUSE_CLICK_PRESENT;
        g_mouseRightClickX = g_mousePosition.x;
        g_mouseRightClickY = g_mousePosition.y;
    }
}

RVA(0x00002a60, 0xf)
void ClearMouseClicks(void) {
    g_mouseLeftClick = MOUSE_CLICK_NONE;
    g_mouseRightClick = MOUSE_CLICK_NONE;
}

// A pending right-click cancels: consume the clicks, optionally drop the
// hovered and selected objects, and play the cancel sound.
RVA(0x00002a70, 0x40)
GZ_ENUM_RETURN(MouseClickState, i16) TakeMouseCancel(i16 clearSelection) {
    if (g_mouseRightClick == MOUSE_CLICK_NONE) {
        return MOUSE_CLICK_NONE;
    }
    ClearMouseClicks();
    if (clearSelection == 0) {
        return MOUSE_CLICK_NONE;
    }
    ClearMouseSelection();
    PlaySoundEffect(2);
    return MOUSE_CLICK_PRESENT;
}

RVA(0x00002ab0, 0x22)
GZ_ENUM_RETURN(MouseClickState, i16) TakeMouseCancelSound(void) {
    if (g_mouseRightClick == MOUSE_CLICK_NONE) {
        return MOUSE_CLICK_NONE;
    }
    ClearMouseClicks();
    PlaySoundEffect(2);
    return MOUSE_CLICK_PRESENT;
}

RVA(0x00002ae0, 0x18)
GZ_ENUM_RETURN(MouseClickState, i16) TakeMouseLeftClick(void) {
    if (g_mouseLeftClick != MOUSE_CLICK_NONE) {
        ClearMouseClicks();
        return MOUSE_CLICK_PRESENT;
    }
    return MOUSE_CLICK_NONE;
}

RVA(0x00002b00, 0x24)
void SetMouseState(GZ_ENUM_PARAM(MouseButtonBits, i16) buttons, i16 x, i16 y) {
    g_mousePosition.buttons = buttons;
    g_mousePosition.x = x;
    g_mousePosition.y = y;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002b30, 0x7)
i16 GetMouseX(void) {
    return g_mousePosition.x;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00002b40, 0x7)
i16 GetMouseY(void) {
    return g_mousePosition.y;
}

RVA(0x00002b50, 0x7)
GZ_ENUM_RETURN(MouseClickState, i16) GetMouseRightClick(void) {
    return g_mouseRightClick;
}

RVA(0x00002b60, 0x7)
GZ_ENUM_RETURN(MouseClickState, i16) GetMouseLeftClick(void) {
    return g_mouseLeftClick;
}
