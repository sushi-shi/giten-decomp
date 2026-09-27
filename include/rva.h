#ifndef GITEN_RVA_H
#define GITEN_RVA_H

#include <Ints.h>

#if defined(__clang__) && defined(GITEN_EMIT_META)

#define RVA(addr, size) __attribute__((annotate("rva:" #addr " size:" #size), used))

#define OVERRIDE override

#define DATA(addr) __attribute__((annotate("data:" #addr)))

// On a prototype with no body in this TU: names the retail function at `addr`
// (a label-only claim; a definition's RVA() supersedes it).
#define RVA_DECL(addr) __attribute__((annotate("decl:" #addr)))

// Attach to BEGIN_MESSAGE_MAP: the SDK owns both generated data definitions.
#define DATA_MESSAGE_MAP(map, entries)                                                             \
    __attribute__((annotate("mfc-map:" #map " entries:" #entries)))

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#else

#define RVA(addr, size)
#define RVA_DECL(addr)
#define DATA(addr)
#define DATA_MESSAGE_MAP(map, entries)
#define OVERRIDE

#define RVA_COMPGEN(addr, size, symbol)
#define RVA_DYNINIT(addr, size, owner)
#define DATA_COMPGEN(addr, value) value

#endif

#endif // GITEN_RVA_H
