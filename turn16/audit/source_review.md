# Turn16: independent source review of Don turn15

2026-09-21, Astra mechanism hand. Incoming commit
`3cc77c19dbfb0e2584e231d652f833beef43a044`; Don checkout
`/Users/ataeff/arianna/netta-don-turn15-20260919` remained read-only.
References below are relative to this repository unless an absolute retained
artifact path is given. No generator, predictor, fresh world, or full reader
replay was run by this hand. The root/measurement hand owns the complete replay.

## Conclusion

The surface-map construction, causal quote order, and byte-bijection argument
match the implementation. I found no material-law defect in those mechanisms.
The sealed result remains **recorded material PASS**, with one demonstrated
floating-point exception to the separate claim that fast replay preserves NEW
*exactly*. The exception is one ULP in the exhibited saved price; it must be
recorded without silently rewriting the historical gate or its evidence.

The measured post-move benefit and the reversal of slow/fast ranking support
the next authority question. They do not identify a second admission event or
isolate the local-model rebuild as the sole cause of recovery.

## Authenticity and implementation boundary

All **15/15** `turn15/FREEZE.json` digests were recomputed from Don's checkout
and match, including the binary and frozen turn13/turn14 sources. The two
retained trace files used for the concrete counterexample below also match
`turn15/RESULTS_MANIFEST.json`. Python was run with
`PYTHONDONTWRITEBYTECODE=1`.

`turn15/experiment.py:81–92, 136–151` applies a dedicated-seed permutation to
already emitted recombined bytes. The learner receives only raw stdin; the
permutation is recorded in the hidden generator file, not an archive or a
prediction input. Whole and mid transformations are exactly the protocol's
byte transformation. `turn15/verify.py:72–104` independently draws the map and
checks the raw identities. There is no re-emission under a changed command
law. The mean/positivity and equivariance teeth at
`turn15/experiment.py:275–295` match the preregistered thresholds.

## Why the whole-life bijection commutes with the frontend

This is a code argument, in addition to the finite saved-world measurement:

1. In `court4/transfer4_confirm_core.c:155–182`, each distinct pair has a
   distinct first stream position: a position contains exactly one pair.
   Equal count and equal first position therefore cannot leave two distinct
   pairs for the numeric-key tiebreak to choose between. Hash-table location
   does not affect the winner.
2. Map base unit `b` to `pi(b)` and each learned unit `256+j` to `256+j` in
   the renamed build. Inductively the winning pair and its replaced positions
   correspond in every merge round. Expansion lengths and counts correspond
   too (`court4/transfer4_confirm_core.c:184–203, 205–234`). Segmentation applies
   the same ordered replacements (`:236–253`).
3. The floor depends on expansion length and the shared normalizer, and the
   projected unigram and context distributions use only corresponding counts
   (`byte_recurrence/frontend.c:44–72`). Under the same renaming, unit heads
   and byte probabilities correspond. Numeric byte order in normalization
   checks does not select a prediction or renormalize it.
4. `byte_recurrence/frontend.c:80–115` rebuilds only from the seen prefix,
   segments the seen suffix, and constructs equality classes by occurrence
   order. `turn13/episode.c:225–263` changes these into reverse-recency ranks
   and the same local canonical counts. Hence role history, stored matches,
   candidate probabilities at renamed truths, and outer evidence correspond.

The protocol allows floating summation tolerance; the stored full and 4096
gain deltas reported in `RESULT.json` are exactly zero. Those endpoint checks
are the named empirical gate. They are not themselves an exhaustive test of
all byte bijections; the general claim rests on the argument above.

## What is causal at the mid-life seam

`turn13/episode.c:473–505` finishes the common quote, matches, candidate and
live vectors before `fgetc`. Only afterward do `:518–530` update authority,
local counts, the raw-byte frontend, and role history. Frontend rebuilds read
only `state->past[0:state->n]` (`byte_recurrence/frontend.c:80–95`).

Consequently the entire pretruth state at **t=8192** is identical between
`recombined` and `moved_mid`; the rebuild at that index also sees the same
8192 bytes. In particular, the match decision at the first changed byte
cannot already know that the surface moved. Its observed truth can differ,
and later quotes can mix the old and new surfaces. The phrase “stored matches
die at the seam” (`turn15/PROTOCOL.md:29–30`) is an anticipated possible
effect, not an instantaneous state transition implemented by the code.

The model keeps the complete seen prefix at rebuild time; it does not
automatically discard the old surface at the seam. Role histories, local
counts, changed segmentation, current candidate quality, and the outer weight
all contribute to the post-move price. There is no ablation in this turn
that attributes recovery solely to a rebuild.

The raw examples in `turn15/RAW.md` are selected post hoc as the most helpful
and harmful **received** bytes, by `turn15/experiment.py:176–210`. Their metric
is `fast_live-logcold`, not candidate gain or fast-minus-slow. This is an
appropriate human-inspectable contrast, but two extrema cannot describe the
whole temporal recovery curve. The fixed tail gate is aggregate benefit over
8192 bytes, not a measured return time or readmission criterion.

## Authority: what is and is not being recovered

Let `r=P_candidate(x)/P0(x)` and let `w` be the pretruth source weight. The
live relative probability and after-truth source weight are

```
live/P0 = 1-w+w*r
q        = w*r/(1-w+w*r)
w_next   = (1-h)*q
```

This is `turn14/experiment.py:75–92`, also reconstructed with separate Decimal
probability masses at `turn14/verify.py:73–94`. Before admission the price is
cold; the crossing byte remains cold; the next byte begins with `w=1/2`.
The `active` flag is never turned off. Thus the word “re-admits” at
`turn15/PROTOCOL.md:32` does not name an event in this implementation.
Posterior source influence may recover after it becomes small, provided
later candidate evidence pays for that recovery. Old adverse evidence is
not reset and the cold branch has no return transition.

The initial cold capital of one half yields the complete-prefix one-bit
loss bound. After active updates, `w_next <= 1-h`, leaving cold weight at
least `h`; the absorbing cold continuation gives any later interval a
`-log2(h)` loss bound (10 fast, 16 slow). Neutral candidate evidence still
multiplies the source weight by `1-h`.

For the same candidate sequence and shared admission, the update is increasing
in `w` and decreasing in `h`. Therefore `w_fast <= w_slow` by induction. On
every helpful-candidate byte (`r>1`) greater source weight improves the live
price; on every harmful-candidate byte (`r<1`) it worsens it. The measured
turn14/turn15 ranking reversal is therefore a reversal in this realized
exposure tradeoff. It is useful evidence for a next mechanism. The stronger
statement that a changed law makes this memory “permanently stale”
(`turn15/REPORT.md:44–47`) is not established by finite target tails.

## Demonstrated precision exception: fast NEW is not literally exact

Protocol condition 6 says protected NEW stays exactly P0 in both modes
(`turn15/PROTOCOL.md:95–98`). C explicitly copies cold when the candidate
equals it (`turn13/episode.c:495–499`). The inherited Python fast replay uses
the generic log-mixture even when the two prices are equal
(`turn14/experiment.py:79–80`).

Existing, hashed evidence:

```
turn15/results/world160/moved_mid.tsv.gz
turn15/results/outer/world160/moved_mid.tsv.gz
t=186, arm=episode, truth=137, rank=0 (NEW), active_before=1
cold      = -8.2806284658485314
candidate = -8.2806284658485314
fast_live = -8.280628465848533
fast_odds_before = 3.3131365609313943
fast_live - cold = -1.7763568394002505e-15 bit
```

Absolute prefix for those files:
`/Users/ataeff/arianna/netta-don-turn15-20260919/`.

The minimal reproduction is to join these already saved TSVs by `t` and
`arm`, take row 186, and compare the parsed doubles; no model or new data is
needed. The reader compares fast numeric fields within tolerance
(`turn14/verify.py:146–150`), while its exact NEW checks come from inherited C
and candidate reconstruction (`turn13/verify.py:683, 711, 720–722`). Therefore
`c6_normalized_new_exact` does not establish literal fast-output equality.

This is a demonstrated inherited numerical defect, not a demonstrated change
to the scientific ranking or the quoted material margins. I did not rerun or
edit any sealed result. The bounded remedy for a newly authorized
implementation is the already established C equality branch; the historical
precision exception stays visible.

## Hand returned

No source or canonical files changed. No additional experiment or guard
campaign was run. Root's independent full replay supplies aggregate
arithmetic confirmation; this hand supplies the causal/code argument and the
specific precision counterexample above.
