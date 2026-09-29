#ifndef GITEN_GAME_SCENEHOTSPOT_H
#define GITEN_GAME_SCENEHOTSPOT_H

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/AreaNpc.h>
#include <Game/MapCoord.h>
#include <Game/SceneHotspotKind.h>
#include <Gfx/SpriteBitmap.h>
#include <Ints.h>

GZ_ENUM_BEGIN(SceneSpriteFlags)
    SCENE_SPRITE_INTERACTIVE = 1,
    SCENE_SPRITE_FLIP_Y = 0x4000,
    SCENE_SPRITE_FLIP_X = -32768
GZ_ENUM_END(SceneSpriteFlags)

struct FieldObject;

// The sprite description copied whole into each scene hotspot.
typedef struct SceneSprite {
    i32 imageHandle;
    SpriteBitmap* image;
    i16 x;
    i16 y;
    i16 offsetX;
    i16 offsetY;
    GZ_ENUM_STORAGE(SceneSpriteFlags, u16) flags;
    u8 cellX;
    u8 cellY;
} SceneSprite;

typedef struct SceneHotspot {
    GZ_ENUM_STORAGE(SceneHotspotKind, i16) kind;
    union {
        void* object;
        struct AreaNpc* npc;
        struct FieldObject* fieldObject;
    };
    SceneSprite sprite;
    i32 imageHandle;
    MapCoord position;
} SceneHotspot;

i16 AddSceneHotspot(void* object, GZ_ENUM_PARAM(SceneHotspotKind, i16) kind, SceneSprite* sprite);

i16 PickSceneHotspot(i16 x, i16 y);
b16 PollScenePointer(i16 x, i16 y, i16* pointX, i16* pointY);
GZ_ENUM_RETURN(SceneHotspotKind, i16) GetHotspotKind(i16 index);
i16 GetHotspotObjectSlot(i16 index);
SceneSprite* GetHotspotSprite(i16 index);
SceneScript GetHotspotScript(i16 index);
void DrawSceneObjects(i16 x, i16 y, i16 across, i16 along);
i16 DrawSceneSprite(
    i16 mode,
    SceneSprite* sprite,
    struct FieldObject* object,
    GZ_ENUM_PARAM(SceneHotspotKind, i16) kind,
    i16 centered
);

#endif // GITEN_GAME_SCENEHOTSPOT_H
