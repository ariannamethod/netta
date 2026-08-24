# NETTA TRANSFER COURT 2 — PREREGISTRATION (2026-08-24)

Second sitting. Court 1 (protocol SHA-256 230030d7..., verdict in
git history and transfer0/) is closed: reproduced where its law was
tight, annulled as ill-posed where it was not. Its four defects and
two ambiguities are this court's first paragraphs. Any change after
the first measured run is a new experiment, named aloud.

## Standing laws (carried, unchanged)

1. The dead field of Body 0 does not resurrect under any name.
2. Global experience != local authority: the shadow/earn/revoke law
   of court 1 is carried verbatim (always priced in shadow; live
   weight exactly 0 until the ledger earns EARN = 32 bits; enter at
   L = 0.05; live clamp [0.01, 0.5], eta 0.05; revoke below 16 bits
   with hysteresis). It was verified clean by the second hand.
3. Two hands. The builder emits raw artifacts and grades nothing;
   an independent verifier written from this file alone owns every
   result. Repo hygiene: one protocol file, overwritten per court;
   one log; heavy evidence out of the tree, pinned by manifests.

## Repairs from court 1 (each closes a named defect)

- **R1 (was Д1).** No warmup wall. The map exists from byte 0 with
  frequency seeding; it is refined on the geometric chunk schedule
  {1, 2, 4, 8, 16, 32, 64, 128, 256, last}. Nothing is structurally
  zero at any horizon.
- **R2 (was Д2).** The newborn floor is honest: the unigram level's
  escape distribution is length-aware, U(u) = 256^(-len(u)) / Z with
  Z = Σ_inventory 256^(-len), and when the unigram level has no
  counts its FULL mass goes to U (no epsilon waste). An empty model
  prices a byte at exactly 8 bits. Applies to every arm including
  cold.
- **R3 (was Д3).** No candidate renormalization anywhere. Every
  prior is a full probability model over the same support as the
  local model. The position class "prior undefined" is abolished.
- **R4 (was Д4).** Destination worlds are built from an INDEPENDENT
  text the traveller has never eaten. No horizon lies inside
  anything she memorized.
- **R5 (tie-break hole).** BPE selection is a total order: highest
  pair count, then earliest first-appearance position, then smaller
  (left_id, right_id) lexicographically. Ties are impossible by
  construction; the verifier must reproduce every merge list
  byte-identically or the run is void.
- **R6 (candidate-set hole).** Dissolved by R3.

## Source, cargo, map

Source S = A-train: first 90% of `netta.txt` (447545 B, SHA-256
02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb).

**Cargo (layer 0 only, frozen).** The traveller carries the full
byte-level backoff model of S: counts of orders 0..3 (unigram,
bigram, trigram, 4-gram over raw bytes), pricing with the Body 0
chain law (epsilon 0.1 per level, R2 floor). Unit-level cargo is
NAMED for court 3 and enters only if layer 0 earns. Nothing else
travels.

**Map.** A soft doubly-stochastic 256x256 matrix M, pure counting:
initialized from frequency seeding S0[s][d] = -|log f_S(s) -
log f_D(d)| (f_D from the lived destination prefix,
Krichevsky-Trofimov smoothed), refined by relaxation labelling
S = A2·M·B2^T + A2^T·M·B2 + 0.25·S0, then M = Sinkhorn(exp(S/T), 20
iterations), where A2/B2 are the source/lived-destination bigram
transition matrices. Temperature anneals on evidence: T halves when
the mean row entropy of M stops falling (stagnation = change <
0.01 bits over one refinement), floor T = 0.25, start T = 2.0.
Refinement runs on the R1 geometric schedule.

**The prior.** P_prior(d | c) = Σ_s M[s][d] · P_S(s | c_S), where
c_S is the destination context mapped source-ward through the top-3
rows of M per context byte, weighted by their masses, and P_S is the
carried backoff chain. Full support, sums to 1 (verifier checks
within 1e-6 per position). Uniform M degrades this to ~8 bits/byte
— never worse than the honest floor.

## Destination worlds (all constructed before code, pinned by hash)

Base text D = miller `combined.clean.txt` (1301310 B, SHA-256
76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2)
— same hidden language class, never eaten by the traveller.

- **W-iso**: byte-substitution cipher of D, Fisher-Yates over
  0..255, frozen xorshift64 seed `0xB170C5`. Oracle = the
  permutation.
- **W-plain**: D as-is. Natural transfer, same alphabet; oracle =
  identity.
- **W-ghost**: i.i.d. unigram sample from D's byte distribution,
  length of D, seed `0x5EED02`. Verifier proves preserved unigram
  (L1 <= 0.01 vs D) and destroyed relations (bigram MI <= 0.01
  bits). Decisive roles: the D-gate difference and the
  non-imposition gate only.
- **W-half**: first half = cipher of D's first half under the W-iso
  permutation; second half = i.i.d. unigram continuation (seed
  `0x5EED03`). The past must work on half 1 and must not fire on
  half 2; G is measured per half separately.
- **W-ff**: D with its 16 most frequent whitespace-delimited words
  swapped in rank pairs (1st with 2nd, 3rd with 4th, ...); oracle =
  identity plus the swap table.

## Arms (equal budget; the cache/align split of court 1 is retired)

1. **cold** — no past.
2. **carrier** — full layer-0 cargo through the LEARNED map M.
3. **scrambled** — identical machinery, M's source rows permuted by
   Fisher-Yates seed `0x54AFF1E` after every refinement: same
   volume, dead correspondence. The negative control.
4. **oracle** — control, out of competition: the true map replaces
   M from byte 0; every other object and schedule identical to
   carrier (the identity law: their evidence may differ only where
   the map differs).

## Prequential law

Chunks of 1024 bytes, priced before absorption; after absorption
the local model rebuilds from the lived prefix (units for the local
model per Body 0 law with R5 tie-break; the local model itself is
the Body 0 unit chain with the R2 floor). loss(arm, t) = priced
bits of chunk t under P_final = (1 - L_t)·P_local + L_t·P_prior,
with the standing shadow law. Note the two models price the same
bytes: P_local prices unit positions, P_prior prices bytes; both
are converted to bits over the chunk's byte span before mixing at
the chunk level: P_final is applied per unit position with P_prior
evaluated as the product of its per-byte probabilities across the
unit's byte span (exact, no approximation).

## Rulers and gates (frozen)

- G_N(arm) = Σ (loss(cold) - loss(arm)) over the first N bytes,
  N in {1024, 4096, 16384, 65536}; deciding N = 16384;
  MARGIN(N) = 0.01·N bits.
- **D-gate (kept — it worked):** reported effect = G_16384(W-iso) -
  G_16384(W-ghost); must clear MARGIN.
- **Map gate:** top-1 accuracy of M vs the W-iso oracle >= 90% at
  1024 lived bytes, >= 99% at 4096. (Unicity distance for this
  cipher class is ~100 bytes; failing this kills the matcher, not
  the memory.)
- **Oracle floor:** G_1024(oracle, W-iso) >= 3000 bits. If even the
  true map with full cargo cannot beat the newborn early, the cargo
  law is wrong and the court is void — machine identity: on W-iso
  the oracle prior IS the source model on isomorphic text.
- **Ghost line:** G_16384(any past arm, W-ghost) <= 520 bits (the
  KT unigram-regret line, frozen); above it the prior imposes
  structure that is not there — non-imposition fails.
- **Half gate:** G_16384(carrier, W-half half 1) >= MARGIN and
  |G(half 2)| < MARGIN — the past works only where structure is
  shared.
- **FF gate:** G_16384(carrier, W-ff) >= 0 - MARGIN (99% of the
  world is intact; surface authority leaking shows as a loss).
- **Interference:** any past arm at G_16384 <= -MARGIN on any world
  = "past experience interferes", recorded plainly.
- **PASS wording:** "transfer earned (predictively)" iff carrier
  clears MARGIN and beats scrambled by MARGIN on W-iso AND W-plain,
  passes the D-gate, the half gate, the FF gate, and stays under
  the ghost line. Ceiling: predictively admitted; speech value
  needs its own court. No coherence claims.

## Evidence

Per chunk per arm: chunk, byte span, positions, bits, L, ledger,
state. Map snapshots (top-1 table vs oracle) at every refinement.
Worlds, oracle maps, manifests — all SHA-256 pinned. The verifier
reconstructs worlds from seeds, re-derives the cargo, the map
schedule and every price from this file alone, and owns the
verdict.

Frozen constants: chunk 1024 · horizons {1024,4096,16384,65536} ·
margins 0.01·N · EARN 32 · revoke 16 · L_enter 0.05 · clamp
[0.01,0.5] · eta 0.05 · epsilon 0.1 · cargo orders 0..3 · map
refinement schedule {1,2,4,8,16,32,64,128,256,last} · Sinkhorn 20 ·
T start 2.0, halve on stagnation < 0.01 bits, floor 0.25 · S0
weight 0.25 · top-3 context rows · cipher seed 0xB170C5 · ghost
seed 0x5EED02 · half-tail seed 0x5EED03 · scramble seed 0x54AFF1E ·
16 words by rank pairs · merges 2048 · MIN_PAIR 4 · R5 total order.

## Amendments (frozen before code, same day)

- **A1. Candidate-set fixture.** The builder ships a crafted fixture
  suite, separate from R5: hand-built tiny streams driving every
  pricing path (tri hit, tri-miss/bi hit, bi-miss/uni, empty-model
  floor, unit longer than one byte under the R2 law). For each
  fixture position the builder and the verifier must both emit the
  ORDERED candidate set and the normalized probabilities, and they
  must match exactly (1e-12). The court-1 candidate-set ambiguity is
  closed by fixture, not by prose.
- **A2. Prefix-causality gate.** Two auxiliary worlds share an
  identical prefix (first 8192 bytes of W-iso) and diverge after it
  (continuation A = W-iso's own, continuation B = ghost-law bytes,
  seed `0x5EED04`). Everything the court emits before the divergence
  point — predictions, candidate sets, map snapshots, ledgers,
  authority states, losses — must be bit-identical between the two
  runs. This machine-proves scoring causality: a chunk is priced by
  the state built from the PAST prefix only, and the chunk's truth
  enters the state only after its scoring.
- **A3. The null is derived, not observed.** The empty-model price
  follows from the frozen law: with an empty inventory the byte
  floor gives exactly 8.000000 bits per byte, and the first-chunk
  cold price on any world is the law-derived expectation for its
  actual early counts, checked by fixture within 1e-9 — never an
  empirical ceiling read off the evidence.
- **A4. Arm truth table (normative).** The four arms and the control
  differ ONLY as follows; everything not listed is bit-identical by
  law, and the verifier checks that their evidence diverges only
  where a listed object differs:

  | object | cold | carrier | scrambled | oracle |
  |---|---|---|---|---|
  | local model (units, R2 floor) | same | same | same | same |
  | cargo (byte backoff of S) | absent | present | present (same counts) | present |
  | map M | absent | learned (Sinkhorn) | learned, source rows permuted after every refinement | true map, constant |
  | prior | absent | through M | through permuted M | through true map |
  | shadow/earn/revoke | n/a | standing law | standing law | standing law |
  | refinement schedule | n/a | geometric | geometric | same clocks, refinement is a no-op |

- **A5. Map gate classification.** The map gate (top-1 >= 90% at
  1024, >= 99% at 4096 lived bytes on W-iso) is a PASS gate for the
  MATCHER claim only: failing it while the oracle floor passes
  yields "memory carries, matcher weak" and does not void the
  court. Map accuracy on all other worlds is telemetry.
- **A6. Confirmatory world.** The five worlds above are DEVELOPMENT
  worlds: repairs and re-runs against them are lawful before the
  code freeze. The verdict world is ONE additional confirmatory
  world, chosen by Oleg or Mila only AFTER v2 `transfer.c` is
  frozen and hashed; its construction law must be one of the frozen
  classes (cipher / plain / half / ff over an independent text
  named at choice time). No change to code, constants or protocol
  after the confirmatory world is seen; the final PASS wording is
  decided on the confirmatory world with the development worlds
  reported alongside.
