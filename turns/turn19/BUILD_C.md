# Turn19 C build: witness with a commitment ceiling

2026-09-21. Protocol read before implementation. Parent checkout is
`29c384e0a02ab65c2cb111ddbd3065e1f71a514b`, branch
`astra/turn19-commitment-ceiling`. No fresh world, previous-world replay,
commit or publication was performed by this implementation hand.

## Change

`authority.h`, `authority.c`, and `replay.c` derive from frozen turn18.
The existing four modes retain their arithmetic and quote chronology.
New `AR_CEILING` shares witness's old-evidence hazard choice and its
`matched >= 1` clock update. After its ordinary hazard update, it sets
`odds=min(odds,5)`. The observation that first admits a source is cold and
initializes the following quote with odds=evidence=0.

The final `int *clipped` argument of `ar_observe` reports whether this
state's **own** ordinary hazard update exceeded 5. It never compares with
witness's different odds. The flag is measurement output, not persistent
prediction state. Errors leave state and output arguments unchanged.

`ar_quote` remains a separate pure function. Inactive and equal-price
quotes return cold exactly. Replay finishes all five quotes before any
observation. It asserts identical witness/ceiling evidence and hazard choice,
shared first admission, and ceiling prequote/postupdate odds <=5. Its ceiling
drawdown limit is `log2(33)`, alongside the inherited limits for other modes.

Persistent state remains **32 bytes per mode**, **160 bytes for five modes**
of one candidate. The source archive acquires no bytes. Replay statistics,
clip counts and temporary before-state copies are separate measurement data.

## Interface

Input to `authority_replay` is unchanged, without a header:

```text
t cold candidate matched
```

`t` starts at zero and is consecutive. Output preserves the first 27 turn18
columns and appends seven ceiling fields, for **34 columns** total:

```text
t cold candidate matched shadow_before active_before admitted_after
slow_live slow_odds_before slow_slow_used slow_odds_after
fast_live fast_odds_before fast_slow_used fast_odds_after
adaptive_live adaptive_odds_before adaptive_slow_used adaptive_odds_after
witness_live witness_odds_before witness_slow_used witness_odds_after
adaptive_e_before adaptive_e_after witness_w_before witness_w_after
ceiling_live ceiling_odds_before ceiling_slow_used ceiling_odds_after
ceiling_w_before ceiling_w_after ceiling_clipped
```

The actual header and records occupy one TAB-separated line each. Floating
fields use 17 significant digits. `*_slow_used` is -1 for inactive
observations, otherwise 1 for slow hazard and 0 for fast. Stderr reports
state size and per-mode gain, extrema, admission, clock counts and clip count.

## Byte-preserving build after repository relocation

Frozen `turns/turn13/episode.c` still includes `../byte_recurrence/...` and
`../portable_recurrence/...` as in its original layout. Make creates a private
layout under **this turn's** `.build/`:

```text
.build/turn13/episode.c -> ../../../turn13/episode.c
.build/byte_recurrence -> ../../../byte_recurrence
.build/portable_recurrence -> ../../../portable_recurrence
.build/court4 -> ../../../court4
```

These relative symlinks all resolve inside this checkout. No frozen file or
other directory is edited. Compilation uses the staged paths, retaining the
original include relationships and exact source bytes. The resulting
`episode` hash is **identical to the frozen inherited binary**.

## Build and one fixture

Commands run in `turns/turn19`:

```sh
make all authority_fixture
./authority_fixture > FIXTURE.json
```

Both exited **0**. Compiler: Apple clang 21.0.0
(`clang-2100.0.123.102`), target `arm64-apple-darwin25.4.0`.
Flags: `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`; link `-lm`.

The single deterministic fixture contains **33 synthetic price events**.
It covers cold admission crossing, active positive evidence, cap hits,
zero-delta unmatched silence under both hazards, wrong-candidate evidence,
and subsequent recovery. Every event quotes both outcomes of a binary
distribution before observation. It checks pure quoting, the before-state
mixture in long-double mass space, exact equal-price quotes, shared clocks,
excluded admission evidence, and the cap against that mode's own mass-space
posterior. Bounds are measured from the charged prices.

Result: **PASS**, **5 clips**, maximum binary normalization error
`1.6653345369377348e-15`; state sizes **32 / 160 bytes**. `FIXTURE.json`
retains all inputs and all five before/after states and prices for a separate
reader. This is an implementation check; material evidence is reserved for
the preregistered fresh batch.

## Build hashes

| File | SHA-256 |
| --- | --- |
| authority.h | `1230beb36a3b05168b0b83627a160ae967318a7283d60b5482c567ef2e6d24bc` |
| authority.c | `3642f74713559f2d59fc70511b397b3fd218cdd8fcb04dc47d3146dd1536f84e` |
| replay.c | `a6a5e1164bed89c71fa920c913a37892f47b56717ff1613a06d1ac69938d9372` |
| Makefile | `9a80938f2220a123f0fd32bd3b045ef9140578e8d282a5d226844ab7a6e58b51` |
| authority_fixture.c | `a5dec5f9f6450cc09a57f382c4d9f1a559803c1b31b74759cb5c2e0cc0b99b8e` |
| FIXTURE.json | `c8292b6fe8d13aa4dfe0b7cecb41e2b877b4e872d6b5b37b026e0bd276115529` |
| authority_replay | `19d5ff04587428001a21beb52b224849c0b098fe5f835c4b7e37281b4a430794` |
| episode | `784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10` |
