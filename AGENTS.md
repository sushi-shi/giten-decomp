# Giten decompilation

Reconstruct the original C (and the C++ in the Windows platform layer) of **Giten Megami
Tensei** so MSVC 5.0 SP3 (`/Ox /Zp1 /ML`; `/Ox /Ob0 /Zp1 /GX` in the C++ layer) emits COFF matching retail `DDS.EXE`.
Correct structure (types, ownership, storage, control flow, calling
conventions, referents) outranks any score.

## Environment

- Work inside `nix develop`. Builds are per worktree: `export GITEN_DIR=$PWD`.
- Never run or launch the game. Never use the Ghidra decompiler on
  `DDS.EXE`; read assembly, xrefs, RTTI, vtables, data, and relocations.
- `CLAUDE.md` is a symlink to this file; skills live in `.agents/skills/`
  (`.claude/skills` links there). Edit the canonical file only.

## Objective and scores

- Every function has three scores in `config/match_baseline.tsv`, always
  `CUR <= MAX <= HIST`:
  * CUR (`cur_pct`): the score at the latest bank.
  * MAX (`best_pct`): the best score of the function's current source hash.
    It only rises while that source is unchanged; editing the function resets
    it to the new CUR.
  * HIST (`hist_pct`): the all-time peak across every source revision; it
    never resets.
- The goal is MAX = 100 for every function. `HIST > MAX` means an earlier
  source matched better: a lost match to recover from Git history.
- A CUR dip with MAX held (TU-wide codegen perturbation of an unchanged
  function) is not a regression. Overall fuzzy and exact counts are
  navigation only.
- Raw instructions, constants, and ordered relocations decide correctness.
  Objdiff scores function relocation targets strictly. While
  `config/compare.toml` sets `data_matching = false`, data targets and their
  addends are not compared: reach an unowned global through a plain `extern` in
  its owner header; it is listed as debt that must become a definition before
  the switch returns to `true` (`docs/build-system.md`, "Data matching").

## Workflow

1. Pick work from `giten walls inventory --todo --limit N` (ascending HIST:
   never-matched functions first) or from the `HIST > MAX` rows (lost
   matches). Check `giten walls priors <rva>` for an existing verdict.
2. Classify with `giten walls diagnose <rva> --asm`: referent, then
   inline/call-set, then CFG, then register/schedule. Fix the earliest class.
3. Reconstruct with the `matcher` skill; classify plateaus with
   `wall-identifier`; use `giten permute` (the `permute` skill) only for a
   diagnosed register/schedule residue with HIST < 100.
4. Iterate with `giten match <unit|source>`: it compiles, labels, delinks,
   and compares only that TU (a few seconds), even after a header edit other
   TUs include, and reports MAX changes only: an edited function against the
   MAX it replaces, an unchanged one only if it beats its MAX. CUR dips of
   unchanged functions are not reported and need no attention. An edit that
   keeps CUR but lowers MAX through the new source hash is a `reset`: the loop
   records it in `docs/todos/syntactic-recovery.tsv` for a later pass; do not
   chase it while matching. Run
   `giten build` (every TU, no gates) when the change spans units.
5. Gates run only when preparing a merge: `giten build verify` (MAX gate plus
   the fast and normal tiers).
6. Mark a complete body whose residue is bounded by evidence `@early-stop`.

Matching rules that are easy to get wrong:

- Levers are disposable A/B experiments. Never keep probes, unused
  declarations, fake locals, volatile carriers, or distorted source.
- A score dip is not a rejection: if a change moves codegen toward retail's
  shape, keep it and compose the next lever on top. The MAX gate governs only
  what is committed.
- An inline function or macro is a likelier original spelling than a
  hand-expanded body; prefer it as the base unless evidence overrules it.
- The PC-98 original (`DDS98.EXE`, 1997; `~/Projects/giten/investigation`) is
  a naming and structure witness: its `__FILE__` paths, strings, and paired
  functions name modules and inline boundaries. It is a different compiler's
  output; never copy its code shape as evidence for this build's codegen.
- If unchanged source reaches exact under a disposable TU-state experiment,
  bank it while exact, then remove the experiment.
- A 100% match is a match: keep it even if its source breaks a project rule.
  Admit a gate violation through that gate's allow entry, and record the
  function, the rule, and the deviation in `docs/todos/rule-exceptions.tsv`
  (schema in `docs/todos/README.md`).

## Tests

- Matching and modeling work runs no test suites; compare is the
  verification, and the MAX gate runs at merge preparation.
- Tooling changes run the relevant retained `test_*.py` modules
  (`python3 -m unittest giten.<pkg>.test_<name>` from `scripts/`), plus the
  affected command. There is no blanket test requirement for each gate.
- Keep tests for score integrity, CUR/MAX/HIST banking, data identity hidden
  by relaxed comparison, and safe file updates. Use compile/compare and the
  existing gates for reconstruction checks; do not duplicate them with retail
  function snapshots, source-spelling assertions, or CLI-output tests.

## Source rules

- One type, one definition, in a shared header. No TU-local struct or class
  copies, layout views, or placeholder shells.
- Each function and global lives in its evidence-backed owner TU/header. No
  scattered `extern`s; no macro aliases onto hex names.
- Access goes through typed members. No raw offset casts, offset macros,
  casts of `this`, or C-style casts; use named casts only at real boundaries.
- Unclear identity: chase callers, storage, callees, mangling, vptr stores,
  RTTI, and offsets; else leave `@identity-TODO`. Never fabricate.
- Names are semantic: no address-derived names, compiler ordinals, or
  `local_10`-style names.
- Platform headers come only from `<Win32.h>`. Never include `<windows.h>`
  directly or hand-roll SDK declarations; the CRT's own headers (`<stdio.h>`,
  ...) are included directly.
- Use typed enums for proven numeric domains; retyping a parameter changes
  mangling.
- Prove aggregates (`RECT`/`CRect`, `Coord`/`POINT`) from whole-object use; do
  not split one object into overlapping globals.
- Vtables come mechanically from `giten sema class <Class>`: inherited slots
  are not redeclared, overrides use `OVERRIDE`, new slots are `virtual`. Never
  pad with dummy virtuals.
- No fake code, storage, labels, aliases, or padding to improve a score or
  final layout.

## Labels and data

- Address labels are `include/rva.h` macros. Pin `$E` dynamic-init helpers at
  their owner with `RVA_DYNINIT`; never bind compiler ordinals.
- `DATA(...)` records identity, not linker placement. Write pooled strings and
  FP constants bare; `DATA_COMPGEN` only where the oracles cannot reach.
  Header COMMONs and per-TU header-static copies go in
  `config/retail/data_compgen.tsv` (see `docs/data-attribution.md`).
- Never model an interior address as separate storage; refine the owner.
- Aggregate objdiff data percentages do not prove `.data`/`.bss` correctness.
- Marker vocabulary: `docs/comment-markers.md`.

## Repository hygiene

- Tool inputs in `config/`, generated output in ignored `build/`, history in
  Git. Docs describe current usage and contracts only: no diaries, score
  snapshots, or campaign logs.
- `docs/patterns/` is a small mechanism reference; follow its README.
- Source comments are operational only: markers, ABI/codegen constraints,
  unsafe-seam explanations. No history, addresses, scores, or banners.
- `config/retail/reloc_sites.tsv` states relocations the image does not carry
  (`docs/relocations.md`): fix a false or missed site there, never around it.
- Keep every gate green. Preserve concurrent changes; stage only your unit
  of work. Commit messages like `match: reconstruct CThing::Method` or
  `tools: verify relocation targets`. Never commit build state.
