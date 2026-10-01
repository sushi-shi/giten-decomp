#ifndef GITEN_GAME_OBJECTRECORDID_H
#define GITEN_GAME_OBJECTRECORDID_H

#include <EnumDomain.h>
#include <Ints.h>

// Character record ids: human members initialized by InitCharacters and
// named field-object records on the disc.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(ObjectRecordId, i16)
    HUMAN_KATSURAGI = 0,
    HUMAN_TACHIBANA = 2,
    HUMAN_ASUKA = 4,
    HUMAN_NISHINO = 5,
    HUMAN_HAYASAKA = 6,
    HUMAN_SONODA = 7,
    HUMAN_KAMIKAWA = 8,
    HUMAN_SOUMA = 9,
    HUMAN_YAMASE = 10,
    HUMAN_KIRISHIMA = 11,
    HUMAN_NEWTON = 13,
    HUMAN_YAMADA = 14,
    HUMAN_TACHIKAWA = 15,
    OBJECT_RECORD_MARDUK = 0x22,
    OBJECT_RECORD_ISHTAR = 0x26,
    OBJECT_RECORD_PYANKARA = 0x36,
    OBJECT_RECORD_PRIMROSE = 0xce,
    OBJECT_RECORD_DOPPELGANGER = 0x117,
    OBJECT_RECORD_HELL_DOG = 0x18f
GZ_ENUM_END_SPLIT(ObjectRecordId)
// clang-format on

#endif // GITEN_GAME_OBJECTRECORDID_H
