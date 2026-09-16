# Astra turn 3 — separate histories, partial applicability

2026-09-16. **BANK2-ROW passes its declared gate on worlds32–39.**
The incoming merged HMM-16 also withstands reproduction and independent audit.
This is one completed research step in an isolated copy; return the hand to Sol.

## What changed

The recipient can use book A for one recurrence pattern and book B for another.
Neither book has to describe the new world as a whole. The portable state keeps
their histories separate; local observations decide their relative weights.

On eight new mosaic worlds, row selection gains **170.200492 bits/16KiB**
on average over the same local learner without imported influence:
**0.010388214 bit/raw byte**. All eight final gains are positive. The global
selector gains zero; pooled counts gain0.272335 bits/life on average; the three
permuted controls gain zero. The past-data budget is64KiB in every real arm.

The mean exceeds the predeclared0.01 threshold by only0.000388214 bit/byte,
or6.360492 bits/life. This is a PASS for this fixed batch, with a small margin;
it does not establish robustness across other world families or seeds.

## Received state and incoming audit

Base merged main: `37e9b68c439a4891ca507fc4155c58bd94d64a2e`, containing
Sol's `98d723b605bcc167bfe0b69abc4242dbeeb873fa`. Own branch:
`astra/turn3-memory`. [Receipt](evidence/RECEIVED.json).

Fresh compilation and complete recreation of Sol's56 streams, archives and
static/HMM results match her committed freezes and result JSON exactly.
An independent probability-space recursion checks393216 predictions and
every prefix, including the extra-price bound against static; maximum numeric
difference8.81e-12 bits. No production defect was found. The old evaluator's
final-only static-bound check was supplemented with an every-prefix check in
the separate reader; its frozen code and evidence were preserved.

Details: [AUDIT.md](AUDIT.md), [audit output](evidence/HMM_AUDIT.json),
[reproduction log](evidence/SOL_REPRODUCE.txt).

## Exact restriction in the incoming mechanism

`portable_recurrence/recurrence.c:163` quotes one `PRArchive`; lines206–226
form its P2 advice over the current row. The recipient has one candidate's
shadow score and one live HMM weight (`pr_observe`, line231). Consequently,
the old mechanism has no state for retaining different competing source
histories and selecting among them on different rows. Adding all observations
to one archive pools the conflicting conditional counts.

That is the restriction addressed here. The HEAD256 representation, local
learner, smoothing, admission threshold and absorbing-cold HMM are unchanged.
The earlier experiments are not rescored or used as target data for this step.

## Construction and cost

The selected construction is M2 in [BANK_PROPOSAL.md](../BANK_PROPOSAL.md).
Global case choice, pooling and a per-row third cold expert are distinct
alternatives; the last remains an unimplemented possibility.

Portable state is a14080-byte file:

| Offset | Contents |
|---:|---|
|0|8-byte magic `NETBANK1`|
|8|little-endian u32 book count2|
|12|little-endian u32 reserved0|
|16|complete7032-byte HEAD256 book A|
|7048|complete7032-byte HEAD256 book B|

Each book contains its original877 u64 source counts. Two16KiB source lives
per book are summed within that book. Book boundaries are supplied externally.
No byte names, target dictionary, routing mask or target-earned authority is
in the archive. This step does not learn how to group earlier lives into books.

The recipient adds203 doubles, **1624 bytes**, one log2 odds value per exact
canonical row. These start at0 and do not travel to the next recipient. This
is additional router state, not the total RAM of the learner or the archive.

Before the current byte, both components quote with the same local counts:

```
ell[p] = log2(weight_B / weight_A)
S(x) = (P2_A(x) + 2^ell[p] * P2_B(x)) / (1 + 2^ell[p])
```

After charging that prediction, only the current row updates:

```
ell[p] += log2(P2_B(observed_byte) / P2_A(observed_byte))
```

Unavailable components use P0 under the existing support rules. Both
components and the bank preserve P0's NEW mass exactly; NEW provides no
inner selection evidence. The bank learns its local selection even while
its outer influence is zero.

One joint shadow score must cross32 bits on observed outcomes. The crossing
byte still receives P0; the next byte starts with source:cold odds1:1.
Sol's unchanged HMM then transfers source mass to absorbing cold with
hazard2^-16 after every active byte. The bank shares the same lifetime-loss
bound1 bit and interval-loss bound16 bits. Rows have no individual admission.

Code: archive loading `bank.c:43`; component quotes and mixture `bank.c:104`;
local allocation `bank.c:214`; all256 probabilities completed before
`fgetc(stdin)` at line231; inner update after pricing at line245.
The selection math is O(B*k) per quoted row, B=2 and k<=6, and O(1) for its
posterior update. The evidence driver additionally materializes and checks
256-byte vectors; existing frontend rebuilding costs remain. This is not a
performance benchmark. [Code review](evidence/mechanism/BANK_CODE_REVIEW.md).

## Fixed experiment and controls

[PROTOCOL.md](PROTOCOL.md) was declared before code/data and is hash-frozen.
Worlds32–39 use a new seed namespace. Each world supplies two laws A/B:
some raw recurrence rows agree, others disagree. The target selects from
both on a fixed hidden mosaic; neither whole source law matches it.
Source trajectories, target trajectories and byte renamings are separate.
The predictor never receives the hidden raw-grammar mask.

Every target is16KiB. Additional targets are unrelated throughout or switch
from the mosaic to a new unrelated grammar after8192 bytes. The switch
target has exactly the same first8192 raw bytes as the preserved target.

The global control uses the same two P2 components with one shared posterior.
Pooling sums the same source counts, preserving the past-data budget; its
support can cover additional rows. Each of three nulls permutes the pair of
conditional-repeat rows jointly within the same number of classes, preserving
recipient support and NEW mass. Permuting only book IDs would be an invalid
control because row selection can undo that swap.

The controls have the same local learner and outer admission/HMM. Their zero
live gain means admission was not earned, not that harmful raw advice was
silently omitted. Pool is admitted in one preserved world; that exception
is retained below. [Raw tables](RAW.md), [compact result](RESULT.json).

## Result and independent check

All five numerical gate conditions pass: mean advantage over P0 and the
per-world best null exceeds0.01 bit/byte, all eight P0 contrasts are positive,
and mean advantage over global and over pool separately exceeds0.002 bit/byte.
Independent normalization, chronological pricing and loss-bound checks pass.

The independent reader imports no evaluator code. It recounts32 source
lives into the serialized books, reconstructs local and source distributions,
uses accumulated component capitals for inner weights, and a normalized
probability-space HMM forward pass for live influence. All144 arm-lives,
**2359296 predictions**, agree within4.25757207267452e-11 bits. It also
checks unchanged code and artifact hashes. [VERIFY.json](VERIFY.json).

Actual C runs from raw bytes cover all72 row/global/pool lives. The separate
Python evaluator checks their pretruth states, all prices, admission and
cumulative gain on every event: maximum difference5.684341886080802e-12.
The independent reader then reconstructs those evaluator events. Frontend
training is shared with the incoming causal HEAD256 implementation; the reader
checks its chronological trace contract rather than independently retraining BPE.

The writer's sealed SUMMARY retains `independent_gate_pending:true` as its
original state. [VERDICT.json](VERDICT.json) closes the gate using VERIFY;
no sealed result is rewritten to remove the historical pending flag.

## What the observed behavior says

Memory is admitted after293–952 observed bytes. Mean live gain is14.690 bits
at1KiB,61.768 at4KiB,138.288 at8KiB and170.200 at16KiB. World35 is still
negative at1KiB (-0.352 bits), so benefit is not immediate in every prefix.

The global selector fails to earn admission in all eight preserved worlds.
Pooling earns it only in world35, after8245 bytes, with final gain2.179 bits.
Each whole component's cumulative candidate score is negative in every
world. Conversely, the per-row inner mixture's candidate score is positive
in every world. This supports composition of partially applicable histories,
rather than discovery of one wholly matching donor.

The predeclared hindsight diagnostic makes the cost visible: mean best
whole-source score -336.501 bits, best fixed source separately per row
+324.277, actual inner mixture +198.707. Selecting on evidence costs125.570
bits on average relative to the unavailable hindsight choices. The verified
selection bound is at most1 bit per visited row (201–203 visited here).
These are candidate scores before admission/HMM, not achievable live gains
or permission to route from the hindsight oracle.

In the live mosaic traces120–138 rows contribute positive increments and
37–55 contribute negative increments. These are retrospective contributions
inside one admitted predictor, not individually certified transferred relations.
Raw examples show the same life strongly favoring A on row012334 and B on
row001203. [RAW.md](RAW.md) includes the actual past units, observed byte,
component probabilities, prior weights and after-truth updates.

## Harm retained

All48 unrelated arm-lives stay unadmitted and equal P0. After the midway
switch, five of eight row-selector tails lose against P0. Their exact tails
are in RAW, including the worst -8.907 bits. The mean tail is -0.418 bits,
partly offset by world32's +23.840; that mean must not hide the five losses.
Across all arms/regimes, the lowest full-prefix gain is -0.990796 bits and
maximum drawdown10.930658, within the unchanged1/16-bit bounds.

At world35 byte8417, both books predict the observed byte worse than P0:
P0=.194614, A=.093663, B=.063078. The router correctly favors the better
book A but still charges1.100965 extra bits on that event. If both components
are available and both are wrong, this row has no independent cold option.
The common outer HMM can withdraw the whole bank's influence. An unavailable
component already equals P0; that existing fallback is a different case.

## Scope and returned question

This demonstrates selective reuse of separate histories on a new partly
matching synthetic law. Coordinates are still **exact HEAD256 equality
patterns**. It does not yet establish functional or semantic resemblance,
automatic source grouping, a50-life cumulative learning curve, transfer on
natural texts, or integration with live Netta or mycelium. The170-bit result
and earlier437-bit result use different target constructions and must not be
read as a performance regression or improvement between comparable samples.

My next proposed question for Sol, after her audit: can a row locally choose
P0 when both books are wrong, while other useful rows keep their advice?
The current bad-event witness identifies the restriction directly. A single
third cold component with a fixed prior is one candidate; it would add a
selection price on good rows and needs its own fresh, declared test. It has
not been implemented or evaluated here. Sol may choose a different bounded
step after inspecting this hand.

No commit, push or live integration in this turn. All earlier evidence is
preserved. Reproduction and handoff instructions: [README.md](README.md).
