# BANK2-COLD-ROW: compact raw witnesses and all paired worlds

The full 16,384-event prices for each target are retained locally under
`results/c/` (C reader) and `results/events/` (new arms). Their hashes are
in `ARTIFACT_MANIFESTS.json`. This file records readable slices, not a
replacement for the event evidence.

## Preserved mosaic

| World | BANK2-ROW gain | Local-cold gain | Local-cold admission end |
|---:|---:|---:|---:|
| 40 | 185.321 | 183.013 | 245 |
| 41 | 188.146 | 194.872 | 391 |
| 42 | 149.382 | 174.811 | 371 |
| 43 | 144.031 | 150.305 | 870 |
| 44 | 205.398 | 200.660 | 358 |
| 45 | 130.543 | 140.823 | 409 |
| 46 | 94.454 | 140.087 | 320 |
| 47 | 138.608 | 147.919 | 745 |

## Same lives after a new law begins at byte 8192

The number in each tail column is its own gain from byte 8192 through byte
16383 relative to the same local P0 learner. This is the preregistered
comparison; the larger first-half gain does not excuse a worse later tail.

| World | BANK2-ROW tail | Local-cold tail | Paired difference |
|---:|---:|---:|---:|
| 40 | +2.669 | +4.490 | +1.822 |
| 41 | +3.251 | -0.595 | -3.847 |
| 42 | -1.754 | -4.080 | -2.326 |
| 43 | +4.967 | +0.218 | -4.749 |
| 44 | +1.678 | +1.437 | -0.241 |
| 45 | -4.374 | -5.759 | -1.385 |
| 46 | -0.456 | -2.246 | -1.790 |
| 47 | -2.260 | -3.004 | -0.744 |

The unrelated full-life regime remains unadmitted, with gain 0 for both
mechanisms in all eight worlds. The global three-way control likewise never
earns admission. Five of eight local-cold changed tails are negative.

## Both books wrong, but the live question is larger

World 47, changed-target event 8315, pattern `012340`, observed raw byte
255. All prices below are pretruth log2 probabilities in bits:

| Quote | Price |
|---|---:|
| Local P0 | -3.8289413830 |
| Book A | -4.9683495138 |
| Book B | -4.6214497560 |
| BANK2-ROW candidate | -4.9426064862 |
| Local-cold candidate | -3.8417522332 |
| BANK2-ROW live | -4.6636842278 |
| Local-cold live | -3.8414818821 |

The local cold posterior before this byte is 0.9835642304. It correctly
spares 1.1008542530 bits at the candidate layer. On the changed tails,
2,128–3,038 events per world have both book prices below P0. Summing only
those events gives the new candidate 118–234 bits of better price; summing
all events gives about 61–140 bits of better candidate price. The resulting
live tail nevertheless fails the paired gate. One good choice on one byte,
or even a better candidate ledger, is not the same as better received
influence after the outer HMM's own withdrawals.
