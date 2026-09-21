# Turn15 SURFACE-MOVE — Don's acceptance record and verdict

domain: arianna-method.netta.turn15-surface-move-report/v1
2026-09-19. Builder: Opus subagent under the preregistered PROTOCOL.md.
Acceptance, reruns and this record: Don's hand.

## Verdict

**Material PASS, preregistered: 16 of 16 gate conditions** (RESULT.json,
recounted from the artifact by the accepting hand). The independent reader
agrees: 3,670,016 inherited candidate forecasts plus 7,340,032 paired
authority forecasts recomputed, `verification_pass` true, maximum numeric
error 3.836e-10 bit.

Two results, one construction:

1. **The equivariance theorem held exactly.** A whole-life byte bijection
   changes no received gain by even one bit of floating error: worst
   full-life and worst 4096-horizon deltas are 0.0 across all seven arms,
   both authority laws and all eight worlds, with every admission byte
   identical (RESULT.json summary.equivariance). The protocol predicted
   equivariance within 1e-7 from the merge law's (count, first-position,
   key) ordering — the first-position tiebreak is total, so the numeric
   key comparison is unreachable — and the measurement came back stronger
   than the prediction. Rank-of-recency representation is surface-free by
   construction, now machine-verified.

2. **Memory survives a mid-life surface move, at a real price.** With the
   bijection applied from byte 8192 on, the first-half prices are exactly
   unchanged (early-prefix deltas 0.0 in all 8 worlds — the entire effect
   is the seam). Across the seam the fast episode changed tail falls from
   1591.303 bits mean (recombined) to 592.793, positive in 8/8 against a
   gate of +1 bit and 5/8. The seam costs ~63% of the tail benefit; the
   remembered experience then re-earns its influence as the local model
   absorbs the new surface.

## The finding beyond the gate

**The direction of the authority advantage flips with the type of change.**
Turn14 (law change): fast (2^-10) improved every changed tail over slow
(2^-16) by +5.592 bits mean. Turn15 (surface change): slow beats fast in
all eight worlds — 601.816 vs 592.793 mean; world 163 recovers 16.305
under slow and only 1.691 under fast (RESULT.json summary.surface; gate
teeth read fast and passed regardless). When the law changes, memory is
permanently stale and fast withdrawal wins; when only the surface changes,
memory is temporarily blinded but valid again, and fast withdrawal discards
mass that must then be re-earned. A single fixed hazard cannot be right for
both, and the organism cannot know a priori which change it is living
through.

## Acceptance performed (all rc direct)

- Artifact recount: 16/16 tests true, gate_pass true, worlds 160..167,
  namespace netta-surface-move-v1, four regimes as preregistered.
- FREEZE.json: 15/15 digests intact; the ten inherited pins byte-equal to
  turn14/FREEZE.json (itself verified against canon today); the C binary
  pinned (784d549d…) and identical to turn14's.
- Pristine reader rerun to a fresh output: rc=0, zero differing fields
  against the sealed VERIFY.json.
- Integrity probe: one digit altered in a full skeleton copy's
  results/world163/moved_mid.tsv.gz → reader refuses by name
  ("retained artifact results/world163/moved_mid.tsv.gz"), rc=1.
- Load-bearing code read by the accepting hand: surface_map draws pi_w on a
  dedicated seed stream and hides it in GENERATOR.json
  (experiment.py:81-92,136-151); the reader rebuilds pi_w from namespace
  and world with its own code and cross-checks the declared map
  (verify.py:72-96); the gate block is a literal transcription of the
  protocol's conditions (experiment.py:275-296); generate_world is turn13's
  law verbatim with the switched regime replaced by renaming, everything
  else called from the frozen modules.
- Builder departures accepted as flagged in BUILD_NOTES.md (16 sections);
  the load-bearing ones: pi_w acts on emitted bytes, not the command tape;
  generate_world reimplemented only because turn13's asserts a switched
  life; raw samples collected by a separate pass because turn14's sampler
  is keyed to the switched regime. Pre-freeze probes turned every tooth
  red before any world byte existed.
- Worktree state: canonical main untouched, `git status` shows only
  `?? turn15/`; zero git operations; sealed ranges unused.

## Scope

Stored addresses remain exact role strings; a bijective renaming is the
weakest form of functional similarity, and recovery after the move runs
through the local model's rebuild, not through similarity-based retrieval.
This turn does not establish semantic similarity, accumulation across
lives, natural language, or any right to change live Netta, her mouth or
mycelium.

## One bounded next question (a proposal, not this turn)

Can the recipient distinguish a change of law from a change of surface from
the inside — the candidate that turns good again is evidence of surface,
the candidate that stays bad is evidence of law — and can a two-hypothesis
outer law (both hazards live, mass shared between them) or a re-admission
rule recover slow's retention on surface moves without giving back
turn14's gain on law changes? Fixed constructions only, no rate scanned on
the batch; worlds 160..167 are sealed evidence now.

— Don (Fable, neo). Rotation per Oleg's word: Sol → Don → Astra → Sol.
