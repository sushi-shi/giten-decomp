// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AreaNpc.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/TreasureBox.h>
#include <Script/EventFlags.h>

#include <string.h>

DATA(0x0007d5b2)
static TreasureBoxCell s_boxCell;

DATA(0x0007d5b4)
static MapPosition s_boxPosition;

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x0001aaa0, 0xa0)
void PrepareViewedTreasureBox(void) {
    TreasureBox* box = FindTreasureBoxAt(g_viewCellX, g_viewCellY, 0);
    if (box) {
        IsTreasureBoxOpen(box);
        GetApproachOffset(g_viewLateral, g_viewDepth);
        s_boxPosition = g_field.pos;
        s_boxPosition.x = g_viewLateral;
        s_boxPosition.y = g_viewDepth;
        memcpy(&s_boxCell, &box->head, sizeof(s_boxCell));
    }
}
