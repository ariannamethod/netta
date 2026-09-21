# Turn23 raw observations and lane decomposition

The C quote and control quote are paired on the same inherited candidate/P0
trace. `split−fast` below is the charged log2 difference for the true byte.
The samples are selected only to explain the completed FAIL, never to set
the already frozen rule.

| Target after seam | World / byte | Selected length | P0 | Candidate | Split−fast | Selected capital |
|---|---:|---:|---:|---:|---:|---:|
| switched, protection | 237 / 8276 | 4 | −0.882493 | −5.532165 | +4.397803 | long 0.0000000000083 |
| switched, harm | 239 / 8342 | 3 | −1.874508 | −7.085374 | −4.958228 | long 0.994798 |
| moved, protection | 236 / 13089 | 1 | −7.253431 | −15.700713 | +7.698224 | short 0 |
| moved, harm | 238 / 11861 | 4 | −3.428505 | −11.826503 | −8.283556 | long 0.999755 |

Across eight worlds, selected lanes contributed these realized split−fast
bits. The no-match lane contributes exactly zero. Mean mass is conditional
on a selected match and is not a probability of lane use.

| Regime/scope | Lane | Selected bytes | Mean lane mass | Split−fast bits | Sum candidate−P0 raw bits |
|---|---|---:|---:|---:|---:|
| intact/full | short | 40,876 | 0.0739 | −3669.387 | +3790.898 |
| intact/full | long | 52,041 | 0.9471 | +128.060 | +22504.513 |
| changed-law/tail | short | 21,853 | 0.0072 | −164.156 | −3314.776 |
| changed-law/tail | long | 6,711 | 0.0283 | −18.442 | −4907.051 |
| moved-surface/tail | short | 20,528 | 0.0533 | −1060.180 | −437.547 |
| moved-surface/tail | long | 23,877 | 0.9558 | +1546.964 | +7397.576 |

These are attribution sums, not causal intervention estimates. In
particular, each mode carries a different state history; one cannot add a
single quoted event to a counterfactual unchanged future and call that a
whole-life outcome. Full raw TSV.GZ traces and their SHA-256 receipts are
retained locally in `results/` and `RESULTS_MANIFEST.json`.
