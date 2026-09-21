# Turn22 C implementation receipt

2026-09-21. Implementation began after the incoming Don21 reader completed
and root gave GO. `PROTOCOL.md` and `INTERFACE.md` were read before code.
Only turn22-owned files and its private ignored `.build/` outputs changed.
No old/new-world experiment, parameter replay, freeze or commit was run by
this implementation hand.

## Mechanism and API

Modes are `slow fast witness h8l4 selfnorm`. `ar_init`, pure `ar_quote`, and
`ar_observe` remain separate. `ar_cap` exposes the currently permitted cap
for h8l4/selfnorm; uncapped modes return NAN from that diagnostic helper.

The added `absolute` double is updated only for active matched selfnorm
observations. Signed witness evidence is unchanged and identical across
witness/h8l4/selfnorm. Old signed evidence selects hazard; both statistics
then update; `16*max(0,w)/(1+a)` caps the following selfnorm odds. Raising
the permitted cap does not raise surviving odds. The first crossing stays
cold and initializes odds, signed evidence and absolute evidence to zero.

The quote keeps inactive/equal-price outputs bitwise cold. `clipped` is
measurement output from that state's own ordinary odds exceeding its new
limit. It is not stored in prediction state. Replay completes all five
quotes before any state observes and checks shared admission, clock
identity, cap bounds, and the one-bit/16-bit loss limits (fast10-bit).

`ARState` contains four doubles plus mode/active: **40 bytes** on this build.
Five replay modes use **200 bytes**. Only selfnorm needs the added statistic;
the other four modes leave it zero. Measurement statistics remain outside
prediction state. No portable archive byte was added.

## Replay serialization

Input has no header: `t cold candidate matched`, chronological from zero.
Output is the exact **41-column** TSV specified in INTERFACE.md:

```text
t cold candidate matched shadow_before active_before admitted_after
slow_live slow_odds_before slow_slow_used slow_odds_after
fast_live fast_odds_before fast_slow_used fast_odds_after
witness_live witness_odds_before witness_slow_used witness_odds_after
h8l4_live h8l4_odds_before h8l4_slow_used h8l4_odds_after
selfnorm_live selfnorm_odds_before selfnorm_slow_used selfnorm_odds_after
witness_w_before witness_w_after h8l4_w_before h8l4_w_after selfnorm_w_before selfnorm_w_after
selfnorm_a_before selfnorm_a_after
h8l4_cap_before h8l4_cap_after h8l4_clipped
selfnorm_cap_before selfnorm_cap_after selfnorm_clipped
```

The actual header/record is one TAB-separated line. Floating fields have17
significant digits. Slow-used is−1 while inactive,0 for fast hazard,1 for
slow. Before/after cap diagnostics are emitted even while inactive. Stderr
contains event/state sizes and each mode's accumulated statistics.

## Preserved inherited builds

The frozen turn13 byte predictor is compiled through the same private
`.build/turn13/episode.c` and root-core symlink layout used in turn21.
The links resolve inside this checkout. Source bytes are unchanged; the
resulting `episode` hash remains **784d549d…**, identical to the frozen binary.

For the single fixture, Make compiles the **unchanged**
`../turn21/authority.c` into `.build/ref_authority.o`, renaming only exported
symbols through `-D` compiler options. The fixture imports the corresponding
old header under distinct type/enum names. It maps new h8l4 to the old
grid's verified index10 and compares old/new controls on identical prices.
This does not create a new policy or evaluate a previous target life.

## Strict build and one short fixture

Commands in `turns/turn22`, both exit code0:

```sh
make all authority_fixture
./authority_fixture > FIXTURE.json
```

Flags: `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`, link `-lm`.
Apple clang21.0.0 (`clang-2100.0.123.102`), arm64 macOS.

One deterministic **40-event** price journey covers admission, supporting
evidence, contradictions, matched zero evidence, unmatched silence under
both hazards, and renewed supporting observations. Before each observation,
both outcomes of a binary distribution are quoted. Checks cover pure and
prospective pricing, normalization, cap application, the absolute statistic,
domain and loss bounds. Mass-space quote/cap checks use separate arithmetic
from the log-odds update. The fixture retains all inputs and all mode prices,
before/after states, cap values and clip flags in `FIXTURE.json` for an
independent reader.

Result: **PASS**; **13 selfnorm clips**; maximum binary normalization error
**8.881784197001252e-16**. All **160 control-events** (40×4) match turn21
exactly for prices, odds, signed clock, shadow, admission, hazard and clip.
The fixture establishes implementation continuity and order, with material
utility reserved for the preregistered batch.

## SHA-256 after build

| File | SHA-256 |
| --- | --- |
| authority.h | `8f8c0d645cce26a82c3fe54a139709313d0a94ad8b776ad52740684905a3fd9f` |
| authority.c | `dbcfbf4a6f54884dd6b1e20192c00dca39b9699e82fcdd7b070a3bd56620bd93` |
| replay.c | `a637aef6e80e8b0998c2285b03c93b94d8bfe2beab75e4b2564cb39f9cc4c0f4` |
| Makefile | `99b05253daaac5809b30cde634cbdc785a10489a4e47c84403dcfe83a39d7b4c` |
| authority_fixture.c | `52c7d5c0bde659252fb5295a2be9d1b57dcd67d1de082710db15afe9ccdf4dd3` |
| FIXTURE.json | `cb97bbbbbe2801dc943b9d01ee2cd403f08872701e7506b2018ae67a7ef6d138` |
| authority_replay | `b539bec2c1592f21f2b46d8b5b977ecafc6482d7546d12593a19cceaa4eaf6b2` |
| episode | `784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10` |
