#ifndef GITEN_GAME_ITEMSTACK_H
#define GITEN_GAME_ITEMSTACK_H

#include <Ints.h>

// A counted item entry of the bag and the gem item table. An empty
// entry has item -1; attachment is a gem index valid when hasAttachment
// is set. Otherwise the entry carries no attached item.
// @identity-TODO: what `detail` (read by 0x4243d0 into ReadBagEntry's count
// output) holds is unrecovered.
typedef union ItemStack {
    // Script long variables carry the complete packed entry.
    u32 value;
    struct {
        i16 item : 11;
        i16 attachment : 5;
        u16 count : 8;
        u16 detail : 7;
        u16 hasAttachment : 1;
    };
} ItemStack;

#define ClearItemStack(entry)                                                                      \
    do {                                                                                           \
        (entry)->item = -1;                                                                        \
        (entry)->attachment = 0;                                                                   \
        (entry)->count = 0;                                                                        \
        (entry)->detail = 0;                                                                       \
        (entry)->hasAttachment = 0;                                                                \
    } while (0)

#define HasItemStackAttachment(entry) ((entry)->hasAttachment)

static __inline i16 GetItemStackItem(const ItemStack* entry) {
    return entry->item;
}

static __inline i16 GetItemStackCount(const ItemStack* entry) {
    return entry->count;
}

static __inline void SetItemStackCount(ItemStack* entry, u8 count) {
    entry->count = count;
}

// A copied run of non-empty entries: the number copied, then the entries.
typedef struct ItemStackList {
    i16 count;
    ItemStack entries[64];
} ItemStackList;

static __inline i16 GetItemListCount(const ItemStackList* list) {
    return list->count;
}

#define GetItemListEntry(list, index) (&(list)->entries[(index)])

#endif // GITEN_GAME_ITEMSTACK_H
