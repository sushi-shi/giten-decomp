#ifndef GITEN_GAME_AUTOMAP_H
#define GITEN_GAME_AUTOMAP_H

#include <rva.h>

#include <EnumDomain.h>
#include <Enums.h>
#include <Game/GameState.h>
#include <Game/ViewDirection.h>

// The automap window shows AUTOMAP_VIEW_WIDTH x AUTOMAP_VIEW_HEIGHT cells
// around the party; the field's map overlay shows MAP_OVERLAY_SIZE cells square
// with its top left at text cell (MAP_OVERLAY_X, MAP_OVERLAY_Y).
#define AUTOMAP_VIEW_WIDTH 38
#define AUTOMAP_VIEW_HEIGHT 18
#define MAP_OVERLAY_SIZE 7
#define MAP_OVERLAY_X 1
#define MAP_OVERLAY_Y 14

// RunAutomapState's phases: open the window, draw the map, scroll it, close.
GZ_ENUM_BEGIN_SPLIT(AutomapPhase, i16)
    AUTOMAP_PHASE_OPEN = 0,
    AUTOMAP_PHASE_DRAW = 1,
    AUTOMAP_PHASE_SCROLL = 2,
    AUTOMAP_PHASE_CLOSE = 3
GZ_ENUM_END_SPLIT(AutomapPhase)

// The automap's scroll panel rows (its input is the row clicked), and the
// bits UpdateAutomapScrollPanel sets for the directions the map cannot scroll.
GZ_ENUM_BEGIN_SPLIT(AutomapScroll, i16)
    AUTOMAP_SCROLL_UP = 0,
    AUTOMAP_SCROLL_RIGHT = 1,
    AUTOMAP_SCROLL_DOWN = 2,
    AUTOMAP_SCROLL_LEFT = 3
GZ_ENUM_END_SPLIT(AutomapScroll)

GZ_ENUM_FLAGS_BEGIN(AutomapScrollBlock, i16)
    AUTOMAP_BLOCK_NONE = 0,
    AUTOMAP_BLOCK_LEFT = 1,
    AUTOMAP_BLOCK_RIGHT = 2,
    AUTOMAP_BLOCK_UP = 4,
    AUTOMAP_BLOCK_DOWN = 8
GZ_ENUM_FLAGS_END(AutomapScrollBlock)

// Marks a cell explored in the bitmap saved for its area and level.
void MarkAutomapCell(i16 area, i16 level, i16 x, i16 y);

void RotateAutomapRegion(
    i16 x,
    i16 y,
    GZ_ENUM_PARAM(ViewDirection, i16) direction,
    i16* left,
    i16* top,
    i16* width,
    i16* height
);
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
