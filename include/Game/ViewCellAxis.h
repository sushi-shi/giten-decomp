#ifndef GITEN_GAME_VIEWCELLAXIS_H
#define GITEN_GAME_VIEWCELLAXIS_H

#include <EnumDomain.h>

// Which coordinate StepViewCell returns for a view cell.
GZ_ENUM_BEGIN_SPLIT(ViewCellAxis, i16)
    VIEW_CELL_COLUMN = 0,
    VIEW_CELL_ROW = 1
GZ_ENUM_END_SPLIT(ViewCellAxis)

#endif // GITEN_GAME_VIEWCELLAXIS_H
