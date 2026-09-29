// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/ClickWait.h>
#include <Input/Mouse.h>
#include <Input/MouseClickState.h>

// @identity-TODO: -1 for the key code -2, otherwise whether a left click was
// taken; the key-code convention of its callers is unrecovered.
RVA(0x00041ab0, 0x1a)
i16 TakeClickUnlessCancel(i16 key) {
    if (key == -2) {
        return -1;
    }
    return TakeMouseLeftClick() != MOUSE_CLICK_NONE;
}
