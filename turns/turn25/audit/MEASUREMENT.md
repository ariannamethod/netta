# Incoming turn24: measurement audit

2026-09-22, Astra measurement hand. **Independent rerun PASS; material FAIL via W3 is reproduced exactly.** No incoming code or artifact was edited.

## State and one execution

Incoming read-only checkout: `/Users/ataeff/arianna/netta-don-turn18-20260921`, branch `don/turn24-oracle-latency-20260922`, HEAD `798085be9a2bf8b6c41860238d4f38450558e126`. It was clean before and after this audit. The own turn25 clone starts at the same commit.

I read the current global agreements and mandatory bootstrap, handoff, protocol, report, amendment and the reader. Its only filesystem write is the requested output file (`verify.py:645`); imports are standard-library only. `-B` was used, with an absolute fresh output outside Don's tree.

```sh
/opt/homebrew/opt/python@3.14/bin/python3.14 -B /Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn24/verify.py --output /Users/ataeff/arianna-codex/repos/netta-astra-turn25-20260922/turns/turn25/audit/INCOMING_READER.json
```

One direct subprocess execution: **exit 0**, **95.945050958 seconds**. Combined stdout/stderr and exact command/exit metadata are retained in `INCOMING_READER.log` and `INCOMING_READER.receipt.json`.

The fresh receipt is **byte-for-byte equal** to Don's sealed `turns/turn24/VERIFY.json`, **158,574 bytes**, SHA256:

`80f14387b38cf3b335b810c13a0d34333e520da48506dd5330c7e22437317562`.

## Freeze amendment reconciliation

The reader does not itself check FREEZE. I separately hashed all **36** entries: **34** exactly match their original pins, and **2** exactly match the amendment. No third state occurs. All amended entries' `frozen_sha256` values match FREEZE.

| Artifact | SHA256 |
| --- | --- |
| FREEZE.json | `1f51a07631fa4a189185889c630e1ab1f05c4267dfd1220b7047fb3d9a0b3a27` |
| FREEZE_AMENDMENT.json | `8fb1c4d016356b6232f4cd8259858f4ff759f2ddf5cc324eec255828025975bd` |
| Actual verify.py | `b196a2dfe05077c3e6d83a3cbd28dcad7b60a5a53bc68187741e59dd9e1a4ade` |
| Actual experiment.py | `d8b382abd1637bdbbb3a0404f538b8ebdfd754f76e4b722e966f37c73ef74417` |
| PROTOCOL.md | `856c829e3433d97323fdda679e321bb437b3380da8ce493fbd5c6473e7f12d33` |
| RESULT.json | `5ab0f12250cf7a0571c8a6bc8336d58ec4e47e9706908749255ebb2cd4c322f6` |

I reversed the amendment's recorded unified hunks **in memory**, without writing either version. Both reconstructed pre-freeze hashes exactly match their original pins: writer `6fd697a2b9dab3de8c981cdbc9b27b7861e5270e4cc233948650799b6fb90e2b`; reader `6264d47d72278969e59c06fbc7d39f35c7f77f2672d1e238e79e303474f9eee8`.

The diff has three hunks: exclude the nonexistent `moved_mid.commands.bin` from the command-tape comprehension in each script, plus the writer's amendment-aware freeze checker. No forecasting law or gate bar appears in those changes. The statement that RESULT did not yet exist at repair time is recorded provenance in the amendment; the present-day audit directly verifies the file states and diff, not historical filesystem absence.

The successful reader checked the data/extract/memory/labels/results manifests with **248 / 312 / 64 / 1 / 256 entries**, respectively. It independently reconstructs the declared surface bijection and command-tape seam used by oracle labels.

## Reproduced result

- **524,288 byte observations; 7,340,032 arm forecasts**, 32 lives × 16,384 bytes × 14 arms.
- Maximum numeric disagreement: **1.0345502232667059e-10**, below `1e-7`.
- W1, W2, W4, W5 and W6 pass; **W3 alone fails**. RESULT's W6=false is its declared pending-reader convention; VERIFY's W6=true resolves that status.
- All eight hygiene fields pass, including shared admission and the stronger fast-specific drawdown bound. No unrelated admission in this batch.
- RESULT contains exactly the 32 expected distinct `(world,regime)` pairs.

| Delay | O-rate law-tail − fast, mean bits | Rate bars | O-level law-tail − fast, mean bits | Level bars |
| ---: | ---: | ---: | ---: | ---: |
| 0 | +0.274205840841 | 8/8 | 0 | 8/8 |
| 32 | −0.080216664334 | 8/8 | −0.438319544837 | 8/8 |
| 128 | −0.181715525132 | 8/8 | −1.395859760799 | 7/8 |
| 512 | −1.245014307893 | 7/8 | −2.762508650549 | 7/8 |

The sole broken bar at these failing delays is law-tail mean. Largest passing **tested** delays are level32 and rate128. O-level-0 additionally has law whole-life advantage **+10.559739387130335**, moved-tail advantage **+10.9249327857091** with 8/8 wins, and intact retentions exactly 1. The attainable oracle result is concrete.

Level-minus-rate law-tail means at delays0/32/128/512 are **−0.274205840841 / −0.358102880503 / −1.214144235667 / −1.517494342656**. Positive per-world level advantages occur in **2/8, 2/8, 1/8, 3/8** worlds. These saved per-life measurements support the reported W3 diagnosis.

## Raw witness clock: the relevant timing evidence

Directly read from each retained `results/policies/world*/switched-controls.tsv.gz`, using `witness_slow_used`. These are observations of the existing mechanism, not a new replay or proposed mechanism's results.

| World | First fast index before seam | Fast bytes before seam | First fast offset at/after8192 | Tail fast→slow returns |
| --- | ---: | ---: | ---: | ---: |
| 240 | 582 | 48 | 53 | 52 |
| 241 | 139 | 776 | 20 | 203 |
| 242 | 230 | 633 | 0 | 77 |
| 243 | 289 | 820 | 0 | 75 |
| 244 | 114 | 186 | 49 | 34 |
| 245 | 278 | 1330 | 159 | 154 |
| 246 | 291 | 1745 | 158 | 127 |
| 247 | 154 | 1405 | 418 | 200 |

The operational post-seam first-fast median here is **51 bytes**; zero means the arm can already be fast at the seam. The quoted **38.5** is the earlier turn18 statistic, not this batch's measurement. Three current offsets exceed128. Every world already has pre-seam fast episodes. Many tail returns to slow directly confirm the unlatching phenomenon.

Consequently, `REPORT.md:44–51` does not establish that detection played no role in the four prior failures. A past median and a tested oracle delay do not isolate that causal explanation. The oracle knows which regime changed and avoids every pre-seam trigger; an actual latch must handle the observed earlier triggers as well. The delay set `{0,32,128,512}` establishes the largest passing sampled delay, not a precise continuous latency boundary or a demonstrated causal winner. These distinctions leave W1/W2 and the measured unlatching evidence intact.

## Exact reader scope

- `verify.py:336` starts from retained P0, episode candidate, match length and rank. It independently replays all 14 authority laws and admission; it does **not** rebuild source books, learned-unit frontend, candidate distributions or matched suffixes. Hashes bind those inputs.
- The 13 scalar arms use Decimal probability masses. `Split` (`:204–248`) independently uses float portfolios and `math.fsum`. “All fourteen arms in Decimal” would exceed the implementation.
- It checks per-byte prices, selected hazard/odds/clamp fields, oracle identities, and gain/tail/early/minimum/peak/drawdown. It rebuilds all bars and W-tests. It does not directly compare every descriptive RESULT quantity, every horizon, or every reference state column; this audit separately inspected the delay curve and timing rows above.
- Full-vector normalization is the retained C receipt. A label-free reconstruction matching the six reference trajectories demonstrates label-free reproduction on these tapes. It does not by itself prove all possible executions of the C program are label-independent.
- The four CSV streams are iterated with `zip` and the resulting count is checked against N (`:368–445`); extra trailing rows in a longer individual stream are not independently rejected there. No mutation campaign was run. The existing result covers all declared lives and is manifest-bound.

There is no numerical disagreement with sealed VERIFY. The substantive audit correction concerns the timing interpretation and the reader's exact scope. Incoming FAIL and amendment history are preserved; no new worlds or code changes were made.
