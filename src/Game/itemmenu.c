// @identity-TODO: the owning TU is unproven; this unit holds the item-list
// menu family until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/BagItems.h>
#include <Game/EquipRequirements.h>
#include <Game/GameState.h>
#include <Game/ItemMenu.h>
#include <Game/ItemRecord.h>
#include <Game/ObjectRecord.h>
#include <Game/Party.h>
#include <Game/Scene.h>
#include <Game/StateStack.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Script/ScriptVars.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Text/WindowText.h>
#include <Ui/MenuBox.h>
#include <Ui/Panel.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>

DATA(0x00068c48)
static i16 s_itemMenuEquipGroup = -1;

DATA(0x00068c4c)
static i16 s_itemMenuMember = -1;

// The ammunition kind used by the open item menu.
DATA(0x00068c50)
i16 g_itemMenuAmmoType = -1;

DATA(0x0007d5c0)
static ItemStackList* s_itemMenuLimits;

DATA(0x0007d620)
static i32 s_itemMenuStock;

DATA(0x0007d624)
static MenuBox* s_itemMenu;

DATA(0x0007d628)
static i16 s_hideItemMenuTotal;

DATA(0x0007d63c)
static char s_emptyItemLine[1];

#define InitItemMenuContext(menu, divisor, modeValue, totalVariable)                               \
    do {                                                                                           \
        (menu)->context.item.priceDivisor = (divisor);                                             \
        (menu)->context.item.mode = (modeValue);                                                   \
        (menu)->context.item.totalVar = (totalVariable);                                           \
    } while (0)

RVA(0x0001b5c0, 0x93)
void SetItemMenuCharacter(i16 member) {
    i16 ammo;
    i16 group;
    if (member == -1) {
        if (s_itemMenuEquipGroup != -1 && s_itemMenu) {
            RequestMenuRedraw(s_itemMenu);
        }
        s_itemMenuEquipGroup = -1;
        s_itemMenuMember = -1;
        g_itemMenuAmmoType = -1;
        return;
    }
    ammo = GetGunAmmoType(GetCharacterById(member));
    s_itemMenuMember = member;
    group = ReadObjectRecordField(member, 0x20, 2);
    if (s_itemMenu && (s_itemMenuEquipGroup != group || g_itemMenuAmmoType != ammo)) {
        RequestMenuRedraw(s_itemMenu);
    }
    g_itemMenuAmmoType = ammo;
    s_itemMenuEquipGroup = group;
}

RVA(0x0001b660, 0xea)
i16 StepItemBuyMenu(i16* step) {
    switch (*step) {
        case 0: {
            i16 count;
            i16* items = AllocItemMenuStock(GetSceneCellKind(), &count);
            ItemStackList* list = CreateItemMenuEntries(items, count);
            FreeBlock(items);
            s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
            InitItemMenuContext(s_itemMenu, 1, 0, 0x11);
            (*step)++;
            return 0;
        }
        case 1: {
            i16 result = RunMenu(s_itemMenu);
            if (result == -1 || result == 0) {
                return 0;
            }
            if (result == 2) {
                result = -1;
            }
            AdjustItemMenuCount(s_itemMenu, g_hoveredObjectId, result, 99);
            return 0;
        }
        case 2:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return -1;
    }
}

RVA(0x0001b750, 0x6d)
MenuBox* CreateItemMenu(MenuBox* old, ItemStackList* entries, i16 count) {
    MenuBox* menu;
    SetItemMenuCharacter(-1);
    menu = CreateMenuBox(old, 0x19, 2);
    MoveMenuBox(menu, -8, -22);
    SetMenuItems(menu, 9, entries, count, ItemMenuHandler);
    SetTextPlaneCancelEnabled(menu->plane, 0);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, 1);
    menu->list->flags |= 2;
    return menu;
}

RVA(0x0001b7c0, 0x196)
void ItemMenuHandler(MenuBox* menu, i16 index, i16 event) {
    ItemStackList* list = menu->items.itemList;
    i16 i;
    switch (event) {
        case MENU_EVENT_DESTROY:
            if (menu->context.item.mode != 2) {
                ClearPool();
                for (i = 0; i < menu->itemCount; i++) {
                    if (GetItemStackCount(GetItemListEntry(list, i))) {
                        AddToPool(
                            GetItemStackItem(GetItemListEntry(list, i)),
                            GetItemStackCount(GetItemListEntry(list, i))
                        );
                    }
                }
                SetScriptLongVar(0x11, GetItemMenuTotal(list, 1, menu->context.item.priceDivisor));
            }
            s_itemMenuLimits = FreeBlock(s_itemMenuLimits);
            menu->items.itemList = FreeBlock(list);
            menu->itemCount = 0;
            break;
        case MENU_EVENT_ADD_ROW: {
            ItemStack* entry = GetItemListEntry(list, index);
            i32 color = 0x3450;
            i32 price = FormatItemMenuEntry(*entry, 1, menu->context.item.priceDivisor);
            ItemRecord* record = GetLoadedRecord(GetItemStackItem(entry));
            if (!GetItemRecordPrice(record)) {
                AddMenuLine(menu->plane, g_scratchBuffer, 0x3500, GetItemStackItem(entry), 1);
            } else {
                if (menu->context.item.mode == 0 && menu->context.item.priceDivisor == 1) {
                    if (CompareMacca(-1, price) < 0) {
                        color = 0x3500;
                    }
                }
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    color,
                    GetItemStackItem(entry),
                    menu->context.item.mode == 2
                );
            }
            break;
        }
        case MENU_EVENT_END_PAGE: {
            i32 total;
            if (menu->context.item.mode == 2) {
                total = GetScriptLongVar(menu->context.item.totalVar);
            } else {
                total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
            }
            DrawItemMenuTotal(menu->plane, total, 0, index - menu->cursor);
            break;
        }
    }
}

// @early-stop load width: retail extracts the by-value entry's item with
// a dword load and left shift followed by a word arithmetic right shift;
// this build uses word operations throughout. The item/attachment container
// remains a word, as the other packed-entry readers and writers require.
RVA(0x0001b960, 0x1d5)
i32 FormatItemMenuEntry(ItemStack entry, i32 numerator, i32 denominator) {
    i16 item = GetItemStackItem(&entry);
    char marker = ' ';
    ItemRecord* record = GetLoadedRecord(item);
    i16 equipGroup = GetItemEquipCode(record);
    i32 price;
    if (EquipPartOfItem(record) >= 0 && s_itemMenuEquipGroup != -1) {
        if (GetItemCategory(item) == EQUIP_PART_ACCESSORY) {
            Character* member = GetCharacterById(s_itemMenuMember);
            if (member && CanEquipItem(member, item) > 0) {
                marker = 'E';
            }
        } else if (CanGroupEquip(s_itemMenuEquipGroup, equipGroup)) {
            Character* member;
            marker = 'E';
            member = GetCharacterById(s_itemMenuMember);
            record = GetLoadedRecord(item);
            if (record->kind == ITEM_KIND_GUN && GetBattleStatShown(member, 6) > 0) {
                if (LacksItemRequiredStats(member, record, GetBattleStatShown(member, 6))) {
                    marker = 'e';
                }
            } else if (LacksItemRequiredStats(member, record, 0)) {
                marker = 'e';
            }
        }
    }
    record = GetLoadedRecord(item);
    price = ScaleItemPrice(GetItemRecordPrice(record), numerator, denominator, 1);
    if (GetItemRecordPrice(record)) {
        if (!GetItemStackCount(&entry)) {
            sprintf(
                g_scratchBuffer,
                "%c %-22.22s %6ld   ",
                marker,
                GetItemRecordName(record),
                price
            );
        } else {
            sprintf(
                g_scratchBuffer,
                "%c %-22.22s %6ldx%2d",
                marker,
                GetItemRecordName(record),
                price,
                GetItemStackCount(&entry)
            );
        }
    } else {
        if (!GetItemStackCount(&entry)) {
            sprintf(g_scratchBuffer, "%c %-22.22s          ", marker, GetItemRecordName(record));
        } else {
            sprintf(g_scratchBuffer, "%c %-22.22s          ", marker, GetItemRecordName(record));
        }
    }
    return price;
}

RVA(0x0001bb40, 0x17)
i32 ScaleItemPrice(i32 price, i32 numerator, i32 denominator, i16 count) {
    return numerator * price / denominator * count;
}

RVA(0x0001bb60, 0x5a)
i32 GetItemMenuTotal(ItemStackList* list, i32 numerator, i32 denominator) {
    i16 i;
    i32 total = 0;
    for (i = 0; i < GetItemListCount(list); i++) {
        ItemRecord* record = GetLoadedRecord(GetItemStackItem(GetItemListEntry(list, i)));
        total += ScaleItemPrice(
            GetItemRecordPrice(record),
            numerator,
            denominator,
            GetItemStackCount(GetItemListEntry(list, i))
        );
    }
    return total;
}

RVA(0x0001bbc0, 0xda)
void DrawItemMenuTotal(i16 plane, i32 total, i16 redraw, i16 line) {
    if (!redraw) {
        if (!s_hideItemMenuTotal) {
            sprintf(g_scratchBuffer, "                \215\207\214\166 %10ld   ", total);
        } else {
            sprintf(g_scratchBuffer, "                \215\207\214\166 ");
            s_hideItemMenuTotal = 0;
        }
        for (; line < 9; line++) {
            AddMenuLine(plane, s_emptyItemLine, 0x1400, -1, 1);
        }
        AddMenuLine(plane, g_scratchBuffer, 0x1400, -1, 1);
    } else {
        sprintf(g_scratchBuffer, "\215\207\214\166 %10ld   ", total);
        SetTextPlaneCursorLine(plane, 16, 9);
        PrintWindowText(plane, g_scratchBuffer, 0x1400, 1, 1);
    }
}

RVA(0x0001bca0, 0x10b)
void AdjustItemMenuCount(MenuBox* menu, i16 row, i16 delta, i16 limit) {
    ItemStackList* list = menu->items.itemList;
    i16 index = menu->cursor + row;
    i16 previous = GetItemStackCount(GetItemListEntry(list, index));
    i16 count;
    i16 attr;
    i32 total;
    if (GetItemKind(GetItemStackItem(GetItemListEntry(list, index))) == ITEM_KIND_AMMO) {
        delta *= 10;
    }
    count = AddClampShort(GetItemStackCount(GetItemListEntry(list, index)), delta, 0, limit);
    if (count != previous) {
        SetItemStackCount(GetItemListEntry(list, index), count);
        FormatItemMenuEntry(*GetItemListEntry(list, index), 1, menu->context.item.priceDivisor);
        attr = GetMenuLineAttr(menu->plane, row);
        ResetTextPlaneHighlight(menu->plane);
        SetTextPlaneCursorLine(menu->plane, 0, row);
        SetMenuLineText(menu->plane, row, g_scratchBuffer);
        PrintWindowText(menu->plane, g_scratchBuffer, attr, 1, 1);
        total = GetItemMenuTotal(list, 1, menu->context.item.priceDivisor);
        DrawItemMenuTotal(menu->plane, total, 1, 9);
    }
}

RVA(0x0001bdb0, 0x59)
ItemStackList* CreateItemMenuEntries(i16* items, i16 count) {
    ItemStackList* list =
        AllocCleared(1, offsetof(ItemStackList, entries) + count * sizeof(ItemStack));
    i16 i;
    list->count = count;
    for (i = 0; i < count; i++) {
        GetItemListEntry(list, i)->item = items[i];
        GetItemListEntry(list, i)->count = 0;
    }
    return list;
}

RVA(0x0001be10, 0x60)
i16 RunItemBuyMenu(void) {
    i16 step;
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            break;
        case 1:
            step = GetGameStep();
            result = StepItemBuyMenu(&step);
            SetGameStep(step);
            if (result == -1) {
                NextGamePhase();
            }
            break;
        case 2:
            ReturnFromGameState();
            break;
    }
    return 0;
}

RVA(0x0001be70, 0x110)
i16 StepItemSellMenu(i16* step) {
    ItemStackList* list;
    switch (*step) {
        case 0: {
            i16 i;
            s_itemMenuLimits = CopyBagEntries(0, 48, NULL);
            list = CopyBagEntries(0, 48, NULL);
            for (i = 0; i < GetItemListCount(list); i++) {
                GetItemListEntry(list, i)->count = 0;
            }
            s_itemMenu = CreateItemMenu(s_itemMenu, list, GetItemListCount(list));
            InitItemMenuContext(s_itemMenu, 4, 0, 0x11);
            (*step)++;
            return 0;
        }
        case 1: {
            i16 result;
            result = RunMenu(s_itemMenu);
            if (result == -1 || result == 0) {
                return 0;
            }
            if (result == 2) {
                result = -1;
            }
            AdjustItemMenuCount(
                s_itemMenu,
                g_hoveredObjectId,
                result,
                GetItemStackCount(
                    GetItemListEntry(s_itemMenuLimits, s_itemMenu->cursor + g_hoveredObjectId)
                )
            );
            return 0;
        }
        case 2:
            s_itemMenu = DestroyMenuBox(s_itemMenu);
            return -1;
    }
}

RVA(0x0001bf80, 0x54)
i16 RunItemSellMenu(void) {
    i16 step;
    i16 result;
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            break;
        case 1:
            step = GetGameStep();
            result = StepItemSellMenu(&step);
            SetGameStep(step);
            if (result == -1) {
                NextGamePhase();
            }
            break;
        case 2:
            ReturnFromGameState();
            break;
    }
    return 0;
}

RVA(0x0001bfe0, 0x2a)
void RefreshScriptItemMenuTotal(void) {
    DrawItemMenuTotal(s_itemMenu->plane, GetScriptLongVar(s_itemMenu->context.item.totalVar), 1, 9);
}

RVA(0x0001c010, 0x8b)
void OpenScriptItemMenu(i16 totalVar, i16 selling) {
    ItemStack* entries;
    i16 count;
    ItemStackList* list;
    SetItemMenuCharacter(-1);
    entries = GetPoolEntries();
    count = CountPoolEntries();
    list = CopyItemMenuEntries(entries, count);
    s_itemMenu = CreateItemMenu(s_itemMenu, list, count);
    InitItemMenuContext(s_itemMenu, selling ? 4 : 1, 2, totalVar);
    s_hideItemMenuTotal = 1;
    RunMenu(s_itemMenu);
    s_hideItemMenuTotal = 0;
    RefreshScriptItemMenuTotal();
}

RVA(0x0001c0a0, 0x3a)
ItemStackList* CopyItemMenuEntries(ItemStack* entries, i16 count) {
    ItemStackList* list =
        AllocCleared(1, offsetof(ItemStackList, entries) + count * sizeof(ItemStack));
    i16 i;
    list->count = count;
    for (i = 0; i < count; i++) {
        *GetItemListEntry(list, i) = entries[i];
    }
    return list;
}

RVA(0x0001c0e0, 0x19)
void PollScriptItemMenu(void) {
    if (s_itemMenu && s_itemMenu->context.item.mode == 2) {
        RunMenu(s_itemMenu);
    }
}

RVA(0x0001c100, 0x14)
void CloseItemMenu(void) {
    s_itemMenu = DestroyMenuBox(s_itemMenu);
}

RVA(0x0001c120, 0x34)
i16* GetItemMenuStock(i16 index) {
    ItemMenuStockTable* table = HandleReadPtr(s_itemMenuStock);
    if (index < 0 || index >= table->count) {
        index = 16;
    }
    return OffsetBy(table, table->offsets[index]);
}

RVA(0x0001c160, 0x33)
void LoadItemMenuStock(void) {
    if (s_itemMenuStock == 0) {
        FILE* fp = OpenDataFile(8, 12, 0);
        s_itemMenuStock = ReadRawHandle(fp);
        CloseDataFile(fp);
    }
}

RVA(0x0001c1a0, 0x2a)
i16 CountItemMenuStock(i16 index) {
    i16 count = 0;
    i16* items;
    LoadItemMenuStock();
    items = GetItemMenuStock(index);
    while (*items != -1) {
        items++;
        count++;
    }
    return count;
}

RVA(0x0001c1d0, 0x3d)
i16 CopyItemMenuStock(i16 index, i16* items) {
    i16 count = 0;
    i16* stock;
    LoadItemMenuStock();
    stock = GetItemMenuStock(index);
    while (*stock != -1) {
        items[count++] = *stock++;
    }
    return count;
}

RVA(0x0001c210, 0x32)
i16* AllocItemMenuStock(i16 index, i16* count) {
    i16* items;
    *count = CountItemMenuStock(index);
    items = AllocCleared(*count, sizeof(i16));
    CopyItemMenuStock(index, items);
    return items;
}
