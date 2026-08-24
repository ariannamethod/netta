# NETTA TRANSFER COURT — PREREGISTRATION (2026-08-24)

Frozen before any transfer-body code. Body 0 is closed: its results,
thresholds and interpretation are not touched. Any change to this
file after the first measured run is a new experiment, named aloud.

## Two standing laws

1. **The dead field does not resurrect under a new name.** Body 0's
   Hebbian H is deleted and stays deleted. Transfer memory is built
   anew from the surviving representation — unit streams and the
   renaming-invariant descriptors defined below. No copy of H, no
   renamed field mechanism. The independent verifier checks the code
   for this by reading it.
2. **Global experience != local authority.** Past experience may
   offer a prior or a correspondence; it receives local weight in the
   new world only from prospective evidence of that world, by the
   frozen weight law below, starting at the minimum.

## Source and destination worlds (all pinned before code)

Source S = World A of Body 0: `netta.txt`, 447545 bytes, SHA-256
`02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb`.
The traveller's past = S train (first 90%, Body 0 split law).

Destination worlds are CONSTRUCTED, so the court knows the truth:

- **W-iso** — same structure, different alphabet: byte-substitution
  cipher of the whole of A. Permutation of byte values 0..255 by
  Fisher–Yates under the frozen xorshift64 law (x^=x<<13; x^=x>>7;
  x^=x<<17), seed `0xC1F3E5`. The true permutation is the court's
  oracle and is never shown to the organism.
- **W-struct** — similar frequencies, different structure: a
  Markov-1 byte stream of length 447545 sampled from A-train's
  bigram table, frozen seed `0x5EED01`. Long structure is dead by
  construction; experience must not impose itself here.
- **W-ff** — the false-friend world: A with the 16 most frequent
  whitespace-delimited words of A-train swapped in pairs by
  frequency rank (1st with 2nd, 3rd with 4th, ...), all occurrences,
  deterministic, no seed. Identical surface units deliberately
  occupy different relational roles; the true swap map is the
  court's oracle for this world.
- **W-blind** — a natural fourth world chosen by Oleg or Mila only
  AFTER the transfer-body code is frozen and hashed; exploratory,
  not a gate.

The construction of W-iso, W-struct and W-ff is emitted as
artifacts with SHA-256 before any arm runs.

## The organism under the court

The base learner in every arm is Body 0's surviving representation:
learned units + the exact probability chain of PROTOCOL.md, WITHOUT
the field. One frozen amendment for this court only: the BPE
tie-break is **first position of pair appearance in the stream**
(earlier wins), not lexicographic ids — the unit learner must be
permutation-equivariant so that A and cipher(A) grow isomorphic
units by construction. The verifier proves this: units grown on A
and on cipher(A) must be identical up to the known byte permutation.

**Prequential adaptation law.** A destination stream is consumed in
frozen chunks of 1024 bytes. Each chunk is priced (bits, by the
Body 0 probability chain over the organism's current units and
counts) BEFORE being absorbed; after pricing, the chunk joins the
lived prefix, and units and counts are rebuilt from that prefix.
loss(arm, t) is the priced bits of chunk t.

## The transferable memory (renaming-invariant by construction)

For every unit u of a lived world, a descriptor of 12 frozen
scalars, computed from that world's lived stream only:

1. log2 frequency;
2. unit byte length;
3. right transition entropy (bits) over unit bigrams;
4. left transition entropy;
5. log2 right degree (distinct right neighbours);
6. log2 left degree;
7. top-1 right continuation probability;
8. top-4 right continuation mass;
9-11. co-occurrence mass at distances 1, 2, 4 (each = sum of
   1/(1+d)-decayed co-occurrence within that exact distance,
   normalized by frequency) — aggregated scalars, no neighbour names;
12. one refinement step: transition-probability-weighted mean of
   right neighbours' right-entropies (a one-step graph-role
   signature).

Descriptors are z-scored per world over that world's own units.
Similarity = negative Euclidean distance in z-space. No neighbour
identity ever enters the descriptor: the profile is invariant to
unit renaming by construction.

## Arms (equal budget; frozen)

Competition arms:

1. **cold** — newborn: no past.
2. **cache** — past-cache only, active from byte 0: at each
   prediction, the current context unit is matched on the fly to the
   nearest A-unit by descriptor similarity (descriptors of the
   destination side computed prospectively from its lived prefix
   only); the A-side match contributes a prior that redistributes
   probability over the local candidate set by descriptor closeness
   of each candidate to the matched A-unit's top-4 continuation
   descriptors: prior score r(u) = -min over those 4 of z-distance;
   P_prior = softmax over candidates of r(u).
3. **align** — late structural alignment only: cold until the frozen
   warmup of 16384 bytes; then a one-to-one greedy alignment between
   the top-256 units of A and the top-256 units of the destination
   prefix by descriptor similarity, accepting pairs with z-distance
   <= 2.0; after that, the aligned A-unit's continuation
   distribution, carried through the alignment map (only aligned
   continuations count), is the prior. Realignment at every chunk
   boundary thereafter.
4. **cache+align** — both mechanisms; before warmup, the cache
   prior; after warmup, the mixture of the two priors in equal
   halves.
5. **shuffled** — the interference control: identical machinery to
   cache+align, but A-side descriptor rows and continuation tables
   are permuted by Fisher–Yates, frozen seed `0x54AFF1E`, breaking
   every correspondence while preserving volume and distributions.

Control arm, out of competition:

6. **oracle** — the true correspondence (byte permutation for W-iso,
   word-swap map for W-ff) replaces the learned matcher; everything
   else as cache+align. Upper bound only; it can never win the
   court, only calibrate it.

**Prior weight law (frozen).** In every past-carrying arm:
P_final(u) = (1 - L_t) * P_local(u) + L_t * P_prior(u), with
L_0 = L_min = 0.01, L_max = 0.5, and after each priced position t:
L_{t+1} = clamp(L_t * exp(0.05 * (l_local,t - l_prior,t)), L_min,
L_max), where l are the -log2 prices of the truth under each
component. The prior's weight starts at the minimum and grows only
by beating the local model prospectively. All constants frozen now;
they are positions, and their revision after a measured run is a new
experiment.

## Ruler (frozen)

Primary: early prospective adaptation.
G_N(arm) = sum over chunks within the first N bytes of
(loss(cold, t) - loss(arm, t)), in bits. Horizons: N in {1024, 4096,
16384, 65536}; G_16384 is the deciding horizon, the others are
reported. Material margin: MARGIN(N) = 0.01 * N bits (the Body 0
margin scaled to the horizon; at the deciding horizon ~164 bits).
Final bits/byte after the full stream is reported as telemetry,
never as the transfer verdict.

Anti-smoothing difference: the reported transfer effect is
D = G_16384(arm, W-iso) - G_16384(arm, W-struct); generic smoothing
gains appear on both sides and cancel.

## PASS / FAIL (frozen wording)

- **"Transfer earned (predictively)"** for an arm iff, at the
  deciding horizon: G_16384 >= MARGIN on W-iso; the arm beats
  shuffled by >= MARGIN on W-iso; D >= MARGIN; and the arm does not
  fail the false-friend gate. Ceiling: status is predictive only —
  speech-side value needs its own future court.
- **"Transfer not detected"** iff no competition arm meets the
  above. Additionally, if even oracle fails G_16384 >= MARGIN on
  W-iso, the verdict is "transfer not detected: carried memory
  insufficient" — the matcher is exonerated, the memory itself is
  the failure.
- **"Past experience interferes"** for an arm iff
  G_16384 <= -MARGIN on any world. Recorded as a full result, not
  smoothed.
- **False-friend gate:** on W-ff, any arm with
  G_16384 <= -MARGIN fails with "surface authority leaked"; anchors
  may propose, never command.
- **Non-imposition gate:** on W-struct, any past-carrying arm with
  G_16384 <= -MARGIN fails with "past imposed on a strange world".

## Two hands, artifacts, discipline

The builder writes the transfer body and emits raw artifacts only:
world constructions, per-chunk evidence (chunk ordinal, byte span,
per-arm priced bits, per-arm L_t trace), alignment tables per
realignment, all SHA-256-pinned in manifests. An independent
verifier, written by a different model from this file alone,
reconstructs worlds from the frozen seeds, re-derives units (and
proves A/cipher(A) isomorphism), reprices every chunk for every arm,
recomputes every G_N and every gate, and its output is the results
artifact. The builder's totals are sanity, never results. No
coherence claims exist anywhere in this court.

Frozen constants: chunk 1024 · warmup 16384 · top-M 256 · z-gate 2.0
· L_min 0.01 · L_max 0.5 · eta 0.05 · margins 0.01*N bits · horizons
{1024, 4096, 16384, 65536} · cipher seed 0xC1F3E5 · Markov seed
0x5EED01 · shuffle seed 0x54AFF1E · 16 words swapped by rank pairs ·
descriptors: the 12 scalars above · BPE tie-break: first appearance.
