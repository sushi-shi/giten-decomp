# C source cast census

`giten verify c-casts` parses every C translation unit in `config/units.toml`
with the generated target-C compile database. It lists explicit C-style casts
written in `src/` and project headers, deduplicated by Clang spelling location.
A macro cast expanded by several callers appears once with every observed
source type, target type and unit/function context.

```sh
python3 -m giten.graph.compdb
GITEN_DIR=$PWD giten verify c-casts
GITEN_DIR=$PWD giten verify c-casts --kind pointer --scope source --list
GITEN_DIR=$PWD giten verify c-casts --path src/Game/clock.c --json
```

The audit writes `build/gen/c_casts.tsv` and
`build/gen/c_casts_coverage.json`; `--no-report` skips those derived files.
`--kind` selects pointer, scalar or mixed target categories, `--scope` selects
C source or headers, and repeated `--path` options select source paths.
`--max N` fails when the selected written-site count exceeds N. The C++
`reinterpret_cast` ledger is separate.

The command fails if a C source is absent from the manifest or compile
database, a compile database entry is stale or duplicated, libclang reports
a target-C error, or project source changes during the scan. The coverage
report lists every project header visited by C units and those not visited.
Unvisited headers are outside this audit: some are C++ only, and inactive
preprocessor branches cannot be inferred from a target-C AST. A site count
must not be read as complete header coverage without reviewing that list.
For example, the current C units do not include `Platform/D3DApp.h`,
`Sound/MidiStream.h` or `Text/FontApi.h`; their C++ casts belong to the
separate `giten verify casts` review.

## Source-model review still open

- `src/Game/clock.c` converts decoded map bytes into `AreaLevel*`, `u8*` and
  `u16*` views. Some are genuine serialization boundaries; an owner or typed
  accessor can replace a cast only if all producer and consumer paths support
  that representation.
- `src/Script/scripttext.c` reads handle payloads as `u32*`, `i16*` and
  `char*` for different operations. Check the stored element kind and all
  callers before claiming a single array type.
- `src/Game/partyaction.c` casts `GetGamePhase()` and `GetGameStep()` to
  `u16` for switches, and `src/Game/fieldview.c` narrows a direction before
  indexing `s_wallStops`. Keep explicit narrowing where the retail access
  width or encoded domain requires it.

A lower cast count is a navigation signal, not proof of a better source model.
Match the affected instructions, call signatures and referents before keeping
a typed replacement. The [KF1 target-C cast audit](https://github.com/sushi-shi/kings-field-decomp/blob/master/docs/cast-audit.md)
provides the historical method behind this worklist.
