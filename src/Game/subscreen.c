// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/FieldHud.h>
#include <Game/FieldScreen.h>

// @identity-TODO: only the reset and exchange access this flag in this build.
DATA(0x0007be58)
static i16 s_subscreenActive;

RVA(0x0001a1b0, 0x1d)
void InitFieldPanels(void) {
    u32 image = LoadMenuImage(1);
    InitWorldPanel();
    SetFieldPanelImage(image);
}

RVA(0x0001a1d0, 0xa)
void ResetSubscreen(void) {
    s_subscreenActive = 0;
}

RVA(0x0001a1e0, 0x12)
i16 SetSubscreenActive(i16 active) {
    i16 old = s_subscreenActive;
    s_subscreenActive = active;
    return old;
}
