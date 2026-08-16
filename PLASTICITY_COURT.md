# The plasticity court: sealed specification for body 17

Status: **SEALED BEFORE ANY EXPERT RUN**
Date: 2026-08-16
Lineage: the buried prototype's plasticity genomes (old log, evolution
experiment 2), rebuilt under the zero courts. Specified after the
sixteenth-turn audit; to be built only after this specification survives
independent review. No gene range, fitness rule, control, threshold, or
promotion law below may change after any expert score has been observed.

## What is on trial

Hebb-v1 lost its matched frozen-reservoir control on every tested world
and is quarantined behind `--core-hebb-v1`. Its diagnosed defect is
positive feedback: gated potentiation drives hidden saturation
(`|h| 0.9840` at forty thousand bytes) which drives more gated events
(`+34732/-5076`). The frozen reservoir with a delta readout is the
reigning incumbent. This court exists to let plasticity constants earn
their seats instead of being declared -- and to accept, without shame,
the verdict that no candidate beats frozen.

## The jury

Eight genomes, the grave's own number. Each genome owns a complete
shadow copy of the core's mutable memory (`Wxh`, `Whh`, `Who`, prophecy
baseline, record); the innate embeddings are shared because they are
deterministic by construction. Every copy receives the identical lived
experience: the same truth bytes, in the same order, priced
prequentially before its own update, exactly as the default core is.
No genome consumes a draw from the life's rng; genome initialization
derives from the dedicated core seed and the genome index.

Genome zero is the frozen control: no recurrent plasticity at all, the
current default. It sits on the jury so that every candidate is judged
against the incumbent inside the same life, not against a remembered
number.

## The genes

A genome controls only what its own shadow record evaluates -- the
grave's gene-quarantine lesson, kept as law. The gene vector:

1. readout learning rate;
2. recurrent Hebbian rate (zero allowed: a genome may be frozen);
3. surprise gate threshold;
4. modulation clip;
5. input and recurrent decay;
6. a homeostatic activity target with its rate -- the anti-runaway gene
   class Hebb-v1 lacked: potentiation is discounted as mean `|h|`
   approaches the target, so saturation costs fitness before it costs
   bits.

Genes that would touch the shared embeddings, the readout law's form,
the alphabet, or anything outside the shadow copy are frozen by
constitution: what the shadow record cannot evaluate, the genome cannot
mutate.

## Fitness: selective transfer, never raw loss

The sealed Gutenberg verdict proved that raw transfer gain rewards
broad priors: the v19 core transferred 1.780461 to the technical donor
arm and 1.012479 to the shuffled twin. The shuffle numbers also carry a
maturity confound: a travelled witness beats a newborn on structureless
ground partly because its smoothing is mature, not because structure
crossed. The fitness law therefore discounts both:

- For each genome `g`, run the sealed transfer probes (donor life,
  fixed offsets, matched newborn controls) and record median kin gain
  `K_g`, median shuffle gain `H_g`, and median alien gain `L_g` for
  that genome's core.
- **Selectivity** is `S_g = K_g - H_g`: structure-selective transfer
  with census and maturity subtracted by the shuffled twin itself.
- **Breadth veto**: a genome is disqualified if
  `L_g - H_g > (L_0 - H_0) + 0.1`, where genome zero is the frozen
  control -- a candidate may not buy kin selectivity by growing even
  broader priors than the incumbent.
- **Health veto**: a genome is disqualified if its mean `|h|` exceeds
  0.5 on the forty-thousand-byte synthetic life, or if any weight
  leaves the clamp law, or if its synthetic p5-p8 records fall more
  than 0.1 bit/byte behind the frozen control on any world. Saturation
  is death before it is loss.

## Promotion law

The default plasticity seat changes only if some genome `g` satisfies
every veto and `S_g >= S_0 + 0.1` -- the appointment margin the whole
constitution uses, applied to selectivity. Ties keep the incumbent. A
promotion is a new body with its own version bump and its own red arm
preserving the defeated law, exactly as `--core-hebb-v1` preserves v1.
If no genome wins, the frozen incumbent remains and the verdict is a
published null: a result, not a failure of the court.

## The alien control must be measured, not named

The technical-English arm taught that genre intuition is not statistical
distance. Before any expert run, candidate alien tapes are measured
against the English references (the two sealed novels and `netta.txt`)
on two rulers: Jensen-Shannon divergence of the byte census, and
Jensen-Shannon divergence of the conditional bigram distributions. The
measured distances of the technical-English donor set the floor: a
candidate qualifies as alien only if it exceeds the technical donor's
distance on both rulers, and qualifies as a *world* (rather than noise)
only if its own bigram structure is real -- conditional entropy
measurably below census entropy. Candidate classes to measure: machine
code, non-Latin-script text, and structured non-language byte streams.
Random bytes are excluded by the structure requirement; English genres
are excluded by the floor.

## Instrument law

The jury is an instrument, not a resident: it runs only under an
explicit `--jury` invocation on arena lives, because eight shadow cores
multiply the per-byte cost. Jury weights, baselines, and records are
persisted under the same partial-forgery witness discipline as the
core, and a jury life split across restart must be bit-identical. The
jury holds no candidacy, no court privilege, and no biography authority;
its only output is the sealed fitness table and, at most, one promotion
verdict under the law above.

## What this court does not decide

Whether invocation flags belong in the state as a law mask (the open
question is now machine-demonstrated: a life resumed without
`--core-hebb-v1` silently continues under a different law); whether the
core ever earns candidacy in the actor courts; and whether the Atlas
receives a neural navigator. Those are separate bodies with separate
red worlds.
