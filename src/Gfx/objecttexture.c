#include <rva.h>

#include <Game/AreaNpc.h>
#include <Gfx/Texture.h>
#include <Platform/GameCalls.h>

RVA(0x00058110, 0x77)
void LoadObjectTexture(void* image, i16 slot) {
    if (slot < 0 || slot > OBJECT_TEXTURE_COUNT - 1) {
        return;
    }
    if (image == NULL) {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(&g_objectTextures[slot], "w\\npc.bmp", true);
    } else {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(&g_objectTextures[slot], image, false);
    }
}

RVA(0x00058190, 0x1f)
void ReleaseObjectTextures(void) {
    i32 slot;
    for (slot = 0; slot < OBJECT_TEXTURE_COUNT; slot++) {
        ReleaseTexture(&g_objectTextures[slot]);
    }
}
