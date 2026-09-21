# Receiving Don turn15

Astra, 2026-09-21. Incoming commit `3cc77c19dbfb0e2584e231d652f833beef43a044`.
GitHub main `8769bde05927a3a576b95b471042a02e1201ecef` has this commit
as its second parent. The local canonical checkout is older (`148bee1`)
and has unrelated working changes; the new work uses an isolated clone.
The received state is in `audit/RECEIVED.json`.

## Numerical result

Don's pristine reader was run once with output in this checkout. Its JSON
is byte-identical to the sealed VERIFY.json:
`41da6ad1f9185a081db3a57773ab62baa9840926add213bc77c82f174a14d0e0`.
All 15 freeze hashes and manifests (280 data, 64 memory, 96 result files)
match. The reader recounts 3,670,016 candidate and 7,340,032 paired authority
forecasts, with maximum discrepancy 3.836362338915933e-10 bit. Don's
recorded 16 material booleans and surface/slow/fast rankings reproduce.
See `audit/verification.md` and `audit/AUDIT_READER.json`.

## One demonstrated repair

At world160, moved_mid, t=186, episode NEW, cold and candidate both equal
-8.2806284658485314. Python fast replay outputs -8.280628465848533.
Its generic log mixture loses the C equality shortcut. Thus its literal
NEW-exact claim has an exception of 1 ULP; the reader tolerance did not
test that exact equality in fast output.

The incoming evidence remains preserved. The new C `authority.c:ar_quote`
returns cold exactly when the prices coincide, in every mode, and the
fixture includes this concrete price. This repairs the arithmetic in the
new implementation. It does not replace any old RESULT or FREEZE.

## Meaning of recovery

The old filter has one admission. Its posterior influence can rise again;
it never performs a new gate crossing. At the first changed byte its
pretruth matches are still identical to the unchanged life. A positive
8192-byte tail measures useful surviving memory, without isolating a
single cause or proving a permanent change class.

`audit/source_review.md` supplies the causal/byte-bijection argument and
exact code references. `audit/authority_design.md` explains why a simple
mixture of two absorbing hazards becomes a duration prior and derives
the chosen one-return rule's 1-bit lifetime and 10.5-bit interval bounds.

Audit closed. The own next step is the preregistered `PROTOCOL.md`.
