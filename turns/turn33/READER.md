# Turn33 independent reader

The frozen verify.py was written before worlds296--303 and imports no turn33
writer or C implementation. It uses the pinned turn28 source re-counter and
turn30 outer-law reader, then implements turn33 wealth and pooled permission
independently with 50-digit Decimal state.

## Reconstruction scope

For each of eight worlds the reader:

1. reads all four source byte/role tapes and recounts continuation ranks inside
   each life without joining life boundaries;
2. rebuilds bank_full.bin, pooled_full.bin and permuted_full.bin byte-for-byte;
3. checks all 24 selected addresses, exact A+B pooled sums, rotated null counts
   and the 528/912-byte prices;
4. replays all five target regimes and every causal P0/history match;
5. reconstructs pooled, A, B, permuted, corrected, candidate and received
   prices; wealth, permission, flat3 and outer states; shared admission;
6. verifies complete positive normalized support, protected NEW, first/silent
   equality, prefix floor and drawdown;
7. rebuilds all per-life results, paired comparisons, memory costs and E1--E4.

This covers 3,276,800 arm forecasts and 4,032 source counters. The successful
receipt's maximum discrepancy is 2.9814373192493804e-11.

## Supplied pinned boundaries

The reader does not independently rediscover the greedy grammar/record
selection. HEAD256 identities and P0 prices are supplied trace inputs whose
consistency with truth, head lists and residual support is checked. It does
not rebuild the frontend or generator. These limits are explicit in the
receipt and prevent the result from being described as an independent
natural-language or end-to-end model reproduction.

## Frozen refusal and separate repair

The original reader is SHA-256
ffe21d585736f91f7ff2ff37d74ef29ec1d8828db32557631055d0ef9d6c60e5
and remains in FREEZE.json unchanged.

Its first post-result run independently completed world296/recombined, then
refused while comparing the descriptive silent count. Decimal and C-double
wealth agreed within tolerance but differed in strict sign at zero. A first
separate repair passed nine lives and refused on the analogous exact-equality
category in world297/unrelated. Neither attempt wrote an output receipt.

The final verify_repair.py, SHA-256
63fa0c427a961e7e19d1499caa0ca4b38b49a182852aba8170cc64f11a1aedc8,
uses recorded C values for exactly three discontinuous categories only:

- wealth sign before observation;
- wealth sign after observation;
- exact candidate/P0 equality.

Each underlying numeric coordinate is still independently reconstructed and
checked first. Forecast arithmetic, archives, trajectories, thresholds,
comparisons and gates are byte-for-byte the frozen reader's code. The full
diff, both refusals and their hashes are recorded in READER_REPAIR.md,
VERIFY_ORIGINAL_FAILURE.json and VERIFY_REPAIR1_FAILURE.json.

The final reader produced VERIFY.json once. It reports verification_pass,
material_pass and gate_pass all true. A repaired verification PASS does not
erase the two earlier refusals; they are part of the evidence chain.

## Fixture

Before data, earned_router --fixture emitted 24 handcrafted observations over
two record identities. fixture_check.py independently reproduced 384 numeric
coordinates with maximum error 3.552713678800501e-15. It witnessed first and
silent quotes, positive and negative wealth, recovery, record locality, NEW,
no match and exact common forecasts. FIXTURE_READER.json pins the reader,
binary and emitted fixture hashes.
