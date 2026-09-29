#ifndef GITEN_GAME_ABORTFLAG_H
#define GITEN_GAME_ABORTFLAG_H

#include <rva.h>

#include <Ints.h>

#ifdef __cplusplus
extern "C" {
#endif

    RVA_DECL(0x00045550)
    b16 IsAbortPending(void);
    RVA_DECL(0x00045560)
    void SetAbortPending(b16 pending);
    RVA_DECL(0x00045660)
    b16 ExchangeAbortPending(b16 pending);

#ifdef __cplusplus
}
#endif

#endif
