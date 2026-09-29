#ifndef GITEN_SCRIPT_OWNEDFLAG_H
#define GITEN_SCRIPT_OWNEDFLAG_H

#include <EnumDomain.h>

// Flags of bank EVENT_FLAG_BANK_OWNED: the maps and arm-terminal programs the
// player holds, named after the developers' event-flag name table on the disc
// (ET0018, "...所持"/"...マップ入手"). A new game starts with every flag set;
// obtaining the item clears its flag.
GZ_ENUM_CONST_BEGIN(OwnedFlag)
    OWNED_TOCHO_MAP = 0,
    OWNED_TOCHO_2_MAP = 1,
    OWNED_HATSUDAI_MAP = 2,
    OWNED_SHIBUYA_MAP = 3,
    OWNED_GINZA_UNDERGROUND_MAP = 4,
    OWNED_YAESU_UNDERGROUND_MAP = 5,
    OWNED_DDS_V1_0 = 6,
    OWNED_DCS_V1_0 = 7,
    OWNED_DCS_MABUDACHI = 8,
    OWNED_AMS_V1_0 = 9,
    OWNED_ARS_V1_0 = 10,
    OWNED_DAS_V1_0 = 11,
    OWNED_DAS_V1_1 = 12,
    OWNED_DDS_V2_0 = 13,
    OWNED_DCS_V2_0 = 14,
    OWNED_AMS_V2_0 = 15,
    OWNED_DAS_V2_0 = 16,
    OWNED_SABER_1 = 17,
    OWNED_SABER_2 = 18,
    OWNED_SABER_3 = 19,
    OWNED_SABER_4 = 20,
    OWNED_SABER_5 = 21,
    OWNED_DDD = 22,
    OWNED_ELEMENT_A = 23,
    OWNED_ELEMENT_B = 24,
    OWNED_ELEMENT_C = 25,
    OWNED_DEVIL_ERASER = 26
GZ_ENUM_CONST_END(OwnedFlag)

#endif // GITEN_SCRIPT_OWNEDFLAG_H
