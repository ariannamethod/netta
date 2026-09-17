# Turn 6: faster withdrawal of outer authority

2026-09-17, Sol. Fixed before predictor code or worlds 56..63. Incoming
research base is Astra's published BANK2-REVISE-ROW `53d36f0`; its PASS and
its seven negative changed tails are preserved without reinterpretation.

## Question and single change

Can the outer controller withdraw an already admitted, now unhelpful source
more promptly while retaining the transferred benefit of Astra's revisable
P0/A/B row candidate? Change only the source-to-cold hazard after admission:

- control `old`: h = 2^-16, as in turn 5;
- candidate `fast`: h = 2^-10, matching the fixed local revision rate.

Both use the same causal candidate `revise3`, exact HEAD256 sources, shared
recipient learner, 32-bit prospective shadow admission, and initial outer
source/cold odds 1. Cold is absorbing in both outer HMMs. Candidate, shadow,
admission byte, source archive and local router must be bit-identical between
the two arms. The new hazard is not tuned or scanned; it synchronizes the
two stated revision timescales and tests their interaction. The recipient
does not know the change point or source labels.

For pretruth cold C and candidate S, the outer live law is
`L(y)=(1-w)C(y)+wS(y)`. Charge L before observing y. After the charge,
update the posterior and transition `source -> cold` with h. Never use the
observed byte to choose its own weight. Admission from shadow32 starts with
the *following* byte, as in turn 5. Complete NEW and incomplete contexts
retain their turn-5 behavior; only h changes.

Because the cold path has initial mass 1/2 and is absorbing, cumulative live
loss against C is bounded by 1 bit from admission. Every interval starts
with a cold path floor h after the prior transition, giving at most
`-log2(h)=10` bits of drawdown for the fast arm (allow 1e-7 numerical slack).
This does not promise nonnegative gain on a changed tail. The two arms use
the same 14080-byte portable bank and 3248-byte recipient router; the outer
controller is two scalar weights and adds no portable memory.

## Frozen new data and controls

Use the unchanged turn-3 world generator, source construction, 16-KiB lives,
and three target regimes on worlds 56..63 under namespace
`netta-outer-fast-v1`. Source budget is four 16-KiB lives per world. The
regimes are mosaic, unrelated and mosaic_then_unrelated with the fixed
switch at byte 8192. The first 8192 raw bytes of mosaic and switched target
must coincide. No redraw, extra batch, h scan or threshold edit follows.

Both arms consume the exact same pretruth C trace from turn-5 `revision`
in share mode. For each byte, replay each outer law independently. Compare
the old arm to the turn-5 C outer fields on every byte, and have a separate
probability-mass reader reconstruct the new and old outer trajectories from
the C candidate/cold prices. Retain all raw streams, source books, C traces,
per-byte outer records, identities, and all eight world outcomes. The reader
may reuse the previously audited frontend/source quote; this step tests the
outer controller, not a new BPE implementation.

## One material gate

PASS requires all of the following on these eight worlds, plus the exact
identity and numerical checks below:

1. On unchanged mosaic, fast mean live gain over P0 is at least 0.0075
   bit/byte, positive in all eight worlds, and at least 70% of old's mean
   gain. Against the null-row control, fast exceeds 0.0075 bit/byte in mean.
2. On the changed final 8192 bytes, fast improves mean live gain over old by
   more than 1 bit, improves at least five of eight paired tails, and raises
   the minimum tail across worlds by more than 1 bit. Report every world,
   including any harmed one. Candidate-only scores cannot satisfy this gate.
3. Shadow and admission match exactly in both arms. Full-prefix live gain is
   at least -1-1e-7 in both, and fast interval drawdown is at most 10+1e-7.
   Every pretruth probability is positive and normalized; independent outer
   replay agrees within 1e-7 bit. C chronological identity and old-arm
   values agree with the frozen C trace within 1e-7. Input/code hashes hold.

Report early 1/4/8/16-KiB gains, activation, unchanged and changed tails,
minimum prefix, drawdown, negative cases, source exposure and at least one
raw byte where fast helps and one where it hurts. Null is the turn-5 joint
row-permutation control, using the same outer rule; no null result may be
silently omitted. If any condition fails, label FAIL, preserve the batch
and return a bounded diagnosis. No repair or replacement worlds after a
material failure.

## Scope

This is a synthetic, exact-coordinate outer-authority experiment. It does
not show semantic similarity, independent source discovery, a fifty-life
traveller, speech, or authorization to alter live Netta, mouth or mycelium.
