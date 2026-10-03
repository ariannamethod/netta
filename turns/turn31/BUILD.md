# Turn31 C implementation receipt

2026-09-26. Implementation lane wrote `case_router.c`, `Makefile`, and this
receipt. All inherited sources remain unchanged. No protocol worlds or source
data were generated or replayed by this lane.

## Build and inheritance

`make all` completed with rc=0 using:

```
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  case_router.c .build/byte_recurrence/frontend.c \
  .build/portable_recurrence/recurrence.c -lm -o case_router
```

The private `.build` symlinks preserve turn13's include layout. Its episode
source is included with a renamed main; emit and trace delegate to that main.
`episode` is a symlink to `case_router`. No build writes into an older turn.

Fifteen turn30 helper function texts were compared and are byte-identical:
`r3_init3`, `r3_check3`, `r3_init2`, `r3_check2`, `r3_quote3`, `r3_observe3`,
`r3_quote2`, `r3_observe2`, `r3_load_bank`, `r3_record_match`, `r3_same_rules`,
`r3_observe_outer`, `r3_print3`, `r3_free_bank`, `r3_free_grammar`.
This includes Don30's flat posterior normalization and shared-admission outer
law. The address validator now checks the specified two-book permuted archive,
including each book's independent count rotation and the exact A+B pooled sum.

## New state and chronology

`CRFactored` stores two doubles, u and v. Its two posterior updates are computed
into a new struct using old u/v and the charged pretruth Q/S. Conditional v
uses S even when u is near its floor. Both clocks run on every matched visit,
including NEW/equal prices and visits before outer admission. Unmatched visits
do not change a record. Balanced has static case mixture 0.5A+0.5B and the
unchanged binary permission update.

Five source, three conditional, six candidate and six live vectors are complete
and validated before `fgetc`. Equal source forecasts and protected NEW copy P0
exactly. One pooled-candidate shadow supplies prospective admission to all six
arms; each arm's own shadow, odds and gain are retained and checked. The cold
arm shares admission but continues to quote P0 exactly.

CLI and the **75-column TSV** follow `INTERFACE.md`:

```
case_router predict BANK POOLED_SMALL PERMUTED_BANK < RAW
case_router emit ALPHABET NONZERO_SEED < COMMANDS
case_router trace [compact] < RAW
case_router --fixture
```

## Allocated costs

Measured struct sizes are factored **16**, flat **24**, binary **8** bytes.
For twelve records:

| Arm | Portable bytes at R32/J12 | Recipient router bytes |
|---|---:|---:|
| factored | 528 | 192 |
| flat3 | 528 | 288 |
| balanced | 528 | 96 |
| pooled | 336 | 96 |
| permuted | 528 | 192 |
| cold | 0 | 0 |

The single comparison process allocates 864 persistent router bytes at J12.
Common frontend, source caches, outer states, history and forecast scratch are
additional. The twenty named forecast vectors occupy 40960 bytes of stack
scratch. Production stderr prints actual archive sizes, per-arm router sizes,
total routers, outer/history sizes, and vector scratch for the loaded record
count; nominal budget figures do not replace those receipts.

## One predata fixture

`--fixture` executed successfully and emits twenty JSON lines matching the
declared scalar interface, with truth/rank and normalization diagnostics added.
Sources are normalized handcrafted full-byte forecasts. There are two record
identities, a matched NEW event, a matched all-equal event, one unmatched visit,
eight observations where both source histories are worse than P0, and a later
change of the favored case. There is no world/archive/frontend access.

Observed minimum factored u: `0.0009242327874014553`; maximum full-vector
normalization error: 0. NEW/equal quote identities and unmatched state retention
held. This implementation receipt does not replace the separate reader's
arithmetic comparison, which is owned by the root/measurement lanes.

## Handoff SHA256

| File | SHA256 |
|---|---|
| case_router.c | e94d42455f653e46b6ef206fcc6dea22b4df66fc27a63d9db61cdce6450b354e |
| Makefile | bfafb145919adcdd278fc2a558abfb42190ab830b7377fdd29411438cfe217c5 |
| case_router executable | ab7b48a1f28dc283a1e947ef6ba7964713f87957db9cf89505ed8bde5aabb054 |
| inherited turn30/router3.c | ba1d181f8ee9b2a68b9efd66f33690072f0df9cc22fea47e7c251df54be40020 |
| inherited turn13/episode.c | 8509ce692c394eaeea55bc1ce99596e246449a519f32bf0576de4e74dd6d5ad9 |
