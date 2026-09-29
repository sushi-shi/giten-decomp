#ifndef GITEN_GAME_EQUIPPART_H
#define GITEN_GAME_EQUIPPART_H

#include <Enums.h>

// Logical equipment parts, distinct from the packed slot order.
// clang-format off
GZ_ENUM_BEGIN(EquipPart)
    EQUIP_PART_NONE = -1,
    EQUIP_PART_WEAPON = 0,
    EQUIP_PART_GUN = 1,
    EQUIP_PART_AMMO = 2,
    EQUIP_PART_HEAD = 3,
    EQUIP_PART_BODY = 4,
    EQUIP_PART_ARMS = 5,
    EQUIP_PART_LEGS = 6,
    EQUIP_PART_ACCESSORY = 7
GZ_ENUM_END(EquipPart);
// clang-format on

#endif // GITEN_GAME_EQUIPPART_H
