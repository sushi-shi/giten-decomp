#ifndef GITEN_GFX_BACKGROUND_H
#define GITEN_GFX_BACKGROUND_H

#include <rva.h>

#include <Ints.h>

struct ImageRequest;

// vram's picture loader through SeekBitmap (kind 0 with the request variant,
// `mode` as the skip count).
// @identity-TODO: what `mode` selects is unrecovered.
void* LoadImageRequest(struct ImageRequest* request, i16 mode);

// Loads background picture `image` (record 0x5000 + image) and draws it.
// @identity-TODO: the role of `arg` (the request variant and the loader's
// second argument) is unrecovered.
void ShowBackground(i16 image, i16 arg);

// Redraws the background of the party's map position.
void RestoreBackground(void);

// @identity-TODO: the third byte of a background's scene-cell parameters
// is passed to this empty Windows hook; its original role is unrecovered.
void BackgroundParameterNop(i16 parameter, i16 mode);
void RestoreBackgroundParameter(void);

// Legacy palette refresh request and completion wait; disabled on Windows.
RVA_DECL(0x000049b0)
void RequestPaletteRefresh(void);

RVA_DECL(0x000049c0)
void WaitPaletteRefresh(void);

static __inline void RefreshPalette(void) {
    RequestPaletteRefresh();
    WaitPaletteRefresh();
}

#endif // GITEN_GFX_BACKGROUND_H
