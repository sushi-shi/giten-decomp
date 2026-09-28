// @identity-TODO: the original owning TU remains unproven.

#include <rva.h>

#include <Gfx/Texture.h>
#include <Platform/GameApi.h>

#define HOTSPOT_VIEW_BOTTOM 328

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
    i16 direction;
    Hotspot* hotspot;
    Texture* texture;
    u8* pixels;
    u32 width;
    u32 height;
    i32 u;
    i32 v;

    if (GetTextPlane(0)->visible) {
        return FALSE;
    }
    if (y > HOTSPOT_VIEW_BOTTOM - 1) {
        return FALSE;
    }
    hit = -1;
    direction = GetMapPosition()->direction;
    for (i = g_hotspotCount - 1; i >= 0; i--) {
        if (GetHotspot(i)->rect.left > x || GetHotspot(i)->rect.right <= x
            || GetHotspot(i)->rect.top > y || GetHotspot(i)->rect.bottom <= y) {
            continue;
        }
        texture = GetHotspot(i)->texture;
        pixels = GetBitmapPixels(texture->image);
        width = texture->width;
        height = min(256, width);
        u = (x - GetHotspot(i)->rect.left) * width
            / (GetHotspot(i)->rect.right - GetHotspot(i)->rect.left);
        v = height
            - (y - GetHotspot(i)->rect.top) * height
                  / (GetHotspot(i)->rect.bottom - GetHotspot(i)->rect.top)
            - 1;
        ClampHotspotTexel(u, texture->width);
        ClampHotspotTexel(v, static_cast<i32>(height));
        if (pixels[v * width + u] == BMP_TRANSPARENT_INDEX) {
            continue;
        }
        if (hit != -1) {
            hotspot = GetHotspot(hit);
            switch (direction) {
                case VIEW_NORTH:
                    if (GetHotspot(i)->targetY > hotspot->targetY) {
                        hit = i;
                    }
                    break;
                case VIEW_EAST:
                    if (GetHotspot(i)->targetX < hotspot->targetX) {
                        hit = i;
                    }
                    break;
                case VIEW_SOUTH:
                    if (GetHotspot(i)->targetY < hotspot->targetY) {
                        hit = i;
                    }
                    break;
                default:
                    if (GetHotspot(i)->targetX > hotspot->targetX) {
                        hit = i;
                    }
                    break;
            }
        } else {
            hit = i;
        }
    }
    if (hit < 0) {
        return FALSE;
    }
    hotspot = GetHotspot(hit);
    switch (hotspot->kind) {
        case HOTSPOT_BOX:
            StartBoxScene(static_cast<TreasureBox*>(hotspot->data));
            return TRUE;
        case HOTSPOT_NPC:
            StartNpcScene(static_cast<AreaNpc*>(hotspot->data));
            return TRUE;
        case HOTSPOT_TARGET:
            if (hit == g_selectedHotspot) {
                TalkCommand();
                return TRUE;
            }
            if (!AnyObjectInReach() || IsPartyAt(hotspot->targetX, hotspot->targetY)) {
                g_selectedHotspot = hit;
            }
            break;
    }
    return TRUE;
}
