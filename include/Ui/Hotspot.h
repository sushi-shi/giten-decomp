#ifndef GITEN_UI_HOTSPOT_H
#define GITEN_UI_HOTSPOT_H

#include <rva.h>

#include <Win32.h>

#include <EnumDomain.h>
#include <Ints.h>

// Destination, NPC and treasure-box hotspots in the Windows view. NPCs on
// the party's cell or directly ahead temporarily use 14/15 while rendering
// orders the nearest billboards, then become ordinary NPC hotspots.
GZ_ENUM_BEGIN_SPLIT(UiHotspotKind, u32)
    HOTSPOT_TARGET = 1,
    HOTSPOT_NPC = 2,
    HOTSPOT_BOX = 3,
    HOTSPOT_NPC_HERE = 14,
    HOTSPOT_NPC_AHEAD = 15
GZ_ENUM_END_SPLIT(UiHotspotKind)

// A clickable area of the current screen: its kind (the list is kept sorted by
// kind; kind 1 areas lead somewhere), its rectangle, the value it reports and
// the target position it leads to. Treasure boxes (kind 3) also carry their
// texture and box record.
// The data pointer carries the object record selected by kind.
struct Texture;
typedef struct Hotspot {
    GZ_ENUM_STORAGE(UiHotspotKind, u32) kind;
    RECT rect;
    struct Texture* texture;
    void* data;
    i32 value;
    i32 targetX;
    i32 targetY;
} Hotspot;

extern Hotspot g_hotspots[64];

#define GetHotspot(index) (&g_hotspots[(index)])

// The number of live entries in g_hotspots.
extern u32 g_hotspotCount;

// The selected hotspot, or HOTSPOT_NONE.
#define HOTSPOT_NONE (-1)
extern i32 g_selectedHotspot;

void ClearSelectedHotspot(void);
void SetSelectedHotspot(i32 index);
i16 GetSelectedHotspotValue(void);

RVA_DECL(0x00058640)
i16 CountFieldObjects(void);

#endif // GITEN_UI_HOTSPOT_H
