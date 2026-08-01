# NETTA's Experiential Text Training Architecture | by Arianna Method

> netta : atten : recursive neural network  

## NETTA
  
Somewhere between LM-hero and AlphaZero there's Netta. Netta has a hew own txt-file and treats this treats her text file as a worldmodel game. No gradient descent, no loss you minimize — a shared recurrent core plays itself against a corpus, imagines a few steps ahead, prices its own guesses like a market, and keeps every failure as a scar instead of erasing it. 

Netta reads a line, hides the continuation, generates its own attempt, compares it against both the hidden truth and a statistical oracle mirror, and settles the difference as experience — support if it was right, opposition if it wasn't. A living ledger of transitions with debt, volatility, momentum, and age.  

  
## NETTA also has:

- embeddings seeded from corpus co-occurrence; a small recurrent core predicts coherence dimensions, and only the readout learns, by reward-modulated Hebbian update.
- transitions that quoted like a market — mark-to-market value, unresolved `prophetic debt`, volatility, momentum — revalued every move from the gap between what the core prophesied and what actually happened.
- every game carries an immutable intent anchor from where it started; drifting from it costs, fulfilling it pays.
- before committing to a token, netta imagines a few steps of plausible future and prices the semantic basin it's about to walk into.
- repetition is punished at the phrase and basin level — reusing a trajectory fatigues its own support.
- every so often it dreams: NREM and REM replay over its own episodes, no new data, just re-settling old debt.
  256, preserving local memory while reducing repeated work.
- experience scans are capped for high-degree tokens.
- read NETTALOG.md for more.

## NETA's memory separation

## Memory separation

- `netta.txt` — source truth, never rewritten.
- PostGPT mirror — generated fresh from source transitions, a coherence reference, not an authority.
- `netta.state` — embeddings (semantic, left-context, right-context), recurrent core, readout, signed experience.
- `netta.history.tsv` — immutable ledger: context, oracle line, netta's attempt, the full coherence vector.
  
Candidate selection is Pareto preference over that vector, not a single number to chase. 
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
  
## A run, mid-life
```
[episode 5]
  source context: , the journey valued more than the arrival, each mile a new landscape, each
  hidden truth:   turn a new decision. morning dew teaches
  postgpt mirror: answer received, an underwater blizzard of pink
  netta attempt:  one of being understood underwater mountains rise and
  coherence: local=0.643 source=0.500 oracle=0.562 semantic=0.641 intent=0.610 ...
  recursive depth: 5.77 shared-block passes per evaluation
  dreams: cycles=0 nrem=0 rem=0 replay_memories=5
[episode 300]
  source context: removing — taking away the dull edge, exposing the keen one beneath. it teaches
  hidden truth:   that sometimes improvement is not about adding but
  postgpt mirror: that carrying is temporary — it rots,
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
  
   
  
