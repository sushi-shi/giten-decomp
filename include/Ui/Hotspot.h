#ifndef GITEN_UI_HOTSPOT_H
#define GITEN_UI_HOTSPOT_H

#include <rva.h>

#include <Win32.h>

#include <Ints.h>

// A clickable area of the current screen: its kind (the list is kept sorted by
// kind; kind 1 areas lead somewhere), its rectangle, the value it reports and
// the target position it leads to. Treasure boxes (kind 3) also carry their
// texture and box record.
// The data pointer carries the object record selected by kind.
struct Texture;
typedef struct Hotspot {
    u32 kind;
    RECT rect;
    struct Texture* texture;
    void* data;
    i32 value;
    i32 targetX;
    i32 targetY;
} Hotspot;

// Hotspot kinds: an object or place the area leads to, an NPC, a treasure
// box, and (while the 3D view sorts them) an NPC on the party's cell or in
// front of it.
#define HOTSPOT_TARGET 1
#define HOTSPOT_NPC 2
#define HOTSPOT_BOX 3
#define HOTSPOT_NPC_HERE 14
#define HOTSPOT_NPC_AHEAD 15

extern Hotspot g_hotspots[64];

#define GetHotspot(index) (&g_hotspots[(index)])

// The number of live entries in g_hotspots.
extern u32 g_hotspotCount;

// The selected hotspot (-1 = none).
extern i32 g_selectedHotspot;

void ClearSelectedHotspot(void);
void SetSelectedHotspot(i32 index);
i16 GetSelectedHotspotValue(void);

RVA_DECL(0x00058640)
i16 CountFieldObjects(void);

#endif // GITEN_UI_HOTSPOT_H
