#ifndef GITEN_GAME_BAGITEMS_H
#define GITEN_GAME_BAGITEMS_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Character.h>
#include <Game/ItemStack.h>
#include <Ints.h>

#include <stdio.h>

// The bag's storage and the item module's own bag helpers.
// Codegen constraint: kept apart from <Game/ItemBag.h>, which statuspanel.c
// and skilluse.c include and whose declaration count their codegen follows.

// The party's 64-entry item bag.
extern ItemStack g_bagItems[64];

// Empties every bag entry.
void ClearBag(void);

// Copies the non-empty entries among the `count` from `first` (clamped to the
// bag) into `list` (allocated to fit when NULL) and returns it.
ItemStackList* CopyBagEntries(i16 first, i16 count, ItemStackList* list);

// Puts a copied list back into the first bag entries; returns how many (0 for
// a NULL list).
i16 RestoreBagEntries(ItemStackList* list);

// Sets entry `index` to `item` with `attachment` (-1: none), holding nothing.
void SetBagEntry(i16 index, i16 item, i16 attachment);

// Takes up to `amount` from entry `index`, emptying it when none are left;
// returns the amount taken.
i16 TakeFromBagEntry(i16 index, i16 amount);

// Takes `amount` of `item` from the bag, last entries first; returns how many
// could not be taken.
i16 TakeBagItemsFromEnd(i16 item, i16 amount);

// Entry `index` itself, and how many it holds.
ItemStack* GetBagEntry(i16 index);
i16 GetBagEntryCount(i16 index);

// The number of non-empty bag entries.
i16 CountBagEntries(void);

// Copies the bag into `buffer` (allocated when NULL), or with `restore` set
// back from it (a NULL buffer does nothing); returns `buffer`.
ItemStack* SaveOrRestoreBag(ItemStack* buffer, i16 restore);

// Takes `amount` of `item` from entry `index` first, then from the end of the
// bag; returns how many could not be taken.
i16 TakeBagItemsAt(i16 index, i16 item, i16 amount);

i16 GetBagEntryDetail(i16 index);

// The item pool the bag is refilled from (<Game/ItemPool.h>).
// @identity-TODO: what the pool represents is unrecovered.
extern ItemStack g_itemPool[64];

#define GetItemPoolEntry(index) (&g_itemPool[(index)])

// Sets whether bag stores are quiet (no discard menu for a full bag and no
// acquisition stamp); returns the previous setting.
i16 SetBagQuiet(i16 quiet);

// Whether the curse of the item equipped in `part` applies to `character`:
// the item has a curse and the character's level is below its minimum.
b16 IsEquipCurseActive(Character* character, i16 part);

// The item category of `id`'s kind: 0..7 for kinds 11..19, -1 otherwise.
// @identity-TODO: what the categories name is unrecovered.
GZ_ENUM_RETURN(EquipPart, i16) GetItemCategory(i16 id);

// How many of `id` are held (in the gem table or the bag).
i16 CountHeldItem(i16 id);

// The pool's entries, how many are used, and emptying it.
ItemStack* GetPoolEntries(void);
i16 CountPoolEntries(void);
void ClearPool(void);

// Adds `amount` of `item` to the pool, topping up its entries to 99 before
// starting a new one.
void AddToPool(i16 item, i16 amount);

// Takes `amount` of `item` from its first pool entry, emptying it when none
// are left.
void TakeFromPool(i16 item, u8 amount);

// Adds `count` of `item` with `attachment` and `detail` to the bag (scenario items
// to the last sixteen entries, others to the first 48), compacting it first;
// returns how many did not fit.
i16 AddBagItems(i16 item, i16 count, i16 attachment, i16 detail);

// Adds `count` of scenario item `item` to the last sixteen bag entries;
// returns how many did not fit.
i16 AddScenarioBagItems(i16 item, i16 count);

// The last bag entry holding `item` among the first 48 (bit 0 of `groups`)
// or the last sixteen (bit 1), else -1.
i16 FindBagItem(i16 item, u8 groups);

// Adds `amount` to entry `index` up to `limit`; returns the amount added.
i16 AddToBagEntry(i16 index, u16 amount, u16 limit);

// Sets entry `index`'s detail; returns the previous one.
i16 SetBagEntryDetail(i16 index, i16 detail);

// Adds `amount` of `item` (up to `limit`) to entry `index` when it is empty
// (taking `attachment`) or already holds `item`, setting its detail; returns the
// amount added (0 when the entry holds another item).
i16 FillBagEntry(i16 index, i16 item, u16 amount, u16 limit, i16 attachment, i16 detail);

// Merges partly filled stacks and packs the bag's entries, keeping scenario
// items (at most sixteen) in the last sixteen.
void CompactBag(void);

// The gem item attached to entry `index`, else
// -1; detaching clears the attachment and returns the item that was attached;
// attaching `id` returns the item it replaced.
i16 GetBagEntryAttachment(i16 index);
i16 DetachBagEntryItem(i16 index);
i16 AttachBagEntryItem(i16 index, i16 id);

// Writes / reads the bag; returns how many entries were not transferred.
i16 WriteBag(FILE* fp);
i16 ReadBag(FILE* fp);

// Lets the player pick a bag item to discard when the bag is full (among the
// entries whose item has a price and is not a scenario item) and empties its entry.
#ifdef GITEN_BUGFIX
// False when there was none to pick and the player closed the menu.
b32 RunBagDiscardMenu(void);
#else
void RunBagDiscardMenu(void);
#endif

// Clears the mouse clicks and runs one frame (a modal menu's wait).
void WaitMenuFrame(void);

#endif // GITEN_GAME_BAGITEMS_H
