# Compiler profiles

[config/units.toml](../config/units.toml) is the executable manifest for compiler
profiles and per-TU selections. Inspect it before changing flags.

The standard profiles use MSVC 5.0 SP3:

```text
C:                   /Ox /Zp1 /ML
C++ (platform layer): /Ox /Ob0 /Zp1 /ML /GX
C++ without EH:      /Ox /Ob0 /Zp1 /ML
```

The evidence for the defaults (retail bytes, not a recovered IDE setting):

- Optimized for speed with frame-pointer omission: no non-CRT function opens
  with `push ebp; mov ebp,esp`, locals are `[esp+x]`-addressed, and there is no
  store-then-reload of a frame slot. `/O2` (which implies `/Oy`), not `/Od`.
- Functions are 16-byte aligned with `0x90` padding.
- Callers clean the stack: the default `/Gd` cdecl.
- The game is C: no `thiscall` outside one band. The band `0x445d60`-`0x456a50`
  (314 functions) is the Windows port's own platform layer - window and message
  loop, input, the Direct3D dungeon renderer, DirectDraw/GDI, fonts, WAVE and
  the `.mds` MIDI-stream player - not third-party middleware. 40 of its
  functions take `this` in `ecx` (C++ members), and only 6 functions install an
  `fs:[0]` EH frame, so `/GX` applies there. There are no RTTI type descriptors,
  so no `/GR`. Its DirectDraw/Direct3D error-name table shares 116 of the 117
  names of the DirectX 6.1 SDK samples' error-to-string helper plus 18 newer
  codes: likely sample-derived, extended.
- The C code has no string pooling and no function-level linking: `/Ox`, which
  is VC5 `/O2` minus `/Gf` and `/Gy`. OpenDataFile (0x1dd0) keeps thirteen
  separate `"rb"` literals and two copies each of its `fc\\fc%.4x%01d.bin` and
  `et\\et%.4x.bin` formats inside one function, which `/Gf` would pool (and
  whose cross-jumped tails then change). The image keeps many zero-reference
  functions, which `/Gy` plus the linker's default `/OPT:REF` would strip.
  Every C unit scores the same or better under `/Ox`. The C++ layer shows the
  same: "CLASSSDDSWIN" appears twice in 0x450110 and once elsewhere, and each
  error-trace function keeps its own "Unknown Error" and "\n"; under `/O2` cl
  emits C++ literals as pick-any `??_C@` COMDATs that pool them. Every C++
  unit scores the same or better under `/Ox`.
- The C++ layer does not inline: `/Ob0`. The empty in-class constructor
  0x4568c0 (`mov eax,ecx; ret`, the DX6 `D3D_OVERLOADS` vertex ctor) is
  called and tail-jumped out of line from six platform functions, and
  0x44f260 calls the compiler's `vector constructor iterator` out of line;
  `/O2` (`/Ob1`) inlines both, and only `/Ob0` reproduces 0x44f260.
- The C structures are byte-packed: `/Zp1`. The party record keeps a word at
  +0x1f1 and a dword at +0x1f3 (0x40667d `mov ax,[edi+0x1f1]`, 0x406694
  `mov edx,[edi+0x1f3]`) and is allocated as 0x21f bytes (0x42a2c9
  `push 0x21f`); only 1-byte packing gives odd offsets and an odd size. No
  game structure needs more: the ones that differ from natural alignment keep
  4-byte fields at +2 (the motion and shot tables read `[base+i*4+2]` at
  0x404a7b and 0x40563a, the script call frame a dword at +0xe) or have a
  10-byte size (the script timed-list entry), all of which `/Zp1` also gives,
  and none has a proven offset that only natural or 2-byte packing produces.
  The SDK headers set their own packing (`pshpack*.h`), so they are not
  witnesses. Switching the profile left every scored function byte-identical.
- The C++ layer packs the same way: the texture record is 0x43e bytes (an
  i16 at +0x43c after DWORD fields) and 0x44fdf0 walks its arrays at 0x484d20
  and 0x48d860 with stride 0x43e, which natural alignment would round to 0x440;
  `/Zp1` matches the C profile, and no C++ unit's score moves under it.
- Unresolved: the processor switch (`/G5`, `/GB`).

Do not infer a project-wide switch from one function's prologue or change flags
solely to improve a local score. Compile-profile compatibility is not recovery
of the original IDE command line: multiple options can produce identical output
on a selected witness.

For an actual profile change, retain a same-source control with the pinned
compiler. Compare complete code, data, calling conventions, ordered relocations,
and runtime-library directives across affected owners. Byte-neutral output on
one function does not establish that a flag is globally harmless.

See [compiler identification](compiler-detection.md) for binary provenance,
[the build system](build-system.md) for generated commands, and
[linking](linker-flags.md) for the separate candidate-image configuration.
