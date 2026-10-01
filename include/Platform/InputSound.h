#ifndef GITEN_PLATFORM_INPUTSOUND_H
#define GITEN_PLATFORM_INPUTSOUND_H

#include <rva.h>

#include <Input/Mouse.h>
#include <Platform/WinMM.h>

#include <dinput.h>
#include <dsound.h>

// The DirectSound effect player and the DirectInput mouse and keyboard of the
// Direct3D layer (C++ only).

extern LPDIRECTSOUND g_directSound;
// The effect buffers: a short one and one of nine seconds for long sounds.
extern LPDIRECTSOUNDBUFFER g_shortSoundBuffer;
extern LPDIRECTSOUNDBUFFER g_longSoundBuffer;

extern LPDIRECTINPUT g_directInput;
extern LPDIRECTINPUTDEVICE g_mouseDevice;
extern LPDIRECTINPUTDEVICE g_keyboardDevice;

#define ReleaseInputDevice(device)                                                                 \
    do {                                                                                           \
        if ((device) != NULL) {                                                                    \
            (device)->Unacquire();                                                                 \
            (device)->Release();                                                                   \
            (device) = NULL;                                                                       \
        }                                                                                          \
    } while (0)

#define SetInputDeviceAcquired(device, acquire)                                                    \
    do {                                                                                           \
        if ((device) != NULL) {                                                                    \
            if (acquire) {                                                                         \
                (device)->Acquire();                                                               \
            } else {                                                                               \
                (device)->Unacquire();                                                             \
            }                                                                                      \
        }                                                                                          \
    } while (0)

void ReleaseDirectInput(void);

// The sound effects: ids 1..SOUND_COUNT-1.
// @identity-TODO: what the looping effects are is unrecovered.
#define SOUND_COUNT 0x70
#define SOUND_STOP 0x1b
#define SOUND_LOOP_FIRST 0x52
#define SOUND_LOOP_SECOND 0x5d

#endif
