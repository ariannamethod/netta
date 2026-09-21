# Turn13 C build and bounded wiring check

2026-09-19. Derived from sealed turn12 C without changing turn12 or dependencies.
Only arm count/wiring, CLI, TSV blocks and associated cache diagnostics change.
`isolated` is inserted immediately after `episode`.

## Build

```sh
make -C turn13
```

PASS with `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror` and `-lm`.
Makefile is byte-identical to turn12. No protocol world was generated or run.

## Interface

```text
episode predict EPISODE.bin ISOLATED.bin FREQUENCY.bin REVERSE.bin PERMUTED.bin FLAT.bin ROW.bin < RAW
```

TSV keeps the turn12 common fields. Match blocks are episode, isolated,
frequency, reverse, permuted, flat (`matchedL`, `matches`, `votes`). Forecast
blocks have that order followed by row (`candidate`, `live`, `shadow_before`,
`odds_before`, `active_before`, `activated_after`, `gain_after`).
NETEI001, NETFI001, HEAD256 formats and independent528-byte small-arm caps
are unchanged. Five grammar slots, seven arms; all16 vectors (base, cold,
seven candidates and seven live laws) exist and pass checks before truth.
Candidate/live vector payload is14*256*sizeof(double)=28672 bytes; all
other actual cache sizes continue to be reported on stderr.

## Preserved function bodies

All 24 inherited static functions other than `predict` are byte-identical:

`fail`, `la`, `read_bytes`, `le16`, `le32`, `le64`, `append_child`, `load_grammar`, `load_flat`, `load_row`, `validate`, `rank_heads`, `quote_cold`, `observe_cold`, `branch_match`, `flat_match`, `candidate`, `observe_outer`, `print_heads`, `print_match`, `stream_end`, `random64`, `emit`, `trace`.

Thus the new slot has the same match, masking, quote, normalization,
NEW-preservation and HMM implementation; source allocation belongs to the
new learner, not the C quote law.

## One disjoint wiring sanity check

Reused the existing manually constructed128-byte fixture at
`/private/tmp/netta-turn12-finite-i0serbto`, read only. It contains no protocol
world. Supplied its same one-record book to episode/frequency and its same
compatible two-context books to isolated/flat. One new C predict execution
confirmed their corresponding forecasts and authority fields match exactly,
while isolated differs from episode on 30 bytes. This checks
argument/arm wiring; it is not a material experiment. All128 NEW checks pass;
maximum full-vector normalization error is 1.1990408665951691e-14.
No arm admitted on this tiny fixture, so this check does not claim a new
active-HMM replay. The HMM function is inherited byte-for-byte.

## SHA256 identities

- `turn12/episode.c`: `a88f50e38692ab85bd936583c96375cf70f50fd076ab2d384ce57f2ebdc99e51`
- `turn13/PROTOCOL.md`: `ec37980efb94865e6699cb8abe954fddf779924fc3fa5c2fa5f15d659a9594d4`
- `turn13/episode.c`: `8509ce692c394eaeea55bc1ce99596e246449a519f32bf0576de4e74dd6d5ad9`
- `turn13/Makefile`: `9c5bea9aba5637d448c7c0ce3b0c48720886f23507eafaceb32047ea77a25b68`
- `turn13/episode`: `784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10`
