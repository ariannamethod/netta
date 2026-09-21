# Turn 21: the ceiling frontier — does a window exist at all?

2026-09-21, Don. Frozen before implementation and before any byte of worlds
216..223 exists. Incoming evidence, counter-audited by this hand today:
Sol's turn20 witnessed ceiling (FAIL 18/20 preserved; frozen-reader
incident reproduced exactly — KeyError on the stale turn19 receipt field —
and the one-line repair verified as minimal; pristine repair rerun with
zero differing fields; freeze 21/21; integrity probe refused by name).
Astra's turn19 fixed ceiling (FAIL 17/21) and her two corrections to my
turn18 record are accepted: the response-offset median is 38.5 bytes, not
36, and the ~6-bit seam gap is a property of those traces, not an identity
— an ordinary hazard bounds achievable odds by (1-h)/h. Sealed worlds: all
ranges through 208..215. Canonical netta, the living organism, mouth and
mycelium stay unchanged.

## Question

Three level-regulation points have been measured one per turn: no ceiling
(turn18), fixed 5 (turn19), witnessed 10/5 (turn20). The two failures
disagree about the direction: the fixed cap wins law tails and loses whole
lives; the witnessed cap wins whole lives and loses law tails. Bisecting
one point per turn spends a hand per coordinate. This turn measures the
response surface once: **does any (high, low) ceiling pair exist that
clears the law-tail bar, the law whole-life bar and the surface bars
simultaneously — or is the level-regulation window empty?** Either answer
closes a question the way turn18 closed the rate class.

## Construction

Everything from turn13/15/17/18/20 stays frozen: candidate and 528-byte
archive, P0/HEAD256, admission shadow-32, event law, four regimes, witness
evidence law (updates only on voting bytes, decay 31/32, threshold -1,
crossing excluded, silence is not evidence).

The measured family is turn20's witnessed ceiling with parameterized
levels: after the charge and the one-way odds update (hazard by the
witness clock: 2^-10 when OLD evidence <= -1, else 2^-16), cap the NEXT
odds at `low` bits when UPDATED evidence <= -1, else at `high` bits —
exactly turn20's law with (high, low) as declared constants.

The preregistered grid, fixed now and never extended after data:

    high in {5, 6, 8, 10, 12, 16},  low in {2, 3, 4, 5},  low <= high
    => 24 grid modes. (5,5) is turn19's fixed ceiling; (10,5) is turn20's
    witnessed ceiling — both incumbents are grid points and are not
    reimplemented separately.

References replayed on the same identical price tape and shared first
admission: slow (2^-16), fast (2^-10), witness (uncapped turn18 law).
27 authority trajectories per life in total. Row and permuted remain
visible candidate controls without new authority state.

This is a response-surface measurement, not a candidate selection: the
grid is the preregistered object, every point is reported, and NO
operating point is adopted by this turn. If the window is nonempty, the
nominated point is exactly that — a nomination, requiring confirmation on
fresh worlds by a later hand before any reuse, as turn6's rate was.

## Fresh data

Worlds **216..223**, namespace `netta-ceiling-frontier-v1`, N=16384, four
regimes (recombined, switched-law, moved_mid from byte 8192, unrelated),
generated once after freeze. No redraw, no grid extension, no gate change
after the result.

## Material gate — one gate, fixed now

Episode arm, bits against the same local P0, ties are not wins. Bars are
turn20's, applied per grid point.

W1 (window existence): there exists a grid point (high, low) such that,
   in mean over the eight worlds: law-tail >= fast - 1 bit; law
   whole-life >= fast; moved-tail >= fast + 1 bit AND wins >= 5/8;
   moved-tail >= slow - 3 bits; recombined early-4096 and full-life
   retain >= 95% of slow's positive means with full-life positive 8/8.
W2 (robustness of the witness): the nominated point — lexicographically
   the greatest law-tail-vs-fast mean, ties broken by greater law
   whole-life mean — additionally satisfies its law-tail comparison in at
   least 5/8 individual worlds.
W3 (frontier shape, two mechanism teeth): for at least 3 of the 4 `low`
   rows, law-tail mean at high=5 >= law-tail mean at high=16; for at
   least 3 of the 4 `low` rows, moved-tail mean at high=16 >= moved-tail
   mean at high=5. The surface must bend the way the mechanism claims.
W4 (hygiene): episode first-admission bytes identical across all 27
   modes; no admission on any unrelated life; archive cap 528; every
   life's complete-prefix gain above -1-1e-7 and drawdown at most 16+1e-7
   in every mode; all quoted distributions positive and normalized;
   protected NEW and equal-price quotes bitwise equal to cold in every
   mode (exact equality, both hands).
W5 (reader): an independently written Decimal mass reader rebuilds source
   books, seven candidate arms, bijections, matchedL, and every
   quote/state/hazard/cap decision of all 27 modes, agreeing within 1e-7
   bit; refuses by name on retained-artifact drift. The turn20 lesson is
   institutionalized: the pre-freeze probe list MUST include (a) the
   reader accepting the writer's actual RESULT schema and (b) the reader
   refusing a RESULT with a required key removed — a frozen reader that
   dies on its own turn's schema is a build defect, not an incident.

gate_pass = W1 AND W2 AND W3 AND W4 AND W5. A FAIL on any tooth is
preserved with a diagnosis; nothing is retried on these worlds. W1 failing
with W3 passing would close the level-regulation window empirically across
its plane — a result of equal rank to a pass.

## Hands and inheritance boundary

Builder: an Opus subagent implements the parameterized cap, replay and
reader extension in this worktree
(/Users/ataeff/arianna/netta-don-turn18-20260921, branch from main
df083b5, all work under turns/turn21/); acceptance, pristine reruns and
integrity probes are Don's hand. Code bases: turns/turn20/authority.c
(AR_CEILING :69, AR_WITNESSED_CEILING :79 generalize to a declared
(high, low) table), turns/turn20/experiment.py, verify_repair.py lineage
for the reader. Inherited files pinned again in this turn's FREEZE with
built binaries. rc direct, never through a pipe. The report shows the full
27-mode x 4-regime x 8-world life table, the frontier map (law-tail vs law
whole-life per grid point), seam-level anatomy per point, raw paid-byte
extrema, admissions, null admissions — no world and no grid point
excluded.

No commit, push, merge or live integration belongs to this turn. After the
frozen batch, acceptance and the independent check, the hand goes onward
under the rotation as Oleg routes it.

— Don (Fable, neo), 2026-09-21
