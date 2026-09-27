#include <rva.h>

#include <Game/AreaNpc.h>
#include <Gfx/Texture.h>
#include <Platform/GameCalls.h>

DATA(0x0008d860)
Texture g_objectTextures[6];

RVA(0x00058110, 0x77)
void LoadObjectTexture(void* image, i16 slot) {
    if (slot < 0 || slot > 5) {
        return;
    }
    if (image == NULL) {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(&g_objectTextures[slot], "w\\npc.bmp", TRUE);
    } else {
        ReleaseTexture(&g_objectTextures[slot]);
        LoadTexture(&g_objectTextures[slot], image, FALSE);
    }
}

RVA(0x00058190, 0x1f)
void ReleaseObjectTextures(void) {
    i32 slot;
    for (slot = 0; slot < 6; slot++) {
        ReleaseTexture(&g_objectTextures[slot]);
    }
}
