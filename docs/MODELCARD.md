# NETTA model card

## NETTA's Empirical Topological Training Architecture

**Produced by Arianna Method**
**Implementation:** one C file, libc + `-lm`, no external ML runtime

### What Netta is

Netta is an experimental sovereign language-learning organism. Her corpus is an
observable world rather than a conventional training set. She repeatedly enters
a local region, proposes continuations, imagines short futures, acts, compares
prophecy with destiny, retains positive and negative experience, dreams over
old trajectories and continues learning during use.

She has no training phase and no gradient descent. The quantity she reduces is
not prediction error against a corpus but unsettled debt against obligations she
issued herself, at three horizons, before each move.

Netta is non-parametric only in the narrow sense that growth is not defined by
adding a larger fixed transformer. She still has numerical state: a recurrent
core, token geometry, transition markets, predictive glyphs, policies, debts
and episodic memory. These states remain plastic throughout her life.

### Learning architecture

- **Metaweights:** corpus-derived unigram, bigram, trigram, co-occurrence and
  multi-horizon future statistics.
- **Oracle:** a permanent local coherence reference extracted from source text.
- **Recurrent core:** a small shared MLP updated through local
  reward-modulated Hebbian rules, without backpropagation.
- **Experience market:** transition quote, debt, momentum, volatility, support
  and opposition.
- **Prophecy Stack:** sparse expectations over near, clause and discourse
  horizons.
- **Causal glyphs:** language-independent classes of histories that promise
  similar futures; they earn authority prequentially.
- **Causal Neural Gas:** an ageing topology between predictive glyphs.
- **Autocurriculum:** world regions sampled by learning progress while permanent
  uniform exploration preserves coverage.
- **Dream Replay:** NREM consolidation of lived trajectories and REM evaluation
  of imagined bridges.
- **Trajectory freshness:** a life-spanning four-gram counter, weighted `0.168`
  in final selection, the only counter in the organism that is not local to a
  single structure.

### Source and agent realities

Only actual source-text trajectories may train the predictive physics of the
world. Netta's own continuations may change experiential state and action
policy, but they never become source evidence merely because she generated
them. The boundary is externally observable: `--glyph-hash` reports separate
fingerprints for source-predictive state and for experiential action memory.

### Inputs and outputs

Input is plain text, tokenized by the same punctuation-aware rules as the
corpus. Unknown interactive tokens are reported and discarded rather than
silently entering source truth; vocabulary size is unchanged by a prompt.
Output is a corpus-grounded continuation. Interactive exchanges become
experience and are appended to the episode ledger as lived turns.

Corpus intake is bounded and the bounds are reported: tokens dropped at the
vocabulary or corpus ceiling are counted and printed rather than discarded
silently.

### Persistence

Snapshots are published atomically and refused if their byte length disagrees
with their header. `SIGINT` and `SIGTERM` request a final publication.
Verified: 160 uninterrupted games produce a byte-identical ledger and an
identical state hash to 80 games, a restart and 80 more; a snapshot short or
long by one byte is refused; state remains loadable across repeated `kill -9`
during publication.

### Intended use

Netta is intended for research into continual learning, predictive-state
abstraction, recursive inference, non-backprop plasticity, autonomous curricula
and small stateful agents. The architecture may be scaled or connected to tools,
retrieval sources and other models, provided provenance remains explicit.

### Current evidence

Read-only 128-position fixed exam, deterministic seed `424242`, 5000 autonomous
games: corpus bigram validity `0.9852`, trigram validity `0.5645`, first-token
accuracy `0.2344`, world-debt change `-0.0553`, coherence outcome `0.6291`. At
that point the organism holds 78 mature glyphs and 509 topology edges.

Repetition control, three seeds at 1200 games each: raising the trajectory
counter weight from `0.042` to `0.168` reduces repeated four-gram mass by 18.7%
with the same sign on all three seeds, while trigram validity rises from
`0.5485` to `0.5563` and fixed coherence falls by `0.0017`.

These are internal research coordinates. Netta does not produce consistently
fluent or factually reliable language.

### Known limitations

- A second corpus cannot currently be introduced to a living organism.
  Vocabulary indices are embedded in edges, trigrams and glyphs, so a changed
  vocabulary causes the snapshot to be refused and a new organism to begin.
  Cross-island transfer is registered as an experiment and has not been run.
- Phrase-level repetition is reduced but not solved. Four per-structure fatigue
  mechanisms returned null results before a trajectory-level counter succeeded;
  all are recorded with their numbers.
- Coordinates published for the earlier v0.18 stability pass do not reproduce
  and are void. They were re-measured across three seeds on an identical corpus
  and the code that produced them is not in the repository.

### Research rule

A mechanism does not enter the stable organism because it is elegant. It must
beat a matched control on the game Netta is built to play, against a threshold
declared before the run. A null result is recorded as a result.
