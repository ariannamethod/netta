# Turn22: independent incoming reader audit of Don turn21

2026-09-21. Astra measurement hand. **Verification PASS; incoming material FAIL is preserved (8/10 named conditions).** No numeric mismatch remains under the disclosed repaired selector. The stronger exclusion of the entire static-level class is not established.

## Exact state and one rerun

- Read-only incoming checkout: `/Users/ataeff/arianna/netta-don-turn18-20260921`, branch `don/turn21-ceiling-frontier-20260921`, HEAD `8bf5c379d343cf2be246381687fa14e2ef20a788`. Clean before and after.
- Own checkout: `/Users/ataeff/arianna-codex/repos/netta-astra-turn22-20260921`, initial HEAD `bcc1881ee78c76d97645588bfae54e950cf4ad3e`.
- One execution of the supplied repaired reader, exit **0**. The frozen failed reader was not rerun. No fresh data, rebuild, perturbation probe, source edit, commit or push.

```sh
python3 -B /Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn21/verify_repair.py --output /Users/ataeff/arianna-codex/repos/netta-astra-turn22-20260921/turns/turn22/audit/INCOMING_READER.json > /private/tmp/netta-turn22-incoming-reader-20260921/reader.log 2>&1
```

The fresh [INCOMING_READER.json](INCOMING_READER.json) is **byte-for-byte identical**, 94,061 bytes, to sealed `turns/turn21/VERIFY.json`:

`5cdf4b031658744bfc6139f66b4e075bfdd52e88a14cf96b297011bd6d84c4a3`.

Command-log SHA256: `b8890b99b1d4ba1e59bc27fb076c58d427dfefad3c1763e1751e3c1b3887ef9a`.

## Identity and repair provenance

An initial direct SHA256 pass and the supplied reader both accepted all **30 FREEZE pins**, **312 data**, **64 memory**, and **128 result** manifest entries.

| Artifact | SHA256 |
| --- | --- |
| PROTOCOL.md | `06243c2ba6254e92b4627091eaaa86edaf072b38ac5cd9635611f4c25d30a27a` |
| FREEZE.json | `e59c454e2352ff596aa621c98742911b92a78599423186d581bdeeb04a52891f` |
| RESULT.json | `70ea594706515ea2942291d6d29beb1b36346cbecb147fc56d69aa4d763cae4b` |
| Frozen verify.py | `fa8ede135064e345169553c9b1714139b3b26e9208a90629de2ad7592a3cff07` |
| Actual verify_repair.py | `38ff350f9a78fe6285ea6a7cd3dbe54baef4b010bb5ab00994b5ce437b9235cb` |
| Preserved failed VERIFY.log | `489386d5e5de9c8fc7518eabc52f740f49aa416d8c059e4bb893a47c47094398` |

The full reader diff was inspected. It adds a disclosure docstring and one approximate lexicographic selection helper, used at two call sites. Source reconstruction, forecasts, authority equations and numeric gate bars are unchanged. The original reader remains intact and pinned; the fresh receipt identifies the actual repaired reader separately.

The failure log ends at `AssertionError: nominated point`. The original protocol omitted a tie tolerance and the final grid-index fallback. `BUILD_NOTES.md:194` and `:460` disclose those additions. The corrected nomination treats first-key differences within `1e-7` as tied, then compares the second key, then keeps earlier grid order. This is a post-data clarification, not an originally complete preregistration.

Specifically, writer first-key values for `h8l3/h8l4/h8l5` all equal `-0.9155676528937813`; the second key favors `h8l4/h8l5` (`5.680677761061602`) over `h8l3` (`5.627737609240754`). Grid order then selects `h8l4`. The sealed repaired reader has a first-key spread of about `1.42e-14`; the `4.263e-14` in BUILD_NOTES is its separate diagnostic using writer horizons. All three alternatives have only 3/8 worlds above the law-tail bar, so this repair changes no material verdict.

## Reconstructed result

- **3,670,016** candidate forecasts and **14,155,776** authority forecasts rebuilt over 8 worlds × 4 regimes; 7 candidate arms and 27 authority modes.
- Maximum numeric disagreement **2.5136159820249304e-10**, tolerance `1e-7`.
- Maximum supplied full-vector normalization residual **1.354472090042691e-14**.
- W1 window: exactly `h8l2,h8l3,h8l4,h8l5`; W2 nominee `h8l4` fails robustness.
- W3 endpoint conditions pass **4/4 low rows** in both directions.
- W4 fails only `unrelated_never_admits`; shared admission, archive cap, prefix/drawdown, normalization and protected-price checks pass.
- W5 reader checks **5/5**. Both `gate_pass` and `complete_gate_pass` remain false.

Across all 864 mode-lives, smallest recorded prefix gain is **−0.8924434266374619** (world221/moved_mid/slow); largest drawdown **14.775309661552** (world223/recombined/slow). These satisfy the declared whole-prefix and interval limits.

## The nominee: average clearance and individual failures

Independent direct summation of `h8l4_live-fast_live` in retained `results/authority/world*/switched.tsv.gz`, `t>=8192`, gives mean **−0.9155676528938587** bits and **3/8** above the `−1` bar. Maximum difference from RESULT's paired values is `7.50e-13`.

| World | Law tail: h8l4 − fast, bits | At least −1? |
| --- | ---: | :---: |
| 216 | −2.180489033508 | no |
| 217 | −0.125105087720 | yes |
| 218 | +2.247630703957 | yes |
| 219 | −1.666642391255 | no |
| 220 | −2.587799822600 | no |
| 221 | +0.107852594354 | yes |
| 222 | −1.502461560546 | no |
| 223 | −1.617526625834 | no |

Only two worlds are positive. The mean clears its bar by **0.0844323471062 bits**. Other reported h8l4 means are confirmed: law whole-life versus fast **+5.680677761062**, moved tail versus fast **+6.704898499226** with **8/8** wins, moved tail versus slow **−1.716995811883**, recombined early/full retention **0.996439342846 / 0.997177020601**.

## Raw unrelated admission, world218

Both retained files agree: `results/world218/unrelated.tsv.gz` and `results/authority/world218/unrelated.tsv.gz`. Row numbers below use the files' zero-based `t`.

| t | Truth/rank | Match length | Shadow before | Candidate − cold | Active before | Admitted after |
| --- | --- | ---: | ---: | ---: | ---: | ---: |
| 1529 | 100 / NEW | 1 | 31.871557900367602 | 0 | 0 | 0 |
| 1530 | 219 / 2 | 1 | 31.871557900367602 | +0.5878926477639297 | 0 | 1 |
| 1531 | 120 / 3 | 1 | 32.459450548131528 | +0.3038276708533250 | 1 | 0 |

At crossing `t=1530`, cold and every live quote are still exactly **−3.1131864854605933**. At `t=1531`, the first active h8l4/witness price is **−3.2676626630167536**, versus cold **−3.4275599223014428**. Thus `activation=1531` means 1,531 bytes consumed at crossing; the first active forecast has index `t=1531`. No outcome-before-price error is visible in this witness.

Only this one of the eight unrelated lives admits. It ends with positive h8l4 gain **+12.051638608942007**, minimum **−0.2527966902884917**, drawdown **7.983812607543065**. The failing condition is zero unrelated admissions, not a negative final gain. The earlier-batch “first ever” claim was not recounted in this bounded audit; 1/8 here is not a calibrated population false-positive rate.

## What this audit establishes

The reader independently reconstructs source books and seven candidates through the frozen turn13 implementation, recounts matched suffix lengths, then replays every authority mass/clock/hazard/cap against the stored traces. Chronology, protected NEW/equal-price/inactive quotes, shared first admission and the reported statistics are checked. **HEAD256 bindings and P0 remain supplied trace inputs; the independent reader does not rebuild the frontend's complete byte distributions.** The normalization field is a C receipt, supplemented by independent normalization of the authority mixtures.

`REPORT.md:26–30` and `:88` overstate the measured result. The experiment establishes a nonempty mean window and a failing nominated point within **these 24 fixed high/low pairs, this witness clock, and these eight worlds**. It does not exclude every fixed pair or the entire static-level class, nor establish that a world-driven ceiling must win. The finite grid and per-world counterexamples justify investigating that hypothesis. They do not prove the universal conclusion.

The incoming evidence and its two material failures stand. No repair to predictor, artifacts or sealed reader files was required or made by this audit.
