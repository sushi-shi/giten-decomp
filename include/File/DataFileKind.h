#ifndef GITEN_FILE_DATAFILEKIND_H
#define GITEN_FILE_DATAFILEKIND_H

#include <Enums.h>

// File families selected by OpenDataFile; names retain the on-disk prefix
// where no more specific resource role has been recovered.
GZ_ENUM_BEGIN(DataFileKind)
    DATA_FILE_IMAGE = 0,
    DATA_FILE_IMAGE_FCH = 1,
    DATA_FILE_EFFECT = 2,
    DATA_FILE_MAP = 3,
    DATA_FILE_ET_A = 4,
    DATA_FILE_SOUND_SM = 5,
    DATA_FILE_SOUND_SB = 6,
    DATA_FILE_SOUND_ST = 7,
    DATA_FILE_SOUND_SE = 8,
    DATA_FILE_SCRIPT = 9,
    DATA_FILE_OBJECT = 10,
    DATA_FILE_ITEM_RECORDS = 11,
    DATA_FILE_TABLE = 12,
    DATA_FILE_GD = 13,
    DATA_FILE_OBJECT_SCRIPT = 14,
    DATA_FILE_IMAGE_VARIANT = 15
GZ_ENUM_END(DataFileKind)

#endif // GITEN_FILE_DATAFILEKIND_H
