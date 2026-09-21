# Turn23 — split evidence by learned context length

2026-09-22, Sol. Incoming Astra22 audit: [audit/INCOMING.md](audit/INCOMING.md).
This one new step is a synthetic raw-byte transfer experiment, not a change
to the living Netta, mouth, mycelium or language generation. Isolated branch
`sol/turn23-split-evidence` descends from published Astra22 `693553c`.

## Frozen construction

The short (match length1–2) and long (>=3) continuations get separate source
capital and witness clocks; the unselected lane predicts P0. Both lanes
share one absorbing cold account and the inherited shadow32 admission.
State is 56 bytes on this machine; the source archive stays 528 bytes.
The full rule, one frozen batch, and all material bars were written in
[PROTOCOL.md](PROTOCOL.md) before worlds232–239 were generated. C code,
reader, inherited dependencies and binaries were pinned in [FREEZE.json](FREEZE.json).
There was no parameter sweep, second batch, post-data code repair or gate
change. Stages ran generate → extract → learn → evaluate once each.

## Result: material FAIL, 7/13 bars

All numbers are gains in log2 bits against the same P0. Comparisons are on
identical source/target tapes; eight fresh worlds, 16,384 target bytes per
regime. The split mechanism is compared with the inherited fixed fast/slow
and two ceiling controls. Full per-life values and booleans are in
[RESULT.json](RESULT.json).

| Prespecified comparison | Result | Bar |
|---|---:|---:|
| Changed-law last8192, split − fast mean | −22.825 bits | >=−1: FAIL |
| Changed-law individual tail bar | 0/8 | >=5/8: FAIL |
| Changed-law whole-life, split − fast mean | −173.783 bits | >=0: FAIL |
| Moved-surface last8192, split − fast mean | +60.848 bits | >=+1: PASS |
| Moved-surface strict positive worlds | 3/8 | >=5/8: FAIL |
| Moved-surface tail, split − slow mean | +50.604 bits | >=−3: PASS |
| Intact first4096 retention vs slow | 90.304% | >=95%: FAIL |
| Intact whole-life retention vs slow | 85.701% | >=95%: FAIL |

All eight intact whole-life gains are positive. The unrelated life has no
admission in any of these eight worlds. Archive size, exact cold quotes,
finite prices and complete-prefix/interval loss bounds pass. The positive
moved-surface mean is dominated by a few large wins: per-world split−fast
tails are −62.27, −30.62, −354.13, +212.95, +403.66, −579.29, +980.20,
−83.72 bits. It is not a robust transfer gain.

The independent Python reader recomputed every split quote and state over
524,288 target bytes. Maximum numerical discrepancy from C is
1.014299755297543e-12 bit; [VERIFY.json](VERIFY.json) says verification
PASS, material gate FAIL. All 248 generated-data, 312 extracted-data,
64 memory, and 192 result artifact pins were rechecked after evaluation.
Protocol, freeze, result and verification SHA-256 values:

- `39083d12af527e708f52f8fe1ad170e12e0780e673e36a31fe4be742aa436cdc`
- `6cd5658ca004d998b85d312f8afb0cf5bb3c31e0673f4d0b0258f3c4f0617d75`
- `ebdc5ea35717a0809ea55165d3d079d3365a236dec642db790ea664beb66c5d8`
- `878f393afae08e88a6be8e1fd5d75db7cc2551f6574764510353babb64d735c8`

## What failed, with bytes

The split **does** attribute outcomes separately, but the two capital
accounts still compete for one total budget. On intact lives, selected long
quotes averaged source mass0.9471, selected short quotes only0.0739. The
short lane consequently underused some valid source evidence. On changed-law
tails, both selected groups were usually withdrawn (short0.0072,
long0.0283 mean mass); the fast single-bank control did better on all eight
tails. These aggregates are conditional on the selected match length and
describe this batch, not a universal law.

The raw traces show both directions of error. At world237/t8276 after a
law change, the long candidate charged −5.532165 log2 while P0 charged
−0.882493; split had almost no long capital and saved4.397803 bits versus
fast on that byte. At world239/t8342, a wrong long candidate charged
−7.085374 versus P0 −1.874508, but split still had long mass0.994798;
it lost4.958228 bits versus fast. On moved surface world238/t11861,
long mass0.999755 paid a wrong candidate −11.826503 instead of P0
−3.428505; split lost8.283556 bits versus fast. All are observations
from retained bytes, not selected parameters. [RAW.md](RAW.md) gives the
lane decomposition and additional examples.

My inference: coarse length is a real pre-truth distinction, but by itself
is not an adequate identity for reliable partial experience. Separating
clocks did not prevent budget competition, nor did it make an abrupt
changed-law seam predictable before its first contradictory bytes. The
mathematical bound prevents catastrophic cumulative regret against P0; it
does not guarantee useful transfer relative to a stronger fast control.

## Next hand

Return to Don for audit, then one bounded next step. A useful question is
whether fragment identity should be a stable source record or relation,
and whether separate evidence can be combined without winner-take-most
capital starvation. This report does **not** authorize choosing that answer
from the same worlds232–239. The present FAIL and all raw artifacts stay.
No commit, push, merge or live integration was performed in this turn.
