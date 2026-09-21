# Turn 21 raw bytes: what a tighter ceiling buys and what it costs

Every number here is read out of `RESULT.json`, which `experiment.py evaluate`
wrote and which `verify_repair.py` recomputes field by field from its own
Decimal replay. Worlds 216..223, namespace `netta-ceiling-frontier-v1`. Prices
are log2 bits against the same local P0; `live` is what the authority actually
charged at that byte.

The two bytes the protocol asks for are drawn from the `tight_vs_loose`
comparison — `h5l5` minus `h16l5`, the W3 tooth pair at the incumbent low — over
the two tail regimes at `t >= 8192`.

## A tight cap saves the most: world 216, `moved_mid`, byte 12375

| field | value |
| --- | --- |
| world / regime | 216 / moved_mid |
| t | 12375 |
| truth | 128 |
| rank | 4 |
| matched length | 1 |
| cold | -4.3461187136304815 |
| candidate | -15.946576581440837 |
| candidate minus cold | -11.600457867810356 |
| hazard at this byte | 2^-16 |
| witness w before / after | +6.499150641114604 / -5.3044056842305825 |
| `h5l5` odds before / after | +0.9997896290662314 / -10.6006902668681 |
| `h5l5` cap for the next quote | 5.0 |
| `h5l5` clipped at this byte | 0 |
| `h5l5` live | -5.930012184554709 |
| `h16l5` odds before / after | +9.635177994019058 / -1.9653075252257501 |
| `h16l5` cap for the next quote | 5.0 |
| `h16l5` clipped at this byte | 0 |
| `h16l5` live | -13.654170528696223 |
| **`h5l5` minus `h16l5`** | **+7.7241583441415145 bits** |

The surface under this life moved at byte 8192 and the archive is now confidently
wrong: it prices the truth at -15.95 bits where cold prices it at -4.35, an
11.6-bit error on a length-one match. What separates the two ceilings is not this
byte's cap — neither clipped here — but how much confidence each was *allowed to
have accumulated* before it. `h5l5` walked in with 1.00 bit of odds and paid
-5.93; `h16l5` walked in with 9.64 bits and paid -13.65. The tight ceiling saved
7.72 bits on a single byte.

Note the witness clock: it crosses from +6.50 to -5.30 **on this byte**. The
memory only becomes known-wrong here, and the clock can only lower the cap for
the *next* quote. Nothing about the evidence law protected this byte. The
protection came entirely from the ceiling having held the odds down earlier.

## A tight cap costs the most: world 221, `switched`, byte 8309

| field | value |
| --- | --- |
| world / regime | 221 / switched |
| t | 8309 (117 bytes past the seam) |
| truth | 98 |
| rank | 1 |
| matched length | 1 |
| cold | -7.305840676034813 |
| candidate | -1.7307649841781751 |
| candidate minus cold | +5.575075691856638 |
| hazard at this byte | 2^-16 |
| witness w before / after | -0.806008557741515 / +4.794254901544545 |
| `h5l5` odds before / after | -8.484265737442199 / -2.909214990025906 |
| `h5l5` cap for the next quote | 5.0 |
| `h5l5` clipped at this byte | 0 |
| `h5l5` live | -7.129561767155032 |
| `h16l5` odds before / after | +0.6182980945223757 / +6.191741706981131 |
| `h16l5` cap for the next quote | 16.0 |
| `h16l5` clipped at this byte | 0 |
| `h16l5` live | -2.4349077264266654 |
| **`h5l5` minus `h16l5`** | **-4.694654040728366 bits** |

The same machinery, the other way round. Here the archive is right: it prices the
truth 5.58 bits better than cold on a rank-1 repeat. `h5l5` had been driven down
to -8.48 bits of odds and could only pass -7.13 through; `h16l5` still held +0.62
bits and paid -2.43. The tight ceiling cost 4.69 bits.

This is the recovery cost Astra named on turn19 and Sol re-measured on turn20,
now visible as a property of the whole plane rather than of one point: a ceiling
that limits how wrong you can be also limits how fast you can come back.

## The cap is never binding on the byte that shows the difference

Across all **96** retained per-life extrema — three comparisons, help and harm,
both tail regimes, all eight worlds — the number of samples where either side was
clipped **at that byte** is **0**. Every extremal price difference between two
grid points is the accumulated effect of earlier capping, not of a clip firing on
the charged byte. A ceiling is a history, not an event.

## The two incumbents against each other

`witnessed_vs_fixed` is `h10l5` (turn20) minus `h5l5` (turn19), the comparison
Sol ran on worlds 208..215, re-run here on fresh worlds:

| direction | world | regime | t | truth | rank | L | cold | candidate | `h10l5` live | `h5l5` live | delta |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| best | 220 | moved_mid | 16001 | 119 | 3 | 3 | -7.6127 | -2.0859 | -3.4983 | -6.7145 | **+3.2162** |
| worst | 221 | moved_mid | 14728 | 17 | 4 | 1 | -3.0013 | -15.4902 | -12.2371 | -7.5409 | **-4.6963** |

Both extrema are on the moved surface, and they point in opposite directions on
the same regime: the looser ceiling wins when the memory is right (rank 3, match
length 3) and loses when it is wrong (rank 4, match length 1).

## Every world: the changed-law tail

Received gain over the final 8192 bytes of `switched`, where the command tape has
been replaced by the unrelated one. The W1 law-tail bar is `>= fast - 1` in mean.

| world | fast | `h5l5` | `h8l4` | `h16l5` | witness | `h8l4` - fast |
| --- | --- | --- | --- | --- | --- | --- |
| 216 | -4.044 | -3.334 | -6.224 | -9.919 | -9.919 | -2.180 |
| 217 | 162.974 | 162.565 | 162.849 | 160.946 | 160.796 | -0.125 |
| 218 | 277.665 | 277.432 | 279.913 | 277.208 | 276.989 | **+2.248** |
| 219 | -6.277 | -5.036 | -7.943 | -12.161 | -12.161 | -1.667 |
| 220 | 4.093 | 4.177 | 1.505 | -1.172 | -1.172 | -2.588 |
| 221 | -0.174 | 2.758 | -0.066 | -6.020 | -6.020 | **+0.108** |
| 222 | -4.558 | -3.146 | -6.061 | -10.404 | -10.404 | -1.502 |
| 223 | 5.537 | 5.979 | 3.919 | 1.151 | 0.684 | -1.618 |
| **mean** | **54.402** | **55.174** | **53.486** | **49.954** | **49.849** | **-0.916** |

The nominated point clears the -1 bar in the mean by 0.084 bit and clears it in
**3 of 8 worlds**, against W2's bar of 5. Two worlds (218, 221) carry the mean;
five sit between -1.5 and -2.6. This is W2 doing exactly the job it was
preregistered for.

An honest note on scale: after the law switches, the changed tail is worth only a
few bits to anyone — six of the eight worlds are within +-7 bits of zero under
fast, and two (217, 218) hold hundreds. The law-tail comparison is therefore a
small-numbers measurement dominated by two worlds, which is visible here and is
part of why the per-world robustness tooth matters more than the mean.

## Every world: the moved-surface tail

| world | fast | slow | `h5l5` | `h8l4` | `h16l5` |
| --- | --- | --- | --- | --- | --- |
| 216 | 1021.072 | 1032.443 | 1009.753 | 1028.440 | 1030.930 |
| 217 | 593.710 | 605.114 | 590.995 | 601.387 | 602.542 |
| 218 | 114.565 | 119.286 | 121.352 | 121.282 | 116.007 |
| 219 | 1189.785 | 1200.116 | 1177.428 | 1197.191 | 1199.805 |
| 220 | 437.953 | 443.394 | 438.908 | 444.144 | 440.274 |
| 221 | 1114.723 | 1122.980 | 1102.583 | 1119.747 | 1120.913 |
| 222 | 1116.547 | 1127.908 | 1103.874 | 1124.057 | 1126.740 |
| 223 | 653.655 | 658.143 | 649.739 | 659.401 | 657.421 |
| **mean** | **780.251** | **788.673** | **774.329** | **786.956** | **786.829** |

This is where the hundreds of bits are, and where `high=5` loses: -5.92 against
fast in mean, positive in only 2 of 8. `high=8` gains +6.70 with 8/8 wins.

## The seam: what each ceiling is holding at byte 8192

Mean capped odds carried into the first changed-law byte, over the eight
`switched` lives:

| high | low=2 | low=3 | low=4 | low=5 |
| --- | --- | --- | --- | --- |
| 5 | 3.4308 | 3.4308 | 3.4308 | 3.4308 |
| 6 | 4.4282 | 4.4282 | 4.4282 | 4.4282 |
| 8 | 6.3001 | 6.4111 | 6.4111 | 6.4111 |
| 10 | 8.0030 | 8.1264 | 8.2483 | 8.3349 |
| 12 | 9.5888 | 9.7122 | 9.8342 | 9.9532 |
| 16 | 11.1512 | 11.2746 | 11.3966 | 11.5156 |
| uncapped witness | | | | 11.6725 |

Monotone in `high`, and the law-tail ordering is its inverse: the cap is simply
how much confidence is standing there when the world changes. The witness clock
at the seam, per world, is `[10.374, 5.933, -0.743, 11.762, 10.687, 13.128,
7.549, 3.467]` — positive in seven of eight, so at the seam the evidence law
still believes the memory and the high level is the one in force. The clock
cannot anticipate the seam; only the ceiling is already there.

## Clips, and the ceiling that can never bind

Clips over all related (non-`unrelated`) lives:

| point | low clips | high clips | | point | low clips | high clips |
| --- | --- | --- | --- | --- | --- | --- |
| h5l2 | 0 | 63417 | | h10l2 | 631 | 59579 |
| h5l3 | 0 | 63417 | | h10l3 | 376 | 59871 |
| h5l4 | 0 | 63417 | | h10l4 | 133 | 60019 |
| h5l5 | 63417 | 0 | | h10l5 | 37 | 60051 |
| h6l2 | 3 | 63295 | | h12l2 | 952 | 48207 |
| h6l3 | 0 | 63301 | | h12l3 | 739 | 48622 |
| h6l4 | 0 | 63301 | | h12l4 | 493 | 48895 |
| h6l5 | 0 | 63301 | | h12l5 | 192 | 49070 |
| h8l2 | 180 | 62523 | | h16l2 | 1119 | 0 |
| h8l3 | 49 | 62578 | | h16l3 | 954 | 0 |
| h8l4 | 0 | 62618 | | h16l4 | 736 | 0 |
| h8l5 | 0 | 62618 | | h16l5 | 408 | 0 |

`h5l5` reports its clips as low because `low == high` there and the two levels
are the same number; its trajectory is identical to `h5l2`/`h5l3`/`h5l4`, whose
63,417 clips are labelled high. See `BUILD_NOTES.md` note 12.

The `high=16` row never clips at its high level at all. That is the mechanism
confirming its own bound: with hazard 2^-16 the achievable odds are at most
`log2(65535) = 15.99998 < 16`, so a 16-bit ceiling cannot bind and only the low
level ever fires. The largest odds observed anywhere in the batch is
**15.980048277009372**, which sits under that bound.

## Bounds actually observed, all 32 lives x 27 modes

| quantity | observed | bar |
| --- | --- | --- |
| worst complete-prefix gain | -0.8924434266374619 | > -1 - 1e-7 |
| worst drawdown | 14.775309661552 | <= 16 + 1e-7 |
| largest odds after | 15.980048277009372 | — |
| largest normalization error | 1.354472090042691e-14 | <= 1e-8 |

## The unrelated life that admitted

`w4_unrelated_never_admits` is **false**, on one world. World 218's unrelated
stream earns real shadow credit against an archive grown on that world's four
source tapes:

| world | max `episode_shadow_before` on `unrelated` | admitted |
| --- | --- | --- |
| 216 | 0.0000 | no |
| 217 | 5.3221 | no |
| **218** | **53.8380** | **yes, byte 1531** |
| 219 | 0.0000 | no |
| 220 | 0.7656 | no |
| 221 | 0.0000 | no |
| 222 | 4.2945 | no |
| 223 | 3.6424 | no |

The crossing byte, read straight out of `results/world218/unrelated.tsv.gz`:

    t=1528  shadow_before 31.871557900367602  matchedL 1  rank 0
    t=1529  shadow_before 31.871557900367602  matchedL 1  rank 0
    t=1530  shadow_before 31.871557900367602  matchedL 1  rank 2  cold -3.1131864854605933  candidate -2.5252938376966636   -> admitted
    t=1531  shadow_before 32.459450548131528  active_before 1

31.8716 + 0.5879 = 32.4595 >= 32. The admission is in turn13's frozen candidate
trace as `episode_activated_after=1` before any turn21 code runs, and all 27
authority modes take it on the same byte — which is why
`w4_shared_first_admission` passes on this very life. That life then earns 6.595
bits under slow and 12.052 under `h8l4`. This is a draw of this namespace for
world 218, not a property of the ceiling family, and it is reported rather than
excluded: nothing was retried on these worlds.
