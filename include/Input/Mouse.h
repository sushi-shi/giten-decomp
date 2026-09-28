#ifndef GITEN_INPUT_MOUSE_H
#define GITEN_INPUT_MOUSE_H

#include <Ints.h>

// Mouse button bits as polled from DirectInput each frame.
typedef enum MouseButtonBits {
    MOUSE_LEFT_DOWN = 0x01,
    MOUSE_LEFT_WAS_DOWN = 0x02,
    MOUSE_LEFT_PRESSED = 0x04,
    MOUSE_LEFT_RELEASED = 0x08,
    MOUSE_RIGHT_DOWN = 0x10,
    MOUSE_RIGHT_WAS_DOWN = 0x20,
    MOUSE_RIGHT_PRESSED = 0x40,
    MOUSE_RIGHT_RELEASED = 0x80
} MouseButtonBits;

// One button's state in its nibble of those bits (the right button's shifted
// down by MOUSE_RIGHT_SHIFT): up, held, just pressed, just let go.
#define MOUSE_STATE_MASK 0x0f
#define MOUSE_RIGHT_SHIFT 4
#define MOUSE_UP 0
#define MOUSE_HELD (MOUSE_LEFT_DOWN | MOUSE_LEFT_WAS_DOWN)
#define MOUSE_CLICK (MOUSE_LEFT_DOWN | MOUSE_LEFT_PRESSED)
#define MOUSE_LET_GO (MOUSE_LEFT_WAS_DOWN | MOUSE_LEFT_RELEASED)

#define RecordMouseButtonEdges(buttons, current, previous, shift)                                  \
    do {                                                                                           \
        if ((current) != (previous)) {                                                             \
            if (current) {                                                                         \
                (buttons) |= MOUSE_LEFT_PRESSED << (shift);                                        \
            } else {                                                                               \
                (buttons) |= MOUSE_LEFT_RELEASED << (shift);                                       \
            }                                                                                      \
        }                                                                                          \
        if (previous) {                                                                            \
            (buttons) |= MOUSE_LEFT_WAS_DOWN << (shift);                                           \
        }                                                                                          \
        (previous) = (current);                                                                    \
    } while (0)

// The polled cursor position and button bits.
typedef struct MousePosition {
    i16 x;
    i16 y;
    i16 buttons;
} MousePosition;

// A click latch is -1 from the press until a consumer clears it, and keeps
// the cursor position of the press.
extern MousePosition g_mousePosition;
extern i16 g_mouseLeftClick;
extern i16 g_mouseLeftClickX;
extern i16 g_mouseLeftClickY;
extern i16 g_mouseRightClick;
extern i16 g_mouseRightClickX;
extern i16 g_mouseRightClickY;

// @identity-TODO: object ids the cursor code maintains (0x40a050 sets the
// hovered one and copies it to the selected one; the menu pick 0x4537a0 sets
// the item hit and the value it selects); a right-click cancel clears both.
extern i16 g_hoveredObjectId;
extern i16 g_selectedObjectId;

static __inline void ClearMouseSelection(void) {
    g_hoveredObjectId = -1;
    g_selectedObjectId = -1;
}

void LatchMouseClicks(void);
void ClearMouseClicks(void);
i16 TakeMouseCancel(i16 clearSelection);
i16 TakeMouseCancelSound(void);
i16 TakeMouseLeftClick(void);
void SetMouseState(i16 buttons, i16 x, i16 y);
i16 GetMouseX(void);
i16 GetMouseY(void);
i16 GetMouseRightClick(void);
i16 GetMouseLeftClick(void);

#endif // GITEN_INPUT_MOUSE_H
