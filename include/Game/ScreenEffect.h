#ifndef GITEN_GAME_SCREENEFFECT_H
#define GITEN_GAME_SCREENEFFECT_H

#include <EnumDomain.h>
#include <Ints.h>

struct BmpFile;

// Image sequence loaded by the screen-effect game state.
struct BmpFile* GetScreenEffectImage(void);
i32 GetScreenEffectImageSize(void);
void PushScriptAnimation(i16 animation, i16 x, i16 y);
GZ_ENUM_BEGIN_SPLIT(ScriptAnimationPhase, i16)
    SCRIPT_ANIMATION_LOAD = 0,
    SCRIPT_ANIMATION_RUN = 1,
    SCRIPT_ANIMATION_CLOSE = 2
GZ_ENUM_END_SPLIT(ScriptAnimationPhase)

b16 RunScriptAnimationState(void);
void LoadScriptAnimation(i16 resource);
void LoadScriptAnimationImage(i16 resource);
void GetScriptAnimationPosition(i16 x, i16 y, i16* screenX, i16* screenY);

#endif // GITEN_GAME_SCREENEFFECT_H
