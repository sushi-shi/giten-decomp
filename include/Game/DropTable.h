#ifndef GITEN_GAME_DROPTABLE_H
#define GITEN_GAME_DROPTABLE_H

#include <Game/ItemId.h>
#include <Ints.h>

// An item dropped for the party: the item id (-1 when empty) and the amount.
typedef struct DropSlot {
    i16 item;
    i16 amount;
} DropSlot;

// The sixteen reward slots filled by battle actions and consumed by scripts.
extern DropSlot g_dropSlots[16];

static __inline DropSlot* GetDropSlot(i16 slot) {
    return &g_dropSlots[slot];
}

static __inline void ClearDropSlot(i16 slot) {
    GetDropSlot(slot)->item = ITEM_ID_EMPTY;
    GetDropSlot(slot)->amount = 0;
}

// The handle of the item remap list: pairs of (item, replacement) ids ended
// by -1.
// @identity-TODO: what the remap stands for is unrecovered.
extern i32 g_itemRemapHandle;

// Item reward id at `index` in the startup-loaded reward table.
i16 GetItemRewardAt(i16 index);

// Empties every drop slot.
void ClearDropSlots(void);

// Adds `amount` of `item` (a remapped item as a rolled amount of its
// replacement) to the first empty drop slot or the one holding it; returns
// the slot (-1 for no item or no room).
i16 AddDropSlot(i16 item, i16 amount);

// The replacement listed for `item`, else 0.
i16 RemapItem(u16 item);

// The sum of `count` amounts of `item`: each its high value, or with `random`
// set a random average between its low and high values.
i16 RollItemAmount(i16 item, i16 count, i16 random);

static __inline i16 RollDropAmount(i16 item, i16 amount) {
    return RollItemAmount(item, amount, 1);
}

#endif // GITEN_GAME_DROPTABLE_H
