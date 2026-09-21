# Incoming turn15: numeric verification

2026-09-21, Astra measurement hand. Source checkout read-only:
`/Users/ataeff/arianna/netta-don-turn15-20260919`; branch `don/turn15-surface-move-20260919`,
HEAD `3cc77c19dbfb0e2584e231d652f833beef43a044`. Source `git status --short`
was empty before and after. New evidence is confined to this audit directory.

## Verdict

**No numeric or frozen-code identity mismatch found.** One pristine execution
of Don's reader exited0. Its output is **byte-for-byte identical** to the
sealed `turn15/VERIFY.json`; zero differing JSON fields, no ignored fields.
Verification PASS and material PASS16/16 reproduce.

- Fresh receipt: `AUDIT_READER.json`.
- Receipt and stdout SHA256: `41da6ad1f9185a081db3a57773ab62baa9840926add213bc77c82f174a14d0e0`.
- Captured stdout/stderr: `reader.log` (identical JSON output; no error text).
- Reader SHA256: `67c77f148992f6b5143f4b164ff02f8c186e8b757f1d1e8de9d9d55ccaa1e4d7`.
- RESULT SHA256: `e545c87073070c41dede62cf71d12f734717a3f2178d2f5c95be9dbdb47a5a90`.

Exact execution (rc taken from the process, no pipe):

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /Users/ataeff/arianna/netta-don-turn15-20260919/turn15/verify.py --output /Users/ataeff/arianna-codex/repos/netta-astra-turn16-20260921/turn16/audit/AUDIT_READER.json > /Users/ataeff/arianna-codex/repos/netta-astra-turn16-20260921/turn16/audit/reader.log 2>&1
```

## Integrity and scope of the rerun

All15 frozen files match. The10 file pins shared with turn14's freeze match
exactly. All280 data,64 memory and96 result-manifest entries match.

| Receipt | SHA256 |
| --- | --- |
| FREEZE.json | `3bbd9bfa8beda5a169d3bb00922a39eed3d0740f2b4fe813aa338e99caa0f541` |
| DATA_MANIFEST.json | `4bcad122480c5461d71dd54163b5a74e0dfd1b2bf9e096989bcb2cda875207b5` |
| MEMORY_MANIFEST.json | `43152e68d4d87ef7ff1c2b0d4237710e91722e2635c323dd2a94fbd552ca16e1` |
| RESULTS_MANIFEST.json | `1c983723feef456f0b613f3ea198c458f5ecc95ba686ab260f41ebcc48160d9a` |

The run checks524288 source observations and524288 target observations,
3670016 inherited candidate forecasts, and7340032 paired slow/fast forecasts.
These counts describe overlapping checks of the same target bytes, not extra
independent worlds. Maximum numeric discrepancy is
`3.836362338915933e-10`bits against1e-7; maximum normalization error is
`1.3322676295501878e-14` against1e-8.

The turn15 reader independently reconstructs the bijection from its seed and
compares all transformed bytes (`turn15/verify.py:72,84`). It invokes the
frozen independent turn13 source grammar, joint choice, exact archive and
candidate reconstruction (`turn13/verify.py:232,511,633`), then the independent
Decimal probability-mass replay of both hazards (`turn14/verify.py:62,113`).
It does not import a writer. Every raw target price, order and admission is
checked. Source budget is65536 observed events per world; each episode
archive uses528bytes.

The inherited frontend boundary remains precise: context bindings and sparse
P0 prices are supplied by retained traces. This rerun does not rebuild
HEAD256 segmentation or all256 cold probabilities. It independently rebuilds
the archives, matches and role-level candidate normalization; full-vector
normalization/NEW receipts come from the frozen C quote path. This was one
reader rerun, with no training/generation repeat or perturbation probes.

## Reproduced result

Whole-life renaming yields exactly0 difference in full/4096 gains for all
seven arms, both authority laws and all eight worlds; admission bytes match.
All renamed raw streams and unchanged8192-byte mid-move prefixes satisfy the
recorded manipulation. Fast episode mean changed-tail gain is
`592.792981199085`bits, positive8/8. Mean full-life rate is
`0.10428254615410`bit/raw-byte, positive8/8. Slow exceeds fast on
each of these eight changed tails, by
`9.022643640852`bits in mean.

| world | fast changed tail | slow changed tail | slow−fast | fast whole life | admission byte |
| --- | --- | --- | --- | --- | --- |
| 160 | 350.904333382826 | 355.754522044738 | 4.850188661912 | 1463.765101519817 | 126 |
| 161 | 1014.556534388437 | 1021.212662342307 | 6.656127953870 | 2454.079594134582 | 120 |
| 162 | 607.370863049412 | 618.554284840803 | 11.183421791392 | 1939.507533618450 | 131 |
| 163 | 1.690559420331 | 16.305399713478 | 14.614840293147 | 432.300062151116 | 130 |
| 164 | 887.911410375273 | 893.260401005665 | 5.348990630392 | 2496.919365551871 | 101 |
| 165 | 338.956956128859 | 346.037846211872 | 7.080890083013 | 924.319461701255 | 149 |
| 166 | 910.299192256560 | 921.562285329788 | 11.263093073228 | 2279.309271191935 | 83 |
| 167 | 630.654000590987 | 641.837597230847 | 11.183596639860 | 1678.321499641712 | 106 |

Permuted never admits in any regime or mode. No unrelated arm admits.
Worst complete-prefix gain is `-0.9994519360420785`bits;
worst fast drawdown `8.939953270717865`bits;
worst slow drawdown `14.936315621750737`bits.
The frozen one/ten/sixteen-bit bounds hold.

The16 recomputed conditions are:

- `c1_bijection`: PASS.
- `c1_mid_divergence`: PASS.
- `c1_prefix_identity`: PASS.
- `c1_whole_divergence`: PASS.
- `c2_equivariance_admission`: PASS.
- `c2_equivariance_early`: PASS.
- `c2_equivariance_full`: PASS.
- `c3_recovery_tail_count`: PASS.
- `c3_recovery_tail_mean`: PASS.
- `c4_whole_life_gain`: PASS.
- `c4_whole_life_positive`: PASS.
- `c5_permuted_never_admitted`: PASS.
- `c6_fast_drawdown`: PASS.
- `c6_normalized_new_exact`: PASS.
- `c6_prefix_bound`: PASS.
- `c6_slow_drawdown`: PASS.

## Raw help and harm inspected directly

Besides the reader's exhaustive raw-extrema comparison, these actual trace
records were inspected against their raw `.bin` byte and paired authority row:

- `results/world161/moved_mid.tsv.gz`, t9861: raw byte76, rank2, heads174/76/199,
  history `02022310231003100331222103102331`, matched length2, record1,
  votes `[4594,224,2464,501,0,0,0]`. Cold log2 price−7.875385027363873,
  candidate−1.628344131705410, fast live−1.781623680216955:
  **+6.093761347146917bits**. Fast pretruth log2 odds3.1359311382578965.
- `results/world167/moved_mid.tsv.gz`, t15420: raw byte38, rank4,
  heads125/217/98/38, history `13130112310023330122331002311203`, matched
  length1, record2, votes `[5218,5215,1477,4375,0,0,0]`. The observed fourth
  repeat has zero source count, so only KT smoothing protects it.
  Cold−4.523927071331939, candidate−15.951838973098470,
  fast live−12.446916125562995: **−7.922989054231056bits**.
  Fast pretruth log2 odds8.050064558634052.

Paired rows are in `results/outer/world161/moved_mid.tsv.gz` and
`results/outer/world167/moved_mid.tsv.gz`, `arm=episode` at those same t.
The quoted values match Don's RAW.md examples.

## Interpretation needed for the next hand

Admission happens once per life. After the surface seam the already-active
mixture can regain source posterior weight; it does not execute a second
32-bit admission. All episode admissions here precede byte150, far before the
seam. The smallest positive fast tail is world163 at1.690559420326bits, versus
16.305399713480slow, so that case should remain visible.

The measured slow/fast ordering above is established for this surface-move
batch. No new law-change recipient was generated or tested in this audit.
The stored addresses are exact relation strings, and this experiment measures
bijective surface renaming and its mixed-prefix recovery. The raw harm and
the supplied-frontend boundary remain part of the result handed onward.
