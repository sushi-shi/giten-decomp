#ifndef GITEN_PLATFORM_JOYSTICK_H
#define GITEN_PLATFORM_JOYSTICK_H

#include <rva.h>

#include <Ints.h>

// The joystick, read through winmm, with the arrow keys, Return, Space and
// Shift standing in for it (C++ only).

// The joystick and its keyboard stand-ins: ReadJoystick's button bits.
#define JOY_UP 0x01
#define JOY_DOWN 0x02
#define JOY_LEFT 0x04
#define JOY_RIGHT 0x08
#define JOY_BUTTON_SHIFT 4

// The third button (Shift): step sideways or back instead of turning.
#define JOY_SIDESTEP (4 << JOY_BUTTON_SHIFT)

// What ReadJoystick reports: the bits, each axis's offset from its centre and
// its half travel.
struct JoystickState {
    u32 buttons;
    i32 x;
    i32 y;
    i32 z;
    i32 r;
    i32 xHalf;
    i32 yHalf;
    i32 zHalf;
    i32 rHalf;
};

// Each axis's centre and half its travel, from the device caps.
struct JoystickRange {
    i32 xCenter;
    i32 xHalf;
    i32 yCenter;
    i32 yHalf;
    i32 zCenter;
    i32 zHalf;
    i32 rCenter;
    i32 rHalf;
};

#define SetJoystickAxisRange(center, half, minimum, maximum)                                       \
    do {                                                                                           \
        (half) = ((maximum) - (minimum)) / 2;                                                      \
        (center) = (minimum) + (half);                                                             \
    } while (0)

// A key standing in for a joystick bit.
struct JoystickKey {
    u32 bit;
    i32 key;
};

// The keys ReadJoystick folds into the joystick bits, zero-terminated.
extern JoystickKey g_joystickKeys[8];

void InitJoystick(void);
u32 ReadJoystick(JoystickState* state);

#endif // GITEN_PLATFORM_JOYSTICK_H
