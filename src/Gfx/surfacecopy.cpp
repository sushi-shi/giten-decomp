// @identity-TODO: the original owning TU remains unproven.

#include <rva.h>

#include <Platform/GameApi.h>

#include <string.h>

RVA(0x000590a0, 0xd3)
void CopySurfaceSquare(IDirectDrawSurface* dest, IDirectDrawSurface* source, i32 size) {
    HDC destDC;
    HDC sourceDC;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    u16 color;
    dest->GetDC(&destDC);
    source->GetDC(&sourceDC);
    StretchBlt(destDC, 0, 0, size, size, sourceDC, 0, 0, size, size, SRCCOPY);
    dest->ReleaseDC(destDC);
    source->ReleaseDC(sourceDC);
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY;
    dest->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_READONLY | DDLOCK_NOSYSLOCK, NULL);
    color = *static_cast<u16*>(desc.lpSurface);
    dest->Unlock(desc.lpSurface);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = color;
    dest->SetColorKey(DDCKEY_SRCBLT, &key);
}
