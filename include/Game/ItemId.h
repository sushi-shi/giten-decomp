#ifndef GITEN_GAME_ITEMID_H
#define GITEN_GAME_ITEMID_H

#include <EnumDomain.h>
#include <Ints.h>

// Item ids identified by the item record table.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(ItemId, i16)
    ITEM_KUSHINADA_JAR = 0x21,
    ITEM_SOMA_CUP = 0x24,
    ITEM_CORE_SHIELD = 0x71
GZ_ENUM_END_SPLIT(ItemId)
    // clang-format on

// The empty encoding shared by equipment, bag entries and drop slots.
GZ_ENUM_CONST_BEGIN(ItemIdEncoding)
    ITEM_ID_EMPTY = -1
GZ_ENUM_CONST_END(ItemIdEncoding)

#endif // GITEN_GAME_ITEMID_H
