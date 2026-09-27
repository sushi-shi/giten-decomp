// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Input/Mouse.h>
#include <Ui/Menu.h>

RVA(0x00045320, 0xc)
void SetHoveredObject(i16 item) {
    g_hoveredObjectId = item;
}

RVA(0x00045330, 0xc)
void SetSelectedObject(i16 value) {
    g_selectedObjectId = value;
}
