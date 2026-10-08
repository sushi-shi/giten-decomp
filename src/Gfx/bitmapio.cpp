// @identity-TODO: the original filename and final object boundary are unproven.
// The display routines reproduce one ordinary initialized-data contribution.

#include <rva.h>

#include <EnumDomain.h>
#include <Game/AbortFlag.h>
#include <Game/AreaLevel.h>
#include <Game/AreaNpc.h>
#include <Game/MapArea.h>
#include <Game/ViewDirection.h>
#include <Gfx/DDError.h>
#include <Gfx/Texture.h>
#include <Platform/Com.h>
#include <Platform/D3DApp.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>

#include <io.h>
#include <new.h>
#include <stdio.h>
#include <string.h>

RVA(0x00056a70, 0x70)
BmpFile* ReadBitmapFile(const char* path) {
    FILE* file = fopen(path, "rb");
    u32 size;
    BmpFile* bmp;
    if (file == NULL) {
        return NULL;
    }
    size = _filelength(_fileno(file));
    bmp = static_cast<BmpFile*>(operator new(size));
    if (fread(bmp, 1, size, file) != size) {
        operator delete(bmp);
        fclose(file);
        return NULL;
    }
    fclose(file);
    return bmp;
}

RVA(0x00056ae0, 0x1c)
void FreeBitmap(BmpFile** bmp) {
    if (*bmp != NULL) {
        operator delete(*bmp);
    }
    *bmp = NULL;
}

// Positions `fp` at the start of bitmap `skip` in a file of concatenated BMPs
// (`limit` bytes long) and reports that bitmap's size; NULL when the chain is
// broken.
RVA(0x00056b00, 0xc1)
FILE* SeekBitmap(FILE* fp, i16 skip, i32 limit, u32* size) {
    BITMAPFILEHEADER header;
    if (fread(&header, sizeof(header), 1, fp) < 1) {
        return NULL;
    }
    if (!HasBitmapFileSignature(&header)) {
        return NULL;
    }
    while (skip > 0) {
        if (fseek(fp, header.bfSize - sizeof(header), SEEK_CUR) != 0) {
            return NULL;
        }
        limit -= header.bfSize;
        if (limit <= 0) {
            return NULL;
        }
        if (fread(&header, sizeof(header), 1, fp) < 1) {
            return NULL;
        }
        if (!HasBitmapFileSignature(&header)) {
            return NULL;
        }
        skip--;
    }
    if (fseek(fp, -static_cast<i32>(sizeof(header)), SEEK_CUR) != 0) {
        return NULL;
    }
    *size = header.bfSize;
    return fp;
}

// Converts the 8-bit bitmap into the locked 16-bit surface through the
// surface pixel format; palette entry 1 becomes the source colour key.
RVA(0x00056bd0, 0x20a)
b32 LoadBitmapToSurface16(BmpFile* bmp, IDirectDrawSurface** surface, DWORD* colorKey) {
    DDSURFACEDESC desc;
    u16 colors[256];
    DDCOLORKEY key;
    u8* src;
    u8* dst;
    u8* p;
    RGBQUAD* palette;
    DWORD count;
    DWORD i;
    i32 row;
    i32 col;
    DWORD value;

    desc.dwSize = sizeof(desc);
    if (IDirectDrawSurface_Lock(
            *surface,
            NULL,
            &desc,
            DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
            NULL
        )
        != DD_OK) {
        return false;
    }
    dst = static_cast<u8*>(desc.lpSurface);
    src = GetBitmapPixels(bmp) + (bmp->info.biHeight - 1) * bmp->info.biWidth;
    memset(colors, 0, sizeof(colors));
    palette = bmp->colors;
    count = bmp->info.biClrUsed ? bmp->info.biClrUsed : 256;
    for (i = 0; i < count; i++) {
        colors[i] = PackSurfaceColor(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
    }
    for (row = 0; row < bmp->info.biHeight; row++) {
        p = src;
        for (col = 0; col < bmp->info.biWidth * 2; col += 2) {
            WriteSurfacePixel16(dst, col, colors[*p]);
            p++;
        }
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    IDirectDrawSurface_Unlock(*surface, NULL);
    // The surface key uses RRGGBB rather than COLORREF channel order.
    value =
        RGB(palette[BMP_TRANSPARENT_INDEX].rgbBlue & 0x1f,
            palette[BMP_TRANSPARENT_INDEX].rgbGreen & 0x1f,
            palette[BMP_TRANSPARENT_INDEX].rgbRed & 0x1f);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = value;
    IDirectDrawSurface_SetColorKey(*surface, DDCKEY_SRCBLT, &key);
    if (colorKey != NULL) {
        *colorKey = value;
    }
    return true;
}

// Loads the 8-bit bitmap into the surface with its own palette; colour 1 is
// the source colour key.
RVA(0x00056de0, 0x141)
b32 LoadBitmapToSurface8(
    BmpFile* bmp,
    IDirectDrawSurface** surface,
    PALETTEENTRY* entries,
    IDirectDrawPalette** palette
) {
    DDSURFACEDESC desc;
    HRESULT result;
    DDCOLORKEY key;
    u8* src;
    u8* dst;
    DWORD row;
    // API-forced: RGBQUAD and PALETTEENTRY have different packed channel byte orders.
    ConvertBitmapPalette(reinterpret_cast<DWORD*>(bmp->colors), reinterpret_cast<DWORD*>(entries));
    if (IDirectDraw_CreatePalette(g_ddraw, DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, palette, NULL)
        != DD_OK) {
        return false;
    }
    result = IDirectDrawSurface_SetPalette(*surface, *palette);
    if (result != DD_OK) {
        return false;
    }
    desc.dwSize = sizeof(desc);
    if (IDirectDrawSurface_Lock(
            *surface,
            NULL,
            &desc,
            DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK,
            NULL
        )
        != DD_OK) {
        return false;
    }
    dst = static_cast<u8*>(desc.lpSurface);
    src = GetBitmapPixels(bmp) + (desc.dwHeight - 1) * desc.dwWidth;
    for (row = 0; row < desc.dwHeight; row++) {
        memcpy(dst, src, desc.dwWidth);
        dst += desc.lPitch;
        src -= desc.dwWidth;
    }
    IDirectDrawSurface_Unlock(*surface, NULL);
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = BMP_TRANSPARENT_INDEX;
    IDirectDrawSurface_SetColorKey(*surface, DDCKEY_SRCBLT, &key);
    return true;
}

RVA(0x00056f30, 0x1e7)
b32 CopyResourceBitmap16(BitmapResource* bmp, IDirectDrawSurface** surface, i32 x, i32 y) {
    DDSURFACEDESC desc;
    u16 colors[256];
    DDCOLORKEY key;
    RGBQUAD* palette;
    DWORD count;
    DWORD i;
    i32 row;
    i32 col;
    u8* src;
    u8* dst;
    u8* pixels;
    desc.dwSize = sizeof(desc);
    if ((*surface)->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK, NULL)
        != DD_OK) {
        return false;
    }
    memset(colors, 0, sizeof(colors));
    palette = bmp->colors;
    count = bmp->info.biClrUsed;
    if (count == 0) {
        count = 256;
    }
    for (i = 0; i < count; i++) {
        colors[i] = PackSurfaceColor(palette[i].rgbRed, palette[i].rgbGreen, palette[i].rgbBlue);
    }
    src = GetResourceBitmapLastRow(bmp);
    dst = static_cast<u8*>(desc.lpSurface) + desc.lPitch * y + x * 2;
    for (row = 0; row < bmp->info.biHeight; row++) {
        pixels = src;
        for (col = 0; col < bmp->info.biWidth * 2; col += 2) {
            WriteSurfacePixel16(dst, col, colors[*pixels]);
            pixels++;
        }
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    (*surface)->Unlock(NULL);
    ZeroMemory(&key, sizeof(key));
    (*surface)->SetColorKey(DDCKEY_SRCBLT, &key);
    return true;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x00057120, 0x162)
b32 CopyResourceBitmap8(
    BitmapResource* bmp,
    IDirectDrawSurface** surface,
    PALETTEENTRY* entries,
    IDirectDrawPalette** palette,
    i32 x,
    i32 y
) {
    DDSURFACEDESC desc;
    HRESULT result;
    DDCOLORKEY key;
    i32 row;
    u8* src;
    u8* dst;
    // API-forced: RGBQUAD and PALETTEENTRY have different packed channel byte orders.
    ConvertBitmapPalette(reinterpret_cast<DWORD*>(bmp->colors), reinterpret_cast<DWORD*>(entries));
    if (g_ddraw->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, entries, palette, NULL) != DD_OK) {
        return false;
    }
    result = (*surface)->SetPalette(*palette);
    if (result != DD_OK) {
        return false;
    }
    desc.dwSize = sizeof(desc);
    if ((*surface)->Lock(NULL, &desc, DDLOCK_WAIT | DDLOCK_WRITEONLY | DDLOCK_NOSYSLOCK, NULL)
        != DD_OK) {
        return false;
    }
    src = GetResourceBitmapLastRow(bmp);
    dst = static_cast<u8*>(desc.lpSurface) + desc.lPitch * y + x;
    for (row = 0; row < bmp->info.biHeight; row++) {
        memcpy(dst, src, bmp->info.biWidth);
        dst += desc.lPitch;
        src -= bmp->info.biWidth;
    }
    (*surface)->Unlock(NULL);
    ZeroMemory(&key, sizeof(key));
    (*surface)->SetColorKey(DDCKEY_SRCBLT, &key);
    return true;
}

RVA(0x00057290, 0x4c)
b32 BlitImage(LPDIRECTDRAWSURFACE surface, u16 image, i32 x, i32 y) {
    HRSRC resource = FindResource(NULL, MAKEINTRESOURCE(image), RT_BITMAP);
    BitmapResource* bmp;
    if (resource == NULL) {
        return false;
    }
    bmp = static_cast<BitmapResource*>(LockResource(LoadResource(NULL, resource)));
    if (bmp == NULL) {
        return false;
    }
    CopyResourceBitmap16(bmp, &surface, x, y);
    return true;
}

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
    InitOffscreenSurfaceDesc(desc, surfaceWidth, surfaceHeight, primaryDesc.ddpfPixelFormat);
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
        if (IsPalettizedSurface(desc)) {
            key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = colorKey;
        } else {
            ZeroMemory(&key, sizeof(key));
        }
        picture->surface->SetColorKey(DDCKEY_SRCBLT, &key);
    } else if (result == DDERR_INVALIDOBJECT) {
        OutputDebugString("GetSurfaceDesc() returns DDERR_INVALIDOBJECT\n");
    } else if (result == DDERR_INVALIDPARAMS) {
        OutputDebugString("GetSurfaceDesc() returns DDERR_INVALIDPARAMS\n");
    } else {
        OutputDebugString("GetSurfaceDesc() returns UNKNOWN\n");
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

// The largest texture loaded; bigger bitmaps are cropped.
#define TEXTURE_MAX_WIDTH 512
#define TEXTURE_MAX_HEIGHT 256

// Loads texture `texture` from bitmap `name`: a system-memory surface gets the
// bitmap (8-bit through a palette, else 16-bit with its colour key), a device
// surface of the same shape loads it, and the ramp device gets its shades.
// With `fromFile` the bitmap was read from a file and is freed here. Returns
// FALSE on any failure.
RVA(0x000577a0, 0x2c2)
b32 LoadTexture(Texture* texture, const char* name, b32 fromFile) {
    BmpFile* bmp;
    BITMAPINFOHEADER* header;
    DDSURFACEDESC desc;
    DDSURFACEDESC sourceDesc;
    HRESULT result;
    IDirect3DTexture2** source;

    bmp = OpenTextureBitmap(texture, name, fromFile);
    if (bmp == NULL) {
        return false;
    }
    header = &bmp->info;
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = min(TEXTURE_MAX_WIDTH, header->biWidth);
    desc.dwHeight = min(TEXTURE_MAX_HEIGHT, header->biHeight);
    desc.ddpfPixelFormat = g_textureFormat;
    desc.ddsCaps.dwCaps = g_deviceType != D3D_DEVICE_HAL
                              ? DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY | DDSCAPS_ALLOCONLOAD
                              : DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY | DDSCAPS_ALLOCONLOAD;
    sourceDesc = desc;
    sourceDesc.ddsCaps.dwCaps = DDSCAPS_TEXTURE | DDSCAPS_SYSTEMMEMORY;
    // Preserve the interface output address across surface creation.
    source = &texture->source;
    if (IDirectDraw_CreateSurface(g_ddraw, &sourceDesc, &texture->sourceSurface, NULL) != DD_OK) {
        if (fromFile == true) {
            FreeBitmap(&bmp);
        }
        return false;
    }
    IDirectDrawSurface_QueryInterface(
        texture->sourceSurface,
        IID_IDirect3DTexture2,
        reinterpret_cast<void**>(source) // API-forced: QueryInterface takes void**.
    );
    texture->width = min(TEXTURE_MAX_WIDTH, header->biWidth);
    if (IsPalettizedSurface(desc)) {
        if (!LoadBitmapToSurface8(
                bmp,
                &texture->sourceSurface,
                texture->palette,
                &texture->ddPalette
            )) {
            return false;
        }
        texture->colorKey = BMP_TRANSPARENT_INDEX;
    } else if (!LoadBitmapToSurface16(bmp, &texture->sourceSurface, &texture->colorKey)) {
        return false;
    }
    result = IDirectDraw_CreateSurface(g_ddraw, &desc, &texture->surface, NULL);
    if (result != DD_OK) {
        if (fromFile == true) {
            FreeBitmap(&bmp);
        }
        return false;
    }
    if (IsPalettizedSurface(desc)
        && IDirectDrawSurface_SetPalette(texture->surface, texture->ddPalette) != DD_OK) {
        if (fromFile == true) {
            FreeBitmap(&bmp);
        }
        return false;
    }
    if (fromFile == true) {
        FreeBitmap(&bmp);
    }
    if (IDirectDrawSurface_QueryInterface(
            texture->surface,
            IID_IDirect3DTexture2,
            reinterpret_cast<void**>(&texture->texture) // API-forced: QueryInterface takes void**.
        )
        != DD_OK) {
        return false;
    }
    if (IDirect3DTexture2_Load(texture->texture, texture->source) != D3D_OK) {
        return false;
    }
    if (IDirect3DTexture2_GetHandle(texture->texture, g_d3dDevice, &texture->handle) != D3D_OK) {
        return false;
    }
    ShadeTexture(texture, TEXTURE_SHADE_NORMAL);
    ShadeTexture(texture, TEXTURE_SHADE_LIT);
    return true;
}

RVA(0x00057a70, 0xb1)
BmpFile* OpenTextureBitmap(Texture* texture, const char* name, b32 fromFile) {
    BmpFile* bmp;
    u8* pixels;
    i32 row;
    i32 col;
    if (HasTextureHandle(texture)) {
        return NULL;
    }
    if (fromFile == true) {
        bmp = ReadBitmapFile(name);
        if (bmp == NULL) {
            return NULL;
        }
    } else {
        // API-forced: borrowed BMP input when the argument is not a path.
        bmp = reinterpret_cast<BmpFile*>(const_cast<char*>(name));
    }
    texture->image = static_cast<BmpFile*>(AllocClearedLong(1, bmp->file.bfSize));
    if (texture->image == NULL) {
        return NULL;
    }
    memcpy(texture->image, bmp, bmp->file.bfSize);
    pixels = GetBitmapPixels(bmp);
    texture->bottomMargin = 0;
    for (row = 0; row < bmp->info.biHeight; row++) {
        for (col = 0; col < bmp->info.biWidth; pixels++, col++) {
            if (*pixels != BMP_TRANSPARENT_INDEX) {
                texture->bottomMargin = row;
                row = bmp->info.biHeight;
                break;
            }
        }
    }
    return bmp;
}

DATA(0x0006dc10)
static WorldMapBlitRegion s_worldMapRegions[6] = {
    {0, 0, 112, 36, 176, 164},
    {176, 0, 0, 36, 288, 164},
    {464, 0, 0, 36, 176, 164},
    {0, 164, 112, 0, 176, 164},
    {176, 164, 0, 0, 288, 164},
    {464, 164, 0, 0, 176, 164},
};

// Reloads a texture whose device surface was lost.
RVA(0x00057b30, 0x41)
void RestoreTexture(Texture* texture) {
    if (texture != NULL && texture->surface != NULL
        && IDirectDrawSurface_IsLost(texture->surface) == DDERR_SURFACELOST) {
        IDirectDrawSurface_Restore(texture->surface);
        IDirect3DTexture2_Load(texture->texture, texture->source);
    }
}

RVA(0x00057b80, 0xb)
void ShowScenePicture(void) {
    g_scenePicture.visible = true;
}

RVA(0x00057b90, 0x59)
void LoadScenePicture(BmpFile* bmp, u16 id) {
    if (bmp != NULL) {
        LoadBitmapToSurface16(bmp, &g_scenePicture.surface, NULL);
        g_scenePicture.id = id;
        GetBitmapRect(g_scenePicture.rect, bmp);
        g_scenePicture.visible = true;
    }
}

RVA(0x00057bf0, 0x4b)
void ClearSceneSurfaces(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    ClearDisplaySurface(g_viewCachePicture.surface, NULL);
    GetTextPlane(0)->visible = false;
    g_scenePicture.visible = false;
}

RVA(0x00057c40, 0x30)
void ClearScenePicture(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    GetTextPlane(0)->visible = false;
    g_scenePicture.visible = false;
}

static __inline void GetWorldMapBlitRects(i16 slot, RECT* dest, RECT* source) {
    dest->left = s_worldMapRegions[slot].destX;
    dest->top = s_worldMapRegions[slot].destY;
    dest->right = s_worldMapRegions[slot].destX + s_worldMapRegions[slot].width;
    dest->bottom = s_worldMapRegions[slot].destY + s_worldMapRegions[slot].height;
    source->left = s_worldMapRegions[slot].sourceX;
    source->top = s_worldMapRegions[slot].sourceY;
    source->right = s_worldMapRegions[slot].sourceX + s_worldMapRegions[slot].width;
    source->bottom = s_worldMapRegions[slot].sourceY + s_worldMapRegions[slot].height;
}

RVA(0x00057c70, 0x106)
void LoadWorldMapTile(BmpFile* bmp, i16 slot) {
    if (bmp != NULL && slot <= 5) {
        Picture tile;
        RECT source;
        RECT dest;
        memset(&tile, 0, sizeof(tile));
        CreatePicture(
            &tile,
            bmp->info.biWidth,
            bmp->info.biHeight,
            bmp->info.biWidth,
            bmp->info.biHeight,
            true
        );
        LoadBitmapToSurface16(bmp, &tile.surface, NULL);
        GetWorldMapBlitRects(slot, &dest, &source);
        IDirectDrawSurface_Blt(
            g_scenePicture.surface,
            &dest,
            tile.surface,
            &source,
            DDBLT_WAIT,
            NULL
        );
        g_scenePicture.rect.left = 0;
        g_scenePicture.rect.top = 0;
        g_scenePicture.rect.right = 640;
        g_scenePicture.rect.bottom = 328;
        g_scenePicture.visible = false;
    }
}

RVA(0x00057d80, 0xfc)
void LoadWorldMapOverlay(BmpFile* bmp, i16 slot) {
    if (bmp != NULL && slot <= 5) {
        Picture tile;
        RECT source;
        RECT dest;
        memset(&tile, 0, sizeof(tile));
        CreatePicture(
            &tile,
            bmp->info.biWidth,
            bmp->info.biHeight,
            bmp->info.biWidth,
            bmp->info.biHeight,
            true
        );
        LoadBitmapToSurface16(bmp, &tile.surface, NULL);
        GetWorldMapBlitRects(slot, &dest, &source);
        IDirectDrawSurface_Blt(
            g_viewCachePicture.surface,
            &dest,
            tile.surface,
            &source,
            DDBLT_WAIT | DDBLT_KEYSRC,
            NULL
        );
        g_viewCachePicture.rect.left = 0;
        g_viewCachePicture.rect.top = 0;
        g_viewCachePicture.rect.right = 640;
        g_viewCachePicture.rect.bottom = 328;
    }
}

RVA(0x00057e80, 0x94)
void LoadWallTextures(i16 wallSet, i16 variant) {
    DATA(0x0006ddc8)
    static const char* s_wallTextureNames[16][4] = {
        {"w\\wall00_0.bmp", "w\\wall00_1.bmp", "w\\wall00_2.bmp", "w\\wall00_3.bmp"},
        {"w\\wall01_0.bmp", "w\\wall01_0.bmp", "w\\wall01_0.bmp", "w\\wall01_0.bmp"},
        {"w\\wall02_0.bmp", "w\\wall02_1.bmp", "w\\wall02_2.bmp", "w\\wall02_0.bmp"},
        {"w\\wall03_0.bmp", "w\\wall03_1.bmp", "w\\wall03_0.bmp", "w\\wall03_1.bmp"},
        {"w\\wall04_0.bmp", "w\\wall04_1.bmp", "w\\wall04_2.bmp", "w\\wall04_0.bmp"},
        {"w\\wall05_0.bmp", "w\\wall05_0.bmp", "w\\wall05_0.bmp", "w\\wall05_0.bmp"},
        {"w\\wall06_0.bmp", "w\\wall06_1.bmp", "w\\wall06_2.bmp", "w\\wall06_3.bmp"},
        {"w\\wall07_0.bmp", "w\\wall07_0.bmp", "w\\wall07_0.bmp", "w\\wall07_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall10_0.bmp", "w\\wall10_0.bmp", "w\\wall10_0.bmp", "w\\wall10_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
        {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    };
    GZ_ENUM_LOCAL(MapAreaId, u8) area;
    u8 level;
    ReleaseTexture(&g_roomTexture);
    g_fixedLighting = false;
    if ((wallSet & 0xf) == WALL_TEXTURE_UNLIT) {
        g_fixedLighting = true;
    }
    if (wallSet == WALL_TEXTURE_MAP_OVERRIDE) {
        area = GetMapArea();
        level = GetMapLevel();
        if ((area == MAP_AREA_CHIYODA_LINE && level > 1)
            || (area == MAP_AREA_HIBIYA_LINE && level < 4)) {
            LoadTexture(&g_roomTexture, "w\\wall11_0.bmp", true);
            return;
        }
    }
    LoadTexture(&g_roomTexture, s_wallTextureNames[wallSet & 0xf][variant & 3], true);
}

RVA(0x00057f20, 0x112)
b16 DecodeLayerImage(BmpFile* data, i16 layer, i32 size) {
    b32 loaded = true;
    BmpFile* bmp = data;
    i32 frame;
    u32 consumed;
    u32 bitmapSize;
    if (data == NULL) {
        return loaded;
    }
    for (frame = 0; frame < ENEMY_TEXTURE_FRAMES; frame++) {
        ReleaseTexture(&g_enemyTextures[layer][frame]);
    }
    consumed = 0;
    for (frame = 0; frame < ENEMY_TEXTURE_FRAMES; frame++) {
        if (consumed > size) {
            break;
        }
        bitmapSize = bmp->file.bfSize;
        if (!LoadTexture(
                &g_enemyTextures[layer][frame],
                // API-forced: borrowed BMP input, selected by fromFile = FALSE.
                reinterpret_cast<const char*>(bmp),
                false
            )) { // API-forced: borrowed BMP input.
            loaded = false;
        }
        // Byte-forced: packed complete BMP files advance by bfSize.
        bmp = reinterpret_cast<BmpFile*>(
            reinterpret_cast<u8*>(bmp) + bitmapSize
        ); // Byte-forced: packed BMPs.
        consumed += bitmapSize;
    }
    if (!loaded) {
        for (frame = 0; frame < ENEMY_TEXTURE_FRAMES; frame++) {
            Texture* texture = &g_enemyTextures[layer][frame];
            ReleaseTextureSurfaces(texture);
        }
    }
    return loaded;
}

RVA(0x00058040, 0xc4)
void DecodeLayerImageAlt(BmpFile* data, i16 layer, i32 size) {
    BmpFile* bmp;
    i32 frame;
    u32 consumed;
    u32 bitmapSize;
    if (g_enemyPictures[0].id != 0) {
        return;
    }
    bmp = data;
    if (bmp == NULL) {
        return;
    }
    for (frame = 0; frame < ENEMY_TEXTURE_FRAMES; frame++) {
        ReleaseTexture(&g_enemyTextures[layer][frame]);
    }
    consumed = 0;
    for (frame = 0; frame < ENEMY_TEXTURE_FRAMES; frame++) {
        if (consumed > size) {
            break;
        }
        bitmapSize = bmp->file.bfSize;
        OpenTextureBitmap(
            &g_enemyTextures[layer][frame],
            // API-forced: borrowed BMP input, selected by fromFile = FALSE.
            reinterpret_cast<const char*>(bmp),
            false
        ); // API-forced: borrowed BMP input.
        // Byte-forced: packed complete BMP files advance by bfSize.
        bmp = reinterpret_cast<BmpFile*>(
            reinterpret_cast<u8*>(bmp) + bitmapSize
        ); // Byte-forced: packed BMPs.
        consumed += bitmapSize;
    }
    consumed = 0;
    for (frame = 0; frame < 6; frame++) {
        if (consumed > size) {
            break;
        }
        bmp = data;
        bitmapSize = bmp->file.bfSize;
        LoadBitmapToSurface16(bmp, &g_enemyPictures[frame].surface, NULL);
        // Byte-forced: each packed BMP starts after the preceding bfSize bytes.
        data = reinterpret_cast<BmpFile*>(reinterpret_cast<u8*>(data) + bitmapSize);
        consumed += bitmapSize;
    }
}

RVA(0x00058110, 0x77)
void LoadObjectTexture(void* image, i16 slot) {
    if (slot < 0 || slot > OBJECT_TEXTURE_COUNT - 1) {
        return;
    }
    if (image == NULL) {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(&g_objectTextures[slot], "w\\npc.bmp", true);
    } else {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(
            &g_objectTextures[slot],
            // API-forced: borrowed BMP input selected by fromFile = false.
            static_cast<const char*>(image),
            false
        );
    }
}

RVA(0x00058190, 0x1f)
void ReleaseObjectTextures(void) {
    i32 slot;
    for (slot = 0; slot < OBJECT_TEXTURE_COUNT; slot++) {
        ReleaseTexture(&g_objectTextures[slot]);
    }
}

DATA(0x0006dca0)
static RECT s_cursorPreviewRect = {8, 8, 119, 119};

RVA(0x000581b0, 0x10c)
i16 HitTestWorldMap(i16 x, i16 y, i16 layer) {
    i32 index = LayerIndexAtPoint(x, y);
    u16 color;
    i32 result;
    if (index >= 0 && g_layerStack[index]->slot != SCREEN_LAYER_AUTOMAP) {
        return WORLD_MAP_HIT_NONE;
    }
    result = WORLD_MAP_HIT_NONE;
    if (index < 0) {
        if (layer & 1) {
            color = ReadSurfaceWord(g_viewCachePicture.surface, x, y, 640);
        } else {
            color = ReadSurfaceWord(g_scenePicture.surface, x, y, 640);
        }
    } else {
        x -= g_layerStack[index]->x;
        y -= g_layerStack[index]->y;
        result = WORLD_MAP_HIT_AUTOMAP_LAYER;
        color = ReadSurfaceWord(g_layerStack[index]->canvas, x, y, 128);
    }
    if (color == g_markerColors[0].color) {
        result |= WORLD_MAP_HIT_MARKER_COLOR;
    } else if (color == g_markerColors[2].color) {
        result |= WORLD_MAP_HIT_MARKER_COLOR;
    } else if (color == g_markerColors[3].color) {
        result |= WORLD_MAP_HIT_MARKER_COLOR;
    } else if (color == g_markerColors[1].color) {
        result |= WORLD_MAP_HIT_MARKER_COLOR;
    } else {
        result = WORLD_MAP_HIT_NONE;
    }
    return result;
}

// The incoming position is replaced with the current party marker.
RVA(0x000582c0, 0x64)
b16 IsWorldMapMarkerNearEdge(i16 x, i16 y) {
    i16 slot = GetWorldMapMarker(&x, &y);
    if (slot >= 0 && slot < MAP_SCREEN_COUNT) {
        x += g_mapScreenOffsets[slot].x;
        y += g_mapScreenOffsets[slot].y;
        if (x < 16 || x > 624) {
            return true;
        }
        if (y < 16 || y > 312) {
            return true;
        }
    }
    return false;
}

// @identity-TODO: the returned map codes are known by their marker colours.
RVA(0x00058330, 0xbd)
u8 ReadWorldMapTileCode(i16 x, i16 y, i16 slot, i16 layer) {
    u16 color;
    if (slot < 0) {
        return 0;
    }
    if (slot > 5) {
        return 0;
    }
    x += g_mapScreenOffsets[slot].x;
    y += g_mapScreenOffsets[slot].y;
    if (x < 0) {
        x = 0;
    } else if (x >= MAP_MARKER_MAX_X) {
        x = MAP_MARKER_MAX_X;
    }
    if (y < 0) {
        y = 0;
    } else if (y >= MAP_MARKER_MAX_Y) {
        y = MAP_MARKER_MAX_Y;
    }
    if (layer & 1) {
        color = ReadSurfaceWord(g_viewCachePicture.surface, x, y, 640);
    } else {
        color = ReadSurfaceWord(g_scenePicture.surface, x, y, 640);
    }
    if (color == g_markerColors[0].color) {
        return 15;
    }
    if (color == g_markerColors[2].color) {
        return 4;
    }
    if (color == g_markerColors[3].color) {
        return 5;
    }
    if (color == g_markerColors[1].color) {
        return 2;
    }
    return 0;
}

RVA(0x000583f0, 0x10c)
void DrawWorldMapCursor(i16 x, i16 y, i16 layer) {
    RECT source;
    if (LayerIndexAtPoint(x, y) >= 0) {
        return;
    }
    source.left = x - 7;
    source.top = y - 7;
    source.right = source.left + 13;
    source.bottom = source.top + 13;
    if (source.left < 0) {
        source.left = 0;
        source.right = 13;
    } else if (source.right >= 640) {
        source.right = 639;
        source.left = 626;
    }
    if (source.top < 0) {
        source.top = 0;
        source.bottom = 13;
    } else if (source.bottom >= 328) {
        source.bottom = 327;
        source.top = 314;
    }
    g_screenLayers[SCREEN_LAYER_AUTOMAP]
        ->canvas->Blt(NULL, NULL, NULL, DDBLT_COLORFILL, &g_clearBltFx);
    if (layer & 1) {
        g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas->Blt(
            &s_cursorPreviewRect,
            g_viewCachePicture.surface,
            &source,
            DDBLT_KEYSRC,
            NULL
        );
    } else {
        g_screenLayers[SCREEN_LAYER_AUTOMAP]->canvas->Blt(
            &s_cursorPreviewRect,
            g_scenePicture.surface,
            &source,
            DDBLT_KEYSRC,
            NULL
        );
    }
}

// Converts a screen point to an automap cell offset from the map centre
// (8-pixel cells, 14 x 14 cells).
RVA(0x00058500, 0x7c)
void ScreenToAutomapCell(i16* x, i16* y) {
    *x += -8 - g_screenLayers[SCREEN_LAYER_AUTOMAP]->x;
    *y += -8 - g_screenLayers[SCREEN_LAYER_AUTOMAP]->y;
    if (*x < 0) {
        *x = 0;
    }
    if (*x >= 0x70) {
        *x = 0x6f;
    }
    if (*y < 0) {
        *y = 0;
    }
    if (*y >= 0x70) {
        *y = 0x6f;
    }
    *x = *x / 8 - 7;
    *y = *y / 8 - 7;
}

RVA(0x00058580, 0xb)
void ClearSelectedHotspot(void) {
    g_selectedHotspot = HOTSPOT_NONE;
}

RVA(0x00058590, 0xa)
void SetSelectedHotspot(i32 index) {
    g_selectedHotspot = index;
}

RVA(0x000585a0, 0x1a)
i16 GetSelectedHotspotValue(void) {
    if (g_selectedHotspot < 0) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    return GetHotspot(g_selectedHotspot)->value;
}

RVA(0x000585c0, 0x2a)
i16 GetSelectedHotspotObject(void) {
    if (g_selectedHotspot < 0) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    if (g_screenLayers[SCREEN_LAYER_PANEL]->visible) {
        return FIELD_OBJECT_INDEX_NONE;
    }
    return GetHotspot(g_selectedHotspot)->value;
}

// Whether a kind-1 hotspot leads to (x, y).
// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it
// (`giten sema xref --tree`); retail keeps it because the link had no /OPT:REF.
RVA(0x000585f0, 0x48)
b16 HasHotspotTo(i16 x, i16 y) {
    u32 i;
    for (i = 0; i < g_hotspotCount; i++) {
        if (GetHotspot(i)->kind == HOTSPOT_TARGET && GetHotspot(i)->targetX == x
            && GetHotspot(i)->targetY == y) {
            return true;
        }
    }
    return false;
}

RVA(0x00058640, 0x90)
i16 CountFieldObjects(void) {
    char buffer[128];
    i16 count = IsAbortPending();
    if (count) {
        SetAbortPending(false);
        count = -1;
    } else if (g_renderMode == RENDER_MODE_VIEW) {
        u32 i;
        sprintf(buffer, "\nobjcnt = %d\n", g_hotspotCount);
        OutputDebugString(buffer);
        for (i = 0; i < g_hotspotCount; i++) {
            if (GetHotspot(i)->kind == HOTSPOT_TARGET) {
                count++;
            }
        }
    } else {
        count = CountActiveObjects();
    }
    return count;
}

RVA(0x000586d0, 0x27)
void UnplaceAllSprites(void) {
    i32 slot;
    for (slot = 0; slot < SPRITE_SLOT_COUNT; slot++) {
        g_spriteOrder[slot] = SPRITE_UNPLACED;
        SetSpriteSlotFrame(GetSpriteSlot(slot), SPRITE_UNPLACED);
    }
}

RVA(0x00058700, 0x7a)
void UnplaceSprite(i16 slot) {
    i16 order;
    if (slot < 0 && slot > SPRITE_SLOT_COUNT - 1) {
        return;
    }
    GetSpriteSlot(slot)->group = 0;
    SetSpriteSlotFrame(GetSpriteSlot(slot), SPRITE_UNPLACED);
    GetSpriteSlot(slot)->x = 0;
    GetSpriteSlot(slot)->y = 0;
    for (order = 0; order < SPRITE_SLOT_COUNT; order++) {
        if (g_spriteOrder[order] == slot) {
            // The byte count is deliberately not scaled by the element size.
            memmove(
                &g_spriteOrder[order],
                &g_spriteOrder[order + 1],
                SPRITE_SLOT_COUNT - 1 - order
            );
            g_spriteOrder[SPRITE_SLOT_COUNT - 1] = SPRITE_UNPLACED;
            return;
        }
    }
}

RVA(0x00058780, 0x71)
void PlaceSprite(i16 id, i16 slot, i16 frame, i16 x, i16 y) {
    i16 order;
    if (slot < 0 || slot > SPRITE_SLOT_COUNT - 1) {
        return;
    }
    GetSpriteSlot(slot)->group = id;
    SetSpriteSlotFrame(GetSpriteSlot(slot), frame);
    GetSpriteSlot(slot)->x = x * 8;
    GetSpriteSlot(slot)->y = y;
    for (order = 0; order < SPRITE_SLOT_COUNT; order++) {
        if (g_spriteOrder[order] == SPRITE_UNPLACED) {
            g_spriteOrder[order] = slot;
            return;
        }
    }
}

RVA(0x00058800, 0x17)
b16 IsSpritePlaced(i16 slot) {
    return GetSpriteSlotFrame(GetSpriteSlot(slot)) != SPRITE_UNPLACED;
}

RVA(0x00058820, 0x44)
void FreeSpriteImages(i16 slot) {
    i32 frame;
    if (slot < 0 || slot > SPRITE_GROUP_COUNT - 1) {
        return;
    }
    for (frame = 0; frame < SPRITE_FRAME_COUNT; frame++) {
        ReleaseComObject(GetSpriteFramePicture(slot, frame)->surface);
        GetSpriteFramePicture(slot, frame)->visible = false;
    }
}

RVA(0x00058870, 0x50)
b16 IsSpriteFrameLoaded(i16 slot, i16 frame) {
    if (IsSpriteFrameIndexOutOfRange(slot, frame)) {
        return false;
    }
    if (GetSpriteFramePicture(slot, frame)->surface == NULL) {
        return false;
    }
    return GetSpriteFramePicture(slot, frame)->visible != false;
}

RVA(0x000588c0, 0xd0)
void LoadSpriteFrames(BmpFile* data, i16 slot, i16 image, i32 size) {
    BmpFile* bmp;
    i32 frame;
    i32 width;
    i32 height;
    i32 displayWidth;
    i32 displayHeight;
    if (slot < 0 || slot > SPRITE_GROUP_COUNT - 1 || data == NULL) {
        return;
    }
    bmp = data;
    for (frame = 0; frame < SPRITE_FRAME_COUNT; frame++) {
        width = bmp->info.biWidth;
        height = bmp->info.biHeight;
        displayWidth = width;
        displayHeight = height;
        if (image >= 0x2000 && image < 0x3000 && (image & 0xf) == SPRITE_IMAGE_HALF_SIZE) {
            displayWidth /= 2;
            displayHeight /= 2;
        }
        if (!CreatePicture(
                GetSpriteFramePicture(slot, frame),
                displayWidth,
                displayHeight,
                width,
                height,
                false
            )) {
            return;
        }
        GetSpriteFramePicture(slot, frame)->id = image;
        LoadBitmapToSurface16(bmp, &GetSpriteFramePicture(slot, frame)->surface, NULL);
        bmp = GetNextBitmap(bmp);
        if (reinterpret_cast<u8*>(data) + size
            <= reinterpret_cast<u8*>(bmp)) { // Byte-forced: stream end.
            return;
        }
    }
}

DATA(0x00090bfc)
static EffectImageCode s_effectImage;
DATA(0x00090c00)
static i16 s_effectShotX;
DATA(0x00090c04)
static i16 s_effectShotY;
DATA(0x00090c08)
static i16 s_effectShotZ;

#define CacheEffectFrame(bitmap, imageCode)                                                        \
    do {                                                                                           \
        if (EffectImagesDiffer(s_effectImage, imageCode)) {                                        \
            LoadBitmapToSurface16(bitmap, &g_effectFramePicture.surface, NULL);                    \
            s_effectImage = (imageCode);                                                           \
        }                                                                                          \
    } while (0)

// Descriptor and key retain their caller-scope stack homes.
#define SetEffectSurfaceColorKey(surface)                                                          \
    DDSURFACEDESC desc;                                                                            \
    DDCOLORKEY key;                                                                                \
    key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = 0;                                      \
    desc.dwSize = sizeof(desc);                                                                    \
    desc.dwFlags = DDSD_ALL;                                                                       \
    if ((surface)->GetSurfaceDesc(&desc) == DD_OK && IsPalettizedSurface(desc)) {                  \
        key.dwColorSpaceLowValue = key.dwColorSpaceHighValue = BMP_TRANSPARENT_INDEX;              \
    }                                                                                              \
    (surface)->SetColorKey(DDCKEY_SRCBLT, &key)

RVA(0x00058990, 0x4c)
void ClearEffectLayer(i16 unused) {
    ClearDisplaySurface(g_backdropPicture.surface, NULL);
    GetShotPosition(&s_effectShotX, &s_effectShotY, &s_effectShotZ);
    s_effectImage.frame = 0xfff;
}

DATA(0x0006dcb0)
static i16 s_effectLateralOffsets[4] = {60, 0, 0, 0};
DATA(0x0006dcb8)
static i16 s_effectHeightOffsets[4] = {0, 80, 56, 55};

// @early-stop register allocation: retail splits the size word into dl and bl,
// which frees ebx and reloads `code` for the mirror tests; here the low byte is
// masked from a word copy and `code` stays in ebx through the offsets.
RVA(0x000589e0, 0x457)
void DrawProjectedEffectSprite(EffectImageCode code, i16 x, i16 y) {
    i16 screenX;
    i16 screenY;
    i16 frame = ProjectEffectFrame(x, y, &screenX, &screenY);
    i32 size;
    BmpFile* bmp = GetEffectFrame(&size, frame);
    i16 left;
    i16 right;
    i16 top;
    i16 bottom;
    RECT dest;
    RECT source;
    float scaleX;
    float scaleY;
    for (u16 i = 0; i < code.frame; i++) {
        if (!HasBitmapFileSignature(&bmp->file)) {
            return;
        }
        u32 imageSize = bmp->file.bfSize;
        if (size <= imageSize) {
            break;
        }
        bmp = GetNextBitmap(bmp);
        size -= imageSize;
    }
    if (!HasBitmapFileSignature(&bmp->file)) {
        return;
    }
    CacheEffectFrame(bmp, code);
    left = GetEffectBitmapOffsetX(bmp) * 8;
    top = GetEffectBitmapOffsetY(bmp) * 8;
    const u16 dimensions = bmp->file.bfReserved2;
    const i32 width = LOBYTE(dimensions) * 8;
    const i32 height = HIBYTE(dimensions) * 8;
    if (code.mirrorHorizontal) {
        left = -1 - left;
        right = left - width + 1;
    } else {
        right = left + width - 1;
    }
    if (code.mirrorVertical) {
        top = -1 - top;
        bottom = top - height + 1;
    } else {
        bottom = top + height - 1;
    }
    left += screenX;
    right += screenX;
    top += screenY;
    bottom += screenY;
    SortShortPair(&left, &right);
    SortShortPair(&top, &bottom);
    if (screenX & 7) {
        right++;
    }
    if (left < 0) {
        left = 0;
    }
    if (right > 639) {
        right = 639;
    }
    if (top < 0) {
        top = 0;
    }
    if (bottom > 327) {
        bottom = 327;
    }
    if (left > right || top > bottom) {
        return;
    }
    if (static_cast<i16>(GetEffectImageBase()) == EFFECT_IMAGE_LATERAL_ADJUST && code.frame == 0) {
        dest.left = screenX < 320 ? left + s_effectLateralOffsets[frame / 2]
                                  : left - s_effectLateralOffsets[frame / 2];
        dest.right = screenX < 320 ? right + s_effectLateralOffsets[frame / 2]
                                   : right - s_effectLateralOffsets[frame / 2];
    } else {
        dest.left = left;
        dest.right = right;
    }
    dest.top = top + s_effectHeightOffsets[GetShotPower()];
    dest.bottom = dest.top + bottom - top;
    source.left = source.top = 0;
    source.right = bmp->info.biWidth;
    source.bottom = bmp->info.biHeight;
    scaleX = static_cast<float>(dest.right - dest.left) / bmp->info.biWidth;
    scaleY = static_cast<float>(dest.bottom - dest.top) / bmp->info.biHeight;
    if (dest.left < 0) {
        source.left = -dest.left / scaleX;
        dest.left = 0;
    }
    if (dest.top < 0) {
        source.top = -dest.top / scaleY;
        dest.top = 0;
    }
    if (dest.right > SCREEN_WIDTH) {
        source.right = (dest.right - SCREEN_WIDTH) / scaleX;
        source.right = bmp->info.biWidth - source.right;
        dest.right = SCREEN_WIDTH;
    }
    if (dest.bottom > VIEW_HEIGHT) {
        if (code.mirrorVertical) {
            source.top = (dest.bottom - VIEW_HEIGHT) / scaleY;
        } else {
            source.bottom =
                bmp->info.biHeight - static_cast<i32>((dest.bottom - VIEW_HEIGHT) / scaleY);
        }
        dest.bottom = VIEW_HEIGHT;
    }
    DDBLTFX fx;
    InitEffectBlitFx(fx, code);
    g_backdropPicture.surface
        ->Blt(&dest, g_effectFramePicture.surface, &source, DDBLT_DDFX | DDBLT_KEYSRC, &fx);
    SetEffectSurfaceColorKey(g_backdropPicture.surface);
}

// @early-stop register allocation: retail holds the vertical offset byte in dl
// while this source keeps it in cl; the later rectangle arithmetic is the same.
RVA(0x00058e40, 0x252)
void DrawScreenEffectSprite(BmpFile* imageData, EffectImageCode code, i16 x, i16 y) {
    BmpFile* bmp = imageData;
    for (u16 i = 0; i < code.frame; i++) {
        if (!HasBitmapFileSignature(&bmp->file)) {
            return;
        }
        bmp = GetNextBitmap(bmp);
    }
    if (!HasBitmapFileSignature(&bmp->file)) {
        return;
    }
    CacheEffectFrame(bmp, code);
    i16 screenX;
    i16 screenY;
    GetScriptAnimationPosition(x, y, &screenX, &screenY);
    RECT dest;
    RECT source;
    source.left = source.top = 0;
    source.right = bmp->info.biWidth;
    source.bottom = bmp->info.biHeight;
    const i8 horizontalOffset = GetEffectBitmapOffsetX(bmp);
    dest.left = screenX + horizontalOffset * 8;
    const i8 verticalOffset = GetEffectBitmapOffsetY(bmp);
    const i32 topOffset = verticalOffset - bmp->info.biHeight / 2;
    dest.top = (screenY + topOffset) * 11 / 10;
    dest.right = dest.left + bmp->info.biWidth;
    dest.bottom = dest.top + bmp->info.biHeight * 11 / 10;
    if (dest.left < 0) {
        source.left = -dest.left;
        dest.left = 0;
    }
    if (dest.top < 0) {
        dest.bottom -= dest.top;
        dest.top = 0;
    }
    if (dest.right > SCREEN_WIDTH) {
        source.right = bmp->info.biWidth - dest.right + SCREEN_WIDTH;
        dest.right = SCREEN_WIDTH;
    }
    if (dest.bottom > SCREEN_HEIGHT) {
        source.bottom = (bmp->info.biHeight - dest.bottom + SCREEN_HEIGHT) * 10 / 11;
        dest.bottom = SCREEN_HEIGHT;
    }
    DDBLTFX fx;
    InitEffectBlitFx(fx, code);
    g_backdropPicture.surface
        ->Blt(&dest, g_effectFramePicture.surface, &source, DDBLT_DDFX | DDBLT_KEYSRC, &fx);
    SetEffectSurfaceColorKey(g_backdropPicture.surface);
}

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

#define ClampHotspotTexel(value, extent)                                                           \
    do {                                                                                           \
        if ((value) < 0) {                                                                         \
            (value) = 0;                                                                           \
        } else if ((value) > (extent) - 1) {                                                       \
            (value) = (extent) - 1;                                                                \
        }                                                                                          \
    } while (0)

RVA(0x00059180, 0x24f)
b32 ClickHotspotAt(const i32 x, const i32 y) {
    if (GetTextPlane(0)->visible) {
        return false;
    }
    if (y > VIEW_HEIGHT - 1) {
        return false;
    }
    i32 hit = HOTSPOT_NONE;
    const GZ_ENUM_LOCAL(ViewDirection, i16) direction = GetMapPosition()->direction;
    for (i32 i = g_hotspotCount - 1; i >= 0; i--) {
        const Hotspot* candidate = GetHotspot(i);
        if (candidate->rect.left > x || candidate->rect.right <= x || candidate->rect.top > y
            || candidate->rect.bottom <= y) {
            continue;
        }
        const Texture* texture = candidate->texture;
        const u8* pixels = GetBitmapPixels(texture->image);
        const u32 width = texture->width;
        const u32 height = min(256, width);
        i32 u = (x - candidate->rect.left) * width / (candidate->rect.right - candidate->rect.left);
        i32 v =
            height
            - (y - candidate->rect.top) * height / (candidate->rect.bottom - candidate->rect.top)
            - 1;
        ClampHotspotTexel(u, texture->width);
        ClampHotspotTexel(v, static_cast<i32>(height));
        if (pixels[v * width + u] == BMP_TRANSPARENT_INDEX) {
            continue;
        }
        if (hit != HOTSPOT_NONE) {
            const Hotspot* selected = GetHotspot(hit);
            i32 candidateCoord;
            i32 selectedCoord;
            switch (direction) {
                case VIEW_NORTH:
                    selectedCoord = selected->targetY;
                    candidateCoord = candidate->targetY;
                    goto nearerGreater;
                case VIEW_EAST:
                    selectedCoord = selected->targetX;
                    candidateCoord = candidate->targetX;
                    goto nearerLess;
                case VIEW_SOUTH:
                    selectedCoord = selected->targetY;
                    candidateCoord = candidate->targetY;
                    goto nearerLess;
                default:
                    selectedCoord = selected->targetX;
                    candidateCoord = candidate->targetX;
                    goto nearerGreater;
                nearerGreater:
                    if (candidateCoord > selectedCoord) {
                        hit = i;
                    }
                    break;
                nearerLess:
                    if (candidateCoord < selectedCoord) {
                        hit = i;
                    }
                    break;
            }
        } else {
            hit = i;
        }
    }
    if (hit < 0) {
        return false;
    }
    const Hotspot* hotspot = GetHotspot(hit);
    switch (hotspot->kind) {
        case HOTSPOT_BOX:
            StartBoxScene(static_cast<TreasureBox*>(hotspot->data));
            return true;
        case HOTSPOT_NPC:
            StartNpcScene(static_cast<AreaNpc*>(hotspot->data));
            return true;
        case HOTSPOT_TARGET:
            if (hit == g_selectedHotspot) {
                TalkCommand();
                return true;
            }
            if (!AnyObjectInReach() || IsPartyAt(hotspot->targetX, hotspot->targetY)) {
                g_selectedHotspot = hit;
            }
            break;
    }
    return true;
}
