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
hashed from their names. The explicitly zero-initialized definitions
(`static` or global) follow them in definition order. Every item of eight
bytes or more starts on an eight-byte boundary; smaller ones take four-byte
slots. A static that retail places after a zero-initialized global in the
same run therefore has a zero initializer itself and is defined after that
global.

Signature: an object's `.bss` run that interleaves globals read by other
objects with statics, or that follows the order in which its code reads
them (counters from the smallest unit up, a queue's handle, capacity, read
and write positions). Giten's C objects zero-initialize their file-scope
statics: define the run with zero initializers in address order. Where one
item is read before the code that precedes it in the run, the whole run sits
ahead of that code. Compiling the object then reproduces the run's retail
offsets, including the eight-byte alignment gaps.

Evidence: compile `static short s1; short g1 = 0; static short s2;
short g2 = 0; static short s3 = 0; short g3 = 0;` plus a few more
uninitialized statics with the `c` profile. The symbol table puts every
uninitialized static first, then `g1`, `g2`, `s3` and `g3` in that order.
Uninitialized statics named for a route queue (`s_route`, `s_routeCapacity`,
`s_routeRead`, ...) come out scattered. Arrays and structures of 8, 12, 16
and 64 bytes after a two-byte static all start eight-byte aligned, with or
without `/Zp1`, `/O1` or `/Od`.

Limits: the hashed order of uninitialized statics orders objects, not
source declarations, and cannot be reproduced without the original names.
`.data` follows declaration order. A `.bss` run does not say whether the
retail file was C with a zero initializer or C++ (an uninitialized C++
global joins the hashed block). An unreferenced datum can fill a slot in
the run without any other trace. A retail item of eight bytes or more on a
four-byte boundary is several smaller definitions, not one.
