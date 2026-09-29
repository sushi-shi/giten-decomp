#ifndef GITEN_GAME_ACTIONWAIT_H
#define GITEN_GAME_ACTIONWAIT_H

#include <Ints.h>

// The action readiness mark and its remaining wait, shared by roster and
// field actor records.
typedef struct ActionWait {
    i8 ready;
    u16 remaining;
} ActionWait;

#define IsActionWaitMarked(wait) ((wait)->ready != 0)

#define IsActionWaitPending(wait) ((wait)->remaining != 0)

#define IsActionWaitPickable(wait) ((wait)->ready <= 1 && (wait)->remaining <= 1)

#define ResetActionWaitDelay(wait) ((wait)->remaining = 0xff)

static __inline void DelayActionWait(ActionWait* wait, u16 amount) {
    wait->remaining = amount + wait->remaining;
}

static __inline void ResetActionWait(ActionWait* wait) {
    wait->ready = 0;
    ResetActionWaitDelay(wait);
}

static __inline void QueueActionWait(ActionWait* wait) {
    wait->remaining = 1;
    wait->ready = 1;
}

#define ClearActionWait(wait)                                                                      \
    do {                                                                                           \
        (wait)->ready = 0;                                                                         \
        (wait)->remaining = 0;                                                                     \
    } while (0)

#endif // GITEN_GAME_ACTIONWAIT_H
