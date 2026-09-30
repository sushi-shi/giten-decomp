#ifndef GITEN_TEXT_MENULINEFLAGS_H
#define GITEN_TEXT_MENULINEFLAGS_H

#include <EnumDomain.h>

// Disabled lines cannot be highlighted. Unchoosable lines can be highlighted
// but yield no selected value.
GZ_ENUM_FLAGS_BEGIN(MenuLineFlags, i16)
    MENU_LINE_NORMAL = 0,
    MENU_LINE_DISABLED = 0x1,
    MENU_LINE_UNCHOOSABLE = 0x2
GZ_ENUM_FLAGS_END(MenuLineFlags)

#endif // GITEN_TEXT_MENULINEFLAGS_H
