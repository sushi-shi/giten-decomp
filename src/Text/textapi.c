// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Text/TextPlane.h>

DATA(0x00083c98)
static TextPlaneHook s_hook;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00045280, 0xe)
void RefreshTextPlane(i16 plane) {
    RedrawTextPlane(plane);
}

RVA(0x00045290, 0x2a)
void ToggleTextHighlight(i16 plane, i16 x, i16 y) {
    RedrawTextRun(plane, x, y, ToggleTextRunHighlight(plane, x, y));
}

RVA(0x000452c0, 0x2f)
void PaintTextRun(i16 plane, i16 x, i16 y, u16 attr) {
    RedrawTextRun(plane, x, y, SetTextRunAttr(plane, x, y, attr));
}

RVA(0x000452f0, 0x10)
TextPlaneHook SetTextPlaneHook(TextPlaneHook hook) {
    TextPlaneHook old = s_hook;
    s_hook = hook;
    return old;
}

RVA(0x00045300, 0x1e)
void CallTextPlaneHook(i16 plane, i16 event, i16 value) {
    if (s_hook != NULL) {
        s_hook(plane, event, value);
    }
}
