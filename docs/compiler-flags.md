# Compiler profiles

[config/units.toml](../config/units.toml) defines the complete profiles and
per-TU selections. MSVC 5.0 SP3 is identified from [retail evidence](compiler-detection.md).

| Profile | Flags (besides `/nologo /c`) |
| --- | --- |
| `c` | `/Ox /Zp1 /ML` |
| `cpp` | `/Ox /Ob0 /Zp1 /ML /GX` |
| `cpp-noeh` | `/Ox /Ob0 /Zp1 /ML` |

Evidence for the defaults:

- `/Ox`: optimized, frame-pointer-omitted code without `/Gf` string pooling
  or `/Gy` function packaging. `OpenDataFile` retains thirteen separate `"rb"`
  literals; the platform layer also repeats `"CLASSSDDSWIN"` and error strings.
  `/O2` pools literals and changes cross-jumped tails on these witnesses.
- `/Ob0` in C++: the empty DX6 vertex constructor at VA `0x4568c0` is called
  out of line from six platform functions; VA `0x44f260` also calls the vector
  constructor iterator. `/Ob1` expands these; `/Ob0` reproduces the calls.
- `/Zp1`: the party record has a word at `+0x1f1`, a dword at `+0x1f3`, and
  allocation size `0x21f`. C++ texture records have stride `0x43e`, rather than
  naturally aligned `0x440`. SDK headers select their own packing.
- `/ML`: retail matches SP3 `LIBC.LIB`, not the multithreaded runtime.
- `/GX` in the Windows layer: six functions install C++ exception frames.
  There are no RTTI type descriptors supporting `/GR`.
- Stack cleanup supports default cdecl. The processor switch (`/G5`, `/GB`)
  remains unresolved.

These are compatibility constraints, not a recovered IDE command line.
For a profile change, compare the same source with the pinned compiler across
affected owners: code, data, calling conventions, ordered relocations and
runtime-library directives. One function's score does not establish a global flag.
See [candidate linking](linker-flags.md) for linker settings.
