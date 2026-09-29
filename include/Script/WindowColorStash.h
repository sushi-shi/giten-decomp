#ifndef GITEN_SCRIPT_WINDOWCOLORSTASH_H
#define GITEN_SCRIPT_WINDOWCOLORSTASH_H

#include <Enums.h>

// Whether StashWindowColor saves the window's attribute or restores it.
GZ_ENUM_BEGIN(WindowColorStashAction)
    WINDOW_COLOR_RESTORE = 0,
    WINDOW_COLOR_SAVE = 1
GZ_ENUM_END(WindowColorStashAction)

#endif // GITEN_SCRIPT_WINDOWCOLORSTASH_H
