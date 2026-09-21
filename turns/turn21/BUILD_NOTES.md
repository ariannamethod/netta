# Turn 21 build notes

Every ambiguity in `PROTOCOL.md` resolved minimally, every departure from a
literal reading, and every place where the implementation had to choose. The
protocol itself was not edited. Builder hand: Opus. Acceptance, pristine reruns
and integrity probes are Don's.

Workspace: `/Users/ataeff/arianna/netta-don-turn18-20260921/`, an isolated clone
of netta on branch `main` at `df083b5425137431f02941000256ef96348d1424`. All work
lives in `turns/turn21/`; `git status --porcelain` on the clone reports exactly
one line, `?? turns/turn21/`. No git operation of any kind was run. No network.
Canonical netta, `~/arianna-codex/`, `~/arianna-shared/` and every sealed world
range were not touched.

## 1. The 27 modes, and which of turn20's modes survive

The protocol declares 24 grid modes plus "slow (2^-16), fast (2^-10), witness
(uncapped turn18 law)" — 27 authority trajectories. Turn20's `AR_ADAPTIVE` is
therefore **not** carried into turn21: it is neither a grid point nor one of the
three named references, and keeping it would make 28. Its code is deleted from
`authority.c` rather than left dead, because leaving it would put an unmeasured
mode in the shared `ARState` table and in every emitted row.

`ARState` stays 32 bytes (`authority_fixture` asserts `sizeof(ARState) == 32`
and prints it; 27 modes = 864 bytes of total prediction state). The `(high, low)`
pair is therefore **not** a struct field — it is a static const table
`ar_grid[24]` indexed by `mode - AR_GRID`, which is what "a mode table of
(high, low) pairs" asks for and what keeps the record size at 32.

## 2. Grid order and naming

`high in {5,6,8,10,12,16} x low in {2,3,4,5}` with `low <= high` excludes nothing
here — every low is <= every high — so the grid is the full 6x4 = 24. Order is
high-major, low-minor; mode names are `h<high>l<low>`, e.g. `h5l2 .. h16l5`. The
fixture rebuilds the table independently from two literal arrays and asserts it
equals `ar_grid` element by element, so a mistyped constant is a build failure,
not a silent relabelling.

## 3. (5,5) and (10,5) are grid points, proved rather than asserted

The protocol says both incumbents "are not reimplemented separately". They are
grid points by construction, but construction is a claim, so it was measured:
turn20's own `replay.c`/`authority.c` were compiled into the scratchpad and both
binaries were run on one identical 4000-event synthetic tape.

    96,000 fields compared, 0 mismatches

`h5l5` is bitwise turn19's fixed ceiling (turn20's `AR_CEILING`), `h10l5` is
bitwise turn20's witnessed ceiling, and turn21's `slow`, `fast` and `witness`
columns are bitwise turn20's. Compared fields: the seven head columns, all three
reference modes' `live`/`odds_before`/`odds_after`, `witness_slow_used`,
`witness_w_after`, and each incumbent's `live`/`odds_after`/`clipped`.

## 4. Where the cap sits relative to the clock update

Turn20 had two different orderings: `AR_CEILING` capped at 5 **before** the
witness evidence update, `AR_WITNESSED_CEILING` capped **after** it. Turn21's
family is "exactly turn20's law with (high, low) as declared constants", i.e. the
witnessed ordering: hazard by the OLD evidence, one-way odds update, evidence
update, then cap the next odds at `low` if the UPDATED evidence <= -1 else at
`high`. Every grid point uses that single ordering.

For `h5l5` this is provably the same trajectory as turn19's pre-update cap: the
evidence update reads `state->evidence` and `delta` only, never `odds`, so moving
the cap across it cannot change either quantity, and the cap level for (5,5) is 5
whichever side of the update it is read on. The bitwise probe in note 3 is the
measurement of that argument.

## 5. The exact diff against turn20/authority.c

Two hunks, 58 diff lines. Verbatim:

```
@@ -3,6 +3,17 @@
 #include <math.h>
 #include <string.h>

+/* The preregistered grid: high in {5,6,8,10,12,16}, low in {2,3,4,5}, low <= high.
+   Declared here as constants, exactly as turn20 declared its (10, 5). */
+const ARCeiling ar_grid[AR_GRID_POINTS] = {
+    { 5, 2}, { 5, 3}, { 5, 4}, { 5, 5},
+    { 6, 2}, { 6, 3}, { 6, 4}, { 6, 5},
+    { 8, 2}, { 8, 3}, { 8, 4}, { 8, 5},
+    {10, 2}, {10, 3}, {10, 4}, {10, 5},
+    {12, 2}, {12, 3}, {12, 4}, {12, 5},
+    {16, 2}, {16, 3}, {16, 4}, {16, 5},
+};
+
 static int valid_mode(ARMode mode) {
     return mode >= AR_SLOW && mode < AR_MODES;
 }
@@ -56,28 +67,22 @@
         }
     } else {
         slow = state->mode == AR_SLOW ||
-               (state->mode == AR_ADAPTIVE && state->evidence >= 1.0) ||
-               ((state->mode == AR_WITNESS || state->mode == AR_CEILING ||
-                 state->mode == AR_WITNESSED_CEILING) &&
-                state->evidence > -1.0);
+               (state->mode != AR_FAST && state->evidence > -1.0);
         double hazard = slow ? 0x1p-16 : 0x1p-10;
         double posterior = state->odds + delta;
         if (!isfinite(posterior)) return -1;
         next.odds = log1p(-hazard) / log(2.0) -
                     logadd(-posterior, log2(hazard));
-        /* Charge used the old quote. Only this state's own next odds are capped. */
-        if (state->mode == AR_CEILING && next.odds > 5.0) {
-            next.odds = 5.0;
-            cap = 1;
-        }
-        if (state->mode == AR_ADAPTIVE)
-            next.evidence = (255.0 / 256.0) * state->evidence + delta;
         /* Silence is not evidence: a byte no stored record voted on leaves w. */
-        if ((state->mode == AR_WITNESS || state->mode == AR_CEILING ||
-             state->mode == AR_WITNESSED_CEILING) && matched >= 1)
+        if (state->mode >= AR_WITNESS && matched >= 1)
             next.evidence = (31.0 / 32.0) * state->evidence + delta;
-        if (state->mode == AR_WITNESSED_CEILING) {
-            double limit = next.evidence <= -1.0 ? 5.0 : 10.0;
+        /* Charge used the old quote. Only this state's own next odds are capped,
+           at this grid point's low level when the updated witness evidence says
+           the memory is currently wrong, otherwise at its high level. */
+        if (state->mode >= AR_GRID) {
+            const ARCeiling *level = &ar_grid[state->mode - AR_GRID];
+            double limit = next.evidence <= -1.0 ? (double)level->low
+                                                 : (double)level->high;
             if (next.odds > limit) {
                 next.odds = limit;
                 cap = 1;
```

The witness evidence law is byte-identical: updated only when `matched >= 1`,
decay 31/32, threshold -1, the crossing observation excluded (it sets
`evidence = 0` at admission and `slow_used = -1`). `ar_quote`, `logadd`,
`valid_prices`, the admission shadow and the 32-byte record are untouched.

## 6. Column layout: what the C emits, and the one column dropped

91 columns. Seven head columns; `live`/`odds_before`/`odds_after` for each of the
three references; `witness_slow_used`, `witness_w_before`, `witness_w_after`
once; then `live`/`odds_after`/`clipped` for each of the 24 grid points.

Two receipts are quoted **once** rather than 24 times, and each is paid for with
a counted check rather than an assumption:

- **The witness clock and the hazard choice.** Every grid point carries the
  witness clock exactly (the cap never feeds back into `evidence`). The C checks
  `evidence` before, `evidence` after and `slow_used` against `AR_WITNESS` for
  every grid point on every byte and counts the checks:
  `clock_identity_checks` on stderr must equal `N*24`, and `experiment.py`
  asserts that number. `verify.py` does not take this from C at all: it rebuilds
  each of the 24 clocks independently in its own Decimal replay and refuses with
  `grid/witness clock identity <mode>` on any difference, counting `N*24` of its
  own checks.
- **`odds_before` for grid points.** It is by construction the previous row's
  `odds_after` (the state is carried; both are 0 before admission). The C holds a
  `previous[]` array and fails with `carried odds continuity` on any difference,
  counting `carry_checks == N*27`; the writer recomputes `odds_before` from the
  carry and additionally checks the three reference modes' emitted
  `odds_before` against its own carried value (`exactness.carry_exact`, a W4
  input); the reader checks its own reconstructed `odds_before == previous[mode]`
  for all 27.

This was a size decision: with `odds_before` repeated per grid point the retained
authority traces would have been ~40% larger. It removes no independent
information — the grid `odds_after` column at t-1 *is* the `odds_before` receipt
at t — and the identity is now a counted number instead of an unstated one.

## 7. Six W1 conditions, read literally

W1's six bullets, as implemented per grid point, means over the eight worlds:

| protocol text | implemented as |
| --- | --- |
| law-tail >= fast - 1 bit | `mean(tail_switched[p] - tail_switched[fast]) >= -1` |
| law whole-life >= fast | `mean(gain_switched[p] - gain_switched[fast]) >= 0` |
| moved-tail >= fast + 1 bit AND wins >= 5/8 | `mean(gap) >= 1` and `count(gap > 0) >= 5` |
| moved-tail >= slow - 3 bits | `mean(tail_moved[p] - tail_moved[slow]) >= -3` |
| recombined early-4096 and full-life retain >= 95% of slow's positive means | `mean(early[p])/mean(early[slow]) >= .95` and the same for `gain`, both requiring the slow mean positive |
| with full-life positive 8/8 | `all(gain_recombined[p][w] > 0)` |

**Departure, recorded:** the protocol also says "Bars are turn20's". Turn20's
moved-tail mean tooth was a strict `avg(moved_fast) > 1`; the protocol's own W1
text says `>= fast + 1 bit`. They disagree only on an exact tie, which no mean in
this batch is anywhere near (the closest is `h16l2` at +0.9874, and it is below
the bar under either reading). The protocol's own wording was followed: `>=`.
The five other bars are identical under both readings (`>= -1`, `>= -3`, `>= .95`,
`> 0`, `>= 5`).

"law whole-life >= fast" has no turn20 counterpart (turn20 reported whole
switched lives in prose but did not gate on them); `>= fast` is read as a mean
difference `>= 0`.

## 8. W2: which points may be nominated, and the tie-break the protocol lacks

W2 says the nominated point "**additionally** satisfies its law-tail comparison
in at least 5/8 individual worlds". *Additionally* to W1's conditions — so the
nomination is over the **W1-passing set**, not over all 24. If the window is
empty there is no nominated point and W2 is false. Consequence, declared rather
than hidden: emptying the window necessarily takes W2 with it, and the pre-freeze
probe for W1 expects both teeth to flip.

Because that reading makes the frontier's overall peak invisible when W1 fails,
`quantities.best_law_tail_point_overall` reports the argmax over all 24 points
under the same key. It is a raw quantity and no gate tooth reads it.

The protocol's lexicographic rule names two keys — greatest law-tail-vs-fast
mean, then greater law whole-life mean. **In this batch those two keys do not
separate the field:** `h8l4` and `h8l5` are exactly equal on both
(`-0.9155676529` and `+5.6806777611`, to every printed digit). A third
deterministic key was therefore necessary: the earliest grid index, which picks
`h8l4`. This is an addition to the protocol's rule and is flagged as such. It
changes nothing material — `h8l4` and `h8l5` have identical per-world law-tail
values, so W2 is false under either choice, and the reader applies the same
third key and agrees on the name.

"Its law-tail comparison" is W1's first bullet applied per world:
`count(w : tail_switched[p][w] - tail_switched[fast][w] >= -1) >= 5`.

## 9. W3: raw tail means, and why that is the same comparison

"law-tail mean at high=5 >= law-tail mean at high=16" is implemented on the raw
mean switched tail, not on the vs-fast gap. The two are the same comparison: both
points are read on the same eight worlds against the same `fast` reference, so
subtracting it is a common constant that cancels. The vs-fast gaps are reported
per point regardless. Same for the moved-tail tooth.

## 10. W4: bars and one superset

- "drawdown at most 16+1e-7 in every mode" is applied as stated, to all 27.
  Separately, `replay.c` fails fast on a per-mode bound `log2(2^high + 1)` for
  grid points (10 for fast, 16 for slow/witness), which is the mass-space bound
  the cap actually implies; it is looser than the gate bar for `high=16`
  (16.000022 vs 16.0) and tighter for every other high, so it can only catch
  violations earlier, never excuse one. The gate bar is the protocol's.
- "complete-prefix gain above -1-1e-7" is `minimum >= -1-TOL`, matching
  turn13/experiment.py:432 and turn13/verify.py:494, which use `>=` for the same
  bound. Same resolution as turn15 note 7.
- "protected NEW and equal-price quotes bitwise equal to cold in every mode" is
  checked with hard `!=` on the parsed column, no tolerance, in both hands. The
  implementation also checks **inactive** quotes equal cold, and each grid
  point's `cap_bound_exact` — a superset of what W4 names, so it can only be
  stricter. Both extras were red-probed independently.
- A violation of any of these is recorded in `exactness`/`cap_bound_exact` and
  carried to the gate, never raised: a gate condition must be able to come out
  false and be preserved.

## 11. gate_pass, and where W5 lives

`RESULT.json.gate_pass` is the conjunction of the ten named `w1_*`/`w2_*`/`w3_*`/
`w4_*` booleans — everything the writer can establish about itself. W5 is the
reader's own word and cannot be in the writer's file; `VERIFY.json` carries
`complete_gate_pass = all(tests) and all(reader_tests)`, which is the protocol's
`gate_pass = W1 AND W2 AND W3 AND W4 AND W5`. `RESULT.json` keeps turn14's
`independent_reader_pending: true`. Ten tests, `assert len(tests) == 10` in both
hands.

One honesty note: `reader_tests.w5_retained_digests_intact` is a constant `True`
in the emitted JSON, because `identity()` raises long before it is reached. Its
real content is the refusal, which was probed (note 13); the field records that
the refusal did not fire, not that a check was computed.

## 12. The writer's second hand, and the low==high labelling degeneracy

`replay.c` accumulates its own per-mode `Stats` from its own state, independently
of the columns it prints. `experiment.py` parses that stderr summary and asserts
agreement for all 27 modes: `gain`, `minimum`, `drawdown` within 1e-9, and
`admission`, `slow`, `fast`, `clipped` exactly. The set of C admission bytes
across 27 modes is required to be a singleton (`exactness.c_admission_shared`,
a W4 input) — so the admission-sharing tooth is measured from a channel the
column parser does not touch, rather than being true by construction.

**Degeneracy, recorded:** clips are labelled low or high by `limit == low`. When
`low == high` the two levels are the same number and every clip is labelled
"low". The only such point is `h5l5`, which reports 63,417 low clips and 0 high
clips where `h5l2`/`h5l3`/`h5l4` report 0 and 63,417 for the identical
trajectory. No gate tooth reads the low/high split (turn20's
`mechanism_both_levels_clip` is not a turn21 condition), so this affects no
verdict; it is a reporting artifact of a degenerate point and is stated here so
the clip table is not misread.

## 13. Pre-freeze probes

Run before `freeze` and before any byte of worlds 216..223 existed, in the
scratchpad, not in `turns/turn21/` and not deliverables.

**The C layer.** `make all fixture` rc 0 under `-std=c11 -Wall -Wextra
-Wpedantic -Werror`, zero diagnostics. `authority_fixture` rc 0 on a disjoint
33-event synthetic journey: `"pass":true`, 27 modes, 24 grid points,
`sizeof_ARState` 32, binary-complement normalization error 1.554e-15, 95 low
clips and 115 high clips, all 24 points clipping at their low level and 19 at
their high level, both hazards exercised including silent bytes. Per event and
per grid point it asserts `clipped == (unbounded_odds > limit)` and
`|odds_after - min(unbounded_odds, limit)| < 1e-10` in long-double mass space,
with `limit` recomputed from the updated clock.

**Equivalence.** Note 3: 96,000 fields, 0 mismatches against turn20's binary.

**Counters.** On a 4000-event tape: `carry_checks=108000` (= 4000x27),
`clock_identity_checks=96000` (= 4000x24), `odds_bound_checks=96000`.

**Whole pipeline.** A scratch repo with symlinked inherited directories and
`WORLDS=(900,)` ran `make all fixture` -> `freeze` -> `generate` -> `extract` ->
`learn` -> `evaluate` -> `render_tables` -> `verify.py`, each rc 0. It measured
the footprint (1.5 MB data, 124 KB memory, 25 MB results per world) and the
reader's cost (34 s per world). `render_tables` produced 4x27 = 108 life rows and
2x3x2 = 12 byte examples, the shapes that scale to 864 and 96.

This probe also found a real ordering dependency: `FREEZE.json` pins
`turns/turn21/authority_fixture`, so the build must be `make all fixture`; plain
`make all` leaves `freeze` to die on a missing file. The frozen run used
`make all fixture`.

**W5 (a), the reader accepting the writer's actual schema.** The scratch
`verify.py` run above is that probe: rc 0 against the `RESULT.json` the scratch
`experiment.py` had just written, max numeric error 1.151e-10, all five
`reader_tests` true.

**W5 (b), the reader refusing a RESULT with a required key removed.** Each of the
19 keys in `REQUIRED_RESULT_KEYS` was deleted in turn and `verify.py` re-run:
**19/19 refused by name** (`RESULT.json schema is missing required key: <key>`),
rc 1, each in ~0.06 s. The schema check runs before `identity()` and before any
reconstruction precisely so this refusal is immediate rather than an hour in.
The writer's half is `assert set(result) == set(RESULT_KEYS)` before `save()`;
the two lists are written independently in the two files, so a drift between
them turns the probe red.

**Every gate tooth goes red.** A synthetic harness on 8 fake worlds built a
baseline where all ten teeth are green, then applied 16 mutations, running
**both** `experiment.summarize` and `verify.independent_gate` on each and
requiring the two hands to agree on all ten booleans every time:

| mutation | tooth that went red |
| --- | --- |
| every point's law tail 2 bits under fast | `w1_window_exists` (+ `w2`, note 8) |
| mean above the bar, 4/8 worlds below it | `w2_nominated_point_robust` |
| law-tail slope inverted across `high` | `w3_law_tail_favours_tight_high` |
| moved-tail slope inverted across `high` | `w3_moved_tail_favours_loose_high` |
| one life's `first_admission_shared` false | `w4_shared_first_admission` |
| one life's `carry_exact` false | `w4_shared_first_admission` |
| one life's `c_admission_shared` false | `w4_shared_first_admission` |
| one unrelated life admits | `w4_unrelated_never_admits` |
| one `episodes.bin` at 529 bytes | `w4_archive_cap` |
| one mode's minimum at -1-2e-7 | `w4_prefix_and_drawdown` |
| one mode's drawdown at 16+2e-7 | `w4_prefix_and_drawdown` |
| one life's norm error 1e-7 | `w4_distributions_normalized` |
| one life's `column_new_exact` false | `w4_distributions_normalized` |
| one life's `equal_price_exact` false | `w4_exact_protected_quotes` |
| one point's `cap_bound_exact` false | `w4_exact_protected_quotes` |
| one life's `new_exact` false | `w4_exact_protected_quotes` |

Each mutation flipped exactly its declared set and nothing else. Four green
counter-probes confirm the teeth are not always-false: minimum at `-1+1e-9`,
drawdown at `16-1e-9`, normalization exactly `1e-8`, and a law tail sitting
exactly on the `-1` bar each flip **zero** of the ten — which is also the direct
check that these bars are `>=` and not `>`.

**Both refusal channels of the reader.** (i) A frozen file edited after the
freeze: refused in 0.1 s with `frozen file turns/turn21/render_tables.py` — this
one fired for real during probing, when a scratch edit was made to a pinned file.
(ii) A retained artifact perturbed: refused with
`retained artifact results/authority/world900/switched.tsv.gz`. (iii) The one
that matters for W5 — the same trace perturbed by 1e-5 bit in a single
`h10l5_live` field **with `RESULTS_MANIFEST.json` updated to match**, so the
digest tooth cannot fire: the reader still refused, naming the column and the
error, `authority h10l5_live: -4.1307779839821634 != -4.130787983982163,
error=9.999999999621423e-06`. The reader compares values, not only hashes. All
perturbed files were restored and re-checked byte for byte.

## 14. Departures from turn20's implementation, complete list

1. `AR_ADAPTIVE` removed (note 1).
2. Per-grid `odds_before` not emitted; replaced by counted carry checks (note 6).
3. `gzip` compresslevel 6 for the authority traces; turn20 used the library
   default 9. This changes only the byte size of retained traces, which the
   manifests pin as written. The source traces are written by turn13's frozen
   code and are untouched.
4. `replay.c` emits three check counters on stderr (note 6) and the writer
   asserts them.
5. The writer cross-checks the C's own per-mode statistics (note 12); turn20 did
   not.
6. Raw samples are collected for three comparisons rather than two
   (note 15).

## 15. Which comparisons the retained bytes record

Turn20 kept per-life extrema for `witnessed_ceiling` against the fixed ceiling
and against uncapped witness. Turn21 keeps three, all of them named by the
protocol's own structure rather than chosen after seeing data:

- `witnessed_vs_fixed`: `h10l5` minus `h5l5` — turn20's incumbent against
  turn19's, continuity with both preserved batches.
- `witnessed_vs_witness`: `h10l5` minus `witness` — turn20's second comparison.
- `tight_vs_loose`: `h5l5` minus `h16l5` — the W3 tooth pair at the incumbent
  low, which is where the protocol asks whether the surface bends.

Help and harm extrema, both tail regimes, all eight worlds: 3 x 2 x 2 x 8 = 96
rows in `BYTE_EXAMPLES.tsv`. `RAW.md` quotes the `tight_vs_loose` extrema, which
are the protocol's "a tight cap saves bits" and "a tight cap costs bits".

## 16. Order of operations, and what was irreversible

`make all fixture` (rc 0) -> probes (all rc 0) -> `freeze` (rc 0) -> `generate`
(rc 0) -> `extract` (rc 0) -> `learn` (rc 0) -> `evaluate` (rc 0) ->
`render_tables.py` (rc 0) -> `verify.py`. `freeze` ran with no `data/`, `memory/`
or `results/` present, verified by `ls` in the same command. Once `generate`
completed nothing was regenerated, retried or tuned: one run of each stage, one
gate, on worlds 216..223. `FREEZE.json`, the three manifests, `RESULT.json` and
`VERIFY.json` are all written with `open('x')`; `evaluate` additionally refuses
to start if `results/` exists, and `verify.py` refuses if its output exists.

No edit was made to any file under `turns/turn21/` after `freeze` ran — `verify.py`
re-hashes all 30 pinned files on every run and would refuse.

## 17. The result, and what it is not

Material gate **FAIL 8/10**, preserved with the diagnosis in `REPORT_NOTES` below
and in `RAW.md`. Nothing was retried on these worlds; no world and no grid point
is excluded from any table.

**W1 passed.** The window is not empty: it is exactly the four `high=8` points.
That is the one outcome the protocol did not pre-name — it registered "a window
exists" and "the window is empty" as the two results of equal rank, and the
measurement returned a window one column wide. `high=5` and `high=6` fail on the
moved surface (moved-tail -5.92 and +1.57 against a +1 bar with 2/8 and 4/8
wins); `high >= 10` fails on changed law (law-tail -2.38 or worse against a -1
bar). Only `high=8` clears both sides at once, and it clears the law-tail bar by
0.084 bit.

**W2 failed, and that is the sharper finding.** The nominated point `h8l4` holds
its law-tail advantage in the *mean* but in only **3 of 8 worlds** (bar: 5). Two
worlds carry it (`218: +2.248`, `221: +0.108`, `217: -0.125`) while five sit
between -1.5 and -2.6. A window that exists on the mean and not on a majority of
worlds is exactly what W2 was preregistered to detect.

**W4 failed on a draw, not on the mechanism.** `w4_unrelated_never_admits` is
false because world 218's unrelated life genuinely admits: its episode shadow
reaches 31.8716 bits by t=1528 and crosses at t=1530 (31.8716 + 0.5879 =
32.4595 >= 32), activation byte 1531, peak shadow 53.8380 bits. Every other
world's unrelated life peaks at 5.3221 bits or less. The admission is inherited
from turn13's frozen candidate arm — it is visible in
`results/world218/unrelated.tsv.gz` as `episode_activated_after=1`, before any
turn21 code runs — and all 27 modes share it identically, which is why
`w4_shared_first_admission` passes on the same life. This is a property of this
namespace's draw for world 218, not of the ceiling family. It is reported, not
excluded.

**W3 passed on both teeth, 4/4 rows each**, against a 3/4 bar. The surface bends
the way the mechanism claims, with a monotone seam level: mean capped odds at
t=8192 on switched lives runs 3.4308 (high=5) -> 4.4282 -> 6.3001/6.4111 ->
8.0030..8.3349 -> 9.5888..9.9532 -> 11.1512..11.5156 (high=16), against 11.6725
for uncapped witness. The cap is what holds the odds down at the seam, and the
law-tail ordering is the inverse of that level.

`high=16` never clips at its high level in any related life (0 clips at all four
lows), which is the mechanism confirming the bound: with hazard 2^-16 the
achievable odds are at most `log2(65535) = 15.99998 < 16`, so a 16-bit ceiling
cannot bind and only the low level ever fires there.

## 18. The frozen reader refused, and why — verify_repair.py

The frozen `verify.py` reconstructed all 32 lives, agreed on every number, every
life statistic, every table, the window membership and all ten gate booleans, and
then refused:

    AssertionError: nominated point

preserved in `VERIFY.log`. It is a defect in the nomination rule I wrote, not in
the data, and not in either reconstruction. Diagnosis, from this batch's own
retained numbers:

- On `switched`, `h8l3`, `h8l4` and `h8l5` have **bitwise identical** per-world
  tails, so the writer's law-tail-vs-fast means are bitwise equal:
  `-0.9155676528937813` for all three. (They differ over the 8192-byte prefix,
  where `h8l3` takes 49 low clips; the trajectories re-converge because repeated
  capping at `high=8` drives all three to exactly 8.0 and they stay equal.)
- The two hands compute that tail by different formulas. The writer accumulates
  `live - cold` directly over `t >= MOVE`. The reader, inheriting turn20's
  `Authority.result()`, computes `gain - horizons['8192']` — a difference of two
  numbers near 2000 bits. Both are correct; they agree to about 1e-13 but not
  bitwise, and the subtraction does not preserve an exact tie.
- Measured on the writer's own retained per-mode `horizons`, recomputing the
  subtraction form gives `h8l3 -0.9155676528943246`, `h8l4/h8l5
  -0.9155676528943673`: a spread of **4.263e-14 bit**, whose argmax on key 1 is
  `h8l3`. That is the reader's answer, and the whole disagreement.
- My rule tested ties with exact float equality, so the protocol's third-key
  fallback only fired in the hand whose arithmetic happened to preserve the tie.

`verify_repair.py` is the disclosed reader-only correction, following turn20's
precedent exactly: the frozen `verify.py` was **not** edited (its sha256 is still
`fa8ede135064e345169553c9b1714139b3b26e9208a90629de2ad7592a3cff07`, the value
pinned in `FREEZE.json`, and the repair re-hashes it along with the other 29
pinned files on every run). The repair adds one helper, `lexicographic_best`, and
uses it at the rule's two call sites. Nothing else differs — the diff is the
docstring note, the helper, and the two call sites.

The repaired rule treats two points as tied when their means agree within the
gate's own `TOL = 1e-7`, then lets the earlier grid index keep the nomination.
Probed on five inputs before the run: the writer's exact-tie values give `h8l4`;
the reader's 4.263e-14 spread gives `h8l4`; a 1e-6 separation on key 1 wins; a
1e-8 separation does not; a 3.3-bit separation on key 2 wins. Hand-independent.

**This changed no verdict.** `h8l3`, `h8l4` and `h8l5` all clear the per-world
law-tail comparison in 3 of 8 worlds, so `w2_nominated_point_robust` is false
whichever of them is nominated, and no gate boolean reads the nominated point's
identity. `quantities.best_law_tail_point_overall` is `h5l2` under both rules and
both formulas — the `h5` row's four points have literally identical trajectories,
so no prefix divergence exists there to perturb the tie.

Honest statement of what the writer got right: `RESULT.json`'s
`nominated_point: h8l4` **is** the correct answer under the repaired rule, but
the writer reached it with the same unstable exact-equality test, and only
because its tail formula preserves the tie bitwise. The writer's rule is not more
correct than the frozen reader's; both needed a tolerance. The writer's file is
frozen evidence and was not edited, and the repaired reader independently
recomputes and confirms its value.

## 19. The verification run

`verify_repair.py --output VERIFY.json`, one run to completion:

| quantity | value |
| --- | --- |
| verification_pass | true |
| gate_pass (W1..W4, recomputed) | **false** |
| complete_gate_pass (W1..W5) | **false** |
| candidate forecasts rebuilt | 3,670,016 (= 8 x 4 x 16384 x 7) |
| authority forecasts rebuilt | 14,155,776 (= 8 x 4 x 16384 x 27) |
| maximum numeric error | 2.5136159820249304e-10 (bar 1e-7) |
| maximum normalization error | 1.354472090042691e-14 (bar 1e-8) |
| reader sha256 | 38ff350f9a78fe6285ea6a7cd3dbe54baef4b010bb5ab00994b5ce437b9235cb |
| VERIFY.json sha256 | 5cdf4b031658744bfc6139f66b4e075bfdd52e88a14cf96b297011bd6d84c4a3 |
| RESULT.json sha256 | 70ea594706515ea2942291d6d29beb1b36346cbecb147fc56d69aa4d763cae4b |
| FREEZE.json sha256 | e59c454e2352ff596aa621c98742911b92a78599423186d581bdeeb04a52891f |

All five `reader_tests` true. The reader's ten gate booleans are equal to the
writer's element by element, its `window_points` and `nominated_point` equal the
writer's, and `gate_pass` agrees. It re-hashed all 30 `FREEZE.json` entries and
all 312 data, 64 memory and 128 result manifest entries.

## 20. Irreversibility, checked rather than asserted

Re-running each irreversible stage after the fact, rc direct:

| command | rc | refusal |
| --- | --- | --- |
| `experiment.py freeze` | 1 | `AssertionError` (data/memory/results exist) |
| `experiment.py evaluate` | 1 | `AssertionError: results exist; preserve evidence` |
| `verify_repair.py --output VERIFY.json` | 1 | `AssertionError: verification output already exists` |

`RESULT.json` and `VERIFY.json` hashed identically before and after these three
attempts. Nothing was retried, regenerated or tuned on worlds 216..223.

## 21. Not this turn's business

No commit, push, merge, branch or live integration. `RESULT.json` carries
`independent_reader_pending: true`, turn14's convention: the writer's file
records that it was written before the reader ran, and `VERIFY.json` is the
reader's own word. Canonical netta, the living organism, mouth, mycelium and
every sealed world range were not touched. `git status --porcelain` reports
`?? turns/turn21/` and nothing else.
