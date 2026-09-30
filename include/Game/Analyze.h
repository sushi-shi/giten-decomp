#ifndef GITEN_GAME_ANALYZE_H
#define GITEN_GAME_ANALYZE_H

#include <rva.h>

#include <EnumDomain.h>

// The analyze (DAS) window's steps: close; show the name; show the data;
// wait for a click; offer the detailed analysis; run that menu; show the
// detail on the status screen; and return from it. The detail shows a copy of
// the target through roster entry ANALYZE_ROSTER_ENTRY.
GZ_ENUM_BEGIN_SPLIT(AnalyzeStep, i16)
    ANALYZE_STEP_CLOSE = -1,
    ANALYZE_STEP_SHOW_NAME = 0,
    ANALYZE_STEP_SHOW_DATA = 1,
    ANALYZE_STEP_WAIT = 2,
    ANALYZE_STEP_OFFER_DETAIL = 3,
    ANALYZE_STEP_RUN_MENU = 4,
    ANALYZE_STEP_SHOW_DETAIL = 5,
    ANALYZE_STEP_END_DETAIL = 6
GZ_ENUM_END_SPLIT(AnalyzeStep)

#define ANALYZE_ROSTER_ENTRY 15

RVA_DECL(0x0001ad50)
i16 RunAnalyzeWindow(void);

#endif // GITEN_GAME_ANALYZE_H
