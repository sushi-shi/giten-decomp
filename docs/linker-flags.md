# Candidate linking

The active command and defaults live in
[scripts/giten/graph/link.py](../scripts/giten/graph/link.py).
Run `giten link --help` inside `nix develop` for overrides.

Retail is a flat link: no incremental-link thunk band, the import tables merged
into `.rdata` (there is no `.idata`), and no `.reloc` with
`IMAGE_FILE_RELOCS_STRIPPED` set. The candidate therefore uses the Windows
subsystem's default `WinMainCRTStartup` (LIBC's), a map file, an explicit image base,
`/INCREMENTAL:NO` and `/FIXED`; `--incremental` isolates that variable. The
default adds `/OPT:REF /OPT:ICF`: retail keeps ordinary dead code but removes
unreferenced COMDATs. `--keep-all` selects `/OPT:NOREF` for contribution studies;
it retains unused CRT signal handlers and an import absent from retail.
`--no-icf` keeps identical COMDATs separate. Retail shares the three identical
CRT member-call wrappers and the pinned empty SDK constructor bodies. The
object order comes from the complete, evidence-backed
`config/retail/link_order.tsv`; an explicit `--order` overrides it. The library
line puts `libc.lib`
first and then the eight DLLs in retail's import-descriptor order (KERNEL32,
USER32, GDI32, ADVAPI32, DDRAW, DSOUND, DINPUT, WINMM), with `dxguid.lib` last.
Use the generated response file to inspect the exact object order, libraries,
and options for a particular build.

Candidate linking freezes the linker process's wall clock at the UTC time
decoded from the original `build/local/DDS.EXE` COFF header. The Nix environment
supplies `libfaketime`; the wrapper uses `faketime -f` with `TZ=UTC` and
`FAKETIME_DONT_FAKE_MONOTONIC=1`, leaving timeout clocks running normally.
The linker writes its own PE and map timestamps, and both must equal the
requested value. No image bytes are patched afterwards. `--real-time` uses
the current clock for contribution analysis. The lower-level
`giten tool link --at 'YYYY-MM-DD HH:MM:SS'` accepts an explicit frozen UTC time; without `--at`
that command keeps the current clock. This does not change Wine's runtime
selection or shared prefix configuration.

`config/retail/imports.tsv` records all observed DLL lookup identities, including
ordinal imports, with the caller symbols and SHA-pinned SDK archive members.
The import generator preserves genuine SDK members and both archive indexes,
serializing only the observed hint fields that differ. Replacements occupy
their original library-line positions. This reconstructs import metadata; it
does not claim recovery of the original import archives.

`config/retail/link_libraries.tsv` pins genuine static-library inputs separately
from DLL import metadata. `dxguid.lib` comes from the Microsoft DirectX
Foundations 6.1 SDK: its original `obj\i386\dxguid.obj` member contains all 391
GUID definitions, and its complete 6,256-byte readonly payload equals the
original executable. Supply that unmodified archive at
`build/local/lib/dxguid.lib`; binaries stay outside Git. The validator checks
the whole archive and member hashes, both Microsoft archive indexes, section
alignment, definitions, and raw original bytes before selecting it at the
existing `dxguid.lib` library-line position. Missing or mismatched artifacts
fail; the installed SDK's different GUID archive is not a fallback.

The contract records the preserved original SDK package's URL, byte count and
hashes. To recover the artifact, verify the user-supplied `dx61sdkimage.exe`
against those package hashes, then extract `lib/dxguid.lib`, `dxreadme.txt`, and
`license/DirectX SDK EULA.txt` with an offline archive tool such as `7z x`.
Do not run the installer or replace installed SDK headers. Preserve the readme
and license with the local package, and copy the unchanged library to the
contract's artifact path. Validate it without linking with
`python3 -m giten.graph.static_libraries` inside `nix develop`. The validator
does not synthesize GUID definitions, rewrite archives, or patch an image.

Compile matching and final-image matching are different checks. A function's
normalized COFF match does not establish final RVA placement, import binding,
or startup correctness. Use the checks below.

Do not infer original source ownership from proximity alone, fabricate padding
to force addresses, or equate identical COMDAT selection with arbitrary
identical-code folding. Retail instructions, relocations, and independently
identified contributions constrain those decisions.

## Checks

After `giten link`, run `giten verify link-tier` (or `--census` for section
sizes). It reads the candidate EXE/map under `build/exe/`, checks symbol closure
and section sizes, compares linked bytes of exact functions with relocation
masking, and compares `.rsrc` with the original's. A masked pointer still needs referent evidence; see [data attribution](data-attribution.md).

For raw binary identity, use
`giten verify link-tier --strict --report build/link-identity.json`.
This compares the original executable, full section
layout, headers, raw linked bytes and references. Only baseline MAX-partial
function bodies are eligible for exclusion; misplaced bodies and MAX-exact
current dips remain findings. The report records every exclusion and input
hash. Equal section sizes or a passing masked comparison alone do not establish
binary identity.

`giten link` compiles `.rsrc` from the recovered `src/Giten/Giten.rc`; the
payload files it names come from the original executable supplied through
`GITEN_RETAIL_EXE`, and the payloads and `.res` stay under ignored `build/`.
The link tier also compares the linked `.rsrc` with the original's
(`giten rsrc check` runs that comparison alone); only the section RVA may
differ. See [resource linking](build-system.md#candidate-linking-and-resources). External game files and valid
runtime settings are still required, and correct gameplay is not yet verified.
Do not use `/FORCE` or fabricated padding to hide
unresolved symbols, duplicate definitions, or placement differences.
