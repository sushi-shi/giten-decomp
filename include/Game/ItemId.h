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
    ITEM_CORE_SHIELD = 0x71
GZ_ENUM_END_SPLIT(ItemId)
// clang-format on

#endif // GITEN_GAME_ITEMID_H
