# Turn31 diagnosis: a changed preference with little net paid improvement

Post hoc, 2026-09-26. Uses only saved source counts, router states and prices.
No alternative forecast, changed parameter or additional world was evaluated.
`python3 -B turns/turn31/diagnose.py` reproduces `DIAGNOSIS.json`; that script
does not import the experimental evaluator. JSON includes the input hashes,
all paired world values, descriptive subsets and the protocol-selected raw rows.

## Result retained

Independent `VERIFY.json`: 3,932,160 forecasts reconstructed, maximum error
1.2590817277668975e-11. F1 FAIL; F2/F3/F4 PASS. Material **FAIL** remains.
The missing condition is mean partial-tail factored-minus-flat3 > 1 bit:
the observed mean is **0.7426422583 bits**, with 6/8 positive worlds.
Factored-minus-balanced is 9.6935418516 bits, with 5/8 positive worlds.

All following contrasts are mean bits per life. Candidate means the recorded
inner forecast; live means the actual recorded forecast after admission/HMM.

| Regime and window | Factored−flat3 candidate | Factored−flat3 live | Flat3−balanced live | Factored−pooled live |
|---|---:|---:|---:|---:|
| Recombined early 4096 | 0.328640 | 0.392451 | 85.780624 | −6.603447 |
| Recombined whole | 0.787605 | 0.845850 | 401.106560 | −16.753768 |
| Partial whole | 1.277810 | 1.350214 | 202.470311 | −54.875236 |
| Partial tail | 0.733971 | 0.742642 | 8.950900 | −50.303221 |
| Switched tail | −0.173811 | −0.179531 | −15.108303 | 19.285652 |
| Moved tail | 0.452255 | 0.504619 | 98.871624 | −18.973509 |
| Unrelated whole | 0.596136 | 0.989423 | −4.519628 | 3.289647 |

Retention versus pooled passes: recombined early 98.419358%, recombined whole
99.124219%, partial whole 96.163508%. Passing that retention threshold is
compatible with losing 50.303221 bits to pooled on the manipulated partial tail.

## Conditional evidence changes; permission limits its immediate influence

For each saved flat state, derive `u_flat=wA+wB` and `v_flat=wA/u_flat`.
The actual coefficient of remembered advice in a live forecast is
`outer_source_probability * u`. This is an identity of the recorded mixture,
not a re-evaluation under other weights. Before admission that coefficient is zero.

Partial-tail descriptions below cover all eight lives. Counts are matched
observations; the bins overlap. The final column is the sum of their actual
factored-minus-flat3 prices divided by eight, not a gain per observation.

| Recorded pretruth subset | Observations | Mean absolute Δv | Mean live memory mass, factored / flat | Live contrast, bits/life |
|---|---:|---:|---:|---:|
| All matched | 41,239 | 0.073200 | 0.608184 / 0.606560 | 0.742642 |
| Flat u ≤ 1/2 | 14,793 | 0.177233 | 0.096844 / 0.091264 | 0.482798 |
| Flat u ≤ 2ρu0 = 7/4096 | 216 | 0.412566 | 0.001355 / 0.001301 | 0.000092 |
| Opposite A/B majority | 2,459 | 0.482581 | 0.161897 / 0.186155 | 0.727132 |

The smallest-u bin corresponds to the preceding flat conditional reset
`lambda=rho*u0/u_next ≥ 1/2`. It is a descriptive region of the derived
mechanism, not a newly selected gate. Factored and flat have different
preferences on 41,223/41,239 matched partial-tail rows at tolerance 1e-6;
the maximum difference is 0.875855. Thus the conditional-state change operated.
Its most pronounced low-permission differences carry very little probability
mass into the current forecast.

The remaining limitation includes cancellation. Partial-tail absolute
bytewise contrasts sum to 94.990290 candidate bits and 96.337897 live bits
per life, while their signed sums are only 0.733971 and 0.742642. The HMM
does not account for the missing material gain by hiding a large positive
candidate result. Those two arms also have different outer histories; their
candidate/live difference is not an isolated causal price of outer authority.
On 9,797 matched tail rows both source prices equal P0 exactly and both
contrasts are zero despite possible conditional-state differences.

Opposite case-majority rows contribute +0.727132 bits on the partial tail,
but −2.534562 bits over the recombined whole life. This labels observations
in the actual run; it does not hold u or the outer state fixed to estimate
the isolated effect of v.

## Harmful source advice stays in the result

This required subset is selected after truth: both PA(x) and PB(x) < P0(x).
It is descriptive and was never filtered out of any gate. Whole-life values:

| Regime | Such observations, all worlds | Factored gain vs P0, bits/life | Flat3 gain vs P0 | Factored−flat3 |
|---|---:|---:|---:|---:|
| Recombined | 17,102 | −861.848983 | −869.423794 | 7.574811 |
| Partial | 17,470 | −1063.992926 | −1064.640021 | 0.647094 |
| Switched | 18,106 | −811.412128 | −814.830180 | 3.418052 |
| Moved | 18,203 | −812.561843 | −818.859079 | 6.297236 |
| Unrelated | 18,477 | −402.917934 | −404.287461 | 1.369527 |

On the **partial tail**, the 8,392 both-worse observations instead contribute
−1.689726 bits to factored-minus-flat3. Where exactly one source beats P0 and
the other loses, the 13,390 observations contribute +5.112753 bits. These
observed contributions prohibit reducing the result to a universal retreat
benefit. They also do not establish that the conditional clock alone causes
either contribution: u and later outer authority inherit its history.

## The protocol's largest help and harm: same prefix, different permission

Both selected extrema are world292/recombined, record8, prefix `3101`.
Immutable source counts for roles 0..6:

```
A: [18, 24, 20, 967, 0, 0, 0]
B: [21, 45, 996, 22, 0, 0, 0]
```

| t / observed rank | log2 P0 | log2 PA | log2 PB | u factored / flat | v factored / flat | Live factored−flat |
|---|---:|---:|---:|---:|---:|---:|
| 1456 / 3 | −2.130248 | −1.514612 | −7.013096 | 0.407355 / 0.970931 | 0.000789 / 0.000482 | **+3.271381** |
| 1518 / 2 | −6.173465 | −6.659974 | −1.129019 | 0.008358 / 0.204217 | 0.032742 / 0.021884 | **−2.550876** |

At the largest help both prefer B, and A would price truth better. The lower
factored permission limits the wrong B advice: live prices −2.850355 versus
−6.121736. At the largest harm both again prefer B, now useful, but factored
has little permission left: live −5.841809 versus −3.290934. Their outer source
odds are positive in both examples; the severe permission gap is already inside
the candidate (−5.841604 versus −3.287717 at t1518).

The adjacent **visits to this record**, rather than adjacent unrelated bytes,
show the actual carried state:

| t | Observed rank | u factored / flat before truth | v factored / flat before truth | Live contrast |
|---|---:|---:|---:|---:|
| 1444 | 2 | 0.123063 / 0.871393 | 0.013738 / 0.001920 | −1.585640 |
| 1456 | 3 | 0.407355 / 0.970931 | 0.000789 / 0.000482 | 3.271381 |
| 1468 | 3 | 0.024373 / 0.536549 | 0.034928 / 0.022080 | 0.876494 |
| 1480 | 3 | 0.004970 / 0.130326 | 0.620542 / 0.505094 | −0.303375 |
| 1494 | 2 | 0.018324 / 0.305211 | 0.986179 / 0.977445 | 0.248343 |
| 1506 | 2 | 0.007525 / 0.165023 | 0.606693 / 0.483897 | −0.067736 |
| 1518 | 2 | 0.008358 / 0.204217 | 0.032742 / 0.021884 | −2.550876 |
| 1530 | 2 | 0.212777 / 0.892312 | 0.001219 / 0.000962 | −1.057412 |

Raw 33-byte neighborhoods, with the center byte being the priced truth:

```
t1456: c3c38181c3232381230923098181cdcd095454cd54165416cdcd141416c8c814c8
t1518: 0909f0f00925f025f0f0f0152525818125eb81eb81810deb81812d2d812deb2d81
```

## One next question: distinguish case learning from repairing its starting shape

Adaptive flat3 beats the fixed equal normalized mixture by 401.106560 bits
on recombined whole lives, while flat3 loses 17.599618 bits to pooled. The
equal mixture itself loses 418.706178 bits to pooled. These observed comparisons
show a useful adaptive-case contrast within that starting representation;
they do not identify an economic advantage of storing distinct cases over
pooling their evidence.

The source distributions begin differently. For valid repeat roles,
`(a_r+.5)/(NA+k/2)` and `(b_r+.5)/(NB+k/2)` are separately normalized,
whereas pooling uses `(a_r+b_r+.5)/(NA+NB+k/2)`. Equal averaging discards
relative source support and allocates the smoothing mass differently.
In the 96 stored records, 76 have A's share of repeat observations more
than 0.1 away from 1/2; five have no A repeat observations and seven no B
repeat observations. Those are source-only counts across all repeat roles;
they are not target-weighted or causal explanations of the gain.

**Next question for Sol:** can distinct cases add earned predictive corrections
when the initial forecast preserves the pooled source mass and smoothing,
rather than first asking recipient learning to repair an equal case average?
That would separate useful case distinctions from the starting-distribution
cost. This turn measures neither that construction nor a better clock; the
current small effect and failed F1 remain the result handed back.
