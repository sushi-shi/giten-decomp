#ifndef GITEN_GAME_GEMITEMS_H
#define GITEN_GAME_GEMITEMS_H

#include <Game/ItemStack.h>
#include <Ints.h>

#include <stdio.h>

// The sixteen gem items, numbered from the loaded gem base, with the count
// held of each.
extern ItemStack g_gemItems[16];

#define GetGemItemEntry(index) (&g_gemItems[(index)])

// The most of item `id` one entry holds: 99 for most kinds, 1 for kinds 11,
// 12 and 14..18 (and unknown kinds).
u16 GetItemStackLimit(i16 id);

// The index of gem item `id` in the table, else -1.
i16 GemItemIndex(i16 id);

// Adds `amount` to entry `index` up to the item's stack limit; returns the
// amount added (-1 for a bad index).
i16 AddGemItemsAt(i16 index, u16 amount);

// Takes up to `amount` from entry `index`; returns the amount taken (-1 for a
// bad index).
i16 TakeGemItemsAt(i16 index, i16 amount);

// The first item id in the loaded gem group.
i16 GetGemItemBase(void);

// Numbers the table from `base` and empties every entry.
void ResetGemItems(i16 base);

// AddGemItemsAt/TakeGemItemsAt by item id (-1 for an id outside the
// table).
i16 AddGemItems(i16 id, u16 amount);
i16 TakeGemItems(i16 id, i16 amount);

// The count held of entry `index` / item `id` (-1 outside the table).
i16 CountGemItemsAt(i16 index);
i16 CountGemItems(i16 id);

// Copies the table into `buffer` (allocated when NULL) / back from `buffer`
// (a NULL buffer does nothing); both return `buffer`.
ItemStack* SaveGemItems(ItemStack* buffer);
ItemStack* RestoreGemItems(ItemStack* buffer);

// Writes / reads the table; returns how many entries were not transferred.
i16 WriteGemItems(FILE* fp);
i16 ReadGemItems(FILE* fp);

// The game state that lets the player give a gem item to the script's
// actor from a menu (phase 0 opens it, 2 runs it, 1 closes it); the item
// raises the actor's familiarity.
i16 RunGemItemGift(void);

#endif // GITEN_GAME_GEMITEMS_H
