# Turn22 incoming mechanism audit: Don's ceiling frontier

2026-09-21, independent mechanism hand. Received clone:
`bcc1881ee78c76d97645588bfae54e950cf4ad3e`. Source code and evidence were read
only; no model, candidate, grid point or new world was run. All references
below are repository-relative. The five relevant turn21 frozen pins
(protocol, authority.h/.c, replay.c, experiment.py) match. RESULT SHA-256:
`70ea594706515ea2942291d6d29beb1b36346cbecb147fc56d69aa4d763cae4b`.

## Verdict and exact implementation

No new C defect was demonstrated. The incoming **FAIL remains FAIL**:
W2 nomination robustness and W4 unrelated admission fail. The repaired reader
incident stays part of the received record.

The proposed control `h8l4` is grid index10, declared in
`turns/turn21/authority.c:8–14`. Its order is:

1. Quote from existing odds; inactive/equal prices return cold exactly
   (`authority.c:40–48`).
2. Add the observed candidate-minus-cold price to lifetime shadow. First
   crossing32 initializes future odds and evidence to zero; the crossing
   does not advance the witness clock (`:57–67`).
3. When already active, select hazard from **old** witness evidence:
   slow iff w>-1. Apply the ordinary one-way hazard update (`:69–75`).
4. On matchedL>=1, update w=(31/32)w+delta; otherwise hold w (`:76–78`).
5. Cap the **next** odds using **updated** w: low4 when w<=-1, otherwise
   high8 (`:79–89`). Cap is a withdrawal only; raising the permitted limit
   later does not raise the odds by itself.

All27 quotes finish before observations in `replay.c:90–104`; carried odds,
first admission, witness-clock identity, and cap bounds are checked at
`:105–127`. State is still three doubles plus mode/active:32B.

For the mass interpretation, q=v*r/(1-v+v*r) is the observed posterior,
v_h=(1-h)q, and clipping restricts v_next to at most
2^cap/(1+2^cap). Every clipping transfer goes to absorbing cold. This
preserves the initial half-cold capital bound; high8 implies an interval
loss bound log2(257). No return mass is introduced.

## What the grid actually establishes

I read all24 saved grid summaries, not only the nomination. The W1 window is
exactly h8l2,h8l3,h8l4,h8l5. **Every one** has only3/8 law tails >=fast−1.
Thus none of these24 points simultaneously meets the W1 aggregate window
and W2 individual-world requirement on this batch.

However, high5 and high6 meet that *individual law-tail requirement in8/8*.
They fail other bars. Therefore the report's unrestricted “a static commitment
level cannot be robust” is stronger than these observations. The measured
result is a conflict among the declared bars within this finite family.
It does not exclude unmeasured constants, different clocks, or every static
regulation law.

| high | law-tail qualifying worlds, low2/3/4/5 | W1 window | binding tradeoff in saved means |
| --- | --- | --- | --- |
|5|8/8,8/8,8/8,8/8|none|law-whole −16.519bits vsfast; moved-tail −5.922 vsfast|
|6|8/8,8/8,8/8,8/8|none|law-whole about−3.265 vsfast; moved-tail −6.856 vsslow|
|8|3/8,3/8,3/8,3/8|all4|aggregate window passes; individual law-tail requirement fails|
|10|2/8,2/8,1/8,1/8|none|law-tail −2.385..−2.555 vsfast|
|12|0/8,1/8,1/8,1/8|none|law-tail −3.193..−3.846 vsfast|
|16|0/8,1/8,1/8,1/8|none|law-tail −3.646..−4.448 vsfast|

W1 conditions are literal at `experiment.py:407–415`. The nomination at
`:466–469` chooses greatest law-tail mean, then whole-law mean, then earlier
grid index. Saved h8l3/h8l4/h8l5 have the same first key, while h8l4/h8l5
also share the second key; index selects h8l4. The disclosed independent
reader discrepancy was in near-tie comparison. `verify_repair.py:449–474`
uses the stated TOL on each key. Its repair does not turn either failed
material condition green. h8l4 remains a nomination requiring fresh
confirmation, as the incoming protocol says.

One useful mechanical fact: **h8l4 has zero low-cap clips on all related
lives combined**, versus62618 high-cap clips. Its low4 branch did not bind
in those observations. h8l5 has the same saved law/whole means. The main
observed restraint of h8l4 is its upper8 branch, not successful extra
withdrawal at4. The zero is an empirical fact of this batch.

## Per-world support at the seam and the nomination's misses

The following prequote states at t=8192 were read from retained authority
TSVs, with the TSV digests checked against RESULTS_MANIFEST.json. Grid
odds_before is the previous row's odds_after, exactly as replay records it.
The support clock and pre-seam state are shared between the two changed
regimes because their first8192 raw bytes coincide.

|world|w before seam|h8l4 odds|fast odds|law-tail h8l4−fast|moved-tail h8l4−fast|moved-tail h8l4−slow|
|---|---:|---:|---:|---:|---:|---:|
|216|10.374354|7.983126|7.495344|−2.180489|+7.367962|−4.003144|
|217|5.933367|4.892971|4.291798|−0.125105|+7.677570|−3.726598|
|218|−0.743445|3.180783|1.018450|+2.247631|+6.716770|+1.995499|
|219|11.761616|7.983126|7.413027|−1.666642|+7.406335|−2.924808|
|220|10.687256|7.988729|8.120997|−2.587800|+6.190499|+0.749964|
|221|13.128207|7.994353|7.818764|+0.107853|+5.024086|−3.233833|
|222|7.548689|7.982516|6.367573|−1.502462|+7.510499|−3.850930|
|223|3.467091|3.283539|3.313503|−1.617527|+5.745467|+1.257883|

The five law-tail misses are216,219,220,222,223. Support at the seam is not
a simple ordering of the eventual gain:221 has the largest w and meets the
law-tail bar, while223 has modest w and fails it. This is descriptive saved
evidence, not a fitted new policy. h8l4's mean law-tail gap is−0.915568bit,
mean law-whole gap+5.680678, and moved-tail gap+6.704898 vsfast.

## Raw helpful and harmful observations with similar positive support

These are the two already published turn21 tight/loose extreme positions
for world216 switched, selected post hoc in that turn. I joined their
saved source rows to authority rows to show h8l4 on the **same** observations.
No new extreme search or candidate was run. Truth values are raw byte values.

|field|t8669|t8675|
|---|---:|---:|
|truth byte|108|81|
|rank|3|2|
|current reverse-recency heads|81,93,108|93,81,108|
|matchedL|2|4|
|w before|6.7230746320190553|6.8524837420144138|
|cold log2|−4.6200052237685316|−2.1632852720851772|
|candidate log2|−0.74200594416281085|−5.9547384890206372|
|h8l4 live log2|−2.0608652390283848|−3.4804625872586006|
|fast live log2|−3.4700520189259008|−2.5272621544264684|
|h8l4 odds before|−0.84809904048853069|0.86326434832887888|
|h8l4 odds after|3.0296984381212484|−2.9282137747083525|

The first candidate helps and additional exposure helps; the second
candidate harms and additional exposure harms. Both old support values
are positive and close. A future support-based cap must be judged by its
charged sequence, not by calling positive w a correct next prediction.

## The unrelated admission is upstream of every ceiling

Exact retained world218 unrelated rows:

```
t1529: shadow_before31.871557900367602, active0, delta0
t1530: truth219, rank2, matchedL1
       shadow_before31.871557900367602
       cold=-3.1131864854605933
       candidate=-2.5252938376966636
       delta=+0.5878926477639297
       admitted_after1, h8l4_live=cold, odds_after0, clipped0
t1531: shadow_before32.459450548131528, active1
```

All modes share this initial admission;1531 names the following active
byte, whereas1530 is the charged crossing. The cap cannot have caused or
prevented it: inactive observations use the same lifetime shadow and bypass
all caps. This is a demonstrated negative-control admission in1/8 unrelated
lives of this batch. It is not a calibrated population false-positive rate.
The admitted unrelated life ultimately has h8l4 gain+12.051639bits, so the
control violation also must not be relabelled as a demonstrated loss.

## State implications for the next bounded design

The existing reusable support state is w, a signed discounted sum over
matched observations since admission; unmatched silence does not advance
its decay. Its units are discounted log2 evidence, not a calibrated
probability or an independently calibrated confidence interval. The seam
state table gives real variation in this quantity and its separation from
the surviving odds.

A new cap derived from w can change how much authority survives while
preserving this clock and its32B state. The incoming code supplies the
necessary causal ordering: observe the priced byte, update support, cap
the following odds. If only excess mass is withdrawn and a finite maximum
cap is retained, the same cold-capital proof applies. A raised cap must
permit future earning rather than directly add odds. This identifies a
bounded implementation opportunity; this hand has not selected or run its
functional form.

Retained raw paths used above, under
`/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn21/`:
`results/authority/world216..223/switched.tsv.gz`,
`results/world216/switched.tsv.gz`, and both source/authority
`world218/unrelated.tsv.gz`. All opened trace files passed their recorded
digest. Root's independent full reader owns the complete replay; this hand
stops after this code and bounded evidence audit.
