#ifndef GITEN_GFX_TEXTURE_H
#define GITEN_GFX_TEXTURE_H

#include <Enums.h>
#include <Gfx/Bitmap.h>
#include <Ints.h>
#include <Platform/Direct3D.h>
#include <Platform/Com.h>

// clang-format off
GZ_ENUM_BEGIN(TextureShade)
    TEXTURE_SHADE_NORMAL = 0,
    TEXTURE_SHADE_DARK = 1,
    TEXTURE_SHADE_LIT = 2,
    TEXTURE_SHADE_COUNT = 3
GZ_ENUM_END(TextureShade);
// clang-format on

// A texture loaded from an 8-bit bitmap: its palette, the
// system-memory source texture, the device surface and texture, its device
// handle (0 until loaded), the material the ramp device draws it with
// (plain, dark and lit), the loaded .bmp file and its transparent bottom margin.
typedef struct Texture {
    PALETTEENTRY palette[256];
    i32 width; // the loaded width, at most 512
    LPDIRECTDRAWSURFACE sourceSurface;
    LPDIRECT3DTEXTURE2 source;
    LPDIRECTDRAWSURFACE surface;
    LPDIRECT3DTEXTURE2 texture;
    LPDIRECTDRAWPALETTE ddPalette;
    D3DTEXTUREHANDLE handle;
    LPDIRECT3DMATERIAL2 materials[TEXTURE_SHADE_COUNT];
    D3DMATERIALHANDLE materialHandles[TEXTURE_SHADE_COUNT];
    DWORD colorKey;
    BmpFile* image;
    u16 bottomMargin;
} Texture;

#define GetTextureHandle(texture) ((texture)->handle)

#define HasTextureHandle(texture) (GetTextureHandle(texture) != 0)

#define GetTextureMaterialHandle(texture, shade) ((texture)->materialHandles[shade])

#define ReleaseTextureSurfaces(value)                                                              \
    do {                                                                                           \
        ReleaseComObject((value)->ddPalette);                                                      \
        ReleaseComObject((value)->texture);                                                        \
        ReleaseComObject((value)->surface);                                                        \
        ReleaseComObject((value)->source);                                                         \
        ReleaseComObject((value)->sourceSurface);                                                  \
        (value)->handle = 0;                                                                       \
    } while (0)

#ifdef __cplusplus
extern "C" {
#endif

    // The textures loaded and released by the display setup.
    extern Texture g_textBoxTexture;    // w\txrtbox.bmp
    extern Texture g_stairsUpTexture;   // w\up.bmp
    extern Texture g_stairsDownTexture; // w\dn.bmp
    extern Texture g_darkWallTexture;   // w\darkwall.bmp
    extern Texture g_npcTexture;        // w\npc.bmp
    // @identity-TODO: roles inferred only from the renderer reading them.
    extern Texture g_roomTexture;
    extern Texture g_enemyTextures[2][5];
    extern Texture g_objectTextures[6];

    BmpFile* OpenTextureBitmap(Texture* texture, const char* name, b32 fromFile);
    void RestoreTexture(Texture* texture);
    void ShadeTexture(Texture* texture, TextureShade shade);

#ifdef __cplusplus
}
#endif

#define SelectNpcBillboardTexture(texture, slot)                                                   \
    do {                                                                                           \
        if ((slot) >= 0 && (slot) <= 5) {                                                          \
            if (HasTextureHandle(&g_objectTextures[(slot)])) {                                     \
                (texture) = &g_objectTextures[(slot)];                                             \
            } else {                                                                               \
                (texture) = &g_npcTexture;                                                         \
            }                                                                                      \
        } else {                                                                                   \
            (texture) = &g_npcTexture;                                                             \
        }                                                                                          \
    } while (0)

#endif // GITEN_GFX_TEXTURE_H
