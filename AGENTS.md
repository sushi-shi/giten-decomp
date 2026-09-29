# Giten decompilation

Recover Giten's original C and Windows-layer C++ with MSVC 5.0 SP3.
Correct types, ownership, control flow, calling conventions, and referents
outrank fuzzy scores. Compiler profiles live in `config/units.toml`.

## Environment

- Work in `nix develop`; set `GITEN_DIR=$PWD` for this worktree.
- Never use Ghidra's decompiler on `DDS.EXE`. Use assembly,
  xrefs, RTTI, vtables, data, and relocations.
- `CLAUDE.md` links here; `.claude/skills` links to `.agents/skills`. Edit the
  canonical files. See [setup](README.md#quickstart) and [build](docs/build-system.md).

## Scores and workflow

`config/match_baseline.tsv` tracks CUR (latest bank), MAX (best for the current
source hash), and HIST (all-time peak): `CUR <= MAX <= HIST`. Editing a function
resets MAX to CUR; an unchanged function's CUR dip is not a regression.
MSVC 5.0 codegen depends on TU state: a new `#include` (even an empty header),
enumerator or defined symbol can shift unrelated functions. Such dips keep MAX;
do not skip evidence-backed headers, enums, or names to keep other objects
byte-identical.
Aim for MAX = 100. `HIST > MAX` identifies a lost match to recover from Git.

1. Pick ascending-HIST work with `giten walls inventory --todo --limit N`, or
   a lost match. Read `giten walls priors <rva>`.
2. Run `giten walls diagnose <rva> --asm`. Fix the first divergence: referent,
   inline/call-set, CFG, then register/schedule. Use the `matcher` skill;
   use `wall-identifier` for plateaus and `holista` for hidden helpers.
3. Iterate with `giten match <unit|source>`; run `giten build` for changes
   spanning units. The loop reports MAX changes and records source-hash resets
   in `docs/todos/syntactic-recovery.tsv`; do not chase resets in this pass.
4. Use `permute` only for diagnosed register/schedule residue with HIST < 100.
   Probes are disposable. If unchanged source becomes exact, bank it while
   exact, then remove the probe. Never keep fake locals or unused declarations.
5. Keep evidence-backed structure through intermediate score dips; compose
   the next lever. Prefer recovered helpers/macros over hand expansion.
   Use `@early-stop` only for a complete body with evidence-bounded residue.
6. Before merging, run `giten build verify` (MAX gate plus fast/normal tiers).
   Whenever preparing a PR for merge, regenerate the README score block with
   `giten verify readme` after the final build and include the updated
   `README.md` in the PR.
   Keep an exact match even if it violates a rule: add the gate's allow entry
   and a row in `docs/todos/rule-exceptions.tsv` in the same commit.

Raw instructions, constants, and ordered relocations decide correctness.
With `data_matching = false`, objdiff hides data targets and addends; plain
externs in owner headers are temporary debt. Follow [Data matching](docs/build-system.md#data-matching).
PC-98 `DDS98.EXE` in `~/Projects/giten/investigation` is a naming/helper-boundary
witness, not evidence for this compiler's code shape.

## Source rules

- One type definition in a shared header; functions/globals belong to their
  evidence-backed owner TU/header. No local layout views, placeholder shells,
  scattered externs, address-derived names, compiler ordinals, or hex aliases.
- Use typed members, not raw offsets or casts of `this`. No C-style casts;
  named casts belong only at real boundaries. Use typed enums for proven domains; parameter
  retyping changes mangling. Prove aggregates from whole-object use.
- Trace callers, storage, mangling, vptr stores, RTTI, and offsets before
  naming identity; otherwise leave `@identity-TODO`. Never fabricate storage,
  padding, code, or overlapping globals to improve a score or final placement.
- Derive vtables with `giten sema class <Class>`: no inherited redeclarations,
  `OVERRIDE` for overrides, `virtual` for new slots, no dummy slots.
- Platform headers come through `<Win32.h>`; no hand-rolled SDK declarations.
  Include CRT headers directly.
- Labels use `include/rva.h`. Pin dynamic initializers with `RVA_DYNINIT` at
  their owner. `DATA` records identity, not placement. Write literals bare;
  reserve `DATA_COMPGEN` for identities the oracles cannot reach. Header
  COMMONs/copies use `config/retail/data_compgen.tsv`; see [data attribution](docs/data-attribution.md).
- Fix false/missing relocation sites in `config/retail/reloc_sites.tsv`.
  Data-section percentages alone do not prove storage or referent correctness.

## Tests and repository hygiene

- Matching/modeling runs no test suites: compile/compare is verification.
  For tooling, run relevant retained `test_*.py` modules from `scripts/` with
  `python3 -m unittest giten.<pkg>.test_<name>`, then the affected command.
- Keep tests for scoring, banking, masked data identity, and safe writes.
  Do not add retail snapshots, source-spelling or CLI-output tests, or require
  a test for every gate.
- Inputs go in `config/`, generated artifacts in ignored `build/`, history in
  Git. Docs explain current usage/contracts, not campaigns or score snapshots.
  [Patterns](docs/patterns/README.md) need distinct, reproducible mechanisms.
- Source comments are for [markers](docs/comment-markers.md), ABI/codegen
  constraints, and unsafe boundaries; no history, scores, addresses, or banners.
- Preserve concurrent work; stage only your changes. Never commit build state.
  Use messages such as `match: reconstruct CThing::Method` or `tools: fix labels`.
