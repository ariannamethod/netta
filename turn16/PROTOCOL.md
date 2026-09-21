# Turn16: one earned return of portable memory

2026-09-21, Astra. Written before implementation and before any new world.
Incoming Don turn15, `3cc77c19dbfb0e2584e231d652f833beef43a044`, is audited
in `audit/`: 15 freeze pins, all retained manifests, and a pristine reader
rerun identical to Don's saved output. His 16-condition result stands.

## Question and construction

Can one renewed admission, paid from a fixed loss budget, recover useful
memory after a change of byte surface while preserving fast withdrawal
after a change of law? Only the outer authority changes. The source books,
528-byte portable cap, candidate, P0, HEAD256 frontend and first admission
remain the inherited turn13 mechanism.

Ordinary mixing of slow and fast one-way HMMs is insufficient as a design
for this question: the ratio of their surviving source paths is
`[(1-h_fast)/(1-h_slow)]^n`, independent of observations. It approaches
the slow duration prior. This turn instead tests one observable return.

Four authority modes receive the identical prospective C=P0 and S=candidate:

- `slow`: existing h=2^-16, initial source/cold mass 1/2 each.
- `fast`: existing h=2^-10, initial source/cold mass 1/2 each.
- `budget`: h=2^-10, initial cold mass c=2^-1/2, source 1-c; no return.
- `return`: same initial prior and hazard as budget, with one earned return.

`budget` is the direct control for the new return, separating its effect
from the cost of reserving half the loss budget. No rate is searched.

## Exact prospective rule

All modes start inactive and quote C. After charging observation y, add
`delta = log2(S(y)/C(y))` to the unchanged lifetime shadow. Crossing 32
activates the following byte, with the mode's initial odds z0. An active
quote uses the PREVIOUS source/cold odds z:

    Q(y) = [C(y) + 2^z S(y)] / [1 + 2^z]

Then update z using the frozen one-way source-to-cold law:

    z <- log2(1-h) - log2(2^[-(z+delta)] + h).

For `return`, until its single return has been spent, apply these steps
only AFTER the quote and the ordinary posterior/hazard update:

1. If armed, accumulate `r <- max(0, r + delta)`.
2. If armed and z >= z0, natural recovery has already occurred: disarm
   and clear r. This takes precedence over an artificial return.
3. Otherwise, if armed and r >= 32, set z=z0 for the NEXT byte, mark the
   return spent permanently, disarm and clear r.
4. If not armed and not spent and z <= -32, arm with r=0. The observation
   that arms the counter is excluded from its renewed evidence.

Initial admission never arms recovery on its own crossing observation.
No mode receives world ID, regime, seam, command labels, surface map or
future prices. The evidence counter is a repeated suffix scan; 32 does
not imply an anytime false-alarm probability of 2^-32. False returns are
measured and their loss is paid by the rule below.

## Loss budget, state and implementation

Each budget/return epoch retains a cold path of mass c. At most two
epochs give prefix likelihood >= c*c times P0, so complete-prefix gain
remains >= -1 bit. An interval within an active epoch retains at least
h cold mass. One crossing the return retains at least h*c, so the new
interval drawdown ceiling is 10.5 bits; ordinary fast/budget remain 10,
slow remains 16. Before a return, budget's cost against ordinary fast
is at most log2[0.5/(1-c)] = 0.7715534 bits for any complete prefix.

Implement a small C authority module with separate quote and observe
functions. A C replay executable consumes the frozen candidate/P0 prices;
it has no regime input. The seven candidate arms remain episode, isolated,
frequency, reverse, permuted, flat, row. Report exact sizeof persistent
authority state; the source archive acquires no bytes. Statistics and
traces are separate from state used to predict.

## Fresh worlds, fixed before data

Worlds **176..183**, namespace **netta-earned-return-v1**, N=16384.
Four new 16-KiB source lives per world. Turn13's generator, source
extraction, joint selection and seven archives are inherited unchanged.
The target regimes are `recombined`, `switched`, `moved_mid`, `unrelated`.
The switched law follows turn13; moved_mid is the recombined raw stream
renamed by turn15's independent uniform byte bijection from byte 8192 on.
Both changed lives share recombined's first 8192 bytes. The learner sees
only bytes and the authority sees only pretruth distributions and their
subsequently observed prices. Target labels are used only in measurement.

Build/fixate all code and this protocol before generation. One batch,
no parameter scan, redraw, second candidate or retuned gate on these worlds.
Earlier world ranges remain sealed. A material failure receives a diagnosis.

## One gate

All comparisons below use the episode arm and paired worlds. Gains are
log-likelihood savings relative to the same local P0. A tie is not a win.
The material gate requires ALL of the following:

1. **Useful return after a surface move:** on the final 8192 moved_mid
   bytes, return exceeds budget by >1 bit in mean and wins >=5/8; exceeds
   fast by >1 bit in mean; and trails slow by at most 1 bit in mean.
2. **Retain withdrawal after a law change:** on the final 8192 switched
   bytes, return trails fast by at most 1 bit in mean; exceeds slow by
   >1 bit in mean and wins >=5/8.
3. **Keep the earned benefit:** return retains >=90% of slow's positive
   mean early-4096 and full recombined gains, and of fast's positive mean
   whole-switched gain. Full recombined gain is positive in 8/8; early
   mean >=0.005 bit/raw-byte and positive >=6/8. No denominator with a
   nonpositive reference passes retention.
4. **The return actually occurs:** at least one episode moved_mid life
   returns after its seam. Report every return's time and old odds,
   support, candidate prices, and every return in other regimes as well.

Structural checks are part of acceptance: archives/candidates/first
admission identical across modes; unchanged byte prefixes and bijection
reconstructed; at most one return per arm/life; all prefix and interval
bounds above; no permuted admission anywhere and no arm admitted on
unrelated lives; positive normalized quotes and exact protected NEW.
An independently written Decimal probability-mass reader must reproduce
all new authority quotes/updates and gate fields within 1e-7 bit, using
the frozen turn13 independent source/candidate reader. HEAD256/P0 supplied
bindings remain that reader's explicitly inherited boundary.

## Required visible result and return of the hand

Report every world and mode: early, whole life, changed tail, return count,
minimum prefix and drawdown. Show a raw return and its subsequent prices,
one gain and one loss; include empty return sets. Distinguish policy benefit
on the two regimes from a semantic classification of them. This is a
single-return experiment; it does not measure fifty accumulated lives.

Work only in this isolated checkout. Preserve incoming records. No live
Netta, mouth or mycelium changes; no commit, push or merge is authorized.
After the audit and this measured step, hand the artifacts to Sol under
Sol -> Don -> Astra -> Sol.
