// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Gfx/DisplayConfig.h>

// Reads the display settings; any missing value (or another version) falls
// back to the defaults.
RVA(0x00045bb0, 0xe6)
i32 LoadDisplayConfig(DisplayConfig* config) {
    DWORD type = REG_BINARY;
    DWORD size = 2;
    HKEY key;
    b32 failed = true;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, "Software\\ASCII\\GITEN_DDS", 0, KEY_QUERY_VALUE, &key)
        == ERROR_SUCCESS) {
        size = 1;
        if (RegQueryValueExA(key, "Version", NULL, &type, &config->version, &size) == ERROR_SUCCESS
            && config->version == 4) {
            size = sizeof(GUID);
            if (RegQueryValueExA(key, "GUID", NULL, &type, (BYTE*)&config->driver, &size)
                == ERROR_SUCCESS) {
                size = 1;
                if (RegQueryValueExA(key, "AutoSelect", NULL, &type, &config->autoSelect, &size)
                    == ERROR_SUCCESS) {
                    failed = false;
                }
            }
        }
    }
    if (failed) {
        SetDefaultDisplayConfig(config);
    }
    return RegCloseKey(key);
}

RVA(0x00045ca0, 0x32)
void SetDefaultDisplayConfig(DisplayConfig* config) {
    config->driver = GUID_NULL;
    config->autoSelect = 1;
    config->version = 4;
}
