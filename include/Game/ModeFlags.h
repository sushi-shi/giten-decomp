#ifndef GITEN_GAME_MODEFLAGS_H
#define GITEN_GAME_MODEFLAGS_H

#include <Ints.h>

// These lifecycle flags may be set together.
typedef enum ModeFlags {
    MODE_FIELD = 0x0001,
    MODE_WORLD_MAP = 0x0002
} ModeFlags;

i16 TestFeatureMask(i16 bits);
i16 TestModeFlags(i16 bits);
i16 SetModeFlags(i16 bits);
i16 ClearModeFlags(i16 bits);

#endif // GITEN_GAME_MODEFLAGS_H
