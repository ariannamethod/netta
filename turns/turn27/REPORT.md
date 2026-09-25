# Turn27 CUSUM-LATCH — Don's acceptance record and verdict

domain: arianna-method.netta.turn27-cusum-latch-report/v1
2026-09-25. Builder: Opus subagent under the preregistered PROTOCOL.md.
Acceptance, the reader ruling, reruns and this record: Don's hand.

## Verdict

**Material FAIL: C1, C3, C6 fail; C2, C4, C5, C7 pass; C8 resolved by a
separate repair reader** (below). Both hands agree on every tooth:
writer's RESULT.json and the repair reader's VERIFY.json (3,145,728
forecasts, max numeric error 1.195e-10, all nine hygiene subtests true;
pristine red probe refused by name with the full path, rc=1). Freeze
39/39 intact. gate_pass = false.

## The finding — and my own refuted premise, named first

The protocol's a-priori argument was mine and it was wrong. It priced
intact excursions from the complete-prefix received-gain floor (−1 by
the mixture law) — a BOUNDED quantity — and concluded 8 bits of net
guilt was unreachable. But the per-byte witness delta is UNBOUNDED
below: world 263 pays −4.13/−3.94/−4.21 within twelve voting bytes on an
intact life, carrying S from 1.55 to 8.18. With measured drift −0.72..
−0.79 per voting byte and σ ≈ 1.0–1.1 over ~10^4 voting bytes, the
Lindley running maximum lands at ~6.9 bits — and the intact per-life S
maxima (6.80–13.07) bracket the 8-bit threshold from both sides. **A
never-forgetting CUSUM on an unbounded-influence statistic crosses any
fixed threshold, given a life long enough.** The conflation of a bounded
aggregate with an unbounded per-byte statistic is the exact error class
of "attribution is a claim": I derived a bound for the wrong variable.

Consequences measured, not argued: 13 false latches out of 24
non-switched lives (8/8 moved, 5/8 recombined; the three intact
survivors peaked within 1.1 bits of the threshold); three of the
switched "detections" fired BEFORE the seam at the same byte as their
intact twins (prefix trajectories verified bit-identical across regimes
— they are false alarms pointing the right way); genuine post-seam
detection within the deadline: 2 of 8.

## What still moved forward

- **C5 passed:** cusum's law tail (−2.9402) beats the hysteresis
  incumbent (−4.9883) by +2.0481 — accumulation beats forgetting even
  while both lose; the statistic's family is right, its influence is not
  bounded.
- **O-rate-0 reconfirmed the bar-set on fresh worlds** (all eight bars;
  law-tail −0.1612, law-whole +10.81): the game remains winnable.
- **A new tension with turn24's budget framing, measured:** latch
  latency does not predict outcome — world 261 latched at seam+93 and
  finished +0.133 OVER fast (the retained stock repaid, turn24's W3
  holding); world 259 latched at seam+102 and finished −5.876. Nine
  bytes apart, opposite fates: what differs is whether the post-latch
  tail retains structure that can repay the stock. The tail deficit
  saturates near 5.9 bits (the stock burning off) between seam and
  latch. Settling this needs O-rate-32/128 arms on fresh worlds; this
  turn froze only O-rate-0 and cannot.

## Ruling on the reader defect (C8)

The frozen verify.py ran to completion — schema, all 39 pins, 5
manifests, labels re-derived, all 32 lives at 1e-7, agreement with the
writer on all seven gated verdicts and the c1..c6 detail blocks — then
refused with KeyError 'c7': its rebuilt details dict builds c1..c6 while
the comparison loop iterates every gated prefix. The builder correctly
refused to repair it: RESULT.json already existed, and wall (b) of the
turn24 amendment ruling forbids amending frozen code after a result.

My ruling: the amendment walls govern FROZEN files and are not invoked.
The lawful path is turn20's precedent — a SEPARATE, disclosed repair
reader. verify_repair.py (sha256 be63f644…) differs from the frozen
verify.py by a documentation header and exactly one entry,
c7=dict(hygiene) — the same hygiene dict the frozen reader had already
compared and gated under its own key. The frozen file is untouched; its
refusal (VERIFY.rc, VERIFY.stderr) remains part of the record. Standing
clarification appended to the turn24 ruling: after a result exists,
frozen code is never amended — a separate repair artifact with its own
sha and the original refusal retained is the only door, and it may
change no law, no threshold and no verdict. Here it changed none: every
tooth identical between hands.

## Acceptance performed (all rc direct)

- FREEZE 39/39, drift NONE; teeth recounted from RESULT.json; writer's
  test_details carries c1..c8 (the defect was reader-side only).
- verify_repair.py written by my hand, compiled, run: rc=0, 3,145,728
  forecasts, max error 1.195e-10, full agreement.
- Red probe: one digit in results/world260/switched.tsv.gz → refusal by
  name with full path, rc=1.
- Builder's preflight accepted: every tooth reddened individually with
  zero collateral, both C6 failure modes exercised, 30/30 schema keys
  refused individually, C-vs-Decimal handcrafted-tape agreement 4.17e-14.
- Equivalence accepted as stated: hysteresis bitwise vs a replay REBUILT
  from turn25's pinned sources (its binaries are gitignored — recorded
  without overclaim); orate0 bitwise vs turn24's retained, sha-matched
  binary; shared arms 0 mismatches over three full lives.

## One bounded next question (a proposal, not this turn)

Bounded-influence guilt. The statistic family is right (C5); its
per-byte influence is not. Clip each voting byte's contribution before
the same accumulation law: S_next = max(0, S + min(c, −delta) − k) with
c fixed a priori from THIS turn's published tape statistics (σ ≈ 1.0–1.1,
drift −0.72..−0.79): c = 2 bits caps any single byte at 1.5 net, pushes
the intact Lindley maximum well below 8, and leaves a law tail's
sustained guilt (mean gap ~8.6 bits over the stretch) able to reach the
threshold within the budget. Same latch, same h and k, one new constant
— declared, not scanned. Secondary, if a hand wants it: O-rate-32/128
arms on fresh worlds to settle the repay-structure question turn27
opened.

— Don (Fable, neo). Rotation per Oleg's word: Sol → Don → Astra. Worlds
256..263 sealed.
