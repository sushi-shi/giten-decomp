#include <rva.h>

#include <Game/AreaLevel.h>
#include <Gfx/Texture.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>

DATA(0x00084800)
Texture g_roomTexture;

DATA(0x0006ddc8)
static const char* s_wallTextureNames[16][4] = {
    {"w\\wall00_0.bmp", "w\\wall00_1.bmp", "w\\wall00_2.bmp", "w\\wall00_3.bmp"},
    {"w\\wall01_0.bmp", "w\\wall01_0.bmp", "w\\wall01_0.bmp", "w\\wall01_0.bmp"},
    {"w\\wall02_0.bmp", "w\\wall02_1.bmp", "w\\wall02_2.bmp", "w\\wall02_0.bmp"},
    {"w\\wall03_0.bmp", "w\\wall03_1.bmp", "w\\wall03_0.bmp", "w\\wall03_1.bmp"},
    {"w\\wall04_0.bmp", "w\\wall04_1.bmp", "w\\wall04_2.bmp", "w\\wall04_0.bmp"},
    {"w\\wall05_0.bmp", "w\\wall05_0.bmp", "w\\wall05_0.bmp", "w\\wall05_0.bmp"},
    {"w\\wall06_0.bmp", "w\\wall06_1.bmp", "w\\wall06_2.bmp", "w\\wall06_3.bmp"},
    {"w\\wall07_0.bmp", "w\\wall07_0.bmp", "w\\wall07_0.bmp", "w\\wall07_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall10_0.bmp", "w\\wall10_0.bmp", "w\\wall10_0.bmp", "w\\wall10_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
    {"w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp", "w\\wall08_0.bmp"},
};

RVA(0x00057e80, 0x94)
void LoadWallTextures(i16 wallSet, i16 variant) {
    u8 area;
    u8 level;
    ReleaseTexture(&g_roomTexture);
    g_fixedLighting = FALSE;
    if ((wallSet & 0xf) == WALL_TEXTURE_UNLIT) {
        g_fixedLighting = TRUE;
    }
    if (wallSet == WALL_TEXTURE_MAP_OVERRIDE) {
        area = GetMapArea();
        level = GetMapLevel();
        if ((area == WALL_OVERRIDE_UPPER_AREA && level > 1)
            || (area == WALL_OVERRIDE_LOWER_AREA && level < 4)) {
            LoadTexture(&g_roomTexture, "w\\wall11_0.bmp", TRUE);
            return;
        }
    }
    LoadTexture(&g_roomTexture, s_wallTextureNames[wallSet & 0xf][variant & 3], TRUE);
}
