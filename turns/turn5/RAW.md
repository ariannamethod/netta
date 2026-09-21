# Turn 5 — raw prediction witnesses

These are binary-byte observations, not natural speech. All prices and weights below are **before the displayed truth** unless explicitly marked after. Prices are log2 probabilities; a larger value assigns more probability to the observed byte. `t` is a **zero-based raw-byte index**. All four witnesses use `mosaic_then_unrelated`.

The examples were selected after the run for explanation. The first two illustrate large helpful candidate contributions with changed majority preferences; the third deliberately exposes harm. The NEW example comes from the existing [WITNESSES.json](analysis/WITNESSES.json). These choices do not alter the tested sample or its gate.

All-world aggregates: [RESULT_READING.md](analysis/RESULT_READING.md). Exact decomposition and selection context: [MECHANISM_RESULT.md](analysis/MECHANISM_RESULT.md). Complete extracted records and source SHA-256 values: [WITNESSES_SELECTED.json](analysis/WITNESSES_SELECTED.json). The original WITNESSES.json is unchanged.

## 1. An old cold preference is reconsidered

World **52**, `t=8865`, pattern `012340`, observed byte **204 / 0xCC**, group **1**.

### Exact past context

Each comma-separated hex string is one already-observed unit expansion. HEAD256 forms its row from the first byte of each of these six expansions; unit IDs refer only to this current local inventory.

| Trace field | Saved value |
| --- | --- |
| `unit_hex` | `88,cc,5050,29,d2,88` |
| `unit_ids` | `136,204,400,41,210,136` |
| `heads` | `136,204,80,41,210` |
| `context_len` | `6` |
| `trained_bytes` | `8192` |
| `nunits` | `471` |

### Pretruth quote

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Pbase log2 price | -11.093519280739097 | -11.093519280739097 |
| P0 log2 price | -3.630621002218998 | -3.630621002218998 |
| P2_A log2 price | -4.198830956994712 | -4.198830956994712 |
| P2_B log2 price | -2.8581652718439283 | -2.8581652718439283 |
| Candidate log2 price | -3.6974831886791844 | -2.986869170787056 |
| Live log2 price | -3.67401196230767 | -2.989136591315555 |
| Cold weight | 0.5154535766327366 | 0.09889493573678457 |
| A weight | 0.37576010564998813 | 0.0732818437792689 |
| B weight | 0.10878631771727529 | 0.8278232204839465 |
| Outer log2 source/cold odds | 0.9199708425804162 | 7.834207476847273 |
| Outer source weight, derived from saved odds | 0.65422774652467974 | 0.9956371563161438 |

The static row puts its largest mass on cold; revision instead gives B most of the weight. B prices the actual byte better than P0. The received advantage is **+0.6848753709921152 bits**.

Sources: [past unit trace](traces/world52/mosaic_then_unrelated.tsv.gz), [event prices and states](results/events/world52/mosaic_then_unrelated.tsv.gz), both at `t=8865`. The event file contains one row per arm per raw byte; select `arm=static3` and `arm=revise3`.

## 2. A previously excluded source becomes useful

World **54**, `t=8540`, pattern `012023`, observed byte **232 / 0xE8**, group **0**.

### Exact past context

Each comma-separated hex string is one already-observed unit expansion. HEAD256 forms its row from the first byte of each of these six expansions; unit IDs refer only to this current local inventory.

| Trace field | Saved value |
| --- | --- |
| `unit_hex` | `e8,3d,43,e8,43,75` |
| `unit_ids` | `232,61,67,232,67,117` |
| `heads` | `232,61,67,117` |
| `context_len` | `6` |
| `trained_bytes` | `8192` |
| `nunits` | `447` |

### Pretruth quote

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Pbase log2 price | -12.173741590688902 | -12.173741590688902 |
| P0 log2 price | -4.7950651185604665 | -4.7950651185604665 |
| P2_A log2 price | -4.925334123533319 | -4.925334123533319 |
| P2_B log2 price | -3.6874139877497587 | -3.6874139877497587 |
| Candidate log2 price | -4.919294855216865 | -3.839522233082505 |
| Live log2 price | -4.914136061776117 | -3.8433185710890463 |
| Cold weight | 0.04437077273410363 | 0.008908509923354947 |
| A weight | 0.9556278014911782 | 0.16543113184841146 |
| B weight | 1.4257747180677702e-06 | 0.8256603582282335 |
| Outer log2 source/cold odds | 4.591167454404236 | 7.518120235685295 |
| Outer source weight, derived from saved odds | 0.96016482997268371 | 0.99457427820673916 |

The static row gives A about 95.6% and nearly excludes B. Revision gives B about 82.6%; B predicts this byte better than P0. The received advantage is **+1.0708174906870704 bits**. This is source-to-source reconsideration, without claiming a cold-majority decision.

Sources: [past unit trace](traces/world54/mosaic_then_unrelated.tsv.gz), [event prices and states](results/events/world54/mosaic_then_unrelated.tsv.gz), both at `t=8540`. The event file contains one row per arm per raw byte; select `arm=static3` and `arm=revise3`.

## 3. Remaining harm: a better candidate still receives too much influence

World **53**, `t=10593`, pattern `000123`, observed byte **143 / 0x8F**, group **1**.

### Exact past context

Each comma-separated hex string is one already-observed unit expansion. HEAD256 forms its row from the first byte of each of these six expansions; unit IDs refer only to this current local inventory.

| Trace field | Saved value |
| --- | --- |
| `unit_hex` | `de,de,de,8f,82,8c` |
| `unit_ids` | `222,222,222,143,130,140` |
| `heads` | `222,143,130,140` |
| `context_len` | `6` |
| `trained_bytes` | `10240` |
| `nunits` | `477` |

### Pretruth quote

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Pbase log2 price | -10.761721718828541 | -10.761721718828541 |
| P0 log2 price | -6.880919663406624 | -6.880919663406624 |
| P2_A log2 price | -8.777522031062503 | -8.777522031062503 |
| P2_B log2 price | -2.851390024128369 | -2.851390024128369 |
| Candidate log2 price | -8.54879760788255 | -8.461168503542856 |
| Live log2 price | -6.880919663406656 | -7.991606657422421 |
| Cold weight | 0.06308210529891424 | 0.06468476586247539 |
| A weight | 0.9369178345817453 | 0.9341611633962528 |
| B weight | 6.011934040432587e-08 | 0.0011540707412719773 |
| Outer log2 source/cold odds | -44.83293138730674 | 2.0612814570645135 |
| Outer source weight, derived from saved odds | 3.1911179647432959e-14 | 0.80670981819849852 |

Both arms still put most row weight on A, which predicts this byte poorly. Revision slightly improves the candidate, but its outer source weight is much larger. The received difference is **−1.110686994015765 bits**. This is the largest harmful paired live event in the eight changed tails and remains part of the evidence.

Sources: [past unit trace](traces/world53/mosaic_then_unrelated.tsv.gz), [event prices and states](results/events/world53/mosaic_then_unrelated.tsv.gz), both at `t=10593`. The event file contains one row per arm per raw byte; select `arm=static3` and `arm=revise3`.

## 4. NEW has no selection evidence, but the row clock still ticks

World **48**, `t=8192`, pattern `012234`, observed byte **46 / 0x2E**, group **5**.

### Exact past context

Each comma-separated hex string is one already-observed unit expansion. HEAD256 forms its row from the first byte of each of these six expansions; unit IDs refer only to this current local inventory.

| Trace field | Saved value |
| --- | --- |
| `unit_hex` | `b6,95,eeee,ee,8a,9696` |
| `unit_ids` | `182,149,360,238,138,256` |
| `heads` | `182,149,238,138,150` |
| `context_len` | `6` |
| `trained_bytes` | `8192` |
| `nunits` | `486` |

### Pretruth quote

| Quantity | static3 | revise3 |
| --- | ---: | ---: |
| Pbase log2 price | -10.856408810265062 | -10.856408810265062 |
| P0 log2 price | -11.550510280020013 | -11.550510280020013 |
| P2_A log2 price | -11.550510280020013 | -11.550510280020013 |
| P2_B log2 price | -11.550510280020013 | -11.550510280020013 |
| Candidate log2 price | -11.550510280020013 | -11.550510280020013 |
| Live log2 price | -11.550510280020013 | -11.550510280020013 |
| Cold weight | 0.19292927015596237 | 0.19392766872544084 |
| A weight | 1.57439045093724e-07 | 0.015152210247701489 |
| B weight | 0.8070705724049924 | 0.7909201210268575 |
| Outer log2 source/cold odds | 10.55443269977633 | 10.47880943463751 |
| Outer source weight, derived from saved odds | 0.99933547629888475 | 0.99929973920032278 |

Byte 0x2E is absent from the five named heads, so its outcome is NEW. P0, A, B, both candidates and both live predictions have exactly the same saved price. The likelihood update is zero. Static ratios remain unchanged; revision applies its declared after-observation clock toward the prior. The learner is not told that this byte is also the generator’s switch boundary.

| Row log2 ratio | static3 before | static3 after | revise3 before | revise3 after |
| --- | ---: | ---: | ---: | ---: |
| log2(wA/w0) | -20.224847231466498 | -20.224847231466498 | -3.677918495763072 | -3.638671910820135 |
| log2(wB/w0) | 2.0646227944632836 | 2.0646227944632836 | 2.028013341723792 | 2.0278844954752966 |

The transition is `(1−2^-10) * posterior + 2^-10 * (1/8, 7/16, 7/16)`. It follows pricing on every complete-row occurrence, including NEW; it is not evidence from NEW.

Sources: [past unit trace](traces/world48/mosaic_then_unrelated.tsv.gz), [event prices and states](results/events/world48/mosaic_then_unrelated.tsv.gz), both at `t=8192`. The event file contains one row per arm per raw byte; select `arm=static3` and `arm=revise3`.

