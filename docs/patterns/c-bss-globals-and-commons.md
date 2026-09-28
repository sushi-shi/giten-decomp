# A C global inside an object's .bss run was zero-initialized

VC5 compiles a C file-scope definition without an initializer (`int g;`) as a
COMMON symbol (section 0, value = size). `link.exe` 5.10 allocates COMMONs
after every object's and library member's `.bss` contribution, so they land in
the trailing block after LIBC's `.bss`. A definition with an explicit zero
initializer (`int g = 0;`, `T g[4] = {0};`) is emitted in the object's own
`.bss`, like a `static`. Uninitialized C++ globals are not COMMONs either.

Signature: a C global whose retail address lies inside a run of one object's
`.bss` statics. Spell it with a zero initializer. A datum in the trailing
COMMON block is an uninitialized C global with external linkage, never a
`static`; COMMONs there pack to their own size (two-byte ones sit two bytes
apart), while each object's `.bss` members are four-byte aligned.

Evidence: compile `int a; int b = 0; static int c;` with the `c` profile and
read the COFF symbol table (`b` and `c` are in `.bss`, `a` has section 0);
link two such objects and read the map (`<common>` entries follow both
`.bss` contributions).

Within one object, the uninitialized statics come first, in an order
derived from their names. The explicitly zero-initialized definitions
(`static` or global) follow them in definition order. A static that retail
places after a zero-initialized global in the same run therefore has a zero
initializer itself and is defined after that global.

Evidence: compile `static short s1; short g1 = 0; static short s2;
short g2 = 0; static short s3 = 0; short g3 = 0;` plus a few more
uninitialized statics with the `c` profile. The symbol table puts every
uninitialized static first, then `g1`, `g2`, `s3` and `g3` in that order.

Limits: VC5 does not emit uninitialized statics in declaration or first-use
order, so their addresses order objects, not source declarations. `.data`
follows declaration order. A `.bss` run does not say whether the retail file
was C with a zero initializer or C++. An unreferenced datum can fill a slot
in the run without any other trace.
