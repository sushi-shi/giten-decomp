#ifndef GITEN_GFX_SCREENMODE_H
#define GITEN_GFX_SCREENMODE_H

#include <rva.h>

// The screen mode the frame renderer draws.

// @identity-TODO: 0x48fc60 lets the message handler 0x50540 skip its 0x3c-count input delay
// (0x490ac0) and is cleared after 0x51xxx dispatch; confirm what 0x490ac0 counts.
RVA_DECL(0x00049b80)
void AllowImmediateInput(void);

RVA_DECL(0x00049c30)
void RefreshScreenMode(void);

// @identity-TODO: the role of dword 0x490ac8 (1 here, 0 in RestoreScreenMode) is unrecovered.
RVA_DECL(0x00049c50)
void SaveScreenMode(void);

// @identity-TODO: same 0x490ac8 role as SaveScreenMode.
RVA_DECL(0x00049c80)
void RestoreScreenMode(void);

// @identity-TODO: that 0x1de40 (called with g_field.pos by value via 0x1e6f0) draws the first-
// person view is inferred; decode 0x1de40.
RVA_DECL(0x00049fa0)
void RedrawFieldView(void);

#endif // GITEN_GFX_SCREENMODE_H
