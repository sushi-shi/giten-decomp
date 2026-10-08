#ifndef GITEN_GAME_ITEMBAG_H
#define GITEN_GAME_ITEMBAG_H

#include <rva.h>

#include <Game/Character.h>
#include <Ints.h>

// The party's 64-entry item bag (g_bagItems in <Game/BagItems.h>).

i16 GetBagItem(i16 index);

// Copies one item and its optional attachment from bag entry `index` into
// `slot`, and the entry's detail into `count`; returns hasAttachment.
// @identity-TODO: the entry detail returned through `count` is unrecovered.
i16 ReadBagEntry(i16 index, ItemSlot* slot, i16* count);

// The total count of `item` in the bag.
i16 CountBagItem(i16 item);

static __inline void LimitItemSlotToBag(ItemSlot* slot) {
    if (slot->quantity > CountBagItem(slot->item)) {
        slot->quantity = CountBagItem(slot->item);
    }
}

// Takes up to `count` of `item` out of the bag; returns how many were taken.
i16 TakeBagItems(i16 item, i16 count);

// Puts `count` of `item` (with its slot attachment) into the bag.
i16 StoreBagItem(i16 item, i16 count, i16 attachment);

// itemrecord's take from entry `index` first (also in <Game/BagItems.h>).
// Codegen constraint: declared here for party.c; including <Game/BagItems.h>
// there perturbs EquipItem (TU state).
i16 TakeBagItemsAt(i16 index, i16 item, i16 amount);

// Closes the gaps between the bag's entries.
RVA_DECL(0x00024310)
void CompactBag(void);

// Applies the effect of using item `item` by `user` on `target` (a copy of
// its record picks the handler by kind, <Game/ItemEffect.h>).
void ApplyItemEffect(i16 item, CharacterCore* user, CharacterCore* target);

#endif // GITEN_GAME_ITEMBAG_H
