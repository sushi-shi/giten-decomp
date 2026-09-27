// The joystick: its axis ranges from the device caps, and the per-frame read
// that folds in the keyboard stand-ins.

#include <rva.h>

#include <Win32.h>

#include <Platform/Joystick.h>
#include <Platform/WinMM.h>

#include <string.h>

// The joysticks read (at most one; -1 before InitJoystick).
DATA(0x0006b4dc)
static i32 s_joystickCount = -1;

DATA(0x00084330)
static JoystickRange s_joystickRanges[1];

// Finds the joysticks and keeps the first one's axis ranges.
RVA(0x000492b0, 0xbb)
void InitJoystick(void) {
    JOYCAPS caps;
    i32 i;

    s_joystickCount = joyGetNumDevs();
    if (s_joystickCount > 1) {
        s_joystickCount = 1;
    }
    for (i = 0; i < s_joystickCount; i++) {
        memset(&caps, 0, sizeof(caps));
        joyGetDevCaps(i, &caps, sizeof(caps));
        s_joystickRanges[i].xHalf = (caps.wXmax - caps.wXmin) / 2;
        s_joystickRanges[i].xCenter = caps.wXmin + s_joystickRanges[i].xHalf;
        s_joystickRanges[i].yHalf = (caps.wYmax - caps.wYmin) / 2;
        s_joystickRanges[i].yCenter = caps.wYmin + s_joystickRanges[i].yHalf;
        s_joystickRanges[i].zHalf = (caps.wZmax - caps.wZmin) / 2;
        s_joystickRanges[i].zCenter = caps.wZmin + s_joystickRanges[i].zHalf;
        s_joystickRanges[i].rHalf = (caps.wRmax - caps.wRmin) / 2;
        s_joystickRanges[i].rCenter = caps.wRmin + s_joystickRanges[i].rHalf;
    }
}

DATA(0x0006b7a8)
static JoystickKey s_joystickKeys[8] = {
    {JOY_UP, VK_UP},
    {JOY_DOWN, VK_DOWN},
    {JOY_LEFT, VK_LEFT},
    {JOY_RIGHT, VK_RIGHT},
    {1 << JOY_BUTTON_SHIFT, VK_RETURN},
    {2 << JOY_BUTTON_SHIFT, VK_SPACE},
    {4 << JOY_BUTTON_SHIFT, VK_SHIFT},
    {0, 0},
};

// Returns the joystick bits: the arrow keys, Return, Space and Shift, then the
// stick past a quarter of its travel and its buttons. `state` (optional) gets
// the bits, the axes' offsets from centre and their half travels (without a
// joystick, the arrow keys as -1..1 on a travel of 1).
RVA(0x00049370, 0x1be)
u32 ReadJoystick(JoystickState* state) {
    u32 buttons = 0;
    JoystickKey* key;
    BOOL read;
    JOYINFOEX info;
    i32 axis;

    if (state != NULL) {
        memset(state, 0, sizeof(*state));
    }
    if (s_joystickCount < 0) {
        return 0;
    }
    for (key = s_joystickKeys; key->bit != 0; key++) {
        if (GetAsyncKeyState(key->key) & 0x8000) {
            buttons |= key->bit;
        }
    }
    read = FALSE;
    if (s_joystickCount > 0) {
        info.dwSize = sizeof(info);
        info.dwFlags = JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | JOY_RETURNR | JOY_RETURNBUTTONS
                       | JOY_RETURNCENTERED;
        if (joyGetPosEx(0, &info) == JOYERR_NOERROR) {
            read = TRUE;
        }
    }
    if (read) {
        i32 x = info.dwXpos;
        i32 y = info.dwYpos;
        i32 threshold = s_joystickRanges[0].xHalf / 4;
        if (x < s_joystickRanges[0].xCenter - threshold) {
            buttons |= JOY_LEFT;
        } else if (x > s_joystickRanges[0].xCenter + threshold) {
            buttons |= JOY_RIGHT;
        }
        threshold = s_joystickRanges[0].yHalf / 4;
        if (y < s_joystickRanges[0].yCenter - threshold) {
            buttons |= JOY_UP;
        } else if (y > s_joystickRanges[0].yCenter + threshold) {
            buttons |= JOY_DOWN;
        }
        buttons |= (info.dwButtons & 0xff) << JOY_BUTTON_SHIFT;
        if (state != NULL) {
            state->x = info.dwXpos - s_joystickRanges[0].xCenter;
            state->y = info.dwYpos - s_joystickRanges[0].yCenter;
            state->z = info.dwZpos - s_joystickRanges[0].zCenter;
            state->r = info.dwRpos - s_joystickRanges[0].rCenter;
            state->xHalf = s_joystickRanges[0].xHalf;
            state->yHalf = s_joystickRanges[0].yHalf;
            state->zHalf = s_joystickRanges[0].zHalf;
            state->rHalf = s_joystickRanges[0].rHalf;
        }
    } else {
        axis = 0;
        if (buttons & JOY_LEFT) {
            axis = -1;
        }
        if (buttons & JOY_RIGHT) {
            axis++;
        }
        if (state != NULL) {
            state->x = axis;
            state->xHalf = 1;
        }
        axis = 0;
        if (buttons & JOY_UP) {
            axis = -1;
        }
        if (buttons & JOY_DOWN) {
            axis++;
        }
        if (state != NULL) {
            state->y = axis;
            state->yHalf = 1;
            state->zHalf = 0;
            state->rHalf = 0;
        }
    }
    if (state != NULL) {
        state->buttons = buttons;
    }
    return buttons;
}
