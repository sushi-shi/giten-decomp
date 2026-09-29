#ifndef GITEN_SCRIPT_WINDOWREVERSEMODE_H
#define GITEN_SCRIPT_WINDOWREVERSEMODE_H

#include <EnumDomain.h>
#include <Enums.h>

// SetWindowReverse's mode: reverse the window's attribute, reset it, or both.
GZ_ENUM_BEGIN_SPLIT(WindowReverseMode, i16)
    WINDOW_ATTR_REVERSE = -1,
    WINDOW_ATTR_RESET = 0,
    WINDOW_ATTR_RESET_AND_REVERSE = 1
GZ_ENUM_END_SPLIT(WindowReverseMode)

#endif // GITEN_SCRIPT_WINDOWREVERSEMODE_H
