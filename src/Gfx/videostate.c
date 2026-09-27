// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span of PC-98 video stubs until link-order evidence names it.

#include <rva.h>

#include <Gfx/VideoState.h>

// @identity-TODO: the buffer the two plane accessors hand out for every plane;
// nothing in the retail image sets it.
DATA(0x00091060)
static VideoPlane* s_planeBuffer;

// The viewport of planes 0..2; the Windows stub shares one backing plane.
RVA(0x00045b40, 0x1a)
VideoViewport* GetPlaneData(i16 plane) {
    switch (plane) {
        case 0:
        case 1:
        case 2:
            return &s_planeBuffer->viewport;
    }
    return NULL;
}

// The plane flags followed by its viewport.
RVA(0x00045b60, 0x17)
VideoPlane* GetPlaneHeader(i16 plane) {
    switch (plane) {
        case 0:
        case 1:
        case 2:
            return s_planeBuffer;
    }
    return NULL;
}

// @dead-code
// Zero-ref: no retail call, jump or relocated pointer reaches this helper.
// The legacy viewport clipping is disabled in the Windows build.
RVA(0x00045b80, 0x4)
i16 ClipViewportRect(VideoViewport* viewport, i16* left, i16* right, i16* top, i16* bottom) {
    return 0;
}

RVA(0x00045b90, 0x1)
void SaveVideoState(void* state) {}

RVA(0x00045ba0, 0x1)
void RestoreVideoState(void* state) {}
