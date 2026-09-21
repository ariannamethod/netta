# Turn19: fixed posterior commitment ceiling

2026-09-21. Independent mathematical review before new data. Received HEAD:
`29c384e0a02ab65c2cb111ddbd3065e1f71a514b`. Read-only inputs: turn18's
authority, protocol, RESULT, REPORT and RAW, plus turn17's authority.
No target replay, parameter search or simulation was performed.

## Exact proposed change

Keep the first shadow-32 admission, candidate S, cold C, matched-record
indicator and witness clock unchanged. Use the old clock to choose the
ordinary hazard, price the current byte, perform the ordinary Bayes/hazard
update, then store

    z_next = min(z_hazard, 5).

The next quote uses this stored state. The existing seam is
[ar_observe](../../turn18/authority.c:41), after its odds update; the quote
remains [ar_quote](../../turn18/authority.c:30). Initial source mass 1/2
already satisfies the ceiling. The cap applies globally on active updates.
It adds no prediction state or source bytes; the fixed ceiling is a constant.

Writing v for source mass and q for its posterior after observing y:

    Q_t(y) = (1-v_t) C_t(y) + v_t S_t(y)
    q_t = v_t S_t(y_t) / Q_t(y_t)
    v_hazard = (1-h_t) q_t
    v_next = min(v_hazard, 32/33).

The witness evidence trajectory and hazard sequence are identical to uncapped
witness: their inputs are the unchanged delta and matched indicator, never
authority's quote or odds. This is a useful structural equality to retain.
Equal-price and protected-NEW quotes retain their exact equality branch.

## Pathwise loss guarantees

Use unnormalized capitals A for cold and B for source. After truth and the
ordinary hazard update:

    A_bar = A C_t(y_t) + h_t B S_t(y_t)
    B_bar = (1-h_t) B S_t(y_t).

Clipping transfers

    d = max(0, [B_bar - 32 A_bar]/33)
    A_next = A_bar + d;  B_next = B_bar - d.

The transfer conserves total capital, so the charged forecast remains Q_t.
Also `A_next >= A C_t(y_t)`. From the admitted half/half prior, cold capital
therefore supplies at least one half of C's likelihood for every complete
prefix. The **one-bit complete-prefix bound survives**. Inactive observations
and the crossing observation use C exactly; prospective admission does not
alter the argument.

Before every active quote, `A/(A+B) >= 1/33`. Starting an interval there and
following the absorbing cold capital gives

    product_interval Q_t(y_t)/C_t(y_t) >= 1/33.

Thus **every interval loss and maximum drawdown are at most
log2(33) = 5.044394119358453 bits**. This strengthens the inherited 16-bit
bound. An interval starting before admission has the stronger half-prior
bound. These proofs allow causal, observation-dependent transitions and
adaptive local C/S; they require common normalized C/S and the stated update
order, not a fixed-transition hidden-state interpretation.

## Silence: evidence is frozen; authority still decays

On a non-voting stretch with S=C, the witness evidence stays unchanged. If it
enters that stretch on slow hazard, n equal-price observations give exactly

    v_n = v_0 (1-2^-16)^n,
    z_n = log2[v_n/(1-v_n)].

Since mass decreases, the cap does not fire on that stretch. Starting at
32/33 leaves roughly 97% source mass available initially, while slow hazard
continues to withdraw it gradually. All those equal-price observations cost
exactly C immediately; their changed authority affects later unequal quotes.
If the old wrongness state was already <= -1, silence preserves fast hazard
instead. The cap supplies no new silence detector or automatic slow switch.

This separates a ceiling on accumulated confidence from the subsequent
withdrawal speed. It does not ensure a given surface tail's gain is retained:
the useful and harmful future observations still compete. Repeated clipping
can permanently forgo useful candidate capital. For example, a quote held at
the cap has likelihood ratio `(32 + C(y)/S(y))/33` against pure S; as S/C grows,
its loss approaches log2(33/32)=0.044394119 bits per such observation. Repeated
beneficial observations can accumulate a cost. There is no constant regret
guarantee against the uncapped witness or fixed fast.

## What the incoming evidence establishes

[RESULT.json](../../turn18/RESULT.json) retains FAIL 17/20, including the
law-tail deficit against fast. [RAW.md](../../turn18/RAW.md) records mean
seam odds 11.002 for witness and 4.975 for fast, a roughly six-bit gap on
those eight lives. Those observations motivate a fixed C=5 near the received
fast mean; they do not make five an optimum, nor give a one-bit law-tail
regret theorem against fast. The individual fast seam odds differ materially.
The stored first-fast offsets have median 38.5, rather than the handoff's 36;
the observed count of seven timely worlds is unchanged.

The statement in [REPORT.md](../../turn18/REPORT.md) that hazard controls
rate but never level, and therefore the entire withdrawal-rate class is
closed, goes beyond these artifacts and the update equations:

- An ordinary hazard already gives `v_next <= 1-h`, hence an odds ceiling
  `log2[(1-h)/h]`: approximately 15.999978 for slow and 9.998590 for fast.
- With a constant likelihood ratio R, positive stationary odds, when they
  exist, equal `[(1-h)-1/R]/h`. The rate law affects attainable level.
- The observed six-bit difference is not an invariant for arbitrary causal
  histories, initial conditions or clocks. The shared half prior itself has
  zero odds difference at admission.
- The proposed clipping is itself equivalent to an after-truth effective
  transfer hazard `max(h_t, 1-(32/33)/q_t)` when q_t>0. This can approach
  1/33, exceeding the old fast rate 1/1024. It regulates posterior level by
  an additional state-dependent withdrawal, so a universal division into
  disjoint rate and level classes would be misleading.

Already charged loss cannot be refunded by a later transition; that part of
the incoming diagnosis is correct. The broader exclusion would need a
specified class and an impossibility argument, rather than four mechanisms.

## Bounded empirical interpretation

One fixed C=5 step is mathematically sound and directly tests whether that
ceiling reduces law-change commitment debt without losing the witness's
surface and unchanged-life benefit. Keep uncapped witness as the immediate
control, alongside slow and fast. Report clipping and lived prices, including
where the cap harms useful memory; do not substitute shadow gain for them.

A pass would support this construction on the fresh batch. A failure would
reject this fixed ceiling under its named gate. It would not establish that
the entire feasible ceiling window is empty or exclude all commitment caps.
No second ceiling, changed clock or retry is proposed here.
