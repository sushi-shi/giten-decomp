#ifndef GITEN_GAME_ITEMPOOL_H
#define GITEN_GAME_ITEMPOOL_H

#include <rva.h>

#include <Ints.h>

// A pool of items held aside and later moved into the bag (g_itemPool in
// <Game/BagItems.h>, filled by AddToPool, emptied by ClearPool when panel
// 0x113 opens).
// @identity-TODO: what the pool represents is unrecovered.

// Stores every pooled entry into the bag (a remapped item as a rolled amount
// of its replacement); returns how many did not fit.
i16 GivePooledItems(void);

// Takes every pooled entry's count back out of the bag.
void TakePooledItems(void);

// Whether the pooled items fit in the bag.
i32 PooledItemsFit(void);

#endif // GITEN_GAME_ITEMPOOL_H
