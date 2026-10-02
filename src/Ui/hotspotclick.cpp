// @identity-TODO: the original owning TU remains unproven.

#include <rva.h>

#include <EnumDomain.h>
#include <Game/ViewDirection.h>
#include <Gfx/Texture.h>
#include <Platform/GameApi.h>
#include <Platform/Scene3D.h>
#include <Ui/Hotspot.h>

#define ClampHotspotTexel(value, extent)                                                           \
    do {                                                                                           \
        if ((value) < 0) {                                                                         \
            (value) = 0;                                                                           \
        } else if ((value) > (extent) - 1) {                                                       \
            (value) = (extent) - 1;                                                                \
        }                                                                                          \
    } while (0)

RVA(0x00059180, 0x24f)
b32 ClickHotspotAt(i32 x, i32 y) {
    i32 hit;
    i32 i;
    GZ_ENUM_LOCAL(ViewDirection, i16) direction;
    Texture* texture;
    u8* pixels;
    u32 width;
    u32 height;
    i32 u;
    i32 v;
    i32 candidateCoord;
    i32 selectedCoord;

    if (GetTextPlane(0)->visible) {
        return false;
    }
    if (y > VIEW_HEIGHT - 1) {
        return false;
    }
    hit = HOTSPOT_NONE;
    direction = GetMapPosition()->direction;
    for (i = g_hotspotCount - 1; i >= 0; i--) {
        Hotspot* candidate = GetHotspot(i);
        if (candidate->rect.left > x || candidate->rect.right <= x
            || candidate->rect.top > y || candidate->rect.bottom <= y) {
            continue;
        }
        texture = candidate->texture;
        pixels = GetBitmapPixels(texture->image);
        width = texture->width;
        height = min(256, width);
        u = (x - candidate->rect.left) * width
            / (candidate->rect.right - candidate->rect.left);
        v = height
            - (y - candidate->rect.top) * height
                  / (candidate->rect.bottom - candidate->rect.top)
            - 1;
        ClampHotspotTexel(u, texture->width);
        ClampHotspotTexel(v, static_cast<i32>(height));
        if (pixels[v * width + u] == BMP_TRANSPARENT_INDEX) {
            continue;
        }
        if (hit != HOTSPOT_NONE) {
            Hotspot* selected = GetHotspot(hit);
            switch (direction) {
                case VIEW_NORTH:
                    selectedCoord = selected->targetY;
                    candidateCoord = candidate->targetY;
                    goto nearerGreater;
                case VIEW_EAST:
                    selectedCoord = selected->targetX;
                    candidateCoord = candidate->targetX;
                    goto nearerLess;
                case VIEW_SOUTH:
                    selectedCoord = selected->targetY;
                    candidateCoord = candidate->targetY;
                    goto nearerLess;
                default:
                    selectedCoord = selected->targetX;
                    candidateCoord = candidate->targetX;
                    goto nearerGreater;
            nearerGreater:
                if (candidateCoord > selectedCoord) {
                    hit = i;
                }
                break;
            nearerLess:
                if (candidateCoord < selectedCoord) {
                    hit = i;
                }
                break;
            }
        } else {
            hit = i;
        }
    }
    if (hit < 0) {
        return false;
    }
    Hotspot* hotspot = GetHotspot(hit);
    switch (hotspot->kind) {
        case HOTSPOT_BOX:
            StartBoxScene(static_cast<TreasureBox*>(hotspot->data));
            return true;
        case HOTSPOT_NPC:
            StartNpcScene(static_cast<AreaNpc*>(hotspot->data));
            return true;
        case HOTSPOT_TARGET:
            if (hit == g_selectedHotspot) {
                TalkCommand();
                return true;
            }
            if (!AnyObjectInReach() || IsPartyAt(hotspot->targetX, hotspot->targetY)) {
                g_selectedHotspot = hit;
            }
            break;
    }
    return true;
}
