# Turn 15 raw bytes: what the surface move did to a remembering life

Every number here is read out of `RESULT.json`, which is written by
`experiment.py evaluate` and recomputed field by field by `verify.py`.
`received` is `fast_live - logcold` at that byte: what the admitted
candidate actually bought, in bits, under the fast authority (hazard
2^-10). Samples are drawn only from `moved_mid` at t >= 8192, the changed
tail, where the surface under the process has already moved.

## The two extreme bytes across the batch

### Memory helps most: world 161, byte 9861

| field | value |
| --- | --- |
| world | 161 |
| regime | moved_mid |
| t | 9861 |
| truth | 76 |
| rank | 2 |
| matched_length | 2 |
| logcold | -7.875385027363873 |
| candidate | -1.62834413170541 |
| candidate_minus_cold | 6.247040895658463 |
| slow_live | -1.6306539388431425 |
| fast_live | -1.7816236802169554 |
| received | 6.093761347146917 |
| fast_gain_after | 1516.4602981130963 |

### Memory harms most: world 167, byte 15420

| field | value |
| --- | --- |
| world | 167 |
| regime | moved_mid |
| t | 15420 |
| truth | 38 |
| rank | 4 |
| matched_length | 1 |
| logcold | -4.523927071331939 |
| candidate | -15.95183897309847 |
| candidate_minus_cold | -11.427911901766532 |
| slow_live | -15.73544140730499 |
| fast_live | -12.446916125562995 |
| received | -7.9229890542310555 |
| fast_gain_after | 1642.3743130108815 |

The helping byte is a repeat the archive still recognises through its rank history: the stored branch pushes the truth from a cold price of -7.875385 bits to a candidate of -1.628344, and the authority passes 6.093761 bits of that through. The harming byte is the same machinery betting on the wrong rank: the candidate prices the truth at -15.951839 against a cold -4.523927, and the mixture still eats 7.922989 bits. Both are ordinary bytes of the same arm in the same regime; the second is why the one-bit prefix bound and the drawdown ceiling are load-bearing rather than decorative.

## Every world: the best and the worst post-move byte

| world | kind | t | truth | rank | matched L | logcold | candidate | fast_live | received |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| 160 | help | 10242 | 187 | 1 | 5 | -6.589042 | -1.307296 | -1.360815 | +5.228227 |
| 160 | harm | 9844 | 15 | 4 | 1 | -6.667529 | -15.805129 | -12.801329 | -6.133800 |
| 161 | help | 9861 | 76 | 2 | 2 | -7.875385 | -1.628344 | -1.781624 | +6.093761 |
| 161 | harm | 16169 | 108 | 4 | 1 | -4.771266 | -16.145103 | -12.425826 | -7.654560 |
| 162 | help | 15707 | 222 | 2 | 5 | -6.542896 | -1.418113 | -1.437426 | +5.105470 |
| 162 | harm | 15207 | 161 | 4 | 2 | -4.643688 | -13.833373 | -11.701193 | -7.057504 |
| 163 | help | 15787 | 253 | 3 | 3 | -3.557791 | -1.282108 | -1.363422 | +2.194369 |
| 163 | harm | 15836 | 110 | 4 | 1 | -7.312305 | -16.597105 | -10.702734 | -3.390429 |
| 164 | help | 14756 | 133 | 2 | 4 | -6.657881 | -1.147698 | -1.158256 | +5.499625 |
| 164 | harm | 13653 | 219 | 4 | 1 | -6.687666 | -15.409711 | -13.709437 | -7.021770 |
| 165 | help | 11584 | 0 | 2 | 3 | -5.769627 | -0.710310 | -0.715278 | +5.054349 |
| 165 | harm | 15093 | 135 | 4 | 3 | -5.875824 | -12.803157 | -11.857301 | -5.981477 |
| 166 | help | 9427 | 29 | 3 | 7 | -5.255827 | -0.526738 | -0.545829 | +4.709998 |
| 166 | harm | 12492 | 23 | 4 | 2 | -6.735286 | -14.882597 | -13.366027 | -6.630741 |
| 167 | help | 10459 | 152 | 1 | 1 | -6.680601 | -1.187242 | -1.214915 | +5.465686 |
| 167 | harm | 15420 | 38 | 4 | 1 | -4.523927 | -15.951839 | -12.446916 | -7.922989 |

## All eight moved_mid episode changed tails

Received gain over the final 8192 bytes, the segment that is entirely on
the new surface. This is the recovery tooth of condition 3.

| world | fast (2^-10) | slow (2^-16) | whole life fast | early 4096 fast | admitted at |
| --- | --- | --- | --- | --- | --- |
| 160 | 350.9043 | 355.7545 | 1463.765 | 474.318 | 126 |
| 161 | 1014.5565 | 1021.2127 | 2454.080 | 580.993 | 120 |
| 162 | 607.3709 | 618.5543 | 1939.508 | 577.137 | 131 |
| 163 | 1.6906 | 16.3054 | 432.300 | 81.252 | 130 |
| 164 | 887.9114 | 893.2604 | 2496.919 | 750.777 | 101 |
| 165 | 338.9570 | 346.0378 | 924.319 | 179.048 | 149 |
| 166 | 910.2992 | 921.5623 | 2279.309 | 570.984 | 83 |
| 167 | 630.6540 | 641.8376 | 1678.321 | 426.393 | 106 |
| **mean** | **592.7930** | **601.8156** | **1708.565** | **455.113** | |

Positive in 8 of 8 worlds under the fast law; the gate asked for 5 and for a mean of at least +1 bit. World 163 is the thin one: 1.6906 bits of recovery against 16.3054 under the slow law. The whole-life rate is 0.104283 bit per raw byte, positive in 8 of 8.

## The move is visible only after the move

`moved_mid` shares its first 8192 bytes with `recombined` byte for byte, so
the two lives must be identical up to the seam. The 8192-byte horizon
difference between them, fast episode arm, per world:

    0.0  0.0  0.0  0.0  0.0  0.0  0.0  0.0

Exactly zero everywhere. Everything below is the cost of the seam alone.

## Per-arm tables, fast authority (hazard 2^-10)

### recombined

| arm | mean gain | mean early 4096 | mean changed tail | admitted | worst prefix gain | worst drawdown |
| --- | --- | --- | --- | --- | --- | --- |
| episode | 2707.075 | 455.113 | 1591.303 | 8/8 | -0.9296 | 8.5236 |
| isolated | 1529.612 | 194.670 | 988.489 | 8/8 | -0.9849 | 8.4793 |
| frequency | 1182.476 | 114.047 | 815.574 | 7/8 | -0.9772 | 8.3662 |
| reverse | 396.715 | 16.323 | 311.999 | 5/8 | -0.9530 | 8.5485 |
| permuted | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| flat | 1519.253 | 195.550 | 976.962 | 8/8 | -0.9582 | 8.4840 |
| row | 281.687 | 173.422 | 43.094 | 8/8 | -0.9662 | 5.2998 |

### moved_whole

| arm | mean gain | mean early 4096 | mean changed tail | admitted | worst prefix gain | worst drawdown |
| --- | --- | --- | --- | --- | --- | --- |
| episode | 2707.075 | 455.113 | 1591.303 | 8/8 | -0.9296 | 8.5236 |
| isolated | 1529.612 | 194.670 | 988.489 | 8/8 | -0.9849 | 8.4793 |
| frequency | 1182.476 | 114.047 | 815.574 | 7/8 | -0.9772 | 8.3662 |
| reverse | 396.715 | 16.323 | 311.999 | 5/8 | -0.9530 | 8.5485 |
| permuted | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| flat | 1519.253 | 195.550 | 976.962 | 8/8 | -0.9582 | 8.4840 |
| row | 281.687 | 173.422 | 43.094 | 8/8 | -0.9662 | 5.2998 |

### moved_mid

| arm | mean gain | mean early 4096 | mean changed tail | admitted | worst prefix gain | worst drawdown |
| --- | --- | --- | --- | --- | --- | --- |
| episode | 1708.565 | 455.113 | 592.793 | 8/8 | -0.9296 | 8.5357 |
| isolated | 749.073 | 194.670 | 207.950 | 8/8 | -0.9849 | 8.4793 |
| frequency | 418.306 | 114.047 | 51.404 | 7/8 | -0.9772 | 8.7340 |
| reverse | 88.355 | 16.323 | 3.640 | 5/8 | -0.9530 | 8.5318 |
| permuted | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| flat | 681.995 | 195.550 | 139.703 | 8/8 | -0.9582 | 8.9400 |
| row | 244.392 | 173.422 | 5.800 | 8/8 | -0.9662 | 5.2998 |

### unrelated

| arm | mean gain | mean early 4096 | mean changed tail | admitted | worst prefix gain | worst drawdown |
| --- | --- | --- | --- | --- | --- | --- |
| episode | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| isolated | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| frequency | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| reverse | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| permuted | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| flat | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |
| row | 0.000 | 0.000 | 0.000 | 0/8 | 0.0000 | 0.0000 |

### slow authority (hazard 2^-16), episode arm only

| regime | mean gain | mean changed tail | admitted |
| --- | --- | --- | --- |
| recombined | 2729.495 | 1603.332 | 8/8 |
| moved_whole | 2729.495 | 1603.332 | 8/8 |
| moved_mid | 1727.979 | 601.816 | 8/8 |
| unrelated | 0.000 | 0.000 | 0/8 |

`recombined` and `moved_whole` agree to every printed digit because they agree exactly: the worst full-life difference over all seven arms, both laws and all eight worlds is 0.0 bits, and the worst 4096-horizon difference is 0.0 bits. `unrelated` never admits any arm and receives nothing, which is the withdrawal control doing its job.

