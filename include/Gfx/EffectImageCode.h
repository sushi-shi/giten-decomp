#ifndef GITEN_GFX_EFFECTIMAGECODE_H
#define GITEN_GFX_EFFECTIMAGECODE_H

#include <Ints.h>

typedef struct EffectImageCode {
    u16 frame : 14;
    u16 mirrorVertical : 1;
    u16 mirrorHorizontal : 1;
} EffectImageCode;

#define EffectImagesDiffer(first, second)                                                          \
    ((first).frame != (second).frame || (first).mirrorVertical != (second).mirrorVertical          \
     || (first).mirrorHorizontal != (second).mirrorHorizontal)

#endif // GITEN_GFX_EFFECTIMAGECODE_H
