# NETTA ZERO

**NETTA's Empirical Topological Training Agent** is a continual-learning
research organism that plays next-action games on immutable byte worlds.

This is the zero line: the earlier word-level prototype is invalid and is
kept only in git history below commit `5ea1374`. `NETTALOG2.md` is the sole
technical source of truth for the living line.

## What exists now

- The world is an immutable byte tape. Its canonical address is
  `(island, byte offset)`; every byte value is legal.
- The floor always has 256 atomic actions. Repeated lived sequences may earn
  byte-exact units without retokenizing or rewriting the world.
- Five prequential witnesses share one ruler, bits per raw byte:
  `atomic-uni`, `byte-bi`, `byte-tri`, `unit-uni`, and `move-bi`.
- Authority is earned and revocable. Byte actors compete on their lived
  records; the semi-Markov move actor must first survive real probation.
- The move actor can search its last 16 already-observed bytes for an exact
  semi-Markov route and run one model-only move ahead. Search never reads the
  target span, and its resulting policy is still priced by the external world.
- Mandates are global, verdicts are local. Every island keeps its own record
  of every witness; a travelling seat that cannot keep a KEEP lead over the
  island's own newborn record is refused on that island only, while the home
  mandate and kin transfer stay untouched.
- Experience can cross islands, while worlds, receipts, and counterfactual
  controls remain distinguishable.
- State is restart-safe and published atomically. Resume is refused if the
  ordered islands, state invariants, or external hash-chained biography do
  not match.

This is a foundation, not a finished language model. The present arenas are
mostly controlled synthetic worlds. There is no neural core, prompt mode,
dreaming, glyph system, island atlas, unit retirement, or unbounded life yet.

## Build and test

```sh
cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o netta
sh zero_tests.sh
```

The 80-gate suite includes red twins, restart equivalence, sanitizer runs,
matched transfer controls, played-action judgment, a causal-prefix search
twin, a random-order navigation null, island-local revocation arms, and
failure-closed state and biography checks. A passing build ends with
`ALL GATES PASS`.

## Start a life

```sh
./netta netta.txt \
  --reset \
  --seed 42 \
  --episodes 4 \
  --steps 800 \
  --state netta0.state \
  --bio netta0.bio.tsv
```

`--reset` begins a new biography. Without `--reset`, Netta resumes and
requires the same ordered list of island files:

```sh
./netta netta.txt \
  --episodes 4 \
  --steps 800 \
  --state netta0.state \
  --bio netta0.bio.tsv
```

Never use an island path as `--state` or `--bio`; Netta refuses aliases so
source truth cannot become writable memory.

## Multiple islands and matched controls

Register islands in a stable order and choose one by index:

```sh
./netta island-a.bytes island-b.bytes \
  --reset --island 0 --episodes 4 --steps 800 \
  --state voyage.state --bio voyage.bio.tsv

./netta island-a.bytes island-b.bytes \
  --island 1 --start 16 --episodes 2 --steps 800 \
  --state voyage.state --bio voyage.bio.tsv
```

`--start OFFSET` fixes the source address of each requested episode. It is a
measurement instrument for paired controls; ordinary lives omit it.

Useful experimental flags:

- `--no-units` disables the earned-vocabulary tissue.
- `--no-mv-nav` disables route search and restores the unanchored move player;
  this is the matched red control for the first earned move mandate.
- `--no-island-court` disables the local revocation valve; this is the matched
  red control for island-local authority.
- `--actor-lock uni|bi|tri|mv` pins an actor for a matched falsifier.
- `--seed N` initializes a newborn life. On resume, RNG continuity comes from
  state; use `--start` when source positions must be held equal across arms.

Counts must be finite non-negative integers. Infinite-life syntax such as
`--steps -1` is not part of NETTA ZERO.

## Living files

- `netta.c` — the organism.
- `NETTALOG2.md` — constitutional and empirical record.
- `zero_tests.sh` — executable research law.
- `netta.txt` — an optional raw-byte example island, not model authority.
- `netta0.state` — restart state (generated, not source truth).
- `netta0.bio.tsv` — append-only external biography (generated).

Negative experience is not cleaned out of the record. A failed claimant, a
reopened null, or a superseded measurement stays visible with its correction.
