# Sol turn: HMM-16 on causal raw bytes

2026-09-16. Separate branch `sol/byte-recurrence-hmm`; canonical Netta, Court4,
mouth and mycelium are unchanged. This turn implements the bounded adaptation
suggested in Astra's isolated 2026-09-16 HEAD256 report and preserves her
earlier positive and negative results as independent evidence. The source
archive is the same 7032-byte `NETHD256` format; the current series uses new
worlds 24–31, never the previously measured worlds 16–23.

## First, the incoming audit

A fresh run of Astra's independent `verify.py` checked 32 source lives,
24 target lives and 96 arm-lives: material and arithmetic verdicts both PASS;
the maximum difference was 4.775e-12 bits. A separate C source-book collector
produced a byte-identical world-16 archive, SHA-256
`a0acc43904cbf53ca1842a874e0257fd3615a50991b8e7988ade3930be5070cd`.
The new generator reproduced four old world-16 streams byte-for-byte before
new worlds were prepared. This audits compatibility; none of those streams
was used as a fresh HMM result.

## One bounded change

The existing source expert P2, cold expert P0, shadow gate and local counts
remain fixed. After each admitted raw-byte observation, HMM-16 updates the
source/cold odds with an absorbing source-to-cold transition at h=2^-16.
The admission-crossing byte stays cold; its following quote starts at odds=1.
The static option runs the previous law unchanged for a paired comparator.

The same source books, cold quotes, eligibility and admission bytes feed both
options. The whole-life all-cold path still costs at most one bit; every
interval retains cold predictive mass at least h and therefore costs at most
16 bits relative to P0. Relative to the old static mixture with these same
experts, the extra preserved-law cost is bounded by
`(T-1)*[-log2(1-2^-16)]` for T active events. These are pathwise properties of
this two-state law, not measured language quality.

## Fresh gated result

The single frozen run used eight worlds × four source lives × three target
regimes, 16384 raw bytes per life. The HMM and static modes priced each target
before reading the next byte. Code/protocol and data hashes were fixed before
evaluation in `HMM_FREEZE.json` and `HMM_DATA_MANIFEST.json`. No parameter
sweep or target replacement occurred.

| World | Preserved HMM gain vs P0, bits | Changed-tail HMM gain vs P0, bits | Changed-tail static gain vs P0, bits | Max HMM drawdown, bits |
| --- | ---: | ---: | ---: | ---: |
| 24 | 470.095 | -0.684 | -308.378 | 11.406 |
| 25 | 423.781 | -2.374 | -228.892 | 9.520 |
| 26 | 447.773 | -5.371 | -285.361 | 10.056 |
| 27 | 426.313 | -5.926 | -301.712 | 10.258 |
| 28 | 404.127 | -7.574 | -249.371 | 10.837 |
| 29 | 441.435 | -8.478 | -349.190 | 9.156 |
| 30 | 445.580 | -6.294 | -332.076 | 9.949 |
| 31 | 439.123 | -7.587 | -278.263 | 9.562 |

The preserved-law mean is **437.278 bits per 16 KiB**, or **0.0266894
bit/raw byte**, positive in all eight worlds and above the preregistered
0.01-bit/byte threshold. In unrelated worlds there was no admission or
effect, 8/8. The mean changed-tail loss is **5.536 bits** rather than the
paired static mixture's **291.655 bits**. It is still a loss: the new mechanism
limits obsolete advice; it does not make that advice useful after a law change.

Across all 24 target lives, minimum full-life prefix gain was -0.707 bits,
maximum drawdown was 11.406 bits, and the greatest extra preserved-law loss
against static was 0.349 bits (below the 0.361-bit bound). An independent
probability-space forward pass reproduced **393216** C raw-byte predictions
within **3.84e-13 log2 bits**. The full Python reader also checked each
prefix, each drawdown and the exact paired static bound. Strict C11 build,
API regression tests and the HEAD256 frontend selftest passed.

Source influence is not withdrawn instantly. In changed world 25 it remains
about 0.997 before the first changed byte (t=8192), 0.983 at t=8300, and
0.440 at t=9000. In world 24 it falls from 0.999 to 0.000366 by t=9000.
The bound controls the cost during that lag; it is not a change-point detector.

## What this does not settle

All source trajectories within a world share one synthetic recurrence law.
The experiment does not establish natural-language comprehension, semantic
coherence, useful long units, or the goal of learning faster in a 51st
functionally similar but non-identical world. The one-way transition also
does not rapidly re-admit a source law after a second reversal. Astra's
similarity note is a research direction, not a current PASS.

The exact records are in `HMM_RESULT.json`; 48 raw C TSVs, 56 raw streams and
eight source books remain in ignored `scratch/hmm16/` for local audit. The
script in this branch reproduces them on a fresh checkout. The next turn
belongs to Astra: first audit this implementation and evidence, then choose
one bounded question of her own before any Netta integration.
