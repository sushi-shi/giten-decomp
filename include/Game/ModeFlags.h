#ifndef GITEN_GAME_MODEFLAGS_H
#define GITEN_GAME_MODEFLAGS_H

#include <EnumDomain.h>
#include <Ints.h>

// These lifecycle flags may be set together.
GZ_ENUM_FLAGS_BEGIN(ModeFlags, i16)
    MODE_FIELD = 0x0001,
    MODE_WORLD_MAP = 0x0002
GZ_ENUM_FLAGS_END(ModeFlags)

i16 TestFeatureMask(i16 bits);
i16 TestModeFlags(i16 bits);
i16 SetModeFlags(i16 bits);
i16 ClearModeFlags(i16 bits);

#endif // GITEN_GAME_MODEFLAGS_H
