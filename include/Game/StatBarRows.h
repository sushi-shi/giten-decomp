#ifndef GITEN_GAME_STATBARROWS_H
#define GITEN_GAME_STATBARROWS_H

#include <EnumDomain.h>

// Which rows of a stat bar the status panel draws.
GZ_ENUM_BEGIN_SPLIT(StatBarRows, i16)
    STAT_BAR_ROWS_FIRST = -1,
    STAT_BAR_ROWS_BOTH = 0,
    STAT_BAR_ROWS_SECOND = 1
GZ_ENUM_END_SPLIT(StatBarRows)

#endif // GITEN_GAME_STATBARROWS_H
