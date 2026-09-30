#ifndef GITEN_GAME_CELLCODE_H
#define GITEN_GAME_CELLCODE_H

#include <Enums.h>

// Map cell codes, named after what the cell does: the forced moves push the
// party toward a direction (code - CELL_FORCED_MOVE_NORTH) or back the way it
// came, the spinners turn it, and the dark and command-blocked cells are
// properties the field code tests. The traps take a share of each member's HP;
// the alignment traps (CELL_ALIGNMENT_TRAP_FIRST..LAST) spare the alignment
// sides their mask leaves out. Lists of cells end with CELL_LIST_END.
// @identity-TODO: the individual NPC picture and other object codes, and the
// kind-10/12/13 cells 0x7c, 0x88, 0x89, 0x8b, 0x8c, 0x8f and 0x65 are unnamed.
GZ_ENUM_BEGIN(CellCode)
    CELL_SERVICE_TERMINAL = 0x40,
    CELL_EXIT = 0x41,
    CELL_STAIRS_UP = 0x42,
    CELL_STAIRS_DOWN = 0x43,
    CELL_CHUTE = 0x47,
    CELL_AREA_NPC_FIRST = 0x48,
    CELL_AREA_NPC_LAST = 0x4e,
    CELL_SOFTWARE_SHOP = 0x50,
    CELL_WEAPON_SHOP = 0x51,
    CELL_TRANSFER_DEVICE = 0x52,
    CELL_MEDICINE_SHOP = 0x53,
    CELL_HERETIC_MANSION = 0x54,
    CELL_HOSPITAL = 0x55,
    CELL_SPRING = 0x56,
    CELL_RECOVERY_HALL = 0x57,
    CELL_ARMOR_SHOP = 0x58,
    CELL_BAR = 0x59,
    CELL_ITEM_SHOP = 0x5b,
    CELL_DAMAGE_TRAP = 0x60,
    CELL_FORCED_MOVE_BACK = 0x64,
    CELL_WARP_HIDING_OBJECTS = 0x67,
    CELL_ALIGNMENT_TRAP_FIRST = 0x68,
    CELL_ALIGNMENT_TRAP_LAST = 0x6e,
    CELL_FORCED_MOVE_NORTH = 0x70,
    CELL_FORCED_MOVE_EAST = 0x71,
    CELL_FORCED_MOVE_SOUTH = 0x72,
    CELL_FORCED_MOVE_WEST = 0x73,
    CELL_SPIN_RIGHT = 0x74,
    CELL_SPIN_AROUND = 0x75,
    CELL_SPIN_LEFT = 0x76,
    CELL_MARKED_WARP = 0x77,
    CELL_FACING_SCRIPT = 0x79,
    CELL_ARM_TERMINAL = 0x7b,
    CELL_STAIRS_TO_SUBWAY_PLATFORM = 0x7d,
    CELL_FROZEN_SCENE = 0x7f,
    CELL_DARK = 0x8d,
    CELL_COMMAND_BLOCKED = 0x8e,
    CELL_STEPS_UP = 0x90,
    CELL_STEPS_DOWN = 0x91,
    CELL_LIST_END = 0xff
GZ_ENUM_END(CellCode)

#endif // GITEN_GAME_CELLCODE_H
