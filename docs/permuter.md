# Classified permutation experiments

`giten permute` searches a complete, credible function only after
`giten walls diagnose` identifies a register-allocation/scheduling residue.
The public command also refuses a function whose historical MAX is already
100%. It is an evidence generator, not a substitute for reconstruction.

Two bounded modes are supported:

```sh
giten permute state --source src/Giten/GameLevel.cpp --rva 0x160450 \
  --trials 60 --jobs 4 --state-summary /tmp/probe-head-states.json

giten permute variants src/Giten/GameLevel.cpp 0x160450 \
  --axes-from /tmp/probe-head-axes.json --min-depth 0 --max-depth 2 \
  --state-trials 32 --state-insertion target --jobs 4 \
  --wall-time-seconds 900 -o /tmp/probe-head-manifest.json --run
```

The campaign front end derives and classifies the source-owned population, then
runs the approximation loop directly:

```sh
giten permute candidates --output /tmp/permute-candidates.json
giten permute campaign --targets 3 --islands 32 --frontier 4 \
  --output /tmp/permute-campaign
```

This is a bounded N-island/M-frontier search, not a claim that fuzzy score proves
source correctness. Each island is a deterministic compiler-state sample crossed
with class-appropriate source shapes. The batch retains the best representative
of the M highest-scoring distinct normalized target states. An agent compares
those states with retail, identifies a repeated source-level mechanism, makes one
defensible source A/B, checks it with `giten match <unit>`, and starts another
round. The live inventory is
re-derived for every round; there is no hand-kept queue.

`state` leaves the target body unchanged and inserts deterministic parser-visible
declaration forests either beside it or after the leading directive block.
`--only-trial N` replays an indexed state. `--state-summary` records unique
byte/extent/relocation states even when none is exact. `--retain-best` preserves
the best fuzzy or structural-topology clue and snippet, or failure diagnostics
when no trial produced a scored target; otherwise sub-100
artifacts are discarded. `--jobs N` compiles disposable sibling sources in
parallel without editing the authored TU.

`variants` forms one explicit product from:

- byte-exact, hand-reviewed axes supplied by `--axes-from`;
- conservative libclang edit trees up to `--max-depth N`;
- optional deterministic TU-state candidates from `--state-trials`.

These are a real Cartesian matrix: each selected source shape is tested in the
baseline compiler state and in every requested TU-state island. Exact-span axis
options may include atomic `extra_edits`, allowing a helper definition and its
call-site rewrite to remain one reviewed choice. The source-tree ceiling defaults
to three mutations; use `--min-depth 0 --max-depth 0` for axes/state only. There
is no regex rewriter or random hill-climber. Use one
manifest containing the complete legal candidate family per site so interactions
are measured rather than laddered.

The syntax-aware families cover value-neutral operand order, independent
assignment order, terminal return-pair inversion, declaration split/merge/hoist,
reviewed inline-helper extraction, cursor read/advance, and identity-safe local
renaming. The declaration forest varies typedef, record, packing, prototype,
calling-convention, and function shapes; C++ forests also include classes and
methods. The source suffix and explicit compiler language override select the
generator language. C sources reject the C++-only `class` and `member` families.
The forest is broader than a flat count sweep because VC5 front-end state is
sensitive to declaration kind and order.

Only successfully scored variants count as executed compiler states. Summaries
report attempted, compiled, failed, and valid mutation counts separately. A run
without any scored mutation is inconclusive and exits nonzero; a baseline alone
cannot establish a flat search. Retained sources preserve their language suffix.

Every compile has a timeout; a batch may also have a total wall-time bound.
Source restoration is guarded by a process lock and exact source bytes; `state`
also rechecks the per-function fingerprint. The first audited exact candidate
normally stops a direct search. Campaigns continue after exact so the M-solution
frontier remains available for pattern extraction.

`state` accepts a unique authored `RVA` anchor or an explicit `RVA_COMPGEN`
claim for an SDK function. The SDK body remains in its canonical header; its
header fingerprint and the claiming TU's unchanged suffix are checked alongside
the ordinary exact-byte and ordered-relocation audit. Ambiguous anchors are rejected.
SDK targets reject the `include` and `mixed` families because the authored TU does
not contain the header body's identifier census needed for the macro guard.

`variants --jobs N` compiles N disposable sibling copies of the complete TU in
parallel. It does not use a reduced function harness: the real include closure,
declaration population, inline candidates, and sibling functions remain present.
Each disposable object and the retail object are canonicalized through the same
same-function jump-table, compiler-private-symbol, and explicit folded-function
alias transforms as `giten build` before scoring; otherwise raw `$L...` labels or
constructor aliases can hide an exact result. Alias identities come from the
Model; every emitted alias body in each candidate is re-proved against retail
before its call targets are normalized. Parallel
results are still scored and audited one target at a time, and only a real-TU
result can bank MAX. A target-only harness may be useful as an explicitly
non-authoritative prefilter, but it is not the default because it can change C1
handle state; on `gamelevelmove` it was also no faster after warm-up because the
shared header parse dominated both 0.90-second compiles.
Exact closure requires all of:

1. unrounded objdiff score exactly 100%;
2. target extent equal to retail;
3. completely decoded ordered relocation streams;
4. equal relocation offsets, types, identities, and addends.

Instruction/branch/return topology is recorded as a separate ranking signal for
sub-100 candidates. It is only a clue: it can select a more structurally useful
candidate for inspection, but it cannot satisfy the exact-closure gate.

A source-only exact candidate is written as `exact.cpp` for review. A candidate
containing disposable TU state is written as `exact-disposable.cpp`; never apply
its probes. If unchanged source reaches exact, use `--record-max` with `state`
only after auditing the retained manifest; the source stays unchanged. Authentic
source changes still require retail evidence and ordinary focused commit
discipline; the gates run with `giten build verify` at merge preparation.
