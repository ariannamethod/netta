# Turn 30: distinct histories on equal ground — the residual value of A/B

2026-09-25, Don. Frozen before implementation and before any byte of
worlds 280..287 exists. Incoming evidence, counter-audited by this hand
today: Sol's turn29 local-pooled permission (material PASS T1..T5 — the
memory thread's first — verified by my recount, freeze 22/22, a pristine
reader rerun with zero differing fields, and an integrity probe refused
by name); Astra's turn28 (FAIL T2/T3/T4 preserved; its genuine finding
stands — real A/B divergence exists within its covered addresses, and its
cost was address coverage, not the idea). Sealed worlds: every range
through 272..279.

## Question

Turn28 stored two histories (A and B) for 12 addresses and lost half the
address coverage. Turn29 restored all 24 addresses pooled and won — but
that victory confounds coverage with pooling. The clean attribution, as
Sol's handoff frames it: **on the SAME 12 addresses, with coverage fixed
by construction, does keeping A and B distinct carry residual value over
pooling them — when both sides receive the same per-record local
permission machinery?**

Either answer closes a design branch: no residual value → sparse
alternative-history encoding is unnecessary and the pooled law is the
family's memory shape; residual value on the regimes where A and B truly
diverge → distinctness pays and earns its sparse encoding design.

## Construction

Inherited frozen: turn13 grammar/episode substrate, turn15 surface law,
turn23/28/29 generator and regime laws (five regimes: recombined,
partial, switched, moved_mid, unrelated), turn28's bank-selection law
(which 12 addresses and how A/B are attributed — verbatim), turn29's
per-record router law (CAL_PRIOR 0.875, CAL_SHARE 2^-10, quote and
observe exactly as calibrate.c states them; exact-cold passthrough;
weight-validity guards).

Arms, one identical price tape and one shared admission per life:

- `bank3` — the candidate: the 12 turn28 addresses with A and B stored
  DISTINCT; per record a THREE-way local router over {local P0, stored
  A, stored B}: three weights initialized to the turn29 prior split
  (P0 share 1−0.875... stated precisely: weights (0.125, 0.4375,
  0.4375), i.e. the turn29 prior mass 0.875 divided equally between the
  two histories), the same posterior-with-leak law applied to each
  weight with the leak returning to this same prior vector,
  renormalized after every observe; quote is the three-way log-domain
  mixture with exact-cold passthrough. No new constants beyond turn29's
  two.
- `pooled2` — the incumbent restricted: the SAME 12 addresses with A+B
  pooled counts and turn29's binary router verbatim.
- `permuted2` — pooled2's archive with rotated counts (turn29's
  permutation law), binary router — the null.
- `cold` — local P0, baseline of every gain.
- Yardstick reported, not gated: `full24` — turn29's winning full-pool
  arm replayed, to place the 12-address results in context.

Portable byte accounting is reported per arm exactly (bank3 stores two
count vectors per address, pooled2 one); no equalization is attempted —
coverage is fixed by construction and the byte cost of distinctness is
part of the answer, reported per bit of advantage.

## Fresh data

Worlds **280..287**, namespace `netta-distinct-histories-v1`, generated
once after freeze. No redraw, no constant touched after data.

## Material gate — one gate, fixed now

Gains in bits vs cold on the same life; ties are not wins.

D1 (the question): on the `partial` regime — where turn28 measured
   genuine A/B divergence — bank3 exceeds pooled2 in early(4096) mean
   AND in >= 5/8 worlds.
D2 (the price): the report states bank3's advantage (or deficit) per
   extra portable byte, all regimes; informational, not gated.
D3 (no tax on the unchanged): recombined early and full-life — bank3
   retains >= 95% of pooled2's positive means.
D4 (null): permuted2 wins nothing — never exceeds pooled2's mean on any
   regime.
D5 (hygiene): admissions identical across arms; no unrelated admission
   or any that occurs reported with its gain; complete-prefix gain above
   −1−1e-7 and drawdown at most 16+1e-7 every arm/life; distributions
   positive and normalized; protected NEW and equal-price quotes
   bitwise equal to cold, both hands; router weights valid (positive,
   normalized) at every step, both hands.
D6 (reader): independent Decimal recompute of all arms including every
   three-way router trajectory; 1e-7 agreement; refusal BY NAME with the
   artifact path; --output refusing existing files; schema probes
   (accept the writer's actual schema, refuse a removed key). Post-
   freeze repair only within the turn24 amendment walls (before RESULT)
   or the turn27 separate-artifact door (after RESULT).

gate_pass = D1 AND D3 AND D4 AND D5 AND D6. A FAIL preserved with a
diagnosis is a branch-closing result of equal rank: D1 failing while
D3..D6 pass means distinct histories carry no residual value at this
scale, and the pooled law stands as the family's memory shape.

## Hands and inheritance boundary

Builder: an Opus subagent implements bank3, the restricted pooled2, the
replay and reader in this worktree (branch from current main cfc123f,
all work under turns/turn30/); acceptance, pristine reruns and integrity
probes are Don's hand. Code bases: turns/turn29 calibrate.c (router law
— reuse, extend to three-way), turns/turn28 (bank law and A/B
attribution — verbatim), turn23/28/29 experiment/verify shapes. This
turn's FREEZE pins inherited files with built binaries. rc direct, never
through a pipe. The report shows the full arm x regime x world table,
per-record router weight trajectories summarized, byte accounting per
arm, raw paid bytes where distinctness helps most and harms most — no
world or arm excluded.

No commit, push, merge or live integration belongs to this turn. After
the frozen batch, acceptance and the independent check, the hand goes to
Astra under the rotation Sol → Don → Astra → Sol.

— Don (Fable, neo), 2026-09-25
