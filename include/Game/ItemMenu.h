#ifndef GITEN_GAME_ITEMMENU_H
#define GITEN_GAME_ITEMMENU_H

#include <rva.h>

#include <Game/ItemStack.h>

struct MenuBox;

// The stock file stores byte offsets to -1-terminated item-ID lists.
typedef struct ItemMenuStockTable {
    i16 count;
    u16 offsets[1];
} ItemMenuStockTable;

i16* GetItemMenuStock(i16 index);
void LoadItemMenuStock(void);
i16 CountItemMenuStock(i16 index);
i16 CopyItemMenuStock(i16 index, i16* items);
i16* AllocItemMenuStock(i16 index, i16* count);

// @identity-TODO: the ammunition word is followed by another initial -1
// with no known references; their storage boundary is not yet established.
extern i16 g_itemMenuAmmoType;

void SetItemMenuCharacter(i16 member);
void ItemMenuHandler(struct MenuBox* menu, i16 index, i16 event);
struct MenuBox* CreateItemMenu(struct MenuBox* old, ItemStackList* entries, i16 count);

i32 FormatItemMenuEntry(ItemStack entry, i32 numerator, i32 denominator);
void DrawItemMenuTotal(i16 plane, i32 total, i16 redraw, i16 line);

void AdjustItemMenuCount(struct MenuBox* menu, i16 row, i16 delta, i16 limit);

i32 ScaleItemPrice(i32 price, i32 numerator, i32 denominator, i16 count);
i32 GetItemMenuTotal(ItemStackList* list, i32 numerator, i32 denominator);
ItemStackList* CreateItemMenuEntries(i16* items, i16 count);

ItemStackList* CopyItemMenuEntries(ItemStack* entries, i16 count);

// Step 0 opens, 1 handles input, 2 closes; no other step is valid.
i16 StepItemBuyMenu(i16* step);
i16 StepItemSellMenu(i16* step);
b16 RunItemBuyMenu(void);
b16 RunItemSellMenu(void);

void RefreshScriptItemMenuTotal(void);
void OpenScriptItemMenu(i16 totalVar, i16 selling);

void PollScriptItemMenu(void);
void CloseItemMenu(void);

#endif // GITEN_GAME_ITEMMENU_H
