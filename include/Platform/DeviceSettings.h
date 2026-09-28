#ifndef GITEN_PLATFORM_DEVICESETTINGS_H
#define GITEN_PLATFORM_DEVICESETTINGS_H

#include <rva.h>

#include <Platform/Direct3D.h>

// The Direct3D summary the device setup reads, per device kind.
// @identity-TODO: filled by the device query and the entry TU (hardwareOnly);
// the three byte tables' roles are read from the device setup only.
struct D3DCaps {
    BOOL hardwareOnly;
    DWORD zBufferDepth[3];
    u8 bilinearFiltering[3];
    u8 dither[3];
    u8 blendMode[3]; // BLEND_MODE_ALPHA: real alpha blending, else stippled
    u8 reserved;
    DWORD driverCaps;
    DWORD surfaceCaps;
    D3DPRIMCAPS primCaps[3];
};
// The complete binary DevConfig registry record.
struct DeviceSettings {
    // @identity-TODO: the registry record's first sixteen bytes are not read by this executable.
    u8 prefix[16];
    D3DCaps caps;
};

extern DeviceSettings g_deviceSettings;

RVA_DECL(0x00045ce0)
b32 LoadDeviceSettings(DeviceSettings* settings);
RVA_DECL(0x00045d60)
void __fastcall GetDeviceSettingsValueName(char* name);

#endif
