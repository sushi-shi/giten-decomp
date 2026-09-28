// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Util/BitSet.h>
#include <Util/PixelMask.h>

RVA(0x0000b6d0, 0x29)
b32 TestBit(u8* bits, i16 index) {
    return (bits[index / 8] & GetPixelMask(index)) != 0;
}

// Each setter returns the bit's previous state.
RVA(0x0000b700, 0x37)
b32 SetBit(u8* bits, u16 index) {
    i32 prev = TestBit(bits, index);
    bits[index >> 3] |= GetPixelMask(index);
    return prev;
}

RVA(0x0000b740, 0x39)
b32 ClearBit(u8* bits, u16 index) {
    i32 prev = TestBit(bits, index);
    bits[index >> 3] &= ~GetPixelMask(index);
    return prev;
}

RVA(0x0000b780, 0x37)
b32 ToggleBit(u8* bits, u16 index) {
    i32 prev = TestBit(bits, index);
    bits[index >> 3] ^= GetPixelMask(index);
    return prev;
}

// op < 0 toggles, op > 0 sets, 0 clears.
RVA(0x0000b7c0, 0x45)
b32 ChangeBit(u8* bits, u16 index, i16 op) {
    if (op < 0) {
        return ToggleBit(bits, index);
    }
    if (op > 0) {
        return SetBit(bits, index);
    }
    return ClearBit(bits, index);
}
