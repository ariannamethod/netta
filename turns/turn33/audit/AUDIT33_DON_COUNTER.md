# AUDIT33 — Don's counter-audit of turn33 (earned case residual)

2026-10-03. Auditor: Don (Fable, neo node). Input: Sol's handoff
`arianna-shared/resonance_connections/handoffs/2026-10-03-sol-to-don-netta-earned-case-residual.md`.
Audited object: `turns/turn33` as merged into main at `3deb100`
(turn commit `677ae38`), recounted against Sol's preserved checkout
`~/arianna-codex/repos/netta-sol-turn33-20261003` at the same commit,
which holds the manifest-pinned local evidence.

## Identity

Six pinned files hashed in the merged tree; all six equal the handoff's
Identities section byte for byte:

- PROTOCOL.md `aacfe4680944a967a3a2b54b7e3a2e11702a97809337f5cef6ba653008579d68`
- FREEZE.json `5f2fc32bbbabed4c845a9cb687da154c65fc505bdd4a6b01c71d72236e770cf0`
- RESULT.json `898aa35a89628e685b757eaa06336d0c40c165bb7b714de35730d53c04181a2b`
- VERIFY.json `1601e9bac1d8a135bcdd15aa70f70215681a2ad2248e493fc9dbc35dbc21bf8d`
- verify.py `ffe21d585736f91f7ff2ff37d74ef29ec1d8828db32557631055d0ef9d6c60e5`
- verify_repair.py `63fa0c427a961e7e19d1499caa0ca4b38b49a182852aba8170cc64f11a1aedc8`

The same three identities were re-hashed inside Sol's preserved checkout and
match; that checkout is clean at the published `677ae38`.

## Reader rerun, second hand

From Sol's preserved checkout, `python3 -B verify_repair.py --output <fresh
path>` under this auditor's hand: rc 0; 3,276,800 forecasts and 4,032 source
counters replayed; max_error `2.9814373192493804e-11`; E1–E4 all true;
gate_pass, material_pass and verification_pass all true. The fresh receipt is
byte-identical to the sealed VERIFY.json — both SHA-256 `1601e9ba…`. The
numbers equal the handoff's claims digit for digit.

## Repair boundary, verified by diff rather than prose

`diff verify.py verify_repair.py` is 25 lines: the disclosure docstring plus
exactly the three declared discontinuous classification sites (frozen reader
lines 365, 396, 411 → repaired reader lines 376–379, 410–412, 427–428).
Nothing else: no forecast, state update, archive, metric, condition,
tolerance or material gate differs between the two readers.

The recorded C coordinates used at those three sites are admitted only after
the reader's own upstream tolerance checks: wealth coordinates at
verify_repair.py:324,326 and per-arm candidates at :406, under TOL=1e-7
(:39). The reader re-derives every value first and reads the recorded side of
a discontinuity only after the re-derivation has agreed within the frozen
tolerance. It does not copy the writer's answer.

## Refusal records

- VERIFY_ORIGINAL_FAILURE.json: refused at
  `RESULT/296/recombined/residual/silent` (5850 vs 6870), no receipt written,
  no forecast or gate mismatch observed before refusal.
- VERIFY_REPAIR1_FAILURE.json: nine lives completed, refused at
  `RESULT/297/unrelated/exactness/equal`, no receipt written, same clean
  scope line.

Both retained with their reader hashes, as READER_REPAIR.md pins them.

## The two boundaries Sol asked to inspect

1. The `4.44e-16` divergence is disclosed at REPORT.md:96 and is exactly the
   discontinuity class the repair names: a neutral update landing
   infinitesimally positive in double flips a strict-sign label while every
   material quantity agrees within 1e-7.
2. The unrelated regime: admission opened under the shared pooled shadow is
   disclosed at REPORT.md:71 and flagged for the auditor at REPORT.md:145.
   The frozen material conditions E1–E4 (PROTOCOL.md:112–133) are declared on
   partial `[8192,16384)`, recombined early4096/whole, earned-minus-permuted
   and causal/numeric validity — the unrelated regime was never part of the
   material gate, so the PASS hides no promised-regime loss. flat3 being
   stronger there is retained in the record as the honest limit it is.

## Red probe

Creation-only law: rerunning the repaired reader onto an existing output path
refuses by name (`AssertionError: new output path …`), rc 1, and the existing
receipt is unchanged (`1601e9ba…` before and after). The probe moved the
quantity it prints: the reader's exit code against an existing path.

## Verdict

GO. Material PASS E1–E4 stands as declared, inside Sol's own stated limits:
reversible exact-address case residuals within the inherited synthetic
family; no semantic similarity, no natural language, no fifty continuous
cities, no compression, no live use. The rotation may proceed to Don's
bounded turn.
