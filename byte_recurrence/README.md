# Causal byte recurrence, isolated Netta research

This directory is an experimental organ, not a Netta deployment. The
HEAD256-v1 frontend and repaired portable recurrence core came from Astra's
2026-09-16 isolated turn. Sol's HMM-16 turn adds one fixed source-to-cold
transition. Neither turn changes Court4, the mouth, mycelium or the canonical
Netta worktree. `NETHD256` books are byte-event archives; the generic core's
`NETTARM1` unit-event archive loader must not be used for them.

The experiment is preregistered in [HMM_PROTOCOL.md](HMM_PROTOCOL.md). The
measured result, freeze hashes and raw-data hashes are in
[HMM_REPORT.md](HMM_REPORT.md), [HMM_RESULT.json](HMM_RESULT.json),
[HMM_FREEZE.json](HMM_FREEZE.json) and
[HMM_DATA_MANIFEST.json](HMM_DATA_MANIFEST.json). The complete 48 prediction
traces and raw bytes stay in ignored `scratch/hmm16/`; they can be regenerated
from the frozen script, not silently edited into the repository.

From the repository root, on a fresh clone:

```sh
make -C portable_recurrence test
make -C byte_recurrence all test
python3 byte_recurrence/hmm16.py freeze
python3 byte_recurrence/hmm16.py prepare
python3 byte_recurrence/hmm16.py run
```

The three Python stages use exclusive files and refuse an existing data set.
The result is a synthetic recurrence-family measurement, not evidence of
semantic coherence, natural-language transfer or fifty diverse worlds.
