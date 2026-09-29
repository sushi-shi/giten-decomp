// @identity-TODO: the owning TU is unproven; this unit holds the equipment
// effect span until link-order evidence names it.

#include <rva.h>

#include <Game/Character.h>
#include <Game/Condition.h>
#include <Game/ConditionAge.h>
#include <Game/EquipEffect.h>
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
    i16 curse;
    i16 minLevel;

    if (item < 1) {
        return;
    }
    record = GetLoadedRecord(item);
    curse = GetItemCurse(record);
    minLevel = GetItemCurseLevel(record);
    if (curse == 0 || minLevel <= character->level) {
        return;
    }
    switch (curse) {
        case 1:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_VITALITY,
                    ClampTo100(GetStatTotal(character, STAT_VITALITY) / 2)
                );
            }
            return;
        case 2:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_PROTECTION,
                    ClampTo100(GetStatTotal(character, STAT_PROTECTION) / 2)
                );
            }
            return;
        case 3:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_AGILITY,
                    ClampTo100(GetStatTotal(character, STAT_AGILITY) / 2)
                );
            }
            return;
        case 4:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_CHARM,
                    ClampTo100(GetStatTotal(character, STAT_CHARM) / 2)
                );
            }
            return;
        case 5:
            if (timing == EQUIP_EFFECT_STAT_UPDATE) {
                SetStatTotal(
                    character,
                    STAT_FORTUNE,
                    ClampTo100(GetStatTotal(character, STAT_FORTUNE) / 2)
                );
            }
            return;
        case 6:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.hp, -1);
                ApplyEmptyPools(character);
            }
            return;
        case 7:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.hp, -3);
                ApplyEmptyPools(character);
            }
            return;
        case 8:
            if (timing == EQUIP_EFFECT_STEP_TICK) {
                ChangePool(&character->pools.mp, -1);
            }
            return;
        case 16:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), 0x16);
            }
            return;
        case 17:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), 0x18);
            }
            return;
        case 18:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), 0xe);
            }
            return;
        case 19:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), 0x12);
            }
            return;
        case 20:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), 0x11);
            }
            return;
        case 21:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), 0x13);
            }
            return;
        case 22:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), 0xc);
            }
            return;
        case 23:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), 0x1a);
            }
            return;
        case 24:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x40) {
                AddCondition(GetCharacterConditions(character), 0x1d);
            }
            return;
        case 25:
            if (timing == EQUIP_EFFECT_ACTION && RandomUpTo(0xff) < 0x20) {
                AddCondition(GetCharacterConditions(character), 0x1f);
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
    ApplyItemCurse(character, GetCharacterEquipment(character)[0].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[1].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[2].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[3].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[4].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[5].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[6].item, timing);
    ApplyItemCurse(character, GetCharacterEquipment(character)[7].item, timing);
}
