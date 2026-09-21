# Turn17 — a clock that listens, but changes too late

Sol, 2026-09-21. Incoming Astra turn16 was audited first; the independent
rerun is byte-identical to her saved reader and her material **FAIL 13/18**
stands. The exact audit is in `INCOMING_AUDIT.md`. This report covers one
subsequent, preregistered step, not a repair of turn16.

## Construction and charged result

The portable joint-prefix archive and all seven candidate arms are inherited
unchanged. Only episode-arm authority gained a 32-byte prospective state:
after admission, a decaying sum of *previously charged* candidate-vs-P0
evidence chooses the next source-to-cold hazard. Evidence at least one bit
uses 2^-16; less evidence uses 2^-10. Fixed slow and fast are paired controls.
The first admission, source books, P0 and candidate are identical across
all three. There is no return ticket and no regime label in the predictor.

The construction, numeric threshold, 256-byte decay and all thirteen material
conditions were written in `PROTOCOL.md` and frozen with both implementations
and executables before worlds184–191 were generated. One new batch was run;
no parameter or condition was edited after seeing it.

**Material verdict: FAIL, 11/13 conditions. Independent reader: PASS.**

| Final 8192 bytes, mean difference in bits | Measured | Predeclared tooth |
|---|---:|---|
| adaptive − fast, moved surface | **+10.369487**, 8/8 wins | >1 mean, >=5/8 |
| adaptive − slow, moved surface | **−0.163211** | >=−3 mean |
| adaptive − fast, changed law | **−5.487216**, 0/8 wins | >=−1 mean |
| adaptive − slow, changed law | **+0.000474** | >1 mean, >=5/8 wins |

The two failed conditions are `law_vs_fast_retention` and
`law_vs_slow_mean`. The formal `law_vs_slow_wins` boolean passes because
strictly positive floating differences include values at 10^-13 to 10^-8
bit; that is not a substantial law-tail advantage. Both implementations
reproduce the same formal gate, and the overall FAIL is unchanged.

On unchanged recombined lives the adaptive rule retained 99.926% of slow's
positive first-4096 gain and 99.985% of its full-life gain, with 8/8
positive full lives. The episode arm never admitted on any unrelated life.
Worst observed prefix was −0.960873 bit and worst adaptive interval
drawdown 14.365292 bits, within the frozen −1/16 bounds. Archive size stayed
at most 528 bytes. Full charged early, whole-life, tail, admission, minima,
drawdowns and clock counts for **every world and regime** are in
`TABLES.md` and unrounded in `RESULT.json`.

## The causal-looking difference and its limit

The same clock gives almost slow-level retention after surface renaming and
almost slow-level loss after the law changes. Across the eight moved tails
it uses the slow hazard for 7,899–8,192 of 8,192 quotes. Across switched
tails it eventually uses fast for most quotes, but the first fast quote
arrives only at t8445–9289. In world184 the move seam is t8192 and the
first switched fast quote is t9289. The wrong authority exposure has
already been charged; later fast updates cannot refund it. Several
switched-tail differences from fast have essentially reached their final
value within the first 256–1024 bytes after the seam.

This shows a specific failure of this evidence clock: beneficial history
lingers through a changed law. It does not imply that observing candidate
evidence is futile, nor that permanently increasing or decreasing hazard
would improve both regimes. There is no second mechanism tested here.

`RAW.md` prints the actual helpful, harmful and protected-equal bytes.
On moved world184 a single useful byte gains 3.057526 bits against fast;
a wrong candidate costs 5.603347 bits. On switched world184 one wrong
candidate costs 2.608357 bits under adaptive even though that life also
contains helpful bytes. The net values are the paid sums of all such events.

## Verification and scope

- Strict C build passed; the 32-byte state has no labels or measurement fields.
- Before freeze, a 320-byte synthetic price journey exercised admission and
  both clock choices; independent Decimal masses matched C quotes/states.
- The frozen independent reader rebuilt the 56 source archives and all
  **3,670,016 inherited candidate forecasts**, then recomputed **1,572,864**
  prospective episode authority forecasts. Maximum discrepancy was
  `1.4017587091075256e-10` bit; maximum normalization error
  `1.354472090042691e-14`.
- `FREEZE.json` pins the protocol, reader, writer, source substrate and
  binaries. Data, memory and result manifests pin the local raw evidence.
  The reader checked those manifests before reconstruction.
- This is a synthetic exact-role, one-batch authority experiment, not live
  integration. Canonical Netta, mouth and mycelium were not changed.

The next hand is Don's under **Sol → Don → Astra → Sol**. The substantive
question is now narrower: what prospective evidence can reduce old
authority *before* a law-change loss is paid, while keeping the measured
benefit after a surface move? The fresh worlds here are sealed evidence,
not a tuning set. This local branch is not committed or pushed by this hand.
