// @identity-TODO: the original owning TU remains unproven.

#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/DDError.h>
#include <Gfx/Picture.h>
#include <Gfx/Texture.h>
#include <Platform/Com.h>
#include <Platform/D3DApp.h>
#include <Platform/GameApi.h>

#include <string.h>

RVA(0x000572e0, 0x91)
b32 DrawResourceBitmap(IDirectDrawSurface* surface, u16 bitmap) {
    HRSRC resource = FindResource(NULL, MAKEINTRESOURCE(bitmap), RT_BITMAP);
    BitmapResource* bmp;
    HDC dc;
    if (!resource) {
        return false;
    }
    bmp = static_cast<BitmapResource*>(LockResource(LoadResource(NULL, resource)));
    if (!bmp) {
        return false;
    }
    surface->GetDC(&dc);
    SetStretchBltMode(dc, COLORONCOLOR);
    StretchDIBits(
        dc,
        0,
        0,
        bmp->info.biWidth,
        bmp->info.biHeight,
        0,
        0,
        bmp->info.biWidth,
        bmp->info.biHeight,
        bmp->pixels,
        reinterpret_cast<BITMAPINFO*>(bmp), // API-forced: BITMAPINFO has one palette entry.
        DIB_RGB_COLORS,
        SRCCOPY
    );
    surface->ReleaseDC(dc);
    return true;
}

RVA(0x00057380, 0x191)
b32 CreatePicture(
    Picture* picture,
    i32 width,
    i32 height,
    i32 surfaceWidth,
    i32 surfaceHeight,
    b32 colorKey
) {
    DDSURFACEDESC primaryDesc;
    DDSURFACEDESC desc;
    DDCOLORKEY key;
    HRESULT result;
    if (picture->surface != NULL) {
        return true;
    }
    primaryDesc.dwSize = sizeof(primaryDesc);
    primaryDesc.dwFlags = DDSD_ALL;
    if (g_primarySurface->GetSurfaceDesc(&primaryDesc) != DD_OK) {
        return false;
    }
    memset(picture, 0, sizeof(*picture));
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = surfaceWidth;
    desc.dwHeight = surfaceHeight;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    desc.ddpfPixelFormat = primaryDesc.ddpfPixelFormat;
    if (g_ddraw->CreateSurface(&desc, &picture->surface, NULL) != DD_OK) {
        return false;
    }
    picture->rect.right = width;
    picture->rect.bottom = height;
    picture->rect.left = picture->rect.top = 0;
    picture->surfaceWidth = surfaceWidth;
    picture->surfaceHeight = surfaceHeight;
    picture->id = 0;
    result = picture->surface->GetSurfaceDesc(&desc);
    if (result == DD_OK) {
        if (desc.ddpfPixelFormat.dwRGBBitCount < 16) {
            key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = colorKey;
        } else {
            ZeroMemory(&key, sizeof(key));
        }
        picture->surface->SetColorKey(DDCKEY_SRCBLT, &key);
    } else if (result == DDERR_INVALIDOBJECT) {
        OutputDebugString("GetSurfaceDesc() returns DDERR_INVALIDOBJECT");
    } else if (result == DDERR_INVALIDPARAMS) {
        OutputDebugString("GetSurfaceDesc() returns DDERR_INVALIDPARAMS");
    } else {
        OutputDebugString("GetSurfaceDesc() returns UNKNOWN");
    }
    return true;
}

RVA(0x00057520, 0x62)
b32 LoadPictureFile(Picture* picture, const char* path) {
    BmpFile* bmp = ReadBitmapFile(path);
    if (bmp == NULL) {
        return false;
    }
    LoadBitmapToSurface16(bmp, &picture->surface, NULL);
    picture->rect.left = 0;
    picture->rect.top = 0;
    picture->surfaceWidth = picture->rect.right = bmp->info.biWidth;
    picture->surfaceHeight = picture->rect.bottom = bmp->info.biHeight;
    FreeBitmap(&bmp);
    return true;
}

DATA(0x0006dbe8)
static float s_textureDiffuse[TEXTURE_SHADE_COUNT][3] = {
    {1.0f, 1.0f, 1.0f},
    {0.0f, 0.0f, 0.0f},
    {1.0f, 1.0f, 1.0f},
};

RVA(0x00057590, 0x131)
void ShadeTexture(Texture* texture, TextureShade shade) {
    D3DMATERIAL material;
    HRESULT result;
    if (g_deviceType != D3D_DEVICE_RAMP) {
        return;
    }
    if (texture->materials[shade] != NULL) {
        return;
    }
    result = g_d3d->CreateMaterial(&texture->materials[shade], NULL);
    if (result != D3D_OK) {
        TraceD3DCallError("lpD3D->CreateMaterial() returns ", result);
    }
    result = texture->materials[shade]->GetHandle(g_d3dDevice, &texture->materialHandles[shade]);
    if (result != D3D_OK) {
        TraceD3DCallError("lpTS->material[col]->GetHandle() returns ", result);
    }
    ZeroMemory(&material, sizeof(material));
    material.dwSize = sizeof(material);
    material.dwRampSize = 16;
    material.diffuse.r = s_textureDiffuse[shade][0];
    material.diffuse.g = s_textureDiffuse[shade][1];
    material.diffuse.b = s_textureDiffuse[shade][2];
    material.hTexture = GetTextureHandle(texture);
    material.power = 1.0f;
    if (shade == TEXTURE_SHADE_LIT) {
        material.emissive.r = material.emissive.g = material.emissive.b = 1.0f;
    }
    result = texture->materials[shade]->SetMaterial(&material);
    if (result != D3D_OK) {
        TraceD3DCallError("lpTS->material[col]->SetMaterial() returns ", result);
    }
}

RVA(0x000576d0, 0xc8)
void ReleaseTexture(Texture* texture) {
    i32 shade;
    ReleaseTextureSurfaces(texture);
    if (g_deviceType == D3D_DEVICE_RAMP) {
        for (shade = 0; shade < TEXTURE_SHADE_COUNT; shade++) {
            ReleaseComObject(texture->materials[shade]);
            texture->materialHandles[shade] = 0;
        }
    }
    if (texture->image != NULL) {
        FreeBlock(texture->image);
        texture->image = NULL;
    }
}
