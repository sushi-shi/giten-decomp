#ifndef GITEN_GFX_RENDER_H
#define GITEN_GFX_RENDER_H

#include <rva.h>

#include <Ints.h>

extern i16 g_renderMode;

// The frame renderer's mode (the index into its handler table 0x46bc58).
// @identity-TODO: modes 0 (event picture), 3 (the view with the field
// composited) and 9 (layers and text only) are named from their handlers.
#define RENDER_MODE_EVENT 0
#define RENDER_MODE_VIEW 1
#define RENDER_MODE_SCENE 2
#define RENDER_MODE_VIEW_FRAME 3
#define RENDER_MODE_PICTURE 4
#define RENDER_MODE_PANEL 5
#define RENDER_MODE_FIELD 6
#define RENDER_MODE_BLANK 7
#define RENDER_MODE_STATUS 8
#define RENDER_MODE_LAYERS 9
#define RENDER_MODE_SAVE 15

// The blank screen's sub-modes (see StepBlankRenderMode).
#define BLANK_STEP_NONE 0
#define BLANK_STEP_FIRST 1
#define BLANK_STEP_SECOND 2

// @identity-TODO: The naming of render modes is inferred from the mode-2 frame handler (0x4ea10
// draws the background surface when 0x8d700 is set); unproven.
RVA_DECL(0x00049bd0)
void SetSceneRenderMode(void);

// @identity-TODO: The naming of render mode 7 is inferred from its handler 0x4f010 (colour-
// fill, then per sub-mode 0x847f8 extra draws); unproven.
RVA_DECL(0x00049ce0)
void SetBlankRenderMode(void);

RVA_DECL(0x00049ba0)
void SetViewRenderMode(void);

// @identity-TODO: What screen mode 5 serves is unproven: handler 0x4ed80 draws the backdrop
// plus overlay 0x48fd84 over one of six hot cells (0x16810).
RVA_DECL(0x00049bb0)
void SetPanelRenderMode(void);

// @identity-TODO: That mode 8 is the status screen is inferred from its handler 0x4f0b0 (blits
// 0x490aa4 at x=0x28 then text planes) and caller 18b60.
RVA_DECL(0x00049cc0)
void SetStatusRenderMode(void);

// Steps the blank screen's fade: from sub-mode 1 to 2 (blank mode 7), from 2
// back to sub-mode 0 (scene mode 2).
// @identity-TODO: what the sub-modes (0x4847f8) show is unrecovered.
RVA_DECL(0x00049d10)
void StepBlankRenderMode(void);

RVA_DECL(0x00049d50)
i16 GetRenderMode(void);

#endif // GITEN_GFX_RENDER_H
