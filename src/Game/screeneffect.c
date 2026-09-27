// @identity-TODO: the original TU name is unproven; this unit holds the
// screen-effect state and its contiguous resource and coordinate helpers.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/FieldSight.h>
#include <Game/ScreenEffect.h>
#include <Game/StateStack.h>
#include <Gfx/Motion.h>
#include <Gfx/Vram.h>
#include <Mem/Alloc.h>

DATA(0x00080110)
static i32 s_animationImageSize;

DATA(0x000809a4)
static struct BmpFile* s_animationImage;

DATA(0x00080a28)
static u8* s_animationScript;

DATA(0x00080a2c)
static i16 s_animationResource;

DATA(0x00080a30)
static i16 s_animationX;

DATA(0x00080a34)
static i16 s_animationY;

RVA(0x000286a0, 0x6)
struct BmpFile* GetScreenEffectImage(void) {
    return s_animationImage;
}

// @dead-code
// Zero-ref: no retail call, jump, or relocated pointer reaches this accessor.
RVA(0x000286b0, 0x6)
i32 GetScreenEffectImageSize(void) {
    return s_animationImageSize;
}

RVA(0x000286c0, 0x2f)
void PushScriptAnimation(i16 animation, i16 x, i16 y) {
    s_animationResource = animation * 16;
    s_animationX = x;
    s_animationY = y;
    PushGameState(37);
}

RVA(0x000286f0, 0x9e)
i16 RunScriptAnimationState(void) {
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            LoadScriptAnimation(s_animationResource);
            LoadScriptAnimationImage(s_animationResource);
            StartEffectScript(s_animationScript, 0);
            break;
        case 1:
            if (!StepScreenEffectScript()) {
                NextGamePhase();
            }
            break;
        case 2:
            s_animationImage = FreeImageFile(s_animationImage);
            ClearEffectLayer(1);
            s_animationScript = FreeBlock(s_animationScript);
            ReturnFromGameState();
            break;
    }
    return 0;
}

RVA(0x00028790, 0x32)
void LoadScriptAnimation(i16 resource) {
    FILE* file = OpenDataFile(resource + 0x3000, 2, 0);
    s_animationScript = ReadRawAlloc(file);
    CloseDataFile(file);
}

RVA(0x000287d0, 0x3b)
void LoadScriptAnimationImage(i16 resource) {
    ImageRequest request;
    request.file = resource + 0x3000;
    request.variant = 0;
    request.flags = 2;
    s_animationImage = LoadImageVariant(&request, &s_animationImageSize);
}

RVA(0x00028810, 0x2a)
void GetScriptAnimationPosition(i16 x, i16 y, i16* screenX, i16* screenY) {
    *screenX = (x + s_animationX) * 8;
    *screenY = y + s_animationY;
}
