# Disclosed reader-only repair

2026-10-03. The original frozen reader remains byte-identical to its pre-data
pin:

- verify.py SHA-256:
  ffe21d585736f91f7ff2ff37d74ef29ec1d8828db32557631055d0ef9d6c60e5
- first repair attempt SHA-256:
  7d0ac81156da097f95b2a07526d6112f98ce863e16998f40e99ba1c7b77437be
- final repair SHA-256:
  63fa0c427a961e7e19d1499caa0ca4b38b49a182852aba8170cc64f11a1aedc8

## Original refusal

The original reader was executed once after RESULT.json existed. It stopped
while comparing the first saved life:

    RESULT/296/recombined/residual/silent: 5850 != 6870

No VERIFY.json was written. Before that refusal it had already rebuilt the
world296 source archives and independently replayed the complete recombined
life without a forecast, state, normalization or outer-law mismatch.

## Cause

The forecasts use a continuous numeric tolerance of 1e-7. The descriptive
silent/active counter instead asks whether a wealth coordinate is strictly
greater than zero. Independently replayed Decimal and C-double wealth can be
numerically equal within the frozen tolerance while landing on opposite sides
of exact zero. The resulting category count is discontinuous and is not a
material gate quantity.

## Repair boundary

verify_repair.py is a separate artifact, following the turn20/turn27 reader
repair precedent. It changes only three discontinuous classifications:

- pre-observation silent versus active;
- post-observation became-active versus fell-silent.
- whether a candidate is exactly equal to P0 for the exact-passthrough flag.

At each point the repair uses the recorded C coordinate only after it has been
independently reconstructed and checked under the frozen tolerance. Archive
reconstruction, source prices, corrected forecasts,
permission and wealth updates, outer trajectories, help/harm witnesses,
tables, conditions, thresholds, E1--E4 and every material comparison are
unchanged. The diff is one disclosure docstring and the two named
classifications. The first repair attempt passed nine full lives and then
refused on the third exact-equality classification at world297/unrelated; its
hash and refusal are retained separately.

No predictor, result, manifest, generated world, threshold or frozen file was
edited. No second material batch is permitted or run.
