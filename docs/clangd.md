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

## Boolean values

`python3 -m giten.tool.bool_returns` reports functions whose return values are
provably zero or one, and initialized locals whose writes stay in that domain.
Globals, file/function statics, static members, and scalar fields are checked
across all compilation-database translation units as well. Add `--write` to
update their types, function declarations, and literals under `src/` and
`include/`. The pylibclang script uses the same compilation database, follows direct calls recursively, and checks conditional
returns, bitwise combinations of proven flags, and initialized local flags.
Locals are checked in every function, including functions returning other
values or `void`. Mixed declarations are left intact when a sibling is not
proven Boolean. Unknown values, escaping locals, indirect or virtual calls,
uneditable macros, and missing return paths are left alone.

Storage needs an owned definition, proven Boolean initializers/writes, and a
flag-domain witness (a possible one, a comparison/logical result, or an existing
Boolean type). Zero-only cleanup handles and unclassified cleared words are
left alone. The
scan includes constructor member initializers and distinguishes value arguments
from mutable references. Address escapes, unknown member implementations,
aggregate initialization/copies, raw-memory access, volatile storage, and
bitfields are excluded. Whole-object exclusions also cover nested records.
This is a closed-source analysis: unmodeled writers outside the compilation
database require manual review. Parse errors abort the rewrite.

`Ints.h` owns the boolean aliases and C `true`/`false` definitions. Storage and
return width and signedness are preserved; C++ `bool` stays `bool`. Integer-valued C++
ternaries retain their arithmetic type through explicit casts. Review the
report and run `giten build verify` after rewriting: value inference alone
does not establish domain meaning or byte equivalence to retail.
