#ifndef GITEN_GAME_PICKFLAGS_H
#define GITEN_GAME_PICKFLAGS_H

#include <EnumDomain.h>
#include <Ints.h>

GZ_ENUM_BEGIN_SPLIT(PickFlags, u8)
    PICK_ITEM_SKILL = 4,
GZ_ENUM_END_SPLIT(PickFlags)

#endif // GITEN_GAME_PICKFLAGS_H
