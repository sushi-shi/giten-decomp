// @identity-TODO: the owning TU is unproven; this unit holds the contiguous
// field-location mark family until link-order evidence names it.

#include <rva.h>

#include <Game/AreaNpc.h>
#include <Game/FieldMap.h>
#include <Game/GameState.h>

DATA(0x00068a5c)
static i16 s_markedArea = -1;
DATA(0x00068a60)
static i16 s_markedLevel = -1;
DATA(0x00068a64)
static i16 s_markedX = -1;
DATA(0x00068a68)
static i16 s_markedY = -1;
DATA(0x00068a6c)
static i16 s_markedDirection = -1;
DATA(0x00068a70)
static i16 s_currentRoomCode = -1;

RVA(0x0001a090, 0x3b)
void SetCellMark(i16 area, i16 level, i16 x, i16 y, i16 direction) {
    s_markedArea = area;
    s_markedLevel = level;
    s_markedX = x;
    s_markedY = y;
    s_markedDirection = direction;
}

RVA(0x0001a0d0, 0x32)
void SaveFieldPosition(void) {
    SetCellMark(
        g_field.pos.area,
        g_field.pos.level,
        g_field.pos.x,
        g_field.pos.y,
        g_field.pos.direction
    );
}

RVA(0x0001a110, 0x67)
i16 IsOnCellMark(i16 checkDirection) {
    if (g_field.pos.x == s_markedX && g_field.pos.y == s_markedY && g_field.pos.area == s_markedArea
        && g_field.pos.level == s_markedLevel) {
        if (checkDirection && g_field.pos.direction != s_markedDirection) {
            return 1;
        }
        return 0;
    }
    return -1;
}

RVA(0x0001a180, 0x7)
i16 GetCurrentRoomCode(void) {
    return s_currentRoomCode;
}

RVA(0x0001a190, 0x12)
i16 SetCurrentRoomCode(i16 code) {
    i16 previous = s_currentRoomCode;
    s_currentRoomCode = code;
    return previous;
}
