# Turn22 numerical tables

One preregistered batch, worlds 224–231. Gains are charged bits relative to the same local P0. No data-dependent selection among policies. Raw values are in RESULT.json; the table is a rendering.

## Mean gains

| Regime | Mode | First 4096 bytes | Full 16384 bytes | Last 8192 bytes |
| --- | --- | ---: | ---: | ---: |
| recombined | slow | 668.127493 | 3473.271745 | 1956.953242 |
| recombined | fast | 662.648577 | 3451.093675 | 1945.888317 |
| recombined | witness | 667.411860 | 3472.180607 | 1956.752787 |
| recombined | h8l4 | 665.863731 | 3463.810553 | 1952.045686 |
| recombined | selfnorm | 660.907137 | 3442.999952 | 1942.138426 |
| switched | slow | 668.127493 | 1524.742477 | 8.423975 |
| switched | fast | 662.648577 | 1519.143253 | 13.937895 |
| switched | witness | 667.411860 | 1523.988333 | 8.560513 |
| switched | h8l4 | 665.863731 | 1524.764959 | 13.000093 |
| switched | selfnorm | 660.907137 | 1514.807318 | 13.945792 |
| moved_mid | slow | 668.127493 | 2380.160073 | 863.841570 |
| moved_mid | fast | 662.648577 | 2362.734239 | 857.528881 |
| moved_mid | witness | 667.411860 | 2378.117692 | 862.689872 |
| moved_mid | h8l4 | 665.863731 | 2374.862253 | 863.097387 |
| moved_mid | selfnorm | 660.907137 | 2351.264205 | 850.402679 |
| unrelated | slow | 1.660837 | 1.660837 | 0.000000 |
| unrelated | fast | 2.270690 | 2.270690 | 0.000000 |
| unrelated | witness | 1.702442 | 1.702442 | 0.000000 |
| unrelated | h8l4 | 2.085032 | 2.085032 | 0.000000 |
| unrelated | selfnorm | 2.413762 | 2.413762 | 0.000000 |

## Every world, paired gains

| World | Law tail: new − fast | Law tail: new − h8l4 | Law whole: new − h8l4 | Moved tail: new − fast |
| ---: | ---: | ---: | ---: | ---: |
| 224 | +0.155460 | +1.008740 | -9.920171 | -14.147247 |
| 225 | +2.065075 | +3.994249 | -4.595881 | -3.625925 |
| 226 | +1.077107 | +2.484857 | -13.078840 | -14.955742 |
| 227 | -1.103282 | +0.366333 | -5.243998 | -2.843705 |
| 228 | -0.287549 | -0.229638 | -16.564414 | -11.816830 |
| 229 | -0.242569 | -1.020030 | -16.390030 | +0.924004 |
| 230 | +0.490323 | +2.415242 | -6.460775 | -6.149857 |
| 231 | -2.091386 | -1.454160 | -7.407019 | -4.394316 |

## Actual cap use on intact lives

| World | Prequote cap min | Prequote cap max | Cap at seam | Signed w at seam | Absolute a at seam | Clips |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 224 | 0.000000 | 14.215278 | 8.411043 | 12.666543 | 23.095072 | 10951 |
| 225 | 0.000000 | 14.804428 | 6.837292 | 10.461874 | 23.481910 | 6641 |
| 226 | 0.000000 | 14.062746 | 6.758914 | 9.070495 | 20.472077 | 10070 |
| 227 | 0.000000 | 13.558550 | 8.533090 | 13.069414 | 23.505850 | 7706 |
| 228 | 0.000000 | 13.794075 | 4.486347 | 6.579636 | 22.465455 | 10051 |
| 229 | 0.000000 | 11.523713 | 0.194529 | 0.263289 | 20.655522 | 5311 |
| 230 | 0.000000 | 14.188781 | 7.820383 | 12.735026 | 25.055042 | 6811 |
| 231 | 0.000000 | 13.820695 | 8.181833 | 12.115962 | 22.693393 | 7642 |

## All material conditions

| Condition | Result |
| --- | --- |
| s12_admission | PASS |
| s13_unrelated | FAIL |
| s14_archive | PASS |
| s15_bounds | PASS |
| s16_normalized | PASS |
| s17_exact_quotes | PASS |
| s18_support_cap | PASS |
| u01_law_tail | PASS |
| u02_law_robust | PASS |
| u03_law_whole | FAIL |
| u04_moved_mean | FAIL |
| u05_moved_wins | FAIL |
| u06_moved_slow | FAIL |
| u07_early_retention | PASS |
| u08_full_retention | PASS |
| u09_intact_positive | PASS |
| u10_law_vs_static | FAIL |
| u11_whole_vs_static | FAIL |

**Material result: 11/18 conditions pass; overall FAIL.**
