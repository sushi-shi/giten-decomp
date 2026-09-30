#ifndef GITEN_GAME_ITEMID_H
#define GITEN_GAME_ITEMID_H

#include <EnumDomain.h>
#include <Ints.h>

// Item ids from the record table; -1 is an empty stored slot and record 0
// is the zero-initialized no-item entry.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(ItemId, i16)
    ITEM_ID_EMPTY = -1,
    ITEM_ID_NONE = 0,
    ITEM_KUSHINADA_JAR = 0x21,
    ITEM_SOMA_CUP = 0x24,
    ITEM_CORE_SHIELD = 0x71,
    ITEM_LOVER_RIGHT_LEG = 0xad,
    ITEM_LOVER_LEFT_LEG = 0xae,
    ITEM_LOVER_RIGHT_ARM = 0xaf,
    ITEM_LOVER_LEFT_ARM = 0xb0,
    ITEM_LOVER_CHEST = 0xb1,
    ITEM_LOVER_ABDOMEN = 0xb2,
    ITEM_LOVER_HEAD = 0xb3,
    ITEM_LOVER_HEART = 0xb4
GZ_ENUM_END_SPLIT(ItemId)
// clang-format on

#endif // GITEN_GAME_ITEMID_H
