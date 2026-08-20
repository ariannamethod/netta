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
Hebbian increment/decay persistence), LoRAgrad×DoE (a parliament deciding
what may become weight), Q (one living metaweight mass).

## The five laws

1. **No arrow back.** The mycelium reads the lives' public traces —
   speech, biographies, receipts — and never writes into a life, never
   steers one, never opens a life's files for writing. If a reverse
   channel is ever wanted, it is a separate body with a public contract
   and Oleg's word. (Molequla's `field_steering` chose control; we choose
   witness.)
2. **Ledger before power.** Every event of the mycelium — a grave opened,
   a speech eaten, an edge born or dead, a corpse spoken — is one line in
   an append-only chain with provenance, verified by an independent
   reader that shares no code with the writer. A weight may change; it
   may not silently change the past it will be judged by.
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
5. **Only signed speech is food.** A speech stream enters a grave only if
   its digest matches a signature (`s` event) in the life's own
   biography. The mycelium eats what a life has claimed as its own act —
   nothing anonymous, nothing tampered.

## The bodies (roadmap; each preregistered in its own turn)

Turns alternate as in the zero line: Don builds a body, Sol audits it,
pushes on Oleg's word only. Each body lands with its own gates and red
hands; this list is a map, not a promise of shape.

- **Body 1 — the graft** (this turn): `mycelium.cpp`, a C++ port of
  microkarpathy's resonance mechanics (trigram hash field, ingest,
  resonate with per-source cap, unfold with lineage) plus the two organs
  the prototype never had: the ledger of law 2 and the signed-speech gate
  of law 5. Input: signed speech of two Nettas raised on different texts.
  C++ earns its place with long-lived objects — Grave, Fragment, Field,
  Ledger — identity, ownership, lifetime.
- **Body 2 — the evolving tokenizer.** Units over events: recurring
  event-shapes promoted into named tokens by lived support, paying rent,
  dying, resurrecting — Netta's unit law lifted one floor (molequla's
  evolving BPE as the other parent).
- **Body 3 — the school.** Leo's loop with the world as the teacher: a
  recurring pattern becomes a hypothesis; the exam is prequential — does
  it price the FUTURE stream better than its absence (never
  retrospective); passing coins a glyph and consolidates. The track
  record of hypotheses is itself ledger material.
- **Body 4 — the parliament.** LoRAgrad×DoE admission: experts vote on
  what may become weight; verdicts (PASS/WEAKEN/FREEZE/SCAR/DARK/
  SILENCE-with-receipt) are ledger events with provenance.
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

Later, by Oleg's word: code islands and the code arena; conversations
with Yent as a third hand on large changes; the GTA disciplines
(streaming of note-zones, heat, missions-before-sandbox) wherever the
scale demands them.

## Body 1 contract (frozen before code)

- `mycelium/mycelium.cpp` — single file, C++17, standard library only,
  built strict (`-Wall -Wextra -Wpedantic`), no dependency on netta.c.
- `mycelium/ledger_check.c` — the independent reader of the ledger,
  plain C, shares no code with the writer.
- The ledger `.mycelium.ledger`: newline-sealed records, FNV-1a-64
  chain over raw record bytes (seed `cbf29ce484222325`), record types:
  `G` grave opened (life name, speech digest, matching s-line copied
  verbatim), `F` fragments eaten (source, count, edges added), `U`
  unfold (prompt digest, corpse digest, sources touched), `L` lint
  (edges decayed, orphans), `X` edge death. Chain and count printed on
  every run and checked by the reader.
- Signed-speech gate: `ingest(speech, biography)` recomputes the speech
  file's FNV digest and requires an `s` event with that exact candidate
  digest in the supplied biography; refusal by name, nothing eaten.
  Re-eating an already-eaten digest refuses: the grave remembers its
  dead.
- Resonance mechanics faithful to `mycelium.py`: char-trigram hash
  embedding (DIM 96), cosine resonance over all fragments, per-source
  cap, clause cut, lineage naming life and s-line for every clause.
- Determinism: same inputs, byte-identical field, ledger, and stdout.
- Red hand, prediction sealed here before the first run: a
  census-preserving shuffle of one life's speech, signed by nothing,
  refuses at the gate; the same shuffle force-fed through a forged
  biography line must fail the biography's own reader — the gate cannot
  be argued with, only fed.
