# Turn25: witnessed hazard with a hysteresis latch

2026-09-22. Independent review before new worlds or implementation. Received
HEAD `798085be9a2bf8b6c41860238d4f38450558e126`. Only existing turn24
protocol, report, oracle code and RESULT summaries were inspected. No new
authority trajectory, parameter search or old-target replay was run.

## What the oracle experiment establishes

[RESULT.json](../../turn24/RESULT.json) provides a concrete privileged-information
policy that clears the eight utility bars on that batch. O-level-0 obtains
exact fast tail prices by construction while staying slow on the other
regimes. O-rate-0 also clears the bars, with mean law-tail difference
+0.274206 bits against fast. Thus the bars are numerically compatible on
those observations, and immediate level replacement was unnecessary for that
successful oracle policy.

The experiment does not establish that a causal winner exists, that rate-only
response universally dominates a clamp, or that detecting the seam has ceased
to matter. The oracle has both perfect regime selection and a supplied seam:
it never reacts to an innocent prefix or a moved surface. A causal witness
does not receive those privileges.

The largest successful **tested** delays are 32 for level and 128 for rate,
from {0,32,128,512}. In particular O-rate-128 has mean law-tail difference
-0.181716 bits, while O-rate-512 has -1.245014 and fails that bar. This is
a four-point response measurement, not a universal latency limit or a proof
that every delay below 128 succeeds and every larger delay fails. The old
38.5-byte median is measured after a known seam on a different batch; a
median alone also omits slow cases and false alarms. Comparing it with 128
does not isolate post-detection policy as the sole cause of past failures.

In [oracle.c](../../turn24/oracle.c), changing the rate at t=seam+d leaves
the carried odds unchanged for that quote; the fast update affects the next
quote. The level oracle additionally changes the current quote's odds before
pricing. Detection/update/quote timestamps should remain distinct.

## Why the irreversible first alarm was rejected before data

The received recombined summaries already contain witness fast updates before
t8192 in all eight worlds240–247:

    48, 776, 633, 820, 186, 1330, 1745, 1405.

Because witness evidence is independent of authority, a first-ever negative
witness latch would be spent during every one of those unchanged prefixes.
This is a deduction from existing clock activity, not a replay of new latch
prices. It shows why first-ever activation is a different problem from first
fast activity after a supplied seam. Permanently fast afterwards could lose
the slow policy's useful-transfer advantage on intact and moved lives.

Root therefore chose the handoff's allowed *single rearm law* before new data:
the existing negative threshold -1, with a symmetric +1-bit rearm threshold.
No permanent-latch experiment or threshold sweep is added.

## Exact causal law of the selected step

State: inherited lifetime shadow, source/cold log odds z, witness evidence w,
active flag, and one Boolean `fast_latched`, initially false. No odds reset,
cap, repeated admission or source-memory modification.

For an already-active observation t, let ell_t be the old latch:

    h_t = 2^-10 if ell_t, otherwise 2^-16
    Q_t = (C_t + 2^z_t S_t)/(1 + 2^z_t)
    delta_t = log2[S_t(y_t)/C_t(y_t)]
    z_(t+1) = log2(1-h_t) - log2(2^[-(z_t+delta_t)] + h_t)

Price truth using Q_t before the updates. Then update the inherited witness:

    w_(t+1) = (31/32) w_t + delta_t    if matchedL >= 1,
              w_t                     otherwise.

Finally set the latch for the following observation:

    ell_(t+1) = true                   if w_(t+1) <= -1,
                false                 if w_(t+1) >= +1,
                ell_t                 otherwise.

The exact inequalities and old-state hazard are part of the construction.
Inactive quotes remain C. The shadow-32 crossing observation initializes
z=w=0 and ell=false for the next quote; it does not enter the clock.

If truth at t first crosses -1, that observation still uses the old slow
hazard. The first fast update is on t+1; its first possible quote effect is
t+2. The corresponding convention applies to rearming slow. Before the first
negative crossing, this agrees with witness; it differs when w has recovered
above -1 but not yet reached +1. A positive surprise can cross +1 immediately;
the rule promises no minimum holding time.

The new information is one bit. A separate unsigned latch in the original
three-double, mode-and-active record normally changes sizeof from 32 to 40
bytes through alignment; a packed flag need not add eight bytes. Report the
actual implementation size. Source archive growth is zero, update work O(1).

## Guarantees and remaining failure cases

With unnormalized cold/source capitals A,B, every active update is

    A_next = A*C(y) + h_t*B*S(y),
    B_next = (1-h_t)*B*S(y).

Thus A_next>=A*C(y). Initial cold mass 1/2 supplies the complete-prefix
one-bit loss bound for any causal latch sequence. Each update puts at least
h_min=2^-16 probability mass in cold; its absorbing continuation supplies
the 16-bit interval/drawdown bound. These statements do not require a fixed
hazard or prohibit rearming. State changes never inject authority or refund
already charged loss. After a fast update, the next quote has at least
2^-10 cold mass, but the global bound remains 16: a newly set latch alone
does not retroactively apply a fast transition.

Known limitations to measure, not hide behind an oracle analogy:

- Early innocent alarms still activate fast; +1 rearming limits permanence
  but can charge additional intact-life withdrawal before recovery.
- Non-voting silence freezes w. If it enters the interval (-1,+1) while
  latched fast, it stays fast; the original witness could already be slow.
- Positive runs inside a changed law can rearm slow. This is hysteresis,
  not the oracle's permanent knowledge of which regime has arrived.
- The source probability can recover through ordinary positive evidence
  even while hazard is fast. Conversely rearming slow does not reset a low
  source weight. Distinguish a hazard mode from source trust itself.

## Independent gate and visible measurements

Inherit the exact eight utility bars from
[arm_bars](../../turn24/experiment.py:371), all evaluated on new worlds:

1. Mean switched-tail difference versus fast >= -1 bit.
2. Mean whole-switched-life difference versus fast >= 0.
3. Mean moved-tail difference versus fast >= +1 bit.
4. Moved-tail improvement versus fast in at least 5/8 worlds; ties lose.
5. Mean moved-tail difference versus slow >= -3 bits.
6. Recombined early4096 mean retains >=95% of a positive slow mean.
7. Recombined whole-life mean retains >=95% of a positive slow mean.
8. Recombined whole-life gains against C are positive in all eight worlds.

Use slow, fast and unchanged witness as the direct references. Do not replace
whole-life prices by tail-only success or make oracle128 a new selection rule.
The independent reader should reconstruct quotes, w, latch transitions and
old-latch hazards from chronological prices; check unchanged admission and
clock, exact protected/equal quotes, normalized distributions and capital
bounds. Inherit the received admission-identity/reporting treatment, rather
than silently reinstating an obsolete zero-unrelated-admission gate.

Report every first alarm and rearm, time spent fast, prefix versus tail gain,
and raw trigger/next-price windows. Separate pre-seam alarms, already-fast at
the seam, and first post-seam activations; an already-fast policy has not just
detected that seam. These measurements expose what hysteresis actually changes
without supplying labels to the controller. One fixed rule, one fresh gate,
then preserve the result and return the hand.
