---
name: decomp-permuter
description: Use simonlindholm/decomp-permuter's C AST randomizer to search diagnosed Giten x86 register or scheduling residues, with real MSVC 5.0 COFF compilation and Giten objdiff scoring. Use for high-scoring, never-exact C functions after matcher and wall classification; use the repository permute skill for Giten's separate native campaign tool.
---

# Decomp-permuter for Giten

The [upstream decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
parses and mutates C, but its command-line scorer supports MIPS, PowerPC, and
ARM32 rather than this project's x86 COFF. Run its `src.randomizer.Randomizer`
through [scripts/run.py](scripts/run.py), which compiles each disposable complete
Giten translation unit with MSVC 5.0 and scores the target with Giten's objdiff.
Do not describe this adapter as an upstream x86 backend. Do not use the upstream
networked worker mode for proprietary target objects.

Pick a live queue from `giten permute candidates --output /tmp/candidates.json`
after `giten build`. Filter for `hist_max < 100`, `bank < 100`, high `cur`, a C
source, and `classification == "regalloc"`. Exclude known strict-data referent
regressions. Check `giten walls priors <rva>`, `diagnose <rva> --asm`, and
`semdiff <rva>` one function at a time. Equal call counts alone are not enough:
verify calls, referents, branches, and first divergence before randomizing.
Honor an existing `@early-stop` unless a new hypothesis explains its residue.

For each candidate, create a small temporary parser preamble containing the
typedefs, aggregates, globals, and prototypes needed to parse **that one C
function**. It is only for pycparser; the actual compile uses the entire authored
TU and real headers. Give `--signature` the unique function declaration prefix,
and `--end-marker` the next `RVA(` line or another unique following marker.
Use `--end-marker EOF` for the last function in a file.
If a function contains an unparseable project macro, `--parser-replace OLD=NEW`
may substitute its **proven actual preprocessing expansion** in the parser
surrogate. Keep the original macro in any final authored source.
Example:

```sh
export GITEN_DIR=$PWD
nix develop --command bash -lc '
  export GITEN_DIR=$PWD
  PYTHONPATH=scripts python3 .agents/skills/decomp-permuter/scripts/run.py \
    --upstream /tmp/giten-decomp-permuter \
    --source src/Game/itemrecord.c --rva 0x23e10 \
    --function TakeFromBagEntry --signature "i16 TakeFromBagEntry(" \
    --preamble /tmp/take-preamble.c \
    --output /tmp/take-permuter --trials 64'
```

The adapter stops if trial zero fails to reproduce the authoritative baseline.
This check matters when `data_matching = false`: both disposable and banked
objects must use Giten's relaxed-data relocation transform. A generated variant
is evidence, even if its fuzzy score rises; inspect its source, first differing
instructions, size, and ordered relocations. Translate a useful compiler-state
clue into a plausible original source spelling, then verify it with
`giten match <unit>`. Never copy random fake locals, redundant assignments,
casts, or changed behavior into authored source. Remove all probes and keep
campaign artifacts in `/tmp`. A claimed exact match needs 100% unrounded score,
equal extent, full decoding, and identical ordered relocation identities.

The native `giten permute` skill covers separate deterministic TU-state and
reviewed-axis campaigns; combine its evidence only after the same wall
classification. Matching work runs no test suites. Validate changes with
`giten match`, and run `giten build` if they span TUs. Gates belong to merge
preparation.
