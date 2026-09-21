# Authority design review: one earned return

2026-09-21. Independent mathematical review before new data. Scope: authority
only; no source memory, candidate, first-admission evidence or target generator
change. No fresh worlds or sealed target traces were used in this review.

## Observed motivation and exact seam

The incoming [turn15 report](../../turn15/REPORT.md)
describes opposite preferences: fast withdrawal helped after a permanent law
change in turn14; slow withdrawal retained more gain after the midlife surface
change in turn15. These are observations about those fixed batches. They do
not establish that an authority rule can infer the physical cause of a loss.

The implementation seam is [Outer.step](../../turn14/experiment.py:75).
It receives causal log prices for the same normalized byte distributions
`C_t=P0_t` and `S_t=candidate_t`. Source/candidate learning proceeds identically
regardless of the authority's weight. The quote is made before observing the
next byte. A lifetime shadow threshold of 32 admits the following quote.

After admission, write `w_t` for source mass and `h` for withdrawal hazard:

    Q_t = (1-w_t) C_t + w_t S_t
    delta_t = log2[S_t(y_t)/C_t(y_t)]
    q_t = w_t S_t(y_t) / Q_t(y_t)
    w_(t+1) = (1-h) q_t.

The existing absorbing cold path gives the ordinary half/half initial policy
a one-bit complete-prefix loss bound. After every active transition cold mass
is at least `h`, giving interval loss at most `-log2(h)`. These bounds are
pathwise likelihood bounds relative to C, not promises of positive gain.

## Why ordinary Bayesian mixing of two hazards is insufficient here

Suppose the complete slow and fast controllers start together, with a half
meta-prior on each, and share C and S. The mixture can be represented by one
absorbing cold state and two source states, both emitting S. After `n` active
transitions their unnormalized surviving source capitals are

    b_s = (1/4) (1-h_s)^n product_t S_t(y_t)
    b_f = (1/4) (1-h_f)^n product_t S_t(y_t).

Consequently `b_f/b_s=[(1-h_f)/(1-h_s)]^n`: observations cancel exactly.
For `h_s=2^-16` and `h_f=2^-10`, 8192 transitions suppress the surviving fast
source branch by approximately 11.4 bits. The posterior over the complete
controllers can still move because of their cold paths; the surviving source
mixture nevertheless ages deterministically toward the slower duration law.
This remains true when the shared causal C and S learn from the same history.
It does not provide evidence-based selection of a temporary versus permanent
change. I do not recommend it as the next empirical construction.

A fixed arithmetic average of the two *quote streams* would avoid meta-weight
starvation. Jensen's inequality preserves the one-bit whole-life bound and
gives a 13-bit interval bound, the mean of 16 and 10. It retains both clocks
without selecting between their meanings; it is an unselected simple
alternative, not an additional proposed trial.

## Recommended construction: a single renewed admission

The [frozen prospective specification](../PROTOCOL.md) supplies the details.
Reserve the existing one-bit life budget as two half-bit epochs:

    c = 2^-1/2                    # initial cold mass in either epoch
    z0 = log2[(1-c)/c]           # approximately -1.271553303
    h = 2^-10.

The first admission still uses the unchanged lifetime shadow threshold 32,
and the crossing byte is cold. Its following quote starts at z0. The ordinary
after-truth fast update in source/cold log odds is

    z' = log2(1-h) - log2(2^[-(z+delta)] + h).

Before a return has been spent, `z' <= -32` arms a suffix evidence counter
`r=0`. The arming observation is excluded. While armed, subsequent observed
prices update `r=max(0,r+delta)`. If ordinary authority has recovered to
`z' >= z0`, disarm and clear r first. Otherwise `r>=32` resets the *next*
quote to z0, spends the one return permanently and clears r. A naturally
recovered attempt may arm again after another collapse; at most one actual
reset occurs. Neither the lifetime shadow nor source/local counts reset.

This gives old useful memory a route out of deep accumulated authority debt
after fresh candidate success. It does not name a change of surface or law.
The `budget` control has the same prior and fast hazard without a return;
ordinary `slow` and `fast` retain their original half/half priors.

Prediction state added to the old controller is one double r and one small
mode (`unarmed`, `armed`, `spent`); conventional alignment costs about 16
bytes. Actual C sizeof must be reported. Portable source bytes added: zero.
One fixed-size update per byte is sufficient; no suffix buffer is required.

## Guarantees and their scope

**Complete prefixes.** Within an epoch the absorbing cold branch retains
initial mass c, hence its likelihood ratio against C is at least c. A prefix
crossing two epochs has ratio at least `c*c=1/2`. Thus gain is at least -1 bit
for every complete prefix, including prefixes before entry or before return.
If no return occurs, the stronger -0.5-bit bound applies after first entry.
Data-dependent epoch boundaries do not invalidate the argument: the reset
is after truth and each product inequality holds for the realized segment.

**Intervals.** Starting inside an epoch, the prequote cold mass is at least
h, since initial c is also above h. Staying on its absorbing cold path gives
interval likelihood ratio at least h. Crossing the single return contributes
one additional factor c. The respective worst losses are 10 and 10.5 bits.
An interval beginning before first admission and crossing return has the
stronger bound from c squared. A 10-bit bound must not be claimed for all
return-crossing intervals. The same statements bound maximum peak-to-trough
drawdown of cumulative gain relative to C.

**Cost of reserving the return.** Until an actual reset, budget and ordinary
fast mix the same two starting-path laws, A for initially cold and B for
initially source. For every realized prefix,

    [c A + (1-c) B] / [(A+B)/2] >= 2(1-c).

Budget's extra loss against ordinary fast is therefore at most
`log2[0.5/(1-c)] = 0.771553303...` bits. This comparison concerns complete
prefixes up to reset; it is not a post-reset regret claim against fast.

**No semantic or false-alarm guarantee.** Repeated suffix scanning is not one
fixed-origin likelihood-ratio test. The threshold 32 alone therefore does
not imply a life-wide false-return probability of `2^-32`. A permanently
wrong candidate can earn a fortunate positive suffix, and neutral hazard
decay can eventually arm the counter. Both events remain within the finite
loss budget. Conversely a transient disturbance can fail to earn 32 new
bits, or can receive its sole return too late. Nothing ensures recovery
after multiple later changes; that limitation is deliberate in this step.

## One falsifiable question

On the single fresh batch specified before data in [PROTOCOL.md](../PROTOCOL.md),
does return improve the moved-surface tail over the same-prior budget control,
while keeping fast withdrawal's permanent-change tail benefit and recombined
gain? Compare actual lived prices, not candidate shadow alone. Record each
return's old odds, suffix evidence, triggering truth and subsequent quotes,
including returns in unchanged or unrelated lives and empty return sets.

The protocol's numerical gate is an empirical decision, not a mathematical
consequence of the bounds above. Its budget control separates renewed
admission from prior allocation. A failure remains a failure even if the
pathwise guarantees and arithmetic verification pass. No additional rate,
second return or hazard experiment is proposed in this memo.
