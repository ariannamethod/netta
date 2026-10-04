# Sol audit of Don turn34

Date: 2026-10-04

Audited object: commit `3c780af57b11e716d8d7c127917f52302618a959`
(`don/turn34-earned-selection-20261003`), whose protocol was frozen at
`dc55922`.  Oleg explicitly routed Don's published hand directly to Sol for
this round.  That instruction overrides the stale recipient label in the
handoff, without changing the standing rotation after this turn.

## Verdict

**GO, with one recordkeeping reservation.**  The accepted second batch is
internally complete, reproducible, and independently verified.  It honestly
fails its material gate at G4.  I do not certify the claim that the discarded
first batch was byte-identical to the accepted batch: the referenced
`t34_defective_batch1/` could not be found anywhere under `/Users/ataeff`, so
that equality is no longer independently auditable.  This does not invalidate
the preserved accepted batch; it narrows what this audit signs.

## Independent checks

- Strict C builds passed with `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`.
  `sel4_router` SHA-256 is
  `0dd69fff40e6ab514de6d7094250af8c6786c41546ce01ca66b814a71862a6cf`.
- The staged turn33 source differs from its frozen parent on exactly the two
  expected `main` declarations.  The parent fixture and the staged fixture are
  byte-identical, SHA-256
  `4d67e152969d07bcd28b1f86abf3a804ff6dd6fe9b8d73af2fe0b82a55149c0d`.
- A fresh turn34 fixture reconstruction checked 28 events and 364 numerical
  values.  Its receipt is byte-identical to the committed receipt; maximum
  error is `2.6645352591003757e-15`.
- A fresh full independent reader checked 3,276,800 forecasts and 6,720 source
  counters.  Its receipt is byte-identical to the committed `VERIFY.json`,
  SHA-256
  `1e6c7758de8a1a83c83930b44f60c131ee75d0d1143c8d1abd13f888dba88cec`;
  maximum numerical error is `3.3651303965598345e-11`.
- All 26 freeze pins and all four manifests match.  A second reader run aimed
  at an existing receipt correctly refused before writing and left the receipt
  hash unchanged.
- The reader checks each continuous wealth value before using the recorded C
  double to classify a discontinuous zero crossing.  The disclosed turn33
  convention therefore does not hide any material quantity here.

## Direct G4 recount

I independently summed `sel4_live - earned2_live` over `t >= 8192` in the
eight switched traces, without importing the writer or its verdict code:

| world | tail gain, bits |
|---:|---:|
| 304 | +6.305015085166 |
| 305 | -0.594170382372 |
| 306 | +9.631746211057 |
| 307 | +1.813632182923 |
| 308 | +3.062929384999 |
| 309 | -0.005896803615 |
| 310 | +10.702308618237 |
| 311 | +7.820446865293 |

Mean `+4.842001395211` bits, wins `6/8`, against the preregistered price
`6.72` bits for 672 additional portable bytes: **G4 FAIL**.  The result is not
massaged into a pass.  G1, G2, G3, and independent G5 pass.

## Scientific hand

Turn34 establishes that distinct episodes can earn distinct influence, but
not that every remembered relation deserves full episode resolution.  The
next bounded question is whether source-only evidence can decide *where* fine
resolution is worth carrying, before any recipient world is seen.
