#ifndef GITEN_GFX_MOTION_H
#define GITEN_GFX_MOTION_H

#include <Enums.h>
#include <Gfx/EffectImageCode.h>
#include <Gfx/Vram.h>
#include <Ints.h>
#include <Math/Vec3.h>

#include <stdio.h>

// clang-format off
GZ_ENUM_BEGIN(EffectOpcode)
    EFFECT_SOUND = 0xfd,
    EFFECT_PALETTE = 0xfe,
    EFFECT_JUMP = 0xff
GZ_ENUM_END(EffectOpcode);
// clang-format on

GZ_ENUM_BEGIN(EffectCommandResult)
    EFFECT_COMMAND_ADVANCE = -1,
    EFFECT_COMMAND_END = 0,
    EFFECT_COMMAND_FRAME = 1
GZ_ENUM_END(EffectCommandResult)
;

typedef struct EffectSprite {
    EffectImageCode image;
    i16 x;
    i16 y;
} EffectSprite;

// clang-format off
GZ_ENUM_BEGIN(EffectImageBase)
    EFFECT_IMAGE_LATERAL_ADJUST = 0x100
GZ_ENUM_END(EffectImageBase);
// clang-format on

typedef struct EffectFrameCommand {
    u8 delay;
    u8 count;
    EffectSprite sprites[1];
} EffectFrameCommand;

typedef struct EffectJumpCommand {
    u8 opcode;
    u16 offset;
} EffectJumpCommand;

typedef struct EffectParameterCommand {
    u8 opcode;
    u8 value;
} EffectParameterCommand;

typedef union EffectCommand {
    u8 opcode;
    EffectFrameCommand frame;
    EffectJumpCommand jump;
    EffectParameterCommand parameter;
} EffectCommand;

// A motion: `count` steps of (x, y) offsets in 2.14 fixed point.
typedef struct MotionStep {
    i16 x;
    i16 y;
} MotionStep;

typedef struct MotionPath {
    u16 count;
    MotionStep steps[1];
} MotionPath;

static __inline MotionStep* GetMotionPathStep(MotionPath* path, u16 index) {
    return &path->steps[index];
}

// The loaded motion table keeps its path pointers right after the count, on
// a 2-byte boundary (retail reads them at +2); it has room for seven paths.
typedef struct MotionTable {
    u16 count;
    MotionPath* paths[7];
} MotionTable;

// The motion file block: path count, byte offsets from the block start, then
// the paths. The load buffer has 16-byte capacity.
typedef union MotionFile {
    struct {
        u16 count;
        u16 offsets[1];
    } table;
    u8 bytes[16];
} MotionFile;

extern MotionFile g_motionFile;
extern MotionTable g_loadedMotionTable;

// A battle effect: its record (data file kind 2, id (0x600 + effect) * 16),
// which the effect script follows (script offsets are from the record
// start), and a set of seven depth frames (pictures 0x6000 + set * 16 +
// frame). Both are cached in one slot each; `EFFECT_FRAMES` frames of a set
// are loaded.
// @identity-TODO: the record fields are named from StartEffect/CloseEffect,
// which pass them on; `frames` has room for ten.
#define EFFECT_SLOTS 1
#define EFFECT_IMAGE_SETS 1
// The id of a free record or image-set slot, and what the finders return when
// no slot holds an id.
#define EFFECT_ID_NONE (-1)
#define EFFECT_FRAMES 7

typedef struct EffectRecord {
    u8 pad00[4];
    i16 palette;
    u16 script;
    u16 closingScript;
    u8 pad0a[2];
    i16 shotHeight;
    u8 pad0e[2];
    i16 shotKind;
    i16 motion;
    i16 motionDelay;
    i16 motionScale;
    i16 imageSet;
} EffectRecord;

typedef struct EffectSlot {
    i16 id;
    EffectRecord* record;
} EffectSlot;

struct BmpFile;

typedef struct EffectFrame {
    i32 size;
    struct BmpFile* image;
} EffectFrame;

typedef struct EffectImageSet {
    i16 id;
    EffectFrame frames[10];
} EffectImageSet;

// The palette entries 14 and 15 an effect uses.
typedef struct EffectPalette {
    i16 colors[2];
} EffectPalette;

// Re-evaluate the palette pointer after updating the first entry.
#define ApplyEffectPalette(palette)                                                                \
    (SetPaletteEntry(PALETTE_SIZE - 2, (palette)->colors[0]),                                      \
     SetPaletteEntry(PALETTE_SIZE - 1, (palette)->colors[1]))

void SetSceneFlags(i32 bits);
i16 GetSceneFlags(void);
void StartMotion(i16 index, i16 delay, i16 scale);
b16 StepMotion(i16 immediate);
void MotionPoint(i16 x, i16 y, Vec3* out);
i32 ClampEffectCount(i16 count);
void StartEffectScript(u8* base, u16 offset);
void StopEffectScript(void);
EffectCommand* GetEffectScript(void);
EffectCommand* StepEffectScript(void);
EffectCommand* StepScreenEffectScript(void);
GZ_ENUM_RETURN(EffectCommandResult, i16) ExecuteEffectCommand(void);

void InitEffectImageSets(void);
b16 ExchangeEffectSkipping(b16 skipping);
i16 FindEffectImageSet(i16 id);
void LoadEffectImageSet(i16 slot, i16 id);
void FreeEffectImageSet(i16 slot);
i16 AcquireEffectImageSet(i16 id);
i16 ProjectEffectFrame(i16 x, i16 y, i16* screenX, i16* screenY);
struct BmpFile* GetEffectFrame(i32* size, i16 frame);
i32 GetEffectImageBase(void);
void InitEffectSlots(void);
i16 FindEffectSlot(i16 id);
void LoadEffectRecord(i16 slot, i16 id);
void FreeEffectRecord(i16 slot);
i16 AcquireEffectRecord(i16 id);
u16 StartEffect(i16 id, i16 strength);
void CloseEffect(void);
void LoadEffectPalettes(FILE* fp);
void ReleaseEffectPalette(void);
i16 SetEffectPalette(i16 index);
void ReleaseEffectPalettes(void);

void ResetSceneInput(void);

void LoadMotionTable(FILE* fp);

#endif // GITEN_GFX_MOTION_H
