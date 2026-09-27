#ifndef GITEN_GAME_FIELDLAYER_H
#define GITEN_GAME_FIELDLAYER_H

#include <Game/ObjectRecord.h>
#include <Script/ScriptBlock.h>

typedef struct FieldLayer {
    u32 image;
    ObjectRecord record;
    ScriptBlock* scripts[32];
} FieldLayer;

static __inline ScriptBlock* GetLayerScript(FieldLayer* layer, i16 index) {
    return layer->scripts[index];
}

void FreeLayerScripts(FieldLayer* layer);
void LoadLayerScriptSet(FieldLayer* layer, i16 set);

#endif // GITEN_GAME_FIELDLAYER_H
