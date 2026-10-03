# Turn33 interface

`earned_router predict BANK_FULL POOLED_FULL PERMUTED_FULL < RAW` emits one
tab-separated causal row per raw byte. The row contains the HEAD256/P0 quote,
matched address and source truth prices, pre/post pooled permission, pre/post
real and permuted case log-wealths, candidate/live prices and complete outer
state for five arms. All 256-vector validation occurs before truth is read.

`earned_router --fixture` emits JSON lines for a frozen handcrafted sequence.
It does not read generated worlds or archives. `fixture_check.py` reconstructs
those rows with independent Decimal arithmetic.

`earned_router emit` and `earned_router trace` delegate unchanged to the
inherited turn13 episode program used by the frozen generator and extractor.

`experiment.py` has immutable stages:

1. `freeze`
2. `generate`
3. `extract`
4. `learn`
5. `evaluate`

Every post-freeze stage checks all frozen identities and its incoming manifest.
Outputs are creation-only. `verify.py --output PATH` is a separate reader: it
does not import the writer/evaluator or the C router and creates one receipt.
