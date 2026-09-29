#ifndef GITEN_PLATFORM_SCREENFADE_H
#define GITEN_PLATFORM_SCREENFADE_H

#include <rva.h>
#include <EnumDomain.h>
#include <Enums.h>

// clang-format off
GZ_ENUM_BEGIN_SPLIT(ScreenFadeMode, i16)
    SCREEN_FADE_NONE = 0,
    SCREEN_FADE_FROM_BLACK = 1,
    SCREEN_FADE_TO_BLACK = 2,
    SCREEN_FADE_FROM_WHITE = 5,
    SCREEN_FADE_TO_WHITE = 6
GZ_ENUM_END(ScreenFadeMode);
// clang-format on

#define IsScreenFadeIn(mode) ((mode) & 1)

#ifdef __cplusplus
extern "C" {
#endif

    // The mode stays a signed word at the platform and game-state boundary.
    RVA_DECL(0x00049d60)
    void StartScreenFade(GZ_ENUM_PARAM(ScreenFadeMode, i16) mode, i16 steps);
    void StepScreenFade(void);
    GZ_ENUM_RETURN(ScreenFadeMode, i16) GetScreenFade(void);
    void FinishScreenFade(void);

#ifdef __cplusplus
}
#endif

#endif // GITEN_PLATFORM_SCREENFADE_H
