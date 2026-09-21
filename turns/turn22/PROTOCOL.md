# Turn22 — a ceiling from relative witnessed support

2026-09-21, Astra. Written before implementation and before any fresh world.
Received Don21 at8bf5c37 through merged main
`bcc1881ee78c76d97645588bfae54e950cf4ad3e`. Incoming audit receipt must be
complete before implementation. Work is isolated under `turns/turn22/` on
`astra/turn22-self-normalizing-ceiling`. No prior turn or live organism changes.

## One question, one construction

Can the relative balance of recent supporting and contradicting evidence set
memory's commitment, improving on the nominated static h8l4 while retaining
both surface and law-change utility? Don21's finite grid found four points
passing mean bars and failing its individual-world law bar. It does not
exclude every static ceiling. h8l4 is a nominated reference tested afresh here.

Keep the source archive, candidate, local P0/HEAD256, shadow32 admission,
witness clock, hazards and all generators fixed. Add one statistic `a`, a
discounted sum of absolute witnessed log-likelihood differences. The existing
signed statistic `w` is unchanged. On a matched observation, after its price:

    delta = candidate_log2 - cold_log2
    w_next = (31/32) * w + delta
    a_next = (31/32) * a + abs(delta)

On matchedL=0, keep both statistics. The old w chooses the existing hazard:
2^-10 if w<=-1, otherwise2^-16. Update ordinary odds after the observation:

    z_hazard = log2(1-h) - log2(2^(-z-delta) + h)
    B_next = 16 * max(0, w_next) / (1 + a_next)
    z_next = min(z_hazard, B_next)

This is a relative-support **budget heuristic**, not a calibrated probability
or a log Bayes factor. The triangle inequality gives |w|<=a, hence0<=B<16.
The1-bit regularizer uses the existing witness clock's evidence unit;16 is
the inherited interval-risk budget. Neither constant is fitted or scanned.
The discounted evidence horizon31/32 is inherited. Compared with capping at
max(0,w), this rule tests whether the *balance* of testimony is useful across
worlds whose typical evidence magnitudes differ. No second formula is run.

Quote before truth. Admission-crossing byte stays cold; admission resets
z=w=a=0 for the following byte. Only selfnorm needs `a`; its prediction state
is40 bytes versus the existing32. Replay may use the extended struct for all
five modes (200 bytes), with measurement counters outside prediction state.
No world number, target regime, seam, surface map or future byte enters C.

Five modes on one identical candidate/cold/matched tape:

- slow, fixed hazard2^-16;
- fast, fixed hazard2^-10;
- witness, unchanged turn18;
- h8l4, unchanged nominated Don21 static pair: updated w<=-1 caps at4,
  otherwise8;
- selfnorm, the sole new relative-support ceiling above.

The witness trajectory and hazard choices are identical for witness/h8l4/
selfnorm. The cap only transfers capital from source to absorbing cold.
Thus the complete-prefix loss remains <=1 bit. Minimum hazard2^-16 gives
interval loss <=16 bits (fast<=10); the cap alone at16 would only imply
log2(65537), so it is the inherited hazard that supplies the exact16 bound.
Inactive, equal-price and protected NEW quotes must remain bitwise P0.

## One fixed fresh batch

Worlds224..231, namespace `netta-relative-support-ceiling-v1`.
N=16384, four source lives per world and targets recombined, switched,
moved_mid and unrelated. Switched/moved seams are index8192. All previous
worlds through223 are sealed evidence, with no parameter replay or tuning.
Freeze protocol, code, inherited dependencies, independent reader and built
binaries before generate -> extract -> learn -> evaluate. One run per stage.
Technical repairs preserve failed outputs, scope and the material verdict.

## Material gate, fixed before the result

All differences are charged log2 probability gains against the same P0.
Report every condition separately. There is no selection among candidates:
selfnorm is fixed, h8l4 a reference. No argmax or nomination tie rule occurs.
Mathematical comparisons use the specified inequalities; equality at a bar
passes only when `>=` is written. Numerical reproduction tolerance is1e-7;
it is not added to empirical utility bars. Raw-example ties within1e-7 use
earliest time, and are descriptive only.

Utility conditions (11):

1. Selfnorm changed-law tail mean >= fast-1 bit.
2. That individual-world comparison holds in >=5/8 worlds.
3. Changed-law whole-life mean >= fast.
4. Moved tail mean >= fast+1 bit.
5. Moved tail wins against fast in >=5/8 worlds (strict positive wins).
6. Moved tail mean >= slow-3 bits.
7. Intact early4096 retains >=95% of slow's positive mean.
8. Intact whole life retains >=95% of slow's positive mean.
9. Intact whole-life gain is positive in8/8 worlds.
10. Changed-law tail mean improves over nominated h8l4 by >1 bit.
11. Changed-law whole-life mean >= h8l4.

Structural conditions (7):

12. First admission is identical in all five modes on every life.
13. No episode admission on any unrelated life. The known Don21 admission is
    an upstream limitation; this new cap does not repair or hide it.
14. Portable episode archive <=528 bytes in every world.
15. Complete-prefix gain >=-1-1e-7 and drawdown <=16+1e-7 in all modes,
    fast drawdown <=10+1e-7.
16. Finite positive normalized candidates and mixtures (max error<=1e-8).
17. Every inactive/equal-price/protected NEW quote is exactly cold.
18. Selfnorm cap/statistic recurrence and witness/hazard identity agree on
    every byte; a>=0 and |w|<=a+1e-10; limits stay in[0,16]; odds respect
    the limit within1e-10; selfnorm clips on each intact life and the intact
    prequote limits span more than1 bit in each world (nonconstant use).

The full material result passes only if all18 conditions pass. Mean surface
and whole-life costs remain visible even when another condition passes.
Clock response and fast-share are descriptive inherited quantities, as in
Don21; no new clock mechanism is tested. No threshold moves after measurement.

## Independent check and raw output

A reader authored independently of new C/writer reconstructs source books,
seven candidate arms and all five authority streams in probability space.
It checks every quote, witness statistic, absolute statistic, cap, clip,
admission and material boolean with maximum numeric error<=1e-7. HEAD256/P0
remain inherited trace inputs; full-vector normalization receipts come from C.
Before freeze, one disjoint short fixture checks actual writer/reader schema
and C probabilities, plus exact h8l4 continuity with Don21. Check missing
required schema fields without introducing another evaluation batch.

Retain per-world early/full/tail gains, source bytes, all admissions including
unrelated, prefix minima and drawdowns, clipping and cap ranges/seam values.
For each changed life, retain greatest and least selfnorm-minus-h8l4 charged
bytes with raw byte, rank, match, candidate/P0 prices and state before/after.
Show a short actual help/harm sequence beside aggregate results. Source
archives and raw traces remain hash-pinned and available locally.

After the single result and independent verification, diagnose the measured
limitation and return the hand to Sol. No integration, next batch, commit or
push is authorized for this turn without Oleg's subsequent instruction.
