# Turn34 tables

All values are received log2 probability gain relative to the shared P0;
margins m4 = 10.56 and m42 = 6.72 bits per life are bound to the measured
portable deltas 1056 and 1056-384 bytes.

## recombined

| arm | early4096 | whole16384 | tail |
|---|---:|---:|---:|
| sel4 | 727.107919 | 3651.104928 | 1988.384204 |
| earned2 | 728.691613 | 3651.007305 | 1986.300288 |
| pooled | 721.422156 | 3577.266685 | 1938.016698 |
| permuted4 | 711.220633 | 3553.022082 | 1929.076921 |
| cold | 0.000000 | 0.000000 | 0.000000 |

| sel4 minus | key | mean | wins |
|---|---|---:|---:|
| pooled | early | 5.685763 | 6/8 |
| pooled | gain | 73.838243 | 8/8 |
| pooled | tail | 50.367506 | 8/8 |
| permuted4 | early | 15.887286 | 7/8 |
| permuted4 | gain | 98.082846 | 8/8 |
| permuted4 | tail | 59.307283 | 8/8 |
| earned2 | early | -1.583695 | 2/8 |
| earned2 | gain | 0.097623 | 4/8 |
| earned2 | tail | 2.083917 | 6/8 |

## partial

| arm | early4096 | whole16384 | tail |
|---|---:|---:|---:|
| sel4 | 727.107919 | 2441.109269 | 778.388545 |
| earned2 | 728.691613 | 2435.018319 | 770.311302 |
| pooled | 721.422156 | 2330.301954 | 691.051967 |
| permuted4 | 711.220633 | 2303.062514 | 679.117353 |
| cold | 0.000000 | 0.000000 | 0.000000 |

| sel4 minus | key | mean | wins |
|---|---|---:|---:|
| pooled | early | 5.685763 | 6/8 |
| pooled | gain | 110.807315 | 8/8 |
| pooled | tail | 87.336578 | 8/8 |
| permuted4 | early | 15.887286 | 7/8 |
| permuted4 | gain | 138.046755 | 8/8 |
| permuted4 | tail | 99.271192 | 8/8 |
| earned2 | early | -1.583695 | 2/8 |
| earned2 | gain | 6.090950 | 7/8 |
| earned2 | tail | 8.077244 | 7/8 |

## switched

| arm | early4096 | whole16384 | tail |
|---|---:|---:|---:|
| sel4 | 727.107919 | 1725.017958 | 62.297234 |
| earned2 | 728.691613 | 1722.162250 | 57.455233 |
| pooled | 721.422156 | 1675.084594 | 35.834607 |
| permuted4 | 711.220633 | 1661.948261 | 38.003100 |
| cold | 0.000000 | 0.000000 | 0.000000 |

| sel4 minus | key | mean | wins |
|---|---|---:|---:|
| pooled | early | 5.685763 | 6/8 |
| pooled | gain | 49.933364 | 7/8 |
| pooled | tail | 26.462627 | 6/8 |
| permuted4 | early | 15.887286 | 7/8 |
| permuted4 | gain | 63.069697 | 8/8 |
| permuted4 | tail | 24.294134 | 6/8 |
| earned2 | early | -1.583695 | 2/8 |
| earned2 | gain | 2.855708 | 6/8 |
| earned2 | tail | 4.842001 | 6/8 |

## moved_mid

| arm | early4096 | whole16384 | tail |
|---|---:|---:|---:|
| sel4 | 727.107919 | 2913.080972 | 1250.360248 |
| earned2 | 728.691613 | 2905.877735 | 1241.170718 |
| pooled | 721.422156 | 2855.601829 | 1216.351843 |
| permuted4 | 711.220633 | 2829.199421 | 1205.254260 |
| cold | 0.000000 | 0.000000 | 0.000000 |

| sel4 minus | key | mean | wins |
|---|---|---:|---:|
| pooled | early | 5.685763 | 6/8 |
| pooled | gain | 57.479143 | 8/8 |
| pooled | tail | 34.008406 | 8/8 |
| permuted4 | early | 15.887286 | 7/8 |
| permuted4 | gain | 83.881551 | 8/8 |
| permuted4 | tail | 45.105988 | 8/8 |
| earned2 | early | -1.583695 | 2/8 |
| earned2 | gain | 7.203237 | 5/8 |
| earned2 | tail | 9.189530 | 7/8 |

## unrelated

| arm | early4096 | whole16384 | tail |
|---|---:|---:|---:|
| sel4 | 0.342734 | 18.185911 | 17.485028 |
| earned2 | 0.308660 | 17.713631 | 17.065029 |
| pooled | -0.053462 | 9.630354 | 9.679366 |
| permuted4 | 0.014429 | 8.181742 | 8.132690 |
| cold | 0.000000 | 0.000000 | 0.000000 |

| sel4 minus | key | mean | wins |
|---|---|---:|---:|
| pooled | early | 0.396196 | 1/8 |
| pooled | gain | 8.555557 | 3/8 |
| pooled | tail | 7.805662 | 3/8 |
| permuted4 | early | 0.328305 | 1/8 |
| permuted4 | gain | 10.004170 | 3/8 |
| permuted4 | tail | 9.352338 | 3/8 |
| earned2 | early | 0.034074 | 1/8 |
| earned2 | gain | 0.472281 | 2/8 |
| earned2 | tail | 0.419999 | 2/8 |

## Residual activity and selection

| regime | matched | first | silent | active | became | fell | episode actives (1..4) | divergent | concentrated>=0.9 |
|---|---:|---:|---:|---:|---:|---:|---|---:|---:|
| recombined | 101283 | 192 | 39047 | 62236 | 793 | 686 | 20740/20617/30645/32703 | 44297 | 25348 |
| partial | 93088 | 192 | 27307 | 65781 | 851 | 709 | 18946/19105/38126/40042 | 47511 | 27700 |
| switched | 84259 | 192 | 23845 | 60414 | 781 | 623 | 22566/23160/33485/34005 | 43846 | 24644 |
| moved_mid | 99710 | 192 | 37060 | 62650 | 813 | 699 | 22084/21831/31053/33567 | 44574 | 23782 |
| unrelated | 67469 | 172 | 9142 | 58327 | 425 | 272 | 19592/23305/36700/32557 | 43309 | 33598 |

Retention vs pooled on recombined: early 1.007881, whole 1.020641. Unrelated admissions: 15 arm-lives.
