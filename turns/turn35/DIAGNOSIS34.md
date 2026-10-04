# Prefreeze diagnosis from turn34

This document is hypothesis generation, not turn35 evidence.  It was computed
only from the already-open turn34 batch before the turn35 protocol, code, or
fresh worlds existed.  No turn35 world is represented here.

## Candidate source-only score

Each of the 24 selected relations has four source-life continuation
histograms, grouped as two coarse cases: `(E1,E2)` and `(E3,E4)`.  For each
episode and its case parent I computed a Jeffreys-smoothed self-description
gain

`sum_r n[e,r] * log2(p[e,r] / p[parent,r])`,

where `p[x,r] = (n[x,r] + 1/2) / (sum_r n[x,r] + 7/2)`.  The two case gains are
summed and divided by `1 +` the total number of source successors.  This
information density is determined entirely by the four source lives.  Stable
ties retain the original selected-record order.

The proposed memory keeps every relation at two-case resolution, then keeps
the episode split only for the ten highest-density relations.  A same-size
control keeps the ten lowest-density relations instead.

## Retrospective replay on turn34 only

Using the frozen turn34 candidates and outer law, I replayed hybrid choices on
the already observed switched tails.  Each fine relation costs exactly 28
additional bytes over its coarse representation.

| fine relations | mean gain over coarse, bits | wins | byte price |
|---:|---:|---:|---:|
| 6 highest | 0.969840 | 3/8 | 1.68 bits |
| 8 highest | 1.779577 | 5/8 | 2.24 bits |
| **10 highest** | **2.823954** | **7/8** | **2.80 bits** |
| 12 highest | 3.377236 | 6/8 | 3.36 bits |
| 16 highest | 3.686254 | 6/8 | 4.48 bits |
| all 24 | 4.842001 | 6/8 | 6.72 bits |
| 10 lowest | 0.994133 | 6/8 | 2.80 bits |

The ten-relation result clears its retrospective price by only `0.023954`
bits.  It is therefore a useful knife-edge choice, not a victory.  Turn35 must
freeze this choice and ask new worlds; failure is fully admissible.
