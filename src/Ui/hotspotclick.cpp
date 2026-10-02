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
b32 ClickHotspotAt(const i32 x, const i32 y) {
    if (GetTextPlane(0)->visible) {
        return false;
    }
    if (y > VIEW_HEIGHT - 1) {
        return false;
    }
    i32 hit = HOTSPOT_NONE;
    const GZ_ENUM_LOCAL(ViewDirection, i16) direction = GetMapPosition()->direction;
    for (i32 i = g_hotspotCount - 1; i >= 0; i--) {
        const Hotspot* candidate = GetHotspot(i);
        if (candidate->rect.left > x || candidate->rect.right <= x
            || candidate->rect.top > y || candidate->rect.bottom <= y) {
            continue;
        }
        const Texture* texture = candidate->texture;
        const u8* pixels = GetBitmapPixels(texture->image);
        const u32 width = texture->width;
        const u32 height = min(256, width);
        i32 u = (x - candidate->rect.left) * width
            / (candidate->rect.right - candidate->rect.left);
        i32 v = height
            - (y - candidate->rect.top) * height
                  / (candidate->rect.bottom - candidate->rect.top)
            - 1;
        ClampHotspotTexel(u, texture->width);
        ClampHotspotTexel(v, static_cast<i32>(height));
        if (pixels[v * width + u] == BMP_TRANSPARENT_INDEX) {
            continue;
        }
        if (hit != HOTSPOT_NONE) {
            const Hotspot* selected = GetHotspot(hit);
            i32 candidateCoord;
            i32 selectedCoord;
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
    const Hotspot* hotspot = GetHotspot(hit);
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
