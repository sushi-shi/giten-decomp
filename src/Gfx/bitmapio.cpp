// Bitmap I/O: reading BMP files, loading them and bitmap resources into
// surfaces, the pictures, the textures and the world map tiles. One TU: its
// .data run holds s_textureDiffuse and s_worldMapRegions, then the literals in
// first-use order, ReadBitmapFile's before CreatePicture's and ShadeTexture's;
// s_wallTextureNames opens the next object. ReadBitmapFile calls operator new,
// so it is C++, with C linkage kept through the headers; PackSurfaceColor and
// GetWorldMapBlitRects inline, so it is built without /Ob0.

#include <rva.h>

#include <Gfx/DDError.h>
#include <Gfx/Texture.h>
#include <Platform/Com.h>
#include <Platform/D3DApp.h>
#include <Platform/GameApi.h>

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
        if (fromFile == TRUE) {
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
        if (fromFile == TRUE) {
            FreeBitmap(&bmp);
        }
        return false;
    }
    if (IsPalettizedSurface(desc)
        && IDirectDrawSurface_SetPalette(texture->surface, texture->ddPalette) != DD_OK) {
        if (fromFile == TRUE) {
            FreeBitmap(&bmp);
        }
        return false;
    }
    if (fromFile == TRUE) {
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
    if (fromFile == TRUE) {
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
    g_scenePicture.visible = TRUE;
}

RVA(0x00057b90, 0x59)
void LoadScenePicture(BmpFile* bmp, u16 id) {
    if (bmp != NULL) {
        LoadBitmapToSurface16(bmp, &g_scenePicture.surface, NULL);
        g_scenePicture.id = id;
        GetBitmapRect(g_scenePicture.rect, bmp);
        g_scenePicture.visible = TRUE;
    }
}

RVA(0x00057bf0, 0x4b)
void ClearSceneSurfaces(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    ClearDisplaySurface(g_viewCachePicture.surface, NULL);
    GetTextPlane(0)->visible = FALSE;
    g_scenePicture.visible = FALSE;
}

RVA(0x00057c40, 0x30)
void ClearScenePicture(void) {
    ClearDisplaySurface(g_scenePicture.surface, NULL);
    GetTextPlane(0)->visible = FALSE;
    g_scenePicture.visible = FALSE;
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
            TRUE
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
        g_scenePicture.visible = FALSE;
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
            TRUE
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
