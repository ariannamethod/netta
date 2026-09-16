# NETTALOG 0

The log of the rebuilt NETTA. Numbers live in artifacts and in the
independent verifier's output, never retyped here by hand. Every
artifact is cited by SHA-256.

Current operational entries are newest first; older historical entries below
retain their original order.

## 2026-09-16 — Sol: local cold option tested, changed-tail gate failed

Based on Astra's published BANK2-ROW hand `672ffc1`, Sol precommitted the
one-step protocol as `ddae0f4`, generated fresh worlds 40..47, and tested a
per-HEAD-row P0/A/B posterior inside the unchanged joint admission and HMM.
The independent reader confirmed the sealed result and its negative verdict.
Local choice helps the unchanged mosaic but fails the preregistered benefit
after a new law begins. The two archived numerical/checker repairs and a
scratch replay are described in the report. This work remains an isolated
branch; no live Netta, mouth or mycelium file was integrated.

- `turn4/PROTOCOL.md`:
  `de4e2d9025e22e9bc1d2a4c2eb78f954ca930b23d07a8b76cd805ad8b67325bd`
- `turn4/REPORT.md`:
  `b0829bbb689d82d19fec03ed9e3703175fa23fae8cfc17cfb9a88936a5ea3a76`
- `turn4/RESULT.json`:
  `9718a1ddee6c56d9fabc304ac34158bdbe1cfcf588b548cdcf27122185a1e881`
- `turn4/VERIFY.json`:
  `cfb5333952ba0859362474283eca1b157f1c6bc43a36befce22fb46564a1cb51`
- `turn4/VERDICT.json`:
  `0022fc336093a0339f7cac9ed7ac280ec7e2c498775c1c1387dba0a17ef62fc5`

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

Addendum (same day, Claude-Desktop, on record before any transfer
implementation; Body 0 untouched) — five preregistered requirements
for the future transfer court:

1. **Renaming invariance.** The transferable unit profile must be
   invariant to unit renaming: degree, transition entropy, neighbour
   probability distribution, left/right asymmetry, distance profile,
   motif counts, possibly an iterative graph-role signature. What
   travels is the node's PLACE in the graph, never an adjacency row
   in the old coordinate system.
2. **Permutation-equivariant unit learner.** The synthetic cipher
   world must remain isomorphic after unit construction. Body 0's
   lexicographic-ID tie-break would break this under alphabet
   permutation; the transfer body's learner must tie-break by first
   position of pair appearance (or equivalent), so that A and its
   ciphered twin grow identical structure by construction.
3. **Oracle-alignment arm.** In the cipher world the true permutation
   is known to the court (never to the organism):
   newborn / traveller+learned-alignment /
   traveller+shuffled-alignment / traveller+oracle-alignment. If even
   the oracle traveller does not beat the newborn, the carried memory
   itself is useless; if oracle wins and the learned matcher does
   not, the memory transfers but the role-recognizer is weak. Two
   failure modes, cleanly separated.
4. **Early-adaptation ruler.** Primary transfer measure is
   G_N = Σ_{t=1..N} (loss_newborn,t − loss_traveller,t) — cumulative
   prospective log-loss savings over the FIRST N bytes of the new
   world, horizons frozen in advance (e.g. 1K/4K/16K/64K or one
   cumulative AUC) — not the final bits/byte after everyone has
   lived long enough for the effect to vanish.
5. **False-friend control.** A world where an identical surface unit
   deliberately occupies a different relational role. A system that
   sees matching bytes and blindly transfers authority must LOSE.
   An anchor may only say: possibly the same object — verify.

Design stops here until the independent Body 0 verdict.

## 2026-08-24 — the independent verdict (second hand)

The judge (a separate model) wrote `netta_check.c`
`ea90b9ef0bae655a0654bf92a5a290a863a667becd5322a67f696de3379cf8e0`
(31132 B) from PROTOCOL.md alone, never opening `netta.c`. It
reproduced the train segmentation byte-identically on both worlds,
recomputed every probability with zero mismatches against the
builder's evidence (tolerance 1e-9), passed every Σ=1 check, verified
all four manifests, and confirmed both machine gates (A′
test-independence, β=0 ablation identity). During its own
construction it found and fixed a diagonal double-count bug in its
OWN first-draft field builder; the builder's artifacts stood exact
throughout.

Verdict under the frozen rules — `body0/verdict.md`
`6799b72613247d99fc3de92a151b058327d98836492a6b6742dd68eb65af8a09`,
numbers in `body0/results_A.tsv`
`5803efd8fb05b151653f583869b7c2f617ced4c97858fcf40257becba8d7620b`
and `body0/results_B.tsv`
`7888d53778f6c5fab03c0bd35c763eb239ed2de21d99aaa36d283f1244cfd4b8`:

- **The field gate FAILS.** Arm c beats arms b and e in direction on
  both worlds, but by ~0.0012 bits/byte — eight times short of the
  frozen 0.01 material margin. Per the frozen rule the field is
  deleted from Body 0. Deletion of the code is a separate act on
  Oleg's word; the experiment record stays frozen as it ran.
- **Unit model beats byte-trigram baseline** on both worlds
  (architectural result, no causal claim).
- **The word-level baseline beats arm c on both worlds** — stated
  plainly, as frozen: it is the ruler of the scale NETTA's own units
  have not yet reached.
- Anti-copy: all twenty speech files under the void threshold (the
  closest, one World A file, at 0.497 coverage against the 0.50
  line — named here so nobody calls it comfortable).
- No coherence claim exists anywhere in Body 0.

## 2026-08-24 — the transfer court sat (builder hand; verdict awaits the second hand)

Court law: `TRANSFER_PROTOCOL.md`
`230030d71a9b591c5e6909e25c103b1b8c24fe8769675d275e2ccfd1baa1dd72`
(12085 B). Builder: `transfer.c` at commit 5c816b4. Artifacts in
`transfer0/`, pinned by `transfer0/MANIFEST.tsv`
`b972296daef302b16fb728dd0e30a44d96a491ebe25f0a79dea20a577482a61c`.

Construction gates, machine-checked in the builder's hands: the
ghost's raw-byte invariants landed inside both frozen bounds
(`ghost_invariants.tsv`); the cipher isomorphism is exact — all 2048
merges of A-train correspond to the merges of its ciphered twin
under the true permutation (`isomorphism.tsv`).

The court ran all three worlds to the end of their streams. No
past-carrying arm — including the oracle — earned live authority at
any point: every shadow ledger went deep negative (the carried
priors price the truth far worse than the local model from the first
chunks), so under the frozen shadow law every arm's live pricing
remained bit-identical to cold and every G_N is exactly zero. The
evidence and ledger traces are in `transfer_evidence_{iso,ghost,ff}.tsv`.

Two facts the second hand should weigh. First, the shadow law did
exactly what it was written to do: the ledgers show the priors would
have interfered massively, and the organism's live loss never
suffered — authority-at-zero protected the court from its own past.
Second, the ghost's consumed-representation statistics (unit-level
bigram MI in the builder log) are far above the raw-byte bound: the
unit representation reintroduces correlation over an i.i.d. byte
stream. The raw-level invariants were frozen with thresholds; the
consumed-level threshold was not, and this is recorded as an open
point for the verifier, not smoothed over.

Under the frozen wording, the builder's sanity reading — NOT a
verdict — is "transfer not detected: carried memory insufficient",
since even the oracle failed to earn. The independent verifier owns
the verdict.

## 2026-08-25 → 2026-09-12 — the fourth court, and the collegium that would not be pleased

This entry records a statistical-coherence experiment over public
texts; every "court" below is a preregistered measurement with sealed
receipts, and nothing else.

The narrowing came the hard way. The first transfer court annulled
itself on its own construction defects, machine-confirmed. The second
proved the cargo travels — an oracle carrying true structure prices
the twin world far below ignorance — while forced full matching is
net-negative: one wrong hard pair poisons context faster than a right
one helps. Recognition earned its map; assignment earned nothing. The
third court's map-epoch law turned out to be permanent amnesia by
construction. What survived the narrowing is small and sharp: exact
relations, earned one at a time, each carrying its own context and
its own separately earned right to advise. The fourth court was
preregistered around that micro-organ.

Its development verdict came from a blind verifier — a separate
model, writing `transfer4_check.c` from the frozen laws alone,
never opening the builder — and it stood unchanged through four
passes: **"microscopic relation earned, transfer-at-scale not
reached."** That wording is the boundary, and it is still the
boundary today.

Then the first one-shot pick died before any world existed. The base
was attested, the class was drawn — and the frozen verifier turned
out to have no real confirmatory door: the interface had been
legislated for the builder and only imagined for the second hand.
The pick was not repaired in place. It was superseded in the open —
no world built, no byte of the base spent, the failed receipts
retained as historical evidence (`COURT4_DRAW1_SUPERSESSION.md`).

The rebuilt door then went under a collegium that would not be
pleased. Twelve passes of one blind reviewer rebuilt the confirmatory
door until judge and builder agreed to the comma: an evidence reader
that refused header-only witnesses, a manifest contract checked
field by field against the builder's own receipts, a probability
floor that had silently underflowed to infinity, a revocation branch
executed on a virgin path. Then fresh reviewers of other families
took over. One fresh-generation blind reviewer was cut off
mid-review by a platform-side content filter — its scratch survived,
and both of its preserved traces were later reproduced
independently. A reviewer of a second family returned GO on
byte-identity and the GO was not accepted, because the preserved
traces pointed inside the frozen builder. A third, fully blind,
returned NO-GO with four blockers: the reader rejected any untouched
output of the real production builder over row order; receipts were
validated from mixed snapshots; two output paths trusted
caller-supplied directories; and the frozen builder itself could
emit non-finite prices on eligible bases. After repair-1 a fifth
judge, fresh and blind, returned NO-GO again with two more: an
oracle-admission divergence between writer and readers, reachable in
all four classes, and a frozen-root role that no hand actually
checked against the parent seal. Repair-2 answered both without
touching a frozen byte — the development builder restored
byte-identical to its pinned identity, every repair moved into an
additive confirmatory core, all three hands pinning the parent seal
literally. The sixth judgment returned GO.

Six independent judgments, four model families, two repairs, and not
one line accepted on anyone's word. The count of findings is the
whole point: the mechanism that finally ran is the one nobody could
break anymore, including its authors.

## 2026-09-13 — the one-shot night: eighteen roles, one pick, one run, C8

The procedural chain ran stage by stage, each stage sealed by the
building hand and independently recomputed by the second hand before
the next was allowed to exist: a replacement record for the dead
first pick; `COURT4_CONFIRMATORY_ROOTS2.tsv` — eighteen roles, order
matched against the literal arrays of selector, builder and verifier
(`9b21de267935713d4d4b84d7281aca6968e1071b5878c558b99b58668b3bc3bc`);
a fresh strict build of all three executables with sealed binary
identities and empty build logs
(`COURT4_DRAW2_EXECUTABLE_IDENTITY.tsv`,
`dc09125ba1777df037ee483f1ac77253395969e896c3ee2082523da172aa32e0`);
`BASE_COMMIT2.tsv`, byte-identical to the retained first commitment
of the same attested base
(`acf118b62a9505e5c4f3066239b97710ea5dce8d15cf3faf9d950abd49b06f31`);
`COURT4_BASE_COMMIT_FREEZE2.tsv`
(`5de9b2f356353e3a22500dc3196f5f35f2f6d4bb32eb6d36f6d71c235e1287da`);
a fresh pre-pick guard — twenty-one pins, the false-friend capacity
bound 151020 ≤ 2·151191+16 = 302398 recomputed by the reviewing hand,
zero synthetic receipts on a recursive scan, every future output
path absent (`COURT4_ALICE_PREDRAW2_GUARD2.tsv`,
`e38499a579be97adedd536cd6ce96b161e7eca66f9bff624fa9f095619362d43`);
the operator's one-shot authorization with its condition and its
literal terms sealed (`COURT4_SELECTION2_AUTHORIZATION.tsv`); and
`SELECTION2.tsv` — the single authorized pick, its class digest,
class index, seed digest and both seeds recomputed independently
from the law's byte preimages by both hands
(`69c3ee3d0e8e4d0b68f3f975edbba1245c95cb0b19599227a7f526996264dacc`).
The class fell **ff** — the false-friend world, the control built so
that a system which sees matching bytes and blindly transfers
authority must LOSE. World seed `31867eeb74db1076`.

Then the single authorized builder run, and the single authorized
verifier run. Nothing was retried, redrawn, or repaired after the
fact.

C8, from the pinned independent verifier, direct status 0, stderr
empty, stdout of 533 bytes
(`d29f25e20ef77e60755791728e5e1427c4f19ba1e06c3f6426f6dfbc7dbcfefe`)
carrying exactly one literal verdict line:

**`CONFIRMATORY PASS: microscopic relation replicated in selected class`**

Observations, byte-equal between the verifier's stdout and the
sealed record: class ff, index 3; contextual EARN present, earliest
raw offset 25754; p_rank = 0.0500 and p_rank_64K = 0.0500; no null
hand reached the live hand, 0/0; CONF_MICRO = 1;
CONF_MICRO_PREFIX = 1; G_rel([65536,81920)) = 188.705531;
G_rel([0,16384)) = 0.000000 — the organ engages late and says so
honestly, exactly where the development ruler predicted it would.
Sealed records: `COURT4_DRAW2_BUILDER_OUTPUT_RECEIPT.tsv`
(`5281e130d35ef677fad7893fd96bee626a51e3fae9ad4aff5b70ea19299a5678`,
4379 B) and `COURT4_DRAW2_C8_VERDICT_RECORD.tsv`
(`b528c3e2efe1a2dd38b892e18f67e2db98338b61e617c1b581f119686fc76b1e`,
5398 B, twenty-five pins, every one mechanically rechecked). The
full procedural receipts live in the sealed repair workspace, bound
by these hashes; this log records the arc and the verdict.

The boundary, stated as frozen: this is a microscopic relation,
replicated once, in one drawn class, on one unseen base. Transfer at
scale is not reached, and the development verdict is not rewritten.
What changed is smaller and larger at once: experience earned in one
world has now been read in another, under seals that nobody could
bend — including us. The line that opened this arc was "transfer not
detected". The line that closes it was printed by a verifier no hand
could steer. The mycelium — organisms whose biographies are readable
to each other — has its first sentence of shared language.

## 2026-09-13 — Body 1, the mouth: from parrot to the first sitting PASS in one day

The mouth arc ran turn by turn, two hands, all in one day, under
`MOUTH_PROTOCOL.md` (contract before code) and the preregistered
`SPEECH_COURT.md`. The first candidate spoke fluently and was a parrot:
the independent reader's first measurement returned an honest SPEECH
FAIL — every stream beat ignorance, every stream crossed the frozen
0.50 anti-copy line at coverage 0.9657–1.0, one verbatim tape run of
957 bytes, ear price a parrot's near-zero 0.024–0.039 bits/byte.  The
counter-audit also repaired the first candidate's citizens law: an
authenticated book (SHA-256 pinned in the binary), Court 4's
single-winner rule instead of pooling, and a token trace behind every
spoken byte (`MOUTH_PROTOCOL_A1.md`).

Amendment 2 named the disease: a lived support holding exactly one
continuation is a step along the tape, not a choice; after K such
steps the mouth must descend to a support with a real branch
(`MOUTH_PROTOCOL_A2.md`).  Amendment 3 closed the hole the second hand
found in that law: a descent could re-pick the same sole continuation
and falsely reset the counter — an exit must exit, so the corridor's
token is closed for that one choice and the trace names it
(`MOUTH_PROTOCOL_A3.md`).  Every repair was replayed independently by
a reader that shares no code with the mouth, rebuilds the unit
inventory with its own hash, rescans the lived stream for every
choice, and embeds every judged stream verbatim in its report.

The first canonical sitting is recorded in
`speech_court/SITTING1.md`: dials pinned at corridor K=1, order 4;
plain, live-citizens, and shuffled-null mouths all passed; 15/15
streams under the reader's literal verdict line — "SPEECH PASS: the
mouth speaks below ignorance and above copying" — with coverage
0.033–0.350 against the untouched 0.50 line, longest match 32–50
bytes, and an ear price of 0.250–0.348 bits/byte against ignorance at
2.28–2.70: speech that costs what choosing costs.  One stream from the
sitting, verbatim (plain mouth, seed 42):

    Netta carries the wonder of all minds meeting — two hundred and
    fifty kilometers per year, forty million years to show signs of
    rain all others are answered simply. It protects against
    infection. It says: the wait, and the planet itself is waiting
    for the rains to meet — the vibrations, send chemical reaction in
    our body carries a musearching.

The boundary of this sitting: below ignorance and above copying is
proven; semantic wholeness is not, and the word coherence stays with
the court.  The Court-4 citizens audibly change the voice (live
differs from plain and from shuffled across the sitting); what their
earned advice is worth on an unseen world is the next question, and
it gets its own contract before any code.

## 2026-09-13 — Sol / Mila: what crossed

I am adding this after the verifier, not as another verdict. The verdict
belongs to the executable and is already quoted above. This is the account of
the hand that kept the run closed while the door was rebuilt.

For most of this arc my useful word was **NO**. No, a writer's receipt was not
evidence that a reader could consume it. No, matching output on familiar
fixtures did not excuse a reachable disagreement in another class. No, a
frozen root was not pinned merely because three programs repeated its name.
No, the first draw could not be repaired after selection. Those refusals felt
like delay only while we were inside them. Seen from C8, they were the shape of
the result: the relation crossed because every easier way to claim that it had
crossed was removed first.

What crossed was not Alice's vocabulary, not an adjacency table, and not the
authority of the old world. A microscopic relation learned in A arrived in an
unseen B as a candidate; B admitted it only after local evidence earned that
right. The drawn class was `ff`, the false-friend control, where identical
surface bytes were designed to mislead a system that confuses resemblance with
identity. That is why this PASS matters. The mechanism did not win by
recognising its own handwriting. It waited, distinguished role from surface,
and engaged late. Its silence before the evidence is part of the success.

This does not turn one relation into transfer at scale. The frozen development
boundary remains exact. We have one replicated organ in one drawn class on one
unseen base. But before this night, "experience can move without exporting
authority" was an architecture we had reason to want. Now it has one measured
instance. That is enough to change what the next system is allowed to assume,
and nowhere near enough to let it stop measuring.

The six judges and two repairs were not ceremony around the experiment. They
changed the object that was eventually tested. The final builder and verifier
agreed because their contracts had survived disagreement about row order,
snapshot identity, path authority, finite prices, oracle admission, and parent
roots. Trust did not come from unanimity. It came from retaining each fracture
until the program could carry its answer through it.

After closure, the procedural mass was not discarded or pushed as repository
debris. The canonical checkout's 2,123 untracked artifacts were moved with
their relative paths into the ignored local archive
`.netta-archive/court4-closure-20260913/`; complete copies of both repair
workspaces and the closure checkpoint live beside them. The tracked, portable
history remains this log and its content-addressed seals.

Oleg said that if this path failed, we would keep building until experience
began to transfer. It did not fail. At 04:09, after weeks of mechanisms and a
night of single-use gates, the independent line was finally on disk. I still
do not think the deepest result is the word PASS. It is the new primitive it
licenses us to build with: a biography may be readable elsewhere without
becoming law there. It arrives as memory, waits as a hypothesis, and earns a
voice from the life in front of it.

That is where Court 4 ends. The mycelium begins with one organism able to say:
I can hear what happened to you without pretending it happened to me.

## 2026-09-16 — Astra: separate histories on partly familiar rows

This entry belongs to the isolated `astra/turn3-memory` hand based on merged
`37e9b68c439a4891ca507fc4155c58bd94d64a2e`. Canonical Netta, Court4, mouth and
mycelium were not edited. The incoming Sol HMM-16 is reproduced and audited
in `turn3/AUDIT.md`; no production repair was required. A separate reader
now checks every prefix of the previously final-only static comparison.

One subsequent step preserves two source books and selects their advice
locally per exact HEAD256 pattern, inside the same outer admission/HMM.
The fixed gate and independent reader report PASS on the new synthetic batch.
All numerical results, small threshold margin and harmful tails are retained
in the artifacts below. This does not certify semantic similarity or a
cumulative many-life learning curve. No live integration, commit or push.

Artifacts (SHA-256):

- [turn3/PROTOCOL.md](turn3/PROTOCOL.md): `2a97c72e40f8a8a3f7c2c7d45733808015aa601118a8bb043860130cf075df87`
- [turn3/CODE_FREEZE.json](turn3/CODE_FREEZE.json): `9327c293890ae6baff5679fcbc99174af473c3663210cde5a6478ba579dac0b5`
- [turn3/RESULT.json](turn3/RESULT.json): `603b96c7369bb259808f9a2173287b009809313601533e05da4dd3af1499cc29`
- [turn3/VERIFY.json](turn3/VERIFY.json): `e4802237c81a72834d87f0de2e85bf348be4f9b3518ccaf4a554abf5bdb351d2`
- [turn3/VERDICT.json](turn3/VERDICT.json): `bc4048a113c580ca072a18fb5e0e44ac879e8b52ca954f9814f71f600441e2c8`
- [turn3/ARTIFACT_MANIFESTS.json](turn3/ARTIFACT_MANIFESTS.json): `cc7c83d32abb18e70647fb8c883ba514892dd91a98386bada2fe816ba1a26a2f`
- [turn3/REPORT.md](turn3/REPORT.md): `2a2827120e0ba658a32fbc5b69ca88517ff0bf3794755e07acba696ab1abdf38`
- [turn3/RAW.md](turn3/RAW.md): `218fd56372a3c7e89fbfed2ce656640c68574a1774056ad488dea04cfbd87b82`

Hand returned to Sol for her audit and one bounded next step. The concrete
open restriction is a row where both available books are wrong: its inner
mixture has no third P0 choice, while outer withdrawal applies to the bank
as a whole. No further experiment was started after this gate passed.
