# NETTA BODY 0 — PROTOCOL FREEZE (2026-08-24)

Frozen before the first line of measured code. Any change after the
first measured run is a new experiment, named aloud.

## Worlds (pinned)

- World A: `netta.txt`, 447545 bytes,
  SHA-256 `02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb`.
- World B: `~/arianna-datasets/miller/combined.clean.txt`, 1301310 bytes,
  SHA-256 `76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2`.
- World C: a third blind corpus chosen by Oleg or Mila AFTER code
  freeze, never by the builder.

## Split

train = first floor(0.9·len) bytes, test = the remainder. The cut is
made by byte offset before any counting; merges, counts and the field
see train only; test is segmented with train merges.

Test-independence gate: world A′ = first 90% of A + the last 10%
REVERSED byte-wise. The model-table artifact hash of a run on A and a
run on A′ must be byte-identical — machine proof the test suffix never
entered the model.

## Units (exact BPE law)

Initial alphabet: 256 byte units, id = byte value. Counting stream =
train bytes, one contiguous stream. Pair count = number of adjacent
index pairs (i, i+1) in the current token stream, overlaps counted as
they stand. Replacement = left-to-right greedy non-overlapping.
Selection = maximum count; tie-break = lexicographically smallest
(left_id, right_id). Stop after 2048 merges or when max count < 4,
whichever first. New unit id = 256 + merge ordinal. Segmentation of
ANY text (including test) = replay the merges in creation order, each
applied left-to-right greedy non-overlapping. Whitespace may merge —
no exceptions. These are LEARNED units; the word "earned" is not
granted in Body 0.

## Field (symmetric, frozen)

H is defined over unordered pairs: for train positions i < j with
j − i ≤ 8, H_raw({a,b}) accumulates 1/(1+(j−i)) where {a,b} =
{tok[i], tok[j]}; H = H_raw / max(H_raw). Symmetric by definition:
H(a,b) = H(b,a).

Field factor: F(u | past) = Π_{j=1..min(4,t)} (1 + β·H(u, p_{t−j})),
β = 0.3. "Past" = already-emitted units (generation) or the true
preceding TEST units (teacher-forced pricing).

Dataflow: N₁, N₂, N₃ and H are built once from train bytes and are
read-only afterwards; F reads only (u, past); no term reads the
current truth or any future byte.

## Probability law (exact, sums to 1)

Candidate sets from train counts: C₃(c) = {u : N₃(u₁,u₂,u) > 0},
C₂(c) = {u : N₂(u₂,u) > 0}, C₁ = {u : N₁(u) > 0}. Level weights
w_k(u) = N_k(u|c) · F(u|past); in no-field arms F ≡ 1. The field
applies at EVERY level. V = size of the unit inventory.

- P₁(u) = (1−ε)·w₁(u)/Σ_{C₁} w₁ + ε·(1/V)
- P₂(u) = C₂ nonempty ? (1−ε)·w₂(u)/Σ_{C₂} w₂ + ε·P₁(u) : P₁(u)
- P(u|c) = C₃ nonempty ? (1−ε)·w₃(u)/Σ_{C₃} w₃ + ε·P₂(u) : P₂(u)

ε = 0.1. Each level is normalized and the mixture is convex, so
Σ_u P(u|c) = 1 exactly; the verifier checks Σ = 1 within 1e-6 at
every position.

## Boundary (cold)

The test stream starts cold: position 1 is priced by P₁ with an empty
field past; position 2 by the P₂ law with a 1-unit context; from
position 3 the full law. The field past contains test-side units
only; no train unit crosses the boundary.

## Arms (equal budget, trigram order, same ε and laws)

- (a) raw byte trigram, no field;
- (b) learned-unit trigram, no field;
- (c) learned-unit trigram + field;
- (e) learned-unit trigram + PERMUTED field: F_perm uses
  H(π(u), π(p)) with the frozen bijection π(id) = V−1−id — destroys
  the unit↔field correspondence while preserving the value
  distribution and the whole pipeline;
- (d) word-level trigram sanity baseline: a word = maximal run of
  [A-Za-z'], every other byte its own token; an unseen test word pays
  1/256 per byte (frozen escape); no field.

## Ruler

Held-out bits per byte = Σ −log₂ P(truth) ÷ test bytes,
teacher-forced. This measures predictive compression and nothing
else. NO coherence claim exists in Body 0: free-running speech is a
qualitative artifact; the word "coherent" requires a separate frozen
speech court with an external evaluator.

## Generation (qualitative artifact + anti-copy)

Sampling law (same in all speaking arms): S′(u) = ln w(u, active
level) − ln(1 + 0.5·f₁₂(u)) where f₁₂ = frequency of u among the last
12 emitted units; top-15 by S′; P ∝ exp(S′/T), T = 0.8. Seeds frozen:
{7, 19, 42, 101, 271}; all five reported, no selection. Speech is
written as raw .bin; the builder reports hashes and exit codes only.

Anti-copy (verifier-computed): the longest train substring match of
the output in bytes, and the fraction of output bytes covered by
train matches ≥ 32 bytes. Coverage > 50% = the mouth is a copier and
all generative claims are void.

## Two hands

`netta.c` (builder) emits raw artifacts: the merge list, the train
token stream, per-position evidence (byte span, truth id, per-arm
P(truth)), speech .bin files. `netta_check.c` (verifier, sharing no
scoring code) re-derives segmentation from the merge list, recounts
all tables, recomputes every probability and every Σ=1 check, totals
bits/byte, and computes anti-copy and telemetry. THE VERIFIER'S
OUTPUT IS THE RESULTS ARTIFACT. All artifacts are pinned by SHA-256 +
exact byte length. NETTALOG0.md cites artifacts by hash and copies no
numbers by hand.

## Ablation identity

`--no-field` sets F ≡ 1 and changes nothing else. Machine proof: an
arm-c run with β = 0 must be bit-identical (cmp) to the arm-b run on
the same seed — the flag ablates exactly the field term.

## PASS / FAIL (frozen before the first measured run)

- The field earns its place iff arm c beats BOTH arm b and arm e on
  held-out bits/byte on BOTH worlds A and B. Otherwise the field is
  deleted.
- Arm b vs arm a is reported as "unit model beats byte-trigram
  baseline" — an architectural result, not a causal attribution to
  units. A causal claim would need a byte-context-matched control.
- If arm d beats arm c, the record states plainly: the word-level
  baseline is better.
- distinct3 / lived4-support are telemetry, never PASS criteria.

Frozen constants: merges 2048 · MIN_PAIR 4 · window 8 · K 4 · β 0.3 ·
ε 0.1 · T 0.8 · top-15 · repetition 0.5 over window 12 · split 90/10 ·
seeds {7,19,42,101,271}.

## Amendments (frozen before code, same day, pre-first-run)

1. **Separation of hands.** The builder head writes ONLY `netta.c` and
   the raw-evidence specification below. `netta_check.c` is written by
   a different model, independently, from this frozen PROTOCOL.md —
   one head must not implement both the accused and the judge.
2. **Status ceiling.** Body 0 can grant the field at most the status
   PREDICTIVELY ADMITTED. It does not become a surviving organ until a
   separate, pre-frozen speech court shows a useful causal effect on
   the mouth (invariant 4).
3. **Blind world C.** Chosen only AFTER `netta.c` is frozen and
   hashed. No change to code, coefficients or protocol after C is
   seen.
4. **Permutation control.** π is a deterministic Fisher–Yates shuffle
   of [0, V) with the frozen xorshift64 seed `0xC0FFEE` (same xorshift
   law as the organism: x^=x<<13; x^=x>>7; x^=x<<17; iterate from
   i = V−1 down to 1, j = next() mod (i+1), swap). Reversal is
   rejected: BPE ids carry order-of-birth structure.
5. **Material margin.** The field gate requires arm c to beat BOTH
   arm b and arm e by at least **0.01 bits/byte on each world**
   (A and B separately). A smaller victory is not an effect; the field
   is then deleted from Body 0 regardless of sign.

## Raw evidence specification (builder output; the verifier's input)

All artifacts land in `body0/<world>/`, every file listed in
`MANIFEST.tsv` as `path \t bytes \t sha256`. The builder prints only
hashes and exit codes; every number in the record comes from the
independent verifier.

- `merges.tsv` — one line per merge, creation order:
  `ordinal \t left_id \t right_id`.
- `train_tokens.u32` — final train token stream, little-endian u32.
  The verifier re-derives segmentation from train bytes + merges.tsv
  and must reproduce this stream byte-identically before trusting any
  of it.
- `evidence_<arm>.tsv`, arm ∈ {a,b,c,e,d} — one line per test
  position of that arm's own tokenization:
  `ordinal \t byte_offset \t byte_len \t truth_id \t level \t P`
  where `level` ∈ {3,2,1,0} is the deepest supported level used
  (0 = escape floor) and `P` is P(truth) printed as `%.17g`.
  Arms a and d carry their own position grids (byte and word).
- `speech_<arm>_<seed>.bin` — raw emitted bytes, one file per frozen
  seed, arms c (full) and b (no-field).
- The verifier recomputes every P from scratch, checks Σ P = 1 within
  1e-6 per position, checks its P against the builder's within 1e-9
  relative, totals bits/byte itself, and computes anti-copy and
  telemetry from the raw .bin files. The verifier's output is the
  results artifact.
