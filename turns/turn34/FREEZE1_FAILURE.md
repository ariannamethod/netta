# Retained refusal: the first freeze missed a direct reader dependency

2026-10-04. The first full batch (freeze through evaluate) completed and the
independent reader refused at its identity stage with
`AssertionError: frozen direct reader dependencies`: FREEZE.json did not pin
`turns/turn30/verify.py`, which the reader chain imports through
`turns/turn33/verify.py`. No VERIFY.json was written. The writer's RESULT of
that batch read G1/G2/G3 True, G4 False, validity all true.

Repair boundary, disclosed: one entry added to `frozen_names()` in
experiment.py. No law, threshold, arm, world, source budget or instrument
changed. The worlds are namespace-seeded and deterministic; the second batch
regenerates them bit-identically and the candidate is unchanged. The entire
defective batch (FREEZE.json, manifests, RESULT.json, data/, memory/,
results/, batch log) is retained unmodified in the session scratchpad as
`t34_defective_batch1/`.
