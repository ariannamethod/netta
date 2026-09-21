# Turn22: relative witnessed support as a commitment budget

2026-09-21. Independent design review; no new data, old-target replay or
simulation. Received HEAD `bcc1881ee78c76d97645588bfae54e950cf4ad3e`.
Inputs: turn21 protocol, authority, report and RESULT summaries. The proposed
change concerns only the next-quote ceiling; candidate, source memory,
admission and witness hazard remain inherited.

## What the finite frontier establishes

[RESULT.json](../../turn21/RESULT.json) has four sampled points in the mean
window, h8l2–h8l5. All four satisfy the individual law-tail comparison in
only 3/8 worlds. Nominated h8l4 has mean law-tail difference -0.915568 bits
against fast, mean whole-law difference +5.680678 and moved-tail difference
+6.704898. Its nomination still requires fresh confirmation.

Those outcomes establish the failed robustness condition for the sampled
window. They do not establish that every static ceiling must fail, that
unsampled intermediate levels fail, or that an adaptive ceiling will succeed.
W2 names the nominated point; the four stored summaries additionally show
the same 3/8 limitation throughout this sampled window. The unrelated
admission is a separate failure of the common pre-authority candidate gate.
It is not evidence for or against the new ceiling rule.

## Three short constructions

Let `delta=log2(S(y)/C(y))`, lambda=31/32 and w be the existing signed
discounted witness evidence. All updates below use only already charged,
matched observations. The proposed extra statistic is

    a' = lambda*a + abs(delta),    initially a=0.

Then P=(a+w)/2 and N=(a-w)/2 are the discounted positive and negative
log-price magnitudes. They can be derived when needed; two extra counters
are unnecessary.

1. **Direct support:** B=min(16,max(0,w)). No extra state. It literally
   limits odds by a recent discounted log-score sum. A weak but sustained
   useful source can have only a few bits in this 32-vote window, so this
   rule may repeatedly remove authority already earned over longer history.
2. **Support/contradiction odds:**
   B=min(16,max(0,log2((1+P)/(1+N)))). One extra a. This regularizes a
   positive-to-negative testimony ratio, but compresses even large positive
   support logarithmically and can be tighter than direct support. P and N
   are score magnitudes, so these are not Bayesian model posterior odds.
3. **Relative support budget — recommended:**

       B = 16 * max(0,w)/(1+a).

   One extra a. This allocates a fraction of the inherited 16-bit commitment
   budget according to net support relative to total testimony. The one-bit
   denominator term is an explicit regularizer at the existing witness
   clock's unit scale, not a fitted constant. Sixteen is the inherited
   interval-risk scale. Neither constant is selected from new outcomes.

The third form separates equal net w reached by consistent positive evidence
and by extensive cancellation: more contradiction increases a and lowers
the ceiling. With constant positive witnessed delta=d, its stationary values
are w=a=32d, giving B=512d/(1+32d). As an algebraic example, d=1/32 gives
B=8 while the first two rules give B=1. Thus this rule deliberately allows
more odds than the recent net log-score itself. **B is a budget heuristic,
not a claim of eight independently re-earned likelihood-ratio bits.**

## Exact prospective order

The seam is [ar_observe](../../turn21/authority.c:52), replacing only the
grid's cap calculation after the inherited ordinary odds update.

1. Quote with old authority z. Preserve the exact inactive/equal-price
   branch in [ar_quote](../../turn21/authority.c:41).
2. Charge truth and update the unchanged lifetime shadow by delta. If
   previously inactive, the shadow-32 crossing initializes z=w=a=0 for the
   following byte; the crossing observation enters neither witness counter.
3. When already active, choose h from OLD w: fast 2^-10 if w<=-1,
   otherwise slow 2^-16. Compute the inherited hazard-updated z_hazard.
4. If matched>=1, set w'=lambda*w+delta and a'=lambda*a+abs(delta).
   Otherwise leave both unchanged.
5. Compute B' from updated w',a', then z_next=min(z_hazard,B'). Only the
   following quote sees it. Increasing B does not restore discarded source
   capital, reset odds, or change admission.

The clock/hazard sequence remains exactly the uncapped witness sequence:
its delta, matched and w inputs are unchanged. a adds one double, ordinarily
eight bytes, making the current 32-byte record 40 bytes; confirm sizeof in
the implementation. Source archive cost is zero. Updates use constant work,
one absolute value and scalar arithmetic; no retained observation window.

## State invariant and pathwise guarantees

From a=w=0, a>=0 and |w|<=a follow by induction:

    |lambda*w+delta| <= lambda*|w|+|delta|
                         <= lambda*a+|delta|.

Non-voting observations preserve the inequality. Hence 0<=B<16 in exact
arithmetic; implementation checks should declare their floating tolerance.
For bounded |delta|<=M, a<=32M. Finite-state arithmetic guards remain needed
as in the inherited module; this proof is not a bound on arbitrary double
inputs.

Let A,Bcap be unnormalized cold/source capitals. After truth and the ordinary
hazard transition, cold capital is

    A_bar = A*C(y) + h*Bcap*S(y).

Clipping can only transfer nonnegative mass from source into this cold
capital, conserving total capital. Therefore A_next>=A*C(y). Initial cold
mass 1/2 gives a complete-prefix gain of at least -1 bit, independently of
the history-dependent ceiling and first-admission stopping time.

The ordinary hazard update puts at least h_min=2^-16 mass in cold; clipping
can only increase it. Starting at any interval, its absorbing cold path
therefore gives likelihood ratio at least 2^-16, hence interval loss and
maximum drawdown at most 16 bits. **This exact 16-bit bound comes from the
minimum hazard.** A ceiling of 16 alone would give log2(1+2^16), slightly
greater than 16. Prospective cap raises never move mass out of cold, so
they do not invalidate either capital argument.

## Counterexamples and measurement limits

- Large contradictory observations can raise a while reducing w, cutting
  useful source authority sharply after a transient error. The subsequent
  recovery handicap seen in earlier global caps can recur here.
- Consistent positive evidence can approach the full 16-bit ceiling, leaving
  substantial pre-change commitment. Self-normalization does not guarantee
  a law-tail gain over static h8l4 or fixed fast.
- Unmatched silence freezes both counters but still applies the inherited
  hazard to authority. Matched zero-delta testimony decays both counters;
  the fixed denominator regularizer then gradually reduces the ceiling.
- The counters restart after first admission. Early useful transfer must
  be measured, rather than assuming a normalized score cures all startup
  cost. The formula does not use world labels or the hidden seam.

The one empirical question is whether this fixed relative-support law improves
individual-world law-tail robustness while retaining whole-life and surface
benefits against fresh static h8l4, with slow/fast/witness as references.
Report the actual ceiling trajectory, clipping and charged helpful/harmful
observations. A failed new gate rejects this construction under that gate;
it does not close every static or self-normalizing authority class. No second
functional form or parameter retry is proposed for the fresh batch.
