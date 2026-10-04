# Turn35 independent reader

`verify.py --output PATH` imports no turn35 writer and no turn35 C code. It
recounts all four source lives, independently implements the frozen
Jeffreys-density score and stable ordering, and rebuilds `dense10`,
`sparse10`, full four-episode, two-case, and pooled archives byte-for-byte.

At Decimal precision 50 it then replays every forecast: conditional
two/four-way wealth, full and coarse controls, pooled permission, common
admission, outer trajectories, metrics, and G1–G5. Frozen readers from
turn34 and their pinned parent chain supply only inherited mathematical
laws and supplied HEAD256 bindings; all are pinned in `FREEZE.json`.

The writer must leave G5 and `gate_pass` false. Only a successful reader
receipt can complete G5. The receipt path must not exist. A second call on
the same path is therefore a red probe as well as a no-overwrite guarantee.

## Receipt of record

The single fresh batch was replayed successfully. `VERIFY.json` reports:

- `verification_pass=true`, `material_pass=false`, `gate_pass=false`;
- G1 false, G2 true, G3 false, G4 true, G5 true;
- 3,932,160 forecasts, 5,376 rebuilt source counters and 192 independently
  recomputed score values;
- maximum writer/reader disagreement
  `3.5456082514429e-11` bits;
- every validity field true, including byte-exact archives, source-only
  selection, chronology, shared-prefix identity, common admissions,
  normalization and bounds.

Receipt SHA-256:
`d19a2c38c021f3e359dc8dc2f21a8ba681c892c8eba71db7cdbdec6c13d2a311`.
A second invocation with that same path refused at rc=1 before writing; the
receipt hash remained identical.
