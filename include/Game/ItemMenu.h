#ifndef GITEN_GAME_ITEMMENU_H
#define GITEN_GAME_ITEMMENU_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/ItemStack.h>
#include <Ui/MenuBox.h>

struct MenuBox;

// The stock file stores byte offsets to item-ID lists ending with
// ITEM_STOCK_END.
#define ITEM_STOCK_END (-1)
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
void ItemMenuHandler(struct MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);
struct MenuBox* CreateItemMenu(struct MenuBox* old, ItemStackList* entries, i16 count);

i32 FormatItemMenuEntry(ItemStack entry, i32 numerator, i32 denominator);
void DrawItemMenuTotal(i16 plane, i32 total, b16 redraw, i16 line);

void AdjustItemMenuCount(struct MenuBox* menu, i16 row, i16 delta, i16 limit);

i32 ScaleItemPrice(i32 price, i32 numerator, i32 denominator, i16 count);
i32 GetItemMenuTotal(ItemStackList* list, i32 numerator, i32 denominator);
ItemStackList* CreateItemMenuEntries(i16* items, i16 count);

ItemStackList* CopyItemMenuEntries(ItemStack* entries, i16 count);

// An item menu shows ITEM_MENU_ROWS rows, then its total. Prices show at
// 1/priceDivisor of the item price (buying or selling). A shop menu leaves
// its total in script long variable ITEM_MENU_TOTAL_VAR; a script's menu
// (ITEM_MENU_MODE_SCRIPT) shows the total the script keeps in its own.
#define ITEM_MENU_ROWS 9
#define ITEM_PRICE_DIVISOR_BUY 1
#define ITEM_PRICE_DIVISOR_SELL 4
#define ITEM_MENU_MODE_SHOP 0
#define ITEM_MENU_MODE_SCRIPT 2
#define ITEM_MENU_TOTAL_VAR 0x11

// The shop menus' steps (no other step is valid), and the phases of their
// game states: enter, run the steps until they finish, and return.
GZ_ENUM_BEGIN_SPLIT(ItemMenuStep, i16)
    ITEM_MENU_STEP_OPEN = 0,
    ITEM_MENU_STEP_RUN = 1,
    ITEM_MENU_STEP_CLOSE = 2
GZ_ENUM_END_SPLIT(ItemMenuStep)

GZ_ENUM_BEGIN_SPLIT(ItemMenuPhase, i16)
    ITEM_MENU_PHASE_ENTER = 0,
    ITEM_MENU_PHASE_RUN = 1,
    ITEM_MENU_PHASE_RETURN = 2
GZ_ENUM_END_SPLIT(ItemMenuPhase)

i16 StepItemBuyMenu(i16* step);
i16 StepItemSellMenu(i16* step);
b16 RunItemBuyMenu(void);
b16 RunItemSellMenu(void);

void RefreshScriptItemMenuTotal(void);
void OpenScriptItemMenu(i16 totalVar, i16 selling);

void PollScriptItemMenu(void);
void CloseItemMenu(void);

#endif // GITEN_GAME_ITEMMENU_H
