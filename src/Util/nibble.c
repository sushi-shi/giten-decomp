// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Text/TextAttr.h>

// Codegen: the unit's TU state includes the CRT stdio header.
#include <stdio.h>

RVA(0x000451d0, 0x2d)
void SpreadLowNibble(TextAttr* attr) {
    attr->fg = attr->bg;
    attr->dim = attr->bg;
}

RVA(0x00045200, 0x30)
u16 SwapOuterNibbles(u16 value) {
    TextAttr attr;
    TextAttr swapped;
    u16 fg;

    attr.value = value;
    swapped = attr;
    fg = attr.fg;
    swapped.fg = attr.bg;
    swapped.bg = fg;
    return swapped.value;
}

RVA(0x00045230, 0x24)
u16 SwapMiddleNibbles(u16 value) {
    TextAttr attr;
    TextAttr swapped;

    attr.value = value;
    swapped = attr;
    swapped.fg = attr.dim;
    swapped.dim = attr.fg;
    return swapped.value;
}

RVA(0x00045260, 0x1e)
u16 SwapLowNibbles(u16 value) {
    TextAttr attr;
    TextAttr swapped;

    attr.value = value;
    swapped = attr;
    swapped.dim = attr.bg;
    swapped.bg = attr.dim;
    return swapped.value;
}
