# Charged bytes of the witness clock

Every row below is a retained observation from worlds 192..199, not a
constructed example. Trace index `t` is zero-based; the seam is at t=8192.
C is the local P0 log2 price of the actual raw byte, S the episode
candidate's price for the same byte. The four `live` columns are what each
mode actually charged. The full traces live in ignored
`results/world*/*.tsv.gz` and `results/authority/world*/*.tsv.gz`, with their
SHA-256 values in `RESULTS_MANIFEST.json`; these tables are a readable
window, not a substitute.

## The byte where witnessed guilt saves bits

world 195, `moved_mid`, t=11100, actual byte `0xf8` (248), rank 4,
episode matchedL 2 — a stored record did vote here.

| field | value |
|---|---|
| C (cold) | -3.4304897149273974 |
| S (candidate) | -14.366210526429526 |
| hazard in force | 2^-10 |
| w before the byte | -1.8981749893552051 |
| w after the byte | -12.774577832439984 |
| slow live | -7.38434954809019 |
| fast live | -3.700463599179075 |
| adaptive live | -7.38434954809019 |
| witness live | -6.613886927687849 |
| witness - slow | **+0.7704626204023413** |

The memory is badly wrong on this byte: it prices the truth 10.9 bits below
cold. Because two earlier voting bytes had already driven w to -1.898, the
witness was at 2^-10 when the byte arrived, so it had already walked its
commitment down and paid 6.614 bits where the slow default paid 7.384. The
0.770 bits are the whole of what witnessed guilt buys at one byte. The
adaptive incumbent, reading an undifferentiated evidence stream that still
stood at +18.27, was indistinguishable from slow here.

## The byte where the hedge costs bits

world 199, `moved_mid`, t=14298, actual byte `0x0b` (11), rank 4,
episode matchedL 1.

| field | value |
|---|---|
| C (cold) | -6.148565701114575 |
| S (candidate) | -17.323975850607983 |
| hazard in force | 2^-16 |
| w before the byte | 6.62861180676166 |
| w after the byte | -4.75394246169305 |
| slow live | -15.462208989733067 |
| fast live | -9.201233536107754 |
| adaptive live | -15.338943619535616 |
| witness live | -15.092045013536982 |
| witness - fast | **-5.890811477429228** |

Here the surface move had left enough correct votes behind that w still
stood at +6.63, so the witness hedged at 2^-16 and paid 15.092 bits where
fast paid 9.201. The law is symmetric and it pays both sides: this single
byte costs 5.891 bits, and the very same retention is what earns +10.849
bits over fast across world 193's moved tail. Nothing about the seam, the
regime or the world was supplied to the clock.

## Extreme charged byte per tail life

`save` is the largest witness-minus-slow saving on a byte the witness held
at 2^-10; `cost` is the largest witness-minus-fast loss on a byte it held
at 2^-16. `mL` is the episode matchedL at that byte.

| world | regime | save t | witness-slow | mL | cost t | witness-fast | mL |
|---|---|---:|---:|---:|---:|---:|---:|
| 192 | switched | 8345 | +0.0195 | 1 | 8296 | -2.5466 | 3 |
| 193 | switched | 8239 | +0.0047 | 2 | 8216 | -1.5555 | 2 |
| 194 | switched | 8264 | +0.0024 | 2 | 8200 | -2.2077 | 2 |
| 195 | switched | 8292 | +0.0881 | 2 | 8408 | -2.1407 | 4 |
| 196 | switched | 9392 | +0.7278 | 2 | 8638 | -3.3347 | 4 |
| 197 | switched | 8893 | +0.1766 | 1 | 9105 | -2.5330 | 4 |
| 198 | switched | 8322 | +0.0675 | 3 | 8218 | -0.6991 | 2 |
| 199 | switched | 9775 | +0.1847 | 3 | 9494 | -3.2142 | 3 |
| 192 | moved_mid | 10114 | +0.1895 | 3 | 13443 | -5.7112 | 1 |
| 193 | moved_mid | 9308 | +0.1932 | 2 | 12795 | -4.4305 | 2 |
| 194 | moved_mid | 9605 | +0.2880 | 3 | 9676 | -5.7691 | 3 |
| 195 | moved_mid | 11100 | +0.7705 | 2 | 13743 | -4.6229 | 2 |
| 196 | moved_mid | 13553 | +0.0764 | 1 | 14126 | -5.6735 | 1 |
| 197 | moved_mid | 11005 | +0.3575 | 1 | 15016 | -5.2850 | 1 |
| 198 | moved_mid | 14585 | +0.4410 | 1 | 13339 | -5.5403 | 1 |
| 199 | moved_mid | 8504 | +0.6470 | 3 | 14298 | -5.8908 | 1 |

The asymmetry is itself a measurement. On the switched lives the best
single saving is under 0.73 bits while the worst single hedge costs 3.33;
on the moved lives the worst hedge costs 5.89 and the best saving 0.77.
No byte of either seam pays the clock back more than a bit.

## Final-8192 gain, every mode, every changed life

Bits against the same local P0. `ff` is the offset of the first byte at
which the witness clock reached 2^-10 hazard, measured from the seam;
`fast bytes` counts the final-8192 bytes it held there.

### switched-law

| world | slow | fast | adaptive | witness | w-f | w-s | w-a | ff | fast bytes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 192 | -0.739 | 5.104 | -0.739 | -0.717 | -5.821 | +0.022 | +0.022 | 40 | 7648 |
| 193 | -11.386 | -5.401 | -11.386 | -11.376 | -5.976 | +0.010 | +0.010 | 28 | 8110 |
| 194 | -8.818 | -2.989 | -8.818 | -8.814 | -5.825 | +0.004 | +0.004 | 21 | 6295 |
| 195 | 9.861 | 15.191 | 9.817 | 9.876 | -5.315 | +0.014 | +0.058 | 65 | 7155 |
| 196 | 56.486 | 60.851 | 56.475 | 57.924 | -2.926 | +1.438 | +1.449 | 32 | 5627 |
| 197 | 38.728 | 43.858 | 38.719 | 39.002 | -4.856 | +0.274 | +0.283 | 311 | 6938 |
| 198 | 3.736 | 9.482 | 3.736 | 3.835 | -5.647 | +0.099 | +0.099 | 120 | 6296 |
| 199 | 54.273 | 58.143 | 53.959 | 53.963 | -4.180 | -0.310 | +0.004 | 37 | 5111 |
| mean | 17.768 | 23.030 | 17.720 | 17.962 | **-5.068** | **+0.194** | **+0.241** | | |

### mid-life surface move

| world | slow | fast | adaptive | witness | w-f | w-s | w-a | ff | fast bytes |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|
| 192 | 994.957 | 983.617 | 994.930 | 993.776 | +10.158 | -1.181 | -1.154 | 154 | 851 |
| 193 | 1133.289 | 1121.975 | 1133.289 | 1132.824 | +10.849 | -0.465 | -0.465 | 106 | 335 |
| 194 | 557.393 | 552.129 | 556.320 | 554.593 | +2.465 | -2.800 | -1.727 | 47 | 2178 |
| 195 | 913.162 | 907.795 | 913.134 | 912.268 | +4.473 | -0.894 | -0.866 | 43 | 888 |
| 196 | 989.382 | 983.915 | 989.170 | 987.634 | +3.719 | -1.749 | -1.536 | 91 | 1377 |
| 197 | 548.144 | 536.894 | 546.853 | 545.672 | +8.778 | -2.471 | -1.181 | 137 | 1781 |
| 198 | 861.724 | 850.252 | 860.485 | 859.364 | +9.111 | -2.360 | -1.121 | 17 | 1701 |
| 199 | 218.165 | 207.603 | 215.678 | 214.721 | +7.118 | -3.443 | -0.957 | 247 | 2484 |
| mean | 777.027 | 768.023 | 776.233 | 775.107 | **+7.084** | **-1.920** | **-1.126** | | |

### unchanged controls

Recombined full-life means: slow 3232.832, fast 3210.957, adaptive
3232.308, witness 3231.866. Unrelated is exactly 0.000 in every mode and
every world: nothing is ever admitted there, so nothing is ever charged.

## Where the law-tail bits actually go

The switched tail was split at each world's own first fast byte, from the
retained traces:

| segment | witness | fast | slow | witness - fast |
|---|---:|---:|---:|---:|
| t=8192 .. first fast byte | -0.018 | 2.431 | -0.018 | **-2.449** |
| first fast byte .. t=16383 | 17.980 | 20.599 | 17.786 | **-2.619** |

Half the deficit is paid before the clock reacts and half after it has
already reacted, so reaction speed is not what is missing. The reason is
visible in the odds:

| world | witness odds at t=8192 | fast odds at t=8192 | bytes to fall to fast's seam level |
|---|---:|---:|---:|
| 192 | 11.214 | 5.186 | 40 |
| 193 | 12.144 | 6.132 | 11 |
| 194 | 8.852 | 2.836 | 11 |
| 195 | 10.827 | 4.768 | 65 |
| 196 | 9.562 | 3.522 | 33 |
| 197 | 13.738 | 7.726 | 44 |
| 198 | 9.666 | 3.633 | 124 |
| 199 | 12.013 | 5.998 | 23 |
| mean | 11.002 | 4.975 | 43.9 |

The witness arrives at the seam 6.03 bits more committed to memory than
fast, in every world, to within 0.05 bits — the log2 ratio of the two
hazards. A hazard clock sets the rate at which commitment decays, not the
level it has reached. Switching to 2^-10 at byte 8213 buys back the rate
within tens of bytes and buys back none of the level; the 6-bit head start
is charged once and never refunded, which is the -5.068 mean.
