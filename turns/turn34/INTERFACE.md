# Turn34 interface

`sel4_router predict BANK4_FULL BANK2_FULL POOLED_FULL PERMUTED4_FULL < RAW`
emits one tab-separated causal row per raw byte. The row carries the
HEAD256/P0 quote, matched address, per-life and per-case source truth prices,
pre/post pooled permission, pre/post four episode log-wealths for the real
and permuted banks, pre/post two case log-wealths for the frozen earned2
control, candidate/live prices and complete outer state for five arms. All
256-vector validation occurs before truth is read.

`sel4_router --fixture` emits JSON lines for a frozen handcrafted 28-step
sequence; it reads no generated worlds or archives. `fixture_check.py`
reconstructs those rows with independent Decimal arithmetic.

`sel4_router predict33` and `--fixture33` expose the included frozen turn33
organ unchanged, as provenance probes: the staged copy differs from
`turns/turn33/earned_router.c` only in the renamed entry point, and the
Makefile fails if that drift exceeds the two expected lines.

`sel4_router emit` and `sel4_router trace` delegate unchanged to the
inherited turn13 episode program used by the frozen generator and extractor.

`experiment.py` has immutable stages: `freeze`, `generate`, `extract`,
`learn`, `evaluate`. Every post-freeze stage checks all frozen identities and
its incoming manifest. Outputs are creation-only. `verify.py --output PATH`
is a separate reader: it does not import the writer/evaluator or the C
router and creates one receipt. Its discontinuous classifications follow the
recorded C double after independent reconstruction within the frozen
tolerance — the turn33 disclosed repair convention, in law from birth.
