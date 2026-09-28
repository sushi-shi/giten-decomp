// @identity-TODO: the owning TU is unproven; this unit holds the world-map
// event span until link-order evidence names it.

#include <rva.h>

#include <File/DataFile.h>
#include <Game/FieldView.h>
#include <Game/Scene.h>
#include <Game/WorldMap.h>
#include <Mem/Handle.h>
#include <Platform/GameCalls.h>
#include <Script/EventFlags.h>
#include <Util/Range.h>

DATA(0x00068c0c)
static i16 s_markedLayer = -1;
DATA(0x00068c10)
static i16 s_markedX = -1;
DATA(0x00068c14)
static i16 s_markedY = -1;
DATA(0x0007d5c8)
static i32 s_events;

RVA(0x0001b390, 0x14)
void FreeWorldMapEvents(void) {
    s_events = FreeHandle(s_events);
}

RVA(0x0001b3b0, 0x2f)
void LoadWorldMapEvents(void) {
    FILE* fp;
    FreeWorldMapEvents();
    fp = OpenDataFile(32, 12, 0);
    s_events = ReadRawHandle(fp);
    CloseDataFile(fp);
}

#define SetMarkedWorldMapEvent(layerValue, xValue, yValue)                                         \
    do {                                                                                           \
        s_markedLayer = (layerValue);                                                              \
        s_markedX = (xValue);                                                                      \
        s_markedY = (yValue);                                                                      \
    } while (0)

RVA(0x0001b3e0, 0x4f)
void MarkWorldMapEventSpot(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    SetMarkedWorldMapEvent(block * 2 + IsOddMapLayer(), x, y);
}

RVA(0x0001b430, 0x189)
b16 CheckWorldMapEvent(i16 x, i16 y) {
    i16 block = GetWorldMapBlock(x, y);
    i16 left;
    i16 top;
    i16 marked;
    i16 index;
    WorldMapEventTable* table;
    WorldMapEvent* events;
    x = GetWorldBlockX(x);
    y = GetWorldBlockY(y);
    left = x - 2;
    x += 2;
    top = y - 2;
    y += 2;
    block = block * 2 + IsOddMapLayer();
    table = HandleReadPtr(s_events);
    events = OffsetBy(table, table->offsets[block]);
    marked = 0;
    for (index = 0; events[index].x != -1; index++) {
        if (events[index].x >= left && events[index].x <= x && events[index].y >= top
            && events[index].y <= y) {
            if ((events[index].flagBank || events[index].flagIndex)
                && IsEventFlagSet(events[index].flagBank, events[index].flagIndex)) {
                continue;
            }
            if (events[index].x == s_markedX && events[index].y == s_markedY
                && block == s_markedLayer) {
                marked = 1;
            } else {
                SetMarkedWorldMapEvent(block, events[index].x, events[index].y);
                SetSceneCell(&events[index]);
                SetSceneScriptByIndex(7, 8);
                return true;
            }
        }
    }
    if (!marked) {
        SetMarkedWorldMapEvent(-1, -1, -1);
    }
    return false;
}
