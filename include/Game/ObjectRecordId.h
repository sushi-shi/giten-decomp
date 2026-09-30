#ifndef GITEN_GAME_OBJECTRECORDID_H
#define GITEN_GAME_OBJECTRECORDID_H

#include <EnumDomain.h>
#include <Ints.h>

// Names carried by the corresponding object records on the disc.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(ObjectRecordId, i16)
    OBJECT_RECORD_MARDUK = 0x22,
    OBJECT_RECORD_ISHTAR = 0x26,
    OBJECT_RECORD_PYANKARA = 0x36,
    OBJECT_RECORD_PRIMROSE = 0xce,
    OBJECT_RECORD_DOPPELGANGER = 0x117,
    OBJECT_RECORD_HELL_DOG = 0x18f
GZ_ENUM_END_SPLIT(ObjectRecordId)
// clang-format on

#endif // GITEN_GAME_OBJECTRECORDID_H
