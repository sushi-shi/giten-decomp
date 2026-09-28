# Candidate linking

The active command and defaults live in
[scripts/giten/graph/link.py](../scripts/giten/graph/link.py).
Run `giten link --help` inside `nix develop` for overrides.

Retail is a flat link: no incremental-link thunk band, the import tables merged
into `.rdata` (there is no `.idata`), and no `.reloc` with
`IMAGE_FILE_RELOCS_STRIPPED` set. The candidate therefore uses the Windows
subsystem, `WinMainCRTStartup` (LIBC's), a map file, an explicit image base,
`/INCREMENTAL:NO` and `/FIXED`; `--incremental` isolates that variable. The
keep-all mode adds `/OPT:NOREF /OPT:NOICF`. The library line puts `libc.lib`
first and then the eight DLLs in retail's import-descriptor order (KERNEL32,
USER32, GDI32, ADVAPI32, DDRAW, DSOUND, DINPUT, WINMM), with `dxguid.lib` last.
Use the generated response file to inspect the exact object order, libraries,
and options for a particular build.

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
and section sizes, and compares linked bytes of exact functions with relocation
masking. A masked pointer still needs referent evidence; see [data attribution](data-attribution.md).

The candidate omits `.rsrc` until a resource script and assets are reconstructed.
It is a link/layout artifact. Do not use `/FORCE` or fabricated padding to hide
unresolved symbols, duplicate definitions, or placement differences.
