# Turn35 interface

`hybrid_router predict DENSE10 SPARSE10 BANK4 BANK2 POOLED < RAW` emits one
tab-separated causal row per raw byte. Each row carries the supplied
HEAD256/P0 quote, common matched address, frozen source-only fine tags, all
episode/case/hybrid source prices, pre/post pooled permission, pre/post
hybrid/full/coarse wealths, candidate/live prices, and complete outer state
for six arms. Every 256-vector normalization is checked before truth is read.

`NETEH010` stores a NETEB001-compatible two-case record for every selected
address. Reserved value 1 marks exactly ten fine records. After the base
records, their E1 and E3 counter blocks occur in record order; E2 and E4 are
reconstructed by exact subtraction. The loader rejects any tag count, size,
address, parent sum, or full-episode projection mismatch.

`hybrid_router --fixture` emits JSON lines for a frozen no-world sequence
containing one fine and one coarse record. `fixture_check.py` independently
reconstructs it with Decimal state. `--fixture34` exposes the staged parent
fixture; it must be byte-identical to turn34. `predict34` similarly exposes
the frozen parent predictor as a provenance probe.

`emit` and `trace` delegate unchanged to the inherited turn13 episode
program. `experiment.py` has creation-only stages `freeze`, `generate`,
`extract`, `learn`, and `evaluate`; every stage after freeze checks the
frozen identities and incoming manifest. `verify.py --output PATH` imports
no turn35 writer or C implementation and creates exactly one receipt.
