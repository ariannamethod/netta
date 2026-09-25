# Turn25 — hold the withdrawal rate until positive evidence returns

2026-09-22, Astra. Synthetic byte prediction; one bounded mechanism and one
fresh batch. Branch `astra/turn25-witness-latch`, based on Don24 commit
`798085be9a2bf8b6c41860238d4f38450558e126`.

## Result

**Material FAIL: 7/8 inherited conditions. Verification and all six validity
conditions PASS.** The remaining failed condition is switched-law tail mean
relative to the fixed fast reference: **−4.945839 bits**, against a floor of
−1. The fixed hysteresis improves that tail over witness by only **+0.180753
bits** on average, and loses **0.047493 bits** over the whole switched life.
No threshold, source file, data or frozen law was changed after the result.

## Incoming audit and the design decision

The independent rerun of Don24 reconstructed 7,340,032 authority forecasts.
Its output is byte-for-byte equal to Don's VERIFY.json; W3 remains the only
failed incoming condition. All 36 freeze entries match either their original
pin or the two disclosed amended states. Reversing the recorded amendment
in memory recovers the two original hashes. See [measurement audit](audit/MEASUREMENT.md).

The numerical oracle result stands. Its interpretation needed narrowing:
128 bytes is the largest passing sampled rate delay in that batch, with a
hidden regime label. It is not a guaranteed causal detection budget. The
current-batch first-fast median is 51 bytes, not the historical 38.5. Three
of eight current first-fast offsets exceed 128.

More decisively for the next construction, every unchanged life already has
a threshold crossing before the seam, at bytes 113–581. A lifetime latch on
the first crossing would fire in all eight unchanged worlds. This is read
from old raw traces, not an evaluation of a new candidate on old outcomes.
See [mechanism audit](audit/MECHANISM.md), including the neutral-byte return
at world240 / byte8246. The report's claim that detection played no part in
the four previous failures is not established by these observations.

I selected the handoff's allowed **re-arm variant**, fixed before new data.
The new flag turns fast at updated w<=−1, turns slow at updated w>=+1, and
holds in between. Current odds select the quote; the old flag selects the
following probability transition; only then does the charged observation
update w and the next flag. Neutral observations can no longer clear a fast
flag. Existing source capital is retained. [Protocol](PROTOCOL.md),
[C implementation](latch.c), [interface](INTERFACE.md).

## Fresh measurement

Worlds **248..255**, namespace `netta-witness-latch-v1`; four source lives and
four recipient regimes per world, 16384 bytes per life. The frontend, source
selection, 528-byte archive and shadow admission are inherited unchanged.
Only the authority law changes. All numbers below are mean paired bits over
eight worlds unless labelled otherwise.

| Condition | Latch result | Required | Verdict |
| --- | ---: | ---: | --- |
| Law tail vs fast | −4.945839 | >=−1 | FAIL |
| Law whole life vs fast | +5.308592 | >=0 | PASS |
| Moved-surface tail vs fast | +6.693739 | >=1 | PASS |
| Moved-surface individual wins vs fast | 7/8 | >=5/8 | PASS |
| Moved-surface tail vs slow | −1.273317 | >=−3 | PASS |
| Intact early retention | 99.887028% | >=95% | PASS |
| Intact whole-life retention | 99.966194% | >=95% | PASS |
| Intact positive whole lives | 8/8 | 8/8 | PASS |

| World | Law tail latch−fast | Law tail latch−witness | Moved tail latch−fast |
| ---: | ---: | ---: | ---: |
| 248 | −4.305637 | +0.065287 | +2.920423 |
| 249 | −5.732337 | −0.025029 | −3.679525 |
| 250 | −4.254824 | +0.295987 | +8.941306 |
| 251 | −5.409590 | +0.078744 | +6.015529 |
| 252 | −4.240845 | +0.875063 | +11.337920 |
| 253 | −4.494082 | approximately 0 | +10.670148 |
| 254 | −5.661647 | +0.100060 | +10.385291 |
| 255 | −5.467748 | +0.055912 | +6.958817 |

Witness itself passes the same seven bars on this batch. The new flag is
therefore not an advance in the number of satisfied utility conditions.
On intact full lives it costs 0.307343 bits relative to witness; on moved
tails it costs 0.246711 bits. Those costs are retained alongside its small
law-tail benefit. The mean latch law-tail gain relative to P0 is +0.822220
bits; failing against fast does not mean every comparison is negative.

## Observable bytes: help and harm

These are actual retained second-half rows. Delta is latch log-price minus
witness log-price; positive is a cheaper encoding of the observed byte.
The complete source bytes and both traces remain under ignored `data/` and
`results/`; RESULT's raw_help/raw_harm records identify these rows.

| World / regime / byte index | Truth byte | w before | Latch before→after | Witness log2 price | Latch log2 price | Delta bits |
| --- | ---: | ---: | --- | ---: | ---: | ---: |
| 252 / law / 8610 | 101 | −2.590275 | 1→1 | −4.653992 | −4.351194 | +0.302798 |
| 248 / law / 9151 | 72 | +5.062714 | 0→0 | −3.540263 | −3.851980 | −0.311717 |
| 254 / moved / 9175 | 67 | −4.090066 | 1→1 | −3.916328 | −3.256355 | +0.659973 |
| 255 / moved / 9091 | 177 | −1.889231 | 1→0 | −2.960409 | −3.338136 | −0.377727 |

At world252/8610 the candidate is wrong relative to P0, and lower inherited
influence helps. At world255/9091 the candidate prices the observed byte at
−1.309634 versus P0's −5.281801; the reduced influence misses part of a real
benefit. That observation raises w to +2.141975 and clears the flag for the
next update. The current quote cannot retroactively regain that benefit.

## Verification and cost

- The independent Decimal reader rebuilt **2,097,152 forecasts**, all four
  authority laws, every recorded state/quote, life metrics and eight bars.
  Maximum numerical difference **1.3847056834492832e−10**; tolerance 1e−7.
- Shared admission, archive bound, prefix/interval bounds, normalization,
  exact P0 quotes and latch chronology all pass. There are no unrelated
  admissions in this batch. Across all arms, lowest prefix gain is −0.622991
  bits and largest drawdown is 14.219509 bits.
- The artificial pre-freeze fixture has 48 observations, 3 sets, 3 returns,
  26 neutral holds. The separate reader also reconstructs it; max difference
  5.33e−15. Actual writer schema, a named changed-artifact refusal, and refusal
  to overwrite an output were checked before data. No additional full run.
- Every source archive is **528 bytes**. New prediction state is **48 bytes**
  versus inherited 40: one logical flag plus C layout padding, constant work
  per observation. It does not enlarge the amount of archived experience.
- The reader starts from pinned P0/candidate/match inputs; it is not an
  independent source-book or HEAD256 implementation. Full-vector normalization
  remains the inherited C receipt. [Reader scope](READER.md).

The [saved-price diagnosis](DIAGNOSIS.md) partitions the remaining law-tail
gap: −2.029825 bits through the first fast-update byte (whose quote precedes
that update), then −2.916013 bits afterwards. It also recounts 855 neutral
holds with no release. Mean carried source/cold odds at the seam are
12.050203 bits for latch and 6.112680 for fast. These are descriptions of
observed histories, not a counterfactual attribution of each loss.

Execution and regeneration: [REPRODUCE.md](REPRODUCE.md), RUN.json,
PREFLIGHT.json, FIXTURE.json, VERIFY_RUN.json. The post-result diagnosis is
kept separately; it can inspect this evidence but cannot select another law
on worlds 248..255.

## Evidence identities

| Artifact | SHA256 |
| --- | --- |
| PROTOCOL.md | `db4d93eb8835973330cbe6e4a356b9228d230e9251b7fbc7bb7f0bb074cdf730` |
| FREEZE.json | `e656eeab8d5f17165c1488ca1792349cebc40f958acbb96b8f44dd8c5d2109fb` |
| RESULT.json | `4042928ee54d5adc3b647cab3825ec3c59ffe493be72168d50a5acdba15ab5b1` |
| VERIFY.json | `a3367a839f2e5e3c78e1adc1f84e85c1789a6b4d4e7cfa16008632ffc7c83fa3` |

## Integration and next hand

The construction remains under turns/turn25 in this isolated checkout. No
canonical Netta, mouth or mycelium files changed. No commit or push is part
of this authorization. Next recipient: **Sol**.

Oleg also asked when C performance will catch up with the measured Python
version. Don24's published diff contains only turns/turn24 and does not
implement that correction. Carry a separate, concrete task to Sol: compare
the measured C and Python paths doing the same work, identify the cause,
then take one measured C change with behavior parity. It is not an additional
experiment inside this turn, and no unmeasured speedup is claimed here.

## Publication authorization — 2026-09-23

Oleg explicitly requested commit and push after receiving this result. The
sealed turn is published on `astra/turn25-witness-latch`, including this
report, the code, manifests, audit receipts and diagnostic excerpts. Large
raw streams remain locally in the documented ignored directories and can be
regenerated with REPRODUCE.md. The material FAIL and its evidence are retained
unchanged. The shared handoff records the resulting exact commit after push.
