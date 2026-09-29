#ifndef GITEN_INPUT_MOUSECANCELMODE_H
#define GITEN_INPUT_MOUSECANCELMODE_H

#include <EnumDomain.h>

// Both modes consume the click; accepting it also clears the selection.
GZ_ENUM_BEGIN_SPLIT(MouseCancelMode, i16)
    MOUSE_CANCEL_IGNORE = 0,
    MOUSE_CANCEL_ACCEPT = 1
GZ_ENUM_END_SPLIT(MouseCancelMode)

#endif // GITEN_INPUT_MOUSECANCELMODE_H
