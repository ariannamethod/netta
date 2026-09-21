# What changed in the received tail

2026-09-16. Read-only analysis of the fixed, already-produced worlds48..55.
No new predictor, parameter, dataset, replayed intervention or experiment.
The parent/independent reader owns final verification of the material PASS.

Inputs: `turn5/RESULT.json` and all eight
`turn5/results/events/worldXX/mosaic_then_unrelated.tsv.gz` files. The result
SHA-256 is
`650028b662f6697849a80bfd0e7a3d948c75bad0ca29cb9f554d6405d31f6d21`.
Every tail is the fixed raw-byte interval t=8192..16383. No world is removed.

## Exact decomposition, with a declared reference order

For each byte let r_old=S_static/P0, r_new=S_revision/P0, and let w_old,
w_new be the corresponding **actual recorded** outer source weights before
the byte (zero before admission). Define

```
F(w,r) = log2(1 + w*(r-1))
candidate part = F(w_old,r_new) - F(w_old,r_old)
outer part     = F(w_new,r_new) - F(w_old,r_new)
paired live    = candidate part + outer part.
```

The first term changes the current candidate while holding the recorded old
outer weight; the second changes the outer weight at the new candidate.
This is an algebraic decomposition of saved quotes, not an independently
executed policy with frozen old authority. Reversing the reference order
would give different component totals but the same paired live difference.
It uses no hidden generator routing information or oracle comparator.

For numerical stability the calculation used
`log1p(w*expm1(log_ratio*ln(2)))/ln(2)`. The largest event-wise discrepancy
between the two-term sum and saved live-price difference is
**3.552713678800501e-15 bits**.

## All eight changed tails, compared with static3

Positive differences favor revise3. Tail gains in the first two columns are
each arm's actual received gain against P0, not its ungated candidate score.

| World | static3 tail | revise3 tail | Candidate change at old outer | Outer change | Paired live difference |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 48 | -6.813181 | -5.240461 | 6.296485 | -4.723765 | 1.572720 |
| 49 | -1.688264 | -1.196879 | 2.003864 | -1.512479 | 0.491385 |
| 50 | -7.709000 | -7.580725 | 0.256104 | -0.127829 | 0.128275 |
| 51 | -7.186879 | -7.279908 | 2.779605 | -2.872634 | -0.093029 |
| 52 | -8.686464 | -4.436425 | 10.325809 | -6.075769 | 4.250040 |
| 53 | -6.434440 | -4.559615 | 6.170343 | -4.295518 | 1.874825 |
| 54 | -5.752437 | -2.815567 | 8.821850 | -5.884981 | 2.936869 |
| 55 | -6.594059 | 8.497687 | 13.429576 | 1.662170 | 15.091746 |
| Sum | — | — | **50.083635** | **-23.830805** | **26.252830** |
| Mean | **-6.358091** | **-3.076487** | **6.260454** | **-2.978851** | **3.281604** |

The candidate-change term is positive in all eight worlds. The outer-change
term is negative in seven. Thus the new local revision improves prices at
times when the old arm still had influence, but its changed outer posterior
cancels a substantial part of that gain. World51 makes this explicit:
+2.779605 is outweighed by -2.872634, leaving the single negative paired
static3 comparison. World55 contributes a large +15.091746 of the total;
its positive outer term is visible separately rather than hidden in the mean.

The ungated candidate tail advantage over static3 is much larger (mean
92.248825 bits). It must not replace the received +3.281604-bit comparison.
Seven revised tails still lose to P0, and the average revised tail still
loses 3.076487 bits. The passing comparison establishes an improvement over
the paired alternatives, not elimination of changed-law harm.

## Raw witness: an old cold preference is reconsidered

World52, `t=8865`, HEAD256 row `012340`, observed byte **204 / 0xCC**,
repeat group1. All weights/prices below are before this truth is charged.

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Cold weight | 0.5154535766 | 0.0988949357 |
| A weight | 0.3757601056 | 0.0732818438 |
| B weight | 0.1087863177 | 0.8278232205 |
| Outer source weight | 0.6542277465 | 0.9956371563 |
| Candidate log2 price | -3.6974831887 | -2.9868691708 |
| Live log2 price | -3.6740119623 | -2.9891365913 |

The common component prices are P0=-3.6306210022, A=-4.1988309570,
B=-2.8581652718. Here the revision arm trusts the useful B prediction while
the static row still gives its largest mass to cold. Its live advantage is
**0.6848753710 bits**: +0.4953844871 candidate part and +0.1894908839 outer
part. This is a concrete change in remembered confidence, not a new source
archive or a different local P0 learner.

## Raw witness: a previously dominant source loses its monopoly

World54, `t=8540`, row `012023`, observed byte **232 / 0xE8**, repeat group0.

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Cold weight | 0.0443707727 | 0.0089085099 |
| A weight | 0.9556278015 | 0.1654311318 |
| B weight | 0.0000014258 | 0.8256603582 |
| Outer source weight | 0.9601648300 | 0.9945742782 |
| Candidate log2 price | -4.9192948552 | -3.8395222331 |
| Live log2 price | -4.9141360618 | -3.8433185711 |

Common prices: P0=-4.7950651186, A=-4.9253341235,
B=-3.6874139877. Static A confidence nearly excludes B. Revision has moved
the dominant weight to B, which prices this observation better than P0.
Live advantage: **1.0708174907 bits**, including a +1.0465060504 candidate
part at the old outer weight. This witness is source-to-source reconsideration;
it does not claim that cold became the majority branch.

Both helpful witnesses were selected after the run as large positive
candidate contributions among examples with the displayed majority changes.
They illustrate mechanisms; they are not an independently selected test set.
The table above retains every world and the failed paired tail.

## Remaining harm: more authority can still receive a wrong prediction

World53, `t=10593`, row `000123`, observed byte **143 / 0x8F**, repeat group1.

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| A weight | 0.9369178346 | 0.9341611634 |
| B weight | 0.0000000601 | 0.0011540707 |
| Cold weight | 0.0630821053 | 0.0646847659 |
| Outer source weight | 3.1911179647e-14 | 0.8067098182 |
| Candidate log2 price | -8.5487976079 | -8.4611685035 |
| Live log2 price | -6.8809196634 | -7.9916066574 |

Common prices: P0=-6.8809196634, A=-8.7775220311,
B=-2.8513900241. Revision preserves considerably more B probability than
static, yet still puts 93.4% on the wrong A branch before this event. Its
candidate is 0.0876291043 bits better than static's, while its received price
is **1.1106869940 bits worse**. The old outer arm is effectively cold, so
the candidate term is approximately zero and the outer term accounts for
the harm. This is the largest negative paired live event in the eight tails,
retained as a counterexample rather than filtered from the explanation.

For these three witnesses the full source records are respectively

```
turn5/results/events/world52/mosaic_then_unrelated.tsv.gz  t=8865
turn5/results/events/world54/mosaic_then_unrelated.tsv.gz  t=8540
turn5/results/events/world53/mosaic_then_unrelated.tsv.gz  t=10593
```

Each contains static3/revise3 pretruth weights, before/after log ratios,
component/candidate/live prices and outer states. The matching learned-unit
expansions are in `turn5/traces/worldXX/mosaic_then_unrelated.tsv.gz` at the
same raw-byte index. The parent prepares those expansions for its raw report.

## Scope of the conclusion

On this fixed synthetic batch, bidirectional local revision changes received
predictions usefully, not only candidate accounting. Its price improvement
survives the separate outer mechanism often enough to satisfy the declared
aggregate comparisons. The outer interaction remains material and the
revised predictor still makes harmful individual decisions and loses most
changed tails against P0. This closes the requested analysis of the passed
step; no next mechanism or parameter trial is started here.
