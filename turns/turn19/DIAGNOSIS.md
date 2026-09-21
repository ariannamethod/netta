# Turn19: post-hoc diagnosis of the fixed five-bit ceiling

2026-09-21. Description of the completed batch, worlds200–207. No new
forecasts, alternative authority rules, cap values or worlds were run.
[DIAGNOSIS.json](DIAGNOSIS.json) contains the recorded-price partitions,
input hashes, per-life counts and raw examples. Trace `t` below is zero-based;
the seam is t8192. All numbers concern the episode candidate.

## What failed, and which change produced it

[RESULT.json](RESULT.json) retains **FAIL 17/21**: the three moved-tail utility
conditions and the moved-tail quiet-clock condition failed. The law-tail
conditions passed. Ceiling improves law tails over witness in all eight
worlds, by 6.061269 bits in mean; its moved tails lose 11.120039 bits in mean.

The quiet-clock failure is inherited on this fresh batch: only five worlds
have fast-clock share <=25%. Witness and ceiling have exactly equal clock
states and hazard choices on all 524,288 retained authority observations.
The cap cannot have caused that clock failure. Its utility differences are
the effect of changing authority, with identical candidate prices and clocks.

## Benefit after the seam does not pay the entire preceding cost

Mean ceiling-minus-witness gain, bits per life:

| Regime | First8192 | Final8192 | Whole16384 |
|---|---:|---:|---:|
| Recombined | -23.344413 | -33.033599 | -56.378012 |
| Switched law | -23.344413 | +6.061269 | -17.283144 |
| Moved surface | -23.344413 | -11.120039 | -34.464452 |
| Unrelated | 0 | 0 | 0 |

The shared prefix is the same evidence in the three related regimes, not
three independent repetitions. Recombined whole-life retention against slow
is 97.961507%, passing its >=95% condition. That relative retention pass
does not satisfy the separate moved-tail requirement: ceiling trails slow
there by 12.405970 bits, against the fixed tolerance of three bits.

## Exact observed price decomposition

For the same old witness evidence, the hazard update is monotone in source
odds. Starting equal, adding `min(z,5)` therefore keeps ceiling authority at
or below witness authority. The recorded prequote and postupdate odds obey
that order everywhere. Consequently ceiling loses relative to witness when
the observed candidate price exceeds C, gains when it is below C, and quotes
exact C when both prices are equal.

The following are **mean paired bits per life**, grouped by the exact sign of
the saved candidate-minus-cold price. Realized helpfulness is a descriptive
partition, unavailable before truth.

| Segment | Candidate helps | Candidate harms | Equal | Net |
|---|---:|---:|---:|---:|
| Common first8192 | -278.588164 | +255.243751 | 0 | -23.344413 |
| Recombined final8192 | -279.970951 | +246.937352 | 0 | -33.033599 |
| Switched final8192 | -57.168978 | +63.230248 | 0 | +6.061269 |
| Moved final8192 | -313.614180 | +302.494141 | 0 | -11.120039 |

Across moved tails there are 22,546 helpful, 12,922 harmful and 30,068 exactly
equal observations. The net loss is a small difference between large lost
benefits and saved losses. It is not evidence that either side is negligible.

## Clipping timing and persistence

Across eight lives, the common prefix clips 11,637 times. The recombined,
switched and moved tails clip 15,410, 174 and 7,243 times respectively.
Every recorded clip occurs after a positive candidate log-ratio, under the
slow witness clock. There are no clips on unrelated lives. A clip occurs
after charging its observation and affects the next quote; its flag does not
attribute the current quote's difference to that same transition.

| World | Moved-tail clips | Fast-clock share, both modes | Ceiling minus witness tail |
|---|---:|---:|---:|
| 200 | 1990 | 5.3223% | -29.724792 |
| 201 | 1400 | 6.2622% | -20.885226 |
| 202 | 179 | 37.0605% | +4.139194 |
| 203 | 1207 | 7.9956% | -18.842367 |
| 204 | 1298 | 6.3843% | -22.588912 |
| 205 | 1099 | 14.3311% | -9.834916 |
| 206 | 8 | 43.8965% | +4.345500 |
| 207 | 62 | 40.2954% | +4.431206 |

The three non-quiet lives benefit from the cap; the five quieter lives lose.
This is an association in the retained trajectories, not a tested conditional
cap rule. The fixed global limit is repeatedly binding while the existing
witness clock is still treating the source as useful.

## Raw sequence: protection now, slower use of useful advice later

World201, moved_mid, from its saved authority trace and raw byte file:

| t, truth | Observed S-C | Ceiling / witness odds before | Ceiling minus witness price | Clip after truth |
|---|---:|---:|---:|---|
| 14635,0xf0 | +0.712239 | 5 / 14.474566 | -0.017110 | yes |
| 14638,0x5e | -10.458134 | 4.998548 / 13.749866 | +5.307396 | no |
| 14709,0xe3 | +4.562340 | -7.400599 / 1.268995 | -3.906403 | no |

The last prior clip is t14635. There are no additional clips in the 73
intervening observations before t14709. At t14638 the lower authority saves
5.307396 bits on bad advice. At t14709 a length-nine stored match prices truth
at S=-1.364209 versus C=-5.926550, yet ceiling quotes -5.746245 and witness
-1.839841. This is the largest single moved-tail loss: useful advice receives
little authority after the earlier withdrawal. The difference persists when
the cap is currently inactive; it is more than a small per-quote saturation
cost. Earlier transfers also contribute, so the whole later gap is not
attributed solely to the last clip.

The largest moved-tail saving is world205,t14773,truth0x12: matched length1,
C=-3.308345, S=-15.358620. Ceiling quotes -7.179244 versus witness -14.056753,
saving **6.877509 bits**. Prequote odds are 3.773743 versus 11.498065; clipping
is again off on that observation. These two extremes expose both consequences
of carrying less source authority.

## One bounded next question

Can the existing witnessed-wrongness signal make a commitment ceiling apply
when withdrawal is actually warranted, while preserving confidence during
repeated useful advice? That question isolates unconditional commitment
clipping from evidence-conditioned commitment control. The present traces
do not measure that intervention, its reaction cost, or its guarantees.
No conditional law, parameter sweep or replacement cap is chosen here.

This fixed cap demonstrated the promised prefix and log2(33) interval bounds
and improved law tails, but failed the complete named gate. Neither this
result nor the earlier rate experiments exclude every cap or every adaptive
authority law.
