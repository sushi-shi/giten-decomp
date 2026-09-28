# clangd and source navigation

Inside `nix develop`, generate the database with `giten build` or:

```sh
python3 -m giten.graph.compdb
python3 -m giten.graph.compdb --check
```

Open the repository root in a clangd-enabled editor. `.clangd` points to
`build/clangd/compile_commands.json`; `giten lsp` uses the same database for
references, hover and USR-based renames. Regenerate it after changing toolchain paths.

The database also supplies label extraction and source fingerprints. It uses
clang-cl's i386/MSVC 11.00 compatibility mode and the VC5/DirectX headers,
with lowercase include mirrors under `build/clangd/inc-lower/`. DirectX headers
precede VC5's older SDK copies. The generator checks coverage through the consumer.

`.clangd` keeps `-ferror-limit=0` because stopping early on SDK dialect errors
can lose the rest of the AST. Clang reads this dialect approximately; its
diagnostics do not establish build or match correctness. Use VC5 and objdiff.
