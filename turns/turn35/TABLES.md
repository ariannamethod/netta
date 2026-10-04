# Turn35 tables

All values are independently replayed log2 probability gains. Means are over
worlds 312–319. `tail` means `t>=8192`; wins are strict positive per-world
differences.

## Portable state and price

| arm | bytes | extra over earned2 | priced margin |
|---|---:|---:|---:|
| cold | 0 | — | — |
| pooled | 528 | — | — |
| earned2 | 912 | 0 | 0.00 bits |
| dense10 | 1,192 | 280 | 2.80 bits |
| sparse10 | 1,192 | 280 | 2.80 bits |
| sel4 | 1,584 | 672 | 6.72 bits |

## Gate-bearing switched comparisons

| comparison | whole mean | whole wins | tail mean | tail wins |
|---|---:|---:|---:|---:|
| dense10 − earned2 | +1.345177 | 5/8 | +0.473800 | 6/8 |
| dense10 − sparse10 | +1.208798 | 5/8 | +0.194460 | 5/8 |
| sel4 − earned2 | +1.874318 | 5/8 | +1.258645 | 5/8 |
| dense10 − pooled | +62.901011 | 8/8 | +17.522573 | 8/8 |

## Mean arm gains by regime

| regime | arm | early4096 | whole16384 | tail8192+ |
|---|---|---:|---:|---:|
| recombined | dense10 | 663.854044 | 3453.084546 | 1952.355123 |
| recombined | earned2 | 663.242107 | 3448.006809 | 1948.148762 |
| recombined | sparse10 | 662.867930 | 3448.160373 | 1948.445288 |
| recombined | sel4 | 663.414308 | 3453.388903 | 1952.915183 |
| recombined | pooled | 646.116338 | 3344.148514 | 1888.797528 |
| partial | dense10 | 663.854044 | 2190.157839 | 689.428415 |
| partial | earned2 | 663.242107 | 2187.101915 | 687.243868 |
| partial | sparse10 | 662.867930 | 2188.879465 | 689.164380 |
| partial | sel4 | 663.414308 | 2193.640441 | 693.166721 |
| partial | pooled | 646.116338 | 2100.763647 | 645.412661 |
| switched | dense10 | 663.854044 | 1601.574941 | 100.845518 |
| switched | earned2 | 663.242107 | 1600.229764 | 100.371718 |
| switched | sparse10 | 662.867930 | 1600.366143 | 100.651058 |
| switched | sel4 | 663.414308 | 1602.104082 | 101.630363 |
| switched | pooled | 646.116338 | 1538.673931 | 83.322945 |
| moved_mid | dense10 | 663.854044 | 2634.620118 | 1133.890694 |
| moved_mid | earned2 | 663.242107 | 2627.172877 | 1127.314830 |
| moved_mid | sparse10 | 662.867930 | 2631.684725 | 1131.969639 |
| moved_mid | sel4 | 663.414308 | 2640.890335 | 1140.416616 |
| moved_mid | pooled | 646.116338 | 2548.015250 | 1092.664264 |
| unrelated | dense10 | 28.006681 | 82.222183 | 33.235480 |
| unrelated | earned2 | 27.366778 | 81.609878 | 33.421630 |
| unrelated | sparse10 | 27.350793 | 81.691828 | 33.605382 |
| unrelated | sel4 | 27.865900 | 82.204310 | 33.398343 |
| unrelated | pooled | 25.626807 | 71.553470 | 28.044284 |

## Per-world switched-tail deltas

| world | dense10−earned2 | dense10−sparse10 | sel4−earned2 |
|---:|---:|---:|---:|
| 312 | +3.889224 | +3.783553 | +7.384116 |
| 313 | +0.001932 | +0.002036 | +0.000128 |
| 314 | −2.482817 | −1.724847 | −3.624227 |
| 315 | −0.123574 | −0.131247 | −0.109949 |
| 316 | +1.580344 | −0.489738 | +3.261778 |
| 317 | +0.061574 | +0.087561 | −0.159593 |
| 318 | +0.006497 | +0.006500 | +0.008068 |
| 319 | +0.857223 | +0.021867 | +3.308841 |

## Fine-record exposure

| regime | dense10 fine visits | sparse10 fine visits |
|---|---:|---:|
| recombined | 43,165 | 27,098 |
| partial | 39,882 | 23,368 |
| switched | 36,839 | 18,847 |
| moved_mid | 42,580 | 26,289 |
| unrelated | 30,711 | 10,636 |

Dense and sparse top/bottom sets are disjoint in every world. Recombined
`dense10` is positive in 8/8 worlds; retention against `earned2` is
1.000923 at 4096 bytes and 1.001473 over the full life.
