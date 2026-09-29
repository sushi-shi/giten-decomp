#ifndef GITEN_GAME_AREANPC_H
#define GITEN_GAME_AREANPC_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Ints.h>

// A 42-byte record of an NPC placed in the current area: its cell, picture,
// the event flag that marks it gone, and the values its map record carries.
// `script`/`entry` are the scene script StartNpcScene runs.
// @identity-TODO: the first 0x14 bytes (passed whole to 0x41f6d0 when the NPCs
// are drawn) and the legacy `image` code are unrecovered.
typedef struct AreaNpc {
    u8 pad00[0x14];
    i16 x;
    i16 y;
    i16 image;
    u8 pad1a[4];
    i16 flagBank;
    i16 flagIndex;
    i16 script;
    i16 entry;
    u8 pad26[2];
    i16 textureSlot;
} AreaNpc;

// An NPC picture's texture, after a word the table starts as 0xffff.
// @identity-TODO: what `other` holds is unrecovered.
typedef struct NpcTexture {
    u32 other;
    u32 texture;
} NpcTexture;

// Twelve records; only the first six are used by the readers and loader.
extern NpcTexture g_npcTextures[12];

// The map cell the field view is drawing.
extern i16 g_viewCellX;
extern i16 g_viewCellY;

// The scene script (file, entry) a cell or an NPC starts.
typedef struct SceneScript {
    i16 script;
    i16 entry;
} SceneScript;

struct TreasureBox;

// Latches a treasure box's cell as the scene cell and returns the script its
// bytes 5/6 name.
SceneScript BeginBoxScene(struct TreasureBox* box);

// An NPC's scene script (its words +0x22/+0x24).
SceneScript GetNpcScript(AreaNpc* npc);

// The room-region grid and its helpers (areanpc.c).
void FillRegionRect(i16 x0, i16 y0, i16 x1, i16 y1, u8 value);
void SetRoomRegion(i16 x, i16 y, u8 value);
void SetGridByte(i32* grid, i16 x, i16 y, u8 value);
void FillEmptyRegions(i16 width, i16 height, u8 value);
u8 GetRoomRegion(i16 x, i16 y);
u8 GetGridByte(i32* grid, i16 x, i16 y);
b16 IsRegionFlagOn(u8* list, i16 offset);
void MarkRegionList(u8* list, i16 stride, u8 code, i16 width, i16 height);
u8* FindRegionData(u8* list, i16 stride, i16 index);
void SetPrevRegion(i16 x, i16 y, u8 value);
u8 GetPrevRegion(i16 x, i16 y);
u8* GetRoomData(i16 code);
void EnterRoom(i16 code);

i16 FindCellObject(i16 id, i16 x, i16 y);
i16 NextNpcSlot(void);
void CountPlacedNpc(void);
i16 GetNpcImageOfCode(i16 code);
void ClearAreaNpcs(void);
void AddAreaNpc(const u8* record);
void MarkAreaNpcs(void);
void ReleaseNpcTextures(void);
void LoadNpcTexture(i16 slot, i16 code, i16 mode);
u16* LoadNpcPalette(u16* colors);
void LoadAreaNpcImages(u8* record);

// What a skill's field effect did: failed, had no effect, or was done.
GZ_ENUM_BEGIN(FieldEffectResult)
    FIELD_EFFECT_FAILED = -1,
    FIELD_EFFECT_NONE = 0,
    FIELD_EFFECT_DONE = 1
GZ_ENUM_END(FieldEffectResult)

GZ_ENUM_RETURN(FieldEffectResult, i16) RunFieldEffect(i16 effect);
GZ_ENUM_RETURN(FieldEffectResult, i16) KnockBack(i16 who);
GZ_ENUM_RETURN(FieldEffectResult, i16) ShieldTarget(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) SetTargetFlag21(void);
b16 ScatterObjects(void);
b16 ReturnToLeaderWarp(void);
b16 ReturnToLeaderMark(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) KnockBackActor(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) SpawnActorGroup(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) SealTarget(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag23(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag25(void);
GZ_ENUM_RETURN(FieldEffectResult, i16) RaiseTargetFlag26(void);

// fieldscreen's scene refresh.
void RefreshFieldScene(void);

// vram's palette calls, declared here rather than through <Gfx/Vram.h>:
// retail's NPC palette loader pushes the GRB word unextended, so its
// declaration of GrbToRgb took a 16-bit word while vram.c defines it on a
// u32 (see docs/todos/rule-exceptions.tsv).
b16 SetPaletteColor(u8 index, i16 color);
u32 GrbToRgb(u16 grb);

// The six object textures (g_objectTextures): load `image` into slot `slot`
// (0..5), and release them all.
// @identity-TODO: label-only until the texture TU claims them.
RVA_DECL(0x00058110)
void LoadObjectTexture(void* image, i16 slot);
RVA_DECL(0x00058190)
void ReleaseObjectTextures(void);
void DrawAreaNpcs(void);
u32 GetNpcTexture(i16 slot);
u32 DrawNpcAt(i16 x, i16 y, i16 depth, AreaNpc* npc, i16 index);

#endif // GITEN_GAME_AREANPC_H
