// @identity-TODO: the owning TU is unproven; this unit holds the equipment
// effect span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/EquipEffect.h>
#include <Game/EquipSlotIndex.h>
#include <Game/ItemCurse.h>
#include <Game/ItemRecord.h>
#include <Game/Pool.h>
#include <Game/Stats.h>
#include <Util/Range.h>

// Applies the curse of equipped item `item` to `character` when its level is
// below the item's: at timing 0 a halved stat, at 2 an HP or MP drain, at 1 a
// chance of a condition.
// @identity-TODO: the curse codes (record byte +0x1e) and minimum level
// (+0x1f) are named from this reader only.
RVA(0x00026070, 0x400)
void ApplyItemCurse(
    Character* character,
    i16 item,
    GZ_ENUM_PARAM(EquipmentEffectTiming, i16) timing
) {
    ItemRecord* record;
    GZ_ENUM_LOCAL(ItemCurse, i16) curse;
    i16 minLevel;

    if (item < 1) {
        return;
    }
    record = GetLoadedRecord(item);
    curse = GetItemCurse(record);
    minLevel = GetItemCurseLevel(record);
    if (curse == ITEM_CURSE_NONE || minLevel <= character->level) {
        return;
    }
    switch (curse) {
        case ITEM_CURSE_HALVE_VITALITY:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_VITALITY,
                    ClampTo100(GetStatTotal(character, STAT_VITALITY) / 2)
                );
            }
            return;
        case ITEM_CURSE_HALVE_PROTECTION:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_PROTECTION,
                    ClampTo100(GetStatTotal(character, STAT_PROTECTION) / 2)
                );
            }
            return;
        case ITEM_CURSE_HALVE_AGILITY:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_AGILITY,
                    ClampTo100(GetStatTotal(character, STAT_AGILITY) / 2)
                );
            }
            return;
        case ITEM_CURSE_HALVE_CHARM:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_CHARM,
                    ClampTo100(GetStatTotal(character, STAT_CHARM) / 2)
                );
            }
            return;
        case ITEM_CURSE_HALVE_FORTUNE:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_FORTUNE,
                    ClampTo100(GetStatTotal(character, STAT_FORTUNE) / 2)
                );
            }
            return;
        case ITEM_CURSE_DRAIN_HP:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.hp, -1);
                ApplyEmptyPools(character);
            }
            return;
        case ITEM_CURSE_DRAIN_HP_HEAVY:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.hp, -3);
                ApplyEmptyPools(character);
            }
            return;
        case ITEM_CURSE_DRAIN_MP:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.mp, -1);
            }
            return;
        case ITEM_CURSE_BURNING:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), CONDITION_BURN);
            }
            return;
        case ITEM_CURSE_MAGIC_SEALED:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), CONDITION_MAGIC_SEAL);
            }
            return;
        case ITEM_CURSE_PANIC:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), CONDITION_PANIC);
            }
            return;
        case ITEM_CURSE_CONFUSED:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), CONDITION_CONFUSION);
            }
            return;
        case ITEM_CURSE_CHARMED:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), CONDITION_CHARM);
            }
            return;
        case ITEM_CURSE_DANCING:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), CONDITION_DANCE);
            }
            return;
        case ITEM_CURSE_BOUND:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), CONDITION_BIND);
            }
            return;
        case ITEM_CURSE_BERSERK:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), CONDITION_BERSERK);
            }
            return;
        case ITEM_CURSE_TIPSY:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), CONDITION_TIPSY);
            }
            return;
        case ITEM_CURSE_SLIME:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), CONDITION_SLIME);
            }
            return;
    }
}

// Applies the curses of all eight equipped items.
RVA(0x00026470, 0xe0)
void ApplyEquipmentEffects(
    Character* character,
    GZ_ENUM_STORAGE(EquipmentEffectTiming, i16) timing
) {
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_HEAD].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_BODY].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_ARMS].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_LEGS].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_ACCESSORY].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_WEAPON].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_GUN].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[EQUIP_SLOT_AMMO].item, timing);
}
