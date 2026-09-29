#ifndef GITEN_UTIL_BITCHANGEMODE_H
#define GITEN_UTIL_BITCHANGEMODE_H

#include <EnumDomain.h>

// How ModifyEventFlag changes a flag bit.
GZ_ENUM_BEGIN_SPLIT(BitChangeMode, i16)
    BIT_CHANGE_TOGGLE = -1,
    BIT_CHANGE_CLEAR = 0,
    BIT_CHANGE_SET = 1
GZ_ENUM_END_SPLIT(BitChangeMode)

#endif // GITEN_UTIL_BITCHANGEMODE_H
