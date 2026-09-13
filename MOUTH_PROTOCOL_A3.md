# NETTA BODY 1 — MOUTH PROTOCOL, AMENDMENT 3: AN EXIT MUST EXIT

domain: arianna-method.netta.body1-mouth-contract-amendment/v3

Written after the independent replay of candidate `11a7d7e` and before
the code it binds.  The replay reproduced Amendment 2 exactly: the plain
mouth passed at orders 4 and 3 with K=1; live citizens failed three of five
streams at order 4; the shuffled null failed seed 101 at order 3.  The
reader accepted every emitted token and every corridor count.  The frozen
0.50 anti-copy line does not move.

## The missed invariant

Amendment 2 says that, after K single-continuation steps, the mouth descends
to a support with a real branch in order to escape the corridor.  Candidate
`11a7d7e` descends, but leaves the corridor's sole continuation in the
lower support.  Sampling may therefore choose that same continuation and
reset the counter even though the mouth has not exited.  A change of level
is not by itself a change of path.

## The exit law

When Amendment 2 clause 3 fires, let T be the sole continuation in the
highest support H.  The mouth descends exactly as before to the nearest
lower lived support containing at least two continuation types.  For this
one choice only, T is closed.  The mouth samples among the remaining lived
continuations, then resets the corridor counter.  Citizen advice is applied
after T is closed and cannot reopen it.

If no lower support contains an alternative to T, no exit is claimed: the
mouth takes T from H and increments the counter, exactly as Amendment 2
already requires.  Closing T never licenses an unobserved token.  Every
admitted alternative remains a lived continuation at the level recorded
for the choice.

## Both hands show the closed door

The trace gains `corridor_veto`: `-` when no exit is taken, otherwise the
exact token id T closed for that choice.  `support_types`,
`support_occurrences`, and `chosen_occurrences` describe the admitted
support after T is removed.  The independent reader reconstructs H,
identifies its unique T, rebuilds the lower support, removes T by its own
hand, and refuses a trace whose level, veto, admitted support, or counter
differs.

The ear prices the emitted token inside that admitted lived support.  The
speech court, citizens book, citizen weight, triple, seed set, and all
thresholds remain unchanged.  This amendment repairs the corridor law; it
does not tune the ruler.

— Sol (Arianna Method, Codex), 2026-09-13
