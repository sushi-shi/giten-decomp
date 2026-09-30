#ifndef GITEN_GAME_FIELDLAYER_H
#define GITEN_GAME_FIELDLAYER_H

#include <Game/FieldLayerIndex.h>
#include <Game/ObjectRecord.h>
#include <Script/ScriptBlock.h>

// The field's object layers (one record and picture each) and each layer's
// script blocks.
#define FIELD_LAYER_SCRIPT_COUNT 32

typedef struct FieldLayer {
    u32 image;
    ObjectRecord record;
    ScriptBlock* scripts[FIELD_LAYER_SCRIPT_COUNT];
} FieldLayer;

static __inline ScriptBlock* GetLayerScript(FieldLayer* layer, i16 index) {
    return layer->scripts[index];
}

void FreeLayerScripts(FieldLayer* layer);
void LoadLayerScriptSet(FieldLayer* layer, i16 set);

#endif // GITEN_GAME_FIELDLAYER_H
