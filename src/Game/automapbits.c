// @identity-TODO: the owning TU is unproven; this unit holds one contiguous
// retail span until link-order evidence names it. The automap bitmaps.

#include <rva.h>

#include <Game/AreaMap.h>
#include <Game/AreaNpc.h>
#include <Game/Automap.h>
#include <Game/AutomapData.h>
#include <Game/FieldHud.h>
#include <Game/FieldObject.h>
#include <Game/FieldScreen.h>
#include <Game/FieldSight.h>
#include <Game/FieldSupport.h>
#include <Game/FieldView.h>
#include <Game/ModeFlags.h>
#include <Game/SaveGame.h>
#include <Game/StateStack.h>
#include <Gfx/Render.h>
#include <Gfx/ScreenLayer.h>
#include <Gfx/VramAccess.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Platform/PlatformApi.h>
#include <Script/EventFlags.h>
#include <Text/Font.h>
#include <Text/TextWindow.h>
#include <Ui/Panel.h>
#include <Util/BitSet.h>
#include <Util/Scratch.h>

#include <stddef.h>
#include <stdio.h>
#include <string.h>

DATA(0x00068b30)
static const AutomapIcon s_mapIcons[] = {
    {0x40, 6, 1},  {0x41, 7, 1},  {0x42, 8, 1},  {0x43, 9, 1},  {0x44, 10, 1}, {0x45, 10, 1},
    {0x46, 10, 1}, {0x48, 11, 2}, {0x50, 12, 2}, {0x51, 13, 2}, {0x52, 14, 2}, {0x53, 15, 2},
    {0x54, 16, 2}, {0x55, 17, 2}, {0x56, 18, 2}, {0x57, 19, 2}, {0x58, 20, 2}, {0x59, 21, 2},
    {0x5b, 22, 2}, {0x7b, 23, 2}, {0x7d, 9, 2},  {0x85, 18, 2}, {0x86, 18, 2}, {0x87, 18, 2},
    {0x90, 8, 1},  {0x91, 9, 1},  {0xbf, 24, 1}, {0xff, 24, 1},
};

// The per-area level tables (256 handles).
DATA(0x0007bee0)
static i32 s_areaStore[256];

// The unpacked bitmap of the current level.
DATA(0x0007c2f8)
static AutomapBitmap s_levelBuffer;

DATA(0x0007d5e0)
static i32* s_areas;

DATA(0x0007d5e4)
static i16 s_mapDirection;

DATA(0x0007d5e8)
static i16 s_mapOriginX;

DATA(0x0007d5ec)
static i16 s_mapOriginY;

DATA(0x0007d5f0)
static i16 s_mapWidth;

DATA(0x0007d5f4)
static i16 s_mapHeight;

DATA(0x0007d5f8)
static i16 s_mapScreenX;

DATA(0x0007d5fc)
static i16 s_mapScreenY;

DATA(0x0007d600)
static b16 s_mapActive;

DATA(0x0007c2e0)
static i16 s_mapPlane;

DATA(0x0007c2e8)
static MapPosition s_mapPosition;

DATA(0x0007d608)
static Panel* s_mapPanel;

DATA(0x0007d604)
static i16 s_mapDetail;

DATA(0x0007d60c)
static AutomapBitmap* s_levelBitmap;

// The area and level whose bitmap is unpacked (-1: none).
DATA(0x00068c40)
static i16 s_levelArea = -1;

DATA(0x00068c44)
static i16 s_levelIndex = -1;

static __inline void EnsureAutomapStore(void) {
    if (s_areas == NULL) {
        memset(s_areaStore, 0, sizeof(s_areaStore));
        s_areas = s_areaStore;
    }
}

RVA(0x0001cde0, 0x2e)
void InitAutomap(void) {
    s_levelBitmap = &s_levelBuffer;
    EnsureAutomapStore();
}

// Makes sure the current area has a bitmap for each of its levels.
RVA(0x0001ce10, 0x12d)
void AllocAutomapLevels(void) {
    i16 area;
    i16 count;
    i32* slot;
    i32 levels;
    i16 level;
    MapCoord size;
    i16 bytes;
    i32 bitmap;
    AutomapBitmap* data;
    EnsureAutomapStore();
    area = GetCurrentArea();
    count = GetAreaLevelCount();
    slot = &s_areas[area];
    levels = *slot;
    if (levels == 0) {
        levels = CreateArrayHandle(GetAutomapLevelTableSize(count), 1);
        *slot = levels;
        ((AutomapLevels*)HandleWritePtr(levels))->header.count = count;
    }
    for (level = 0; level < count; level++) {
        if (GetAutomapLevelHandle(HandleReadPtr(levels), level) == 0) {
            size = GetAreaSize(level);
            bytes = ((i16)(size.x * size.y) + 7) / 8;
            bitmap = CreateArrayHandle(bytes + 8, 1);
            SetAutomapLevelHandle(HandleWritePtr(levels), level, bitmap);
            data = HandleWritePtr(bitmap);
            data->header.width = size.x;
            data->header.size = bytes;
            data->header.height = size.y;
        }
    }
}

RVA(0x0001cf40, 0x7f)
void FreeAutomap(void) {
    i16 i;
    i32 levels;
    i16 count;
    i16 k;
    if (s_areas == NULL) {
        return;
    }
    for (i = 0; i < 256; i++) {
        levels = s_areas[i];
        if (levels != 0) {
            count = GetAutomapLevelCount(HandleReadPtr(levels));
            for (k = 0; k < count; k++) {
                FreeHandle(GetAutomapLevelHandle(HandleReadPtr(levels), k));
            }
            FreeHandle(levels);
        }
    }
    s_areas = NULL;
}

// Writes the unpacked level bitmap back to its handle.
RVA(0x0001cfc0, 0x89)
void StoreAutomapLevel(void) {
    AutomapLevels* levels;
    AutomapBitmap* data;
    i32 bitmap;
    u16 size;
    if (s_levelArea >= 0 && s_levelIndex >= 0 && s_areas != NULL && s_areas[s_levelArea] != 0) {
        levels = HandleReadPtr(s_areas[s_levelArea]);
        if (GetAutomapLevelCount(levels) > s_levelIndex) {
            bitmap = GetAutomapLevelHandle(levels, s_levelIndex);
            if (bitmap != 0) {
                size = GetAutomapBitmapSize(&s_levelBitmap->header);
                data = HandleWritePtr(bitmap);
                memmove(data, s_levelBitmap, size);
            }
        }
    }
    s_levelIndex = s_levelArea = -1;
}

RVA(0x0001d050, 0x88)
void LoadAutomapLevel(i16 area, i16 level) {
    AutomapLevels* levels;
    i32 bitmap;
    AutomapBitmap* data;
    if (s_levelArea == area && s_levelIndex == level) {
        return;
    }
    StoreAutomapLevel();
    if (s_areas == NULL || s_areas[area] == 0) {
        return;
    }
    levels = HandleReadPtr(s_areas[area]);
    if (GetAutomapLevelCount(levels) <= level) {
        return;
    }
    bitmap = GetAutomapLevelHandle(levels, level);
    if (bitmap == 0) {
        return;
    }
    data = HandleReadPtr(bitmap);
    memmove(s_levelBitmap, data, (u16)(GetAutomapBitmapSize(&data->header)));
    s_levelArea = area;
    s_levelIndex = level;
}

RVA(0x0001d0e0, 0x3d)
void MarkAutomapCell(i16 area, i16 level, i16 x, i16 y) {
    if (s_levelArea == area && s_levelIndex == level) {
        SetBit(s_levelBitmap->bits, AutomapCellIndex(s_levelBitmap, x, y));
    }
}

// 0x100 when x/y of `area`/`level` has not been explored (or has no bitmap);
// coordinates wrap at the level size.
RVA(0x0001d120, 0xe1)
i16 IsAutomapCellHidden(i16 x, i16 y, i16 area, i16 level) {
    AutomapLevels* levels;
    AutomapBitmap* data;
    i32 bitmap;
    u8* bits;
    i16 index;
    if (s_levelArea == area && s_levelIndex == level) {
        return TestBit(s_levelBitmap->bits, AutomapCellIndex(s_levelBitmap, x, y)) != 1 ? 0x100 : 0;
    } else {
        if (s_areas == NULL) {
            return 0x100;
        }
        if (s_areas[area] == 0) {
            return 0x100;
        }
        levels = HandleReadPtr(s_areas[area]);
        if (GetAutomapLevelCount(levels) <= level) {
            return 0x100;
        }
        bitmap = GetAutomapLevelHandle(levels, level);
        if (bitmap == 0) {
            return 0x100;
        }
        data = HandleReadPtr(bitmap);
        if (x >= data->header.width) {
            x %= data->header.width;
        }
        if (y >= data->header.height) {
            y %= data->header.height;
        }
        bits = data->bits;
        index = AutomapCellIndex(data, x, y);
    }
    return TestBit(bits, index) != 1 ? 0x100 : 0;
}

RVA(0x0001d210, 0xd4)
void RotateAutomapRegion(
    i16 x,
    i16 y,
    i16 direction,
    i16* left,
    i16* top,
    i16* width,
    i16* height
) {
    i16 oldWidth;
    switch (direction) {
        case 0:
            *left = -x;
            *top = -y;
            break;
        case 1:
            *left = -y;
            *top = x - *width + 1;
            oldWidth = *width;
            *width = *height;
            *height = oldWidth;
            break;
        case 2:
            *left = x - *width + 1;
            *top = y - *height + 1;
            break;
        case 3:
            *left = y - *height + 1;
            *top = -x;
            oldWidth = *width;
            *width = *height;
            *height = oldWidth;
            break;
    }
}

RVA(0x0001d2f0, 0x8b)
void DrawAutomapMark(i16 mark, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        DrawPlaneMapMark(mark, x, y, s_mapPlane);
    }
}

RVA(0x0001d380, 0xb4)
void TransformAutomapPoint(i16* x, i16* y) {
    i16 oldX;
    switch (s_mapDirection) {
        case 0:
            *x -= s_mapOriginX;
            *y -= s_mapOriginY;
            break;
        case 1:
            oldX = *x;
            *x = *y - s_mapOriginY;
            *y = s_mapOriginX - oldX;
            break;
        case 2:
            *x = s_mapOriginX - *x;
            *y = s_mapOriginY - *y;
            break;
        case 3:
            oldX = *x;
            *x = s_mapOriginY - *y;
            *y = oldX - s_mapOriginX;
            break;
    }
}

RVA(0x0001d440, 0xf0)
void MarkMapCell(i16 kind, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        if (g_field.pos.area == 0x82 && g_field.pos.level == 15) {
            if (!g_fieldStatus.navigationFixed && (g_field.pos.direction & 1)) {
                x += 3;
                y += 2;
            } else {
                x += 2;
                y += 3;
            }
        }
        DrawMapMark(kind, x, y);
    }
}

RVA(0x0001d530, 0x360)
b16 RunAutomapState(void) {
    i16 savedState;
    i16 input;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        ReturnFromGameState();
        return false;
    }
    SetLayersRenderMode();
    switch (GetGamePhase()) {
        case 0:
            NextGamePhase();
            s_mapActive = true;
            RestoreDrawState(SaveDrawState());
            s_mapPosition = g_field.pos;
            s_mapPlane = CreateTextPlane(31, 0);
            s_mapPanel = CreateKindPanel(s_mapPanel, 0x11d, 4, 31);
            if (g_fieldStatus.automapFixed) {
                s_mapPosition.direction = 0;
            }
            s_mapDetail = 0;
            if (!IsEventFlagSet(2, 0x39)) {
                s_mapDetail = 2;
            }
            if (s_mapDetail < 1) {
                SetGamePhase(3);
            }
            break;
        case 1:
            NextGamePhase();
            savedState = SaveDrawState();
            DrawAutomapViewport(s_mapPosition);
            UpdateAutomapScrollPanel();
            RestoreDrawState(savedState);
            break;
        case 2:
            input = RunPanelInput(s_mapPanel);
            if (input == -2) {
                NextGamePhase();
            } else if (input != -1) {
                savedState = SaveDrawState();
                switch (input) {
                    case 0:
                        ScrollPlaneMapDown(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 0, -1);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, s_mapWidth, 1, 0, 0);
                        break;
                    case 1:
                        ScrollPlaneMapLeft(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 1, 0);
                        DrawAutomapRegion(
                            s_mapOriginX,
                            s_mapOriginY,
                            1,
                            s_mapHeight,
                            s_mapWidth - 1,
                            0
                        );
                        break;
                    case 2:
                        ScrollPlaneMapUp(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, 0, 1);
                        DrawAutomapRegion(
                            s_mapOriginX,
                            s_mapOriginY,
                            s_mapWidth,
                            1,
                            0,
                            s_mapHeight - 1
                        );
                        break;
                    case 3:
                        ScrollPlaneMapRight(s_mapPlane);
                        OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, s_mapDirection, -1, 0);
                        DrawAutomapRegion(s_mapOriginX, s_mapOriginY, 1, s_mapHeight, 0, 0);
                        break;
                }
                UpdateAutomapScrollPanel();
                RestoreDrawState(savedState);
            }
            break;
        case 3:
            CloseTextWindow(s_mapPlane);
            s_mapPanel = ReleasePanel(s_mapPanel, 1);
            RequestFieldRefresh();
            RunFieldPanelRow(7, 0, 0, 0);
            ReturnFromGameState();
            s_mapActive = false;
            break;
    }
    return false;
}

RVA(0x0001d890, 0x1f0)
void UpdateAutomapScrollPanel(void) {
    i16 width;
    i16 height;
    i16 blocked;
    ClearPanelChecksAgain(s_mapPanel);
    GetMapSize(&width, &height);
    blocked = 0;
    switch (s_mapDirection) {
        case 0:
            if (s_mapOriginX == 0) {
                blocked |= 1;
            }
            if (s_mapOriginX + s_mapWidth >= width) {
                blocked |= 2;
            }
            if (s_mapOriginY == 0) {
                blocked |= 4;
            }
            if (s_mapOriginY + s_mapHeight >= height) {
                blocked |= 8;
            }
            break;
        case 1:
            if (s_mapOriginY == 0) {
                blocked |= 1;
            }
            if (s_mapOriginY + s_mapWidth >= height) {
                blocked |= 2;
            }
            if (s_mapOriginX == width - 1) {
                blocked |= 4;
            }
            if (s_mapOriginX - s_mapHeight < 0) {
                blocked |= 8;
            }
            break;
        case 2:
            if (s_mapOriginX == width - 1) {
                blocked |= 1;
            }
            if (s_mapOriginX - s_mapWidth < 0) {
                blocked |= 2;
            }
            if (s_mapOriginY == height - 1) {
                blocked |= 4;
            }
            if (s_mapOriginY - s_mapHeight < 0) {
                blocked |= 8;
            }
            break;
        case 3:
            if (s_mapOriginY == height - 1) {
                blocked |= 1;
            }
            if (s_mapOriginY - s_mapWidth < 0) {
                blocked |= 2;
            }
            if (s_mapOriginX == 0) {
                blocked |= 4;
            }
            if (s_mapOriginX + s_mapHeight == width) {
                blocked |= 8;
            }
            break;
    }
    SetPanelRowFlags(s_mapPanel, 3, PANEL_HIDDEN, blocked & 1);
    SetPanelRowFlags(s_mapPanel, 1, PANEL_HIDDEN, blocked & 2);
    SetPanelRowFlags(s_mapPanel, 0, PANEL_HIDDEN, blocked & 4);
    SetPanelRowFlags(s_mapPanel, 2, PANEL_HIDDEN, blocked & 8);
    PaintPanel(s_mapPanel, s_mapPlane);
}

RVA(0x0001da80, 0x110)
void DrawAutomapRegion(i16 x, i16 y, i16 width, i16 height, i16 across, i16 along) {
    i16 row;
    i16 column;
    i16 tile;
    MapCoord cell;
    OffsetMapCoord(&x, &y, s_mapDirection, across, along);
    for (row = 0; row < height; row++) {
        for (column = 0; column < width; column++) {
            cell.x = x;
            cell.y = y;
            cell = MoveMapCoord(cell, s_mapDirection, column, row);
            tile = GetRotatedWallAtOffset(cell.x, cell.y, s_mapDirection, 0, 0);
            tile = GetWallStopCode(tile, WALL_STOP_MOVEMENT);
            DrawAutomapTile(tile, cell.x, cell.y);
        }
    }
    IsCellBlocked(s_mapPosition.level, 0, 0, 0);
    if (s_mapPosition.level == g_field.pos.level) {
        DrawAutomapMark(
            TurnDirection(g_field.pos.direction, -s_mapDirection),
            g_field.pos.x,
            g_field.pos.y
        );
    }
}

RVA(0x0001db90, 0xc1)
void DrawAutomapTile(i16 tile, i16 x, i16 y) {
    i16 mapX = x;
    i16 mapY = y;
    if (s_mapActive) {
        TransformAutomapPoint(&x, &y);
        if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
            if (!IsAutomapCellHidden(mapX, mapY, s_mapPosition.area, s_mapPosition.level)) {
                DrawPlaneMapTileLit(tile, x, y, s_mapPlane);
            } else if (IsLevelMapRevealed()) {
                DrawPlaneMapTile(tile, x, y, s_mapPlane);
            }
        }
    }
}

RVA(0x0001dc60, 0x1e0)
b16 DrawAutomapViewport(MapPosition position) {
    i16 width;
    i16 height;
    i16 left;
    i16 top;
    i16 viewWidth;
    i16 viewHeight;
    i16 x;
    i16 y;
    i16 direction;
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= 38) {
        viewWidth = width;
    } else {
        viewWidth = 38;
        if (-left * 2 > 38) {
            left = -19;
            switch (direction) {
                case 0:
                    if (x + 19 > width) {
                        left = width - x - 38;
                    }
                    break;
                case 1:
                    if (y + 19 > width) {
                        left = width - y - 38;
                    }
                    break;
                case 2:
                    if (x - 18 < 0) {
                        left = x - 37;
                    }
                    break;
                case 3:
                    if (y - 18 < 0) {
                        left = y - 37;
                    }
                    break;
            }
        }
    }
    if (height <= 18) {
        viewHeight = height;
    } else {
        viewHeight = 18;
        if (-top * 2 > 18) {
            top = -9;
            switch (direction) {
                case 0:
                    if (y + 9 > height) {
                        top = height - y - 18;
                    }
                    break;
                case 1:
                    if (x - 8 < 0) {
                        top = x - 17;
                    }
                    break;
                case 2:
                    if (y - 8 < 0) {
                        top = y - 17;
                    }
                    break;
                case 3:
                    if (x + 9 > height) {
                        top = height - x - 18;
                    }
                    break;
            }
        }
    }
    s_mapDirection = direction;
    s_mapWidth = viewWidth;
    s_mapHeight = viewHeight;
    s_mapOriginX = x;
    s_mapOriginY = y;
    OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, direction, left, top);
    DrawAutomapRegion(s_mapOriginX, s_mapOriginY, viewWidth, viewHeight, 0, 0);
    return false;
}

RVA(0x0001de40, 0x3d0)
void DrawMapOverlay(MapPosition position) {
    i16 width, height;
    i16 left, top;
    i16 viewWidth, viewHeight;
    i16 x, y, direction;
    i16 screenX;
    i16 screenY;
    i16 row, column;
    i16 tile;
    i16 edge;
    MapCoord cell;
    if (TestModeFlags(MODE_WORLD_MAP)) {
        return;
    }
    ClearLayerSurface(6);
    if (g_fieldStatus.navigationFixed) {
        position.direction = 0;
    }
    s_mapDetail = 0;
    if (!IsEventFlagSet(2, 9)) {
        s_mapDetail = 1;
    }
    if (!IsEventFlagSet(2, 15)) {
        s_mapDetail = 2;
    }
    if (!IsEventFlagSet(2, 0x38)) {
        s_mapDetail = 3;
    }
    if (s_mapDetail < 1) {
        return;
    }
    if (IsDarkCell(g_field.pos.x, g_field.pos.y)) {
        return;
    }
    screenX = 1;
    GetMapSize(&width, &height);
    x = position.x;
    y = position.y;
    direction = position.direction;
    RotateAutomapRegion(x, y, direction, &left, &top, &width, &height);
    if (width <= 7) {
        viewWidth = width;
        screenX = 8 - width;
    } else {
        viewWidth = 7;
        if (-left * 2 > 7) {
            left = -3;
            switch (position.direction) {
                case 0:
                    if (position.x + 4 > width) {
                        left = width - x - 7;
                    }
                    break;
                case 1:
                    if (position.y + 4 > width) {
                        left = width - y - 7;
                    }
                    break;
                case 2:
                    edge = x - 3;
                    if (edge < 0) {
                        left = x - 6;
                    }
                    break;
                case 3:
                    edge = y - 3;
                    if (edge < 0) {
                        left = y - 6;
                    }
                    break;
            }
        }
    }
    if (height <= 7) {
        viewHeight = height;
        screenY = 21 - height;
    } else {
        screenY = 14;
        viewHeight = 7;
        edge = -top * 2;
        if (edge > 7) {
            top = -3;
            switch (position.direction) {
                case 0:
                    if (position.y + 4 > height) {
                        top = height - y - 7;
                    }
                    break;
                case 1:
                    edge = x - 3;
                    if (edge < 0) {
                        top = x - 6;
                    }
                    break;
                case 2:
                    edge = y - 3;
                    if (edge < 0) {
                        top = y - 6;
                    }
                    break;
                case 3:
                    if (position.x + 4 > height) {
                        top = height - x - 7;
                    }
                    break;
            }
        }
    }
    s_mapDirection = position.direction;
    s_mapWidth = viewWidth;
    s_mapHeight = viewHeight;
    s_mapScreenX = screenX;
    s_mapScreenY = screenY * 8;
    s_mapOriginX = position.x;
    s_mapOriginY = position.y;
    OffsetMapCoord(&s_mapOriginX, &s_mapOriginY, direction, left, top);
    for (row = 0; row < viewHeight; row++) {
        for (column = 0; column < viewWidth; column++) {
            cell.x = position.x;
            cell.y = position.y;
            cell = MoveMapCoord(cell, direction, column + left, row + top);
            tile =
                GetRotatedWallAtOffset(position.x, position.y, direction, column + left, row + top);
            tile = GetWallStopCode(tile, WALL_STOP_GEOMETRY);
            DrawMapOverlayTile(tile, cell.x, cell.y);
        }
    }
    IsCellBlocked(g_field.pos.level, 0, 0, 0);
    if (s_mapDetail >= 2) {
        MarkAreaNpcs();
    }
    if (s_mapDetail >= 3) {
        MarkObjectsOnMap();
    }
    MarkMapCell(TurnDirection(g_field.pos.direction, -direction), g_field.pos.x, g_field.pos.y);
}

RVA(0x0001e210, 0xf0)
void DrawMapOverlayTile(i16 tile, i16 x, i16 y) {
    if (s_mapActive && !IsLevelMapRevealed()) {
        if (IsAutomapCellHidden(x, y, s_mapPosition.area, s_mapPosition.level)) {
            return;
        }
    }
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        if (g_field.pos.area == 0x82 && g_field.pos.level == 15) {
            if (!g_fieldStatus.navigationFixed && (g_field.pos.direction & 1)) {
                x += 3;
                y += 2;
            } else {
                x += 2;
                y += 3;
            }
        }
        DrawMapTile(tile, x, y);
    }
}

RVA(0x0001e300, 0x50)
b16 IsCellInView(i16 x, i16 y) {
    TransformAutomapPoint(&x, &y);
    if (x >= 0 && x < s_mapWidth && y >= 0 && y < s_mapHeight) {
        return true;
    }
    return false;
}

RVA(0x0001e350, 0x70)
void DrawAutomapCellIcon(u8 code, i16 x, i16 y) {
    i16 index = 0;
    while (s_mapIcons[index].code < code) {
        index++;
    }
    if (s_mapIcons[index].code == code && s_mapDetail >= s_mapIcons[index].detail) {
        if (GetRenderMode() == RENDER_MODE_LAYERS) {
            DrawAutomapMark(s_mapIcons[index].mark, x, y);
        } else {
            MarkMapCell(s_mapIcons[index].mark, x, y);
        }
    }
}

RVA(0x0001e3c0, 0x160)
i16 WriteAutomapAreas(FILE* fp) {
    i16 errors = 0;
    i16 area;
    i16 level;
    i16 count;
    i16 bytes;
    i32 handle;
    i32 bitmap;
    AutomapLevels* levels;
    AutomapBitmap* data;
    memset(g_scratchBuffer, 0, 256);
    if (!s_areas) {
        for (area = 0; area < 4; area++) {
            errors += 256 - fwrite(g_scratchBuffer, 1, 256, fp);
        }
        return errors;
    }
    StoreAutomapLevel();
    errors = 256 - fwrite(s_areas, 4, 256, fp);
    for (area = 0; area < 256; area++) {
        handle = s_areas[area];
        if (handle) {
            levels = HandleReadPtr(handle);
            count = GetAutomapLevelCount(levels);
            bytes = GetAutomapLevelTableSize(count);
            errors += bytes - fwrite(levels, 1, bytes, fp);
            for (level = 0; level < count; level++) {
                levels = HandleReadPtr(handle);
                bitmap = GetAutomapLevelHandle(levels, level);
                if (bitmap) {
                    data = HandleReadPtr(bitmap);
                    bytes = GetAutomapBitmapSize(&data->header);
                    errors += bytes - fwrite(data, 1, bytes, fp);
                }
            }
        }
    }
    LoadAutomapLevel(g_field.pos.area, g_field.pos.level);
    return errors;
}

RVA(0x0001e520, 0x1d0)
i16 LoadAutomapAreas(FILE* fp) {
    i16 errors;
    i16 area;
    i16 level;
    i16 count;
    i16 bytes;
    i32 handle;
    i32 bitmap;
    AutomapLevelHeader levelHeader;
    AutomapBitmapHeader bitmapHeader;
    AutomapLevels* levels;
    AutomapBitmap* data;
    StoreAutomapLevel();
    FreeAutomap();
    EnsureAutomapStore();
    errors = 256 - fread(s_areas, 4, 256, fp);
    if (errors) {
        return errors;
    }
    for (area = 0; area < 256; area++) {
        if (s_areas[area]) {
            errors += 1 - fread(&levelHeader, 4, 1, fp);
            count = levelHeader.count;
            handle = CreateArrayHandle(GetAutomapLevelTableSize(count), 1);
            s_areas[area] = handle;
            levels = HandleWritePtr(handle);
            levels->header = levelHeader;
            errors += count - fread(levels->levels, 4, count, fp);
            for (level = 0; level < count; level++) {
                levels = HandleWritePtr(handle);
                if (GetAutomapLevelHandle(levels, level)) {
                    errors += 1 - fread(&bitmapHeader, 8, 1, fp);
                    bitmap = CreateArrayHandle(GetAutomapBitmapSize(&bitmapHeader), 1);
                    data = HandleWritePtr(bitmap);
                    data->header = bitmapHeader;
                    bytes = data->header.size;
                    errors += bytes - fread(data->bits, 1, bytes, fp);
                    levels = HandleWritePtr(handle);
                    SetAutomapLevelHandle(levels, level, bitmap);
                }
            }
        }
    }
    return errors;
}

RVA(0x0001e6f0, 0x2a)
void DrawFieldView(void) {
    DrawMapOverlay(g_field.pos);
}
