# Raw help, raw harm and retained negative cases

These examples are selected only after the preregistered gate from the saved
switched traces. They illustrate the mechanism; they do not satisfy the gate.
All values are pretruth log2 probabilities for the observed byte.

## Largest one-byte help from fast withdrawal

- world 150, switched target, byte index 9373, truth byte 170
- matched episode prefix length: 3
- cold: -1.6262974328245576
- candidate: -5.8956060971789075
- slow live: -4.715111302133569
- fast live: -1.8644305299293893
- fast minus slow: **+2.85068077220418 bits**

The remembered continuation is badly wrong at this byte. Fast has already
moved more authority to the absorbing local path, so it pays much less.

## Largest one-byte harm from fast withdrawal

- world 144, switched target, byte index 8399, truth byte 181
- matched episode prefix length: 2
- cold: -5.744540714886063
- candidate: -1.3988281791040709
- slow live: -1.590843107728174
- fast live: -4.3481733648291065
- fast minus slow: **-2.7573302571009326 bits**

Here memory is right after the change. Fast has already withdrawn more of its
authority, so it cannot exploit the helpful candidate as strongly. The tail
gain is therefore an accumulation of both help and harm, not a one-way local
improvement.

## All episode changed tails

| World | Slow | Fast | Fast - slow |
| ---: | ---: | ---: | ---: |
| 144 | -4.625 | 1.202 | +5.826 |
| 145 | 9.658 | 15.241 | +5.583 |
| 146 | 0.190 | 5.967 | +5.776 |
| 147 | 35.286 | 40.731 | +5.445 |
| 148 | -11.761 | -5.752 | +6.009 |
| 149 | -5.216 | 0.600 | +5.816 |
| 150 | 14.649 | 19.140 | +4.491 |
| 151 | -6.122 | -0.332 | +5.791 |

Fast improves every pair, but worlds 148 and 151 remain below P0. Before the
change, fast also gives up 11.08--11.24 bits of already earned first-half gain
per world. The tail recovery returns roughly half of that price, so the full
switched life stays 5.23--6.73 bits below slow even though every changed tail
improves; its preregistered requirement was retention of at least 90%, and
measured retention is 99.58%.

Against the fast row control, episode's changed-tail differences are
`-20.743, 17.075, 3.344, 32.673, -4.341, 1.130, -7.669, -4.213` bits. This
four/four split is preserved because mean tail tolerance, not universal local
victory, was the inherited condition.
