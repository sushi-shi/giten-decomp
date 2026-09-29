#ifndef GITEN_INPUT_MOUSECLICKSTATE_H
#define GITEN_INPUT_MOUSECLICKSTATE_H

#include <EnumDomain.h>

// A latched mouse click: none, or a click waiting to be taken.
GZ_ENUM_BEGIN_SPLIT(MouseClickState, i16)
    MOUSE_CLICK_PRESENT = -1,
    MOUSE_CLICK_NONE = 0
GZ_ENUM_END_SPLIT(MouseClickState)

#endif // GITEN_INPUT_MOUSECLICKSTATE_H
