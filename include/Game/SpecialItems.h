#ifndef GITEN_GAME_SPECIALITEMS_H
#define GITEN_GAME_SPECIALITEMS_H

#include <Ints.h>

// The eight lover's body-part items: obtaining one stamps an expiry time (a
// moon cycle of 42560 clock minutes later) into script variables 0xd0..0xd7;
// once a held one expires, the clock clears its event flag.

// The event flag (bank, index) a timed item clears when it expires; a bank of
// -1 clears none.
typedef struct TimedItemFlag {
    i16 bank;
    i16 index;
} TimedItemFlag;

// Stamps timed item `item`'s expiry; returns its index (-1 for another item).
i16 StampSpecialItem(i16 item);

// Clears timed item `item`'s event flag; returns its index (-1 for another
// item).
i16 ExpireSpecialItem(i16 item);

// Whether timed item `item`'s expiry has passed (-1 for another item).
i16 IsSpecialItemExpired(i16 item);

// Expires every held timed item whose time has passed; returns how many.
i16 ExpireSpecialItems(void);

#endif // GITEN_GAME_SPECIALITEMS_H
