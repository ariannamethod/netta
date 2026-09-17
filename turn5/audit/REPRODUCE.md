# Incoming Sol turn4: fresh reproduction

2026-09-16, Astra measurement hand. Final implementation reproduces the numerical FAIL. No source, frozen turn4 output, threshold, generator or target was modified. Only this document and `audit/replay/` were written.

## Exact received state

- Fresh incoming checkout: `cdcd00f560aabebbfb0980399d6192016e1d0600`, branch `astra/turn5-revision`; clean at task start.
- Read-only source: `/Users/ataeff/arianna-codex/repos/netta-sol-local-cold-20260916`, HEAD `8f267f9e96eb8de09a0e7148c76d31e4a6383005`; clean before and after replay.
- Replayed pre-code protocol snapshot: `ddae0f4bf54b1d9611d2e35ad7773b3455e842c6`; only final `turn4/experiment.py` and `turn4/verify.py` were copied into the export.
- Protocol SHA256 `de4e2d9025e22e9bc1d2a4c2eb78f954ca930b23d07a8b76cd805ad8b67325bd`.
- Experiment SHA256 `478bf329f358694776b3a0bc2d49aec382567e370029891e93a9e71ee0e85399`; verifier `e5615f82003f80d91822282b135e42d3478377beae67ebeef42395089388af53`.
- [Snapshot/copy receipt](replay/REPLAY_INPUTS.json), [exact commands, exits and times](replay/REPLAY_COMMANDS.json), [complete raw process log](replay/REPLAY_RUN.log).

## Commands actually completed

Here `REPLAY_ROOT` is `/Users/ataeff/arianna-codex/repos/netta-astra-turn5-20260916/turn5/audit/replay`. The initial export used `git -C SOURCE archive ddae0f4` piped to `tar -x -C REPLAY_ROOT`; both processes exited0. Final scripts were copied with `shutil.copyfile`.

```sh
make -C "$REPLAY_ROOT/byte_recurrence" all
make -C "$REPLAY_ROOT/turn3" bank
python3 "$REPLAY_ROOT/turn4/experiment.py" freeze
python3 "$REPLAY_ROOT/turn4/experiment.py" generate
python3 "$REPLAY_ROOT/turn4/experiment.py" extract
python3 "$REPLAY_ROOT/turn4/experiment.py" evaluate
python3 "$REPLAY_ROOT/turn4/verify.py"
```

All seven commands exited0. Build, generation, extraction, evaluation and verification used only the prescribed historical worlds40..47. The new freeze is a separate replay receipt, not a replacement for Sol’s original repair chain.

| Stage | Seconds | Exit |
| --- | ---: | ---: |
| build byte | 1.869 | 0 |
| build bank | 0.522 | 0 |
| freeze | 0.065 | 0 |
| generate | 3.801 | 0 |
| extract | 48.978 | 0 |
| evaluate | 36.099 | 0 |
| independent verify | 198.194 | 0 |

## Equality with original evidence

- All16 source/binary pins in the new freeze exactly equal the original final `CODE_FREEZE_VERIFY.json` pins, including the freshly compiled executables.
- Data: **65/65 exact SHA256 matches** (56 raw streams, eight generation records, manifest).
- Memory: **57/57 exact matches**, including all16 HEAD256 books and eight14080-byte banks.
- HEAD256 traces: **113/113 exact matches**.
- Results: **48/72 exact file hashes**; the remaining24 Python event gzip files have different wrappers/timestamps, while **all24 decompressed TSV contents match byte for byte**. Original gzip files still match their historical manifest pins.
- Every value in `RESULT.json` matches exactly except the24 `event_sha256` fields identifying those newly wrapped gzip files. The entire numerical summary and all recorded per-life numeric fields are exactly equal.
- Independent `VERIFY.json` matches exactly after excluding only its `result_sha256` field. This includes its numeric error and all decisions.
- [Per-file hashes](replay/REPLAY_COMPARE.json), [exact numerical/metadata distinction](replay/EXACT_NUMERICS.json), [independent-reader comparison](replay/VERIFY_COMPARE.json).

Original RESULT SHA256: `9718a1ddee6c56d9fabc304ac34158bdbe1cfcf588b548cdcf27122185a1e881`.  
Replay RESULT SHA256: `f687879efe4ffb7a8e293948eeda92355d81ef448fd134e78b19460d9b2ce3fc`.  
Replay VERIFY SHA256: `133bc3a42398139aa23076f12fe74276a5e6f9fa19c9fa0011dcdbe719f2fa91`.

## Reproduced decision

Independent reader: **verification PASS; scientific gate FAIL**. It checked786432 recorded new-arm predictions, rebuilt source counts/books from traces, reconstructed the paired row2 comparator, and checked chronology, distributions, NEW and outer-HMM bounds.
Maximum numeric disagreement `1.3060343917459249e-09` bits; maximum normalization error `4.4408920985006262e-16`.

| Declared quantity | Independent replay value | Result |
| --- | ---: | --- |
| Mosaic gain, bit/raw byte | 0.010166100909556927 | PASS ≥0.0075 |
| Positive mosaic worlds | 8/8 | PASS |
| Retained paired row2 gain | 1.0781691319231097 | PASS ≥0.70 |
| Mean changed-tail improvement, bits | -1.6574576570507986 | FAIL >1 |
| Improved changed tails | 1/8 | FAIL ≥5 |
| Worst-tail improvement, bits | -1.3848282063761843 | FAIL >1 |

| World | Changed-tail improvement row3−row2, bits |
| --- | ---: |
| 40 | 1.8216727575942286 |
| 41 | -3.8468547088248783 |
| 42 | -2.3256193250232258 |
| 43 | -4.7488198861548767 |
| 44 | -0.24088981541103749 |
| 45 | -1.3848282063761843 |
| 46 | -1.790390100040824 |
| 47 | -0.74393197216959095 |

The evaluator’s independent arithmetic has the already-existing final-digit difference from the reader (writer gain0.010166100909556970 versus reader0.010166100909556927). The writer reproduces the original writer exactly; the reader reproduces the original reader exactly. This is not a new replay discrepancy.

## Historical repair chain and its limit

- Both `repaired_from_sha256` links are exact: original freeze `88c2cad79b2d17250bf34375448ce2a1e73028228c1c4e8e04d75f8adad240db` → repair freeze `7d529a8bc84b67b86399108c5430821c50edf3b610ad1d7d872bb1538d9347cd` → verify freeze `7beeea10374aeea12054beb338d18629ffe734a8d04fd9d6542f50527937c2da`.
- All three freeze records pin the same protocol and namespace. Only experiment.py/verify.py pins change at each transition; all other14 pins are unchanged. All16 final pins match the received source files.
- The documented second repair is verifier/schema-only in prediction semantics, but **both script hashes changed**. The final experiment includes `verify_freeze` and selection of the final verification receipt; hash equality alone does not prove that this was the sole historical edit.
- In the explicitly inspected `turn4/failed-run-1/`,50 preserved files exist and none are `.py`, `.c` or `.h`. Git has the pre-code protocol commit and the final implementation commit, without intermediate turn4 source snapshots. The complete historical source diff is therefore not recoverable from these inspected artifacts. No wider archive search was performed.
- A bounded comparison of preserved world40 events supports the observable numerical repair: full mosaic32768 arm-event rows and unrelated8833 rows through `t=4416,row3` retain identical event identities, component prices, posterior diagnostic weights and capitals. Maximum before/after differences are2.2737367544323206e-13 and5.684341886080802e-14 respectively, confined to mixture/accumulation rounding.
- [Original receipt-chain check](replay/ORIGINAL_RECEIPTS.json), [preserved prefix comparison](replay/PARTIAL_PREFIX.json).

## Conclusion

No reproduction mismatch or new numerical defect was found. Sol’s final implementation and independent reader reproduce the declared FAIL. The prior step preserved useful mosaic transfer but did not improve the changed tail under its frozen conditions. This audit authorizes no tuning of worlds40..47 and makes no additional semantic or live-system claim. The bounded incoming reproduction is complete.
