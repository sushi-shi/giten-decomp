#ifndef GITEN_GAME_POOL_H
#define GITEN_GAME_POOL_H

#include <EnumDomain.h>
#include <Ints.h>
#include <Util/CurMax.h>
#include <Game/PoolFillMode.h>

#define PayPoolCost(pool, cost)                                                                    \
    do {                                                                                           \
        if ((pool)->cur >= (u16)(cost)) {                                                          \
            (pool)->cur -= (cost);                                                                 \
            (cost) = 0;                                                                            \
        } else {                                                                                   \
            (cost) -= (pool)->cur;                                                                 \
            (pool)->cur = 0;                                                                       \
        }                                                                                          \
    } while (0)

// Drains the pool by -amount when amount is negative, else fills it up to its
// maximum. Declared apart from Game/Stats.h: there the prototype shifts the
// charpool and stattotal code.
void ChangePool(CurMax* pool, i32 amount);

// Refills a pool (mode 0 up to its maximum, 1 without a cap, 2 up to twice
// its maximum). Also declared in <Game/Stats.h>, which itemrecord.c cannot
// include: it shifts SetBagEntry there.
i16 FillPool(CurMax* pool, i32 amount, GZ_ENUM_PARAM(PoolFillMode, i16) mode);

#endif // GITEN_GAME_POOL_H
