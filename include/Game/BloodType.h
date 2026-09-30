#ifndef GITEN_GAME_BLOODTYPE_H
#define GITEN_GAME_BLOODTYPE_H

#include <EnumDomain.h>
#include <Ints.h>

// Indices into the script's blood-type text table, in its A/B/AB/O order.
GZ_ENUM_BEGIN_SPLIT(BloodType, u8)
    BLOOD_TYPE_A = 0,
    BLOOD_TYPE_B = 1,
    BLOOD_TYPE_AB = 2,
    BLOOD_TYPE_O = 3,
    BLOOD_TYPE_COUNT = 4
GZ_ENUM_END_SPLIT(BloodType)

#endif // GITEN_GAME_BLOODTYPE_H
