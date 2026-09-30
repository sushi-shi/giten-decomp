#ifndef GITEN_TEXT_TEXTEVENT_H
#define GITEN_TEXT_TEXTEVENT_H

#include <Enums.h>

// The hook's events from PollMenuInput: cancelled, a line chosen by a left or
// a right click, the highlight taken off and put on (value: the line row).
// PollMenuInput (and RunMenu through it) returns NONE, CANCEL, CHOOSE or
// CHOOSE_RIGHT.
GZ_ENUM_BEGIN(TextEvent)
    TEXT_EVENT_CANCEL = -1,
    TEXT_EVENT_NONE = 0,
    TEXT_EVENT_CHOOSE = 1,
    TEXT_EVENT_CHOOSE_RIGHT = 2,
    TEXT_EVENT_UNHIGHLIGHT = 3,
    TEXT_EVENT_HIGHLIGHT = 4
GZ_ENUM_END(TextEvent)

#endif // GITEN_TEXT_TEXTEVENT_H
