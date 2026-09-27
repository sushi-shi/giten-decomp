# Compiler identification

DDS.EXE was built with MSVC 5.0 and the Visual Studio 97 SP3 toolchain.
The expected local toolchain has these versions and libraries:

```text
link.exe:    5.10.7303
cvtres.exe:  5.00.1668
required libraries: LIBC.LIB
```

The evidence, all from the retail image:

- The PE linker version is 5.10 (the SP3 linker; VS97 SP2's is 5.02.7132).
- The Rich header has the same shape as Gruntz's, a known VC5 + SP3 build:
  `(prodid 19, build 8034)`, `(0, 0)` untagged objects (the pre-Rich `cl`
  11.00), and `(6, 1668)` (cvtres 5.00.1668).
- The statically linked runtime is VS97 SP3 `LIBC.LIB`. 199 bodies match
  exactly at one address under relocation masking; `__setmbcp`, `_setSBCS`
  and `__output` match only SP3's library, nothing only SP2's. LIBCMT explains
  far fewer bodies and there are no `Tls*` imports, so the model is `/ML`.
- No debug directory, no COFF symbols, no PDB/CodeView/.dbg reference.

Those metadata do not by themselves recover each object's compiler invocation,
SDK revision, flags, or source boundaries. Matching against retail with the
pinned toolchain remains necessary. The measurements, match tables and tools
live in the investigation (`~/Projects/giten/investigation/research/
toolchain-attribution.md`, `tools/libmatch.py`).

The DirectX headers the game compiled against are DirectX 6.1 (DirectMusic
interface GUIDs in `.rdata`, no DirectX 7 ones). The pinned package carries
`dmusic*.h`; confirm its exact SDK revision before relying on it for GUID or
data layout.

Use [toolchain setup](toolchain-vc50-sp3.md) to reproduce the package and
[compiler profiles](compiler-flags.md) for the active manifest.
