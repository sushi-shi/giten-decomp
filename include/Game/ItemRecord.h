#ifndef GITEN_GAME_ITEMRECORD_H
#define GITEN_GAME_ITEMRECORD_H

#include <rva.h>

#include <Ints.h>
#include <Game/SkillMessage.h>
#include <Game/ItemKind.h>

typedef struct ItemTable {
    i16 count;
    u16 offsets[1];
} ItemTable;

// An item's record, decoded from its data-file entry by DecodeItemRecord: the
// id, the price, the kind (1..19) and the kind's parameter bytes, then the
// name and description (in the shared text buffers).
// @identity-TODO: `params` (record offsets +7..+0x39) hold per-kind bytes
// whose meaning is mostly unrecovered: +7/+8 are the low/high value that
// 0x423540 rolls between (ReadItemValueRange; +9/+0xa follow them), +0x1c..+0x1f
// the equipment effect (+0x1c defaults to 0xff), +0x2f a gun's magazine size
// (the equipment page loads that many rounds); +0x29/+0x2a are read by
// the helpers named for them.
typedef struct ItemRecord {
    i16 id;
    i32 price;
    GZ_ENUM_STORAGE(ItemKind, u8) kind;
    u8 params[0x33];
    SkillMessage beforeMessage;
    SkillMessage afterMessage;
    char* name;
    char* description;
} ItemRecord;

static __inline char* GetItemRecordName(const ItemRecord* record) {
    return record->name;
}

static __inline i32 GetItemRecordPrice(const ItemRecord* record) {
    return record->price;
}

static __inline u8 GetItemUseModes(const ItemRecord* record) {
    return record->params[2];
}

static __inline u8 GetItemRequiredVitality(const ItemRecord* record) {
    return record->params[0x1f];
}

static __inline u8 GetItemRequiredDexterity(const ItemRecord* record) {
    return record->params[0x20];
}

static __inline u8 GetItemAttackPower(const ItemRecord* record) {
    return record->params[0x19];
}

static __inline u8 GetItemDefensePower(const ItemRecord* record) {
    return record->params[0x1a];
}

#define GetItemDamagePower(record) ((record)->params[0xd])

static __inline u8 GetItemInflictedCondition(const ItemRecord* record) {
    return record->params[0x10];
}

static __inline u8 GetItemHitPower(const ItemRecord* record) {
    return record->params[0xc];
}

static __inline u8 GetItemShotId(const ItemRecord* record) {
    return record->params[0x32];
}

static __inline u8 GetItemTargetArea(const ItemRecord* record) {
    return record->params[3];
}

static __inline u8 GetItemTargetCounts(const ItemRecord* record) {
    return record->params[5];
}

static __inline u8 GetItemTargetFlags(const ItemRecord* record) {
    return record->params[4];
}

static __inline u8 GetItemAttackRange(const ItemRecord* record) {
    return record->params[6];
}

#define GetItemSkillId(record) ((record)->params[0x13])

static __inline u8 GetItemCurse(const ItemRecord* record) {
    return record->params[0x17];
}

static __inline u8 GetItemCurseLevel(const ItemRecord* record) {
    return record->params[0x18];
}

static __inline u8 GetArmorHitModifier(const ItemRecord* record) {
    return record->params[0x30];
}

static __inline u8 GetItemAmmoType(const ItemRecord* record) {
    return record->params[0x27];
}

static __inline u8 GetGunMagazineSize(const ItemRecord* record) {
    return record->params[0x28];
}

static __inline u8 GetGunBurstLimit(const ItemRecord* record) {
    return record->params[0x29];
}

static __inline u8 GetGunTargetLimits(const ItemRecord* record) {
    return record->params[0x2a];
}

static __inline u8 GetEquipmentInflictedCondition(const ItemRecord* record) {
    return record->params[0x24];
}

static __inline u8 GetEquipmentAttribute(const ItemRecord* record) {
    return record->params[0x21];
}

static __inline i16 GetItemRecordPhysicalAccuracyBonus(const ItemRecord* record) {
    i8 bonus = record->params[0x1b];
    return bonus;
}

static __inline i16 GetItemRecordPhysicalEvasionBonus(const ItemRecord* record) {
    i8 bonus = record->params[0x2c];
    return bonus;
}

static __inline i16 GetItemRecordMagicAccuracyBonus(const ItemRecord* record) {
    i8 bonus = record->params[0x2d];
    return bonus;
}

static __inline i16 GetItemRecordMagicPowerBonus(const ItemRecord* record) {
    i8 bonus = record->params[0x2e];
    return bonus;
}

static __inline i16 GetItemRecordMagicDefenseBonus(const ItemRecord* record) {
    i8 bonus = record->params[0x2f];
    return bonus;
}

// Selects the equipped item's stat, resistance, regeneration or drain effect.
#define GetItemPassiveEffectCode(record) ((record)->params[0x25])

// The signed column in the character equipment-permission table; -1 means none.
static __inline i16 GetItemEquipCode(ItemRecord* record) {
    i8 code = record->params[0x15];
    return code;
}

// The text buffers LoadItemFiles allocates for the decoded name and description.
// The shared record the accessors (and the item-use flow) decode into.
extern ItemRecord g_loadedItem;

extern char* g_itemNameText;
extern char* g_itemDescriptionText;

u8* GetItemRecordData(i16 id);

// Loads the item records and auxiliary tables at game startup.
void LoadItemFiles(void);

ItemRecord* DecodeItemRecord(ItemRecord* record, i16 id);

u8* ReadItemValueRange(ItemRecord* item, u8* src);
u8* ReadItemMessages(ItemRecord* item, u8* src, i16 first, i16 second);
u8* ReadItemEquipEffect(ItemRecord* item, u8* src);
u8* ReadItemExtraPair(ItemRecord* item, u8* src);
u8* ReadItemValueRangeAlt(ItemRecord* item, u8* src);

// Accessors that decode item `id` into the shared record first.
char* GetLoadedRecordName(i16 id);
i16 GetItemKind(i16 id);
char* GetItemDescription(i16 id);
i32 GetItemPrice(i16 id);
i16 GetItemValueHigh(i16 id);
i16 GetItemValueLow(i16 id);
ItemRecord* GetLoadedRecord(i16 id);

#endif // GITEN_GAME_ITEMRECORD_H
