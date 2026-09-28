// The textures: loading an 8- or 16-bit bitmap into a system-memory source
// texture and a device texture.

#include <rva.h>

#include <Gfx/Bitmap.h>
#include <Gfx/D3DState.h>
#include <Gfx/DDraw.h>
#include <Gfx/Texture.h>

#include <string.h>

// The largest texture loaded; bigger bitmaps are cropped.
#define TEXTURE_MAX_WIDTH 512
#define TEXTURE_MAX_HEIGHT 256

// The pixel depth below which a texture is palettized.
#define TEXTURE_BITS_16 16

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
    memset(&desc, 0, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    desc.dwWidth = TEXTURE_MAX_WIDTH;
    if (header->biWidth <= TEXTURE_MAX_WIDTH) {
        desc.dwWidth = header->biWidth;
    }
    desc.dwHeight = TEXTURE_MAX_HEIGHT;
    if (header->biHeight <= TEXTURE_MAX_HEIGHT) {
        desc.dwHeight = header->biHeight;
    }
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
        &IID_IDirect3DTexture2,
        (void**)source
    );
    texture->width = header->biWidth > TEXTURE_MAX_WIDTH ? TEXTURE_MAX_WIDTH : header->biWidth;
    if (desc.ddpfPixelFormat.dwRGBBitCount < TEXTURE_BITS_16) {
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
        if (fromFile != TRUE) {
            return false;
        }
        FreeBitmap(&bmp);
        return false;
    }
    if (desc.ddpfPixelFormat.dwRGBBitCount < TEXTURE_BITS_16
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
            &IID_IDirect3DTexture2,
            (void**)&texture->texture
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
