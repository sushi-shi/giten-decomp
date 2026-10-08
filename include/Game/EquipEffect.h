#ifndef GITEN_GAME_EQUIPEFFECT_H
#define GITEN_GAME_EQUIPEFFECT_H

#include <rva.h>

#include <EnumDomain.h>
#include <Game/Character.h>

GZ_ENUM_BEGIN_SPLIT(EquipmentEffectTiming, i16)
    EQUIP_EFFECT_STAT_UPDATE = 0,
    EQUIP_EFFECT_ACTION = 1,
    EQUIP_EFFECT_STEP_TICK = 2,
GZ_ENUM_END_SPLIT(EquipmentEffectTiming)
void ApplyItemCurse(
    CharacterCore* character,
    i16 item,
    GZ_ENUM_STORAGE(EquipmentEffectTiming, i16) timing
);
void ApplyEquipmentEffects(
    CharacterCore* character,
    GZ_ENUM_STORAGE(EquipmentEffectTiming, i16) timing
);

#endif // GITEN_GAME_EQUIPEFFECT_H
