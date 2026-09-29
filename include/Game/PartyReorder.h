#ifndef GITEN_GAME_PARTYREORDER_H
#define GITEN_GAME_PARTYREORDER_H

#include <Enums.h>
#include <Ints.h>

// RunPartyReorder's phases: open (straight on to the first pick), close, and
// pick the two party positions to swap.
GZ_ENUM_BEGIN(PartyReorderPhase)
    REORDER_PHASE_OPEN = 0,
    REORDER_PHASE_CLOSE = 1,
    REORDER_PHASE_PICK_FIRST = 2,
    REORDER_PHASE_PICK_SECOND = 3
GZ_ENUM_END(PartyReorderPhase)

// PickReorderSlot's results when it returns no party position.
#define REORDER_PICK_PENDING (-1)
#define REORDER_PICK_CANCELLED (-2)

b16 RunPartyReorder(void);
i16 PickReorderSlot(void);

#endif // GITEN_GAME_PARTYREORDER_H
