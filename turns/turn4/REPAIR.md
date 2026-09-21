# Numerical repair before seeing the batch result

The protocol was committed as `ddae0f4` before code or data. The first code
freeze is preserved in `CODE_FREEZE.json` (SHA-256
`88c2cad79b2d17250bf34375448ce2a1e73028228c1c4e8e04d75f8adad240db`).
Its `experiment.py` hash was
`8146a761b4f365cf9f6aa6414833d9c11d00acfa2ff8c7b1c960d670ea2254c1`.

Worlds 40..47 were generated and their HEAD256 traces/books collected. The
first evaluation stopped while processing world 40's unrelated control,
before a batch summary existed. A global source arm's posterior probability
underflowed to numerical zero; the assertion incorrectly required every
diagnostic weight to be representably positive, and the subsequent mixture
would have tried `log2(0)`. No result or gate was read or tuned.

One numerical repair retains finite **log weights** for the actual mixture
and permits a rounded-zero diagnostic probability. The independent reader
permits that same underflow while checking normalization and every quoted
probability. The original partial `results/` was moved intact to
`failed-run-1/`; the experiment is replayed into a fresh `results/`.
Prior masses, update, gate, data, books, and threshold are unchanged.
`CODE_FREEZE_REPAIR.json` links the original freeze to the repaired code.

The sealed `RESULT.json` was then written. On its independent read, the
checker stopped at a missing compact `row2.activation` field. This is a
checker-only schema mistake: the paired row2 receipt intentionally contains
gain and tail, while the new arms contain activation. The verifier now skips
that absent comparison and still reconstructs row2 admission internally.
The source predictor, data, sealed result, protocol and thresholds did not
change. `CODE_FREEZE_VERIFY.json` links the numerical repair freeze to the
final checker; `RESULT.json` is not regenerated or rewritten.
