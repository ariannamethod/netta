# Two remembered cases, one partly familiar world

2026-09-16, Astra. Declared before new predictor code, data, or measurements.
One bounded next step after the merged HMM-16 audit. No live integration,
commit or push. Earlier inputs/results remain evidence of their own laws.

## Incoming audit

Merged main37e9b68c439a4891ca507fc4155c58bd94d64a2e contains Sol98d723b.
Fresh build and regeneration exactly reproduce HMM_FREEZE, all56 raw-stream
hashes and HMM_RESULT. Independent probability-space reconstruction checks
393216 predictions, admission timing, and all three bounds at EVERY prefix.
Maximum numeric disagreement8.81e-12 bits. No production defect found.
The old hmm16.py checks extra static cost at the final endpoint; the new
independent audit closes that coverage gap without rewriting its frozen code.

## One question and fixed mechanism: BANK2-ROW

Can distinct past cases help predict a new mosaic world earlier than either
one global case selector or pooling the same past observations?

Two immutable HEAD256 books A/B contain different conditional laws. All
frontend constants, P0/P2 formulas, source support32, local support1,
strength32, gate32 and outer HMM h=2^-16 remain the merged construction.
Each component P2_j is quoted with the SAME recipient local counts and
byte distribution. An unavailable component quotes P0 as the old API does.

The recipient stores one log-odds ell[p] per canonical HEAD256 row, initially0.
Before truth:

    S(x) = (P2_A(x) + 2^ell[p] * P2_B(x)) / (1 + 2^ell[p]).

After charging the quote, update ONLY the observed row:

    ell[p] += log2(P2_B(truth) / P2_A(truth)).

These inner weights learn from every observation, including before outer
admission; none cross a life boundary. Incomplete contexts return Pbase.
NEW has identical probability under P0 and both components, so its update
is exactly0. There is no new per-row gate or inner cold component.
The bank's S is the single candidate of the unchanged outer shadow/HMM.
After one prospective32-bit crossing, following-event odds start at1.
The outer HMM still bounds lifetime loss by1 bit and any interval loss by16.

Without an inner cold component a particular row can choose A or B, but
cannot separately reject both available books. The common outer cold can
reject the bank. This is a deliberate limitation of the single small step.

State:203 double row log-odds (1624 bytes) beyond the existing local counts
and HMM. Portable bank:8-byte magic NETBANK1, LE u32 number_of_books=2,
LE u32 reserved=0, then two complete7032-byte NETHD256 books in slot order.
Total14080 bytes. Case boundaries are supplied by the collector; the learner
does not discover the original case grouping in this experiment.

## Fresh generator, independent of learner

World IDs32..39, namespace netta-bank-v1|world|component for SHA256 seeds.
Keep the earlier raw-byte recurrence generator: last6 raw-byte RGS, NEW.35,
favored repeat.55, other repeats.10 (k1 repeats.65). N=16384 bytes/life.

For each k>=2, shuffle its RGS rows with a separate seed. floor(n_k/2) rows
share the same randomly chosen favored repeat in A/B. Remaining rows differ:
choose A uniformly and B uniformly among the other classes. k1 shares its
only repeat. Random choices, shuffles and target routing use separate seeds.

Within each disagreement stratum D_k, independently shuffle rows and assign
the mosaic's favored repeat alternately from A/B, with a seeded first slot.
No target receives an entire matching source law. Assert before generating
streams that mosaic != A and mosaic != B; failures stop, never redraw.
The mask and grammar descriptions go to provenance only, never the predictor.
Report actual visits to A/B-selected raw patterns because balanced row counts
need not imply balanced observed mass. Raw-pattern masks are not an oracle
for every learned-unit HEAD pattern and must not be passed to the engine.

Two independent source lives per book, each16KiB: total past64KiB, the same
source byte budget as the previous single-book experiments. A/B source
counts are collected with unchanged HEAD256. Each source has independent
trajectory and byte renaming. Three targets per world: mosaic, unrelated,
mosaic_then_unrelated. The third shares the first8192 bytes with mosaic;
its remaining8192 follows a separately sampled recurrence grammar. Targets
use the same trajectory/renaming seeds across regimes as before.

## Fixed comparisons

All modes share the same data, local learner and raw-byte clock.

- cold: P0, no imported authority.
- row: the new per-row Bayesian pair, single outer HMM16.
- global: SAME two P2 components with ONE log-odds shared across rows,
  same prior and after-truth update, same outer HMM16.
- pool: exact integer sum of the two books, original P2 and outer HMM16.
  It has the same past byte budget, though pooled support can cover extra rows.
- null0/null1/null2: row selector with a joint permutation of the two books'
  conditional-repeat rows within k. A common cyclic nonzero donor-row shift
  applies to BOTH books, keeping the pair's diversity. Each recipient book
  keeps its own total support and NEW probability. Single-row strata fixed.
  Null probabilities are exact floating conditional rows, not rounded u64
  counts. They are computed by the independent Python evaluator; direct C
  runs validate real row/global/pool modes. Permuting book IDs is not a null.

Three null seeds are fixed from head-bank-null-j. No capacity/hazard/source
strength/window scan. No selection of a favorable world, horizon or control.

## Single declared gate

The construction passes only if all the following hold on the new batch:

1. Predictions are causal, finite, positive and normalized within1e-8;
   every NEW byte equals P0 within1e-10. All full-prefix gains>=-1-1e-7
   and maximum drawdowns<=16+1e-7 under every real/control outer HMM.
   Independent formulas reproduce counts, inner mixture, gate and HMM.
   Actual C row/global/pool runs agree with Python on every raw event.
2. Mosaic mean row gain exceeds0.01 bit/raw byte vs P0 and vs per-world
   best of the three nulls; gain vs P0 is positive in all8 worlds.
3. Mosaic mean paired row advantage exceeds0.002 bit/raw byte over global
   and separately over pool (32.768 bits per16KiB). Show all8 paired values.

Report8192 and16384 gain, admission, row/global posterior examples, source
coverage, unrelated effects, changed-tail losses, worst prefixes and drawdowns.
Keep negative observations. If gate fails, diagnose from these records and
hand off the most informative bounded next question without retuning this set.

## Diagnostic identities and raw evidence

From the same per-component scores relative to P0 compute hindsight best
whole donor and best donor per HEAD row. These are unavailable-online
diagnostics, not comparators with earned authority. Inner row mixture score
equals sum_p(logadd2(score_pA,score_pB)-1). Its loss to the best fixed donor
on each row is at most1 bit/visited row, at most203 bits total. This identity
does NOT give the same bound for live gain after the outer gate/HMM.

Show actual past unit expansions, pattern, observed byte, P0/P2_A/P2_B/S/live,
inner weight before truth and its subsequent update on representative rows
favoring different books. These are binary prediction witnesses, not speech.

## Scope of a possible PASS

Useful composition across partly shared conditional relations in a known
synthetic family, with supplied case boundaries and fixed representation.
Not functional/semantic recognition, discovery of case families, a50-life
learning curve, natural-language transfer or live-Netta integration. Repeated
canonical RGS is still an exact coordinate; the novelty is selective use of
separate partially applicable histories within it.
