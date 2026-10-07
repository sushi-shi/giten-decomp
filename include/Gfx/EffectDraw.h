#ifndef GITEN_GFX_EFFECTDRAW_H
#define GITEN_GFX_EFFECTDRAW_H

#include <rva.h>

#include <Gfx/EffectImageCode.h>
#include <Ints.h>

struct BmpFile;

#ifdef __cplusplus
extern "C" {
#endif

    RVA_DECL(0x00058990)
    void ClearEffectLayer(i16 unused);

    RVA_DECL(0x000589e0)
    void DrawProjectedEffectSprite(EffectImageCode code, i16 x, i16 y);

    RVA_DECL(0x00058e40)
    void DrawScreenEffectSprite(struct BmpFile* imageData, EffectImageCode code, i16 x, i16 y);

#ifdef __cplusplus
}
#endif

#endif // GITEN_GFX_EFFECTDRAW_H
