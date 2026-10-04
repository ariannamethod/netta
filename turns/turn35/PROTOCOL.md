# Turn35 protocol: conditional episode granularity

Frozen before implementation, fixture output, generated data, or target
evaluation.  Date: 2026-10-04.  Author hand: Sol.

## Question

Can Netta carry four distinct histories only where source-side evidence says
their distinction matters, while retaining a two-case memory everywhere else?
The target is not compression alone.  A useful memory must pay for its extra
portable bytes on unseen worlds and must choose the useful locations without
looking at those worlds.

## Parent and authority

- Parent scientific object: Don turn34 at
  `3c780af57b11e716d8d7c127917f52302618a959`.
- Parent protocol freeze: `dc55922`.
- Turn34 accepted result: honest material FAIL at G4; see
  `audit/AUDIT34.md`.
- Oleg explicitly routed Don's published hand to Sol for this round.  This is
  recorded as a route override, not silently rewritten history.
- Canonical Netta and `/Users/ataeff/arianna` remain read-only.  This turn is
  isolated in a Codex-owned checkout and does not alter live Netta.

## Frozen construction

Worlds are `312..319`.  The five target regimes, horizon, source lives,
HEAD256 binding, local learner, pooled address selection, common admission,
outer law, fixed-share rate, loss/drawdown bounds, and generation chronology
are inherited unchanged from turn34.

There are 24 pooled selected relations.  Every memory stores the two coarse
case histograms `(E1+E2, E3+E4)`.  Exactly ten relations additionally store
enough counts to reconstruct the four individual episodes.  A fine relation
adds E1 and E3; E2 and E4 are recovered by exact subtraction from their coarse
parents.  Thus it costs 14 unsigned 16-bit counters, exactly **28 bytes**, with
no per-record index: a frozen bit in the existing two-byte reserved field says
whether the following ordered residual block exists.

The ten fine relations are selected before recipient data by the source-only
score frozen in `DIAGNOSIS34.md`:

1. For each episode histogram and its paired coarse parent, use the
   Jeffreys-smoothed seven-way probabilities
   `p[x,r]=(n[x,r]+1/2)/(sum n[x]+7/2)`.
2. Sum `n[e,r] log2(p[e,r]/p[parent,r])` over ranks and all four episodes.
3. Divide by `1 +` the total source successor count at that relation.
4. Sort descending; break exact ties by original record index; keep ten.

The equal-byte `sparse10` control uses the ten lowest scores, with stable ties
by original index.  Neither choice may inspect target bytes, forecasts,
wealth, or outcomes.

## Arms

Six arms share the exact pooled admission event and the inherited outer law:

- `dense10`: conditional archive, ten highest-density fine relations;
- `sparse10`: equal-size archive, ten lowest-density fine relations;
- `sel4`: all 24 relations fine, the turn34 mechanism;
- `earned2`: all 24 relations coarse, the turn33/34 control;
- `pooled`: no source-role distinction;
- `cold`: local learner only.

At a fine relation, a hybrid arm uses four separately earned episode wealths.
At a coarse relation, it uses two separately earned case wealths.  A
nonpositive residual has no voice, exactly as in the parent mechanisms.  No
new global selector, target-trained parameter, or target-side routing signal
is permitted.

Expected portable archive sizes are `pooled=528`, `earned2=912`,
`dense10=sparse10=1192`, and `sel4=1584` bytes when the inherited rule/record
counts remain 32/24.  The material price is `0.01 bit` per additional byte:
`m10=2.80` bits over `earned2`; full fine price remains `m24=6.72` bits.
Actual sizes, including any deviation, are measured and priced from the
written archives rather than assumed.

## One batch and preregistered gates

One batch of eight new worlds is generated once after code, reader, fixture,
and freeze are complete.  It is retained regardless of outcome.  All means
below are arithmetic means across the eight worlds; a win is a strictly
positive per-world difference.

Let `D = dense10 - earned2`, `S = dense10 - sparse10`, and
`F = sel4 - earned2`.

- **G1 — paid conditional memory.** On the switched tail (`t>=8192`), mean D
  is greater than the measured `m10`, D wins at least 5/8 worlds, and the
  whole switched-life mean D is nonnegative.
- **G2 — source-only location.** On the switched tail, mean S is strictly
  positive and S wins at least 5/8 worlds.  Dense and sparse archives must
  have identical byte size.
- **G3 — economical retention.** On the switched tail, mean F is positive and
  mean D retains at least half of mean F while using no more than 10/24 of the
  full fine-over-coarse bytes.
- **G4 — ordinary-ground retention.** The recombined `dense10` arm is positive
  in all 8 worlds; its mean early and full gains are each at least 99% of the
  corresponding `earned2` mean.  On the partial tail, mean D is nonnegative.
- **G5 — independent causality and integrity.** A reader sharing no turn35
  writer or C code must rebuild source counts, scores, selections, every
  archive byte, every forecast/state transition, metrics and gates; all
  normalizations and inherited loss/drawdown bounds hold; common admission
  and shared-prefix identities hold; the source-only chronology is proved;
  unrelated admissions are disclosed.

Material PASS requires G1..G5.  The writer cannot award G5.  No threshold,
score, K, archive, gate, or code changes after the first target result.

## Fixture and provenance

Before fresh worlds, an independent Decimal fixture must exercise both a fine
and a coarse relation: first visit, silence, positive and negative residuals,
recovery, locality, NEW, no-match, exact common events, and shared permission.
The staged turn34 fixture must remain byte-identical to the parent.  Strict C
builds use `-Wall -Wextra -Wpedantic -Werror`.  Frozen sources, binaries,
protocol and inherited dependencies are hashed before generation.

## Publication rule

The complete turn, including FAILs, diagnosis, raw measurements, manifests,
reader receipt, audit and handoff, stays under `turns/turn35/`.  No commit,
push, merge, or live integration occurs without Oleg's separate instruction.
