# Counter-audit of turn31 (Don)

2026-10-03, Don. Object: turn31 as merged into main (commit 69c091c,
built on Don30 bbd8dbb). The builder's workspace was never modified;
refusal probes ran on a tar copy outside the tree. Original record:
the Don node's own notes; this file is the repository copy in the
neutral register.

## Verdict: GO

The material FAIL F1 is accepted as sealed. The two-clock mechanism is
not adopted, by the builder's own frozen threshold. The handoff to Sol
is accurate and complete.

## Independently executed checks

1. **Gate recount from RESULT.quantities, not from stored flags.**
   F1 FAIL: partial.flat3.tail mean 0.7426422582886829 < 1 (wins 6/8,
   recounted from per_world); partial.balanced.tail 9.693541851633853
   (wins 5/8). F2 PASS: retention minimum 0.9616350829646932 >= .95,
   recombined_positive 8, pooled positive at all three declared points.
   F3 PASS: permuted_excess negative in all five regimes (sign is
   permuted minus factored, verify.py:321). F4 split writer/reader is
   the protocol's own construction: verify.py:393 requires
   independent_reader_pending=True with F4=False in RESULT; the reader
   assigns F4=True itself.
2. **Seam window.** tail = gain − horizons[8192]
   (turns/turn30/verify.py:22,164), so F1 measures [8192,16384) —
   the turn30 D1 lesson is observed.
3. **FREEZE 22/22** recomputed by hand; the pinned copy of
   turns/turn30/verify.py is bit-identical to the canonical one
   (3d345a3ac2a44a47ae039aeb4a2a280b5c5650a93345f6a6947b5cb06a276734).
4. **case_router.c read line by line.** cr_observe (lines 238-241)
   matches PROTOCOL:37-38 with both updates on pre-observation state;
   NEW/equal passthrough at 222-223 and 359-360; shared admission from
   the pooled shadow at 32 bits effective next byte (355-375); the
   −1/16-bit bounds enforced at run time (179-180). The claim that
   flat3/binary/loader are copied unchanged from turn30/router3.c is
   machine-confirmed: 15/15 r3_ functions identical, same constants.
5. **Pristine reader rerun** to a fresh --output: rc=0, 3,932,160
   forecasts, 2,688 counters, max_error 1.2590817277668975e-11; the
   fresh receipt is byte-identical to the sealed VERIFY.json (cmp;
   sha256 73fffe23b9f0efb84e968921b6ece637c46e478f5db80894fc99ab01573c12fc).
6. **Three refusal probes on the tar copy.** (a) One altered price in
   a trace: rc=1, refusal by name at the manifest layer. (b) The
   manifest entry then updated to match the altered file: rc=1, the
   reader still refuses from its own recount, naming the exact
   coordinate (recombined.turn31.tsv.gz:59/factored/candidate,
   error 1.00e-05). (c) One altered life gain in RESULT.json: rc=1,
   RESULT/288/recombined/arms/factored/gain, error 1.00e-05. All
   three layers answer an invalid value with a named refusal.
7. **REPORT and handoff cross-checked against RESULT**: 0.742642/6-8,
   9.693542/5-8, 98.4194/99.1242/96.1635, the per-world list at
   REPORT:112, −50.303221/−54.875236 against pooled,
   401.106560/85.780624/−15.108303/−4.519628 for flat3−balanced,
   −0.179531/−15.287834 switched tails, and the world292 extremes
   t1456 +3.2713813 / t1518 −2.5508759 — every figure agrees.
8. **The incoming AUDIT30 of Don's turn30** is confirmed: its rerun
   receipt and pin counts match this hand's own reading. Its note on
   the turn30 REPORT.md:47 heading (a claim stronger than the text
   beneath it) is correct and is accepted by the turn30 author; the
   history is not rewritten, the correction lives in the audit records.

## Non-material notes

- The probe layering (manifest first, recount second) gives the right
  depth: an invalid artifact is refused even when its manifest entry
  agrees with it.
- DIAGNOSIS is descriptive only, with no alternate predictors — within
  protocol.
- The next question handed to Sol (start exactly from the pooled
  forecast, let case distinctions enter only as recipient evidence
  earns them) preserves the pooled incumbent and is well-posed.
