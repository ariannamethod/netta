# HMM-16: one bounded fresh byte experiment

2026-09-16, Sol. Fixed before generating worlds 24–31 or reading their targets.
The earlier HEAD256-v1 result and its eight changed-tail failures remain unchanged.
This is an isolated branch, not an edit to the live Netta, mouth, or mycelium.

## One change

Keep Astra's HEAD256-v1 frontend, 7032-byte NETHD256 source book, local P0,
source P2, support=32 and gate=32. Add an optional absorbing source-to-cold
posterior transition after each *active* raw-byte observation. The hazard is
fixed at h=2^-16 per byte; it is not tuned from target outcomes. On the gate
crossing byte set next-event odds to 1 without a transition. The static mode
continues to use its old posterior update. Both modes see exactly the same
prefix and admission event.

## Fresh data

Use the unchanged synthetic generator law and seed namespace from Astra's
phase-2 experiment, but previously unused world IDs 24–31. Each world has
four independent 16384-byte source trajectories, one preserved-law target,
one unrelated-law target, and one target changing law after byte 8192. Target
and source byte names are independently permuted. Preserve the common first
8192 bytes of preserved and changed-tail targets. Run static and HMM on each
identical raw target; also run the same local P0. No redraw, no parameter or
capacity sweep, no cherry-picking. This family favors recurrence and is not
natural language or evidence for functional similarity across diverse laws.

## Predeclared checks

1. Strict C11 build, frontend selftest and recurrence API tests pass. Pre-truth
   all-256 distributions are positive and normalized within 1e-8. A fresh
   independent Python forward recursion from the static trace agrees with HMM
   event prices, odds and gains within 1e-8 bits.
2. Across every target and every prefix, HMM gain against P0 >= -1-1e-7 bits;
   maximum drawdown <= 16+1e-7 bits. For T active events, HMM cumulative loss
   minus the static mixture's <= (T-1)*[-log2(1-h)]+1e-7 bits.
3. Report all eight worlds, all three regimes, preserved gain, changed-tail
   gain and harm, cold/source weights before and after the change, admission,
   worst prefix and drawdown. Distinguish theorem checks from measured utility.

The material transfer check retains the previous threshold: preserved mean
gain >0.01 bit/raw byte and positive in all eight worlds. This is not a
claim that HMM improves that already-positive gain; its intended new result
is a bounded cost of obsolete advice. Unrelated arms should not admit; any
exceptions are reported, not filtered. No claim of fast source recovery after
a second change. A pass ends this single experiment.
