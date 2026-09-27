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
or startup correctness. See [image comparison](image-diff.md).

Do not infer original source ownership from proximity alone, fabricate padding
to force addresses, or equate identical COMDAT selection with arbitrary
identical-code folding. Retail instructions, relocations, and independently
identified contributions constrain those decisions.
