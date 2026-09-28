# Toolchain evidence

Retail DDS.EXE supports MSVC 5.0 / Visual Studio 97 SP3:

- PE linker version 5.10; the matching linker is `5.10.7303` (SP2 is `5.02.7132`).
- Rich records include `(19, 8034)`, untagged pre-Rich compiler objects, and
  `(6, 1668)` for `cvtres 5.00.1668`.
- Relocation-masked library matching identifies SP3 `LIBC.LIB`; `__setmbcp`,
  `_setSBCS` and `__output` distinguish it from SP2. Absence of `Tls*` imports
  also supports the single-threaded `/ML` runtime.
- No debug directory, COFF symbols or CodeView/PDB reference survives.

The investigation's `research/toolchain-attribution.md` and `tools/libmatch.py`
hold the measurements under `~/Projects/giten/investigation`.
Some DirectMusic GUIDs match the pinned SDK, but its `IID_IDirectMusic` and
`IID_IDirectMusicPerformance` definitions are absent from retail. The GUID
subset does not establish an exact SDK release or runtime DirectMusic use;
see [retail libraries and SDK helpers](vendor-libraries.md#sdk-revision-and-unused-guids).

Use [local setup](../README.md#quickstart) and the [compiler profiles](compiler-flags.md).
Toolchain identification does not establish per-object flags or source boundaries.
