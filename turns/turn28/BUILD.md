# Turn28 C build

2026-09-25. New files: `bank.c`, `Makefile`, this receipt. Existing prediction
sources are included or linked without edits. No protocol worlds were run by
this implementation lane.

## Build and inherited functions

```
make all
```

Completed with rc=0 using Apple clang 21.0.0, target
`arm64-apple-darwin25.4.0`, and:

```
-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -lm
```

The Makefile stages symlinks under this turn's `.build` only. `bank.c` includes
`.build/turn13/episode.c`, renaming its main to `inherited_episode_main`.
Consequently `quote_cold`, `observe_cold`, `branch_match`, `candidate`,
`validate`, `observe_outer`, emit and trace are the exact inherited functions.
The old source's relative include layout is preserved. The `episode` symlink
points to `bank`, so the existing generator and extractor can call their usual
executable name; emit and trace delegate to the inherited main.

Prediction builds four source vectors, five router/pooled vectors and five
live vectors before `fgetc`. All are validated over 256 bytes. Exact NEW and
equal-source forecasts explicitly copy P0. Router updates occur on every
matched visit, after pricing, including NEW and visits before outer admission.
No-match visits leave all router state unchanged.

The bank loader checks topology, addressing, duplicate prefix content, byte
budget and reserved zeros. It checks the initial selected-list projection,
shared grammar, A+B counts in both pooled controls, and the declared rotation
of both permuted count vectors. Recipient counts never modify archives.

## Interface and storage

```
bank predict BANK POOLED_FULL POOLED_SMALL PERMUTED < RAW
bank emit ALPHABET NONZERO_SEED < COMMANDS
bank trace [compact] < RAW
bank --fixture
```

Prediction prints the 59 columns from `INTERFACE.md`: 18 common fields,
six before/after weight triples, and five arms with seven outer fields each.
Arms are local, global, pooled_full, pooled_small, permuted.

`sizeof(EBRouter)` is **24 bytes**, three stored probabilities. For J=12,
local and permuted each allocate **288 bytes**; global is **24 bytes**.
These routers together occupy 600 bytes of persistent adaptation state.
The earlier audit's two-log-ratio proposal had a different representation;
it is not the implementation's memory figure. The five inherited outer
states, history, source caches and forecast vectors are additional runtime
storage. Prediction stderr reports actual router/outer/history/vector sizes
and each serialized archive size. Serialized bank budget remains <=528 bytes.

## One predata fixture

`bank --fixture` prints eight JSON lines for one handcrafted sequence and two
local record states. It uses fixed, normalized full-byte distributions and
accesses no world data. Observations include conflicting A/B likelihoods,
matched NEW after non-prior local evidence, matched equal-source forecasts,
and a no-match visit after the global weights changed.

Executed successfully: eight observations; maximum full-vector normalization
error 0 on this fixture; exact equality holds; matched NEW performs fixed
share; unmatched global weights remain unchanged. Each line exposes component
prices, record, match flag, truth/rank, local/global mixtures and before/after
weight arrays for the independent reader's arithmetic check. That independent
check is owned by the root/measurement lanes, not self-certified here.

## SHA256 at implementation handoff

| File | SHA256 |
|---|---|
| bank.c | 82fe7a161b885f7ca0de12b96b58bec51e5949b8069aa90c8e23b55394871e8f |
| Makefile | de709fbde5c53de0d833fc6194c7fc1881d2f47cced79ef0f707ff783bd175ff |
| bank executable | 21b0dc1157338a565426ab3ec9948b6f2bb9c7d66323662849a550ba8dc07ed8 |
| inherited turn13/episode.c | 8509ce692c394eaeea55bc1ce99596e246449a519f32bf0576de4e74dd6d5ad9 |
| inherited frontend.c | 598b5aa2301e48029b7d5c2ca89c9d67372b33df9497d8b14d0b0de646ee3d08 |
| inherited recurrence.c | 1c14a567b30d376d515c22f6280872e98811956a691958057cfc7ef4b48739d3 |
