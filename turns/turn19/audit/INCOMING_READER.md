# Incoming Don turn18 — independent reproduction

2026-09-21, Astra measurement hand. **Mechanism verification PASS; material
FAIL17/20 reproduced exactly.** No frozen-code or numerical mismatch found.
The fresh output is byte-for-byte equal to the preserved VERIFY.json, with
zero differing fields and no excluded metadata.

## Received layout and execution

Read-only source: `/Users/ataeff/arianna/netta-don-turn18-20260921`, branch
`don/turn18-witness-clock-20260921`, HEAD
`29c384e0a02ab65c2cb111ddbd3065e1f71a514b` (pure relocation under `turns/`).
The verifier requires the original root-level paths, so only its32 required
tracked files were reconstructed with `git archive` from
`5f3970ae348e2460e78651f5160c937fdbd60bed` into
`/private/tmp/netta-turn18-pristine-6xmxgtyl`. Existing data/memory/results and the two ignored frozen binaries
are read through symlinks to Don's checkout. No pin, code, relative path,
manifest or artifact was rewritten. Don's tracked worktree stayed clean.

All28 freeze pins pass. Data312, memory64 and results128 manifest entries
pass. No learning, target generation, parameter changes or integrity probes
were repeated. Python bytecode writing was disabled.

One execution, direct process exit0:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 /private/tmp/netta-turn18-pristine-6xmxgtyl/turn18/verify.py --output /Users/ataeff/arianna-codex/repos/netta-astra-turn19-20260921/turns/turn19/audit/INCOMING_READER.json > /private/tmp/netta-turn18-pristine-6xmxgtyl/reader.log 2>&1
```

| Identity | SHA256 |
| --- | --- |
| Fresh and sealed verifier outputs | `bce928ced50ac8b00c7624cc19bd5b81f297a1ed442e9abfd135597e09dd7a6d` |
| Original verifier | `b801a95c3f06be3f3e83deac0da4e89c4f700262b9b84220a7d0f9167e27b444` |
| RESULT | `ee3423fb7388831fb394aff901542b4b59ef543988d1949ef210a44f50dc0b86` |
| FREEZE | `6d3f2f6be82858737a52f2d67690814c6657b4a193a0a428f1b60d0e60d788cb` |
| Data manifest | `bb5fa9b8850030316969d5f71db91b7dbecff40081c799993b33e9f8945ce454` |
| Memory manifest | `071a2074df96d54058241049fbbe8d8708ed6506788baf6d3b0746cc9a7d6f0f` |
| Results manifest | `befa4a65c577f188831a7094b4764ce272fe8dd2aaad0fbf2a4aa6efccb047ab` |

The complete reconstruction record and stdout remain in the named scratch
folder. The two durable audit files are this note and INCOMING_READER.json.

## What reproduces

- 3,670,016 inherited candidate forecasts: independently reconstructed source
BPE/joint selection, seven exact archives, histories, matches and prices.
- 2,097,152 episode authority forecasts: four Decimal probability-mass modes,
every quote, admission, hazard choice, matched-length sequence and clock.
- Maximum numeric discrepancy `1.3153567124390975e-10`bits against1e-7;
normalization `1.3100631690576847e-14` against1e-8.
- Exact equal-price and protected-NEW conditions pass for the four new modes.

The three false gate fields and recomputed means are:

| Condition | Observed mean, bits | Required |
| --- | --- | --- |
| c2_law_vs_fast_retention | -5.068319062262 | >=−1 |
| c2_law_vs_slow_mean | 0.193982660525 | >1 |
| c3_law_vs_adaptive_mean | 0.241149883297 | >1 |

Witness exceeds adaptive on all8 law tails, but by only
0.241149883297bits in mean. On surface tails witness exceeds
fast by7.084011914562bits and trails slow by
1.920293241184bits. The two declared clock conditions pass:
7/8 law offsets are below256;6/8 surface tails use fast hazard at most25%.

The retained RAW.md examples were also read from authority TSV rows and raw
bytes: world195/moved_mid/t11100 is byte248, matched2, w_before−1.8981749893552051,
witness−slow=+0.7704626204023413bits; world199/moved_mid/t14298 is byte11,
matched1, w_before6.6286118067616604, witness−fast=−5.890811477429228bits.
Both help and harm are present in the preserved prices.

## Corrections and claim boundary

1. REPORT and handoff call the median law response36bytes. The supplied
   offsets `[40,28,21,65,32,311,120,37]` have median**38.5**. The7/8 gate
   count is unaffected. No sealed report was edited.
2. The exact seam means read from TSV are witness11.001889930686746,
   fast4.975393083446392, gap6.026496847240353bits. The per-world gap ranges
   6.011654514636155–6.058715366469647. This is close to the six-bit log2
   hazard ratio; it is not an exact identity with that ratio.
3. The data establish the behavior and failure of these specified clocks.
   They do not exclude every withdrawal-rate construction. For prior odds O,
   observed likelihood ratio R and hazard h, the actual update is

   `O_next = (1−h) O R / (1 + h O R)`.

   Thus h also determines a level ceiling `(1−h)/h`. The report's literal
   claim that hazard controls rate but never level is too broad. Already
   charged prices cannot be refunded; the saved-price split is a useful
   accounting diagnosis, not a proof excluding all other prospective rules.
   Likewise, one future ceiling failure would not exclude the entire class
   of level controls.
4. The operational witness is **matchedL>=1**. It is not identical to S≠C:
   a matching record can coexist with protected NEW or ineffective valid-rank
   votes and quote exact P0. Such matched zero-delta observations still decay
   w by31/32. The code matches the declared predicate; the prose interpretation
   of an actual nontrivial vote should retain this distinction.

HEAD256 bindings and sparse P0 are inherited trace inputs. This rerun does
not independently regenerate segmentation or all256 local probabilities.
Full-vector receipts come from the frozen C path; source archives, candidate
matching and the new authority/clock calculations are independently replayed.
No worlds200+ or new mechanism was created by this audit hand.
