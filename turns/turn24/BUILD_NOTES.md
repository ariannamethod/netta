# Turn24 build notes

What was compiled, what was inherited unchanged, how the inheritance was
proved, and what had to be repaired after the batch existed.

## Environment

- Apple clang 21.0.0, target arm64-apple-darwin25.4.0, `-O2 -std=c11 -Wall
  -Wextra -Wpedantic -Werror` on every translation unit.
- Python 3.14.4 (`/opt/homebrew/bin/python3`). The writer and the reader both
  call `math.exp2`, so the floor is 3.11; the system `/usr/bin/python3` (3.9)
  cannot run either and fails with `AttributeError: module 'math' has no
  attribute 'exp2'`. Everything ran with `PYTHONDONTWRITEBYTECODE=1`.
- No network. No git operation. Nothing outside `turns/turn24/` was written:
  the Makefile builds the inherited sources from their read-only locations
  into this directory, using a private `.build/` symlink layout for the one
  source (`../turn13/episode.c`) that still expects the old include tree.

## The four binaries

| binary | sources | sha256 |
| --- | --- | --- |
| `episode` | `../turn13/episode.c` + byte_recurrence + portable_recurrence | `784d549d01f04bc0…` |
| `authority_replay` | `../turn22/replay.c` + `../turn22/authority.c` | `b539bec2c1592f21…` |
| `split` | `../turn23/split.c` | `f25fd00ca4b859e6…` |
| `oracle` | `oracle.c` + `../turn22/authority.c` | `674d54501d82c5d1…` |

No inherited source was copied, edited or vendored. The five reference arms in
`authority_replay` and the sixth in `split` are the frozen laws themselves,
compiled; the oracle links the same `../turn22/authority.c` so its hazard
substrate is that law by construction rather than by resemblance.

Every inherited source was checked against the pin recorded in its own turn
before being compiled. `turns/turn22/{authority.c,authority.h,replay.c}` match
both `turn22/FREEZE.json` and `turn23/FREEZE.json`; `turns/turn23/split.c`
matches `turn23/FREEZE.json`; `turns/turn13/episode.c` matches turn13, turn21,
turn22 and turn23. All 36 pins are restated in `INHERITED_SHA256.txt`.

## Proving the reference arms are byte-equivalent

`turns/turn18/authority_replay` and `turns/turn21/authority_replay` are present
in this clone and match their own turns' FREEZE pins, so they are the authentic
binaries and not rebuilds. Three synthetic 16384-byte price tapes were fed to
all three replays and the shared columns compared as printed `%.17g` strings,
not as parsed floats:

| comparison | columns | rows | mismatches |
| --- | --- | --- | --- |
| turn24 vs turn18 (`slow`, `fast`, `witness` live/odds_before/slow_used/odds_after, `witness_w_before/after`, 7 base columns) | 21 | 3 × 16384 | 0 |
| turn24 vs turn21 (same plus `h8l4` live/odds_after/clipped, `witness_slow_used`) | 22 | 3 × 16384 | 0 |

`cmp turns/turn24/episode turns/turn21/episode` returns 0: the episode binary
rebuilds byte-identically from the frozen source on this toolchain, which is
independent evidence that the compilation is deterministic here.

Reading the laws directly confirms what the bitwise probe measures.
turn18's witness selects the fast hazard on `evidence > -1.0` and moves its
clock by `(31/32)w + delta` only when `matched >= 1`; turn22's `AR_WITNESS`
does the same through `mode != AR_FAST && evidence > -1.0` and
`mode >= AR_WITNESS && matched >= 1`. turn21's h8l4 is grid point
`{high 8, low 4}` and caps at `next.evidence <= -1.0 ? low : high`, computed
from the *updated* clock; turn22's `ar_cap` is called as `ar_cap(&next)` and
returns `evidence <= -1.0 ? 4.0 : 8.0`. Same predicate, same operand, same
order.

`selfnorm` and `split` have no authentic binary in this clone — turn22's and
turn23's are gitignored and absent. Their equivalence rests on two legs
instead: the source bytes are the pinned ones (verified above), and the
independent Decimal reader reimplements both laws from the `.c` files in a
different arithmetic and agrees with the compiled binaries to 1e-10. That is
weaker than a bitwise binary diff and is recorded as such.

## The oracle law

`oracle.c` is the only new law in this turn. It runs `slow` and `fast`
references internally and eight oracle arms over the same tape, and takes one
argument: the seam byte of a switched-law life, or `-1` when the life has no
law change.

The rate response is implemented by flipping the state's `mode` field from
`AR_SLOW` to `AR_FAST` at byte `seam+d`. In the frozen `ar_observe` the mode
enters only through
`slow = mode == AR_SLOW || (mode != AR_FAST && evidence > -1.0)`, and neither
`AR_SLOW` nor `AR_FAST` touches the clock or the cap, so the flip is exactly
"use hazard 2^-10 from this byte onward" and changes nothing else. The
per-byte `slow_used` column makes the hazard actually used auditable.

The level response additionally overwrites `odds` with the fast reference's
carried odds at that same byte, before the quote. Because both arms then hold
the same odds, the same activity flag and the same hazard, an O-level-d arm is
identical to `fast` for every byte from `seam+d` onward. That is a checkable
consequence, not an assumption, and `oracle.c` enforces it at run time
alongside three siblings:

- `pre-switch oracle odds left the slow reference` — before its switch byte
  every oracle arm must equal `slow` bitwise, in odds and in quote;
- `level oracle quote/state left the fast envelope` — from its switch byte on,
  every O-level arm must equal `fast` bitwise;
- `armed oracle used the slow hazard` — after the switch no arm may take 2^-16;
- `switch on a life with no law change` / `missing declared switch` /
  `second switch on one life` / `missing declared clamp` — end-of-stream
  checks that the label was used exactly where and as often as declared.

A violation aborts the run with a named message; none fired on any of the 32
lives. The same properties are re-derived independently by `verify.py` and by
`experiment.py`'s own column comparison, so three hands assert them.

## The label

The seam is constructional, not searched. `turns/turn13/experiment.py:91`
builds `commands['switched'] = commands['recombined'][:N//2] +
commands['unrelated'][N//2:]`, so the law changes at byte 8192 by definition.
`labels/ORACLE_LABELS.json` records `seam = 8192` per world together with four
facts measured on the world's own files:

- the emitted prefix is byte-identical to `recombined` — true in 8/8 worlds;
- the emitted tail differs from `recombined` in 95.25%–98.50% of positions
  (7803–8069 of 8192), reported as a measurement with no bar attached;
- the command prefix is identical to `recombined` — true in 8/8 worlds;
- the command tail is identical to `unrelated` — true in 8/8 worlds.

The last two are the generator line itself. They are worth stating precisely
because the two surfaces behave differently: on the *command* tapes the
switched tail is literally `unrelated`'s, while on the *emitted* bytes the
switched tail differs from `unrelated`'s in 8132–8176 of 8192 positions,
because emission is stateful and uses a different alphabet body and seed.
`verify.py` re-derives all four facts from the data without reading the label
file, and additionally re-derives the `moved_mid` surface bijection from its
declared seed, before comparing.

## Why the reader is independent

`verify.py` shares no line of policy code with the writer. It rebuilds every
arm from the retained candidate/P0 tape in Decimal probability-mass arithmetic
at precision 50, where the C hand works in float log-odds: the authority quote
is `cold + log2(C + S·2^delta)` against the C's
`logadd(cold, odds+candidate) - logadd(0, odds)`, the split quote reweights and
renormalizes three portfolios against the C's single `mix` ratio, and the level
clamp adopts the fast arm's *mass* where the C copies its *odds*. It never
reads any arm's own recorded state as an input.

Label confinement is proved as a property rather than read off a flag. The six
reference arms are rebuilt by objects constructed without the label, whose
`step` takes no label argument; a reference arm that had consulted the seam
could not agree with that recompute. The eight oracle arms are rebuilt with the
label, and their switch byte, hazard transition and clamp value are each
checked against the law. The reader also recomputes all eight bars and W1–W5
itself and refuses if any disagrees with `RESULT.json`, so the gate is
re-derived rather than trusted.

On the real batch: 32 lives, 524288 bytes, 7340032 arm-forecasts, maximum
numeric discrepancy **1.0345502232667059e-10** against a 1e-7 bar. Worst by
regime: switched 1.03e-10, moved_mid 1.42e-11, recombined 5.46e-12,
unrelated 0.

## Refusals

The turn23 regression is corrected: refusals name the artifact. Demonstrated
end to end after the batch, against real retained files, with rc read directly:

```
verify.py refuses: retained artifact …/turns/turn24/results/policies/world243/switched-oracle.tsv.gz:
  sha256 23defd014f76… does not match the recorded 03defd014f76…      rc=1
verify.py refuses: output file …/turns/turn24/VERIFY.json already exists   rc=1
```

The first was produced by altering a single hex digit in a copy of
`RESULT.json` and passing it with `--result`; no retained artifact was touched.

## Pre-freeze probes

All run before `freeze`, on synthetic tapes and synthetic result structures;
no world data existed. Recorded in the scratchpad as `PREFREEZE_PROBES.json`.

- **Reader laws against C**: 4 tapes × 16384 bytes × 14 arms, both seam values,
  maximum error 9.70e-10; every `slow_used`, `clamped` flag and clamp value
  matched exactly.
- **Bars, red individually**: all 8 bars turned false one at a time, writer and
  reader agreeing on every probe, with a green counter-probe where all 8 are
  true. Three bars are arithmetically coupled and are recorded as such:
  `b3`, `b4` and `b5` all read the moved-surface tail, so a perturbation large
  enough to break one can break its neighbours. Each was still shown to fail on
  its own account.
- **Gates, red individually**: W1 (oracle misses a moved bar), W2 (`d* = 0`),
  W3 (rate clears what level clears), W4 (every nominee clears everything),
  W5 (all 8 hygiene teeth broken one at a time), each with a green
  counter-probe; `d*` was driven to 0, 128 and 512 to show the curve reacts.
- **Schema**: the writer's actual serialized RESULT accepted, and each of the
  24 required keys removed in turn produced
  `RESULT.json schema is missing required key: <key>`.
- **Refusals**: digest drift and missing artifact both refused by name with the
  path printed, plus a green counter-probe; `--output` on an existing file
  refused with rc=1 through the real entry point.

Both suites were re-run after the post-data repair below, and the reader-law
probe output was bitwise identical to its pre-repair run.

## FREEZE_AMENDMENT — a post-data repair, declared

`FREEZE.json` was written before any byte of worlds 240..247 existed and has
not been rewritten. After the batch was generated, `labels` crashed:

```
FileNotFoundError: …/turns/turn24/data/world240/moved_mid.commands.bin
```

`labels()` in `experiment.py` and `derive_labels()` in `verify.py` both read
`*.commands.bin` for all four regimes, but `moved_mid` has no command tape —
turn22's `generate_world` builds it by applying a surface bijection to
`recombined.bin` rather than by emission. Both frozen scripts carried the same
bug, and the reader is required by W6, so there was no path that avoided
amending frozen code.

The repair is three hunks, recorded in full with both shas and a unified diff
in `FREEZE_AMENDMENT.json`: the same one-line command-tape fix in each script,
and an amendment-aware `check_freeze` that accepts a listed file at exactly its
frozen sha or its amended sha and nothing else. A third state was constructed
and confirmed to be refused by name.

No arm, bar, W-test, oracle law, clamp rule, reader law, tolerance or world
changed. `oracle.c` and all four binaries are byte-identical to the pre-data
freeze. Worlds 240..247 were generated once and never regenerated, and no
result had been computed when the repair was made — `RESULT.json` did not
exist and `evaluate` had not run.

This is a departure from "code frozen before data" and is Don's to accept or
reject. The diff is three hunks long and reproduces in one command.

## Stage log

| stage | rc | wall |
| --- | --- | --- |
| `make all` | 0 | — |
| `experiment.py freeze` | 0 | — |
| `experiment.py generate` | 0 | 10 s |
| `experiment.py extract` | 0 | 8 s |
| `experiment.py learn` | 0 | 12 s |
| `experiment.py labels` | 0 | — |
| `experiment.py evaluate` | 0 | 87 s |
| `verify.py` | 0 | ~4 min |

`results/` is 239 MB across 32 lives × 4 traces; nothing was deleted at any
point.
