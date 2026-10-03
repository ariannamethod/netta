# Turn31 design review: remember the case independently of its permission

2026-09-26. Read-only review of incoming `bbd8dbb41f88d5a6697ca03533176bdbbfa6ee49`.
No recipient replay, parameter scan, new world, or implementation in this note.
Source files: turn30 protocol/report/router3/experiment and turn29 calibrate.

## What turn30 identifies, and the proposed half-prior does not

Turn30 holds the 12 addresses fixed and compares two complete predictive
systems: three adaptive components against a pooled-count binary mixture.
Its FAIL establishes the declared result on that batch. The early partial
window precedes the partial change; it measures the shared unchanged prefix.
The reported partial-tail gain is a separate observation, not that gate.
Two extrema exhibit actual harmful commitment and helpful retreat. They do
not classify every source of the aggregate difference or close distinct-case
memory as a family.

In `turn30/router3.c:11–16`, the three-way prior is `(1/8,7/16,7/16)`.
Its **aggregate memory prior is 7/8**, identical to pooled2. If both source
experts are the same predictive distribution R, their weights sum to a
binary memory weight whose update is exactly the inherited 7/8-prior law.
There is no collapse to 7/16. Changing pooled2 to 7/16 would test a different
commitment prior. Either success or failure of that arm would leave the
distinction-versus-calibration attribution unresolved.

There is a second difference before any recipient discrimination. On k
repeat roles, with source totals N_A and N_B, the two source probabilities
are proportional to `(a_r+.5)/(N_A+k/2)` and `(b_r+.5)/(N_B+k/2)`.
Their equal probability mixture generally differs from the pooled-count
law `(a_r+b_r+.5)/(N_A+N_B+k/2)`. Counts and smoothing both matter.
Even `a=b=(1,0)` gives identical case experts `(.75,.25)` but pooled
`(5/6,1/6)`. Thus equal *count vectors* are not the predictive-collapse
fixture. Use identical probability vectors to test that identity.

## Exact factorization of the existing three-way update

For one matched record, write

```
u = w_A+w_B,       v = w_A/u,          u0=7/8, v0=1/2
S = v*P_A+(1-v)*P_B
Q = (1-u)*P0+u*S
```

The current Bayes update (`router3.c:59–68`) implies, on observed truth x:

```
u_post = u*S(x)/Q(x)
v_post = v*P_A(x)/S(x)
u_next = (1-rho)*u_post + rho*u0
lambda = rho*u0 / u_next
v_next_flat = (1-lambda)*v_post + lambda*v0
rho = 2^-10
```

This is an algebraic identity, including the existing normalized posterior.
Flat3 already learns conditional A/B evidence on every matched visit. The
loss of that evidence arises in the subsequent share operation. As
`u_post→0`, lambda tends to 1 and nearly resets the case preference. It is
not enough to say that each case began with half the pooled prior.

## One proposed organ: separate the two revision clocks

Keep the same predictive Q and the same update of u. Change only conditional
case revision to

```
v_next = (1-rho)*v_post + rho/2
```

Initialize `u=7/8,v=1/2`; use the inherited rho without a new rate or prior.
Update both scalars only on visits to that record, after the current quote
is charged. This includes NEW/equal observations, whose likelihood ratios
are 1: only the declared shares act. No match means no update. Both states
learn before outer admission and reset at the next life. Counts, dictionary,
record selection, repeat projection and source identities stay unchanged.

The new case revision is weaker than flat3's when `u_next<u0`, equal at u0,
and slightly stronger when `u_next>u0`. It is **decoupling**, not uniformly
slower forgetting. It can retain which past case is comparatively useful
while the recipient currently trusts neither enough to act on it.

State: two doubles per record, expected 192 bytes for 12 records, compared
with flat3's three doubles/288 bytes. Portable NETEB001 remains 528 bytes
when full. Report actual allocation. Predictive normalization is unchanged;
the hierarchy simply factors three nonnegative mixture weights.

## Bounds, finite guards, and a real cost

After matched updates, in exact arithmetic:

```
7/8192 <= u <= 1-1/8192
1/2048 <= v <= 1-1/2048
```

Consequently an unconditional A/B weight can be as small as
`7/16777216`, much smaller than flat3's `7/16384` floor. This is an intended
effect of the changed law. A confidently retained but obsolete case can
harm later recovery; source preference is not a guarantee of applicability.

Use positive finite component distributions, log-sum-exp mixtures and
after-truth probability ratios. Check finite u/v and their domains, allowing
the declared floating tolerance at analytical floors. Preserve exact P0
quotes when both sources equal P0, and exact protected NEW. On an equal
likelihood observation the independent shares still move u/v toward prior;
silently skipping the update would implement a different clock.

Let `c=-log2(1-rho)` and n_j be the number of visits to record j. A fixed
case-A/B no-switch path through both local layers costs at most
`log2(16/7)+2*(n_j-1)*c`; a fixed P0 path costs
`3+(n_j-1)*c`. Sum those charges over visited records for a fixed per-record
expert allocation. Flat3 pays only one share factor on its corresponding
source path. The factorized guarantee therefore has an extra source-path
tax; there is no free improvement theorem. These are candidate bounds.

The unchanged outer mixture retains its full-prefix loss bound of 1 bit and
interval drawdown bound of 16 bits against P0. Normalized causal candidates
are sufficient for those cold-capital bounds; the inner three-state Markov
interpretation need not survive. No cap, reset or altered outer hazard is
required.

## The smallest informative comparison

On one fresh batch, preserve all 12 addresses and source archives. Compare:

| Arm | Memory distribution and local law | What the comparison identifies |
|---|---|---|
| Factorized | Adaptive v, independent rho share for u and v | New intervention |
| Flat3 | Incoming three-way Bayes/share | Effect of decoupling conditional revision |
| Balanced2 | Fixed `S=(P_A+P_B)/2`; binary u with prior 7/8 and rho | Value of learning A/B preference under the new law |
| Pooled2 | Pooled counts; inherited binary u | Overall value against the pooled incumbent |
| Permuted factorized | Same new law, jointly rotated source roles | Correspondence control |

Balanced2 averages the **normalized 256-byte component forecasts**, not
counts, log prices, or source totals. It initially quotes exactly the same
distribution as factorized/flat3. Under identical source distributions,
all three collapse mathematically to the same binary law. With distinct
sources, Balanced2 preserves their initial shape but cannot learn which
case belongs at a particular record.

Retain turn30's explicitly named pooled2-driven shadow32 shared admission,
with the crossing byte still cold, and separate post-entry outer histories.
That controls entry timing; it measures efficacy after an incumbent-triggered
entry. It does not establish autonomous earlier admission by the new organ,
and shared admission is not guaranteed to favour or disadvantage one arm.

Gate partial-change claims on bytes 8192–16383, with an unchanged-prefix
retention condition stated separately. Require actual received gain against
the relevant controls, not merely a favourable v or a better candidate.
Declare every margin before fresh data; no inherited failure is rescored as
a new PASS. Expose rows with low permission, retained preference, subsequent
useful reuse, and retained wrong preference causing harm. Candidate and
received results must remain distinct.

## Exact interpretation of a future result

Factorized beating flat3 would support this change in conditional-history
retention, including its downstream effects on u and outer authority.
Beating Balanced2 would support adaptive case choice beyond a fixed equal
mixture; beating pooled2 would additionally establish utility relative to
pooled counts at these same addresses. None alone establishes that all
observed gains came from the instantaneous case weight on a selected byte.

Failure would reject this fixed mechanism on the declared experiment. It
would not establish that distinct cases, other address representations or
sparse encoding can never help. The practical advance sought is narrowly
defined: retain evidence about a partially useful past case while withholding
its present permission, then reuse that evidence when it becomes applicable.
