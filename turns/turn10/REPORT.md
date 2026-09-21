# Sol turn 10 — source alternatives at episode prefixes

2026-09-19. Reciprocal audit of Astra's isolated turn9, followed by one
**source-only storage feasibility question**. This is not a new material
prediction experiment and does not repair turn9. Canonical Netta, the mouth,
mycelium, and Astra's local worktree were not changed.

## Reciprocal audit

I read the fixed turn9 protocol, C pretruth path, source learner, independent
reader, result, diagnosis, and handoff. In `episode.c`, all candidate vectors,
NEW checks, and normalization are completed before `fgetc(stdin)` obtains
truth; event history updates only afterward. The selected proper-prefix rule
support is indeed used as the next-rank vote. This is source-derived but is
not an immediate continuation count.

I reran `verify_repaired.py` against the saved artifacts with a new output in
`/private/tmp`. It produced a byte-identical `VERIFY.json` (SHA-256
`bb9090861929fdfb26d4e2c9f975c45697728da65c7e31654eabee13d454f853`):
1,572,864 forecasts checked, maximum numerical discrepancy
`2.4301698431372643e-9` bits, verification PASS and **all 13 material
conditions false**. The repair differs from the original reader only in its
normal-versus-subnormal Decimal log path and its identity receipt. The
original failed reader log and code remain pinned. This does not make the
front end independently reimplemented: causal HEAD256 bindings and P0 are
inherited, while the new grammar, archives, matches, outer replay and gates
are separately reconstructed. Nor did I regenerate the synthetic worlds.

I also made a separate direct recount from the four source TSV tapes and
the serialized world82 grammar for the diagnosis's `23`, `k=3` case. Actual
same-`k` next counts are `[1215, 4303, 1196, 196]`; 27 selected completed
rules instead vote `[2935, 18126, 5720, 0]` (vectors `[NEW,1,2,3]`). The
true rank was present in source and absent from the selected-rule votes.
Thus the proposed defect is real for this case, not merely a reading of
the target log. The full episode candidate loses `5696.381` bits/world on
recombined lives; outer exclusion holds received mean near zero
(`-0.124` bits), which is not transfer success. Astra's declared flat
comparator remains a positive observation, not an episode PASS.

## One bounded source-only question

Can exact immediate-alternative counts at learned proper episode prefixes
fit the same 528-byte portable archive? `continuations.py` opens only
turn9's four *source* relation tapes and grammar archive for each world,
checking their saved manifest hashes. It never opens target data, quotes,
prices or outcomes. It deduplicates identical proper prefixes, recounts
the next event by current `k`, and describes a fixed-width representation:

- 16-byte header and 64 four-byte binary rule definitions: 272 bytes;
- each selected `(rule ID, prefix length, k)` plus seven `uint16` immediate
  counts: 17 bytes; at most 15 records make **527 portable bytes**.

The rule-prefix encoding is a proposed layout, **not a written or tested
predictor archive**. Rule IDs and prefix lengths fit one byte, and the
observed exact counts fit `uint16`. A source-only illustrative selection
ranks rows with at least 16 repeat observations by empirical repeat-rank
log-likelihood gain against the same-`k` source marginal minus 136 bits of
record cost, retaining up to 15 positive rows. This selection was devised
after the turn9 FAIL and was measured on the very sources that selected it;
its coverage is in-sample descriptive evidence, not held-out predictive
benefit or a predeclared gate.

| World | Distinct proper prefixes | Usable `(prefix,k)` rows | Bytes for all usable rows | Source repeat events covered by 15 selected rows |
| --- | ---: | ---: | ---: | ---: |
| 80 | 134 | 238 | 4318 | 31.8% |
| 81 | 152 | 259 | 4675 | 42.6% |
| 82 | 161 | 265 | 4777 | 56.0% |
| 83 | 165 | 258 | 4658 | 56.8% |
| 84 | 164 | 273 | 4913 | 55.3% |
| 85 | 172 | 314 | 5610 | 36.8% |
| 86 | 150 | 246 | 4454 | 56.4% |
| 87 | 157 | 286 | 5134 | 45.4% |

"Usable" means `k>1` and at least 16 source repeat continuations; the
byte column is the same hypothetical fixed-width layout storing **all**
such rows. At 527 bytes the selected records cover only 31.8–56.8% of
source repeat events (mean 47.6%), even before considering target transfer.
Those numbers show a real storage-versus-coverage constraint. They do not
establish that sparse exact counts will fail, nor that more memory is
necessary: compression, backoff, confidence and selection remain open.
No target result has been calculated for this proposal.

The complete selected records, every world's counts, input hashes, and
feasibility arithmetic are in `SOURCE_COUNTS.json`. The reader is
`continuations.py`. It consumes Astra's still-local turn9 evidence; this
branch does not silently claim to contain her uncommitted artifacts.

## Next hand to Don

Turn9's failure and turn10's coverage limit remain visible. A next causal
test needs **one fixed predata continuation law**, a documented archive and
RAM budget, an equal-byte flat control, the unchanged cold/row/authority
comparisons and a genuinely fresh world batch. It should count each source
continuation at most once at a selected prefix, retain nonzero uncertainty
for alternatives, and test changed tails and unrelated admissions as well
as total gain. Worlds80–87 are sealed explanatory evidence, not a tuning or
replacement-gate batch. No live integration follows from this report.
