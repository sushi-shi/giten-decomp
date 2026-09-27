// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

// Codegen constraint: this TU does not include <Game/ModeFlags.h>; a prior
// prototype of TestModeFlags swaps its two loads.
#include <Game/GameLoop.h>

// @identity-TODO: the startup flag 0x80 has no reader in the claimed code.
DATA(0x000683f0)
static i16 s_modeFlags = 0x80;

RVA(0x00004370, 0xe)
i16 TestFeatureMask(i16 bits) {
    return g_featureMask & bits;
}

RVA(0x00004380, 0xc)
i16 TestModeFlags(i16 bits) {
    return bits & s_modeFlags;
}

RVA(0x00004390, 0x12)
i16 SetModeFlags(i16 bits) {
    return s_modeFlags |= bits;
}

RVA(0x000043b0, 0x16)
i16 ClearModeFlags(i16 bits) {
    return s_modeFlags &= ~bits;
}
