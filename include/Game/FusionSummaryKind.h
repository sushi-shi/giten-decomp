#ifndef GITEN_GAME_FUSIONSUMMARYKIND_H
#define GITEN_GAME_FUSIONSUMMARYKIND_H

#include <EnumDomain.h>
#include <Ints.h>

// The packed fusion summary's result kind. Special fusion kinds 1..9
// still need distinct identities.
// clang-format off
GZ_ENUM_BEGIN_SPLIT(FusionSummaryKind, i16)
    FUSION_SUMMARY_NO_RESULT = -1,
    FUSION_SUMMARY_LEVEL_HIGHER = 10,
    FUSION_SUMMARY_LEVEL_LOWER = 11,
    FUSION_SUMMARY_LEVEL_EQUAL = 12
GZ_ENUM_END_SPLIT(FusionSummaryKind)
// clang-format on

#endif // GITEN_GAME_FUSIONSUMMARYKIND_H
