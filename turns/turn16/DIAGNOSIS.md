# Turn16: post-hoc authority diagnosis

2026-09-21. This is a descriptive reading of the completed fixed batch, not
another experiment or an amendment to its gate. Only saved `episode` quotes
and authority states from `moved_mid` were aggregated. No alternative policy
was run. [DIAGNOSIS.json](DIAGNOSIS.json) contains input hashes, exact counts,
per-world partitions and raw examples. Trace `t` is zero-based; byte numbers
below are one-based. The analyzed tail is bytes 8193 through 16384 inclusive.

The saved [RESULT.json](RESULT.json) has material **FAIL, 13/18**. Four failed
conditions concern surface recovery; the independent `null_admissions` failure
is an additional failure and is not explained by this authority decomposition.
The independent reader subsequently completed arithmetic verification with
PASS; that does not change the failed material result.
The return-minus-budget surface improvement is 0.964787357 bits in mean,
coming entirely from world178's +7.718298855 bits. Return remains 7.604889161
bits behind slow on the mean surface tail. Slow exceeds ordinary fast by
8.569676518 bits in mean.

## An early spent return does not explain the entire surface gap

All minima and counts below refer to **prequote fast odds after the seam**.

| World | Minimum fast log odds | Bytes with fast odds <= -32 | Return byte | Slow minus fast tail, bits |
|---|---:|---:|---:|---:|
| 176 | -29.689874 | 0 | 1596, before seam | 5.152096 |
| 177 | -62.718377 | 536 | 1094, before seam | 11.154091 |
| 178 | -46.346384 | 486 | 9453, after seam | 5.193044 |
| 179 | -102.587781 | 4351 | 1131, before seam | 5.699815 |
| 180 | -26.865954 | 0 | none | 11.319534 |
| 181 | -15.210066 | 0 | none | 11.370937 |
| 182 | -10.802941 | 0 | none | 11.316679 |
| 183 | -28.937189 | 0 | 1132, before seam | 7.351216 |

Five worlds never reach the collapse threshold in the tail. In particular,
180–182 retain an unused return throughout the whole life, yet slow earns
approximately 11.3 more tail bits in each. Their failure cannot be explained
by having spent a return before the seam. Worlds177 and179 do combine deep
post-seam debt with an early spent return; the saved experiment does not show
what a second return would have done there.

Giving additional return credits alone, while retaining the same collapse
trigger, would not engage in the five tails that never reach it. This follows
from the unchanged observed path up to any first additional intervention; it
does not estimate gains for a different trigger or a second actual return.

The recorded return controller arms before the seam at bytes947/613/875/586
in worlds176/177/179/183 respectively. Those attempts end in the early returns
listed above. After the seam only world178 arms, at byte9017, then returns at
9453. Its counter is armed for 436 subsequent quotes. There are **zero natural
recovery disarm events** in these eight return-controller traces, before or
after the seam. This refers to the explicit `armed -> unarmed, unused` event;
it does not say that ordinary fast authority never naturally rises again.

## Exact price identity: withdrawal charge plus endpoints

For each already-active slow/fast controller with source weight w and hazard h,

    w_after = (1-h) w_before S(y)/Q(y)
    log2 Q(y) = log2 S(y) + log2(1-h)
                + log2 w_before - log2 w_after.

Every analyzed tail quote is active, and the two modes share S exactly. Thus
their observed 8192-byte price difference is exactly

    slow - fast = 8192 log2[(1-h_slow)/(1-h_fast)]
                  + log2(w_slow_start/w_fast_start)
                  - log2(w_slow_end/w_fast_end).

The mean decomposition in bits is:

| Term | Bits |
|---|---:|
| Fixed survival-factor difference | +11.366861270 |
| Starting source-weight ratio | +0.027966400 |
| Negative ending source-weight ratio | -2.825151152 |
| Sum = observed mean slow minus fast | **+8.569676518** |

Maximum absolute per-world reconstruction error is below 3e-12 bits. The end
weight is obtained algebraically from the final recorded quote and ordinary
after-truth update; no counterfactual quote was produced. In worlds180–182
both policies end with substantial source authority, so the endpoint correction
is small and the surviving charge is almost the full 11.3669 bits.

This identity explains why a single rescue does not in general remove the
cost of continuing to withdraw source mass. It is not an ablation proving
that deleting hazard would help. The price of hazard includes protection
against bad candidate observations; changing it also changes the endpoints
and every intervening weight.

## Where the observed difference is paid

The following are **sums over all eight tails**, 65,536 quotes. Help/harm/equal
use the exact sign of the saved `candidate - cold` log price. The partitions
are descriptive; the realized outcome is unavailable at quote time.

| Observed candidate outcome | Quotes | Slow minus fast, bits |
|---|---:|---:|
| Helps relative to C | 23,555 | +2347.895974 |
| Harms relative to C | 13,829 | -2279.338562 |
| Equal to C | 28,152 | 0 |
| Total | 65,536 | **+68.557412** |

| Prequote fast source mass | Quotes | Slow minus fast, bits |
|---|---:|---:|
| < 0.5 | 23,812 | +621.277229 |
| 0.5 through < 0.9 | 8,369 | -248.305522 |
| >= 0.9 | 33,355 | -304.414294 |

For the requested inclusive cut, fast mass >=0.5 covers 41,724 quotes and has
slow-minus-fast **-552.719817 bits**. Slow's net advantage is therefore not
concentrated on already-confident fast predictions. It comes from the low-fast-
weight group: there slow gains +1812.798585 more bits on helpful observations,
while losing 1191.521356 more bits on harmful observations. Fast's better
protection on harmful observations nearly cancels slow's helpful-event gain.

Equal-price observations have zero immediate score difference. The identity's
constant hazard term on such a byte is canceled by its weight-ratio change;
one cannot label all equal observations a directly measured tax, or conclude
that omitting those updates alone would retain the observed benefit.

## Raw prices showing both sides

All numbers are log2 probabilities of the realized byte, from the saved traces.

| World, byte | C | S | Fast quote | Slow quote | Fast / slow source mass |
|---|---:|---:|---:|---:|---:|
| 181,10747: useful candidate, low fast authority | -5.778379 | -2.951084 | -5.447223 | -3.310120 | 0.042316 / 0.743553 |
| 181,15420: candidate surprise, high authority | -5.223246 | -14.969758 | -9.666838 | -14.288099 | 0.955156 / 0.999296 |

The first observation gives slow +2.137103 bits over fast; return is available
but unarmed. The second gives fast +4.621261 bits over slow. Raising source
authority blindly would expose more of the latter loss as well as the former
gain. See `results/authority/world181/moved_mid.tsv.gz`, arm `episode`, trace
t10746 and t15419.

The actual world178 return occurs on byte9453: counter before truth
29.459854641 bits, new candidate evidence +3.270756819 bits, old source odds
-12.757189522. The trigger byte's fast and return quote are both -4.302840983;
only the following byte receives reset odds -1.271553303. Its +7.718299 tail
advantage over the same-prior budget control is a measured effect of the
already-run intervention. At byte11879 in that same world, fast and return
have again reached odds -46.346384205 and quote virtually C. One successful
return did not create permanent authority or prevent later debt.

## One next question for Sol

Can the ongoing withdrawal of authority depend on prospective predictive
evidence, so that a still-useful candidate can rebuild its share after ordinary
setbacks while preserving fast protection against candidate surprises? The
question concerns the current authority dynamics, beyond a single rescue
from extreme debt. These traces identify the competing costs; they do not
choose an evidence clock, parameter, repeat-return policy or new gate. A new
mechanism would require a separate preregistered step on new data.
