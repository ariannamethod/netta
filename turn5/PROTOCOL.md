# Local revision of confidence in both directions

2026-09-16, Astra. Declared before new predictor code or worlds48..55.
One bounded step after the merged BANK2-COLD-ROW failure. The incoming
reproduction matches all numerical outcomes; its independent reader must
finish before the new data are generated. No tuning on worlds40..47.

## Question: BANK2-REVISE-ROW

Can a bounded local chance to revise old confidence improve actual live
prediction after a changed law, while keeping useful early transfer?
Old confidence includes both belief in a source and belief that both sources
should be ignored. Neither source book nor the local learner is erased.

All HEAD256/frontend/source-support/local-learning laws stay fixed. Keep
the same two immutable books, P0, P2_A and P2_B, and the local three-way
prior pi=(1/8,7/16,7/16). Before truth quote

    S(x) = w0*P0(x) + wA*P2_A(x) + wB*P2_B(x).

After pricing every COMPLETE context occurrence of row p, Bayes update that
row's three weights from the observed likelihoods, then apply

    w_j(next visit to p) = (1-rho)*posterior_j + rho*pi_j,
    rho = 2^-10.

The row's clock ticks only when that row occurs, including NEW, a one-class
row, or unavailable source components. NEW has identical likelihood in all
components, so contributes no selection evidence; the planned revision clock
still moves the posterior toward the prior. Incomplete contexts quote Pbase
and do not tick any row. No advance knowledge of a switch is supplied.

This is one change to Sol's static local posterior. Keep the single outer
shadow gate32 and absorbing-cold HMM h=2^-16 unchanged, with each arm using
its OWN candidate evidence. Crossing-byte price remains P0; the next event
starts outer odds1. Outer live loss bounds remain1 bit for the whole prefix
and16 bits for an interval. Candidate gain is never substituted for live gain.

## Parameter choice and cost, before data

Choose the largest inverse power of two rho whose worst-case no-revision
path price at N=16384 stays below the existing32-bit admission cost:
rho=2^-10 costs less than23.1 bits; rho=2^-9 would cost more than46 bits.
This is a declared budget choice, not a fitted timescale. No scan follows.

For n complete context visits over R visited rows, the unchanged-history
path has weight at least(1-rho)^(n-R). Hence the UNGATED revised candidate
can trail the static three-way candidate by at most

    (n-R)*[-log2(1-rho)]

at each full prefix. This is not a bound on live regret or on every suffix.
After transition, w0>=1/8192 and wA,wB>=7/16384. Thus confidence in a single
choice cannot accumulate without limit. The cold floor bounds one local
event loss by13 bits, not all future local losses by13. The global1/16
guarantees come from the retained outer HMM, not this local floor.

Store two log ratios per row relative to P0:406 doubles=3248 bytes, the same
router size as Sol's local-cold arm. Weights start at pi in every recipient.
Bank remains14080 bytes; four16KiB source lives, two per book, total64KiB.
No target-earned posterior travels, no source grouping is discovered here.

## Fixed new data and comparisons

Use unchanged turn3 recurrence generator, world IDs48..55 and namespace
`netta-revision-v1`. Keep source/target lengths, independent trajectories,
renamings and A/B mosaic construction. Three regimes: mosaic, unrelated,
mosaic_then_unrelated. Switch at8192; the first8192 raw bytes are identical
to the mosaic trajectory. No redraw or change in target law after inspection.

Paired real arms, same bytes and books:

- row2: original BANK2-ROW with static per-row A/B choice, unchanged C code.
- static3: Sol's static P0/A/B choice with her prior and unchanged outer.
- revise3: same three components, new local revision transition only.
- null3: revise3 with one joint conditional-repeat row permutation in both
  books, preserving recipient source support and each book's NEW mass.
  Within each k, a common cyclic nonzero shift maps both books' rows;
  singleton strata fixed. Seed component `head-bank-null-0` in the new
  namespace. Exact conditional probabilities, no integer rounding. This
  tests past correspondence; swapping book labels is not a null.

Actual C raw-byte executions validate row2/static3/revise3; null3 may be
evaluated in Python from the full chronological frontend trace. All
book-count, mixture and outer-state checks have an independent reader.

## One declared gate

PASS requires ALL of these on the fixed new eight-world batch:

1. Hash identity, complete chronology and positive normalized pretruth
   distributions within1e-8. NEW equals P0 within1e-10. Independent reader
   reproduces candidate/live prices and local/outer states within1e-7 bits;
   C and Python agree at every real-arm event. Full-prefix live gain>=-1-1e-7
   and interval drawdown<=16+1e-7 for every arm. The candidate-only static
   path bound above holds at every prefix; declared weight floors hold.
2. Mosaic revise3 gains at least0.0075 bit/byte in the mean versus P0 and
   versus null3, is positive versus P0 in all8 worlds, and retains at least
   70% of BOTH row2 and static3 paired mean mosaic gains.
3. On the changed last8192 bytes, revise3 improves mean tail gain by MORE
   THAN1 bit against EACH of row2 and static3, improves at least5/8 paired
   tails against EACH, and improves the worst tail by MORE THAN1 bit against
   EACH comparator. No earlier gain can excuse a worse tail.

Show all worlds, early1/4/8/16KiB gains, admissions, unrelated behavior,
minimum prefix, drawdown, candidate and live tail differences, and actual
raw-byte examples of helpful revision and harm. Where possible show an old
source belief and an old cold belief being revised. Illustrations are
post-hoc explanations, never selection of the tested sample.

If this gate fails, preserve FAIL, diagnose these saved traces, and return
one informative next question. Do not change rho, priors, gates, thresholds,
worlds or arm weights to manufacture a pass. A passing gate ends this step.

## Scope

This tests revision within one recipient on exact HEAD256 coordinates of
partly applicable synthetic source laws. It does not test functional or
semantic matching, automatic source families, cumulative50-life learning,
speech, live Netta integration, or mycelium feedback. The outer cold path
remains absorbing; its residual inertia is deliberately unchanged.
