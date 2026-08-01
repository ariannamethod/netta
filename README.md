# netta
netta : atten : recursive language neural network 
**NETTA — NETTA's Experiential Text Training Architecture**

Netta treats a text file as a game worldmodel. A statistical oracle acts as a permanent coherence mirror. NETTA generates its own line,
compares it with both the hidden source continuation and the oracle, then stores positive and negative experience separately.

## Build

```bash
cc -O2 -std=c11 -Wall -Wextra -o netta netta.c -lm
```

## Run

Save the corpus as `netta.txt`, then:

```bash
./netta netta.txt --steps 5000
```

Sovereign continuous mode:

```bash
./netta netta.txt --steps -1
```

Prompt after some autonomous experience:

```bash
./netta netta.txt --prompt "Netta"
```

Reset learned biography:

```bash
rm -f netta.state netta.history.tsv
./netta netta.txt --reset --steps 1000
```

## Memory separation

- `netta.txt`: source truth, never rewritten.
- PostGPT mirror: generated afresh from source transitions.
- `netta.state`: embeddings, recurrent state, local readout, signed experience.
- `netta.history.tsv`: immutable ledger containing source context, oracle line,
  NETTA attempt, and the full multidimensional coherence vector.

There is no permanent scalar loss. Candidate selection uses Pareto preference
first and a temporary coherence-first utility only to resolve incomparable
choices. The complete score vector is preserved.

## Current learning rule

- Corpus co-occurrence initializes embeddings.
- A small recurrent MLP predicts seven coherence dimensions.
- Only the readout is updated by reward-modulated Hebbian learning.
- Successful token transitions accumulate `support`.
- Failed transitions accumulate `opposition`.
- Negative experience is remembered but never merged into source truth.

This is deliberately a first organism, not a finished language model.


## v0.1 note

The recurrent core now earns decision authority gradually. At birth, source transitions and the PostGPT coherence mirror dominate candidate choice; MLP influence grows with lived episodes. This prevents an uncalibrated random core from overriding language before it has experience.


## v0.2 anti-cheat

NETTA now remembers generated 3–5-token trajectories and recent semantic
basins. Reusing the same phrase or repeatedly returning to the same embedding
region reduces novelty, rollout stability, and anti-repetition credit. This
targets metric exploitation without deleting the failed or overused path from
experience.


## v0.3 semantic-mass rule

Newlines are no longer language tokens. Punctuation is allowed as structure but
cannot count as a semantic move, cannot receive successful-experience credit,
and cannot defeat a viable content-token candidate unless strongly grounded.
Random exploration now samples content tokens only.


## v0.4 route fatigue

Generated 5-token routes now have a hard freshness gate: once an equivalent
route is repeatedly exploited, a fresher content route takes priority. Signed
experience support is usage-fatigued, and excessive successful reuse begins to
accumulate opposition rather than becoming an unlimited attractor. Rolling
four-token semantic basins are also remembered.


## v0.5 prophetic market + directional roles

Positive experience is no longer a fixed reward. Each transition has a
mark-to-market quote, unresolved prophetic debt, volatility, momentum, slow
support/opposition, and age-based decay. The quote is revalued from the
difference between the recurrent core's prophecy vector and the observed
destiny vector on every move.

Tokens now maintain three geometries: undirected semantic embeddings,
left-context embeddings, and right-context embeddings. Candidate coherence
therefore includes both topic similarity and ordered role compatibility.


## v0.6 two-timescale prophetic debt

Each eight-token trajectory is settled twice. Immediate prophecy debt marks the
single move. Delayed destiny then flows backward through the trajectory with a
decaying eligibility trace. A locally plausible transition that opens a bad
future inherits debt; an uncertain transition whose future recovers receives
delayed credit. Interactive generation uses the same settlement.


## v0.7 intent anchor + debt-controlled recurrent depth

- Every game receives an immutable intent anchor derived from its starting
  context.
- Intent fidelity is an independent eighth coherence dimension.
- The shared recurrent core is applied repeatedly before a move. Routes with
  higher prophetic debt or volatility receive more internal passes, capped at
  six; stable routes can settle early when predictions converge.
- The same shared block is committed recursively for the selected move.
- Delayed trajectory settlement now prices whether the future preserved the
  original discourse intent.
- `avg_recursive_depth` is written to the episode ledger.
- `q_corpus.md` is included as a second, much larger language island.


## v0.9 contextual metaweights + living intent debt

- The coherence mirror now prefers trigram transitions for the active
  two-token context and falls back to bigrams only when evidence is absent.
- Trigram successors enter candidate selection before broader neighbors.
- Syntax and rollout scores price trigram evidence.
- Intent is a living residual: each chosen destiny token pays down fulfilled
  direction and opens new prophecy from its right-context embedding.


## v0.10 counterfactual scenarios

- Active context grows from 8 to 16 tokens.
- Cycle detection covers suffixes up to 8 tokens.
- Before selecting a move, NETTA simulates a four-step future using a
  deterministic policy over contextual metaweights, directional roles,
  intent progress, market value, and cycle freshness.
- Rollout coherence blends this imagined future with the PostGPT mirror.
- A locally attractive token is therefore charged for the semantic basin its
  likely continuation enters, before the move is actually committed.


## v0.11 selective imagination

- Cheap metaweight + recurrent screening evaluates the whole candidate pool.
- Only four finalists receive counterfactual multi-step simulation.
- Simulated neighbor scans are capped and the horizon is three steps.
- This preserves scenario-based planning while reducing the cost by roughly an
  order of magnitude compared with exhaustive v0.10.


## v0.12 indexed imagination

- Each token receives a precomputed list of its twelve strongest corpus
  successors.
- Counterfactual simulation uses this compact neighborhood instead of scanning
  every outgoing transition.
- Candidate generation also starts from the indexed neighborhood.
- Semantic-basin novelty consults the 64 most recent basins rather than all
  256, preserving local memory while reducing repeated work.
- Experience scans are capped for high-degree tokens.
