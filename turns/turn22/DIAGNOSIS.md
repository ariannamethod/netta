# Turn22: post-hoc diagnosis of relative-support authority

2026-09-21. Only RESULT summaries and retained authority traces were read;
no new forecasts, rules, parameters or data were evaluated. All comparisons
below use the same episode candidate. Trace t is zero-based, seam t8192.

## Verdict and whole-life accounting

[RESULT.json](RESULT.json) preserves **FAIL 11/18**. Selfnorm meets the law-tail
comparison in 6/8 worlds, versus fresh h8l4's 4/8, and has mean law-tail gain
+0.007897 bits relative to fast. Its +0.945699 bits over h8l4 does not clear
the strict >1-bit gate. The common unrelated-admission failure is separate
from the authority utility failures.

Mean paired gain against h8l4, bits per life:

| Regime | First8192 | Final8192 | Whole16384 |
|---|---:|---:|---:|
| Recombined | -10.903340 | -9.907260 | -20.810601 |
| Switched law | -10.903340 | +0.945699 | -9.957641 |
| Moved surface | -10.903340 | -12.694708 | -23.598048 |

The shared prefix is one body of evidence, repeated by construction across
the three regimes. The law-tail improvement does not repay its earlier cost.
Recombined early/full retention against slow is 98.9193%/99.1284%, passing
the relative retention bars. Moved-tail differences remain -7.126202 bits
against fast and -13.438891 against slow, failing the separate absolute bars.

## What the normalized budget actually does

These are averages of recorded prequote states across active observations;
mean B is calculated per observation, not from mean w and mean a.

| Segment | Mean w | Mean a | Mean B | Median B | B=0 observations |
|---|---:|---:|---:|---:|---:|
| Common first8192 | 7.8978 | 20.1702 | 6.1365 | 6.5724 | 6588/64574 |
| Recombined final8192 | 10.0904 | 21.7607 | 7.1889 | 7.5019 | 1776/65536 |
| Switched final8192 | -9.0216 | 31.2786 | 0.2206 | 0 | 58346/65536 |
| Moved final8192 | 4.5220 | 21.4958 | 4.0452 | 3.8280 | 13676/65536 |

The law change raises absolute testimony a while net w becomes negative,
so the budget spends most of that tail at zero log odds. Zero B permits at
most half source mass; it does not mean source probability is zero. On moved
tails, the budget also falls to zero frequently despite remaining useful
transfer. Conversely B exceeds eight on 9520 moved-tail observations and can
carry more authority than the static incumbent. The formula therefore causes
both stronger withdrawal and episodes of greater commitment.

Exact observed-price decomposition, **mean paired bits per life** relative
to h8l4, grouped by the actual sign of S(y)-C(y):

| Segment | Candidate helps | Candidate harms | Equal | Net |
|---|---:|---:|---:|---:|
| Common first8192 | -85.525284 | +74.621943 | 0 | -10.903340 |
| Switched final8192 | -15.860884 | +16.806584 | 0 | +0.945699 |
| Moved final8192 | -104.661946 | +91.967238 | 0 | -12.694708 |

On moved tails, observations where selfnorm starts with *lower* odds than
h8l4 contribute -118.927473 bits on helpful advice and +107.260332 on harmful
advice: net -11.667141. Where it starts with *higher* odds, it gains 14.265527
on helpful advice and loses 15.293095 on harmful advice: net -1.027567.
These are realized outcome partitions, not prospective selection rules.

## Raw recovery example and its charged neighbors

World228, switched, includes the saved `raw_harm` at t9062. C and S are log2
prices of the realized byte. The last column is selfnorm minus h8l4.

| t | C | S | Selfnorm quote | h8l4 quote | Paired bits |
|---|---:|---:|---:|---:|---:|
| 9059 | -2.560550 | -5.576343 | -3.171904 | -4.868535 | +1.696630 |
| 9060 | -3.053645 | -3.053645 | -3.053645 | -3.053645 | 0 |
| 9061 | -10.421651 | -10.421651 | -10.421651 | -10.421651 | 0 |
| 9062 | -6.480316 | -2.059050 | -5.146560 | -2.849482 | -2.297078 |
| 9063 | -1.764155 | -5.100817 | -2.982266 | -4.694524 | +1.712258 |

At t9059, w goes 2.549351 -> -0.546110 and a goes 29.773691 -> 31.859056;
B drops 1.325470 -> 0 after charging the bad advice. No additional clip
fires: ordinary updated odds are already below that cap. The lower authority
carried into this neighborhood saves bits on t9059 and t9063.

At t9062, useful advice arrives with w=-0.529044, a=30.863461, B=0 and source
odds -3.636077, versus h8l4's +0.333406. The new +4.421265 bits of candidate
evidence raise w to 3.908754 and B to 1.770658, but only after the observation
has already lost 2.297078 bits against h8l4. Raising a permitted ceiling does
not restore discarded source mass. These rows show protection and missed
useful transfer in the same local sequence; they do not attribute the entire
pre-existing authority gap to the one preceding observation.

## The largest moved-tail harm has the opposite cause

World224,t14455, the saved `raw_harm`, carries w=11.915134, a=16.418050 and
B=10.945091. Selfnorm source odds are 10.798627 versus h8l4's 7.209554.
The candidate is badly wrong: S=-13.458765 versus C=-1.932868. Selfnorm
quotes -12.050589 while h8l4 quotes -9.081481, losing **2.969108 bits**.

After truth, w falls to 0.016889, a rises to 27.430883 and B collapses to
0.009505. The ordinary posterior is already below this new cap, so the clip
flag is again zero. The relative-support rule responds after the expensive
observation; its preceding positive testimony had authorized greater exposure.
This example is excessive commitment, not delayed recovery. Both mechanisms
must remain in the diagnosis.

## One next question for Sol

Can support and contradiction be attributed separately to two distinguishable
fragments of transferred experience, so that an error in one does not also
delay using the other? The current global budget merges those judgments.
That is a bounded question about retaining useful partial transfer, rather
than another search for one global cap value. These traces have not tested
the proposed separation or shown which fragment identities would suffice.

The pathwise bounds held, but the complete named gate failed. The result
does not establish impossibility of self-normalization or robust transfer.
No next protocol, mechanism or experiment has been executed in this diagnosis.
