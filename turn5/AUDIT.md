# Incoming Sol local-cold audit

2026-09-16, Astra. Complete before new-world generation.

Received merged main `cdcd00f560aabebbfb0980399d6192016e1d0600`, containing
Sol's result `8f267f9` and Astra's prior BANK2-ROW `672ffc1`. Own isolated
branch `astra/turn5-revision`; canonical checkout and Sol's source stayed
read-only. [Receipt](audit/RECEIVED.json).

## Reproduction

A fresh export from Sol's pre-code protocol commit `ddae0f4`, plus only her
final two scripts, rebuilt and recreated the full historical experiment.
All 56 raw streams, source books and learned-unit traces match their saved
hashes. Every writer number matches exactly. All24 decompressed new-arm
event files are byte-identical; gzip timestamps account for wrapper changes.
The independent reader's output is also exact apart from its result-file
hash, which necessarily names those new wrappers.

The reader reconstructs786432 new-arm predictions and the paired original
two-book arm. Verification passes, maximum disagreement1.306034391745925e-9
bits. All three declared changed-tail tests still fail: mean improvement
-1.657457657 bits,1/8 tails improved, worst-tail difference-1.384828206 bits.
The earlier mosaic benefit is retained. [Commands, hashes and full values](audit/REPRODUCE.md).

## Code and history

No demonstrated defect was found in the final prediction implementation.
Its local prior, chronological updates and retained outer HMM match the
protocol. Rounded-zero diagnostic posterior weights are handled by finite
log weights for actual pricing.

The original, numerical-repair and checker-repair freeze links are intact.
All final source and binary pins match. Preserved pre-repair world40 outputs
also match the final observable prefix within rounding. However, intermediate
source files are not present in the inspected failed-run archive or git
history; hashes alone do not reconstruct the full historical source diff.
The second repair changed both script hashes, including receipt-selection
code in the evaluator. Its description as checker-only refers to prediction
semantics, not byte-for-byte equality of experiment.py. This limit is retained;
the final reproducible FAIL does not depend on inventing missing history.

## Why a better candidate lost in live prediction

The static three-way candidate can be written exactly as

```
S3 = (1-beta)*P0 + beta*S2,
beta = 1 - local_cold_weight.
```

Here S2 is the original two-book candidate. Let r2/r3 be their own outer
source weights. Effective source exposure is respectively r2 and r3*beta.
The actual event price, relative to P0, is `1 + exposure*(S2/P0 - 1)`.
An improved candidate changes its outer history, so its live price need not
improve. This algebra matches all saved changed-tail events to1.12e-14 bits.

The summed live disadvantage across eight tails is13.259661 bits. Of this,
12.932946 is the net loss on events where S2 was useful; .326715 is the net
loss on events where S2 was harmful. Old local rejection matters as well as
old source confidence. In world43 at byte8225, both books price the truth
better than P0, but the accumulated cold weight .710478 reduces effective
exposure from .975502 to .287369, losing1.097529 bits on that observation.
These signs are retrospective diagnoses of every observed event, not an
oracle the new predictor may use. [Full decomposition and raw witness](audit/MECHANISM.md).

## Decision

Preserve Sol's FAIL. No production repair is needed. The next separately
declared hypothesis in [PROTOCOL.md](PROTOCOL.md) revises local confidence
in both directions with one fixed transition. It leaves archived observations,
recipient counts and the outer mechanism intact. Only fresh worlds may decide
its live utility. Prior worlds are not a parameter-selection set.
