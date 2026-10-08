// @identity-TODO: the original module name and membership with the handle routines are unknown.

#include <rva.h>

#include <Game/GameLoop.h>
#include <Game/ModeFlags.h>

// @identity-TODO: the startup flag 0x80 has no reader in the claimed code.
DATA(0x000683f0)
static i16 s_modeFlags = 0x80;

RVA(0x00004370, 0xe)
i16 TestFeatureMask(i16 bits) {
    return g_featureMask & bits;
}

RVA(0x00004380, 0xc)
i16 TestModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return bits & s_modeFlags;
}

RVA(0x00004390, 0x12)
i16 SetModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return s_modeFlags |= bits;
}

RVA(0x000043b0, 0x16)
i16 ClearModeFlags(GZ_ENUM_PARAM(ModeFlags, i16) bits) {
    return s_modeFlags &= ~bits;
}
