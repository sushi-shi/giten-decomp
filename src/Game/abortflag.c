// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it.

#include <rva.h>

#include <Game/AbortFlag.h>
#include <Game/ActorFlag.h>
#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/FieldHud.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldView.h>
#include <Game/Scene.h>
#include <Game/SceneHotspot.h>
#include <Gfx/Blit.h>
#include <Mem/Handle.h>
#include <Util/BitSet.h>

DATA(0x00083ca0)
static SceneHotspot s_hotspots[32] = {0};

DATA(0x000840e0)
static i16 s_hotspotCount = 0;

// @identity-TODO: a pending-abort flag: the next query of the list count
// (0x45680) returns -1 instead and clears it; callers set it around nested
// work.
DATA(0x000840e4)
static b16 s_abortPending = false;

DATA(0x00091240)
i16 g_viewLateral;

DATA(0x00091242)
i16 g_viewDepth;

// @identity-TODO: only the empty legacy blitters read these origin words.
DATA(0x00091248)
i16 g_spriteOriginX;

DATA(0x0009124a)
i16 g_spriteOriginY;

DATA(0x000912d0)
i16 g_viewCellX;

DATA(0x000912d2)
i16 g_viewCellY;

DATA(0x000912d4)
i16 g_viewFacing;

// @identity-TODO: the Windows port never initializes these clipping words;
// only their argument order at the legacy blitter calls is known.
DATA(0x000912d6)
i16 g_spriteClipRight;

DATA(0x000912d8)
i16 g_spriteClipBottom;

DATA(0x000912da)
i16 g_spriteClipLeft;

DATA(0x000912dc)
i16 g_spriteClipTop;

RVA(0x00045550, 0x7)
b16 IsAbortPending(void) {
    return s_abortPending;
}

RVA(0x00045560, 0xc)
void SetAbortPending(b16 pending) {
    s_abortPending = pending;
}

RVA(0x00045570, 0x31)
void FlushPlaneUpdates(void) {
    i16 count = s_hotspotCount;
    i16 i;
    s_hotspotCount = 0;
    for (i = 0; i < count; i++) {
        FreeHandle(s_hotspots[i].imageHandle);
    }
}

RVA(0x000455b0, 0xa7)
i16 AddSceneHotspot(void* object, GZ_ENUM_PARAM(SceneHotspotKind, i16) kind, SceneSprite* sprite) {
    if (s_hotspotCount >= 31) {
        return -1;
    }
    s_hotspots[s_hotspotCount].object = object;
    s_hotspots[s_hotspotCount].kind = kind;
    switch (kind) {
        case 0:
        case 1:
        case SCENE_HOTSPOT_OBJECT:
        case SCENE_HOTSPOT_BOX:
        case 4:
        case 5:
            s_hotspots[s_hotspotCount].sprite = *sprite;
            s_hotspots[s_hotspotCount].imageHandle =
                CopySpriteBitmap(s_hotspots[s_hotspotCount].sprite.image);
            s_hotspots[s_hotspotCount].position.x = g_viewCellX;
            s_hotspots[s_hotspotCount].position.y = g_viewCellY;
            break;
    }
    return s_hotspotCount++;
}

RVA(0x00045660, 0x12)
b16 ExchangeAbortPending(b16 pending) {
    b16 prev = s_abortPending;
    s_abortPending = pending;
    return prev;
}

RVA(0x00045680, 0x59)
i16 CountHotspotsOfKind(GZ_ENUM_PARAM(SceneHotspotKind, i16) kind, b16 consume) {
    i32 count;
    i16 i;
    if (kind == SCENE_HOTSPOT_OBJECT && s_abortPending && consume) {
        count = -1;
    } else {
        count = 0;
        for (i = 0; i < s_hotspotCount; i++) {
            if (s_hotspots[i].kind == kind) {
                count++;
            }
        }
    }
    s_abortPending = false;
    return count;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x000456e0, 0xf3)
i16 PickSceneHotspot(i16 x, i16 y) {
    SceneSprite sprite;
    b16 button = PollScenePointer(x, y, &x, &y);
    i16 index;
    if (button < 0) {
        return button;
    }
    for (index = s_hotspotCount - 1; index >= 0; index--) {
        if (s_hotspots[index].kind >= 0) {
            SpriteBitmap* image;
            i16 hit;
            sprite = s_hotspots[index].sprite;
            image = HandleReadPtr(s_hotspots[index].imageHandle);
            if (sprite.flags & SCENE_SPRITE_INTERACTIVE) {
                hit = TestSceneSpritePixel(
                    sprite.imageHandle,
                    image,
                    sprite.x,
                    sprite.y,
                    sprite.flags,
                    sprite.offsetX,
                    sprite.offsetY,
                    x,
                    y
                );
            } else {
                hit = 0;
            }
            if (hit > 0) {
                return (button << 5) + index;
            }
        }
    }
    return -1;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x000457e0, 0x15)
GZ_ENUM_RETURN(SceneHotspotKind, i16) GetHotspotKind(i16 index) {
    return s_hotspots[index].kind;
}

RVA(0x00045800, 0x1b)
SceneScript GetNpcScript(AreaNpc* npc) {
    SceneScript result;
    result.script = npc->script;
    result.entry = npc->entry;
    return result;
}

RVA(0x00045820, 0x26)
SceneScript BeginBoxScene(TreasureBox* box) {
    SceneScript result;
    result.script = box->dest[0];
    result.entry = box->dest[1];
    SetSceneCell(box);
    return result;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x00045850, 0x7f)
SceneScript GetHotspotScript(i16 index) {
    SceneScript result = {0, 0};
    if (s_hotspots[index].kind == SCENE_HOTSPOT_BOX) {
        TreasureBox* box =
            FindTreasureBoxAt(s_hotspots[index].sprite.cellX, s_hotspots[index].sprite.cellY, 1);
        if (box != NULL) {
            result.script = box->dest[0];
            result.entry = box->dest[1];
        }
    } else {
        result.script = s_hotspots[index].npc->script;
        result.entry = s_hotspots[index].npc->entry;
    }
    return result;
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x000458d0, 0x52)
void DrawSceneObjects(i16 x, i16 y, i16 across, i16 along) {
    g_viewLateral = across;
    g_viewDepth = along;
    g_viewCellX = x;
    g_viewCellY = y;
    OffsetMapCoord(&g_viewCellX, &g_viewCellY, g_viewFacing, across, along);
    DrawFieldObjects();
    DrawAreaNpcs();
}

static __inline b32 IsSceneObjectVisible(void* object, i16 kind) {
    FieldObject* fieldObject = object;
    return (kind != SCENE_HOTSPOT_OBJECT
            || TestFieldObjectFlag(fieldObject, ACTOR_FLAG_INVISIBLE) != true)
           && !GetObjectsHidden();
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x00045930, 0x1b7)
i16 DrawSceneSprite(
    i16 mode,
    SceneSprite* sprite,
    void* object,
    GZ_ENUM_PARAM(SceneHotspotKind, i16) kind,
    i16 centered
) {
    i16 x;
    i16 y;
    if (sprite == NULL || object == NULL) {
        return -1;
    }
    x = sprite->x;
    y = sprite->y;
    if (mode == 0) {
        if (IsSceneObjectVisible(object, kind)) {
            BlitObjectSprite(
                sprite->image,
                sprite->imageHandle,
                x,
                y,
                g_spriteOriginX,
                g_spriteOriginY,
                g_spriteClipLeft,
                g_spriteClipTop,
                g_spriteClipRight,
                g_spriteClipBottom,
                sprite->flags,
                sprite->offsetX,
                sprite->offsetY
            );
        }
        sprite->flags |= SCENE_SPRITE_INTERACTIVE;
        return AddSceneHotspot(object, kind, sprite);
    }
    if (centered) {
        if (sprite->flags & SCENE_SPRITE_FLIP_X) {
            x = -1 - x;
        }
        if (sprite->flags & SCENE_SPRITE_FLIP_Y) {
            y = -1 - y;
        }
        x += 40;
        y += 112;
    } else {
        if (sprite->flags & SCENE_SPRITE_FLIP_X) {
            x = 79 - x;
        }
        if (sprite->flags & SCENE_SPRITE_FLIP_Y) {
            y = 399 - y;
        }
    }
    x += sprite->offsetX;
    y += sprite->offsetY;
    if (IsSceneObjectVisible(object, kind)) {
        BlitSceneSprite(
            sprite->image,
            sprite->imageHandle,
            x,
            y,
            g_spriteOriginX,
            g_spriteOriginY,
            g_spriteClipLeft,
            g_spriteClipTop,
            g_spriteClipRight,
            g_spriteClipBottom,
            sprite->flags
        );
    }
    return AddSceneHotspot(object, kind, sprite);
}

// @dead-code
// Zero-ref: no rel32 call/jmp, relocated reference or data slot reaches it.
RVA(0x00045af0, 0x1c)
i16 GetHotspotObjectSlot(i16 index) {
    return s_hotspots[index].fieldObject->slot;
}

RVA(0x00045b10, 0x14)
SceneSprite* GetHotspotSprite(i16 index) {
    return &s_hotspots[index].sprite;
}

// @identity-TODO: the two input words are unread on Windows; this legacy
// pointer query returns zero and leaves its output coordinates unchanged.
RVA(0x00045b30, 0x4)
b16 PollScenePointer(i16 x, i16 y, i16* pointX, i16* pointY) {
    return false;
}
