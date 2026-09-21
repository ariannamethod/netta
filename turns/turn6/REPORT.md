# Turn 6: external authority withdraws faster, at a measurable price

2026-09-17, Sol. **The one preregistered synthetic gate passes, and an
independent probability-mass reader passes.** This is an isolated research
result on fresh worlds 56..63, not an integration into live Netta.

## Received hand

Astra's BANK2-REVISE-ROW was independently reviewed and published on
`astra/turn5-revision` at `53d36f0`; her earlier PASS and Sol's preceding
local-cold FAIL remain intact. Her new local row router revises confidence
among P0, book A and book B. On the incoming batch, changed local advice was
often muted or amplified by a separate, one-way outer HMM. The new question
is whether its withdrawal timescale can better follow the revisable router.

The [protocol](PROTOCOL.md) was committed as `f733f6f` before predictor code
or data. One alteration was selected: outer source-to-cold hazard `2^-10`
instead of `2^-16`, matching the row revision rate. Candidate quotes, source
books, learned-unit frontend, 32-bit prospective admission, initial outer
odds, and local router are identical. The changed hazard is a design choice,
not a parameter fitted to worlds 48..55 or a sweep on this batch.

## What the fixed batch says

All numbers below are received/live gain against the local P0 in bits per
16,384-byte target life, not ungated candidate gain. The source budget is
four 16-KiB lives per world; the portable bank is 14,080 bytes and the
recipient router 3,248 bytes.

| World | Mosaic old | Mosaic fast | Changed tail old | Changed tail fast | Paired tail change |
| ---: | ---: | ---: | ---: | ---: | ---: |
| 56 | 177.047 | 156.815 | -2.630 | 2.461 | 5.091 |
| 57 | 216.058 | 194.257 | -7.603 | -1.820 | 5.783 |
| 58 | 183.496 | 162.799 | -2.379 | 2.974 | 5.353 |
| 59 | 153.749 | 143.607 | -6.242 | -0.801 | 5.442 |
| 60 | 203.432 | 182.044 | 8.429 | 11.961 | 3.532 |
| 61 | 180.100 | 159.130 | 25.484 | 25.949 | 0.465 |
| 62 | 154.249 | 136.706 | -7.551 | -1.766 | 5.785 |
| 63 | 215.536 | 204.465 | 1.053 | 5.711 | 4.658 |
| Mean | **185.458** | **167.478** | **1.070** | **5.583** | **4.514** |

The fast arm has positive mosaic gain in all eight worlds, mean 0.0102221
bit/byte, and retains 90.30% of the old arm's mosaic gain, above the
preregistered 70% minimum. The permuted-row null has zero live mosaic gain
in both outer modes. On changed tails, fast improves 8/8 paired worlds, by
4.513557 bits on average, and lifts the minimum tail by 5.782910 bits.
Every material condition in [RESULT.json](RESULT.json) is true.

The cost is substantial. On unchanged mosaics the fast arm gives up
17.980306 bits per full life on average. Early mean gains at 1/4/8/16 KiB
are respectively **28.292/70.488/130.485/167.478** for fast versus
**28.665/74.628/141.095/185.458** for old. Admission positions match in
all worlds because candidate shadow is unchanged. On switched lives, the
first-half deficit is larger than the 4.514-bit tail recovery: full-life
mean is **136.069 fast versus 142.165 old**, a 6.096-bit loss. This PASS
therefore establishes a bounded *tail* improvement under its declared
retention gate; it does not establish that fast is the better default policy
for the whole life. Three of eight fast changed tails still lose to P0.

The mean outer source exposure on changed tails falls from 0.2141 to
0.0983. On unchanged mosaic tails it falls from 0.9400 to 0.5231. This is
the intended withdrawal, and also explains the lost useful gain. No
unrelated target was admitted; its received gain is zero in both arms and
all eight worlds. The permuted null was not admitted on these worlds.

## Causality, bounds, and independent check

The pretruth candidate stream comes from Astra's unchanged C `revision`
executable in share mode, run on the new raw worlds. Turn-5's evaluator
checks every C quote and old outer state on the fresh batch. The new writer
replays old and fast outer states over the **same** candidate/cold prices.
Its old arm matches the C/turn-5 event stream on every byte. Admission
occurs only on the byte after the shadow crosses 32.

The separately written [reader](verify.py) reconstructs old and fast
states as probability masses rather than copying the log-odds update. It
checks frozen code and artifact hashes, C quote chronology, raw-byte
identity, every outer event, prospective admission and all world results.
It checked **1,572,864 outer forecasts** with maximum numeric discrepancy
`7.389644451905042e-13` bit and independently recovered the exact gate.
Fast minimum full-prefix gain is -0.837570 bit and maximum interval
drawdown is 4.747895 bits, within the declared -1/10 bounds. The reader
does not independently reconstruct BPE training or source-book prices; this
step changes only the outer controller, and those inputs are held fixed by
the audited C path.

Exact receipts: [CODE_FREEZE.json](CODE_FREEZE.json),
[ARTIFACT_MANIFESTS.json](ARTIFACT_MANIFESTS.json),
[RESULT.json](RESULT.json), [VERIFY.json](VERIFY.json),
[VERDICT.json](VERDICT.json). The previous Astra result and this result
use disjoint fixed world IDs. Raw streams, C traces and per-byte outer
records remain local and hash-pinned but are not included in git;
[README.md](README.md) gives a clean reproduction. The observations in
[RAW.md](RAW.md) include both a helpful and a harmful chosen byte.

## Decision and next hand

This single mechanism passes its own preregistered synthetic gate. It
is not ready to replace the current outer HMM: the gain on changed tails
comes with a measurable early and whole-life tax. A useful next question
for Astra is whether present evidence can govern the outer withdrawal rate
without seeing the change point, preserving more of the old early gain.
Any answer needs a new declaration and new worlds; 56..63 are sealed
evidence, not a tuning set. The broader fifty-city, functional-similarity,
language and live-Netta questions remain open.
