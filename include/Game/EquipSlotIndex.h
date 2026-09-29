#ifndef GITEN_GAME_EQUIPSLOTINDEX_H
#define GITEN_GAME_EQUIPSLOTINDEX_H

#include <Enums.h>

// The order of a character's equipment slots; GetEquipSlot maps an EquipPart
// to its slot.
GZ_ENUM_BEGIN(EquipSlotIndex)
    EQUIP_SLOT_HEAD = 0,
    EQUIP_SLOT_BODY = 1,
    EQUIP_SLOT_ARMS = 2,
    EQUIP_SLOT_LEGS = 3,
    EQUIP_SLOT_ACCESSORY = 4,
    EQUIP_SLOT_WEAPON = 5,
    EQUIP_SLOT_GUN = 6,
    EQUIP_SLOT_AMMO = 7,
    EQUIP_SLOT_COUNT = 8
GZ_ENUM_END(EquipSlotIndex)

#endif // GITEN_GAME_EQUIPSLOTINDEX_H
