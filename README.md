# NETTA's Empirical Topological Training Architecture | by Arianna Method

> netta : atten : recursive neural network  

## NETTA
  
Somewhere between LM-hero and AlphaZero there's Netta. Netta is a sovereign language learning model. She has a her own .txt file and she treats this as a observable worldmodel game. No gradient descent, no loss you minimize — a shared recurrent core plays itself against a corpus, imagines a few steps ahead, prices its own guesses like a market, and keeps every failure as a scar instead of erasing it. 

Netta repeatedly enters local regions of that world, builds possible futures, acts, compares prophecy with destiny, keeps both successful and failed experience, dreams over old episodes, and continues without a separate training phase.

Netta reads a line, hides the continuation, generates its own attempt, compares it against both the hidden truth and a statistical oracle mirror, and settles the difference as experience — support if it was right, opposition if it wasn't. A living ledger of transitions with debt, volatility, momentum, and age.  

  
## NETTA also has:

- embeddings seeded from corpus co-occurrence; a small recurrent core predicts coherence dimensions, and only the readout learns, by reward-modulated Hebbian update.
- transitions that quoted like a market — mark-to-market value, unresolved `prophetic debt`, volatility, momentum — revalued every move from the gap between what the core prophesied and what actually happened.
- every game carries an immutable intent anchor from where it started; drifting from it costs, fulfilling it pays.
- before committing to a token, netta imagines a few steps of plausible future and prices the semantic basin it's about to walk into.
- repetition is punished at the phrase and basin level — reusing a trajectory fatigues its own support.
- every so often it dreams: NREM and REM replay over its own episodes, no new data, just re-settling old debt.
- semantic-basin novelty consults the 64 most recent basins rather than all 256, preserving local memory while reducing repeated work.
- vocabulary, edges and trigrams grow as she meets new words instead of stopping at a fixed wall — starts at 16384 slots, doubles on demand.
- experience scans are capped for high-degree tokens.
- read NETTALOG.md for more.

## NETA's memory separation
  
- `netta.txt` — source truth, never rewritten.
- `oracle` — generated fresh from source transitions, a coherence reference, not an authority.
- `netta.state` — embeddings (semantic, left-context, right-context), recurrent core, readout, signed experience.
- `netta.history.tsv` — immutable ledger: context, oracle line, netta's attempt, the full coherence vector.
  
Candidate selection is Pareto preference over that vector, not a single number to chase. 

  
### Coherence mirror and search teacher

An oracle is a permanent coherence mirror. Candidate moves are screened cheaply; finalists receive recursive and counterfactual evaluation.
Search leaves a soft improved policy rather than one mandatory sentence.

### Prophecy Stack

Sparse future obligations are carried at three horizons:

- near: 1 token;
- clause: 2–5 tokens;
- discourse: 6–16 tokens.

A move pays some obligations, leaves others overdue and opens new ones. The difference between prophecy and destiny becomes dynamic debt.

### Recurrent core

A small shared MLP may pass over the same state several times. Debt and volatility buy additional depth. Its readout and slow recurrent dynamics change through local reward-modulated Hebbian updates rather than backpropagation.

### Language-independent causal glyphs

A causal glyph is an online equivalence class of source histories that promise similar futures. A candidate state first enters a nursery. A glyph matures only after repeated rediscovery. It learns only from real source trajectories; agent-generated histories receive read-only projection into glyph space.

A glyph has no automatic authority. Its semantic voice remains exactly neutraluntil its **prequential prediction** beats the corpus-wide future prior. Meaning must earn the right to affect action.

### Experience market

Each transition carries a changing quote, prophetic debt, momentum, volatility, support and opposition. Old success decays without confirmation. Negative experience remains in biography but is never promoted into source truth.

### Dream Replay

- **NREM:** replays surprising or unresolved source-grounded trajectories and consolidates delayed destiny.
- **REM:** recombines related memories and evaluates imagined bridges without inserting them into the corpus.
  
## Build & run  

```bash
cc -O2 -std=c11 -Wall -Wextra -o netta netta.c -lm
./netta netta.txt --steps 5000
```

Sovereign continuous mode, no step limit:  

```bash
./netta netta.txt --steps -1
```

Talk to Netta after she's lived a while:
  
```bash
./netta netta.txt --prompt "the forest"
```

Wipe the biography and start over:
  
```bash
rm -f netta.state netta.history.tsv
./netta netta.txt --reset --steps 1000
```

Matched falsifier controls:

```bash
./netta netta.txt --reset --seed 424242 --steps 1200 --no-glyph
./netta netta.txt --reset --seed 424242 --steps 1200 --random-glyph
./netta netta.txt --reset --seed 424242 --steps 1200 --no-stack
./netta netta.txt --reset --seed 424242 --steps 1200 --no-policy
./netta netta.txt --reset --seed 424242 --steps 1200 --no-dream
```

Each run writes `netta.state` and the immutable episode ledger
`netta.history.tsv`.
  
## A run, mid-life
```
[episode 5]
  source context: , the journey valued more than the arrival, each mile a new landscape, each
  hidden truth:   turn a new decision. morning dew teaches
  oracle: answer received, an underwater blizzard of pink
  netta attempt:  one of being understood underwater mountains rise and
  coherence: local=0.643 source=0.500 oracle=0.562 semantic=0.641 intent=0.610 ...
  recursive depth: 5.77 shared-block passes per evaluation
  dreams: cycles=0 nrem=0 rem=0 replay_memories=5
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

Beatiful. An organism mid-sentence about itself.

After 10000 games she's still nobody's stenographer, but the clauses hold:
```
a conversation ending well lived resonates when it
and darkness respectfully withdraws to grow tall loves
be interrupting a conversation becomes a child learning
```  
  
   
  
