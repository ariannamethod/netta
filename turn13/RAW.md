# Raw quoted behavior

Synthetic raw byte behavior; this is not speech. Quotes are prepared before
truth; the tables append the subsequently observed byte. t is a zero-based offset.
Role0 means NEW/incomplete; r>0 means the r-th distinct current recency head.

Selection of these illustrations is POSTHOC: in the first world128, take the
first early byte with >.5bit joint gain/P0 and >.5bit advantage over isolated,
and the first early byte with <−.5bit joint gain/P0. Also take the first
<−.5bit byte after switch in the world with the worst paired joint−isolated tail.
These are explanatory excerpts, not a gate or a representative sample.
Complete per-world outcomes and all negative tails remain in TABLES.md.

## early harm: world128, recombined, t=140

Raw preceding16 bytes: `90 90 d6 90 aa 65 f6 65 65 af f6 f6 65 65 6a 65`; truth `f6`.
Completed pretruth role history: `10313122000210313122000210313102`.
Current heads: `R1=65, R2=6a, R3=f6`.
Truth rank3; joint prefix `102`, source counts0..6 `[73, 1133, 1102, 84, 0, 0, 0]`.
Selection step12: marginal score 1426.708245bits; 2392 affected source events; original isolated score 1426.708245bits.

| Forecast for observed truth | Probability | Gain/P0 bits |
|---|---:|---:|
| P0 | 0.126953125 | +0.000000 |
| Joint advice | 0.013868829 | -3.194378 |
| Joint received | 0.028924726 | -2.133921 |
| Isolated advice | 0.013868829 | -3.194378 |
| Isolated received | 0.014554376 | -3.124771 |
| Row received | 0.126953125 | +0.000000 |

Pretruth log2(source/cold odds): joint=2.702871, isolated=7.357155.

| t | Truth hex | Rank | Joint prefix L | Joint received gain | Isolated received gain |
|---:|---|---:|---:|---:|---:|
| 136 | 65 | 3 | 4 | +0.255883 | +0.355274 |
| 137 | 65 | 1 | 5 | +0.241680 | +0.316064 |
| 138 | 6a | 0 | 1 | +0.000000 | +0.000000 |
| 139 | 65 | 2 | 2 | +0.756732 | +0.910500 |
| 140 | f6 | 3 | 3 | -2.133921 | -3.124771 |
| 141 | 24 | 0 | 1 | +0.000000 | +0.000000 |
| 142 | b8 | 0 | 2 | +0.000000 | +0.000000 |
| 143 | 24 | 2 | 3 | +0.269550 | +0.553966 |
| 144 | f6 | 3 | 0 | +0.000000 | +0.000000 |

## early benefit: world128, recombined, t=533

Raw preceding16 bytes: `2d aa aa e0 e0 aa e0 4b f0 7e f0 7e 6a 6a 7e 7e`; truth `f0`.
Completed pretruth role history: `00210313122000210313122000220121`.
Current heads: `R1=7e, R2=6a, R3=f0`.
Truth rank3; joint prefix `1`, source counts0..6 `[6417, 1771, 6447, 1790, 0, 0, 0]`.
Selection step1: marginal score 3704.048195bits; 16425 affected source events; original isolated score 3704.048195bits.

| Forecast for observed truth | Probability | Gain/P0 bits |
|---|---:|---:|
| P0 | 0.029730903 | +0.000000 |
| Joint advice | 0.085519267 | +1.524286 |
| Joint received | 0.063409232 | +1.092730 |
| Isolated advice | 0.033861722 | +0.187692 |
| Isolated received | 0.033858186 | +0.187541 |
| Row received | 0.029730903 | +0.000000 |

Pretruth log2(source/cold odds): joint=0.607119, isolated=10.188878.

| t | Truth hex | Rank | Joint prefix L | Joint received gain | Isolated received gain |
|---:|---|---:|---:|---:|---:|
| 529 | 6a | 0 | 0 | +0.000000 | +0.000000 |
| 530 | 6a | 1 | 0 | +0.000000 | +0.000000 |
| 531 | 7e | 2 | 1 | +0.066175 | +0.111231 |
| 532 | 7e | 1 | 0 | +0.000000 | +0.000000 |
| 533 | f0 | 3 | 1 | +1.092730 | +0.187541 |
| 534 | 6a | 3 | 1 | +0.263644 | +0.317402 |
| 535 | 7e | 3 | 1 | +0.272797 | +0.317441 |
| 536 | f0 | 3 | 1 | +3.078639 | +3.253766 |
| 537 | f0 | 1 | 1 | -0.950541 | -0.971339 |

## changed-tail harm: world132, switched, t=8197

Raw preceding16 bytes: `81 0c 81 bd 81 0c 0c 81 69 69 69 f2 81 b4 b4 81`; truth `b4`.
Completed pretruth role history: `01123023030201123023231201103012`.
Current heads: `R1=81, R2=b4, R3=f2, R4=69`.
Truth rank2; joint prefix `2`, source counts0..6 `[3034, 7529, 1704, 4068, 0, 0, 0]`.
Selection step13: marginal score 1829.240277bits; 12773 affected source events; original isolated score 3090.309599bits.

| Forecast for observed truth | Probability | Gain/P0 bits |
|---|---:|---:|
| P0 | 0.078339932 | +0.000000 |
| Joint advice | 0.035468084 | -1.143226 |
| Joint received | 0.035472136 | -1.143062 |
| Isolated advice | 0.010007742 | -2.968631 |
| Isolated received | 0.010015073 | -2.967575 |
| Row received | 0.069688154 | -0.168834 |

Pretruth log2(source/cold odds): joint=13.369174, isolated=13.186093.

| t | Truth hex | Rank | Joint prefix L | Joint received gain | Isolated received gain |
|---:|---|---:|---:|---:|---:|
| 8193 | 81 | 3 | 0 | +0.000000 | +0.000000 |
| 8194 | b4 | 0 | 0 | +0.000000 | +0.000000 |
| 8195 | b4 | 1 | 0 | +0.000000 | +0.000000 |
| 8196 | 81 | 2 | 0 | +0.000000 | +0.000000 |
| 8197 | b4 | 2 | 1 | -1.143062 | -2.967575 |
| 8198 | ee | 0 | 1 | +0.000000 | +0.000000 |
| 8199 | b4 | 2 | 2 | -0.628679 | -0.628182 |
| 8200 | ee | 2 | 1 | +0.334779 | +0.334487 |
| 8201 | b4 | 2 | 1 | -0.162886 | -0.162752 |

