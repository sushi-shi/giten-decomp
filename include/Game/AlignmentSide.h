#ifndef GITEN_GAME_ALIGNMENTSIDE_H
#define GITEN_GAME_ALIGNMENTSIDE_H

#include <EnumDomain.h>

// The sign of an alignment class (GetAlignmentClassB).
GZ_ENUM_BEGIN_SPLIT(AlignmentSide, i16)
    ALIGNMENT_NEGATIVE = -1,
    ALIGNMENT_NEUTRAL = 0,
    ALIGNMENT_POSITIVE = 1
GZ_ENUM_END_SPLIT(AlignmentSide)

#endif // GITEN_GAME_ALIGNMENTSIDE_H
