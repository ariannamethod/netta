# Turn33 tables

All values are independently reconstructed received bits saved against P0.
Early is [0,4096), tail is [8192,16384), and whole is 16,384 bytes. No world
or life is excluded. VERIFY.json retains every arm/world/regime trajectory,
activation, minimum and drawdown.

## Mean received gain

| Regime/window | earned | pooled | permuted | flat3 | cold |
|---|---:|---:|---:|---:|---:|
| recombined/early | 743.261276025 | 731.159180874 | 725.017197174 | 705.544761311 | 0 |
| recombined/whole | 3641.974550399 | 3592.901281183 | 3581.917371748 | 3476.904721556 | 0 |
| recombined/tail | 1991.470456559 | 1960.118578452 | 1956.710546686 | 1898.665513341 | 0 |
| partial/early | 743.261276025 | 731.159180874 | 725.017197174 | 705.544761311 | 0 |
| partial/whole | 2440.146321939 | 2378.949872265 | 2365.565905462 | 2354.299980622 | 0 |
| partial/tail | 789.642228098 | 746.167169533 | 740.359080400 | 776.060772407 | 0 |
| switched/whole | 1784.391122417 | 1714.359882933 | 1703.991901944 | 1769.395757922 | 0 |
| switched/tail | 133.887028577 | 81.577180201 | 78.785076882 | 191.156549706 | 0 |
| moved_mid/whole | 2902.408814050 | 2859.857619995 | 2847.649623078 | 2764.432692160 | 0 |
| moved_mid/tail | 1251.904720210 | 1227.074917263 | 1222.442798017 | 1186.193483945 | 0 |
| unrelated/early | 19.553210615 | 8.110339176 | 7.938756298 | 17.683148829 | 0 |
| unrelated/whole | 58.648387955 | 26.026152332 | 26.059014103 | 65.096589949 | 0 |
| unrelated/tail | 26.232383935 | 11.811014408 | 11.953551032 | 31.941059910 | 0 |

The four shared-prefix regimes have identical early values by construction
and bytewise verification.

## Preregistered comparisons

| Comparison | Window | Mean earned minus control | Wins |
|---|---|---:|---:|
| pooled | partial tail | +43.475058565 | 7/8 |
| pooled | partial whole | +61.196449674 | 7/8 |
| pooled | recombined early | +12.102095151 | 6/8 |
| pooled | recombined whole | +49.073269216 | 8/8 |
| permuted | partial tail | +49.283147698 | 6/8 |
| permuted | recombined whole | +60.057178651 | 8/8 |

Priced portable-memory margin: 3.84 bits. Recombined retention relative to
pooled: 101.655193% early and 101.365840% whole. Earned whole-life
recombined gain is positive in 8/8 worlds.

## Partial post-change differences by world

| World | earned tail | pooled tail | earned-pooled | permuted tail | earned-permuted |
|---:|---:|---:|---:|---:|---:|
| 296 | 555.915616 | 548.311431 | +7.604185 | 530.126637 | +25.788979 |
| 297 | 1040.732370 | 900.306486 | +140.425884 | 899.264255 | +141.468115 |
| 298 | 546.953896 | 526.503490 | +20.450406 | 547.552510 | -0.598614 |
| 299 | 1058.887323 | 1085.072039 | -26.184716 | 1082.514095 | -23.626772 |
| 300 | 673.469746 | 671.690006 | +1.779740 | 654.823303 | +18.646442 |
| 301 | 870.643613 | 812.520904 | +58.122708 | 807.179774 | +63.463839 |
| 302 | 692.989715 | 608.791038 | +84.198677 | 585.284751 | +107.704964 |
| 303 | 877.545546 | 816.141963 | +61.403583 | 816.127317 | +61.418229 |

## Memory and state price

Every world has 32 grammar rules and 24 selected relations.

| Arm | Portable archive bytes | Recipient state bytes | Description |
|---|---:|---:|---|
| earned | 912 | 576 | full A/B case bank; two wealths plus pooled permission per relation |
| pooled | 528 | 192 | one pooled vector and one permission per relation |
| permuted | 912 | 576 | rotated case-null bank with the same law |
| flat3 | 912 | 576 | immediate P0/A/B router |
| cold | 0 | 0 | P0 |

The earned-versus-pooled delta is 384 portable bytes and 384 recipient bytes.
Only portable bytes determine the frozen material price.

## Residual activity

| Regime | Matched | First | Silent | Active | Became active | Fell silent |
|---|---:|---:|---:|---:|---:|---:|
| recombined | 99,546 | 192 | 46,924 | 52,622 | 741 | 636 |
| partial | 92,651 | 192 | 37,553 | 55,098 | 888 | 752 |
| switched | 84,181 | 192 | 31,396 | 52,785 | 820 | 670 |
| moved_mid | 97,676 | 192 | 45,754 | 51,922 | 735 | 633 |
| unrelated | 69,515 | 169 | 15,634 | 53,881 | 465 | 323 |
| total | 443,569 | 937 | 177,261 | 266,308 | 3,649 | 3,014 |

Counts use the C-double sign only after the reader independently checks both
wealth coordinates. This is the disclosed zero-classification repair; none of
these descriptive counts enters E1--E3.

## Verification envelope

| Quantity | Value |
|---|---:|
| Forecasts independently checked | 3,276,800 |
| Source continuation counters checked | 4,032 |
| Exact archives rebuilt | 24 |
| Maximum reader discrepancy | 2.9814373192493804e-11 |
| Maximum full-vector normalization error | 1.7541523789077473e-14 |
| Lowest received prefix | -0.991865754903 |
| Maximum drawdown | 14.646407349201 |
| Freeze pins | 24 |
| Material / full gate | PASS / PASS |
