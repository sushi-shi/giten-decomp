// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/ClickWait.h>
#include <Game/StatusScreen.h>
#include <Input/Mouse.h>
#include <Input/MouseClickState.h>

RVA(0x00041ab0, 0x1a)
i16 TakeClickUnlessCancel(i16 command) {
    if (command == STATUS_COMMAND_CANCEL) {
        return CLICK_WAIT_CANCELLED;
    }
    return TakeMouseLeftClick() != MOUSE_CLICK_NONE;
}
