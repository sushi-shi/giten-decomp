#ifndef GITEN_UI_MENUSTEP_H
#define GITEN_UI_MENUSTEP_H

#include <EnumDomain.h>

// The steps of a game state that runs a menu box (the debug, system and DDS
// menus, by phase, step or sub-state): open it, close it, run it, and from
// MENU_STEP_PICK_FIRST on, run row n of the menu at MENU_STEP_PICK_FIRST + n.
GZ_ENUM_BEGIN(MenuStateStep)
    MENU_STEP_OPEN = 0,
    MENU_STEP_CLOSE = 1,
    MENU_STEP_RUN = 2,
    MENU_STEP_PICK_FIRST = 3
GZ_ENUM_END(MenuStateStep)

// The debug menu's rows, named after their labels (rows 3, 5, 6 and 14 have
// none).
GZ_ENUM_BEGIN(DebugMenuRow)
    DEBUG_ROW_BGM = 0,
    DEBUG_ROW_SE = 1,
    DEBUG_ROW_MAGIC_EFFECT = 2,
    DEBUG_ROW_DESTROY_ALL_DEMONS = 4,
    DEBUG_ROW_GET_ITEMS = 7,
    DEBUG_ROW_CHANGE_STATS = 8,
    DEBUG_ROW_MOVE_3D = 9,
    DEBUG_ROW_CHANGE_FLAGS = 10,
    DEBUG_ROW_SAVE = 11,
    DEBUG_ROW_LOAD = 12,
    DEBUG_ROW_CHECK_DATA = 13
GZ_ENUM_END(DebugMenuRow)

// The magic-effect test's rows, named after their labels: six steps of the
// tested skill number, the shot distance and running the test.
GZ_ENUM_BEGIN(DebugMagicRow)
    DEBUG_MAGIC_ROW_PLUS_1 = 0,
    DEBUG_MAGIC_ROW_MINUS_1 = 1,
    DEBUG_MAGIC_ROW_PLUS_10 = 2,
    DEBUG_MAGIC_ROW_MINUS_10 = 3,
    DEBUG_MAGIC_ROW_PLUS_100 = 4,
    DEBUG_MAGIC_ROW_MINUS_100 = 5,
    DEBUG_MAGIC_ROW_DISTANCE = 6,
    DEBUG_MAGIC_ROW_RUN = 7
GZ_ENUM_END(DebugMagicRow)

// The system menu's rows: the two display toggles, quitting and the debug menu.
GZ_ENUM_BEGIN(SystemMenuRow)
    SYSTEM_ROW_AUTO_MAPPING = 0,
    SYSTEM_ROW_AUTO_NAVIGATION = 1,
    SYSTEM_ROW_QUIT = 2,
    SYSTEM_ROW_DEBUG = 3
GZ_ENUM_END(SystemMenuRow)

// The DDS menu's commands.
GZ_ENUM_BEGIN(DdsMenuRow)
    DDS_ROW_CALL = 0,
    DDS_ROW_RETURN = 1,
    DDS_ROW_PURGE = 2
GZ_ENUM_END(DdsMenuRow)

#endif // GITEN_UI_MENUSTEP_H
