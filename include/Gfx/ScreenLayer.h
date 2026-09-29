#ifndef GITEN_GFX_SCREENLAYER_H
#define GITEN_GFX_SCREENLAYER_H

#include <rva.h>

#include <Win32.h>

#include <EnumDomain.h>
#include <Enums.h>

#define SCREEN_LAYER_COUNT 15

// Slots shared by the layer painter, input handling and information bar.
// clang-format off
GZ_ENUM_BEGIN(ScreenLayerSlot)
    SCREEN_LAYER_MENU_BAR = 0,
    SCREEN_LAYER_PANEL = 1,
    SCREEN_LAYER_ICON = 2,
    SCREEN_LAYER_MOON_PHASE = 3,
    SCREEN_LAYER_LOCATION = 4,
    SCREEN_LAYER_CURRENCY = 5,
    SCREEN_LAYER_AUTOMAP = 6,
    SCREEN_LAYER_NAVIGATION = 7,
    SCREEN_LAYER_NONPARTY_LAST = SCREEN_LAYER_NAVIGATION,
    SCREEN_LAYER_FIRST_PANEL = 8,
    SCREEN_LAYER_DEFAULT_VISIBLE_BEGIN = SCREEN_LAYER_ICON,
    SCREEN_LAYER_DEFAULT_VISIBLE_END = SCREEN_LAYER_FIRST_PANEL,
    SCREEN_LAYER_TEXT = 14
GZ_ENUM_END(ScreenLayerSlot);
// clang-format on

#define PARTY_PANEL_COUNT 6

// One of the 15 display layers CreateScreenLayer creates: its kind (slot),
// screen position, the BltFast flags it is drawn with (keyed), its source
// rectangle (0, 0, width, height), the painted surface and, for the layers
// that have one, a second work surface drawn over it.
typedef struct ScreenLayer {
    b32 visible;
    GZ_ENUM_STORAGE(ScreenLayerSlot, i32) slot;
    i32 x;
    i32 y;
    DWORD bltFlags;
    RECT source;
    LPDIRECTDRAWSURFACE surface;
    LPDIRECTDRAWSURFACE canvas;
} ScreenLayer;

#define BlitScreenLayer(target, layer)                                                             \
    do {                                                                                           \
        if ((layer)->visible) {                                                                    \
            IDirectDrawSurface_BltFast(                                                            \
                (target),                                                                          \
                (layer)->x,                                                                        \
                (layer)->y,                                                                        \
                (layer)->surface,                                                                  \
                &(layer)->source,                                                                  \
                (layer)->bltFlags                                                                  \
            );                                                                                     \
            if ((layer)->canvas != NULL) {                                                         \
                IDirectDrawSurface_BltFast(                                                        \
                    (target),                                                                      \
                    (layer)->x,                                                                    \
                    (layer)->y,                                                                    \
                    (layer)->canvas,                                                               \
                    &(layer)->source,                                                              \
                    DDBLTFAST_SRCCOLORKEY                                                          \
                );                                                                                 \
            }                                                                                      \
        }                                                                                          \
    } while (0)

extern ScreenLayer* g_screenLayers[SCREEN_LAYER_COUNT];

i16 IsPanelLayerVisible(void);

// The party member id shown by the visible command panel, else -1.
i16 GetShownPanelCharacter(void);

void ScreenToAutomapCell(i16* x, i16* y);

// @identity-TODO: That 0x48fb08 marks a mouse drag whose outline 0x4ba70 draws (rect
// 0x48f560..) is inferred; the setter is unfound.
RVA_DECL(0x00049780)
void CancelLayerDrag(void);

// @identity-TODO: That the argument is a picture id choosing its target surface (>=0x36 ->
// 0x48f404, 0x1e/0x1f -> 0x4847e4, else layer 1) is inferred; ids 0x3f..0x4b are skipped.
RVA_DECL(0x000542d0)
void ErasePictureSurface(i16 picture);

// @identity-TODO: The layers of table 0x48fb10 (layer 7 = compass per 0x4f6e0) are otherwise
// unnamed.
RVA_DECL(0x00054360)
void ShowScreenLayer(GZ_ENUM_PARAM(ScreenLayerSlot, i16) layer);

// @identity-TODO: Which of a layer's two surfaces (+0x24 loaded by 0x57290, +0x28 here) is the
// canvas is unproven.
RVA_DECL(0x000543d0)
void ClearLayerSurface(GZ_ENUM_PARAM(ScreenLayerSlot, i16) layer);

// @identity-TODO: That surface 0x490aa4 is the status-screen picture is inferred from the
// mode-8 handler 0x4f0b0 blitting it.
RVA_DECL(0x00054400)
void ClearStatusPicture(void);

#endif // GITEN_GFX_SCREENLAYER_H
