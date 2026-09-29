#ifndef GITEN_GAME_TREASUREBOX_H
#define GITEN_GAME_TREASUREBOX_H

#include <rva.h>

#include <Game/AreaMap.h>
#include <Ints.h>
#include <Script/EventFlags.h>

// The map's treasure boxes. Kept out of Game/FieldView.h: adding these there
// shifts field.c's TU state (RunFieldEncounter's loads swap operands; see
// docs/patterns/tu-state-probe-family-decides-reachability.md).

// The x of the record ending the box list.
#define TREASURE_BOX_END 0xff

// @identity-TODO: the box kind drawn from the lower half of the box texture.
#define TREASURE_BOX_LOWER 0x8a

typedef struct TreasureBoxCell {
    u8 x;
    u8 y;
} TreasureBoxCell;

static __inline b32 IsTreasureBoxOpen(TreasureBox* box) {
    return IsEventFlagSet(box->flagBank, box->flagIndex) != 0;
}

void PrepareViewedTreasureBox(void);
void IsHotspotTreasureOpen(i32 index);
void OpenTreasureBox(TreasureBox* box);
u16 RotateByDirection(u16 mask, i16 direction);

#endif // GITEN_GAME_TREASUREBOX_H
