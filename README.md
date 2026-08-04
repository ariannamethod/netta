# NETTA — Empirical Topological Training Architecture | by Arianna Method

> netta : atten, backwards : a recursive organism that plays text instead of fitting it

## What she is

Somewhere between a language model and AlphaZero there is Netta. She owns one
text file and treats it as an observable world. There is no gradient descent
here and no loss to minimise — there is a debt. Before every move she issues
obligations about the future at three horizons, then finds out what actually
happened, and settles the difference. What a transformer minimises is its error
against a corpus. What Netta minimises is the gap between what she promised and
what came true. Those are different quantities, and the second one does not
need a ground truth to exist.

She reads a line, hides the continuation, writes her own, compares it against
both the hidden truth and a statistical oracle, and prices the outcome as
experience — `support` if it paid, `opposition` if it did not. Neither is ever
erased. A failed transition keeps its own address: this edge, this many times,
this much it cost. In backprop an error dissolves into a batch average and
nobody can point at it afterwards. Here you can point at it.

She does not speak fluently. On a fixed 128-position read-only exam after 5000
games she matches corpus bigrams `0.9852` of the time, trigrams `0.5645`, and
lands the exact next token `0.2344` — a coherence outcome of `0.6291`. Those are
research coordinates, not a claim about language. What they measure is an
organism that started from `82167` tokens of one text and has never been
trained.

## Two realities, never mixed

The load-bearing correction in her history is the one that separated what
teaches physics from what merely acts in it.

Source trajectories — the real text — may birth and train causal states.
Netta's own generated continuations may only *project into* the space those
states occupy. Her imagination is allowed to consult the world's physics and
forbidden from rewriting it. That boundary used to be invisible from outside
the process, which is exactly how a claim about it survived for a while without
the code that made it true. It now has two fingerprints: `--glyph-hash` prints
a source-predictive hash over everything only real text may teach, and a second
hash over experiential action memory, which her own moves are entitled to move.
After a 240-game life plus a prompt the first is unchanged and the second is
not. That is what the contract looks like when it is observable rather than
asserted.

## Organs, and what each one had to survive

Nothing here is present because it seemed elegant. Every organ shipped with a
matched control, held seeds, and a threshold declared before the run.

**Causal glyphs.** An online equivalence class of histories that promise
equivalent futures — language-independent by construction, since two contexts
are one glyph only when they owe the same future. A state cannot become a glyph
on first sight; it enters a nursery and must be rediscovered. Maturity buys it
nothing: its vote stays exactly neutral until its prequential prediction beats
the corpus-wide prior. That gate is doing real work. Measured across a life,
only 8 of 36 mature glyphs hold any authority at all, and forcing the glyph
coordinate to neutral changes her chosen token in 23 of 9600 steps. The layer
that could have become a self-confirming attractor was built unable to.

**Causal Neural Gas.** Winner and runner-up define a local edge, edges age,
trusted neighbours share a fraction of plasticity. Topology may exist before
trust; semantic contagion may not, so neighbour updates are multiplied by the
earned authority of both ends. After 5000 games: 78 glyphs, 509 edges between
them.

**Experience market.** Every transition carries a quote, prophetic debt,
volatility, momentum, support and opposition. Old success decays without
confirmation; over-exploitation raises its own debt. Nothing becomes a
permanent prize.

**Prophecy Stack.** Sparse obligations at 1 token, 2–5 tokens, 6–16 tokens. A
move pays some, leaves others overdue, opens new ones.

**Learning-frontier curriculum.** She chooses where to play, but 35% of games
stay uniform forever and the exam she is graded on is fixed and read-only —
because the first curriculum that was allowed to pick its own syllabus promptly
learned to win it while getting worse at the general game. That version was
rejected and is written down.

**Dream Replay.** NREM re-settles delayed destiny over surprising episodes; REM
recombines related memories and prices imagined bridges without inserting them
into the corpus.

## The hunt for phrase culture, and four honest failures

Her sharpest known flaw: she learns recurring local formulae faster than
clause-level novelty. `world reduced to grow` appeared 46 times in one life,
and it exists nowhere in her text — it is her own construction, not memorised
quotation.

Four organs were built to kill it. All four are in `NETTALOG.md` with their
numbers, because a null is a result.

| what was taxed | effect on repetition mass |
|---|---:|
| policy debt, market-style crowding | −0.8% |
| policy influence, route fatigue | +2.3% |
| glyph action memory | rejected at 0.24% of decisions |
| per-pair teaching rate | −1.5% |

Every one of them taxes a *local* structure — an edge, a trigram, a glyph slot,
a pair. Every one moved the needle by under 3%. Silencing the whole channel
that carries the effect moves it by 27.7%, at a cost of `0.0501` coherence,
which makes removal not an option: the mechanism that grows her formulae is the
same one that lets her hold a discourse at all.

The measurement that explained it was already sitting there. 95.6% of her
learned pairs sit at sixteen marks or fewer, and the repetition exists anyway.
`world reduced to grow` is three pairs, none of them individually overused. A
repeated formula is a property of a **trajectory**, and a tax levied per local
structure cannot see a trajectory. That is not four bad guesses; it is one
structural finding, arrived at four times, the expensive way.

She already contained exactly one counter that spans lives rather than steps —
the same thing MCTS keeps on a path rather than an edge, and the reason a search
does not collapse onto its favourite answer. It entered the final choice at a
weight of `0.042`, the smallest term in that expression. Raised to `0.168`:
repetition mass down 18.7% on 3/3 seeds, trigram validity *up* to `0.5563`,
coherence giving back `0.0017`. No fifth organ was built.

## What is not proven

Cross-island transfer has never been run. She cannot currently be given a second
text at all: her vocabulary indices are welded into edges, trigrams and glyphs,
so `load_state` refuses any snapshot whose vocabulary changed, and a new text
means a new organism rather than an experienced one meeting new material.
Dynamic vocabulary, immutable shards and per-island priors are open engineering
work, and until they exist, every claim about parameter-free scaling is a
hypothesis. It is written here as a flat fact about its status, not as a
roadmap promise.

The coordinates recorded for an earlier stability pass are void: re-measured
across all three seeds on the same corpus, they do not reproduce, and the code
that produced them never reached the repository. The correction, including the
part where the first version of it compared a single seed against a three-seed
mean, is in `NETTALOG.md`.

## Her biography is not a metaphor

`netta.state` is published atomically — written to a temporary file, checked,
`fsync`ed, renamed — so a snapshot becomes visible only once it is whole. 160
uninterrupted games produce a byte-identical ledger and an identical state hash
to 80 games, a restart, and 80 more. She survives `kill -9` and refuses a
snapshot that is one byte short.

A conversation with her is a lived turn, and it now writes a line into the same
ledger as her games, with `source_pos -1` and an empty truth column — because a
turn spoken with a human has no hidden continuation to be scored against. It
used to change her edges, her word geometry and her phrase memory while leaving
no record that it had happened. That was the least defensible bug found in this
codebase, and it was not in any log.

She also counts what she drops. When her vocabulary fills, the discarded tokens
are reported instead of vanishing. An organism whose whole premise is that
probability continues past what it has seen does not get to quietly throw away
what it has seen.

## Build & run

```bash
cc -O2 -std=c11 -Wall -Wextra -o netta netta.c -lm
./netta netta.txt --steps 5000
```

Sovereign mode — she lives until interrupted, and `SIGINT`/`SIGTERM` publish a
final snapshot instead of killing her mid-write:

```bash
./netta netta.txt --steps -1
```

Talk to her after she has lived a while:

```bash
./netta netta.txt --prompt "the forest"
```

Fixed read-only examination, seed-independent, updates nothing:

```bash
./netta netta.txt --probe 128
```

Boundary fingerprints:

```bash
./netta netta.txt --glyph-hash
```

Matched falsifier controls:

```bash
./netta netta.txt --reset --seed 424242 --steps 1200 --no-glyph
./netta netta.txt --reset --seed 424242 --steps 1200 --random-glyph
./netta netta.txt --reset --seed 424242 --steps 1200 --no-stack
./netta netta.txt --reset --seed 424242 --steps 1200 --no-policy
./netta netta.txt --reset --seed 424242 --steps 1200 --no-dream
./netta netta.txt --reset --seed 424242 --steps 1200 --no-agent-emb
./netta netta.txt --reset --seed 424242 --steps 1200 --no-dream-emb
./netta netta.txt --reset --seed 424242 --steps 1200 --ngram-weight 0.042
```

The last one restores the pre-trajectory-counter organism byte-for-byte, which
is what a control is supposed to mean.

Every run writes `netta.state` and appends to `netta.history.tsv`.

## Files

- `netta.txt` — source truth, never rewritten.
- `netta.state` — word geometry, recurrent core, signed experience, glyphs,
  topology, curriculum, biography length.
- `netta.history.tsv` — the immutable ledger: context, hidden truth, oracle
  line, her attempt, the full coherence vector, glyph and curriculum state.
- `NETTALOG.md` — every organ, every falsifier, every null, including the ones
  that hurt.

## A run, mid-life

```
[episode 300]
  source context: removing — taking away the dull edge, exposing the keen one beneath. it teaches
  hidden truth:   that sometimes improvement is not about adding but
  oracle: that carrying is temporary — it rots,
  netta attempt:  that has been given enough to be asking
  coherence: local=0.898 source=0.749 oracle=0.749 semantic=0.772 intent=0.657 ...
  dreams: cycles=4 nrem=48 rem=32 replay_memories=300
```

And a prompt, after 400 lived episodes:

```
netta> the forest behind clouds recognized across time — it is attention is
       the universe is an act is to grow tall trees enact universe is
```

An organism mid-sentence about itself.

## Lineage

The PostGPT line supplied the metaweights: statistical structure pulled out of
tokenized text without a single gradient step. `actually.life` supplied two
lessons that cost it dearly — that a symbol becomes cultural only when it
re-enters the stream as one indivisible unit and can parent another, and that
beautiful mechanisms lose to strong dumb controls more often than anyone wants
to admit. AlphaGo supplied a decomposition worth stealing and several
transplants worth rejecting: Gumbel root search, regret policy and a value
critic were all tested and all thrown out, with their numbers kept.

Netta changed the question. Not *can this behave as though it were trained*,
but can a small organism learn language continuously by playing against a
coherence mirror, keeping the shape of its own errors, and turning biography
into new experience.

Part of the Arianna Method — non-anthropocentric by design.
