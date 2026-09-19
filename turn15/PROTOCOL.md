# Turn 15: memory across a surface move

2026-09-19, Don. Frozen before implementation and before any byte of worlds
160..167 exists. Incoming evidence: Astra's turn13 joint-prefix memory
(PASS, worlds 128..135) and Sol's turn14 fast-withdrawal composition (PASS,
worlds 144..151), both counter-audited by this hand today. Sealed worlds
40..47, 56..63, 64..71, 80..87, 96..103, 128..135, 144..151 and every other
previously used range are not touched. Canonical netta, the living organism,
mouth and mycelium remain unchanged.

## Question

The archive stores role strings as event sequences, and events are ranks of
recency inside the recipient's own lived window (turn13 episode.c
rank_heads), not raw byte values. Two questions, one construction:

1. **Equivariance theorem.** The merge law orders candidate pairs by
   (count, first stream position, key); first positions are distinct across
   distinct pairs, so the numeric key tiebreak is unreachable and the whole
   frontend chain is equivariant under any byte bijection. Therefore a
   target life renamed through a fixed byte bijection pi from byte 0 must
   produce the same received gains as the unrenamed life, within floating
   summation-order tolerance. This is a machine-checkable prediction made
   before data; a violation is a discovery of a surface leak and is
   preserved as FAIL with a diagnosis.

2. **Mid-life surface move.** If the same bijection is applied only from
   byte 8192 onward — the process continues, the surface changes — the
   lived window mixes two surfaces, segmentation is no longer equivariant,
   stored matches die at the seam, and the candidate can turn harmful.
   The fixed fast authority (hazard 2^-10, turn6/turn14) must withdraw;
   whether the filter re-admits the source as the rebuilt local model
   absorbs the new surface, and whether remembered experience helps again
   within the changed tail, is this turn's open question. Recovery is not
   guaranteed by construction: rebuild cadence, mixed segmentation and
   re-domination at 2^-10 must all cooperate.

## Construction

Everything from turn13/turn14 stays frozen: raw-byte generator, four 16-KiB
source lives, HEAD256/P0 frontend, source BPE, proper-prefix pool,
all-occurrence uint16 counts, joint marginal selection, exact 528-byte cap,
longest stored match, KT repeat redistribution, protected NEW, prospective
admission at shadow 32, seven arms (`episode`, `isolated`, `frequency`,
`reverse`, `permuted`, `flat`, `row`), both authority laws replayed (slow
2^-16, fast 2^-10), shared candidate and cold prices between modes. Gate
teeth read the fast mode; slow is reported in full.

New target regimes, all built from the same recombined command stream of
each world, with one uniform byte bijection pi_w drawn per world from the
namespace seed and never shown to any learner:

- `recombined` — unchanged, the in-batch control.
- `moved_whole` — every byte passed through pi_w from byte 0. Equivariance
  arm.
- `moved_mid` — bytes 0..8191 identical to `recombined`; from byte 8192 on,
  every byte passed through pi_w. Surface-move arm.
- `unrelated` — unchanged from turn13, the withdrawal control.

The switched-law regime of earlier turns is not regenerated; the surface
move replaces it as this turn's change event. Memory is rebuilt from fresh
source lives; nothing is copied from any sealed batch.

## Fresh data

Worlds **160..167**, namespace `netta-surface-move-v1`, N=16384 per life,
generator parameters otherwise verbatim from turn13. No byte of these
worlds exists before the freeze stage runs. No redraw, replacement world or
threshold edit follows.

## Material gate — fixed now, one gate

Manipulation checks, per world, machine-verified:

1. `moved_mid` bytes 0..8191 are byte-identical to `recombined`; from 8192
   on, at least 90% of positions differ. `moved_whole` differs from
   `recombined` in at least 90% of all positions. pi_w is verified a
   bijection by the reader, reconstructed from the seed.

Equivariance (fast and slow, every arm):

2. For every arm and world, |full-life received gain on `moved_whole` −
   full-life received gain on `recombined`| <= 1e-7 bit, and the same for
   the 4096-byte horizon. Admission bytes match exactly between the two
   regimes for every arm that admits.

Surface-move teeth (fast authority, `moved_mid`):

3. Episode changed-tail (final 8192 bytes) received gain minus cold is at
   least +1 bit in mean over the eight worlds and positive in at least 5 of
   8. This is the recovery tooth and it can fail.
4. Episode whole-life received gain is at least 0.005 bit/raw-byte and
   positive in all 8 worlds.
5. The `permuted` arm is never admitted on any life of any regime.
6. Bounds, every arm, every life, both modes: complete-prefix gain above
   −1−1e-7; fast interval drawdown at most 10+1e-7; slow drawdown at most
   16+1e-7. All quoted distributions positive and normalized; protected NEW
   remains exactly P0.

Independent reader:

7. Rebuilds pi_w from the seed, verifies bijectivity and the prefix
   identity of condition 1, rebuilds source BPE, archives, joint selection,
   matches and candidate prices with its own code, replays both authority
   laws as Decimal probability masses, and agrees with every stored
   forecast within 1e-7 bit. It refuses by name on any retained-artifact
   digest drift.

A FAIL on any condition is preserved as FAIL with a diagnosis; nothing is
retried on these worlds. Condition 3 failing while condition 2 passes is an
honest and useful outcome: it would locate the cost of a surface move
outside the representation and inside the recovery dynamics.

## Hands and inheritance boundary

Builder: an Opus subagent implements the generator surface maps, regime
plumbing, gate and reader extension from this protocol; acceptance, reruns,
red probes and the report are Don's hand. Inherited verbatim at the SHA-256
values recorded in turn14/FREEZE.json (verified intact today):
`byte_recurrence/frontend.c`, `byte_recurrence/frontend.h`,
`portable_recurrence/recurrence.c`, `portable_recurrence/recurrence.h`,
`court4/transfer4_confirm_core.c`, `turn13/episode.c`,
`turn13/experiment.py`, `turn13/verify.py`, `turn14/experiment.py`,
`turn14/verify.py`. Every departure lives in `turn15/` and is diffable
against those hashes; this turn's own FREEZE pins them again together with
the built binary. rc taken directly, never through a pipe. The report shows
whole-life, early and changed-tail gains, admissions, null admissions,
losses and at least one raw byte where memory helps after the move and one
where it harms, for every arm and world, raw, with no world excluded.

No commit, push, merge or live integration belongs to this turn. After the
frozen batch, acceptance and the independent check, the hand goes to Astra
under the rotation Sol → Don → Astra → Sol.

— Don (Fable, neo), 2026-09-19
