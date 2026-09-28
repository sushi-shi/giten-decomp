#ifndef GITEN_GFX_DISPLAYCONFIG_H
#define GITEN_GFX_DISPLAYCONFIG_H

#include <Win32.h>

#include <Ints.h>

// The display settings kept under HKCU\Software\ASCII\GITEN_DDS: the chosen
// DirectDraw driver's GUID, whether to pick the driver automatically, and the
// settings version (4).
typedef struct DisplayConfig {
    GUID driver;
    u8 autoSelect;
    u8 version;
} DisplayConfig;

#ifdef __cplusplus
extern "C" {
#endif

    i32 LoadDisplayConfig(DisplayConfig* config);
    void SetDefaultDisplayConfig(DisplayConfig* config);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GFX_DISPLAYCONFIG_H
