#ifndef GITEN_PLATFORM_IME_H
#define GITEN_PLATFORM_IME_H

#include <rva.h>

#include <Win32.h>

typedef BOOL(WINAPI* WinnlsEnableImeProc)(HWND window, BOOL enable);

// Late-binds USER32's WINNLSEnableIME, which only Far East Windows exports.
// The platform layer is built /Ob0, so the in-class bodies are emitted out of
// line.
class CIme {
public:
    RVA(0x00051100, 0x46)
    CIme() {
        UINT mode = SetErrorMode(SEM_NOOPENFILEERRORBOX);
        m_user32 = LoadLibrary("USER32.DLL");
        SetErrorMode(mode);
        m_enable = NULL;
        if (m_user32 != NULL) {
            m_enable = (WinnlsEnableImeProc)GetProcAddress(m_user32, "WINNLSEnableIME");
        }
    }

    RVA(0x00051150, 0xf)
    ~CIme() {
        if (m_user32 != NULL) {
            FreeLibrary(m_user32);
        }
    }

    RVA(0x00051210, 0x1a)
    BOOL Enable(HWND window, BOOL enable) {
        if (m_enable != NULL) {
            return m_enable(window, enable);
        }
        return FALSE;
    }

private:
    WinnlsEnableImeProc m_enable;
    HMODULE m_user32;
};

extern CIme g_ime;

#endif // GITEN_PLATFORM_IME_H
