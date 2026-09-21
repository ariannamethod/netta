# Turn21 CEILING-FRONTIER — Don's acceptance record and verdict

domain: arianna-method.netta.turn21-ceiling-frontier-report/v1
2026-09-21. Builder: Opus subagent under the preregistered PROTOCOL.md.
Acceptance, reruns and this record: Don's hand.

## Verdict

**Material FAIL, preserved: 8 of 10 gate families pass** (RESULT.json
70ea5947…; the repair reader recomputes the same ten booleans —
3,670,016 candidate + 14,155,776 authority forecasts across 27 modes, max
numeric error 2.514e-10; pristine rerun by my hand, zero differing
fields). Failed: `w2_nominated_point_robust`, `w4_unrelated_never_admits`.

**W1 passed: the window is not empty.** It is exactly the high=8 column,
all four points. Below, the surface dies (high=5 moved-tail −5.92 against
a +1 bar); above, the law dies (high≥10 law-tail −2.38 or worse). Eight
bits is simultaneously low enough to descend from and high enough to
survive the silent stretch of a surface seam — but the law-tail clearance
of the whole column is 0.084 bit in mean.

**W2 failed, and this is the turn's finding.** The nominated h8l4 clears
every mean bar (law-tail-vs-fast −0.9156, law-whole +5.68, moved-tail
+6.70 at 8/8, retention 0.9964/0.9972) and holds the law-tail comparison
in only 3 of 8 worlds: two worlds carry the mean, five sit at −1.5..−2.6.
**A static commitment level cannot be robust because the level a world
needs is a property of that world.** The static-level class is hereby
characterized: nonempty window in the mean, unwearable in the individual
life. The next mechanism must set the level from the life itself — a
ceiling the recent witnessed testimony can currently re-earn, not a
constant.

**W3 passed 4/4 on both teeth** — the surface bends exactly as the
mechanism claims (law-tail endpoints high=5 ≥ high=16 by +4.4..+5.2 per
row; moved-tail the reverse by +6.9..+12.5; seam level monotone in the
cap: 3.43 → 11.52 against 11.67 uncapped).

**W4 failed on a draw, not a mechanism: the arc's first false-positive
admission.** World 218's unrelated life crosses the prospective shadow
gate at 32.4595 (31.8716 + 0.5879, activation t=1531), identically in all
27 modes, produced by turn13's frozen candidate law before any turn21
code runs. Across roughly nine prior batches no unrelated life ever
admitted; shadow-32 now has a measured nonzero false-positive rate. The
tooth did its job.

## Reader incident #2, disclosed and accepted

The frozen verify.py refused with `AssertionError: nominated point` after
reconstructing all 32 lives (VERIFY.log, retained). Cause: h8l3/h8l4/h8l5
tie bitwise in the writer, and the two hands compute the same tail by two
valid formulas that agree to 4.263e-14 bit but not bitwise; the frozen
tie test used exact float equality. The repair (verify_repair.py, 48-line
diff, all in one documented selector) treats means within the gate's own
tolerance as tied, earlier grid index keeping the nomination. No verdict
changed. **The protocol defect is mine:** the preregistered lexicographic
rule declared no tie tolerance. Standing law from this incident: any
selection rule in a protocol must declare its tie tolerance the way gate
comparisons declare theirs. The builder's phrasing deserves the log: "a
nomination decided on the fourteenth digit is not a nomination."

## Acceptance performed (all rc direct)

- Both hands recounted: same ten booleans, same two failures; RESULT and
  VERIFY sha256 recorded (70ea5947…, 5cdf4b03…).
- FREEZE 30/30 digests intact; the C predictor binary again 784d549d… —
  the inherited build has now reproduced across turns 14/15/20/21.
- Pristine repair-reader rerun: rc=0, zero differing fields.
- Frozen-reader refusal retained in VERIFY.log and read; selector diff
  read in full (48 lines, one function, documented).
- Integrity probe: one digit in a skeleton copy's
  results/world219/moved_mid.tsv.gz → refusal by name, rc=1.
- Builder probes accepted as flagged: h5l5 and h10l5 bitwise-equal to
  turn20's own binary over 96,000 fields (the incumbents really are grid
  points); 16 gate mutations red, 4 green counter-probes; 19/19 schema
  keys refuse by name (the turn20 lesson, now standing); a value
  perturbation with a patched manifest still refused by column name.
- Departures accepted as flagged in BUILD_NOTES.md, including the
  grid-index third tie-break key the protocol did not name (it fired; see
  the incident above).

## One bounded next question (a proposal, not this turn)

A self-normalizing ceiling: cap the source odds not at a constant but at
what recent witnessed testimony currently re-earns — commitment may never
exceed re-derivable support. One fixed functional form declared before
data (e.g., cap_t = min(high, c · max(0, w_support window sum))), both
seam kinds in one batch, the static h8l4 replayed as the incumbent. The
static class says the needed level varies by world; the question is
whether the world's own witnesses can set it.

— Don (Fable, neo). Rotation as Oleg routes it; worlds 216..223 sealed.
