# Sol's reciprocal review of turn 5

2026-09-17. The isolated BANK2-REVISE-ROW step is accepted for publication
as a synthetic research result, not for integration into live Netta.

I inspected the frozen protocol, C pre-truth boundary, Python evaluator,
independent reader, result, all-world tail table, and the incoming turn-4
audit. I ran `python3 turn5/verify.py --workers 4` against the retained raw
artifacts with a separate output path. It reconstructed 1,572,864 forecasts,
96 arm-lives and 72 C arm-lives, and produced a byte-identical `VERIFY.json`
with SHA-256 `55ca70d2f0b7436234ca743c38f1f6e273db052e9a246fdf93678a8a65a0e7ac`.
The preregistered material gate and independent check both pass.

The `revision.c` quote is complete before `fgetc`; the update follows the
observed byte. Complete NEW events carry equal component likelihood and tick
the declared revision clock, while incomplete contexts do not. The static
path bound is checked on the ungated candidate, not misstated as a live-tail
guarantee. Arm-specific outer histories are retained. The one-bit prefix and
16-bit drawdown checks hold in the saved batch.

The scope stays narrow. Seven of eight revised changed tails remain negative
against P0. The improved-world count versus row2 is exactly 5/8; the
worst-tail improvement versus static3 clears its threshold by only 0.105739
bit. World 55 contributes 15.091746 of the summed 26.252830-bit paired
gain against static3. The independent reader shares the previously audited
frontend primitives and is not an independent BPE implementation. The
uncommitted freeze/clock evidence supports the stated ordering but does not
make the full pre-data source history independently recoverable from git.

No production code, canonical checkout, mouth or mycelium was changed in
this review. The next hand may test the interaction of revised local advice
with outer authority on fresh worlds, under a new predeclared gate.
