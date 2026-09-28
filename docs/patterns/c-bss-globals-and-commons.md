# A C global inside an object's .bss run was zero-initialized

VC5 compiles a C file-scope definition without an initializer (`int g;`) as a
COMMON symbol (section 0, value = size). `link.exe` 5.10 allocates COMMONs
after every object's and library member's `.bss` contribution, so they land in
the trailing block after LIBC's `.bss`. A definition with an explicit zero
initializer (`int g = 0;`, `T g[4] = {0};`) is emitted in the object's own
`.bss`, like a `static`. Uninitialized C++ globals are not COMMONs either.

Signature: a C global whose retail address lies inside a run of one object's
`.bss` statics. Spell it with a zero initializer. A C global in the trailing
COMMON block stays uninitialized.

Evidence: compile `int a; int b = 0; static int c;` with the `c` profile and
read the COFF symbol table (`b` and `c` are in `.bss`, `a` has section 0);
link two such objects and read the map (`<common>` entries follow both
`.bss` contributions).

Limits: within one object, VC5 does not emit uninitialized statics in
declaration or first-use order, so `.bss` addresses order objects, not
source declarations. `.data` follows declaration order. A `.bss` run does not
say whether the retail file was C with a zero initializer or C++.
