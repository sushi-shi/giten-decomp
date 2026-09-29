# Permutation experiments

Use the `permute` skill after reconstructing a complete, credible body.
Direct `state`/`variants` commands require a diagnosed register/schedule residue
and HIST < 100. Candidate/campaign commands classify the live population and
route class-appropriate source experiments.

`random` reuses the pinned [decomp-permuter](https://github.com/simonlindholm/decomp-permuter)
parser and weighted mutation passes. It requires the same register/schedule
diagnosis and HIST < 100. The Nix shell provides `GITEN_DECOMP_PERMUTER`; an
explicit `--upstream <checkout>` supports development with another revision.

```sh
giten permute random src/Game/condition.c 0x3e6d0 \
  --rounds 64 --trials 256 --scored 10000 --chain-depth 8 --jobs 12 --frontier 4 \
  --output build/permute-campaign/condition-random
```

Each round generates seeded mutation chains, compiles complete disposable TUs,
and retains distinct compiled states. The next round carries those candidates
forward alongside the authored source. Source candidates are deduplicated across
rounds; retained parents are rechecked. `--seed` reproduces a campaign, and
`--weights <file>` accepts a TOML `[weights]` table overriding upstream pass names.
Generated inline helpers travel with their callers. Declaration context is
parser-only: original headers, macros, sibling bodies and compiler flags remain
in every actual compile.
The parser resolves `GZ_ENUM_STORAGE` annotations from their active definitions;
ordinary accessor macros remain calls in the mutation AST.
`--scored` counts distinct successfully scored source mutations, excluding the
authored baseline and repeated retained parents. The campaign stops when that
target is reached; exhausting `--rounds` first returns nonzero. Distinct machine
code states are counted separately across all rounds.

`random --resume` continues completed rounds in the same output directory,
restoring retained parents, source deduplication and counters. It rejects changed
source or search inputs. `--stop-on-exact` stops after a round produces an audited
exact candidate. An interrupted unfinished round is replayed with its original
seed; its previous artifacts are preserved separately.

`giten permute mine --output build/permute-campaign/mine-99` runs a resumable
queue in a clean isolated worktree. It builds fresh comparison objects and
records every function with 99 <= MAX < 100, in descending MAX order. Historical
exact matches and structural divergences are routed to review in `status.json`;
register/schedule residues run upstream C campaigns or clang C++ variants.
Defaults are three passes, 10,000 unique scored mutations per C pass, 96 rounds
of 256 mutations, 16 compile workers and a four-state frontier. C++ matrices are
bounded by the candidate limit and report their actual compiled counts. Each pass
uses a distinct seed. Exact candidates stop further passes and await semantic
review. The harness never edits authored functions or banks results.

Run the same command and arguments to resume. `--status` reads the checkpoint;
`--plan-only` builds and classifies without searching. Create `STOP` in the output
directory to stop cleanly, then remove it before resuming. SIGTERM also stops the
active child through its source-restoration handler. A killed C++ matrix is
replayed; C campaigns resume at completed round boundaries. Separate logs,
manifests, sources, assemblies and exact-audit results remain under the output.

This adapter supports C; use the clang `variants` path for C++. It reuses
upstream's mutation engine, not its architecture-specific assembly scorers or
distributed service. Manual alternatives use the existing exact-span axes;
upstream `PERM_*` templates are not accepted by `random`. External/global type
mutation and guard/loop deletion are disabled. Other upstream mutations can
change semantics or introduce artificial constructs: all outputs are diagnostic
candidates requiring review, never automatic source edits or MAX banking.
The campaign JSON separates generation failures, compilation failures, scored
states, elapsed time and retained sources. Only the existing full relocation
audit can certify an exact candidate.
Exactness and size ranking compare objdiff instruction extents on both sides;
the separately recorded CodeView window can include trailing alignment bytes.

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
Parallel variant batches score in manifest order while later TUs compile.
On exit, queued compiles are cancelled and running compiles finish removing
their disposable sources before scratch storage is removed. Alias checks reuse
parsed baseline objects only while their full contents remain identical;
candidate objects are always parsed and proved afresh.

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
