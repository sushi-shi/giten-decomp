#ifndef GITEN_GAME_AUTOMAP_H
#define GITEN_GAME_AUTOMAP_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/GameState.h>

// Marks a cell explored in the bitmap saved for its area and level.
void MarkAutomapCell(i16 area, i16 level, i16 x, i16 y);

void RotateAutomapRegion(i16 x, i16 y, i16 direction, i16* left, i16* top, i16* width, i16* height);
void TransformAutomapPoint(i16* x, i16* y);
void DrawAutomapMark(i16 mark, i16 x, i16 y);
void DrawAutomapTile(i16 tile, i16 x, i16 y);
void DrawMapOverlayTile(i16 tile, i16 x, i16 y);
void DrawAutomapRegion(i16 x, i16 y, i16 width, i16 height, i16 across, i16 along);
b16 DrawAutomapViewport(MapPosition position);
void UpdateAutomapScrollPanel(void);
b16 RunAutomapState(void);

// Detail gates include NPC markers at level two and object markers at level three.
// The map's current level is a word and an icon's minimum level a byte.
// clang-format off
GZ_ENUM_BEGIN(AutomapDetail)
    AUTOMAP_DETAIL_NONE = 0,
    AUTOMAP_DETAIL_BASIC = 1,
    AUTOMAP_DETAIL_NPCS = 2,
    AUTOMAP_DETAIL_OBJECTS = 3,
GZ_ENUM_END(AutomapDetail)

typedef struct AutomapIcon {
    u8 code;
    u8 mark;
    GZ_ENUM_STORAGE(AutomapDetail, u8) detail;
} AutomapIcon;
// clang-format on

// Draws the automap icon of cell `code` at x/y.
void DrawAutomapCellIcon(u8 code, i16 x, i16 y);

void DrawMapOverlay(MapPosition position);

#endif // GITEN_GAME_AUTOMAP_H
