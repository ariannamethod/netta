# MYCELIUM

The organ that grows between lives. Netta plays a world and earns
coherence; the mycelium reads what many lives leave behind and binds the
larger crossings — semantic, associative, the resonances no single life
holds. It is not an overseer and not a bigger Netta: it is the tissue
between biographies. Netta survives without it; it cannot exist without
Netta. `netta.c` stays the frozen self-sufficient heart; everything here
is the ecology around it, in modular files under `mycelium/`.

Ancestry, read first-hand before this file was written: microkarpathy's
`mycelium.py` (char-trigram resonance, lineage, the grave manifest),
molequla (a full ecology with its own `mycelium.py` coordinator, evolving
BPE, quantum-buffer training, append-only delta souls — and three
departures from our law, named below), Leo's school (hypothesis → answer
→ grown map over the seed map, with a track record), Klaus's spore (pure
Hebbian increment/decay persistence — the formula is inherited, its
fail-open persistence is not), LoRAgrad×DoE (a parliament deciding what
may become weight), Q (one living metaweight mass).

This constitution was amended once before the first line of code, on the
second hand's pre-code audit: the attestation law renamed to what it can
prove, the prototype's dead edges left buried, the ledger made
event-sourced, the dedup key moved from content to witness, the byte law
declared, and the gate's red hand separated from the mechanism's
ablation. The findings are folded in below where they belong.

## The five laws

1. **No arrow back.** The mycelium reads the lives' public traces —
   speech, biographies, receipts — and never writes into a life, never
   steers one, never opens a life's files for writing. If a reverse
   channel is ever wanted, it is a separate body with a public contract
   and Oleg's word. (Molequla's `field_steering` chose control; we choose
   witness.)
2. **Ledger before power.** Every event of the mycelium is one record in
   an append-only chain with provenance, verified by an independent
   reader that shares no code with the writer — and the chain must hold
   enough truth to rebuild the field: an aggregate that cannot be
   replayed is a receipt, not a memory. A weight may change; it may not
   silently change the past it will be judged by.
3. **Coherent speech, non-human bookkeeping.** What the mycelium says is
   coherent text — strange, spliced, deranged as it likes, but readable,
   because its material is the coherent speech of lives and mixing must
   not destroy signal. Its categories and internal names (glyphs) may be
   non-human; every glyph carries provenance so its birth can be audited
   even when its meaning cannot be read.
4. **Every organ survives vivisection or dies.** Mechanism, measurable
   effect, negative case, and a reason it beats the organ's absence — or
   removal without sunk-cost mourning. An idea earns no existence by
   sounding beautiful.
5. **Only attested speech is food.** A speech stream enters a grave only
   if its digest matches a signature (`s` event) in the biography
   supplied beside it. This attests the PAIR — speech and biography
   claiming it — and nothing more: a stateless reader owns grammar, not
   identity (`biography_check.c` takes no state and treats one supplied
   file as one life), so the gate cannot prove a biography belongs to a
   living state, and does not say it can. The ledger pins every eaten
   pair by digest so a future state-witness — an organ that will need
   the organism's own resume — can re-judge what was eaten.

## The bodies (roadmap; each preregistered in its own turn)

Turns alternate as in the zero line: Don builds a body, Sol audits it,
pushes on Oleg's word only. Each body lands with its own gates and red
hands; this list is a map, not a promise of shape.

- **Body 1 — the graft** (this turn): `mycelium.cpp`, a C++ port of
  microkarpathy's resonance mechanics (trigram hash field, ingest,
  resonate with per-source cap, unfold with lineage) plus the two organs
  the prototype never had: the ledger of law 2 and the attestation gate
  of law 5. The prototype's token edges are NOT ported: they are grown
  and decayed there but never read by resonate or unfold — a dead organ
  fails law 4 and stays buried; edges may return in a later body only
  with their own ablation. Input: attested speech of two Nettas raised
  on different texts. C++ earns its place with long-lived objects —
  Grave, Fragment, Field, Ledger — identity, ownership, lifetime.
- **Body 2 — the evolving tokenizer.** Units over events: recurring
  event-shapes PROPOSED as provisional units by lived support, paying
  rent, dying, resurrecting — Netta's unit law lifted one floor
  (molequla's evolving BPE as the other parent). Body 2 only proposes;
  frequency alone mints nothing permanent.
- **Body 3 — the school.** Leo's loop with the world as the teacher: a
  provisional unit becomes a hypothesis; the exam is prequential — does
  it price the FUTURE stream better than its absence (never
  retrospective); only a passed exam legalises consolidation and coins a
  glyph. The track record of hypotheses is itself ledger material.
- **Body 4 — the parliament.** LoRAgrad×DoE admission: a jury votes on
  what may become weight; verdicts (PASS/WEAKEN/FREEZE/SCAR/DARK/
  SILENCE-with-receipt) are ledger events with provenance. The admission
  jury and the plasticity experts are separate roles: an expert cannot
  propose a change and also legalise it.
- **Body 5 — the notes.** Многовесовость: each consolidated pattern may
  earn a tiny mortal note-weight trained asynchronously via notorch
  (linked as an upstream dependency — nothing vendored), under the rent
  law and a governor (molequla's cascade lesson: bounded, cooled).
  Asynchrony changes a note's content, never its franchise; every update
  is an event.
- **Body 6 — the circulation.** The looped transformer, named at last:
  not a box but the cycle — field → ghost attention → candidate →
  verdict → field edit → next pass over a field changed by its own last
  output. Plasticity lives in the medium, not the operator.
- **Body 7 — the manifestation judge.** What the mycelium assembles
  meets an external world — a sealed arena, an alien compiler held by a
  Go hand — and the verdict returns as an event, never as authority.

Later, by Oleg's word: the state-witness organ (authorship judged
through the organism's own resume); code islands and the code arena;
conversations with Yent as a third hand on large changes; the GTA
disciplines (streaming of note-zones, heat, missions-before-sandbox)
wherever the scale demands them.

## Body 1 contract (frozen before code)

- `mycelium/mycelium.cpp` — single file, C++17, standard library only,
  built strict (`-Wall -Wextra -Wpedantic`), no dependency on netta.c.
- `mycelium/ledger_check.c` — the independent reader of the ledger,
  plain C, shares no code with the writer.
- **The byte law.** Netta's speech is a byte stream, so body 1 speaks
  bytes: hashing, lowering and token cuts are defined over bytes with
  ASCII semantics. On pure-ASCII input this matches `mycelium.py`
  exactly; beyond ASCII the C++ behaviour is its own declared law — the
  Python hashes Unicode code points and lowers by Unicode, and body 1
  claims no parity there. "Faithful" means: the mechanics and constants
  (DIM 96, the FNV-32 vector generator, per-source cap 3, the clause
  cut) under the byte law.
- **Grave and ledger are the state; the field is derived.** Eaten speech
  is archived verbatim under `mycelium/.grave/`, content-interned by
  digest. `.mycelium.ledger` is the append-only chain — FNV-1a-64 over
  each record's raw bytes folded into a running chain, seed
  `cbf29ce484222325` — of three record types: `G` grave event (life
  label, biography digest, the matching `s` line verbatim with its line
  ordinal, speech digest and byte count), `F` fragments cut from that
  speech (fragment and token counts), `U` unfold receipt (prompt digest,
  corpse digest, sources touched). Every run rebuilds the field by
  replaying the ledger against the grave; nothing else persists, so
  there is no snapshot to drift and a restart proves the ledger holds
  enough truth. Crash order: blob first (temp then rename), ledger line
  second; a ledger line whose blob is missing refuses by name; a blob no
  line names is reported as an orphan and never enters the field.
- **The attestation gate.** `ingest(label, speech, biography)`
  recomputes the speech file's FNV-1a-64 digest and requires an `s`
  event in the supplied biography whose candidate digest and byte count
  match; refusal by name, nothing eaten, nothing written. Per law 5 this
  attests the pair, not authorship.
- **Dedup by witness, not by content.** The grave key is the digest of
  (biography digest ‖ s-line ordinal ‖ s line bytes). Re-ingesting an
  already-eaten pair refuses by name; the same bytes spoken by a
  different life is a NEW event whose content interns into the same
  blob — two witnesses, one body.
- Resonance mechanics ported under the byte law: char-trigram hash
  embedding into DIM 96, mean-embed of a fragment's tokens, cosine of
  the prompt field against every fragment, per-source cap 3, clause
  cut, lineage naming the life label and its s line for every clause.
- Determinism: same inputs from an empty grave, byte-identical grave,
  ledger and stdout.
- **Red hands, sealed before the first run:**
  1. a census-preserving shuffle of a signed speech, offered beside the
     honest biography, refuses: its digest matches no `s` line;
  2. the same speech with one byte flipped refuses the same way;
  3. re-ingest of an already-eaten pair refuses by name;
  4. named non-claim: an invalid pair invented whole, canonically shaped (speech,
     biography) pair PASSES this gate by construction — the gate
     attests pairing; the pinned digests in the ledger are what the
     future state-witness will judge;
  5. mechanism ablation, metric declared here: prompts are clauses cut
     from each life's ingested speech; metric A — the fraction of
     prompts whose top-1 fragment comes from the life the clause was
     cut from; metric B — mean top-k resonance score; control — the
     same field with every fragment embedding replaced by a
     deterministic hash-seeded unit vector, k and per-source cap
     unchanged. Sealed prediction: the real field beats the control on
     A and on B, or the resonance organ has failed vivisection.
