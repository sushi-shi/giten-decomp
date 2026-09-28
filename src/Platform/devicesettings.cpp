#include <rva.h>

#include <Platform/DeviceSettings.h>

#include <string.h>

DATA(0x00084100)
DeviceSettings g_deviceSettings;

RVA(0x00045ce0, 0x7f)
b32 LoadDeviceSettings(DeviceSettings* settings) {
    DWORD type = REG_BINARY;
    DWORD size = sizeof(DeviceSettings);
    HKEY key;
    char name[MAX_PATH];
    BOOL loaded = false;
    if (RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\ASCII\\GITEN_DDS", 0, KEY_QUERY_VALUE, &key)
        == ERROR_SUCCESS) {
        GetDeviceSettingsValueName(name);
        // API-forced: RegQueryValueEx writes the complete binary registry record.
        if (RegQueryValueEx(key, name, NULL, &type, reinterpret_cast<BYTE*>(settings), &size)
            == ERROR_SUCCESS) {
            loaded = true;
        }
        RegCloseKey(key);
    }
    return loaded;
}

RVA(0x00045d60, 0x29)
void __fastcall GetDeviceSettingsValueName(char* name) {
    strcpy(name, "DevConfig");
}
