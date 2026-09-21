# Turn18 WITNESS-CLOCK — Don's acceptance record and verdict

domain: arianna-method.netta.turn18-witness-clock-report/v1
2026-09-21. Builder: Opus subagent under the preregistered PROTOCOL.md (a
first builder hand was terminated by an infrastructure error before any
world byte existed; the batch was never at risk — the replacement hand
rejected the two leftover files and rebuilt from turn17 sources).
Acceptance, reruns and this record: Don's hand.

## Verdict

**Material FAIL, preserved: 17 of 20 gate conditions pass** (RESULT.json
sha256 ee3423fb…, recounted from the artifact by the accepting hand; the
independent reader computes the same 20 booleans with the same three
failures). Failed: `c2_law_vs_fast_retention` (−5.068 mean vs bar ≥ −1),
`c2_law_vs_slow_mean` (+0.194 vs bar > 1), `c3_law_vs_adaptive_mean`
(+0.241 vs bar > 1, though 8/8 worlds positive). The reader passes:
3,670,016 candidate + 2,097,152 authority forecasts, every hazard decision
of all four modes recomputed, max numeric error 1.3154e-10.

What passed matters as much as what failed: moved tails beat fast by
+7.084 mean 8/8 within −1.920 of slow (c1 whole family); recombined
retention 0.998979/0.999701 of slow with 8/8 positive (c4); and both
mechanism teeth — the clock reaches fast within 256 bytes of the law seam
in 7/8 worlds (offsets 40, 28, 21, 65, 32, 311, 120, 37 — median 36,
against turn17's 253–1097), and fast-hazard bytes stay ≤25% of moved
tails in 6/8 (c5, c6). The seam-signature hypothesis is CONFIRMED:
witnessed wrongness distinguishes a law change from a surface move, fast,
in the direction predicted.

## The finding — the debt is a level, not a rate

The clock reacts in tens of bytes and still loses the law tails. The
artifact says why, in every world identically: at the t=8192 seam the
witness (which ran slow, as it should have — slow was correct all
prefix long) stands at 11.002 bits of posterior commitment to memory;
fast stands at 4.975. The gap is 6.03 bits in all eight worlds to within
0.05 — exactly log2(2^-10/2^-16), the ratio of the two hazards. Switching
the hazard buys the descent RATE; the LEVEL already accumulated must be
walked down and paid for byte by byte (11–124 bytes of descent), and the
split accounting shows the loss lands ~2.45 bits before the reaction plus
~2.62 during the descent. Turn17 failed for lateness; turn18 removed the
lateness and exposed the deeper invariant: **a hazard clock sets the rate
at which commitment decays, never the level it has reached. No reactive
hazard switch, however fast, can close a gap that is a function of the
pre-seam hazard ratio.** This closes the whole class of
withdrawal-rate mechanisms: turn14 (fixed fast), turn16 (return after
death), turn17 (adaptive rate), turn18 (witnessed rate) — the survivors
of this arc must regulate the level.

## Acceptance performed (all rc direct)

- Artifact recount: 20 teeth both hands, same three failures; RESULT and
  VERIFY sha256 match the builder's claims (ee3423fb…, bce928ce…); all
  quantity vectors re-averaged by my hand agree with the reported means.
- FREEZE.json: 28/28 digests intact including built binaries; the C
  predictor binary is byte-identical to the turn17-pinned build
  (784d549d…) — reproducible build, no predictor drift.
- Pristine reader rerun to a fresh output: rc=0, zero differing fields
  against the sealed VERIFY.json.
- Integrity probe: one digit altered in a skeleton copy's
  results/world196/switched.tsv.gz → reader refuses by name, rc=1.
- Load-bearing code read: witness branch literal — hazard from OLD w with
  fast at w ≤ −1 (authority.c:60-61), update gated on matchedL ≥ 1 with
  decay 31/32 (:69-70), incumbent adaptive untouched (:66-67), shared
  one-way posterior law, crossing exclusion inherited.
- Builder discipline verified as claimed: pre-freeze probes turned all 20
  teeth red individually before any world existed; the leftover files of
  the terminated first hand were rejected and rebuilt from source; one
  hand-typed RAW.md mean was caught by the builder's own scripted
  re-derivation and corrected before sealing; nothing regenerated after
  `generate`.
- Worktree: only turn18/ touched, no git operations, sealed ranges unused.

## One bounded next question (a proposal, not this turn)

Regulate the level: a commitment ceiling. Cap the source posterior odds at
a fixed C bits (candidates argued from existing evidence, not scanned:
the moved seam's silent stretch bounds C from below — commitment must
survive ~tens of silent bytes without draining; the law-tail debt bounds
it from above — descent from C at 2^-10 must cost ≤1 bit vs fast). The
witness clock stays as built (its mechanism teeth passed); only the odds
update gains a ceiling. The question: does a window exist where capped
commitment keeps c1/c4 (surface retention, unchanged-life retention) while
closing c2/c3 (law-tail debt)? A FAIL that shows the window is empty would
close the level-regulation class the way this turn closed the rate class.

— Don (Fable, neo). Rotation per Oleg's word: Sol → Don → Astra → Sol.
