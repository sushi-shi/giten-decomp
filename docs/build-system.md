# Build and comparison

Run inside `nix develop` with `GITEN_DIR=$PWD`; see [setup](../README.md#quickstart).

```sh
giten match range          # compile, label, delink and compare one unit; no gates
giten build                # every unit; no gates
giten build verify         # merge checks
giten verify status        # scores and regressions
```

`giten match` without arguments builds all units and reports changed objects.
`giten build` accepts Ninja arguments and targets: `-j8 -v`, `base`, `claims`,
`target`, `compare`, or `verify`. Candidate linking is opt-in.

## Pipeline

`config/units.toml` maps each unit to a source and a complete flag profile;
[compiler evidence](compiler-flags.md) explains the profiles. Add a TU by adding
its `[[unit]]` entry, shared declarations, source definitions and labels, then build.

| Stage | Implementation | Output under `build/` |
| --- | --- | --- |
| Configure | `giten.graph.emit` | `build.ninja` |
| Compile with VC5/Wine | `giten.graph.cc` | `objdiff/base/<unit>.obj` |
| clang database | `giten.graph.compdb` | `clangd/compile_commands.json` |
| Extract labels | `giten.retail_labels.source` | `gen/claims/<unit>.tsv` |
| Resolve claims/providers | `giten.model` | `gen/bindings.tsv`, `gen/violations.tsv` |
| Base-object data identity | `giten.graph.dataid` | `gen/data_ids.tsv` |
| Synthetic PDB and delink | `giten.delink` | `pdb/`, `objdiff/target-new/` |
| Normalize and compare | `giten.compare` | `objdiff/normalized/`, `objdiff/compare-new/report.json` |

Producers write only changed content. A code edit with unchanged labels reuses
the retail targets; a label change rebuilds the model and affected targets.
Label extraction reruns when its unit's source, header closure, base object,
clang database or extractor changes, and the unchanged fragment stops the
cascade. No content key skips it: a function body can change a claim (a
`DATA` static local, a C local sharing a global's name, a `DATA_COMPGEN`
payload, a clang-only error). Instead the libclang probes share one parse, and
clang's IR and AST passes run beside it.
The delink also reads the base objects' data topology, so a compile that moves
only data identity (a COMMON becoming `.bss`, a string or vtable COMDAT)
re-delinks through `gen/data_ids.tsv` without a label change.
A comparator re-pin invalidates reports without recompiling. Use
`giten build --force-delink` to force target regeneration.

The retail image has no relocations; [the reviewed site table](relocations.md)
supplies them in `build/exe/DDS.EXE`. Local inputs and all generated artifacts
stay under ignored `build/`. Command logs are `build/giten_usage.{log,jsonl}`;
failures include the output tail. Copy logs out before removing a worktree.

## Labels and comparison identities

Use the macros in [include/rva.h](../include/rva.h), never address comments:

| Macro | Use |
| --- | --- |
| `RVA(rva, size)` | Function definition; extent includes its trailing tables and filler. |
| `RVA_DECL(rva)` | Owner-header prototype for a body not yet reconstructed; owns no bytes. |
| `DATA(rva)` | Real global definition; identity, not linker placement. |
| `RVA_COMPGEN(rva, size, mangled)` | Deterministically named compiler-generated function. |
| `RVA_DYNINIT(rva, size, owner)` | Dynamic initializer pinned at its owning datum. |
| `DATA_COMPGEN(rva, value)` | Last-resort use-site pin for compiler-generated data. |
| `DATA_MESSAGE_MAP(map, entries)` | SDK-generated message-map data. |

Labels come from source annotations and the clang AST. The model joins them
with [retail providers](../config/README.md); conflicting identities or extents
are violations. A definition supersedes its declaration-only claim.
See [data attribution](data-attribution.md) for literals, header statics and COMMONs.

Normalization writes disposable copies, preserving original objects. It handles
compiler-private names, weak externals, COMMON storage and same-function table
labels. Explicit function aliases require matching complete retail bodies;
identical code alone does not establish shared identity. Alias references are
renamed without merging definitions. The normalizer checks its allowed changes.

EH funclets and registration records are derived from retail unwind metadata,
named by their owning function and compared separately from reconstruction totals.
For a proven relocation expression outside an object's extent, record the
site, owner and addend in `config/retail/reloc_referents.tsv`; do not invent storage.

## Scores and banking

CUR/MAX/HIST are defined in [AGENTS.md](../AGENTS.md#scores-and-workflow).
The ledger is `config/match_baseline.tsv`; fingerprints are generated in
`build/gen/func_fingerprints.tsv`. `state=absent` retains an unscored historical row.
The per-function classifier, not aggregate exact counts, determines regression.
The fingerprint cache tracks each unit's source, transitive local includes and
clang compilation database. Changed inputs trigger a reparse; only a changed
function AST resets MAX. Header comments and equivalent typedefs remain neutral.

Stage the reviewed source snapshot before `giten verify bank`. Banking writes
the ledger explicitly; normal builds may refresh only the README's generated
score block. Do not edit that block or maintain a second score report.

## Data matching

`config/compare.toml` controls `data_matching` separately from compiler flags.
The ledger records the mode; checking or banking across modes is refused.

| Behavior | `true` | `false` |
| --- | --- | --- |
| Code references to data | Compare identity and addend | Replace data targets with a shared symbol and zero their addends in comparison copies |
| Unprovided game data | Delinker refuses `UNPROVISIONED_` fences | Emits `DAT_` fences; lists `build/gen/data_debt.tsv` |
| Undefined data externs | Fail | Advisory debt |
| Data placement/coverage and relocation referents | Fail | Advisory; unscored-unit/orphan-payload integrity failures still fail |
| `data-identity`, `data-access` | Fail | Fail |

Relaxation leaves instructions, immediates, function calls/pointers, IAT slots,
jump tables, EH records and data-section relocations strict. It can hide a
wrong datum or member offset at 100%. A plain extern in its owner's header is
temporary debt, not a definition.

`giten verify data-identity` compares pre-relaxation references where relocation
layouts and operand opcodes agree, rejecting contradictory source/retail identities.
Its `build/gen/data_identity.tsv` map is incomplete when sites cannot be paired;
inspect raw ordered referents for those cases.
`giten verify undefined-closure --list` lists placeholder externs;
each build reports both debt counts.

### Re-enabling data matching

1. While relaxed, run `giten verify data-identity` and retain its map. Prove
   unpaired extern addresses from retail references.
2. Set `data_matching = true`, build, and resolve every debt entry with a real
   owner definition/`DATA` claim or supported provider. Strict delinking and
   merge checks refuse the remaining debt.
3. With debt cleared, stage the source changes and run
   `giten verify bank --rebase-data-matching`. This resets MAX and HIST to current
   scores and removes absent rows: modes are incomparable. Run `giten build verify`
   and commit the ledger with the switch once the gates pass.

## Gates

`giten build verify` runs the MAX gate and fast/normal tiers. The authoritative
roster is [tiers.py](../scripts/giten/verify/tiers.py); `giten verify --help`
lists individual commands.

| Tier | Scope |
| --- | --- |
| `fast` | Source rules and ledger checks |
| `normal` | Model ownership, closure, review claims and data integrity |
| `full` | Optional vtable, allocation and caller/referent audits |
| `link` | Optional [candidate-image checks](linker-flags.md) |

Gates report findings; they do not bless baselines. `giten verify board --update`
and `giten verify bank` are explicit writes. Review the modeling change before
accepting a new floor; lowering counters is not evidence of correctness.
Verification policy for matching and tooling is in [AGENTS.md](../AGENTS.md#tests-and-repository-hygiene).

## Candidate linking and resources

`giten link` links the compiled objects into `build/exe/DDS.candidate.EXE`
and a map with VC5 `link.exe`. Linking is opt-in and has no `/FORCE` fallback;
unresolved or duplicate symbols are findings to fix in source.

Resources split into code and payloads. The code is tracked source:
`src/Giten/Giten.rc` declares every resource with its type, ID and language
(`LANGUAGE LANG_JAPANESE, SUBLANG_DEFAULT`), in the order that reproduces the
original's resource data, and `include/Giten/Resource.h` names the IDs. IDs
whose role the code shows carry that name (the software cursor images, the
world-map marker bitmaps, the analyze plate frame, and `IDR_SOUND_n` for the
WAVE that sound effect n plays through `s_soundResources`); the rest use the
resource editor's neutral scheme (`IDB_BITMAPn`, `IDC_CURSORn`, `IDI_ICONn`),
numbered by ascending ID within their type. The C sources do not include the
header yet: opening a new header changes a unit's code generation even when
the header holds only macros ([TU context](patterns/tu-state-probe-family-decides-reachability.md#header-files-not-macros)),
so resource IDs stay literal there until a unit's include set is evidence-backed.

The payloads never enter Git. Three opt-in edges build the resource object:

| Edge | Input | Output |
| :-- | :-- | :-- |
| `rsrc_payloads` (`giten.rsrc.payloads`) | `GITEN_RETAIL_EXE` | `build/gen/rsrc/*.bmp\|wav\|cur\|ico` and the `payloads.tsv` listing |
| `rc` (`giten.tool.rc`) | `Giten.rc`, `Resource.h`, the listing | `build/gen/giten.res` |
| `link` | base objects, `giten.res` | the candidate EXE and map |

Payload files are the forms RC.EXE reads back to the original bytes: each
`RT_BITMAP` with its `BITMAPFILEHEADER` restored, each WAVE as stored, and
each `RT_GROUP_CURSOR`/`RT_GROUP_ICON` reassembled with its `RT_CURSOR`/
`RT_ICON` images into a `.cur`/`.ico`. RC renumbers those images in script
order (cursors 1 and 2, icon 3), which is why the script order matters beyond
the data layout. `giten rsrc extract --disc IMAGE --out DIR` reads
`DDSWIN/DDS.EXE` from a 2352- or 2048-byte-sector disc image instead.
Resource memory flags are not stored in a PE image, so the script uses RC's
defaults.

`giten rsrc check` links the candidate and compares its `.rsrc` with the
original's: the directory tree (types, names, languages, entry order and table
headers), every payload's bytes, every data entry's code page, the offsets of
tables, data entries and payloads within the section, and finally the whole
section with each data entry's `OffsetToData` made section-relative. The
compiled script reproduces the original exactly under that comparison. The
one field outside the script's control is the section RVA, which follows from
the sizes of the sections placed before `.rsrc`; until those match, every
`OffsetToData` differs by the same delta, which the check reports. The link
tier runs the same comparison.

The candidate still needs the original installation's external game files and
valid runtime settings. See [local build/run instructions](../README.md#local-candidate-and-resources).
Optional candidate-image
checks remain in `giten verify check --tier link`.

## Formatting and navigation

The Nix shell enables `.githooks/pre-commit`: clang-format formats and re-stages
whole staged C/C++ files under `src/` and `include/`, so partial staging is not
preserved. `vendor/` is excluded. `.clang-format` defines the style.

`giten sema` reads retail assembly, symbols and references; `giten sema -` accepts
batch queries on stdin. `giten walls diagnose` compares the normalized pair.
[clangd and `giten lsp`](clangd.md) navigate source; VC5/objdiff decide matching.
