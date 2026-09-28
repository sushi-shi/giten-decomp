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
| Synthetic PDB and delink | `giten.delink` | `pdb/`, `objdiff/target-new/` |
| Normalize and compare | `giten.compare` | `objdiff/normalized/`, `objdiff/compare-new/report.json` |

Producers write only changed content. A code edit with unchanged labels reuses
the retail targets; a label change rebuilds the model and affected targets.
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

The resource edge (`giten.rsrc.retail_res`) reads the original EXE named by
`GITEN_RETAIL_EXE` and writes ignored `build/gen/retail.res`. The linker places
those payloads at the candidate's own resource RVA. This path needs no RC.EXE
or reconstructed resource script. Changing the supplied EXE rebuilds the
resource file and candidate; no resource payloads or download links are tracked.

The candidate still needs the original installation's external game files and
valid runtime settings. See [local build/run instructions](../README.md#local-candidate-and-resources)
and [runtime validation](runtime-validation.md). Optional candidate-image
checks remain in `giten verify check --tier link`.

## Formatting and navigation

The Nix shell enables `.githooks/pre-commit`: clang-format formats and re-stages
whole staged C/C++ files under `src/` and `include/`, so partial staging is not
preserved. `vendor/` is excluded. `.clang-format` defines the style.

`giten sema` reads retail assembly, symbols and references; `giten sema -` accepts
batch queries on stdin. `giten walls diagnose` compares the normalized pair.
[clangd and `giten lsp`](clangd.md) navigate source; VC5/objdiff decide matching.
