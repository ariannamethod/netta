# Incoming Sol HMM-16 audit

2026-09-16, Astra. Completed before the BANK2-ROW next step.

## Received state

Oleg merged Sol commit `98d723b605bcc167bfe0b69abc4242dbeeb873fa`.
Remote main is `37e9b68c439a4891ca507fc4155c58bd94d64a2e`; its ancestry
contains that exact Sol commit. Astra's isolated branch is
`astra/turn3-memory`, rooted at the merged commit. Canonical Netta remains
read only and locally still at916cfce with its existing unrelated changes.
Receipt: [RECEIVED.json](evidence/RECEIVED.json).

## Reproduction

From a fresh checkout, strict C11 builds and tests passed, then Sol's frozen
generator, C source collector, static runs and HMM runs were executed.
All56 raw-stream hashes, code/protocol freeze and the complete HMM_RESULT
match the committed artifacts exactly. This is a reproduction of the old
sample, not fresh evidence for the next mechanism.

Commands and outputs: [SOL_REPRODUCE.txt](evidence/SOL_REPRODUCE.txt).
The reproduced raw streams and48 C traces are retained under
`../byte_recurrence/scratch/hmm16/`.

## Independent check

[audit_hmm.py](audit_hmm.py) uses a probability-space forward recursion:
before truth source mass w gives likelihood ratio `(1-w)+w*(P2/P0)`;
after truth the remaining source mass is multiplied by1-h. It reconstructs
admission from accumulated candidate evidence and also reconstructs the
static mixture through sequence capital.

All393216 raw-byte forecasts and every prefix were checked. Bounds:
full-prefix gain>=-1; drawdown<=16; HMM extra loss relative to static
<=`max(0,T-1)*[-log2(1-h)]`. Maximum numerical difference from the committed
numbers was8.810729923425242e-12 bits, including accumulated quantities.
Result: [HMM_AUDIT.json](evidence/HMM_AUDIT.json).

The incoming `hmm16.py` checks the static-cost bound only at the final
endpoint, while its protocol describes prefix checks. The independent
audit now checks that bound inside the loop. This is a repaired coverage
gap in the evidence, not a changed HMM outcome. Sol's frozen script is
preserved so its code hashes and reproduction remain meaningful.

An independent C API review found no production defect. Its bounded probe
confirmed hazard application on active incomplete-context, one-class,
unsupported-row and selected-NEW events, even with zero likelihood-ratio
evidence; maximum difference from a probability-space formula6.43e-17.
[API audit](evidence/mechanism/HMM_API_AUDIT.md) and
[raw probe](evidence/mechanism/hmm_null_evidence.txt).

## Verdict

Sol's HMM-16 result withstands this audit. The code preserves current-byte
pricing before any transition, uses the one-time earned admission, and
retains the all-cold path. Its recorded benefits and losses remain exactly
those in HMM_REPORT: preserved mean437.278 bits/16KiB, changed-tail mean
loss5.536 bits versus paired static291.655, maximum drawdown11.406.
No production repair was necessary. The next step is separately declared
in [PROTOCOL.md](PROTOCOL.md); live Netta integration is outside this turn.
