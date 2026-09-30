#ifndef GITEN_UTIL_BITSET_H
#define GITEN_UTIL_BITSET_H

#include <Ints.h>
#include <Util/BitChangeMode.h>

// Bit arrays, leftmost (most significant) bit of each byte first.
b32 TestBit(u8* bits, i16 index);
b32 SetBit(u8* bits, u16 index);
b32 ClearBit(u8* bits, u16 index);
b32 ToggleBit(u8* bits, u16 index);
b32 ChangeBit(u8* bits, u16 index, GZ_ENUM_PARAM(BitChangeMode, i16) op);

#endif // GITEN_UTIL_BITSET_H
