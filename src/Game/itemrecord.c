// @identity-TODO: the owning TU is unproven; this unit holds the item-record
// decoder's span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <File/DataTableId.h>
#include <Game/Alignment.h>
#include <Game/AlignmentSide.h>
#include <Game/BagItems.h>
#include <Game/BattleEffect.h>
#include <Game/Clock.h>
#include <Game/Condition.h>
#include <Game/DemonTable.h>
#include <Game/DropTable.h>
#include <Game/DropTableEncoding.h>
#include <Game/EquipSlotIndex.h>
#include <Game/Field.h>
#include <Game/FieldSight.h>
#include <Game/GameState.h>
#include <Game/GemItems.h>
#include <Game/ItemBag.h>
#include <Game/ItemBonus.h>
#include <Game/ItemCurse.h>
#include <Game/ItemEffect.h>
#include <Game/ItemId.h>
#include <Game/ItemPool.h>
#include <Game/ItemRecord.h>
#include <Game/Pool.h>
#include <Game/SpecialItems.h>
#include <Game/StateStack.h>
#include <Game/StatUpdate.h>
#include <Input/Mouse.h>
#include <Input/MouseClickState.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Script/Script.h>
#include <Script/ScriptVars.h>
#include <Text/TextAttr.h>
#include <Text/TextPlane.h>
#include <Text/TextWindow.h>
#include <Ui/Menu.h>
#include <Ui/MenuBox.h>
#include <Util/BitChangeMode.h>
#include <Util/Range.h>
#include <Util/Scratch.h>

#include <stdio.h>
#include <string.h>

DATA(0x0007fe60)
ItemStack g_gemItems[16] = {0};

DATA(0x0007fea0)
ItemStack g_itemPool[ITEM_POOL_SIZE] = {0};

DATA(0x0007ffa0)
ItemStack g_bagItems[BAG_ENTRY_COUNT] = {0};

// The record of the item whose effect is being applied.
DATA(0x000800a0)
static ItemRecord s_usedItem = {0};

// The loaded item records, auxiliary index, and item remapping table.
DATA(0x000800e8)
static i32 s_itemDataHandle = 0;

DATA(0x000800ec)
static i32 s_itemIndexHandle = 0;

DATA(0x000800f0)
i32 g_itemRemapHandle = 0;

// Shared text buffers for the decoded item name and description.
DATA(0x000800f4)
char* g_itemNameText = NULL;

DATA(0x000800f8)
char* g_itemDescriptionText = NULL;

// Nonzero while bag stores are quiet (see SetBagQuiet).
DATA(0x000800fc)
static i16 s_bagQuiet = 0;

// The first id of the sixteen gem items (g_gemItems): ResetGemItems
// sets it, GemItemIndex maps a gem id back to its index by subtracting
// it; an item slot's 5-bit `attachment` hold such an index.
DATA(0x00080100)
static i16 s_gemItemBase = 0;

// The gem gift menu and the item id its rows start from.
DATA(0x00080104)
static i16 s_giftItemBase = 0;

DATA(0x00080108)
static MenuBox* s_giftMenu = NULL;

// The event flag each timed item clears when it expires.
DATA(0x00068f38)
static TimedItemFlag s_timedItemFlags[8] = {
    {0, 140},
    {0, 139},
    {0, 138},
    {0, 137},
    {0, 141},
    {0, 142},
    {1, 93},
    {-1, -1},
};

// The familiarity each gem item adds when given.
DATA(0x00064650)
static const i16 s_giftFamiliarity[16] = {2, 2, 3, 3, 4, 5, 6, 7, 8, 9, 10, 20, 30, 40, 50, 60};

DATA(0x000919a0)
DropSlot g_dropSlots[DROP_SLOT_COUNT];

DATA(0x000911c0)
ItemRecord g_loadedItem;

RVA(0x00022d00, 0xf)
static ItemTable* GetItemTableData(void) {
    return HandleReadPtr(s_itemDataHandle);
}

RVA(0x00022d10, 0x2b)
u8* GetItemRecordData(i16 id) {
    ItemTable* table = GetItemTableData();
    if (id >= table->count || id < 0) {
        id = 1;
    }
    return OffsetBy(table, table->offsets[id]);
}

#define ReadItemTargeting(record, src)                                                             \
    ((record)->params[4] = *(src)++, (record)->params[5] = *(src)++, (record)->params[6] = *(src)++)

static __inline u8* ReadItemRestoreParameters(ItemRecord* record, u8* src) {
    record->params[7] = *src++;
    record->params[8] = *src++;
    record->params[9] = *src++;
    return src;
}

static __inline u8* ReadItemAttackParameters(ItemRecord* record, u8* src) {
    record->params[0xc] = *src++;
    record->params[0xd] = *src++;
    record->params[0xe] = *src++;
    record->params[0xf] = *src++;
    return src;
}

// Decodes item `id`'s data-file entry into `record`: the price, the kind, the
// kind's parameter bytes (in the order the entry stores them), then the name
// and the description strings.
// @early-stop control flow: kind 1 joins kind 9 before the last parameter
// reads; retail joins at ReadItemMessages. Advancing before the mirrored
// byte reload duplicates the call tail; an explicit shared tail still merges
// the last parameter read instead of joining at the call.
RVA(0x00022d40, 0x490)
ItemRecord* DecodeItemRecord(ItemRecord* record, i16 id) {
    u8* src = GetItemRecordData(id);

    memset(record, 0, sizeof(ItemRecord));
    record->params[0x15] = 0xff;
    record->id = id;
    memcpy(&record->price, src, sizeof(record->price));
    src += sizeof(record->price);
    record->kind = *src++;
    switch (record->kind) {
        case ITEM_KIND_RESTORATIVE:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            src = ReadItemRestoreParameters(record, src);
            record->params[0x10] = *src;
            record->params[0xa] = *src++;
            record->params[0x32] = *src++;
            src = ReadItemMessages(record, src, 1, 1);
            break;
        case ITEM_KIND_INCENSE:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            record->params[0xb] = *src++;
            break;
        case ITEM_KIND_ENHANCER:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            src = ReadItemMessages(record, src, 1, 1);
            break;
        case ITEM_KIND_ATTACK:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            src = ReadItemAttackParameters(record, src);
            record->params[0xa] = *src;
            record->params[0x10] = *src++;
            record->params[0x11] = *src++;
            record->params[0x32] = *src++;
            src = ReadItemMessages(record, src, 1, 1);
            break;
        case 5:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            record->params[0x11] = *src++;
            src = ReadItemMessages(record, src, 1, 1);
            break;
        case 6:
            src = ReadItemValueRange(record, src);
            record->params[4] = *src++;
            record->params[5] = *src++;
            record->params[0x12] = *src++;
            record->params[0x13] = *src++;
            break;
        case ITEM_KIND_SOFTWARE:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            record->params[0x14] = *src++;
            break;
        case ITEM_KIND_GEM:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            record->params[0xb] = *src++;
            src = ReadItemAttackParameters(record, src);
            record->params[0xa] = *src;
            record->params[0x10] = *src++;
            record->params[0x31] = *src++;
            record->params[0x31] = *src++;
            src = ReadItemRestoreParameters(record, src);
            record->params[0xa] = *src++;
            record->params[0x32] = *src++;
            src = ReadItemMessages(record, src, 1, 1);
            break;
        case ITEM_KIND_KEYCARD:
        case ITEM_KIND_SCENARIO:
            src = ReadItemValueRange(record, src);
            ReadItemTargeting(record, src);
            break;
        case ITEM_KIND_WEAPON:
            src = ReadItemEquipEffect(record, src);
            record->params[0x19] = *src++;
            record->params[0x1a] = *src++;
            record->params[0x1b] = *src++;
            record->params[0x1c] = *src++;
            record->params[0x21] = *src++;
            record->params[6] = *src++;
            record->params[0x1d] = *src++;
            record->params[0x1e] = *src++;
            record->params[0x1f] = *src++;
            record->params[0x20] = *src++;
            src = ReadItemExtraPair(record, src);
            record->params[0x24] = *src++;
            record->params[0x25] = *src++;
            record->params[0x26] = *src++;
            src = ReadItemValueRangeAlt(record, src);
            record->params[0x2d] = *src++;
            record->params[0x2e] = *src++;
            src = ReadItemMessages(record, src, 1, 0);
            break;
        case ITEM_KIND_GUN:
            src = ReadItemEquipEffect(record, src);
            record->params[0x19] = *src++;
            record->params[0x1b] = *src++;
            record->params[0x1c] = *src++;
            record->params[0x1f] = *src++;
            record->params[0x20] = *src++;
            src = ReadItemExtraPair(record, src);
            record->params[0x27] = *src++;
            record->params[0x28] = *src++;
            record->params[0x29] = *src++;
            record->params[0x2a] = *src++;
            record->params[0x2b] = *src++;
            record->params[6] = *src++;
            record->params[0x32] = *src++;
            break;
        case ITEM_KIND_AMMO:
            src = ReadItemEquipEffect(record, src);
            record->params[0x19] = *src++;
            record->params[0x21] = *src++;
            record->params[0x24] = *src++;
            record->params[0x27] = *src++;
            record->params[0x32] = *src++;
            break;
        case ITEM_KIND_FULL_BODY_ARMOR:
        case ITEM_KIND_HEAD_ARMOR:
        case ITEM_KIND_BODY_ARMOR:
        case ITEM_KIND_ARM_ARMOR:
        case ITEM_KIND_LEG_ARMOR:
            src = ReadItemEquipEffect(record, src);
            record->params[0x1a] = *src++;
            record->params[0x2c] = *src++;
            record->params[0x2f] = *src++;
            record->params[0x30] = *src++;
            record->params[0x21] = *src++;
            record->params[0x1f] = *src++;
            src = ReadItemExtraPair(record, src);
            record->params[0x25] = *src++;
            break;
        case ITEM_KIND_ACCESSORY:
            src = ReadItemEquipEffect(record, src);
            record->params[0x25] = *src++;
            src = ReadItemValueRangeAlt(record, src);
            src = ReadItemMessages(record, src, 1, 0);
            record->params[0xb] = 11;
            break;
    }
    strcpy(g_itemNameText, (char*)src);
    record->name = g_itemNameText;
    while (*src != 0) {
        src++;
    }
    strcpy(g_itemDescriptionText, (char*)(src + 1));
    record->description = g_itemDescriptionText;
    return record;
}

RVA(0x000231d0, 0x21)
u8* ReadItemValueRange(ItemRecord* item, u8* src) {
    item->params[0] = *src++;
    item->params[1] = *src++;
    item->params[2] = *src++;
    item->params[3] = *src++;
    return src;
}

RVA(0x00023200, 0x31)
u8* ReadItemMessages(ItemRecord* item, u8* src, i16 first, i16 second) {
    if (first) {
        item->beforeMessage.script = *src++;
        item->beforeMessage.entry = *src++;
    }
    if (second) {
        item->afterMessage.script = *src++;
        item->afterMessage.entry = *src++;
    }
    return src;
}

RVA(0x00023240, 0x21)
u8* ReadItemEquipEffect(ItemRecord* item, u8* src) {
    item->params[0x15] = *src++;
    item->params[0x16] = *src++;
    item->params[0x17] = *src++;
    item->params[0x18] = *src++;
    return src;
}

RVA(0x00023270, 0x15)
u8* ReadItemExtraPair(ItemRecord* item, u8* src) {
    item->params[0x22] = *src++;
    item->params[0x23] = *src++;
    return src;
}

RVA(0x00023290, 0x21)
u8* ReadItemValueRangeAlt(ItemRecord* item, u8* src) {
    item->params[0] = *src++;
    item->params[1] = *src++;
    item->params[0x13] = *src++;
    item->params[3] = *src++;
    return src;
}

RVA(0x000232c0, 0xb9)
void LoadItemFiles(void) {
    FILE* fp;
    i16 count;
    i16 id;

    fp = OpenDataFile(DATA_TABLE_ITEM_RECORDS, DATA_FILE_ITEM_RECORDS, 0);
    s_itemDataHandle = ReadCryptHandle(fp);
    CloseDataFile(fp);

    fp = OpenDataFile(DATA_TABLE_ITEM_INDEX, DATA_FILE_TABLE, 0);
    s_itemIndexHandle = ReadRawHandle(fp);
    g_itemRemapHandle = ReadRawHandle(fp);
    CloseDataFile(fp);

    g_itemNameText = AllocCleared(0x100, 1);
    g_itemDescriptionText = AllocCleared(0x100, 1);

    count = GetItemTableData()->count;
    for (id = 0; id < count; id++) {
        if (GetItemKind(id) == ITEM_KIND_GEM) {
            break;
        }
    }
    ResetGemItems(id);
}

RVA(0x00023380, 0x18)
i16 GetItemRewardAt(i16 index) {
    i16* rewards = HandleReadPtr(s_itemIndexHandle);
    return rewards[index];
}

RVA(0x000233a0, 0x18)
char* GetLoadedRecordName(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return g_loadedItem.name;
}

RVA(0x000233c0, 0x1b)
GZ_ENUM_RETURN(ItemKind, i16) GetItemKind(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return g_loadedItem.kind;
}

RVA(0x000233e0, 0x18)
char* GetItemDescription(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return g_loadedItem.description;
}

RVA(0x00023400, 0x18)
i32 GetItemPrice(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return GetItemRecordPrice(&g_loadedItem);
}

RVA(0x00023420, 0x1b)
i16 GetItemValueHigh(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return g_loadedItem.params[1];
}

RVA(0x00023440, 0x1b)
i16 GetItemValueLow(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    return g_loadedItem.params[0];
}

RVA(0x00023460, 0x13)
ItemRecord* GetLoadedRecord(i16 id) {
    return DecodeItemRecord(&g_loadedItem, id);
}

RVA(0x00023480, 0x70)
u16 GetItemStackLimit(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    switch (g_loadedItem.kind) {
        case ITEM_KIND_RESTORATIVE:
        case ITEM_KIND_INCENSE:
        case ITEM_KIND_ENHANCER:
        case ITEM_KIND_ATTACK:
        case 5:
        case 6:
        case ITEM_KIND_SOFTWARE:
        case ITEM_KIND_KEYCARD:
            return 99;
        case ITEM_KIND_GEM:
            return 99;
        case ITEM_KIND_SCENARIO:
            return 99;
        case ITEM_KIND_WEAPON:
        case ITEM_KIND_GUN:
            return 1;
        case ITEM_KIND_AMMO:
            return 99;
        case ITEM_KIND_FULL_BODY_ARMOR:
        case ITEM_KIND_HEAD_ARMOR:
        case ITEM_KIND_BODY_ARMOR:
        case ITEM_KIND_ARM_ARMOR:
        case ITEM_KIND_LEG_ARMOR:
            return 1;
        case ITEM_KIND_ACCESSORY:
            return 99;
    }
    return 1;
}

RVA(0x000234f0, 0x50)
i16 RemapItem(u16 item) {
    u16* pairs = HandleReadPtr(g_itemRemapHandle);
    i16 i = 0;

    while (pairs[i] != 0xffff) {
        if (pairs[i] == item) {
            return pairs[i + 1];
        }
        i += 2;
    }
    return 0;
}

RVA(0x00023540, 0x60)
i16 RollItemAmount(i16 item, i16 count, i16 random) {
    i16 high = GetItemValueHigh(item);
    i16 low = GetItemValueLow(item);
    i16 total = 0;
    i16 i;

    for (i = 0; i < count; i++) {
        if (random) {
            total += RandomAverage(low, high, 0);
        } else {
            total += high;
        }
    }
    return total;
}

RVA(0x000235a0, 0x50)
b16 IsEquipCurseActive(Character* character, i16 part) {
    ItemSlot slot = GetEquipSlot(character, part);
    ItemRecord* record;

    if (slot.item < 1) {
        return false;
    }
    record = GetLoadedRecord(slot.item);
    if (GetItemCurse(record) == ITEM_CURSE_NONE) {
        return false;
    }
    return GetItemCurseLevel(record) > character->level;
}

RVA(0x000235f0, 0xa0)
GZ_ENUM_RETURN(EquipPart, i16) GetItemCategory(i16 id) {
    DecodeItemRecord(&g_loadedItem, id);
    switch (g_loadedItem.kind) {
        case ITEM_KIND_RESTORATIVE:
        case ITEM_KIND_INCENSE:
        case ITEM_KIND_ENHANCER:
        case ITEM_KIND_ATTACK:
        case 5:
        case 6:
        case ITEM_KIND_SOFTWARE:
        case ITEM_KIND_KEYCARD:
        case ITEM_KIND_GEM:
        case ITEM_KIND_SCENARIO:
            return EQUIP_PART_NONE;
        case ITEM_KIND_WEAPON:
            return EQUIP_PART_WEAPON;
        case ITEM_KIND_GUN:
            return EQUIP_PART_GUN;
        case ITEM_KIND_AMMO:
            return EQUIP_PART_AMMO;
        case ITEM_KIND_FULL_BODY_ARMOR:
            return EQUIP_PART_BODY;
        case ITEM_KIND_HEAD_ARMOR:
            return EQUIP_PART_HEAD;
        case ITEM_KIND_BODY_ARMOR:
            return EQUIP_PART_BODY;
        case ITEM_KIND_ARM_ARMOR:
            return EQUIP_PART_ARMS;
        case ITEM_KIND_LEG_ARMOR:
            return EQUIP_PART_LEGS;
        case ITEM_KIND_ACCESSORY:
            return EQUIP_PART_ACCESSORY;
    }
    return EQUIP_PART_NONE;
}

RVA(0x00023690, 0x20)
i16 SetBagQuiet(i16 quiet) {
    i16 previous = s_bagQuiet;

    s_bagQuiet = quiet;
    return previous;
}

RVA(0x000236b0, 0x30)
i16 CountHeldItem(i16 id) {
    if (GemItemIndex(id) != -1) {
        return CountGemItems(id);
    }
    return CountBagItem(id);
}

RVA(0x000236e0, 0xa0)
i16 StoreBagItem(i16 item, i16 count, i16 attachment) {
    i16 stored;
    i16 left;

    if (item < 1) {
        return count;
    }
    if (GemItemIndex(item) != -1) {
        stored = count;
        AddGemItems(item, count);
    } else {
        GetLoadedRecord(item);
        stored = 0;
        while (count != 0) {
            left = AddBagItems(item, count, attachment, 0);
            stored += count - left;
            count = left;
            if (s_bagQuiet == 0) {
                if (count > 0) {
#ifdef GITEN_BUGFIX
                    if (!RunBagDiscardMenu()) {
                        break;
                    }
#else
                    RunBagDiscardMenu();
#endif
                }
            } else {
                count = 0;
            }
        }
    }
    if (s_bagQuiet == 0) {
        StampSpecialItem(item);
    }
    return stored;
}

RVA(0x00023780, 0x40)
i16 TakeBagItems(i16 item, i16 count) {
    if (GemItemIndex(item) != -1) {
        TakeGemItems(item, count);
        return count;
    }
    return count - TakeBagItemsFromEnd(item, count);
}

RVA(0x000237c0, 0x10)
ItemStack* GetPoolEntries(void) {
    return g_itemPool;
}

RVA(0x000237d0, 0x30)
i16 CountPoolEntries(void) {
    i16 count = 0;
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) != ITEM_ID_EMPTY) {
            count++;
        }
    }
    return count;
}

RVA(0x00023800, 0x20)
void ClearPool(void) {
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        GetItemPoolEntry(i)->item = ITEM_ID_EMPTY;
        GetItemPoolEntry(i)->count = 0;
    }
}

RVA(0x00023820, 0x100)
void AddToPool(i16 item, i16 amount) {
    i16 room;
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) == item) {
            if (GetItemStackCount(GetItemPoolEntry(i)) + amount <= 99) {
                GetItemPoolEntry(i)->count += amount;
                return;
            }
            room = 99 - GetItemStackCount(GetItemPoolEntry(i));
            GetItemPoolEntry(i)->count += room;
            amount -= room;
        }
    }
    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) == ITEM_ID_EMPTY) {
            GetItemPoolEntry(i)->item = item;
            GetItemPoolEntry(i)->count = amount;
            return;
        }
    }
}

RVA(0x00023920, 0x70)
void TakeFromPool(i16 item, u8 amount) {
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) == item) {
            GetItemPoolEntry(i)->count -= amount;
            if (GetItemStackCount(GetItemPoolEntry(i)) == 0) {
                GetItemPoolEntry(i)->count = 0;
                GetItemPoolEntry(i)->item = ITEM_ID_EMPTY;
            }
            return;
        }
    }
}

RVA(0x00023990, 0xa0)
i16 GivePooledItems(void) {
    i16 left = 0;
    i16 item;
    i16 amount;
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) != ITEM_ID_EMPTY) {
            item = RemapItem(GetItemStackItem(GetItemPoolEntry(i)));
            amount = GetItemStackCount(GetItemPoolEntry(i));
            if (item != 0) {
                amount = RollItemAmount(
                    GetItemStackItem(GetItemPoolEntry(i)),
                    GetItemStackCount(GetItemPoolEntry(i)),
                    0
                );
            } else {
                item = GetItemStackItem(GetItemPoolEntry(i));
            }
            amount -= StoreBagItem(item, amount, -1);
            if (amount < 0) {
                amount = 0;
            }
            left += amount;
        }
    }
    return left;
}

RVA(0x00023a30, 0x50)
void TakePooledItems(void) {
    i16 i;

    for (i = 0; i < ITEM_POOL_SIZE; i++) {
        if (GetItemStackItem(GetItemPoolEntry(i)) != ITEM_ID_EMPTY) {
            TakeBagItems(
                GetItemStackItem(GetItemPoolEntry(i)),
                GetItemStackCount(GetItemPoolEntry(i))
            );
        }
    }
}

RVA(0x00023a80, 0x20)
void ClearBag(void) {
    i16 i;

    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        ClearItemStack(&g_bagItems[i]);
    }
}

RVA(0x00023aa0, 0x70)
ItemStackList* CopyBagEntries(i16 first, i16 count, ItemStackList* list) {
    i16 copied = 0;
    i16 i;

    if (list == NULL) {
        list = AllocCleared(1, count * sizeof(ItemStack) + sizeof(list->count));
    }
    count += first;
    if (count > 64) {
        count = 64;
    }
    for (i = first; i < count; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != ITEM_ID_EMPTY) {
            *GetItemListEntry(list, copied++) = g_bagItems[i];
        }
    }
    list->count = copied;
    return list;
}

RVA(0x00023b10, 0x30)
i16 RestoreBagEntries(ItemStackList* list) {
    i16 i;

    if (list == NULL) {
        return 0;
    }
    for (i = 0; i < GetItemListCount(list); i++) {
        g_bagItems[i] = *GetItemListEntry(list, i);
    }
    return GetItemListCount(list);
}

RVA(0x00023b40, 0xb1)
b32 PooledItemsFit(void) {
    i16 previousQuiet = SetBagQuiet(1);
    ItemStackList* savedBag = CopyBagEntries(0, 64, NULL);
    ItemStack* savedGems = SaveGemItems(NULL);
    i16 left = GivePooledItems();
    i16 i;

    ClearBag();
    ResetGemItems(GetGemItemBase());
    RestoreBagEntries(savedBag);
    for (i = 0; i < 16; i++) {
        AddGemItems(GetItemStackItem(&savedGems[i]), GetItemStackCount(&savedGems[i]));
    }
    FreeBlock(savedBag);
    FreeBlock(savedGems);
    SetBagQuiet(previousQuiet);
    return left <= 0;
}

RVA(0x00023c00, 0x20)
void ClearDropSlots(void) {
    i16 i;

    for (i = 0; i < DROP_SLOT_COUNT; i++) {
        ClearDropSlot(i);
    }
}

RVA(0x00023c20, 0x80)
i16 AddDropSlot(i16 item, i16 amount) {
    i16 remap;
    i16 i;

    if (item < 1) {
        return -1;
    }
    remap = RemapItem(item);
    if (remap != 0) {
        amount = RollDropAmount(item, amount);
        item = remap;
    }
    for (i = 0; i < DROP_SLOT_COUNT; i++) {
        if (GetDropSlot(i)->item < 1 || GetDropSlot(i)->item == item) {
            GetDropSlot(i)->item = item;
            GetDropSlot(i)->amount += amount;
            return i;
        }
    }
    return -1;
}

RVA(0x00023ca0, 0x70)
i16 FindBagItem(i16 item, u8 groups) {
    i16 i;

    if (groups & 1) {
        for (i = 47; i >= 0; i--) {
            if (GetItemStackItem(&g_bagItems[i]) == item) {
                return i;
            }
        }
    }
    if (groups & 2) {
        for (i = 63; i >= 48; i--) {
            if (GetItemStackItem(&g_bagItems[i]) == item) {
                return i;
            }
        }
    }
    return -1;
}

RVA(0x00023d10, 0x40)
i16 CountBagItem(i16 item) {
    i16 total = 0;
    i16 i;

    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) == item) {
            total += GetItemStackCount(&g_bagItems[i]);
        }
    }
    return total;
}

RVA(0x00023d50, 0x70)
void SetBagEntry(i16 index, i16 item, i16 attachment) {
    g_bagItems[index].item = item;
    if (attachment != -1) {
        g_bagItems[index].attachment = attachment;
        g_bagItems[index].hasAttachment = true;
    } else {
        g_bagItems[index].attachment = 0;
        g_bagItems[index].hasAttachment = false;
    }
    g_bagItems[index].count = 0;
    g_bagItems[index].detail = 0;
}

// @early-stop instruction selection: retail extracts the count as a word
// (and ax,0xff) and masks the amount to 8 bits before the store; cl reads the
// count byte. The permuter found one compiler island.
RVA(0x00023dc0, 0x50)
i16 AddToBagEntry(i16 index, u16 amount, u16 limit) {
    u16 total = GetItemStackCount(&g_bagItems[index]) + amount;

    if (total > limit) {
        amount = limit - GetItemStackCount(&g_bagItems[index]);
    }
    g_bagItems[index].count += amount;
    return amount;
}

RVA(0x00023e10, 0x60)
i16 TakeFromBagEntry(i16 index, i16 amount) {
    if (amount > GetItemStackCount(&g_bagItems[index])) {
        amount = GetItemStackCount(&g_bagItems[index]);
    }
    amount &= 0xff;
    g_bagItems[index].count -= amount;
    if (GetItemStackCount(&g_bagItems[index]) == 0) {
        SetBagEntry(index, -1, -1);
    }
    return amount;
}

RVA(0x00023e70, 0x40)
i16 SetBagEntryDetail(i16 index, i16 detail) {
    i16 previous = g_bagItems[index].detail;

    g_bagItems[index].detail = detail;
    return previous;
}

RVA(0x00023eb0, 0x70)
i16 FillBagEntry(i16 index, i16 item, u16 amount, u16 limit, i16 attachment, i16 detail) {
    if (GetItemStackItem(&g_bagItems[index]) == ITEM_ID_EMPTY) {
        SetBagEntry(index, item, attachment);
    } else if (GetItemStackItem(&g_bagItems[index]) != item) {
        return 0;
    }
    SetBagEntryDetail(index, detail);
    return AddToBagEntry(index, amount, limit);
}

// @early-stop: retail anchors the scan pointer at the count word; this
// build keeps it at the item word.
RVA(0x00023f20, 0x1d0)
static void CompactBagCore(void) {
#ifdef GITEN_BUGFIX
    // @bug Retail keeps the first 16 scenario entries it finds and clears the
    // rest, which AddScenarioBagItems's overflow into the normal entries would
    // lose on the next compaction. Every scenario entry is kept: the scenario
    // range takes the first 16 again and the rest go to the free normal
    // entries, which the other entries' compaction leaves at its end.
    ItemStack scenarioItems[64];
#else
    ItemStack scenarioItems[16];
#endif
    i16 kept = 0;
    i16 limit;
    i16 from;
    i16 i;
    i16 j;

#ifdef GITEN_BUGFIX
    for (i = 0; i < 64; i++) {
#else
    for (i = 0; i < 16; i++) {
#endif
        ClearItemStack(&scenarioItems[i]);
    }
    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != ITEM_ID_EMPTY
            && GetItemKind(GetItemStackItem(&g_bagItems[i])) == ITEM_KIND_SCENARIO) {
#ifdef GITEN_BUGFIX
            scenarioItems[kept++] = g_bagItems[i];
#else
            if (kept < 16) {
                scenarioItems[kept++] = g_bagItems[i];
            }
#endif
            ClearItemStack(&g_bagItems[i]);
        }
    }
    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) == ITEM_ID_EMPTY) {
            continue;
        }
        limit = GetItemStackLimit(GetItemStackItem(&g_bagItems[i]));
        while (GetItemStackCount(&g_bagItems[i]) < limit) {
            from = FindBagItem(GetItemStackItem(&g_bagItems[i]), 3);
            if (from <= i) {
                break;
            }
            TakeFromBagEntry(from, AddToBagEntry(i, GetItemStackCount(&g_bagItems[from]), limit));
        }
    }
    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != ITEM_ID_EMPTY) {
            continue;
        }
        for (j = i + 1; j < BAG_ENTRY_COUNT; j++) {
            if (GetItemStackItem(&g_bagItems[j]) != ITEM_ID_EMPTY) {
                g_bagItems[i] = g_bagItems[j];
                ClearItemStack(&g_bagItems[j]);
                break;
            }
        }
    }
    for (i = BAG_ORDINARY_ENTRY_COUNT; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != ITEM_ID_EMPTY) {
            continue;
        }
#ifdef GITEN_BUGFIX
        for (j = 0; j < kept; j++) {
#else
        for (j = 0; j < 16; j++) {
#endif
            if (GetItemStackItem(&scenarioItems[j]) != ITEM_ID_EMPTY) {
                g_bagItems[i] = scenarioItems[j];
                ClearItemStack(&scenarioItems[j]);
                break;
            }
        }
    }
#ifdef GITEN_BUGFIX
    for (i = 0; i < 48; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != -1) {
            continue;
        }
        for (j = 0; j < kept; j++) {
            if (GetItemStackItem(&scenarioItems[j]) != -1) {
                g_bagItems[i] = scenarioItems[j];
                ClearItemStack(&scenarioItems[j]);
                break;
            }
        }
    }
#endif
}

RVA(0x000240f0, 0x50)
i16 TakeBagItemsFromEnd(i16 item, i16 amount) {
    i16 i;

    for (i = 63; i >= 0; i--) {
        if (GetItemStackItem(&g_bagItems[i]) == item) {
            amount -= TakeFromBagEntry(i, amount);
            if (amount <= 0) {
                break;
            }
        }
    }
    return amount;
}

RVA(0x00024140, 0x20)
i16 GetBagItem(i16 index) {
    return GetItemStackItem(&g_bagItems[index]);
}

RVA(0x00024160, 0x10)
ItemStack* GetBagEntry(i16 index) {
    return &g_bagItems[index];
}

RVA(0x00024170, 0x20)
i16 GetBagEntryCount(i16 index) {
    return GetItemStackCount(&g_bagItems[index]);
}

RVA(0x00024190, 0x80)
i16 AddBagItems(i16 item, i16 count, i16 attachment, i16 detail) {
    u16 limit;
    i16 i;

    CompactBagCore();
    if (item < 1) {
        return 0;
    }
    if (GetItemKind(item) == ITEM_KIND_SCENARIO) {
        return AddScenarioBagItems(item, count);
    }
    limit = GetItemStackLimit(item);
    for (i = 0; i < BAG_ORDINARY_ENTRY_COUNT; i++) {
        count -= FillBagEntry(i, item, count, limit, attachment, detail);
        if (count <= 0) {
            break;
        }
    }
    return count;
}

RVA(0x00024210, 0x70)
i16 AddScenarioBagItems(i16 item, i16 count) {
    u16 limit = GetItemStackLimit(item);
    i16 i;

    for (i = BAG_ORDINARY_ENTRY_COUNT; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) == ITEM_ID_EMPTY) {
            SetBagEntry(i, item, -1);
            count -= AddToBagEntry(i, count, limit);
            if (count <= 0) {
                break;
            }
        }
    }
#ifdef GITEN_BUGFIX
    // @bug A scenario item goes only into the bag's last 16 entries, and only
    // into empty ones (a second copy takes another entry). With all 16 held
    // StoreBagItem opens RunBagDiscardMenu until the item fits, but that menu
    // lists no scenario entry, so no discard makes room: it opens again after
    // every discard until nothing is left to list, and that last menu never
    // closes. The rest goes to empty normal entries instead; the scenario
    // item's readers find it by item, in any entry.
    for (i = 0; i < 48 && count > 0; i++) {
        if (GetItemStackItem(&g_bagItems[i]) == -1) {
            SetBagEntry(i, item, -1);
            count -= AddToBagEntry(i, count, limit);
        }
    }
#endif
    return count;
}

RVA(0x00024280, 0x30)
i16 CountBagEntries(void) {
    i16 count = 0;
    i16 i;

    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        if (GetItemStackItem(&g_bagItems[i]) != ITEM_ID_EMPTY) {
            count++;
        }
    }
    return count;
}

RVA(0x000242b0, 0x60)
ItemStack* SaveOrRestoreBag(ItemStack* buffer, i16 restore) {
    i16 i;

    if (!restore) {
        if (buffer == NULL) {
            buffer = AllocCleared(64, sizeof(ItemStack));
        }
        for (i = 0; i < BAG_ENTRY_COUNT; i++) {
            buffer[i] = g_bagItems[i];
        }
    } else if (buffer != NULL) {
        for (i = 0; i < BAG_ENTRY_COUNT; i++) {
            g_bagItems[i] = buffer[i];
        }
    }
    return buffer;
}

RVA(0x00024310, 0x79)
void CompactBag(void) {
    i16 i;
    i16 next;

    CompactBagCore();
    for (i = 0; i < 47; i++) {
        if (GetItemStackItem(&g_bagItems[i]) == ITEM_ID_EMPTY) {
            next = i + 1;
            if (next >= 48) {
                return;
            }
            while (next < BAG_ORDINARY_ENTRY_COUNT) {
                if (GetItemStackItem(&g_bagItems[next]) != ITEM_ID_EMPTY) {
                    g_bagItems[i] = g_bagItems[next];
                    SetBagEntry(next, -1, -1);
                    break;
                }
                next++;
            }
            if (next >= 48) {
                return;
            }
        }
    }
}

RVA(0x00024390, 0x40)
i16 TakeBagItemsAt(i16 index, i16 item, i16 amount) {
    if (GetItemStackItem(&g_bagItems[index]) == item) {
        amount -= TakeFromBagEntry(index, amount);
    }
    return TakeBagItemsFromEnd(item, amount);
}

RVA(0x000243d0, 0x20)
i16 GetBagEntryDetail(i16 index) {
    return g_bagItems[index].detail;
}

RVA(0x000243f0, 0x80)
i16 ReadBagEntry(i16 index, ItemSlot* slot, i16* count) {
    slot->item = GetBagItem(index);
    if (!HasItemStackAttachment(&g_bagItems[index])) {
        slot->attachment = -1;
    } else {
        slot->attachment = g_bagItems[index].attachment;
    }
    slot->quantity = 1;
    *count = GetBagEntryDetail(index);
    return HasItemStackAttachment(&g_bagItems[index]);
}

RVA(0x00024470, 0x30)
i16 GetBagEntryAttachment(i16 index) {
    if (!HasItemStackAttachment(&g_bagItems[index])) {
        return -1;
    }
    return GetGemItemBase() + g_bagItems[index].attachment;
}

RVA(0x000244a0, 0x30)
i16 DetachBagEntryItem(i16 index) {
    i16 id = GetBagEntryAttachment(index);

    if (id >= 0) {
        g_bagItems[index].attachment = 0;
        g_bagItems[index].hasAttachment = false;
    }
    return id;
}

RVA(0x000244d0, 0x50)
i16 AttachBagEntryItem(i16 index, i16 id) {
    i16 previous = DetachBagEntryItem(index);

    g_bagItems[index].attachment = id - GetGemItemBase();
    g_bagItems[index].hasAttachment = true;
    return previous;
}

RVA(0x00024520, 0x30)
i16 WriteBag(FILE* fp) {
    return 64 - fwrite(g_bagItems, sizeof(ItemStack), 64, fp);
}

RVA(0x00024550, 0x30)
i16 ReadBag(FILE* fp) {
    return 64 - fread(g_bagItems, sizeof(ItemStack), 64, fp);
}

RVA(0x00024580, 0x30)
i16 GemItemIndex(i16 id) {
    if (GetItemKind(id) == ITEM_KIND_GEM) {
        return id - s_gemItemBase;
    }
    return -1;
}

RVA(0x000245b0, 0xb0)
i16 AddGemItemsAt(i16 index, u16 amount) {
    u16 total;

    if (index < 0 || index >= 16) {
        return -1;
    }
    total = GetGemItemEntry(index)->count + amount;
    if (total > GetItemStackLimit(GetItemStackItem(GetGemItemEntry(index)))) {
        amount = GetItemStackLimit(GetItemStackItem(GetGemItemEntry(index)))
                 - GetGemItemEntry(index)->count;
    }
    GetGemItemEntry(index)->count += amount;
    return amount;
}

RVA(0x00024660, 0x50)
i16 TakeGemItemsAt(i16 index, i16 amount) {
    if (index < 0 || index >= 16) {
        return -1;
    }
    if (amount > GetItemStackCount(GetGemItemEntry(index))) {
        amount = GetItemStackCount(GetGemItemEntry(index));
    }
    GetGemItemEntry(index)->count -= amount;
    return amount;
}

RVA(0x000246b0, 0x40)
void ResetGemItems(i16 base) {
    i16 i;

    s_gemItemBase = base;
    for (i = 0; i < 16; i++) {
        GetGemItemEntry(i)->item = i + base;
        GetGemItemEntry(i)->count = 0;
    }
}

RVA(0x000246f0, 0x30)
i16 AddGemItems(i16 id, u16 amount) {
    i16 index = id - s_gemItemBase;
    if (index >= 0 && index < 16) {
        return AddGemItemsAt(index, amount);
    }
    return -1;
}

RVA(0x00024720, 0x30)
i16 TakeGemItems(i16 id, i16 amount) {
    i16 index = id - s_gemItemBase;
    if (index >= 0 && index < 16) {
        return TakeGemItemsAt(index, amount);
    }
    return -1;
}

RVA(0x00024750, 0x30)
i16 CountGemItemsAt(i16 index) {
    if (index >= 0 && index < 16) {
        return GetItemStackCount(GetGemItemEntry(index));
    }
    return -1;
}

RVA(0x00024780, 0x30)
i16 CountGemItems(i16 id) {
    i16 index = id - s_gemItemBase;
    if (index >= 0 && index < 16) {
        return CountGemItemsAt(index);
    }
    return -1;
}

RVA(0x000247b0, 0x10)
i16 GetGemItemBase(void) {
    return s_gemItemBase;
}

RVA(0x000247c0, 0x40)
ItemStack* SaveGemItems(ItemStack* buffer) {
    i16 i;

    if (buffer == NULL) {
        buffer = AllocCleared(1, sizeof(g_gemItems));
    }
    for (i = 0; i < 16; i++) {
        buffer[i] = *GetGemItemEntry(i);
    }
    return buffer;
}

RVA(0x00024800, 0x30)
ItemStack* RestoreGemItems(ItemStack* buffer) {
    i16 i;

    if (buffer != NULL) {
        for (i = 0; i < 16; i++) {
            *GetGemItemEntry(i) = buffer[i];
        }
    }
    return buffer;
}

RVA(0x00024830, 0x30)
i16 WriteGemItems(FILE* fp) {
    return 16 - fwrite(g_gemItems, sizeof(ItemStack), 16, fp);
}

RVA(0x00024860, 0x30)
i16 ReadGemItems(FILE* fp) {
    return 16 - fread(g_gemItems, sizeof(ItemStack), 16, fp);
}

RVA(0x00024890, 0xc0)
void ApplyItemEffect(i16 item, Character* user, Character* target) {
    s_usedItem = *GetLoadedRecord(item);
    switch (s_usedItem.kind) {
        case ITEM_KIND_RESTORATIVE:
            UseRestoreItem(user, target);
            break;
        case ITEM_KIND_ATTACK:
            UseAttackItem(user, target);
            break;
        case 5:
            UseKind5Item(user, target);
            break;
        case 6:
        case ITEM_KIND_SOFTWARE:
        case ITEM_KIND_KEYCARD:
        case ITEM_KIND_GEM:
        case ITEM_KIND_SCENARIO:
        case ITEM_KIND_WEAPON:
        case ITEM_KIND_GUN:
        case ITEM_KIND_AMMO:
        case ITEM_KIND_FULL_BODY_ARMOR:
        case ITEM_KIND_HEAD_ARMOR:
        case ITEM_KIND_BODY_ARMOR:
        case ITEM_KIND_ARM_ARMOR:
        case ITEM_KIND_LEG_ARMOR:
        case ITEM_KIND_ACCESSORY:
            UseInertItem(user, target);
            break;
    }
}

RVA(0x00024950, 0x100)
void UseRestoreItem(Character* user, Character* target) {
    i16 mp;
    i16 hp;
    i16 result;

    g_statusCondition = 0;
    mp = ComputeRestoreAmount(s_usedItem.params[8], user, target->pools.mp.max);
    g_mpChange = mp;
    hp = ComputeRestoreAmount(s_usedItem.params[7], user, target->pools.hp.max);
    if (hp != 0) {
        user->lastChange = hp;
    } else {
        user->lastChange = mp;
    }
    g_hpChange = hp;
    result = ApplyRestoreEffect(s_usedItem.params[9], hp, target, mp);
    user->lastChange = target->lastChange;
    g_actionResult = result;
    user->pickNoEffect = true;
    user->result = result;
    g_pendingCondition = s_usedItem.params[10];
    if (g_pendingCondition != 0 && RestoreEffectAllowsCondition(result)
        && !IsConditionResisted(target, g_pendingCondition)) {
        g_statusCondition = g_pendingCondition;
        InflictCondition(g_pendingCondition, target);
    }
}

RVA(0x00024a50, 0xd0)
void UseAttackItem(Character* user, Character* target) {
    user->result = 0;
    ResetActionOutcome();
    if (GetItemDamagePower(&s_usedItem) == 0) {
        user->lastChange = 0;
        if (g_targetId >= 0 && IsFieldModeAtLeast(false)) {
            if (GetItemInflictedCondition(&s_usedItem) >= 0x21
                || IsFieldConditionRestricted(GetItemInflictedCondition(&s_usedItem))) {
                return;
            }
        }
        if (ResolveItemAttack(user, target, 0) > 0) {
            g_statusCondition = GetItemInflictedCondition(&s_usedItem);
            if (g_statusCondition != 0) {
                InflictCondition(g_statusCondition, target);
            }
        }
    } else {
        g_hpChange = GetItemHitPower(&s_usedItem);
        user->lastChange = g_hpChange;
        RunItemAttack(user, target);
    }
}

static __inline void SetInertItemOutcome(Character* user, Character* target) {
    ResetActionOutcome();
    user->result = 0;
    user->pickNoEffect = true;
    target->pickNoEffect = true;
}

RVA(0x00024b20, 0x40)
void UseKind5Item(Character* user, Character* target) {
    SetInertItemOutcome(user, target);
}

RVA(0x00024b60, 0x40)
void UseInertItem(Character* user, Character* target) {
    SetInertItemOutcome(user, target);
}

RVA(0x00024ba0, 0x1b0)
void AddItemStatPoints(i16 item, i16* stats) {
    i16 code;

    if (item == 0 || item == ITEM_ID_EMPTY) {
        return;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0 && g_loadedItem.kind != ITEM_KIND_GEM) {
        return;
    }
    code = g_loadedItem.kind != ITEM_KIND_GEM ? GetItemPassiveEffectCode(&g_loadedItem)
                                              : g_loadedItem.params[0xb];
    switch (code) {
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 9:
        case 10:
        case 11:
            stats[g_loadedItem.params[0xb] - 1]++;
            break;
        case 12:
            stats[STAT_STRENGTH]++;
            stats[STAT_CHARM]++;
            break;
        case 13:
            stats[STAT_VITALITY]++;
            stats[STAT_MENTAL_STRENGTH]++;
            break;
        case 14:
            stats[STAT_INTUITION]++;
            stats[STAT_AGILITY]++;
            break;
        case 15:
            stats[STAT_MAGIC]++;
            stats[STAT_PROTECTION]++;
            break;
        case 16:
            stats[STAT_INTELLIGENCE]++;
            stats[STAT_DEXTERITY]++;
            break;
        case 17:
            stats[STAT_STRENGTH]++;
            stats[STAT_VITALITY]++;
            stats[STAT_MENTAL_STRENGTH]++;
            break;
        case 32:
            stats[STAT_MENTAL_STRENGTH] += 4;
            break;
        case 33:
            stats[STAT_INTELLIGENCE] += 3;
            break;
        case 34:
            stats[STAT_PROTECTION]++;
            break;
        case 35:
            stats[STAT_PROTECTION] += 2;
            break;
        case 36:
            stats[STAT_AGILITY] -= 10;
            break;
        case 37:
            stats[STAT_CHARM] += 2;
            break;
        case 38:
            stats[STAT_CHARM] += 4;
            break;
        case 39:
            stats[STAT_CHARM] -= 3;
            break;
        case 40:
            stats[STAT_INTUITION]++;
            break;
        case 41:
            stats[STAT_AGILITY] += 3;
            break;
    }
}

// @early-stop control flow: retail's kind table gives kinds 15..19 their own
// entry (to the same body as 11 and 12); one case group shares a single entry,
// and a duplicated body cross-jumps with the blocks in the other order.
RVA(0x00024d50, 0x1f0)
void AddItemStatBonuses(i16 item, i16* bonuses, i16 indexed) {
    if (indexed && item != ITEM_ID_EMPTY) {
        item += GetGemItemBase();
    }
    if (item == 0 || item == ITEM_ID_EMPTY) {
        return;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0) {
        return;
    }
    switch (g_loadedItem.kind) {
        case ITEM_KIND_WEAPON:
        case ITEM_KIND_FULL_BODY_ARMOR:
        case ITEM_KIND_HEAD_ARMOR:
        case ITEM_KIND_BODY_ARMOR:
        case ITEM_KIND_ARM_ARMOR:
        case ITEM_KIND_LEG_ARMOR:
        case ITEM_KIND_ACCESSORY:
            bonuses[BATTLE_STAT_WEAPON_POWER] += GetItemAttackPower(&g_loadedItem);
            bonuses[BATTLE_STAT_WEAPON_DEFENSE] += GetItemDefensePower(&g_loadedItem);
            bonuses[BATTLE_STAT_WEAPON_ACCURACY] +=
                GetItemRecordPhysicalAccuracyBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_WEAPON_EVASION] += GetItemRecordPhysicalEvasionBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_POWER] += GetItemRecordMagicPowerBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_DEFENSE] += GetItemRecordMagicDefenseBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_ACCURACY] += GetItemRecordMagicAccuracyBonus(&g_loadedItem);
            break;
        case ITEM_KIND_GUN:
        case ITEM_KIND_AMMO:
            bonuses[BATTLE_STAT_GUN_POWER] += GetItemAttackPower(&g_loadedItem);
            bonuses[BATTLE_STAT_GUN_DEFENSE] += GetItemDefensePower(&g_loadedItem);
            bonuses[BATTLE_STAT_GUN_ACCURACY] += GetItemRecordPhysicalAccuracyBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_GUN_EVASION] += GetItemRecordPhysicalEvasionBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_POWER] += GetItemRecordMagicPowerBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_DEFENSE] += GetItemRecordMagicDefenseBonus(&g_loadedItem);
            bonuses[BATTLE_STAT_MAGIC_ACCURACY] += GetItemRecordMagicAccuracyBonus(&g_loadedItem);
            break;
    }
    switch (GetItemPassiveEffectCode(&g_loadedItem)) {
        case ITEM_PASSIVE_WEAPON_POWER_5:
            bonuses[BATTLE_STAT_WEAPON_POWER] += 5;
            break;
        case ITEM_PASSIVE_WEAPON_POWER_10:
            bonuses[BATTLE_STAT_WEAPON_POWER] += 10;
            break;
        case ITEM_PASSIVE_WEAPON_POWER_20:
            bonuses[BATTLE_STAT_WEAPON_POWER] += 20;
            break;
        case ITEM_PASSIVE_WEAPON_POWER_30:
            bonuses[BATTLE_STAT_WEAPON_POWER] += 30;
            break;
        case ITEM_PASSIVE_WEAPON_ACCURACY_20:
            bonuses[BATTLE_STAT_WEAPON_ACCURACY] += 20;
            break;
        case ITEM_PASSIVE_GUN_ACCURACY_20:
            bonuses[BATTLE_STAT_GUN_ACCURACY] += 20;
            break;
        case ITEM_PASSIVE_WEAPON_ACCURACY_BONUS:
            bonuses[BATTLE_STAT_WEAPON_ACCURACY] +=
                GetItemRecordPhysicalAccuracyBonus(&g_loadedItem);
            break;
        case ITEM_PASSIVE_MAGIC_EVASION_4:
            bonuses[BATTLE_STAT_MAGIC_EVASION] += 4;
            break;
        case ITEM_PASSIVE_MAGIC_EVASION_20:
            bonuses[BATTLE_STAT_MAGIC_EVASION] += 20;
            break;
        case ITEM_PASSIVE_DEFENSE_10:
            bonuses[BATTLE_STAT_WEAPON_DEFENSE] += 10;
            bonuses[BATTLE_STAT_GUN_DEFENSE] += 10;
            break;
        case ITEM_PASSIVE_DEFENSE_20:
            bonuses[BATTLE_STAT_WEAPON_DEFENSE] += 20;
            bonuses[BATTLE_STAT_GUN_DEFENSE] += 20;
            break;
        case ITEM_PASSIVE_MAGIC_DEFENSE_15:
            bonuses[BATTLE_STAT_MAGIC_DEFENSE] += 15;
            break;
    }
}

RVA(0x00024f40, 0x100)
i16 SumEquippedMagicDefenseBonus(Character* character, u8 groups) {
    i16 total = 0;
    if (groups & 1) {
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item);
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item);
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item);
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item);
        total +=
            GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item);
    }
    if (groups & 2) {
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item);
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item);
        total += GetItemMagicDefenseBonus(GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item);
    }
    return total;
}

RVA(0x00025040, 0x30)
i16 GetItemMagicDefenseBonus(i16 item) {
    if (item == 0 || item == ITEM_ID_EMPTY) {
        return 0;
    }
    DecodeItemRecord(&g_loadedItem, item);
    return GetItemRecordMagicDefenseBonus(&g_loadedItem);
}

RVA(0x00025070, 0x30)
i16 GetItemMagicAccuracyBonus(i16 item) {
    if (item == 0 || item == ITEM_ID_EMPTY) {
        return 0;
    }
    return GetItemRecordMagicAccuracyBonus(DecodeItemRecord(&g_loadedItem, item));
}

RVA(0x000250a0, 0x30)
i16 GetItemPhysicalEvasionBonus(i16 item) {
    if (item == 0 || item == ITEM_ID_EMPTY) {
        return 0;
    }
    return GetItemRecordPhysicalEvasionBonus(DecodeItemRecord(&g_loadedItem, item));
}

RVA(0x000250d0, 0x30)
i16 GetItemMagicPowerBonus(i16 item) {
    if (item == 0 || item == ITEM_ID_EMPTY) {
        return 0;
    }
    return GetItemRecordMagicPowerBonus(DecodeItemRecord(&g_loadedItem, item));
}

RVA(0x00025100, 0xb5)
void ApplyItemDamageRatio(i16 item, i16* ratios) {
    i16 code;
    i16 slot;
    i16 step;
    i32 value;

    if (item < 1) {
        return;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0) {
        return;
    }
    code = GetItemPassiveEffectCode(&g_loadedItem) - 0x40;
    if (code < 0 || code >= 40) {
        return;
    }
    step = code % 4;
    slot = code / 4;
    if (step >= 2) {
        step++;
    }
    value = (i16)(step * 50) * ratios[slot] / 100;
    if (value < -0x8000) {
        value = -0x8000;
    } else if (value > 0x7fff) {
        value = 0x7fff;
    }
    ratios[slot] = value;
}

RVA(0x000251c0, 0x18b)
i16 ScaleDamageByEquipment(Character* character, i16 damage, i16 element) {
    i16 ratios[10];
    i32 value;
    i16 i;

    if (element < 0 || element >= 10) {
        return damage;
    }
    if (damage < 1) {
        return damage;
    }
    for (i = 0; i < 10; i++) {
        ratios[i] = 100;
    }
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item, ratios);
    ApplyItemDamageRatio(GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item, ratios);
    value = ratios[element] * damage / 100;
    if (value < -0x8000) {
        value = -0x8000;
    } else if (value > 0x7fff) {
        value = 0x7fff;
    }
    return value;
}

RVA(0x00025350, 0x48)
b16 IsItemGuardingElement(i16 item, i16 element) {
    if (item < 1) {
        return false;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0) {
        return false;
    }
    if (element >= 2 && element <= 5 && element == GetEquipmentAttribute(&g_loadedItem)) {
        return true;
    }
    return false;
}

RVA(0x000253a0, 0x104)
i16 CountElementGuards(Character* character, i16 element) {
    i16 count;

    if (element < 0 || element >= 10) {
        return 0;
    }
    count = IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item, element);
    count += IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item, element);
    count += IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item, element);
    count += IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item, element);
    count +=
        IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item, element);
    count +=
        IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item, element);
    count += IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item, element);
    count += IsItemGuardingElement(GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item, element);
    return count;
}

RVA(0x000254b0, 0x143)
PoolRegen ApplyEquipmentRegen(Character* character) {
    PoolRegen regen;

    regen.hp = 0;
    regen.mp = 0;
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item, &regen);
    AddItemRegen(GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item, &regen);
    if (GetFatalCondition(GetCharacterConditions(character)) == 0) {
        FillPool(&character->pools.hp, regen.hp, POOL_FILL_TO_DOUBLE_MAX);
        FillPool(&character->pools.mp, regen.mp, POOL_FILL_TO_DOUBLE_MAX);
    }
    return regen;
}

RVA(0x00025600, 0x7c)
void AddItemRegen(i16 item, PoolRegen* regen) {
    if (item < 1) {
        return;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0) {
        return;
    }
    switch (GetItemPassiveEffectCode(&g_loadedItem)) {
        case ITEM_PASSIVE_HP_REGEN_1:
            regen->hp += 1;
            break;
        case ITEM_PASSIVE_HP_REGEN_2:
            regen->hp += 2;
            break;
        case ITEM_PASSIVE_HP_REGEN_3:
            regen->hp += 3;
            break;
        case ITEM_PASSIVE_HP_REGEN_5:
            regen->hp += 5;
            break;
        case ITEM_PASSIVE_MP_REGEN_1:
            regen->mp += 1;
            break;
    }
}

RVA(0x00025680, 0x1c0)
GZ_ENUM_RETURN(ConditionId, i16) ResolveInflictedCondition(GZ_ENUM_PARAM(InflictCode, i16) code, Character* target) {
    GZ_ENUM_LOCAL(ConditionId, i16) condition = CONDITION_NONE;
    i16 roll;

    switch (code) {
        case INFLICT_NONE:
            break;
        case CONDITION_DEAD:
        case CONDITION_DYING:
        case CONDITION_COLLAPSE:
        case CONDITION_STONE:
        case CONDITION_PARALYSIS:
        case CONDITION_FREEZE:
        case CONDITION_POSSESSION:
        case CONDITION_ZOMBIE:
        case CONDITION_CURSE:
        case CONDITION_STUN:
        case CONDITION_SUFFOCATION:
        case CONDITION_BIND:
        case CONDITION_SLEEP:
        case CONDITION_PANIC:
        case CONDITION_POISON:
        case CONDITION_HALLUCINATION:
        case CONDITION_CHARM:
        case CONDITION_CONFUSION:
        case CONDITION_DANCE:
        case CONDITION_SHOCK:
        case CONDITION_ICE:
        case CONDITION_BURN:
        case CONDITION_BLIND:
        case CONDITION_MAGIC_SEAL:
        case CONDITION_DOZE:
        case CONDITION_BERSERK:
        case CONDITION_HIGH:
        case CONDITION_HAPPY:
        case CONDITION_TIPSY:
        case CONDITION_DRUNK:
        case CONDITION_SLIME:
        case CONDITION_SEVERE_POISON:
        case CONDITION_VAMPIRE:
        case CONDITION_INJURY:
            condition = code;
            break;
        case INFLICT_CHARM_UNLESS_ALIGNED_B_POSITIVE:
            if (GetAlignmentClassB(target) <= ALIGNMENT_NEUTRAL) {
                condition = CONDITION_CHARM;
            }
            break;
        case INFLICT_TIPSY_BY_HALF:
            if (RandomUpTo(100) < 50) {
                condition = CONDITION_TIPSY;
            }
            break;
        case INFLICT_PARALYSIS_OR_TIPSY:
            if (GetDemonClass(target->id) == 8) {
                condition = CONDITION_PARALYSIS;
            } else if (RandomUpTo(100) < 50) {
                condition = CONDITION_TIPSY;
            }
            break;
        case INFLICT_HIGH_HALLUCINATION_OR_BERSERK:
            roll = RandomUpTo(3);
            if (roll <= 1) {
                condition = CONDITION_HIGH;
            } else {
                condition = roll == 2 ? CONDITION_HALLUCINATION : CONDITION_BERSERK;
            }
            break;
        case INFLICT_DRUNK_OR_TIPSY:
            condition = RandomUpTo(100) >= 75 ? CONDITION_DRUNK : CONDITION_TIPSY;
            break;
        case INFLICT_DYING_OR_DEAD:
            condition = RandomUpTo(100) < 75 ? CONDITION_DYING : CONDITION_DEAD;
            break;
        case INFLICT_HIGH_OR_ASH:
            condition = RandomUpTo(100) < 75 ? CONDITION_HIGH : CONDITION_ASH;
            break;
        case INFLICT_DEAD_BY_HALF:
            if (RandomUpTo(100) < 50) {
                condition = CONDITION_DEAD;
            }
            break;
        case INFLICT_PANIC_UNLESS_ALIGNED_B_NEGATIVE:
            if (GetAlignmentClassB(target) != ALIGNMENT_NEGATIVE) {
                condition = CONDITION_PANIC;
            }
            break;
    }
    return condition;
}

RVA(0x00025840, 0x7c)
void InflictCondition(i16 code, Character* target) {
    b16 had = HasCondition(GetCharacterConditions(target), CONDITION_ZOMBIE);
    i16 condition = ResolveInflictedCondition(code, target);

    if (condition >= 1) {
        AddCondition(GetCharacterConditions(target), condition);
    }
    if (HasCondition(GetCharacterConditions(target), CONDITION_ZOMBIE) && !had) {
        UpdateStatTotals(&target->stats);
        RecalcDerivedStats(target);
        ResetBattleStatsToBase(target);
    }
}

RVA(0x000258c0, 0x103)
i16 IsConditionResisted(Character* target, i16 code) {
    i16 condition = ResolveInflictedCondition(code, target);
    i16 resisted;

    if (condition < 1) {
        return 1;
    }
    resisted = ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_HEAD].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_BODY].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_ARMS].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_LEGS].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_ACCESSORY].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_WEAPON].item, condition);
    resisted |= ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_GUN].item, condition);
    resisted |=
        ItemResistsCondition(GetCharacterEquipment(target)[EQUIP_SLOT_AMMO].item, condition);
    return resisted;
}

RVA(0x000259d0, 0x1c4)
b16 ItemResistsCondition(i16 item, GZ_ENUM_PARAM(ConditionId, i16) condition) {
    if (item < 1) {
        return false;
    }
    DecodeItemRecord(&g_loadedItem, item);
    if (GetItemEquipCode(&g_loadedItem) < 0) {
        return false;
    }
    switch (GetItemPassiveEffectCode(&g_loadedItem)) {
        case ITEM_PASSIVE_RESIST_STONE:
            if (condition == CONDITION_STONE) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_PARALYSIS:
            if (condition == CONDITION_PARALYSIS) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_FREEZE_ICE:
            if (condition == CONDITION_FREEZE || condition == CONDITION_ICE) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_BIND:
            if (condition == CONDITION_BIND) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_SLEEP:
            if (condition == CONDITION_DOZE || condition == CONDITION_SLEEP) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_MENTAL:
            if (condition == CONDITION_CONFUSION || condition == CONDITION_HAPPY
                || condition == CONDITION_HALLUCINATION) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_HAPPY:
            if (condition == CONDITION_HAPPY) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_HALLUCINATION:
            if (condition == CONDITION_HALLUCINATION) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_PANIC:
            if (condition == CONDITION_PANIC) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_POISON:
            if (condition == CONDITION_POISON || condition == CONDITION_SEVERE_POISON) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_SHOCK:
            if (condition == CONDITION_SHOCK) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_BURN:
            if (condition == CONDITION_BURN) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_MAGIC_SEAL:
            if (condition == CONDITION_MAGIC_SEAL) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_INTOXICATION:
            if (condition == CONDITION_HIGH || condition == CONDITION_BERSERK
                || condition == CONDITION_TIPSY) {
                return true;
            }
            break;
        case ITEM_PASSIVE_RESIST_FIRE_AND_ICE:
            if (condition == CONDITION_BURN || condition == CONDITION_FREEZE
                || condition == CONDITION_ICE) {
                return true;
            }
            break;
    }
    return false;
}

RVA(0x00025ba0, 0x40)
i16 StampSpecialItem(i16 item) {
    item -= 0xad;
    if (item >= 0 && item < 8) {
        g_scriptVars[0xd0 + item] = GetClockMinutes() + 42560;
        return item;
    }
    return -1;
}

RVA(0x00025be0, 0x50)
i16 ExpireSpecialItem(i16 item) {
    item -= 0xad;
    if (item >= 0 && item < 8) {
        if (s_timedItemFlags[item].bank < 0) {
            return item;
        }
        ModifyEventFlag(
            s_timedItemFlags[item].bank,
            s_timedItemFlags[item].index,
            BIT_CHANGE_CLEAR
        );
        return item;
    }
    return -1;
}

RVA(0x00025c30, 0x30)
i16 IsSpecialItemExpired(i16 item) {
    item -= 0xad;
    if (item >= 0 && item < 8) {
        return GetClockMinutes() >= g_scriptVars[0xd0 + item];
    }
    return -1;
}

RVA(0x00025c60, 0x50)
i16 ExpireSpecialItems(void) {
    i16 expired = 0;
    i16 i;

    for (i = 0; i < 8; i++) {
        if (CountHeldItem(0xad + i) > 0 && IsSpecialItemExpired(0xad + i) >= 1) {
            ExpireSpecialItem(0xad + i);
            expired++;
        }
    }
    return expired;
}

static void DiscardMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x00025cb0, 0xe0)
#ifdef GITEN_BUGFIX
b32 RunBagDiscardMenu(void) {
#else
void RunBagDiscardMenu(void) {
#endif
    i16 entries[64];
    i16 count = 0;
    MenuBox* menu;
    i16 item;
    i16 i;

    for (i = 0; i < BAG_ENTRY_COUNT; i++) {
        item = GetBagItem(i);
        if (item != ITEM_ID_EMPTY && GetItemPrice(item) != 0
            && GetItemKind(item) != ITEM_KIND_SCENARIO) {
            entries[count++] = i;
        }
    }
    menu = CreateMenuBox(NULL, 0x19, 2);
    menu->flags |= 0x1e;
    SetMenuItems(menu, 8, entries, count, DiscardMenuHandler);
    MoveMenuBox(menu, -8, -0x16);
#ifdef GITEN_BUGFIX
    // @bug With no entry to list (every one priceless or a scenario item) the
    // menu, whose cancel is off, can never be left, and the entry it clears
    // would be read from the uninitialised `entries`. The menu then says the
    // item cannot be carried, turns cancel on and discards nothing, and
    // StoreBagItem stops trying to store the rest.
    if (count == 0) {
        SetTextPlaneCancelEnabled(menu->plane, 1);
        while (RunMenu(menu) != TEXT_EVENT_CANCEL) {
            WaitMenuFrame();
        }
        DestroyMenuBox(menu);
        return false;
    }
#endif
    SetTextPlaneCancelEnabled(menu->plane, 0);
    while (RunMenu(menu) != TEXT_EVENT_CHOOSE) {
        WaitMenuFrame();
    }
    DestroyMenuBox(menu);
    SetBagEntry(entries[g_selectedObjectId], -1, -1);
#ifdef GITEN_BUGFIX
    return true;
#endif
}

RVA(0x00025d90, 0x20)
void WaitMenuFrame(void) {
    g_mouseLeftClick = MOUSE_CLICK_NONE;
    g_mouseRightClick = MOUSE_CLICK_NONE;
    RunFrame();
}

RVA(0x00025db0, 0xc0)
static void DiscardMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16* entries = menu->items.entries;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            // "アイテム削除" (delete item)
            sprintf(g_scratchBuffer, "\203\101\203\103\203\145\203\200\215\355\217\234");
            AddMenuLine(menu->plane, g_scratchBuffer, TEXT_ATTR_DEFAULT, -1, MENU_LINE_DISABLED);
#ifdef GITEN_BUGFIX
            if (menu->itemCount == 0) {
                // "アイテムを持ちきれません" (the item cannot be carried)
                AddMenuLine(
                    menu->plane,
                    "\203\101\203\103\203\145\203\200\202\360\216\235"
                    "\202\277\202\253\202\352\202\334\202\271\202\361",
                    0x400,
                    -1,
                    1
                );
            }
#endif
            break;
        case MENU_EVENT_ADD_ROW:
            sprintf(
                g_scratchBuffer,
                "%-18.18s%2d",
                GetLoadedRecordName(GetBagItem(entries[index])),
                GetBagEntryCount(entries[index])
            );
            AddMenuLine(
                menu->plane,
                g_scratchBuffer,
                TEXT_ATTR_FLAG1 | TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_RED, TEXT_COLOR_BLACK),
                index,
                0
            );
            break;
    }
}

static MenuBox* CreateGiftMenu(MenuBox* old);
static void GiftMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event);

RVA(0x00025e70, 0xf0)
b16 RunGemItemGift(void) {
    i16 pick;
    Character* actor;

    switch (GetGamePhase()) {
        case 0:
            SetGamePhase(2);
            s_giftItemBase = GetGemItemBase();
            s_giftMenu = CreateGiftMenu(s_giftMenu);
            break;
        case 1:
            s_giftMenu = DestroyMenuBox(s_giftMenu);
            ReturnFromGameState();
            break;
        case 2:
            pick = RunListMenu(s_giftMenu);
            if (pick == -1) {
                break;
            }
            PrevGamePhase();
            if (pick == -2) {
                break;
            }
            actor = GetScriptActor();
            if (actor == NULL) {
                break;
            }
            actor->familiarity =
                ClampShort(s_giftFamiliarity[g_selectedObjectId] + actor->familiarity, 0, 63);
            TakeBagItems(s_giftItemBase + g_selectedObjectId, 1);
            break;
    }
    return false;
}

RVA(0x00025f60, 0x50)
static MenuBox* CreateGiftMenu(MenuBox* old) {
    MenuBox* menu = CreateMenuBox(old, 0x15, 2);

    SetMenuItems(menu, 16, NULL, 16, GiftMenuHandler);
    MoveMenuBox(menu, 0x2a, 0x50);
    SetTextPlaneFirstSelectableRow(menu->plane, 0, false);
    return menu;
}

RVA(0x00025fb0, 0xc0)
static void GiftMenuHandler(MenuBox* menu, i16 index, GZ_ENUM_PARAM(MenuEvent, i16) event) {
    i16 count;

    switch (event) {
        case MENU_EVENT_DESTROY:
            break;
        case MENU_EVENT_BEGIN_PAGE:
            SetTextPlaneMenuOrigin(menu->plane, 0, 0);
            break;
        case MENU_EVENT_ADD_ROW:
            count = CountGemItemsAt(index);
            sprintf(
                g_scratchBuffer,
                "%-14.14s %2d",
                GetLoadedRecordName(index + s_giftItemBase),
                count
            );
            if (count == 0) {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR(TEXT_COLOR_RED, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    index,
                    MENU_LINE_UNCHOOSABLE
                );
            } else {
                AddMenuLine(
                    menu->plane,
                    g_scratchBuffer,
                    TEXT_ATTR(TEXT_COLOR_WHITE, TEXT_COLOR_GREEN, TEXT_COLOR_BLACK),
                    index,
                    0
                );
            }
            break;
    }
}
