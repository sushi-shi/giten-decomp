#ifndef GITEN_GAME_INFOBAR_H
#define GITEN_GAME_INFOBAR_H

#include <rva.h>

#include <Ints.h>
#include <Gfx/ScreenLayer.h>

#define ClearLocationCaption()                                                                     \
    DrawLayerText(                                                                                 \
        SCREEN_LAYER_LOCATION,                                                                     \
        8,                                                                                         \
        8,                                                                                         \
        "\201@\201@\201@\201@\201@\201@\201@\201@\201@\201@",                                      \
        0x3400                                                                                     \
    )

void DrawMoneyCounters(i16 mode);
void DrawMoneyCounter(i16 mode, i16 row, i32 value, i16 currency);
void DrawAreaInfo(void);

// Draws the moon phase and the leader's magnetite and macca counters.
RVA_DECL(0x0001e860)
b16 DrawInfoBar(i16 layout, i16 partial);

b16 RefreshInfoBar(i16 force);

b16 UpdateInfoBar(void);

#endif // GITEN_GAME_INFOBAR_H
