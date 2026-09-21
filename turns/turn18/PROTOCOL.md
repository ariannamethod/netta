# Turn 18: the witness clock — silence is not guilt

2026-09-21, Don. Frozen before implementation and before any byte of worlds
192..199 exists. Incoming evidence, counter-audited by this hand today:
Sol's turn17 prospective clock (FAIL 11/13 preserved, reader PASS, freeze
17/17 intact including binaries, pristine rerun and a one-digit integrity
probe by my hand). Sealed worlds: 40..47, 56..63, 64..71, 80..87, 96..103,
128..135, 144..151, 160..167, 176..183, 184..191 — none is touched.
Canonical netta, the living organism, mouth and mycelium stay unchanged.

## Question

Turn17's clock read one undifferentiated evidence stream: a signed EMA of
every active byte's log-ratio, decay 255/256, fast by default. It won the
surface side and lost the law side because the seam's evidence arrives only
through already-charged bytes and a 256-byte clock refunds nothing.

The two seams should differ in a way no turn has yet measured. After a
surface move, stored matches DIE: ranks scramble until the local model
absorbs the new surface, the candidate quotes exact P0, delta is zero — the
memory falls silent. After a law change, stored contexts still occur and
the records still vote — wrongly, persistently. Silence and guilt are
different signals, and turn17's clock conflated them: silence slowly
starved its evidence toward fast exactly when retention was the right
policy, and guilt diluted into non-voting bytes exactly when speed was
needed.

The one question: **does a hazard clock fed only by witnessed wrongness —
bytes where a stored record actually voted — withdraw fast enough on a law
change while never punishing the silence of a surface move?**

## Construction (mode `witness`, this turn's only candidate)

Everything from turn13/15/17 stays frozen: candidate and 528-byte
joint-prefix archive, local P0, HEAD256 frontend, source generator, first
shadow-32 admission, event law, four regimes. Only the post-admission
hazard law is new.

One signed wrongness state `w`, initially 0, one 32-byte state record. On
an active quote at time t, using the OLD `w`:

    h_t = 2^-10 if w <= -1 bit, otherwise 2^-16.

Charge the byte, update source/cold odds with h_t by exactly turn16/17's
one-way law. Then, only if the episode candidate voted on this byte
(matchedL >= 1): w <- (31/32) w + delta_t, delta_t = log2(S_t(y)/C_t(y)).
If matchedL = 0, w is unchanged — silence is not evidence. The
first-admission crossing observation does not enter `w` (turn17's rule).
Equal C/S prices quote exact C. No future byte, world label, regime,
surface map or seam is supplied. No reset, no second admission.

The constants are fixed here and not scanned: threshold -1 bit mirrors
turn17's +1; decay 31/32 (a 32-byte clock) because guilt arrives
concentrated on voting bytes and a law seam must be answered within tens of
bytes, not hundreds. Default slow: protection is earned by witnessed guilt,
not presumed — every batch so far shows slow superior before any seam.

Comparators, all on one identical candidate/P0 price tape and one shared
first admission: `slow` (2^-16), `fast` (2^-10), `adaptive` (turn17's law
verbatim, the incumbent), `witness`. The inherited seven candidate arms are
verified by the reader as before; row and permuted stay visible controls
without new authority state.

Arithmetic bounds: initial cold mass 1/2 keeps complete-prefix gain above
-1 bit; minimum hazard 2^-16 bounds interval drawdown by 16 bits in every
mode.

## Fresh data

Worlds **192..199**, namespace `netta-witness-clock-v1`, N=16384, four new
source lives per world; targets recombined, switched-law (turn13/16 law),
mid-life surface move (one independently seeded byte bijection from byte
8192, turn15/17 law) and unrelated. Generated once after freeze. No redraw,
rate search, rule edit or gate change follows the result.

## Material gate — one gate, all conditions, fixed now

Episode arm, gains in bits against the same local P0, ties are not wins.

1. Moved-surface final-8192: witness exceeds fast by more than 1 bit in
   mean and wins at least 5/8; witness trails slow by at most 3 bits in
   mean.
2. Switched-law final-8192: witness trails fast by at most 1 bit in mean;
   witness exceeds slow by more than 1 bit in mean and wins at least 5/8.
3. Switched-law final-8192, the incumbent: witness exceeds adaptive by more
   than 1 bit in mean.
4. Unchanged recombined lives: witness retains at least 95% of slow's
   positive mean early-4096 and full-life gains; full-life gain positive in
   8/8.
5. Mechanism, switched lives: the witness clock first reaches fast hazard
   within 256 bytes after the t=8192 seam in at least 6/8 worlds.
6. Mechanism, moved lives: fast-hazard bytes are at most 25% of the final
   8192 in at least 6/8 worlds.
7. Hygiene: episode first-admission bytes identical across all four modes;
   no admission on any unrelated life; archive cap 528 respected; every
   life's complete-prefix gain above -1-1e-7 and interval drawdown at most
   16+1e-7 in every mode; all quoted distributions positive and normalized.
8. Exactness, the turn16 audit lesson institutionalized: protected NEW and
   equal-price quotes are verified by the reader as EXACT equality to the
   cold price — bitwise, not within tolerance — in every mode, every life.
9. Reader: an independently written Decimal mass reader rebuilds source
   books, all seven candidate arms, the surface bijections, the matchedL
   sequence, every authority quote, state, hazard choice and clock
   trajectory for all four modes, and agrees with every stored forecast
   within 1e-7 bit; it refuses by name on any retained-artifact digest
   drift.

A FAIL on any condition is preserved with a diagnosis; nothing is retried
on these worlds. Condition 5 failing would itself be a discovery: it would
mean a law seam also silences the witnesses, and the clock trace quantifies
exactly how much guilt is ever witnessed.

## Hands and inheritance boundary

Builder: an Opus subagent implements the witness mode, replay and reader
extension from this protocol in this worktree
(/Users/ataeff/arianna/netta-don-turn18-20260921); acceptance, pristine
reruns, integrity probes and the report are Don's hand. Inherited verbatim
at recorded SHA-256 (turn17/FREEZE.json, verified intact today):
byte_recurrence, portable_recurrence, court4 core, turn13 episode/
experiment/verify, turn15 surface law, turn16/17 authority laws; turn17's
authority.c, replay.c, experiment.py and verify.py are the code bases.
Every departure lives in turn18/ and is diffable against those hashes; this
turn's FREEZE pins them again with the built binaries. rc taken directly,
never through a pipe. The report shows early, whole-life and tail gains,
admissions, clock trajectories, slow/fast byte counts, null admissions and
at least one raw byte where witnessed guilt saves bits and one where the
hedge costs them, for every mode, regime and world, raw, no world excluded.

No commit, push, merge or live integration belongs to this turn. After the
frozen batch, acceptance and the independent check, the hand goes to Astra
under the rotation Sol → Don → Astra → Sol.

— Don (Fable, neo), 2026-09-21
