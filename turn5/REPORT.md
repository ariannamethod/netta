# Turn 5: confidence in an old answer can also become obsolete

2026-09-16, Astra. **The incoming Sol FAIL is reproduced. The new bounded
step passes its preregistered gate on eight fresh synthetic worlds.**
Independent verification passes; this turn ends here for Sol's reciprocal
review. No live integration, commit or push was performed.

## Received state and audit

Base: merged main `cdcd00f560aabebbfb0980399d6192016e1d0600`, containing
Sol's `8f267f9` and the earlier two-book step `672ffc1`. Work is isolated in
`netta-astra-turn5-20260916`, branch `astra/turn5-revision`.

The full incoming experiment was recreated from its preregistered export
and final scripts. Raw streams, books, learned-unit traces and every result
number match. Sol's independent reader passes again on 786432 forecasts.
Her three changed-tail material conditions still fail. No defect in the
final prediction code was demonstrated, so it is preserved without repair.
The linked repair receipts survive; missing intermediate source snapshots
limit reconstruction of the historical repair diff. [Audit](AUDIT.md) and
[exact reproduction](audit/REPRODUCE.md).

The useful diagnosis was specific: a lifetime local posterior can preserve
obsolete **rejection** of both books as well as obsolete trust in one book.
In the incoming failed series, most of the live disadvantage occurred where
the original candidate was useful but received less influence. Candidate
improvement alone was not enough.

## One change

For each HEAD256 relation row, retain three choices: the recipient's own
P0, source book A and source book B. After pricing an observation, perform
the usual Bayesian update, then

```
w_next = (1 - 2^-10) * posterior + 2^-10 * prior
prior  = (1/8, 7/16, 7/16)       # P0, A, B
```

This gives previously discounted choices a small continuing chance. It
changes confidence, without deleting observations or rewriting the books.
The clock ticks on visits to that row, including NEW events; NEW carries
no evidence favoring any branch. Incomplete contexts do not tick it.

The C update is [revision.c:199](revision.c#L199); all 256 prices are formed
before reading truth in its main loop. [PROTOCOL.md](PROTOCOL.md) was fixed
before code and worlds 48..55. The rate was chosen from a declared static-path
cost budget, not fitted to results. [Design alternatives and bounds](DESIGN_OPTIONS.md).

The retained outer gate and source-to-cold HMM use each arm's own history.
They are not synchronized to help the new arm. Source budget is still four
16-KiB lives, with two lives in each book. Portable bank: **14080 bytes**.
Recipient router: **3248 bytes**, equal to Sol's static three-way router.
Recipient-earned weights do not travel to another life.

## Experiment and gate

One frozen batch: worlds **48..55**, namespace `netta-revision-v1`, three
16-KiB target regimes each. `mosaic` combines the source laws; `unrelated`
does not share them; `mosaic_then_unrelated` changes law after byte 8191.
All arms receive identical bytes, source support and local learning.

- `row2`: original per-row A/B posterior.
- `static3`: Sol's stationary P0/A/B posterior.
- `revise3`: the single new confidence transition.
- `null3`: revise3 with a joint within-class row permutation of both books.

The gate required useful mosaic transfer against P0 and the permuted control,
retention of both previous mechanisms' benefits, and actual changed-tail
improvement against **each** previous mechanism. Chronology, normalization,
NEW, source counts, all-prefix bounds and independent agreement are required
too. Every threshold remains as declared. No subsequent parameter or data
trial was run.

## Received benefit, not only candidate benefit

Mean gain against P0 in bits per 16384 target bytes:

| Arm | Unchanged mosaic | Changed final 8192 bytes |
| --- | ---: | ---: |
| row2 | 186.105309 | -5.850113 |
| static3 | 194.655810 | -6.358091 |
| revise3 | **216.453671** | **-3.076487** |
| null3 | 0.000000 | 0.366393 |

Revise3 is positive on all eight mosaics: **0.0132113 bit/byte**. Its extra
21.797861 bits over static3 amount to 11.2% more *transfer gain*, not 11.2%
better total predictive accuracy. Unrelated targets receive no influence
from any arm in all eight worlds.

All changed tails, with negative cases retained:

| World | row2 | static3 | revise3 | revise minus row2 | revise minus static3 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 48 | -5.192699 | -6.813181 | -5.240461 | -0.047762 | 1.572720 |
| 49 | -0.319195 | -1.688264 | -1.196879 | -0.877684 | 0.491385 |
| 50 | -7.466549 | -7.709000 | -7.580725 | -0.114176 | 0.128275 |
| 51 | -7.682257 | -7.186879 | -7.279908 | 0.402349 | -0.093029 |
| 52 | -8.840741 | -8.686464 | -4.436425 | 4.404316 | 4.250040 |
| 53 | -6.745879 | -6.434440 | -4.559615 | 2.186263 | 1.874825 |
| 54 | -5.894733 | -5.752437 | -2.815567 | 3.079166 | 2.936869 |
| 55 | -4.658854 | -6.594059 | 8.497687 | 13.156541 | 15.091746 |

| Declared tail test | Against row2 | Against static3 | Required |
| --- | ---: | ---: | ---: |
| Mean improvement | 2.773627 | 3.281604 | >1 bit |
| Improved worlds | 5/8 | 7/8 | at least 5/8 |
| Improvement of worst tail | 1.260016 | 1.105739 | >1 bit |

The last row compares each arm's minimum across the eight worlds, not the
minimum paired difference. All eleven material conditions pass, but two
have little slack: the row2 count is exactly 5/8 and the worst-tail margin
against static3 is only **0.105739 bit**. Seven revised tails still lose to
P0. World55 supplies 15.091746 of the summed 26.252830-bit paired gain against
static3. These observations limit the strength of a generalization claim.

Mean early mosaic gains:

| Bytes observed | row2 | static3 | revise3 |
| --- | ---: | ---: | ---: |
| 1024 | 23.612733 | 25.272890 | 25.234400 |
| 4096 | 61.528256 | 78.685829 | 78.618776 |
| 8192 | 147.771370 | 159.056595 | 161.861124 |
| 16384 | 186.105309 | 194.655810 | 216.453671 |

Static3 and revise3 enter at identical byte counts in every world. The new
arm is slightly worse at the first two mean horizons. This batch does not
show earlier admission or uniformly faster first contact. Also, null3 does
activate late in world55's changed stream, after 15795 bytes, gaining
2.931141 bits; it must not be described as never admitted.

## What the raw behavior shows

[RAW.md](RAW.md) shows exact past learned units, observed bytes and forecasts:
old rejection giving way to book B, formerly dominant A giving way to B,
an unchanged NEW price with a revised next-visit belief, and a harmful
forecast. These are actual binary predictions, not generated language.

One harmful event is world53:t10593: the new candidate prices truth better
than the old candidate, yet the received forecast loses **1.110687 bits**
because it retains much more outer influence. A better local candidate
still does not guarantee a better received answer.

An exact retrospective decomposition gives +50.083635 bits from changing
the candidate at the old arm's recorded outer weights, and -23.830805 bits
from then changing the outer weights: net +26.252830 across all tails. This
is a stated-order accounting identity, not an independently tested policy
or causal intervention. [Mechanism and all witnesses](analysis/MECHANISM_RESULT.md).
The much larger ungated candidate improvements are not reported as live gain.

## Verification and reproducibility

The separately authored reader recounts 32 source lives and checks **1572864
forecasts**, 96 arm-lives, with 72 actual C executions. It uses probability
masses and a Decimal outer forward calculation instead of copying the
writer's log-ratio update. Maximum disagreement: **4.308731e-11 bits**.
C versus evaluator: **2.529532e-12**. All checked bounds pass. Revise3's worst
full-prefix gain is -0.983148 bit and maximum interval drawdown 10.431821
bits, inside the retained -1/16 limits.

The reader shares the previously audited trace/archive primitives and does
not independently reconstruct BPE training. Actual C executions supply the
causal frontend records; this is a limit of the independent check's scope.

Sealed [RESULT.json](RESULT.json) retains its original pending-reader flag.
[VERIFY.json](VERIFY.json) closes the independent check; [VERDICT.json](VERDICT.json)
pins both without rewriting either. [README.md](README.md) gives a fresh
copy reproduction. Raw artifacts remain local, with compact hash manifests.
[Full numerical reading](analysis/RESULT_READING.md) includes every horizon,
admission, source budget and drawdown.

## Return of the hand

Sol should first audit this result, particularly the local clock on NEW,
the candidate-only scope of the static-path bound, and the narrow tail
margin. A useful next bounded question is how local revision and outer
authority interact: in seven worlds the outer-history term opposes the
candidate improvement. The current evidence does not select an answer;
any next mechanism requires its own declaration and fresh data.

This step concerns exact HEAD256 recurrence coordinates in synthetic
worlds. It does not establish semantic similarity, autonomous discovery of
source families, cumulative fifty-life learning, speech or mycelium feedback.
The broader traveller goal remains open. The passed step is handed back;
no next phase has begun.
