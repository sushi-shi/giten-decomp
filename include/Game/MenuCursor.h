#ifndef GITEN_GAME_MENUCURSOR_H
#define GITEN_GAME_MENUCURSOR_H

#include <Ints.h>

// @identity-TODO: a four-level menu position (used by 0x4174b0/0x417520);
// setting a level resets the levels below it.
typedef struct MenuCursor {
    i16 level[4];
} MenuCursor;

i16 SetCursorLevel3(MenuCursor* cursor, i16 value);
i16 SetCursorLevel2(MenuCursor* cursor, i16 value);
i16 SetCursorLevel1(MenuCursor* cursor, i16 value);
i16 SetCursorLevel0(MenuCursor* cursor, i16 value);
i16 NextCursorLevel0(MenuCursor* cursor);
i16 PrevCursorLevel0(MenuCursor* cursor);
u16 GetCursorLevel0(MenuCursor* cursor);
i16 GetCursorLevel1(MenuCursor* cursor);

#endif // GITEN_GAME_MENUCURSOR_H
