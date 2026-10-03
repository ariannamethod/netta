# Turn34 independent reader

`verify.py --output PATH` imports no turn34 writer and no C code. It
recounts the four source lives' continuations per life, rebuilds all four
archives byte for byte (NETEB004 episode bank, NETEB001 case bank, pooled,
permuted), and replays every forecast of every arm with Decimal state at
precision 50: four episode wealths, the frozen turn33 two-case wealth for
the earned2 arm, pooled permission, shared admission, outer trajectories,
metrics and the G1–G5 gates. The frozen reader chain is imported, not
restated: turns/turn33/verify.py supplies Permission, Wealth, log_mix,
distribution and the shared outer law, which it in turn takes from turn30
and turn28; all of them are pinned in FREEZE.json.

Discontinuous classifications — silent versus active at exactly zero
wealth, and exact candidate equality — follow the recorded C double after
the independent reconstruction has agreed within the frozen 1e-7 tolerance.
This is turn33's disclosed reader-repair convention adopted into the law at
birth rather than repaired after a refusal; the three turn33 repair sites
are the precedent, READER_REPAIR.md of turn33 the boundary.

Receipt of record (VERIFY.json, this batch): verification_pass true,
3,276,800 forecasts replayed, 6,720 source counters recounted, max error
`3.3651303965598345e-11`, G1 true, G2 true, G3 true, **G4 false**, G5 true.
The writer's RESULT carries G5 false and gate_pass false by law — the
writer cannot certify the independent reader; the receipt completes G5.

The reader refused the first batch at its identity stage over a missing
frozen pin and wrote no receipt; that refusal is retained in
FREEZE1_FAILURE.md and the defective batch is preserved whole in the
session scratchpad.
