# Turn16 C authority build

2026-09-21. Implementation follows the already written `PROTOCOL.md`.
No protocol world, target prediction or historical model replay was run by
this implementation hand. The only executed fixture uses a short synthetic
sequence of log-price differences to make the update order observable.

## API and state

`authority.h` exposes `ar_init`, pure `ar_quote`, and `ar_observe` for the
four modes in order `slow`, `fast`, `budget`, `return`. A quote may be called
for every outcome before observation. Observation updates only future
authority. Mode priors, hazards, shadow32, arm threshold -32, support32,
natural-recovery precedence, and one-use return follow the protocol.

`ar_quote` returns `cold` directly when inactive or `candidate == cold`.
This corrects the demonstrated inherited fast-NEW rounding exception in the
new implementation without editing turn13, turn14, turn15 or saved evidence.
For budget/return, `c=exp2(-0.5)` and `z0=log2((1-c)/c)`.

Persistent `ARState` has three doubles (shadow, odds, support), mode, and
three unsigned flags (active, armed, used): **40 bytes** on this build.
Seven arms times four modes occupy **1120 bytes**. Gains, extrema, counts,
temporary quotes, and trace fields are outside this prediction state.
The portable source archives acquire no bytes.

## Replay interface

Run `turn16/authority_replay < prices.tsv > authority.tsv`.
Input has no header: `t cold episode isolated frequency reverse permuted flat row`.
Time begins at zero and must be consecutive. No world or regime is accepted.

Output has one row per time and arm, in the input arm order, with this header:

```text
t arm cold candidate shadow_before active_before admitted_after slow_live slow_odds_before fast_live fast_odds_before budget_live budget_odds_before return_live return_odds_before return_armed_before return_support_before return_used_before return_event return_odds_after return_support_after return_armed_after return_used_after
```

The actual separator is TAB. All floating fields use 17 significant digits.
`return_event` is one only when the artificial return is spent. Admission
marks the crossing observation; its price is still cold and the following
observation receives initial authority. Before-state fields are captured
before all observations; all 7×4 quotes finish before any state observes.
The replay checks shared lifetime shadow and first admission, the four loss
ceilings, and the exact equal-price identity. It prints state size and final
measurement summaries to stderr.

`ar_observe` events are `NONE`, `ADMITTED`, `ARMED`, `NATURAL_RECOVERY`,
`RETURNED`; the public TSV exports only artificial return as its return flag.
Arming, natural recovery, and clearing are visible in before/after state.

## Strict build and the single fixture

Executed successfully, exit code 0:

```sh
make -C /Users/ataeff/arianna-codex/repos/netta-astra-turn16-20260921/turn16 all fixture
```

Compiler flags: `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`; link `-lm`.
Compiler: Apple clang 21.0.0 (`clang-2100.0.123.102`),
target `arm64-apple-darwin25.4.0`.

Fixture output:

```text
PASS: exact equality, cold crossing, natural priority, excluded arming event, neutral hazard, prospective return, one-use; sizeof_ARState=40
```

The fixture first admits, arms after adverse evidence, then makes natural
recovery and support32 occur together to verify priority. It later arms
again, charges a neutral observation, earns and spends its return, and
checks that a second adverse/recovery excursion cannot spend another.
It also checks that quotes are pure and that a return changes the next
quote, not the already charged one. This fixture establishes implementation
order; material efficacy awaits the frozen fresh batch and independent reader.

`episode` was compiled from unchanged `turn13/episode.c`, frontend and
portable recurrence sources using the inherited build command. Its binary
SHA is identical to turn15's frozen binary.

## SHA-256 after the build

| File | SHA-256 |
| --- | --- |
| authority.h | `0fbc23a62920e03b580948a853fdea615d730ce5126147229d56f7d57caacdbb` |
| authority.c | `e4e894cbd8dcb19148aa639406f27ef20127253522ca07730bf07a5368f4f81c` |
| replay.c | `ffe2146dbecf6062e8dd9045ea0992c7bddc03b5d34d4011e9c9395025da33d5` |
| Makefile | `a7f83dd957c22fb00b5ffe737172633354cbf7ee819cc5b25a668b2d2316394d` |
| authority_fixture.c | `35f55e8a8692ce9a15d99836ad9d5f12e8624f5a677fb7d39c2bd2298fbcb23d` |
| authority_replay | `9631e4ede0b7c3d52f4686cbf30d33898344710da68c431798aa0bc027a635df` |
| episode | `784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10` |

The tracked-file diff remained empty: only the new `turn16/` directory is
untracked. No commits or publication were performed.
