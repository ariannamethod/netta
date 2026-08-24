# NETTALOG 0

The log of the rebuilt NETTA. Numbers live in artifacts and in the
independent verifier's output, never retyped here by hand. Every
artifact is cited by SHA-256.

## 2026-08-24 — Body 0 built (builder hand only; no verdicts here)

Protocol frozen before code: `PROTOCOL.md`
`1dc3f48a06b620cf9797785d260285513e45befd51960dcb8b4b4b371311a480` (9010 B).
Invariants: `INVARIANTS.md`
`3791e3a5a19473a6aa25441b524e09828ee81e0d0f711cc3e31b42cf5b7d991a` (339 B).

Worlds: A = `netta.txt`
`02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb` (447545 B);
B = miller `combined.clean.txt`
`76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2` (1301310 B).
World C: blind, to be chosen by Oleg/Mila now that the builder is frozen.

Builder runs (artifacts + per-file hashes in each `MANIFEST.tsv`):

- `body0/A/MANIFEST.tsv`
  `03c0d2e7889c1a40fbac3c28baf9392d6bad4d887aeb98e7fa5407d857cf3fdc`
- `body0/Aprime/MANIFEST.tsv`
  `06083f0474901ea21e46bbb4190273d04db2bcf462a69793e52c19cb2637ddcf`
- `body0/A_beta0/MANIFEST.tsv`
  `bbc0a2892cb8c57a20f455499baa3995aa1ceda33c107cfeb3c1277337a6dbb5`
- `body0/B/MANIFEST.tsv`
  `e9b548e3be16e948214d1c401cdc92cb9636e0c1279dac7c13b91e4f1ce8f6d9`

Machine gates passed in the builder's hands (cmp/shasum, this session):

- A′ test-independence: `merges.tsv` and `train_tokens.u32` of the A
  and A′ runs are hash-identical — the test suffix never entered the
  model.
- β=0 equivalence: with `--beta 0`, arm c evidence and all five
  speech files are bit-identical to arm b, and arm b is unaffected by
  β — the ablation flag touches exactly the field term.

Status: evidence emitted for arms a, b, c, e, d on worlds A and B;
speech emitted for arms c and b on the five frozen seeds. The
builder's stderr carries sanity totals only; they are NOT results.
The field's status is undecided until the independent verifier
(separate hand, written from PROTOCOL.md alone) recomputes
everything. No coherence claim is made or implied; speech files are
qualitative artifacts.

## NEXT / transfer — hypothesis on record, NOT an amendment
(recorded 2026-08-24 by Oleg's word, authored by Claude-Desktop;
Body 0, its protocol, code and artifacts stay frozen; nothing here is
implemented before the independent verdict on Body 0)

If the judge confirms that the word-level baseline beats learned
units, that does not make NETTA word-level. It means the useful
statistical scale of language sits near words — and NETTA must EARN
units of that scale herself while staying grounded in raw bytes,
because a word inventory is a closed map of one city: it pays a
brutal per-byte escape on unseen words and transfers nothing across
worlds and scripts.

Three-floor ontology to keep: raw bytes -> learned lexical units ->
transferable relational experience. Bytes are the physics of a
world. Units are locally learned objects. What travels between
worlds is neither literal n-gram counts nor a gifted dictionary but
a unit's RELATIONAL PROFILE: its neighbourhood types, continuation
predictability, left/right geometry, transition entropy, distance
structure. Experience travels; the city map does not.

Law kept in a testable form: global experience != local authority.
Experience from world A enters world B only as a prior; its local
weight in B starts at zero and grows only on B's future evidence.

Pre-sketched falsifiable transfer court (frozen wording, to be
preregistered properly before any transfer body is built):
- newborn(B): never saw A;
- traveller(A->B): lived A, carries structured transferable memory;
- shuffled-traveller(A->B): same memory volume, correspondence and
  relations permuted.
On the first N bytes of world B, compare prospective held-out loss.
Transfer is earned only if the traveller beats BOTH the newborn and
the shuffled traveller stably. A traveller worse than a newborn
means the past was interference — a full result too.

The word-level baseline is not an enemy; it is the ruler showing
what scale NETTA's own units must still reach.
