#ifndef GITEN_PLATFORM_GAMEAPI_H
#define GITEN_PLATFORM_GAMEAPI_H

// The C game headers the C++ platform layer calls into, with C linkage (the
// game code is C; these declarations name its symbols unmangled).
extern "C" {
#include <Game/AreaMap.h>
#include <Game/AutomapData.h>
#include <Game/Field.h>
#include <Game/FieldHud.h>
#include <Game/FieldMain.h>
#include <Game/FieldObject.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/PartyPick.h>
#include <Game/ScreenEffect.h>
#include <Game/TreasureBox.h>
#include <Gfx/Bitmap.h>
#include <Gfx/D3DState.h>
#include <Gfx/DDraw.h>
#include <Gfx/DisplayConfig.h>
#include <Gfx/Motion.h>
#include <Gfx/Render.h>
#include <Gfx/Scene.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/ScreenMode.h>
#include <Gfx/Shot.h>
#include <Gfx/Sprite.h>
#include <Gfx/Vram.h>
#include <Input/Mouse.h>
#include <Mem/Alloc.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Sound/Sound.h>
#include <Text/TextPlane.h>
#include <Ui/Hotspot.h>
#include <Util/Debug.h>
#include <Util/Range.h>
}

#endif // GITEN_PLATFORM_GAMEAPI_H
