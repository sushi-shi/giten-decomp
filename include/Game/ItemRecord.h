#ifndef GITEN_GAME_ITEMRECORD_H
#define GITEN_GAME_ITEMRECORD_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/AttackAttribute.h>
#include <Game/ItemId.h>
#include <Game/ItemKind.h>
#include <Game/Skill.h>
#include <Game/SkillMessage.h>
#include <Game/TargetArea.h>
#include <Game/TargetFlags.h>
#include <Ints.h>

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
    GZ_ENUM_STORAGE(ItemId, i16) id;
    i32 price;
    GZ_ENUM_STORAGE(ItemKind, u8) kind;
    u8 params[0x33];
    SkillMessage beforeMessage;
    SkillMessage afterMessage;
    char* name;
    char* description;
} ItemRecord;

#define GetItemRecordKind(record) ((record)->kind)

static __inline char* GetItemRecordName(const ItemRecord* record) {
    return record->name;
}

static __inline i32 GetItemRecordPrice(const ItemRecord* record) {
    return record->price;
}

static __inline GZ_ENUM_RETURN(SkillUseModes, u8) GetItemUseModes(const ItemRecord* record) {
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

static __inline u8 GetWeaponMinHits(const ItemRecord* record) {
    return record->params[0x1d];
}

static __inline u8 GetWeaponMaxHits(const ItemRecord* record) {
    return record->params[0x1e];
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

#define GetItemAttackAttribute(record) ((record)->params[0xf])
#define GetWeaponHitModifier(record) ((record)->params[0x1c])

static __inline u8 GetItemShotId(const ItemRecord* record) {
    return record->params[0x32];
}

static __inline GZ_ENUM_RETURN(TargetArea, u8) GetItemTargetArea(const ItemRecord* record) {
    return record->params[3];
}

static __inline u8 GetItemTargetCounts(const ItemRecord* record) {
    return record->params[5];
}

static __inline GZ_ENUM_RETURN(TargetFlags, u8) GetItemTargetFlags(const ItemRecord* record) {
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

static __inline GZ_ENUM_RETURN(AttackAttribute, u8) GetEquipmentAttribute(
    const ItemRecord* record
) {
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

// An equipped item's passive effect (its params[0x25]; a gem's params[0xb]):
// stat points (AddItemStatPoints: a point in the item's own stat, a set of
// stats, or a fixed change), a battle-stat bonus (AddItemStatBonuses; the
// number is the bonus), HP or MP regeneration per
// turn (AddItemRegen), or resistance to conditions (ItemResistsCondition:
// mental is confusion, happy and hallucination; intoxication is high,
// berserk and tipsy; fire and ice is burn, freeze and ice), or HP/MP
// returned to the attacker after a weapon hit.
GZ_ENUM_BEGIN(ItemPassiveEffect)
    ITEM_PASSIVE_INTUITION_POINT = 1,
    ITEM_PASSIVE_MENTAL_STRENGTH_POINT = 2,
    ITEM_PASSIVE_MAGIC_POINT = 3,
    ITEM_PASSIVE_INTELLIGENCE_POINT = 4,
    ITEM_PASSIVE_PROTECTION_POINT = 5,
    ITEM_PASSIVE_STRENGTH_POINT = 6,
    ITEM_PASSIVE_VITALITY_POINT = 7,
    ITEM_PASSIVE_AGILITY_POINT = 8,
    ITEM_PASSIVE_DEXTERITY_POINT = 9,
    ITEM_PASSIVE_CHARM_POINT = 10,
    ITEM_PASSIVE_FORTUNE_POINT = 11,
    ITEM_PASSIVE_STRENGTH_CHARM_POINTS = 12,
    ITEM_PASSIVE_VITALITY_MENTAL_STRENGTH_POINTS = 13,
    ITEM_PASSIVE_INTUITION_AGILITY_POINTS = 14,
    ITEM_PASSIVE_MAGIC_PROTECTION_POINTS = 15,
    ITEM_PASSIVE_INTELLIGENCE_DEXTERITY_POINTS = 16,
    ITEM_PASSIVE_STRENGTH_VITALITY_MENTAL_STRENGTH_POINTS = 17,
    ITEM_PASSIVE_MENTAL_STRENGTH_PLUS_4 = 32,
    ITEM_PASSIVE_INTELLIGENCE_PLUS_3 = 33,
    ITEM_PASSIVE_PROTECTION_PLUS_1 = 34,
    ITEM_PASSIVE_PROTECTION_PLUS_2 = 35,
    ITEM_PASSIVE_AGILITY_MINUS_10 = 36,
    ITEM_PASSIVE_CHARM_PLUS_2 = 37,
    ITEM_PASSIVE_CHARM_PLUS_4 = 38,
    ITEM_PASSIVE_CHARM_MINUS_3 = 39,
    ITEM_PASSIVE_INTUITION_PLUS_1 = 40,
    ITEM_PASSIVE_AGILITY_PLUS_3 = 41,
    ITEM_PASSIVE_WEAPON_POWER_5 = 0x30,
    ITEM_PASSIVE_WEAPON_POWER_10 = 0x31,
    ITEM_PASSIVE_WEAPON_POWER_20 = 0x32,
    ITEM_PASSIVE_WEAPON_POWER_30 = 0x33,
    ITEM_PASSIVE_WEAPON_ACCURACY_20 = 0x34,
    ITEM_PASSIVE_GUN_ACCURACY_20 = 0x35,
    ITEM_PASSIVE_WEAPON_ACCURACY_BONUS = 0x36,
    ITEM_PASSIVE_MAGIC_EVASION_4 = 0x37,
    ITEM_PASSIVE_MAGIC_EVASION_20 = 0x38,
    ITEM_PASSIVE_DEFENSE_10 = 0x39,
    ITEM_PASSIVE_DEFENSE_20 = 0x3a,
    ITEM_PASSIVE_MAGIC_DEFENSE_15 = 0x3b,
    ITEM_PASSIVE_HP_REGEN_1 = 0x70,
    ITEM_PASSIVE_HP_REGEN_2 = 0x71,
    ITEM_PASSIVE_HP_REGEN_3 = 0x72,
    ITEM_PASSIVE_HP_REGEN_5 = 0x73,
    ITEM_PASSIVE_MP_REGEN_1 = 0x74,
    ITEM_PASSIVE_RESIST_STONE = 0x77,
    ITEM_PASSIVE_RESIST_PARALYSIS = 0x78,
    ITEM_PASSIVE_RESIST_FREEZE_ICE = 0x79,
    ITEM_PASSIVE_RESIST_BIND = 0x7a,
    ITEM_PASSIVE_RESIST_SLEEP = 0x7b,
    ITEM_PASSIVE_RESIST_MENTAL = 0x7c,
    ITEM_PASSIVE_RESIST_HAPPY = 0x7d,
    ITEM_PASSIVE_RESIST_HALLUCINATION = 0x7e,
    ITEM_PASSIVE_RESIST_PANIC = 0x7f,
    ITEM_PASSIVE_RESIST_POISON = 0x80,
    ITEM_PASSIVE_RESIST_SHOCK = 0x81,
    ITEM_PASSIVE_RESIST_BURN = 0x82,
    ITEM_PASSIVE_RESIST_MAGIC_SEAL = 0x83,
    ITEM_PASSIVE_RESIST_INTOXICATION = 0x84,
    ITEM_PASSIVE_RESIST_FIRE_AND_ICE = 0x85,
    ITEM_PASSIVE_WEAPON_HP_DRAIN = 0x86,
    ITEM_PASSIVE_WEAPON_MP_DRAIN = 0x87
GZ_ENUM_END(ItemPassiveEffect)

// Selects the equipped item's stat, resistance, regeneration or drain effect.
#define GetItemPassiveEffectCode(record) ((record)->params[0x25])

#define GetGemPassiveEffectCode(record) ((record)->params[0xb])

// The signed column in the character equipment-permission table; -1 means none.
static __inline i16 GetItemEquipCode(const ItemRecord* record) {
    i8 code = record->params[0x15];
    return code;
}

// The text buffers LoadItemFiles allocates for the decoded name and description.
// The shared record the accessors (and the item-use flow) decode into.
extern ItemRecord g_loadedItem;

extern char* g_itemNameText;
extern char* g_itemDescriptionText;

const u8* GetItemRecordData(i16 id);

// Loads the item records and auxiliary tables at game startup.
void LoadItemFiles(void);

ItemRecord* DecodeItemRecord(ItemRecord* record, i16 id);

const u8* ReadItemValueRange(ItemRecord* item, const u8* src);
const u8* ReadItemMessages(ItemRecord* item, const u8* src, i16 first, i16 second);
const u8* ReadItemEquipEffect(ItemRecord* item, const u8* src);
const u8* ReadItemExtraPair(ItemRecord* item, const u8* src);
const u8* ReadItemValueRangeAlt(ItemRecord* item, const u8* src);

// Accessors that decode item `id` into the shared record first.
char* GetLoadedRecordName(i16 id);
GZ_ENUM_RETURN(ItemKind, i16) GetItemKind(i16 id);
char* GetItemDescription(i16 id);
i32 GetItemPrice(i16 id);
i16 GetItemValueHigh(i16 id);
i16 GetItemValueLow(i16 id);
ItemRecord* GetLoadedRecord(i16 id);

#endif // GITEN_GAME_ITEMRECORD_H
