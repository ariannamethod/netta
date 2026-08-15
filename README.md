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
  byte-exact units without retokenizing or rewriting the world. The
  vocabulary pays rent: a unit unrecognised for 16384 lived bytes dies and
  releases the living alphabet. Its frozen counts keep the history but lose
  all current probability mass; renewed support resurrects the same identity
  and restores that evidence, never a twin.
- Five prequential witnesses share one ruler, bits per raw byte:
  `atomic-uni`, `byte-bi`, `byte-tri`, `unit-uni`, and `move-bi`.
- Authority is earned and revocable. Byte actors compete on their lived
  records; the semi-Markov move actor must first survive real probation,
  and an island opens that probation only from its own matched shadow record.
- The move actor can search its last 16 already-observed bytes for an exact
  semi-Markov route and run one model-only move ahead. Search never reads the
  target span, and its resulting policy is still priced by the external world.
- Mandates are global, verdicts are local. Every island keeps its own record
  of every witness; a travelling hand that fails the local court is refused
  without losing its home mandate. Fixed uniform `null` is the island's
  eight-bit birth floor until a travelled byte hand earns a local 0.1-bit
  lead, and blind comity cannot be overrun by one oversized episode.
- Experience can cross islands, while worlds, receipts, and counterfactual
  controls remain distinguishable.
- With `--atlas`, navigation becomes an earned organ: among islands present
  in today's convoy, Netta first charts the least-lived shore to 1000 bytes,
  then chooses the lowest already-measured local byte-witness price. Every
  competitive autonomous choice records both winner and runner-up in the
  biography; with one present identity the Atlas is an exact no-op.
- An island's identity is its content, never its seat in today's command
  line. The life keeps an append-only registry of every island it has met
  (capacity 1024): a forward digest, an independently seeded reverse witness,
  and the byte length name the content; arrivals are loud biography events,
  islands absent from today's convoy keep their memory, and a changed file is
  by construction a different island. Simultaneous arrivals are canonically
  ordered by that identity, never by their CLI seats or the selected route.
- State is restart-safe and published atomically. Resume is refused if the
  state invariants or the external hash-chained biography do not match;
  every persisted island identity must agree with its external arrival
  receipt, so neither a forged absent record nor the convoy's order can
  silently change a life's identity.

This is a foundation, not a finished language model. The present arenas are
mostly controlled synthetic worlds. There is no neural core, prompt mode,
dreaming, glyph system, generalizing travel predictor, or unbounded life yet.

## Build and test

```sh
cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o netta
sh zero_tests.sh
```

The 116-gate suite includes red twins, restart equivalence, sanitizer runs,
matched transfer controls, played-action judgment, a causal-prefix search
twin, a random-order navigation null, island-local revocation arms, a fixed
uniform birth-floor control, byte-bounded comity, an island-local probation
door, unit death, tombstone silence and resurrection arms, the island
registry with fresh and resumed convoy-order invariance, an absent-identity
forgery arm, Atlas exploration and earned-choice arms, and failure-closed
state and biography checks. A passing build ends with `ALL GATES PASS`.

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

`--reset` begins a new biography. Without `--reset`, Netta resumes: islands
are recognised by content, in any order, and an unknown island simply joins
the life as an arrival:

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

Present the available islands and either choose one by index:

```sh
./netta island-a.bytes island-b.bytes \
  --reset --island 0 --episodes 4 --steps 800 \
  --state voyage.state --bio voyage.bio.tsv

./netta island-a.bytes island-b.bytes \
  --island 1 --start 16 --episodes 2 --steps 800 \
  --state voyage.state --bio voyage.bio.tsv
```

Or let the Atlas choose from the present convoy using only records already
earned by this life:

```sh
./netta island-a.bytes island-b.bytes \
  --atlas --episodes 4 --steps 800 \
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
- `--no-birth-floor` keeps the body-10 court but disables fixed uniform `null`
  and the byte-bounded comity rule; this is the matched red control for the
  island birth floor.
- `--no-local-probation` restores the old lifetime-wide probation promise;
  this is the matched red control for the island-local door.
- `--no-unit-death` lets the vocabulary keep its seats without rent; this is
  the matched red control for the dead-weight tax.
- `--keep-dead-mass` restores the body-12 leak, letting frozen tombstone
  counts enter current probability denominators; this is the red control for
  tombstone silence.
- `--atlas` enables earned navigation among the identities present in this
  invocation. Manual `--island N` remains the matched helm and the fallback
  when fewer than two distinct shores can be chosen.
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
