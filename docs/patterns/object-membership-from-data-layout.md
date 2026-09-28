# Data layout marks object boundaries

VC5 `cl` writes a translation unit's initialized data to `.data` (and its
`const` data to `.rdata`) in definition order, including function-local
statics at their point of definition. It writes every string literal after
all of the TU's initialized data. Uninitialized statics, zero-initialized C
globals and C++ globals go to the object's `.bss` in an order that follows
their names, not their definitions. `link.exe` 5.10 lays each object's
contributions out contiguously and in link order (the `.text` order).

Signatures:

- A static inside another unit's `.bss` run, with that unit's statics on
  both sides, sits in the same object. When each static is read only by its
  own unit's code, the two units are one TU. A global in another unit's run
  proves less. It may be defined there and read elsewhere.
- Initialized data of several units that interleave in `.data`, or that
  reverse their `.text` order, belong to one TU that defines them in retail
  order. With declaration before use, that order can require a data block
  ahead of the code that reads it.
- A literal between two initialized data ends one object's `.data` run. The
  data after it belong to the next object. Without `/GF`, every object that
  uses a literal, including one from a shared macro, gets its own copy.
- A unit whose `.text` is split by another unit is not one object. Use the
  data it touches to place the boundary. For example, leading helpers that
  touch only statics in the previous object's run belong to that object.

Evidence: compile a scratch file with the `c` profile and read the COFF symbol
table. Test initialized statics and globals defined around several functions
that use literals, and uninitialized statics defined in reverse order. The
data follow definition order and every `$SG` literal follows them. The `.bss`
layout does not change when the definitions are reversed. For link order, see
[C globals and COMMONs](c-bss-globals-and-commons.md).

Limits: the recovered names are not the original names, so `.bss` order says
nothing about source order. An unreferenced datum can occupy a gap without
leaving any other trace. Retail order and declaration before use bound where a
folded TU defines its data, but do not fix that position. Folding units
changes TU state, and unchanged functions in it can gain or lose exact matches.
