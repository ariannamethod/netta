# Turn31 independent probability reader

2026-09-26. Written from the predata PROTOCOL.md and INTERFACE.md without
reading the new writer or C implementation. `verify.py` imports frozen
turn30/verify.py and its frozen turn28 helper. New factorized and balanced
arithmetic is implemented separately in probability space with 50-digit
Decimal state; both factored posterior ratios use the old u/v.

## What it reconstructs

- All source role tapes from raw byte/trace pairs. The frozen independent28
  rolling-context recount independently builds A/B counts and exact bank,
  pooled_full, pooled_small and permuted byte strings. The new comparison
  consumes the same bank prefixes in every arm; all count/source boundaries,
  costs and the 528-byte cap are checked.
- Raw truth-to-rank consistency, complete prior32 role history, longest
  prefix match and all five quoted source prices on every target event.
- Six candidate streams and states: factored u/v, frozen flat3, static
  balanced conditional mixture, pooled, factored permuted and cold. Router
  updates happen only after current prices; matched NEW/equal events tick
  share and unmatched events retain state. Every supplied state and quote
  must agree within1e-7; protected prices and admissions use exact checks.
- Positive normalized complete changed support with unchanged residual NEW,
  in addition to retained C full256 normalization and NEW checks.
- Incumbent-pooled admission shared by all six arms, then separate inherited
  slow outer laws; horizons1024/4096/8192/16384, prefix floor−1 and drawdown16,
  all arm/world/regime metrics, paired contrasts, retention, nulls and costs.
- F1 uses only partial[8192,16384). F2 includes both intact horizons and the
  entire partial life. F3 compares every regime. F4 is closed only after the
  independent reconstruction completes; the writer must leave it pending.
- Exact raw help/harm rows chosen by first strict extrema of received
  factored-minus-flat3, with the surrounding raw byte window.
- Freeze pins, stage manifests, strict trace headers, all40 unique lives and
  exactly **3,932,160** forecast events. Output is a new exclusive path only.

## Declared boundaries

The original greedy selection/grammar is supplied and pinned; its search is
not repeated. HEAD256 bindings and P0 prices are supplied trace inputs,
checked against observed bytes and each other. The full frontend and corpus
generator are not independently rebuilt. Full source distribution effects
are reconstructed from the changed repeat support and unchanged residual;
the C `new_exact`/`max_norm_error` fields remain supplementary implementation
evidence. Source-only selection, quote-before-read program order and hidden
generator labels are also reviewed by the other hands in their own scope.

Both-histories-worse subsets are a requested descriptive analysis of saved
rows, not a separate gate or target filtering rule, and are not needed to
redefine any primary reader comparison.

## One prefreeze arithmetic fixture

`FIXTURE_READER.json`: **20 events over2 records, 360 numerical checks,
maximum error3.552713678800501e-15, PASS**. It includes matched NEW/equal,
no match, poor source forecasts and a changed favored case. Reader and C
binary hashes are recorded. No fresh world was used.

The initial invocation was an inline Python adapter; the same comparison
body is retained as `fixture_check.py` for reproducibility. It imports the
reader, captures `case_router --fixture`, and refuses an existing receipt.
It is not imported by the production reader. It was not rerun simply to
change invocation form.

```sh
python3 -B turns/turn31/fixture_check.py --output /absolute/new/FIXTURE.json
python3 -B turns/turn31/verify.py --output /absolute/new/VERIFY.json
```

An arithmetically correct material FAIL is reported as verification_pass=true
and material_pass/gate_pass=false. A demonstrated artifact/state discrepancy
stops by name without overwriting sealed inputs or modifying a threshold.
