// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <File/DataFileKind.h>
#include <Game/FieldSight.h>
#include <Game/PlayTime.h>
#include <Game/ScreenEffect.h>
#include <Game/WaitLoop.h>
#include <Gfx/Background.h>
#include <Gfx/Motion.h>
#include <Gfx/Shot.h>
#include <Gfx/Vram.h>
#include <Gfx/VramAccess.h>
#include <Input/Mouse.h>
#include <Input/MouseClickState.h>
#include <Math/Vec3.h>
#include <Mem/Alloc.h>
#include <Sound/Sound.h>
#include <Util/Range.h>

#include <stddef.h>

// @identity-TODO: this build only clears these three scene-input words.
DATA(0x00078060)
static i16 s_sceneInputFirst = 0;

DATA(0x00078064)
static i16 s_sceneInputSecond = 0;

// Request bits for a display interrupt that this image does not contain.
DATA(0x00078068)
static i16 s_sceneFlags = 0;

DATA(0x00078070)
static i16 s_sceneInputPending = 0;

DATA(0x00078078)
MotionFile g_motionFile = {0};

// @identity-TODO: this never-written word is read only by an uncalled
// effect-state getter; no writer or stronger type survives in this image.
DATA(0x00078088)
static i32 s_legacyEffectStateValue = 0;

// The playing motion: its path, step delay (0: advance only on request),
// countdown to the next step, current step, and the fixed-point scale.
DATA(0x00078090)
static EffectSlot s_effectSlots[EFFECT_SLOTS] = {0};

DATA(0x00078098)
MotionTable g_loadedMotionTable = {0};

// @identity-TODO: this never-written word is read only by an uncalled
// image-set getter; no writer or stronger type survives in this image.
DATA(0x000780b8)
static i32 s_legacyImageSetStateValue = 0;

DATA(0x000780c0)
static EffectImageSet s_imageSets[EFFECT_IMAGE_SETS] = {0};

DATA(0x00078118)
static EffectPalette s_paletteData[64] = {0};

DATA(0x00078218)
static i16 s_currentImageSet = 0;

// @identity-TODO: set by the effect state's first phase (0) and its skip
// path (1); nothing reads it back.
DATA(0x0007821c)
static b16 s_effectSkipping = false;

DATA(0x00078220)
static i16 s_paletteRefs = 0;

DATA(0x00078224)
static i16 s_motionDelay = 0;

DATA(0x00078228)
static i16 s_motionCountdown = 0;

DATA(0x0007822c)
static i16 s_effectDelay = 0;

DATA(0x00078230)
static EffectPalette* s_palettes = NULL;

DATA(0x00078234)
static u8* s_effectScriptBase = NULL;

DATA(0x00078238)
static MotionPath* s_motionPath = NULL;

DATA(0x0007823c)
static EffectCommand* s_effectScript = NULL;

DATA(0x00078240)
static MotionTable* s_motionTable = NULL;

DATA(0x00078244)
static u16 s_motionStep = 0;

DATA(0x000683f4)
static i16 s_currentEffect = EFFECT_ID_NONE;

DATA(0x000683f8)
static i16 s_motionScale = 1;

// @early-stop: retail loads the word, ORs in 32 bits and stores the low half;
// cl folds `|=` into one `or word ptr` unless the word is volatile, and no
// asynchronous writer or reader exists. Swapped, widened and u16 forms fold.
RVA(0x00004920, 0x14)
void SetSceneFlags(i32 bits) {
    s_sceneFlags |= bits;
}

RVA(0x00004940, 0x34)
void ResetSceneInput(void) {
    ResetPlayTime();
    InitCheckerPatterns();
    g_mouseLeftClick = MOUSE_CLICK_NONE;
    g_mouseRightClick = MOUSE_CLICK_NONE;
    s_sceneInputPending = 0;
    s_sceneInputFirst = 0;
    s_sceneInputSecond = 0;
    SetSceneFlags(0);
}

// @identity-TODO: this unused empty scene-input hook has no recovered name
// or signature; placement within the motion interface is its only context.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00004980, 0x1)
void SkipLegacySceneInputHook(void) {}

RVA(0x00004990, 0x10)
b16 PollIdle(i16 mode, i16 frames) {
    return false;
}

// @dead-code
// Zero-ref: no rel32 caller, data slot or address-taking (giten sema xref --tree).
RVA(0x000049a0, 0x7)
i16 GetSceneFlags(void) {
    return s_sceneFlags;
}

RVA(0x000049b0, 0x10)
void RequestPaletteRefresh(void) {}

RVA(0x000049c0, 0x10)
void WaitPaletteRefresh(void) {}

// @identity-TODO: these empty legacy hooks have no known signatures or names.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000049d0, 0x1)
void SkipLegacyPaletteRefresh(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000049e0, 0x1)
void SkipLegacyPaletteWait(void) {}

// @identity-TODO: original API name and signature are unproven.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x000049f0, 0x1)
void SkipLegacyMotionReset(void) {}

RVA(0x00004a00, 0x65)
void LoadMotionTable(FILE* file) {
    u16 index;
    s_motionTable = &g_loadedMotionTable;
    ReadRawBlock(file, &g_motionFile);
    s_motionTable->count = g_motionFile.table.count;
    for (index = 0; index < g_motionFile.table.count; ++index) {
        void* path = g_motionFile.bytes + g_motionFile.table.offsets[index];
        s_motionTable->paths[index] = path;
    }
}

RVA(0x00004a70, 0x3b)
void StartMotion(i16 index, i16 delay, i16 scale) {
    s_motionPath = s_motionTable->paths[index];
    s_motionScale = scale;
    s_motionStep = 0;
    s_motionDelay = delay;
    s_motionCountdown = delay;
}

static __inline void AdvanceMotionStep(void) {
    if (++s_motionStep >= s_motionPath->count) {
        s_motionStep = 0;
    }
}

// Returns 1 when the motion advanced a step.
RVA(0x00004ab0, 0x7d)
b16 StepMotion(i16 immediate) {
    if (s_motionDelay == 0 && immediate == 0) {
        AdvanceMotionStep();
        return true;
    }
    if (--s_motionCountdown <= 0) {
        s_motionCountdown = s_motionDelay;
        AdvanceMotionStep();
        return true;
    }
    return false;
}

// The shot-relative point at motion depth 731, offset by the current motion
// step scaled by the 2.14 fixed-point scale.
RVA(0x00004b30, 0x7e)
void MotionPoint(i16 x, i16 y, Vec3* out) {
    ShotRelativePoint(x, y, 731, out);
    out->x += GetMotionPathStep(s_motionPath, s_motionStep)->x * s_motionScale / 16384;
    out->y += GetMotionPathStep(s_motionPath, s_motionStep)->y * s_motionScale / 16384;
}

RVA(0x00004bb0, 0x15)
i32 ClampEffectCount(i16 count) {
    if (count > 32) {
        return 32;
    }
    return count;
}

static __inline EffectSprite* GetEffectFrameEnd(EffectCommand* command) {
    return &command->frame.sprites[command->frame.count];
}

// Starts the effect script at `offset` within `base`.
RVA(0x00004bd0, 0x26)
void StartEffectScript(u8* base, u16 offset) {
    void* command;
    s_effectScriptBase = base;
    command = OffsetBy(base, offset);
    s_effectScript = command;
    s_effectDelay = 0;
}

GZ_ENUM_BEGIN(EffectCommandResult)
    EFFECT_COMMAND_ADVANCE = -1,
    EFFECT_COMMAND_END = 0,
    EFFECT_COMMAND_FRAME = 1
GZ_ENUM_END(EffectCommandResult);

RVA(0x00004c00, 0xc1)
EffectCommand* StepEffectScript(void) {
    i16 result;
    i16 count;
    i16 i;
    void* next;
    StepMotion(--s_effectDelay);
    if (s_effectDelay > 0) {
        return s_effectScript;
    }
    while ((result = ExecuteEffectCommand()) < EFFECT_COMMAND_FRAME) {
        if (result == EFFECT_COMMAND_END) {
            return s_effectScript;
        }
    }
    s_effectDelay = s_effectScript->frame.delay;
    count = ClampEffectCount(s_effectScript->frame.count);
    ClearEffectLayer(0);
    for (i = 0; i < count; i++) {
        DrawProjectedEffectSprite(
            s_effectScript->frame.sprites[i].image,
            s_effectScript->frame.sprites[i].x,
            s_effectScript->frame.sprites[i].y
        );
    }
    next = GetEffectFrameEnd(s_effectScript);
    s_effectScript = next;
    return s_effectScript;
}

RVA(0x00004cd0, 0x7f)
i16 ExecuteEffectCommand(void) {
    void* next;
    switch (s_effectScript->opcode) {
        case EFFECT_JUMP:
            if (!s_effectScript->jump.offset) {
                s_effectScript = NULL;
                return EFFECT_COMMAND_END;
            }
            next = OffsetBy(s_effectScriptBase, s_effectScript->jump.offset);
            s_effectScript = next;
            return EFFECT_COMMAND_ADVANCE;
        case EFFECT_PALETTE:
            SetEffectPalette(s_effectScript->parameter.value);
            break;
        case EFFECT_SOUND:
            PlaySoundEffect(MapEffectSoundId(s_effectScript->parameter.value));
            break;
        default:
            return EFFECT_COMMAND_FRAME;
    }
    next = &s_effectScript->parameter + 1;
    s_effectScript = next;
    return EFFECT_COMMAND_ADVANCE;
}

RVA(0x00004d50, 0xb)
void StopEffectScript(void) {
    s_effectScript = NULL;
}

RVA(0x00004d60, 0x6)
EffectCommand* GetEffectScript(void) {
    return s_effectScript;
}

RVA(0x00004d70, 0xc7)
EffectCommand* StepScreenEffectScript(void) {
    i16 result;
    i16 count;
    i16 i;
    void* next;
    struct BmpFile* image;
    if (--s_effectDelay > 0) {
        return s_effectScript;
    }
    while ((result = ExecuteEffectCommand()) < EFFECT_COMMAND_FRAME) {
        if (result == EFFECT_COMMAND_END) {
            return s_effectScript;
        }
    }
    s_effectDelay = s_effectScript->frame.delay;
    count = ClampEffectCount(s_effectScript->frame.count);
    ClearEffectLayer(1);
    image = GetScreenEffectImage();
    if (image) {
        for (i = 0; i < count; i++) {
            DrawScreenEffectSprite(
                image,
                s_effectScript->frame.sprites[i].image,
                s_effectScript->frame.sprites[i].x,
                s_effectScript->frame.sprites[i].y
            );
        }
    }
    next = GetEffectFrameEnd(s_effectScript);
    s_effectScript = next;
    return s_effectScript;
}

// @identity-TODO: the original API name, signature and state meaning are
// unproven; the 32-bit read and storage address are explicit in retail.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00004e40, 0x6)
i32 GetLegacyEffectStateValue(void) {
    return s_legacyEffectStateValue;
}

// @identity-TODO: the original API name, signature and state meaning are
// unproven; the 32-bit read and storage address are explicit in retail.
// @dead-code
// Zero-ref: no effective rel32 caller, relocated pointer or data slot.
RVA(0x00004e50, 0x6)
i32 GetLegacyImageSetStateValue(void) {
    return s_legacyImageSetStateValue;
}

RVA(0x00004e60, 0x39)
void InitEffectImageSets(void) {
    i16 i;
    i16 frame;
    for (i = 0; i < EFFECT_IMAGE_SETS; i++) {
        s_imageSets[i].id = EFFECT_ID_NONE;
        for (frame = 0; frame < EFFECT_FRAMES; frame++) {
            s_imageSets[i].frames[frame].size = 0;
            s_imageSets[i].frames[frame].image = NULL;
        }
    }
    s_currentImageSet = EFFECT_ID_NONE;
}

RVA(0x00004ea0, 0x12)
b16 ExchangeEffectSkipping(b16 skipping) {
    b16 old = s_effectSkipping;
    s_effectSkipping = skipping;
    return old;
}

// The image set slot holding `id`, or EFFECT_ID_NONE.
RVA(0x00004ec0, 0x28)
i16 FindEffectImageSet(i16 id) {
    i16 i;
    for (i = 0; i < EFFECT_IMAGE_SETS; i++) {
        if (s_imageSets[i].id == id) {
            return i;
        }
    }
    return EFFECT_ID_NONE;
}

// Loads the frames of image set `id` (pictures 0x6000 + id * 16 + frame) into
// slot `slot`.
RVA(0x00004ef0, 0x6b)
void LoadEffectImageSet(i16 slot, i16 id) {
    ImageRequest request;
    i16 frame;
    s_imageSets[slot].id = id;
    id <<= 4;
    for (frame = 0; frame < EFFECT_FRAMES; frame++) {
        request.file = frame + id + 0x6000;
        request.variant = 0;
        request.flags = 2;
        s_imageSets[slot].frames[frame].image =
            LoadImageVariant(&request, &s_imageSets[slot].frames[frame].size);
    }
}

RVA(0x00004f60, 0x45)
void FreeEffectImageSet(i16 slot) {
    i16 frame;
    for (frame = EFFECT_FRAMES - 1; frame >= 0; frame--) {
        s_imageSets[slot].frames[frame].image =
            FreeImageFile(s_imageSets[slot].frames[frame].image);
    }
    ClearEffectLayer(0);
    s_imageSets[slot].id = EFFECT_ID_NONE;
}

// Makes image set `id` current, loading it into a free slot (else slot 0)
// when it is not cached; a cached set moves one slot toward the front.
RVA(0x00004fb0, 0xa4)
i16 AcquireEffectImageSet(i16 id) {
    EffectImageSet swap;
    i16 slot = FindEffectImageSet(id);
    if (slot > 0) {
        swap = s_imageSets[slot];
        s_imageSets[slot] = s_imageSets[slot - 1];
        s_imageSets[slot - 1] = swap;
        slot--;
    } else if (slot < 0) {
        slot = FindEffectImageSet(EFFECT_ID_NONE);
        if (slot == EFFECT_ID_NONE) {
            slot = 0;
            FreeEffectImageSet(slot);
        }
        LoadEffectImageSet(slot, id);
    }
    s_currentImageSet = slot;
    return slot;
}

// Projects motion point `x`/`y` to the screen and picks the effect frame for
// its depth (0..6 in steps of two; 2 when the near frame is missing).
RVA(0x00005060, 0xad)
i16 ProjectEffectFrame(i16 x, i16 y, i16* screenX, i16* screenY) {
    Vec3 point;
    ScreenPoint screen;
    i16 frame;
    MotionPoint(x, y, &point);
    screen = ProjectPoint(point.x, point.y, point.z);
    *screenX = screen.x;
    *screenY = screen.y;
    frame = (i16)(point.z - 56) / 450;
    if (frame < 0) {
        frame = 0;
    }
    frame *= 2;
    if (frame > 6) {
        frame = 6;
    } else if (frame < 2 && s_imageSets[s_currentImageSet].frames[frame].image == NULL) {
        frame = 2;
    }
    return frame;
}

RVA(0x00005110, 0x37)
struct BmpFile* GetEffectFrame(i32* size, i16 frame) {
    *size = s_imageSets[s_currentImageSet].frames[frame].size;
    return s_imageSets[s_currentImageSet].frames[frame].image;
}

RVA(0x00005150, 0x19)
i32 GetEffectImageBase(void) {
    return s_imageSets[s_currentImageSet].id << 4;
}

RVA(0x00005170, 0x1b)
void InitEffectSlots(void) {
    i16 i;
    for (i = 0; i < EFFECT_SLOTS; i++) {
        s_effectSlots[i].record = NULL;
        s_effectSlots[i].id = EFFECT_ID_NONE;
    }
}

RVA(0x00005190, 0x23)
i16 FindEffectSlot(i16 id) {
    i16 i;
    for (i = 0; i < EFFECT_SLOTS; i++) {
        if (s_effectSlots[i].id == id) {
            return i;
        }
    }
    return EFFECT_ID_NONE;
}

RVA(0x000051c0, 0x49)
void LoadEffectRecord(i16 slot, i16 id) {
    FILE* fp;
    s_effectSlots[slot].id = id;
    fp = OpenDataFile((id + 0x600) * 16, DATA_FILE_EFFECT, 0);
    s_effectSlots[slot].record = ReadRawAlloc(fp);
    CloseDataFile(fp);
}

RVA(0x00005210, 0x2b)
void FreeEffectRecord(i16 slot) {
    s_effectSlots[slot].record = FreeBlock(s_effectSlots[slot].record);
    s_effectSlots[slot].id = EFFECT_ID_NONE;
}

// Makes effect `id` current, like AcquireEffectImageSet.
RVA(0x00005240, 0x8c)
i16 AcquireEffectRecord(i16 id) {
    EffectSlot swap;
    i16 slot = FindEffectSlot(id);
    if (slot > 0) {
        swap = s_effectSlots[slot];
        s_effectSlots[slot] = s_effectSlots[slot - 1];
        s_effectSlots[slot - 1] = swap;
        slot--;
    } else if (slot < 0) {
        slot = FindEffectSlot(EFFECT_ID_NONE);
        if (slot == EFFECT_ID_NONE) {
            slot = 0;
            FreeEffectRecord(slot);
        }
        LoadEffectRecord(slot, id);
    }
    s_currentEffect = slot;
    return slot;
}

// Starts effect `id`: its image set, palette, shot (of `strength`), motion
// and script; returns the offset of its closing script.
RVA(0x000052d0, 0x77)
u16 StartEffect(i16 id, i16 strength) {
    EffectRecord* record = s_effectSlots[AcquireEffectRecord(id)].record;
    AcquireEffectImageSet(record->imageSet);
    SetEffectPalette(record->palette);
    StartShot(record->shotKind, strength, record->shotHeight);
    StartMotion(record->motion, record->motionDelay, record->motionScale);
    StartEffectScript((u8*)record, record->script);
    return record->closingScript;
}

// Runs the current effect's closing script with the motion and shot stopped.
RVA(0x00005350, 0x32)
void CloseEffect(void) {
    EffectRecord* record = s_effectSlots[s_currentEffect].record;
    StartEffectScript((u8*)record, record->closingScript);
    StartMotion(0, 0, 0);
    StopShot();
}

// Reads the effect palette table and applies its first pair.
RVA(0x00005390, 0x44)
void LoadEffectPalettes(FILE* fp) {
    ReadRawBlock(fp, s_paletteData);
    s_palettes = s_paletteData;
    ApplyEffectPalette(&s_palettes[0]);
}

RVA(0x000053e0, 0x26)
void ReleaseEffectPalette(void) {
    if (s_paletteRefs > 0) {
        ReleasePaletteEntry(14);
        ReleasePaletteEntry(15);
        s_paletteRefs--;
    }
}

// Sets palette entries 14/15 to effect palette `index`.
RVA(0x00005410, 0x49)
i16 SetEffectPalette(i16 index) {
    ReleaseEffectPalette();
    ApplyEffectPalette(&s_palettes[index]);
    s_paletteRefs++;
    return index;
}

RVA(0x00005460, 0x1a)
void ReleaseEffectPalettes(void) {
    while (s_paletteRefs > 0) {
        ReleaseEffectPalette();
    }
}
