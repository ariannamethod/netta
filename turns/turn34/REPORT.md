# Turn34 report: four episodes select their ground, and do not earn their bytes

Don, 2026-10-04. Protocol frozen before implementation at `dc55922`
(sha256 `74fac529…`); base `1fda545` with turn33 and its counter-audit.
Worlds 304–311, namespace `netta-earned-selection-v1`, five arms, one batch,
one independent replay. **Material verdict: FAIL retained, by G4.** The
independent reader completed: 3,276,800 forecasts, max error `3.37e-11`,
G5 true (READER.md).

## The question and the answer

Turn33 proved two coarse case histories can earn reversible residual
influence. Turn34 asked the smallest compositional question on the same
ground: when the same four source lives are remembered as four separate
episodes — paying their own measured 1,056 portable bytes against the
pooled 528 and the two-case 912 — does per-episode selection pay for the
finer split? The answer on these worlds: **selection works, the price is
not met.**

## Gates (margins bound to measured deltas: m4 = 10.56, m42 = 6.72)

| gate | claim | number | verdict |
|---|---|---|---|
| G1 | selection pays after partial change vs pooled | tail mean +87.336578 at 8/8, whole +110.807315 at 8/8 | **PASS** |
| G2 | silent start preserves ordinary ground | retention 1.007881 early / 1.020641 whole; sel4 positive 8/8 | **PASS** |
| G3 | correspondence, not generic state | vs permuted4: recombined +98.082846 at 8/8, partial tail +99.271192 at 8/8 | **PASS** |
| G4 | composition pays its price over coarseness | switched tail vs earned2: mean **+4.842001** against m42 = 6.72, wins 6/8 | **FAIL** |
| G5 | causal and numeric validity, independent reader | VERIFY.json: verification_pass, all validity true | PASS |

Per-world G4 deltas: +6.305, −0.594, +9.632, +1.814, +3.063, −0.006,
+10.702, +7.820. The sign is right in six of eight worlds and the mean is
positive — the finer bank genuinely tracks the switch better — but the
0.01 bits-per-byte law prices the 672 extra bytes at 6.72 bits, and
+4.842 does not cover it. The FAIL is retained as declared; no threshold
moves.

## What the selection actually did

Both coarse and fine banks beat pooled on switched ground (sel4
+26.462627 tail at 6/8; earned2 +21.620626 derived) — the composition
question was decided in the last 4.8 bits, not in whether episodes help.
Selection is real and concentrated: across regimes, 53–78% of
sel4-vs-pooled divergent quotes carry ≥90% of their excess wealth in a
single episode (TABLES.md, selection row), and episode activity orders
exactly as the regime geometry predicts — episodes 3 and 4 dominate on
partial ground (38,126/40,042 active quotes against 18,946/19,105 for
episodes 1 and 2). The mechanism selects; it is the pricing that the
split fails.

## Disclosures

- The protocol's byte expectation (3×384 = 1,152) missed: the four-book
  record shares one 4-byte address, so the measured delta is 1,056 in all
  eight worlds. The binding rule was the measurement, declared before data.
- The first freeze missed one reader-dependency pin and the reader refused
  without a receipt; the one-line repair and the retained defective batch
  are in BUILD.md and FREEZE1_FAILURE.md. The second batch reproduced the
  first number for number.
- Unrelated admissions: 15 arm-lives opened under the shared pooled shadow,
  disclosed in RESULT.json as inherited.
- This remains the synthetic role/emitter family: no semantic similarity,
  no natural language, no live use. The organ connects to nothing.

## Standing for the next hand

The honest frontier after this FAIL: either the price law is right and
finer episodes should only be kept where their selection signal is dense
(a conditional-granularity question), or the switched regime at 16 KiB is
too short for the finer bank to amortize. Both are measurable without
touching this freeze. Rotation: Astra's counter-audit of this record, by
Oleg's routing. No commit, push or merge without Oleg's separate word.
