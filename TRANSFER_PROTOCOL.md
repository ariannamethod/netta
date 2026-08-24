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
- **W-ghost** — the frequency ghost, a provable structural null:
  447545 bytes sampled i.i.d. from A-train's unigram byte
  distribution, frozen seed `0x5EED01`. A Markov-1 ghost was
  rejected in preregistration repair: it preserves first-order
  transition structure — exactly the level the transfer profile
  reads — and would make a negative verdict ambiguous. The verifier
  must prove BOTH invariants of the i.i.d. ghost: preserved
  nuisance — empirical unigram distribution within L1 distance 0.01
  of A-train's; destroyed relations — empirical bigram mutual
  information <= 0.01 bits. The ghost's only decisive roles are the
  anti-smoothing difference D and the non-imposition gate; it is
  never a standalone transfer verdict.
- **W-ff** — the false-friend world: A with the 16 most frequent
  whitespace-delimited words of A-train swapped in pairs by
  frequency rank (1st with 2nd, 3rd with 4th, ...), all occurrences,
  deterministic, no seed. Identical surface units deliberately
  occupy different relational roles; the true swap map is the
  court's oracle for this world.
- **W-blind** — a natural fourth world chosen by Oleg or Mila only
  AFTER the transfer-body code is frozen and hashed; exploratory,
  not a gate.

The construction of W-iso, W-ghost and W-ff is emitted as
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
9-11. partner concentration at exact distances 1, 2, 4: the Simpson
   index Σ_v (c_d(u,v) / N_d(u))², where c_d(u,v) counts v exactly d
   positions after u and N_d(u) = Σ_v c_d(u,v) — repaired in
   preregistration before code: the original decayed-mass form was
   degenerate (near-constant for every unit) and z-scoring it would
   amplify noise; concentration is non-degenerate and remains free
   of neighbour names;
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
   word-swap map for W-ff) replaces the learned matcher. **Oracle
   identity law:** arm 6 differs from arm 3 (align) in exactly one
   object — the correspondence mapping. The warmup schedule, the
   shadow/earn/revoke law and its constants, the local evidence,
   the candidate sets, the prior's functional form and the entire
   remaining pipeline are identical; the verifier checks that the
   two arms' evidence differs only where the mapping differs. Upper
   bound only; it can never win the court, only calibrate it.

**Shadow, earn, revoke (frozen).** The original L_min = 0.01 start
was rejected in preregistration repair: any nonzero live weight
before earned evidence is authority, and the law forbids it. The
repaired law: in every past-carrying arm the prior is ALWAYS priced
in shadow — at each position both l_local,t and l_prior,t (the
-log2 prices of the truth under each component) are computed, and
the shadow ledger A_t = Σ_{τ<=t} (l_local,τ - l_prior,τ)
accumulates in bits. Live mixing
P_final(u) = (1 - L_t) * P_local(u) + L_t * P_prior(u) obeys:

- **Unearned state: L_t = 0 exactly.** P_final = P_local, and the
  arm's emitted choices are bit-identical to cold — a machine gate:
  cmp of the evidence prefix up to the first earn event.
- **Earn:** at the first t with A_t >= EARN = 32 bits, the prior
  goes live with L = L_enter = 0.05.
- **Live update:** L_{t+1} = clamp(L_t * exp(0.05 * (l_local,t -
  l_prior,t)), 0.01, 0.5).
- **Revoke:** whenever A_t < EARN/2 = 16 bits, L resets to 0 and the
  state returns to unearned; re-earning requires A_t >= EARN again
  (a 16-bit hysteresis against oscillation).

All constants frozen now; they are positions, and their revision
after a measured run is a new experiment.

**Prior mechanics, frozen before code:** (i) every prior used for
pricing carries the standard escape into the local model,
P_prior_used = 0.9·P_prior_raw + 0.1·P_local — a truth outside the
prior's support pays a large finite price instead of breaking the
ledger; (ii) the cache prior's softmax temperature is 1; (iii) at a
position where a prior is undefined (unaligned context, empty
carried continuation set), P_prior = P_local exactly, the ledger
delta is zero, and the position is flagged in the evidence.
(iv) The ghost's preserved/destroyed invariants are verified BOTH on
the raw byte stream and on the post-unit representation the court
actually consumes (unit-stream unigram/bigram statistics at the end
of the lived run) — the verifier owns both checks.

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
D = G_16384(arm, W-iso) - G_16384(arm, W-ghost); generic smoothing
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
- **Non-imposition gate:** on W-ghost, any past-carrying arm with
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
· EARN 32 bits · revoke floor 16 bits · L_enter 0.05 · live clamp
[0.01, 0.5] · eta 0.05 · margins 0.01*N bits · horizons {1024, 4096,
16384, 65536} · cipher seed 0xC1F3E5 · ghost seed 0x5EED01 · shuffle
seed 0x54AFF1E · 16 words swapped by rank pairs · descriptors: the
12 scalars above · BPE tie-break: first appearance · ghost
invariants: unigram L1 <= 0.01 preserved, bigram MI <= 0.01 bits
destroyed.
