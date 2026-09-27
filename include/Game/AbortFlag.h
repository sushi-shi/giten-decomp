#ifndef GITEN_GAME_ABORTFLAG_H
#define GITEN_GAME_ABORTFLAG_H

#include <rva.h>

#include <Ints.h>

#ifdef __cplusplus
extern "C" {
#endif

    RVA_DECL(0x00045550)
    i16 IsAbortPending(void);
    RVA_DECL(0x00045560)
    void SetAbortPending(i16 pending);
    RVA_DECL(0x00045660)
    i16 ExchangeAbortPending(i16 pending);

#ifdef __cplusplus
}
#endif

#endif
