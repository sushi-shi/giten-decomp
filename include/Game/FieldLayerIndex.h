#ifndef GITEN_GAME_FIELDLAYERINDEX_H
#define GITEN_GAME_FIELDLAYERINDEX_H

#include <EnumDomain.h>
#include <Ints.h>

// The two loaded field-object groups and their saved layer records.
GZ_ENUM_BEGIN_SPLIT(FieldLayerIndex, i16)
    FIELD_LAYER_FIRST = 0,
    FIELD_LAYER_SECOND = 1
GZ_ENUM_END_SPLIT(FieldLayerIndex)

#define FIELD_LAYER_COUNT 2

#endif // GITEN_GAME_FIELDLAYERINDEX_H
