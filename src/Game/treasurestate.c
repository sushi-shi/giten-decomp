// @identity-TODO: the owning TU is unproven; this unit holds the scene
// treasure-state query until link-order evidence names it.

#include <rva.h>

#include <Game/AreaMap.h>
#include <Game/SceneHotspot.h>
#include <Game/TreasureBox.h>
#include <Script/EventFlags.h>

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref).
RVA(0x0001ab40, 0x46)
i32 IsHotspotTreasureOpen(i32 index) {
    SceneSprite* sprite = GetHotspotSprite(index);
    TreasureBox* box = FindTreasureBoxAt(sprite->cellX, sprite->cellY, 0);
    if (box) {
        return IsTreasureBoxOpen(box);
    }
    return 0;
}
