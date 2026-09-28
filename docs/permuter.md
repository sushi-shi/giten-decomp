# Permutation experiments

Use the `permute` skill after reconstructing a complete, credible body.
Direct `state`/`variants` commands require a diagnosed register/schedule residue
and HIST < 100. Candidate/campaign commands classify the live population and
route class-appropriate source experiments.

```sh
giten permute candidates --output /tmp/candidates.json
giten permute campaign --targets 3 --islands 32 --frontier 4 --output /tmp/campaign
giten permute state --source <source> --rva <rva> --trials 60 --jobs 4 \
  --state-summary /tmp/states.json
giten permute variants <source> <rva> --axes-from /tmp/axes.json \
  --min-depth 0 --max-depth 2 --state-trials 32 --jobs 4 \
  --wall-time-seconds 900 -o /tmp/variants.json --run
```

`state` leaves the body unchanged and inserts disposable declarations.
`--only-trial N` replays a state; `--retain-best` keeps a useful non-exact result
or failure diagnostics. `variants` crosses reviewed exact-span axes, bounded
clang AST edits and requested TU states. Use depth 0 for axes/state only;
atomic `extra_edits` keep helper definitions and call-site changes together.

Trials compile complete disposable TUs with the real headers and sibling
functions. Objects use the normal build's canonicalization and per-candidate
alias proof. Source bytes are restored and checked; `state` also checks source
fingerprints. SDK targets use explicit `RVA_COMPGEN` claims and reject `include`
and `mixed` probes because the body lives outside the authored TU.

Only scored mutations count as executed states. All-failed runs are inconclusive;
a flat successful sweep bounds that experiment, not the compiler's possibilities.
Campaigns retain the best representatives of distinct states for inspection,
including after an exact result. Translate recurring evidence into a defensible
source change, compare with `giten match`, then re-derive the next population.

Exact closure requires unrounded 100%, the retail extent, complete relocation
decoding, and identical ordered relocation offsets/types/identities/addends.
Topology and fuzzy scores rank clues; neither replaces these checks.

`exact.cpp` is a source candidate for review. `exact-disposable.cpp` includes
probe state and must not be applied. After auditing the manifest, `state --record-max` can bank an unchanged source that became exact. Never retain fake
declarations or locals merely to select compiler output.
