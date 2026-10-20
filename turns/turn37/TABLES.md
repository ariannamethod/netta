# Turn37 tables

All values are independently replayed received log2 probability gains. Means
are over worlds 328–335. `tail` means bytes 8192–16383; wins are strict
positive paired differences.

## Portable state and price

| arm | portable bytes | recipient bytes | extra portable over coarse |
|---|---:|---:|---:|
| cold | 0 | 0 | — |
| pooled | 528 | 192 | — |
| coarse | 912 | 576 | 0 |
| fine | 1,584 | 960 | 672 |
| hierarchy | 1,584 | 1,344 | 672 |
| null | 1,584 | 1,344 | 672 |
| direct | 1,584 | 1,344 | 672 |

The fixed direct-over-coarse price is 6.72 bits per recipient life.

## Gate-bearing comparisons

| gate | comparison | switched whole | switched tail | verdict |
|---|---|---:|---:|---:|
| D1 | direct − coarse | +11.518501, 4/8 | +19.028135, 7/8 | PASS |
| D2 | direct − hierarchy | +8.839069, 4/8 | +18.224168, 7/8 | PASS |
| D3 | direct − null | +7.928618, 3/8 | +17.402334, 7/8 | PASS |
| D4 | recombined retention / partial direct − coarse | 99.173524% early, 99.843183% whole | partial +16.993868, 7/8 | PASS |
| D5 | independent replay | 4,587,520 forecasts | max error 3.72e−11 | PASS |

Direct also beats equal-portable flat-fine on the switched tail by
`+17.611191` bits at 7/8.

## Per-world switched-tail deltas

| world | direct−coarse | direct−hier | direct−null | direct−fine |
|---:|---:|---:|---:|---:|
| 328 | +64.794543 | +61.165913 | +62.254241 | +72.286836 |
| 329 | +20.232880 | +20.458406 | +22.199571 | +17.275945 |
| 330 | +34.507894 | +33.425644 | +31.804804 | +29.046344 |
| 331 | +2.934795 | +1.267924 | +4.556585 | +0.600105 |
| 332 | +3.651184 | +7.570596 | +1.843472 | +1.106422 |
| 333 | +9.577837 | +10.448449 | +8.337477 | +5.597718 |
| 334 | −1.413554 | −0.899379 | −1.377705 | −0.364163 |
| 335 | +17.939498 | +12.355787 | +9.600226 | +15.340322 |

## Mean received gains by regime

| regime | direct | coarse | fine | hierarchy | null | pooled |
|---|---:|---:|---:|---:|---:|---:|
| recombined whole | 3696.692105 | 3702.498262 | 3706.161591 | 3708.716036 | 3708.885376 | 3659.483019 |
| partial tail | 1215.691911 | 1198.698043 | 1202.982915 | 1203.316310 | 1203.550607 | 1141.253849 |
| switched tail | 368.999458 | 349.971323 | 351.388266 | 350.775290 | 351.597124 | 235.362688 |
| moved-mid tail | 1249.973278 | 1247.645866 | 1252.028756 | 1253.517697 | 1253.199482 | 1241.285339 |
| unrelated whole | 202.922527 | 202.788209 | 204.716466 | 204.068412 | 203.838164 | 122.438476 |

## Dynamic activity on switched lives

Across the eight whole switched lives, 99,231 matched bytes contain 67,455
active direct states and 31,776 silent states. Direct influence activates 775
times and later revokes 622 times. Both coarse parents are silent at 40,498
matched positions; a direct episode is active at 9,676 of them and changes the
candidate at 7,352.

On the judged switched tails alone, 1,755 bytes expose a speaking direct
episode behind silent coarse parents. Their direct-minus-coarse candidate sum
is `+2.242710` bits; received sum is `+0.902954`. The full switched-tail
candidate comparison is `+19.843451` bits versus coarse, close to the received
`+19.028135`; the PASS is therefore broader than the parent-veto subset.

World334 is the one losing tail. Its candidate direct-minus-coarse sum is
`+5.076531`, but its received sum is `−1.413554`: the inherited slow outer
history reverses the candidate advantage in that world. It is retained.
