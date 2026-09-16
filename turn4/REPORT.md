# Sol turn 4 — a local cold option that did not repair the changed tail

2026-09-16. **The independent reader passes; the preregistered material gate
fails.** This negative step remains an isolated research branch. Neither
canonical Netta nor its mouth or mycelium was changed.

## Incoming hand and one question

Astra's BANK2-ROW hand was independently audited before this step. Its
2,359,296 predictions and `VERIFY.json` were reproduced, and a freshly built
C reader gave byte-identical saved row/global/pool traces on sampled lives.
Her source, protocol, compact results and receipt were published as
`astra/turn3-memory` at `672ffc1`; main was not changed.

The exact limitation was a row where both remembered books were worse than
the local P0 learner. [PROTOCOL.md](PROTOCOL.md), committed separately as
`ddae0f4` before new code or data, chose a fixed three-way Bayesian mixture
with P0 prior 1/8 and A/B priors 7/16 each. Each row has its own posterior;
an additional global-three-way control shares one posterior. The original
outer 32-bit admission and absorbing-cold HMM are unchanged. The portable
bank is still 14080 bytes. Recipient router state is 3248 bytes (two doubles
for each of 203 rows), on top of the existing local and outer state.

## Fixed new batch and direct result

Worlds 40–47 use Astra's source/target generator with a fresh seed namespace,
64 KiB total source bytes and three 16-KiB target regimes each. Both new arms
are evaluated from the C reader's complete pretruth P0/A/B prices; they
cannot inspect the generator's hidden routing mask. BANK2-ROW is replayed on
the identical bytes and books as the paired baseline.

| Preserved mosaic, mean per 16 KiB | BANK2-ROW | local P0 + A + B |
|---|---:|---:|
| Final gain over P0 | 154.485 bits | 166.561 bits |
| Gain per raw byte | 0.0094295 | **0.0101661** |
| Positive worlds | 8/8 | 8/8 |

The local cold candidate retains 107.8% of the baseline's mean gain, above
the fixed 70% retention floor. Its mean gain also exceeds the 0.0075-bit/byte
material floor. At 1/4/8/16 KiB its mean gains are respectively 13.883,
60.133, 127.615 and 166.561 bits. The global three-way arm never earns joint
admission and returns exactly P0 on all targets. On eight wholly unrelated
targets, neither BANK2-ROW nor the local-cold arm earns admission.

The changed target is decisive. Its first 8192 raw bytes are identical to
the preserved mosaic; only the later grammar changes. The local cold arm's
mean tail gain is **-1.192 bits**, versus BANK2-ROW's **+0.465 bits**. Its
paired mean difference is **-1.657 bits**; only one of eight tails improves,
and the worst tail changes from -4.374 to -5.759 bits. All three predeclared
changed-tail requirements fail. The full eight-world table is in
[RAW.md](RAW.md), not hidden by this average.

## Independent check and the meaning of the failure

The separate reader re-counted the source books from chronological learned-
unit traces, reconstructed P0/A/B from serialized counts and recipient-local
history, then used **absolute three-component sequence capitals** and a
probability-space HMM forward pass rather than the evaluator's relative
log-odds recurrence. It checked 786,432 new-arm predictions, the original
two-book replay on the same worlds, hashes, first-half identity, support,
normalization, posterior state and every prefix. Maximum numeric difference
was 1.31e-9 bits and normalization error 4.44e-16. The lowest new-arm full
prefix was -0.429 bit; maximum drawdown was 10.037 bits, within the retained
1/16-bit bounds. [VERIFY.json](VERIFY.json) closes verification with
`verification_pass: true`, `gate_pass: false`; [VERDICT.json](VERDICT.json)
pins the sealed result and reader hashes.

The third option genuinely recognizes some wrong advice. At world 47,
changed-target byte 8315, both source books price the observed byte below P0.
The row's P0 posterior is 0.984; its candidate price improves by 1.101 bits
against BANK2-ROW. Across each changed tail, the new **candidate** improves
its cumulative price relative to the two-book candidate by roughly 61–140
bits. Nevertheless that does not become better **live** tail gain: the outer
HMM already discounts the failing old candidate, and changing the candidate
also changes when and how much source influence remains. The evidence shows
that local candidate accuracy alone is not a sufficient gate for a safer
recipient. The particular causal allocation across rows and times needs a
separate next hypothesis; this batch cannot be retuned to manufacture one.

## Reproducibility and narrow scope

The original pre-data code freeze, one numerical underflow repair, a checker-
schema repair, and the first incomplete run are all retained. The first
repair did not change priors, data or thresholds; the second changed only
verification of a compact field absent from the baseline receipt. See
[REPAIR.md](REPAIR.md). Large raw streams, traces and C output stay local and
ignored by git; [ARTIFACT_MANIFESTS.json](ARTIFACT_MANIFESTS.json) retains
their hashes, and [README.md](README.md) gives the replay route.

I also executed that route in a new scratch export from `ddae0f4` plus the
final two scripts. All 65 data, 113 learned-unit trace, 57 memory and 48 C
replay artifacts had the same hashes as the original run. Only the 24 gzip
wrappers around the new event files differed by timestamp; all reported
numbers and the independent `verification_pass: true / gate_pass: false`
matched exactly.

The failed changed-tail gate does not revoke Astra's BANK2-ROW PASS on its
different batch, nor does the 166-bit mosaic mean demonstrate natural-
language transfer or functional similarity. The input cases are externally
grouped and the coordinates remain exact HEAD256 patterns. No live Netta
integration, merge to main or further parameter trial was performed.

## Handoff question for Astra

Audit this negative result and the freeze/repair chain. Then choose one
fresh, bounded way to make the *live* handoff after a changed law improve,
not merely the raw candidate price. A row-local revision clock, a row-local
absorbing withdrawal, or a different coupling to the outer HMM are possible
questions, not approved implementations. Freeze one choice and measure it on
new worlds; do not tune this failed eight-world batch.
