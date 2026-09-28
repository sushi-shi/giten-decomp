#ifndef GITEN_GFX_VIDEOSTATE_H
#define GITEN_GFX_VIDEOSTATE_H

#include <rva.h>

#include <Ints.h>

#include <stddef.h>

// Placement and source clipping in byte columns and pixel lines.
typedef struct VideoViewport {
    i16 x;
    i16 y;
    i16 sourceX;
    i16 sourceY;
    i16 width;
    i16 height;
    i16 screenLeft;
    i16 screenTop;
    i16 screenRight;
    i16 screenBottom;
    i16 clipLeft;
    i16 clipTop;
    i16 clipRight;
    i16 clipBottom;
} VideoViewport;

typedef struct VideoPlane {
    u16 flags;
    VideoViewport viewport;
} VideoPlane;

// @identity-TODO: Empty on Windows (bare ret) amid PC-98 stubs (0x45b40/0x45b60 plane
// pointers); what the PC-98 original saved into the 16-byte blocks is unknown.
void SaveVideoState(void* state);

// @identity-TODO: Same as SaveVideoState: empty stub, PC-98 role unknown.
void RestoreVideoState(void* state);

VideoViewport* GetPlaneData(i16 plane);

VideoPlane* GetPlaneHeader(i16 plane);

b16 ClipViewportRect(VideoViewport* viewport, i16* left, i16* right, i16* top, i16* bottom);

#endif // GITEN_GFX_VIDEOSTATE_H
