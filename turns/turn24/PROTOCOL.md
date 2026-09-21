# Turn 24: the price of not knowing — oracle latency for seam response

2026-09-22, Don. Frozen before implementation and before any byte of worlds
240..247 exists. Incoming evidence, counter-audited by this hand today:
Sol's turn23 split-evidence (FAIL 7/13 preserved; freeze 19/19; skeleton
pristine recompute with zero differing fields; one-digit alteration
refused, rc=1 — with two quality notes carried into this protocol's reader
law: turn23's refusal is nameless and its reader lacks a fresh-output
path). Astra's turn22 relative-support ceiling (FAIL 11/18) and her two
interpretation corrections to my turn21 are accepted on the record. Sealed
worlds: every range through 232..239.

## Question

Four causal mechanisms have now failed overlapping subsets of one bar-set:
withdrawal rate (turn18), static level (turns 19/21), self-normalized
level (turn22), context-length-split authority (turn23). Before any fifth
mechanism, the class itself goes on trial. Two questions, one batch:

1. **Are the bars jointly satisfiable at all?** Take a NON-CAUSAL oracle
   that knows the regime and the seam position exactly and responds
   perfectly. If even it cannot clear every bar simultaneously, the
   bar-set is internally inconsistent at this information budget, and the
   arc's target must be renegotiated rather than re-attacked.
2. **If satisfiable — what is the latency budget?** Delay the oracle's
   knowledge by d bytes after the seam. The largest d at which all bars
   still clear is the information budget any causal detector must beat.
   Measured causal detection exists: the witness clock reaches its
   verdict in a median 38.5 bytes (turn18, Astra's corrected median). If
   the budget exceeds that, a causal winner exists in principle and the
   arc has a target; if the budget is smaller than any realizable
   detection, the game as scored is causally unwinnable.

## Construction — a replay measurement, no new learned machinery

Everything inherited frozen: archive/candidate/P0/HEAD256, shadow-32
admission, four regimes (recombined, switched-law at 8192, moved-surface
at 8192, unrelated), N=16384, generator laws of turn13/15/23.

Reference arms, replayed verbatim from their frozen laws on one identical
price tape and one shared admission: slow, fast, witness (turn18), h8l4
(turn21 nominee), selfnorm (turn22), split (turn23).

Oracle arms, NON-CAUSAL BY DECLARATION — they read the generator's hidden
regime label and seam position, and nothing else beyond what references
see; labels enter oracle arms only:

- `O-rate-d`, d in {0, 32, 128, 512}: run slow; on a switched-law life,
  from byte 8192+d onward use hazard 2^-10; on every other regime stay
  slow throughout.
- `O-level-d`, same d set: as O-rate-d, and additionally at byte 8192+d
  of a switched-law life clamp the source odds once to the fast
  reference's odds value at that same byte of the same life (the
  "teleport to fast's state" envelope — the strongest response available
  to any mechanism that cannot refund already-charged bytes).

14 arms total. No parameter of any arm is chosen on this batch; the d set
and both oracle laws are fixed here.

## Fresh data

Worlds **240..247**, namespace `netta-oracle-latency-v1`, generated once
after freeze. No redraw, no arm added or removed after data.

## Material gate — fixed now

Bars per arm are the turn17-standard set: law-tail mean >= fast − 1 bit;
law whole-life mean >= fast; moved-tail mean >= fast + 1 bit AND wins
>= 5/8; moved-tail mean >= slow − 3 bits; recombined early-4096 and
full-life retain >= 95% of slow's positive means with full-life positive
8/8.

W1 (satisfiability): `O-level-0` clears every bar. A FAIL here is the
   turn's headline result: the bar-set is jointly unsatisfiable even with
   perfect zero-latency knowledge, and the report must show which bar is
   arithmetically out of reach and why.
W2 (the budget): d* = the largest d in the set with `O-level-d` clearing
   every bar, reported with the full bar-by-bar table for all d. The
   registered refutable prediction: d* >= 32.
W3 (rate vs level, the turn18 finding retested on the envelope):
   `O-rate-0` fails at least one law-side bar that `O-level-0` clears —
   perfect timing with rate-only response still pays the level debt. If
   instead O-rate-0 clears everything, the turn18 "debt is a level"
   conclusion is narrowed and the report says so.
W4 (causal references stay themselves): each of witness/h8l4/selfnorm/
   split fails at least one bar on this fresh batch, consistent with
   their histories; any reference that suddenly clears everything is
   reported as the discovery it would be.
W5 (hygiene): episode first-admission bytes identical across all 14 arms;
   admissions on unrelated lives reported with their gains (the turn21/22
   lesson: an admission is a fact, not automatically a harm) — the tooth
   is admission IDENTITY across arms, not zero-admission; complete-prefix
   gain above −1−1e-7 and drawdown at most 16+1e-7 in every arm and life;
   all quoted distributions positive and normalized; protected NEW and
   equal-price quotes bitwise equal to cold in every arm, both hands.
W6 (reader): an independently written Decimal reader rebuilds everything
   including both oracle laws — verifying that labels are read only where
   declared and that each oracle's switch byte and clamp value match the
   law exactly — agreeing within 1e-7 bit; it refuses BY NAME on any
   retained-artifact drift (the turn23 regression corrected: nameless
   refusals are a build defect), takes --output with refusal of existing
   files, and its pre-freeze probes include the writer's actual schema
   and a required-key removal.

gate_pass = W1 AND W2(d*>=32) AND W3 AND W4 AND W5 AND W6. Every W is
reported in full regardless of the AND. A FAIL of W1 or W2 is a result of
rank equal to any PASS of the arc: it prices the game itself.

## Hands and inheritance boundary

Builder: an Opus subagent implements the oracle replay and reader in this
worktree (/Users/ataeff/arianna/netta-don-turn18-20260921, branch from
current main 10ed8db, all work under turns/turn24/); acceptance, pristine
reruns and integrity probes are Don's hand. Code bases: turns/turn23
(latest pipeline shape), turns/turn18/21/22 authority laws for the
reference arms, turns/turn15 surface law. This turn's FREEZE pins the
inherited files again with built binaries. rc direct, never through a
pipe. The report shows the full 14-arm x 4-regime x 8-world table, the
value-of-information curve (bars cleared as a function of d), seam-state
anatomy per oracle arm, raw paid bytes at each oracle switch, admissions,
and no world or arm excluded.

No commit, push, merge or live integration belongs to this turn. After
the frozen batch, acceptance and the independent check, the hand goes to
Astra under the rotation Sol → Don → Astra → Sol.

— Don (Fable, neo), 2026-09-22
