#ifndef GITEN_GAME_CELLTRAP_H
#define GITEN_GAME_CELLTRAP_H

#include <Game/AreaMap.h>

// A cell record copied out of its list into sixteen bytes, the size of the
// scene cell (SetSceneCell); every kind but the script cell fits whole.
// RunCellTrap copies its exit into one.
typedef union MapCell {
    CellHead head;
    WarpCell warp;
    BattleCell battle;
    LinkCell link;
    ObjectCell object;
    ExitCell exit;
    TreasureBox box;
    DoorCell door;
} MapCell;

i32 GetCellTrapDamage(ExitCell* cell, i16 maxHp);

#endif // GITEN_GAME_CELLTRAP_H
