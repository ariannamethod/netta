# Turn28: incoming CUSUM and the address of a useful memory part

2026-09-25, Astra mechanism lane. Incoming commit
`48e36938f887ec57b26e4ac20b83f58dd5d149ab`. The Don checkout and all inherited
sources were read only. This review inspects code and pinned source archives;
the other independent lane owns the complete numerical replay. No fresh worlds,
candidate simulations, parameter searches, or source changes were performed.

## 1. Incoming implementation

No new C defect was demonstrated in the scoped inspection.

- [cusum.h](../../turn27/cusum.h:7) fixes drift 0.5 and threshold 8.
- [cusum.c](../../turn27/cusum.c:31) works on a state copy, selects the inherited
  slow/fast mode from the **old** latch, and invokes the unchanged turn22 odds
  and admission law. State and output arguments are committed only at the end.
- [cusum.c](../../turn27/cusum.c:39) updates the sum only when active before
  this observation and `matched >= 1`. It then sets the flag at S >= 8. There
  is no release branch. First admission resets S and the flag and excludes its
  crossing observation from the statistic.
- [replay.c](../../turn27/replay.c:112) obtains all six quotes before any
  observation. The oracle alone changes mode at its declared seam
  ([line 102](../../turn27/replay.c:102)). CUSUM and the other causal arms do
  not receive that seam through their APIs.
- [replay.c](../../turn27/replay.c:144) checks shared admission; its later
  assertions inspect the exact inherited hysteresis and CUSUM transitions.

A threshold crossing at observation t affects the hazard used after pricing
observation t+1. The resulting odds affect the quote at t+2. This is the same
old-state chronology as the declared law. An unmatched observation holds S;
a matched equal-price / NEW observation contributes the drift −0.5 and can
reduce S, while the permanent latch remains set. These are distinct clocks.

The measured 56-byte `CSState` is correctly disclosed in
[BUILD_NOTES.md](../../turn27/BUILD_NOTES.md:69); the protocol's anticipated
40-byte class was an estimate. The separately preserved reporting-reader
repair does not change this C law. The incoming material FAIL remains FAIL.

## 2. Exact scope of the clipping proposal

The report correctly rejects deriving a per-observation witness bound from
the whole-prefix live mixture bound. Its stronger statement that this
implemented episode candidate has an **unbounded negative delta** needs a
correction. See [REPORT.md](../../turn27/REPORT.md:19).

For an available repeated head r, the implemented candidate is

    P_j(r) = M * (n_r + 0.5) / (T + 0.5*k),
    M = sum of current cold probabilities over the k repeated heads,
    T = sum of the stored counts over those available heads.

This is [episode.c](../../turn13/episode.c:312). Since P0(r) <= M,

    P_j(r)/P0(r) >= (n_r + 0.5)/(T + 0.5*k).

The archive stores seven uint16 counts per record
([episode.c](../../turn13/episode.c:26)); k <= 6 and T <= 6*65535. Thus a
conservative universal bound for this implementation is

    delta >= -log2(786426) = -19.584951493789514 bits.

For NEW or an unavailable candidate, delta is exactly zero. This finite bound
still permits a single harmful observation to exceed the eight-bit alarm
threshold; it does not rescue CUSUM's observed false alarms.

As a narrow archive census, all eight Don27 episode books were read and
checked against `MEMORY_MANIFEST.json`: each has 32 rules, 24 records and
528 serialized bytes. Maximum per-record total counts by world 256..263 are
5275, 5257, 16395, 16617, 16779, 16506, 16550, 16409. These finite stored
supports are another direct reason the implemented negative influence is
bounded. Source paths are
`/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn27/memory/world*/episodes.bin`.

For the proposed update

    S_next = max(0, S + min(2, -delta) - 0.5),

the proven new fact is an increment bound of 1.5 per voting observation.
Six observations with delta <= -2 give S = 9 from zero. Therefore clipping
does not guarantee S < 8, absence of intact alarms, or a detection deadline.
The report's proposed intact-maximum and deadline claims
([REPORT.md](../../turn27/REPORT.md:99)) need an experiment or a distributional
argument including the life horizon and dependence of observations.

CUSUM is a reflected cumulative sum: S_n is the maximum suffix sum of the
increments, including the empty suffix. It discards earlier debt at a return
to zero. The **latch** is permanent. Negative mean and the reported variance
alone do not bound the maximum over a long life. With appropriate stationary
recurrence assumptions and adverse finite runs of positive probability, even
bounded increments can eventually cross a fixed threshold; this is not a
theorem for every possible input sequence. A single C5 contrast does not
establish that one statistic family is uniquely the right mechanism.

## 3. Where the archive's parts lose their independence

The current source archive already has stable, compact addresses for parts:

1. A branch owns the first `prefix_len` role events of a grammar expansion.
   The loader rejects duplicate prefix contents
   ([episode.c](../../turn13/episode.c:141)). The role alphabet is NEW plus the
   reverse-recency ranks of distinct heads of the previous six learned units
   ([line 225](../../turn13/episode.c:225)).
2. [branch_match](../../turn13/episode.c:279) finds the longest matching stored
   prefix and returns its unique **record index** in `matches[0]`.
3. [candidate](../../turn13/episode.c:312) uses that record's source counts.
   There is no recipient state attached to the record. Shorter matching
   records cannot replace a mistaken longest record.
4. [predict](../../turn13/episode.c:460) owns one outer state per complete arm,
   not per branch. The same odds and shadow pay for every selected episode
   record. A bad branch can spend authority previously earned by another.
5. The printed source trace retains record identity, but
   [turn27/experiment.py](../../turn27/experiment.py:224) reduces the authority
   input to t, cold, candidate, matched length. Record identity is discarded
   before the new CUSUM layer. Changing that layer's hazard alone cannot learn
   which record is appropriate.

There is a second loss of distinctions on the source side:
[branch_candidates](../../turn13/experiment.py:208) sums continuation counts
over all source lives. The grammar and greedy selection likewise aggregate
source tapes ([grow](../../turn13/experiment.py:166),
[joint_select](../../turn13/experiment.py:243)). A recipient therefore cannot
choose between two source cases that supplied conflicting continuations to
the same prefix: those alternatives have already been added together.

The four source lives currently combine two pairs of motifs, while the target
recombines all four ([generator](../../turn13/experiment.py:71)). The moved
surface life is a byte bijection applied after emission
([turn15/experiment.py](../../turn15/experiment.py:136)). These constructions
exercise recombination and a concrete form of renaming. They do not yet give
a recipient a separate stored alternative for each past case.

## 4. Two concrete constructions

### Smallest recipient-only change: confidence per selected record

Keep the archive and longest-match selector. Allocate one log odds per stored
record for source versus P0, initially zero. Before truth, quote

    Q = (1-a_i)*P0 + a_i*P_i,  a_i = logistic2(log_odds_i),

for the selected record i. After truth update only its log odds by
`log2(P_i/P0)`. Other records retain their evidence. No match uses P0. Keep the
same external authority law over Q. This adds 8R recipient bytes (192 for the
current 24 records), O(1) arithmetic per selected event, and no source bytes.
There is no new per-record 32-bit admission barrier.

On each record's selected-event subsequence, its ungated predictive likelihood
ratio telescopes to `(1 + product(P_i/P0))/2`, so its whole-subsequence loss is
at most one bit. The external mixture keeps its own whole-life bound. This
construction can reject an unsuitable record independently; it cannot recover
the distinct source alternatives already pooled into that record, and pure
Bayes accumulates old contrary evidence indefinitely.

### More direct past-case memory: two vectors per prefix, local three-way choice

Keep one grammar and store two continuation vectors A/B at each prefix.
Each selected prefix gets its own recipient weights for P0, A and B. The
component distributions reuse the unchanged `candidate()` function. Updating
only the currently selected prefix allows A to help one relation while B
helps another, without requiring one globally winning source case.

The already implemented [turn5 revision law](../../turn5/revision.c:77) provides
two log ratios per local address; its fixed-share update after truth
([line 199](../../turn5/revision.c:199)) can be reused as mathematics. With its
existing priors (1/8, 7/16, 7/16) and share rate 2^-10, old local confidence can
be revisited. The event clock must be declared: matching is selected before
truth, including a matched NEW observation whose component likelihoods are
identical. The local selector should learn prospectively before the external
32-bit admission as well. It must never select an update from the observed
truth's rank or the generator's labels.

Budget with 32 grammar rules:

    16-byte header + 32*4-byte rules + 12*32-byte paired records = 528 bytes.

Two addresses/count vectors require 30 logical bytes per record: rule id and
prefix length (2), followed by two times seven uint16 counts (28). If records
are 32 bytes, the remaining two bytes need a declared reserved-zero layout.
Recipient router state for 12 records is 12*2 doubles = 192 bytes, plus the
unchanged external authority. The same router using only two global log ratios
is a direct control for the value of **local** selection. A pooled archive
with 24 ordinary records occupies the same 528 bytes; a pooled 12-record
control using the same addresses separates retained alternatives from address
coverage. Fresh data and the actual contrasts still need their preregistration.

The source split, source-only selection objective, record layout and update
clock must be fixed before implementation. Counts must be recounted per source
case rather than recovered from pooled totals. This directly preserves a
distinction that current source construction destroys.

## 5. Feasible C reuse and scope

A new `bank.c` can include the frozen implementation inside one translation
unit:

    #define main inherited_episode_main
    #include ".build/turn13/episode.c"
    #undef main

The private `.build/layout` can use the same symlinks as turn27's Makefile.
This preserves the old source's `../byte_recurrence` includes and keeps all
writes inside turn28. `Quote`, `History`, `quote_cold`, `observe_cold`,
`candidate`, `validate`, `branch_match`, `load_grammar` and the stable log-add
helper are then available. The renamed old main references its existing
helpers, so they remain used under strict compilation. Only the paired loader,
router state and new prediction loop need new C. The old frontend and portable
recurrence objects are linked unchanged.

At each byte: build the common local quote, select the stored address from
past history, construct all A/B/router/control vectors, validate them, then
read the byte. Charge the fixed pretruth quotes; update the selected router and
the external authority; update the common local learner and append the actual
role event. Exact NEW should be copied from P0 explicitly. No archive history
or source count is overwritten by recipient evidence.

Per-byte matching remains O(R*H) with H <= 32, followed by O(256) validation
and mixture construction, or O(k) for the differing head entries themselves.
State for local routing is O(R). This is small beside the existing frontend.

For longer diverse recipient lives, separate source and recipient lengths:
blindly changing the shared `t13.N` also lengthens all four sources and their
counts. [branch_candidates](../../turn13/experiment.py:225) correctly rejects
counts beyond uint16; increasing life length must not silently wrap them.
The proposed part selector still requires exact equality of a stored role
suffix. It can express partial applicability of known relations; fuzzy or
functional matching beyond those coordinates is a separate construction.

This report recommends the paired-prefix memory as the concrete next design
to preregister. It makes no empirical claim that the new selector wins.
