# Turn25 diagnosis — saved prices only

**Post-hoc description, not an intervention or another gate.** Reproduce with
`python3 turns/turn25/diagnose.py` from the repository root. The script reads
RESULT and the eight retained switched authority traces, plus the two moved
traces containing RESULT's extrema. It does not run a predictor, change a
price, or inspect another batch. Input hashes and exact rows are in
`DIAGNOSIS.json`.

## What failed

The material result is 7/8. Only law-tail proximity to constant fast fails:
mean latch − fast is **−4.945838658 bits**, against the preregistered ≥−1.
Latch − witness is only **+0.180752923 bits** on this tail. Its whole switched
life still exceeds fast by +5.308592440 bits; that earlier transfer gain does
not cancel the separately required tail condition.

## First detection does not erase the authority already carried

At the constructed seam all eight latches are slow. Mean log2 source/cold
odds are **12.050203** for latch, **12.105277** for witness, and **6.112680**
for constant fast. These are recorded states, not a claim that the learner
knows the seam.

The table partitions actual latch − fast prices. The “through update” column
includes the first fast update byte, whose quote still precedes that update;
the first quote it can affect is the next byte. All positions are zero-based.

| World | First tail set | First fast update | First return to slow | Through update, bits | Later, bits |
|---|---:|---:|---:|---:|---:|
| 248 | 8227 | 8228 | 8236 | −0.802839 | −3.502798 |
| 249 | 8228 | 8229 | 8237 | −0.830298 | −4.902039 |
| 250 | 8299 | 8300 | 8326 | −2.848195 | −1.406629 |
| 251 | 8289 | 8290 | 8322 | −2.230579 | −3.179011 |
| 252 | 8506 | 8507 | 8561 | −1.534757 | −2.706088 |
| 253 | 8192 | 8193 | 8196 | −0.584992 | −3.909090 |
| 254 | 8228 | 8229 | 12878 | −4.018610 | −1.643037 |
| 255 | 8257 | 8258 | 8332 | −3.388333 | −2.079415 |
| Mean | | | | **−2.029825** | **−2.916013** |

Before the first fast update itself the mean price is −1.913694 bits; its
already quoted byte adds −0.116131. First fast-update delays are
`[36,37,108,98,315,1,37,66]` bytes, median 51.5. World253 starts fast on the
second tail byte yet finishes −4.494082 bits below constant fast. Thus a
short first-detection delay alone is not sufficient on this recorded life.
This is not a test of forcing every life to that delay.

Every life returns to slow after its first tail set. Tail set counts are
`[92,34,35,19,3,28,2,14]`, with one fewer return in each life. The clock sees
positive local bursts even in the permanently changed generator regime.
Those bursts are evidence available to it; the regime label is not.

## The small improvement is an exact benefit/cost cancellation

Partition by the sign of the **recorded candidate − cold** log price:

| Tail observations | Mean count | Latch − fast, bits | Latch − witness, bits |
|---|---:|---:|---:|
| Candidate helps | 1550.500 | +22.052595 | −0.742182 |
| Candidate harms | 1829.875 | −26.998434 | +0.922935 |
| Equal price | 4811.625 | 0 | 0 |
| Total | 8192 | **−4.945839** | **+0.180753** |

The sign pattern follows the unchanged mixture law. If `r=2^z` and
`d=2^(candidate−cold)`, the quoted ratio to cold is `(1+r*d)/(1+r)`;
the after-truth authority transition is
`r'=(1−h)*r*d/(1+h*r*d)`. The transition increases with r and decreases
with h. With shared admission, fast has h at least latch's h; latch has h
at least witness's h because any old w≤−1 already entails a set latch.
Therefore `r_fast ≤ r_latch ≤ r_witness` on this path. The saved rows
respect this ordering. Extra withdrawal helps when the candidate errs and
costs when it is right. Hysteresis has reduced both sides slightly relative
to witness; the favourable net change is small.

Across the eight tails latch uses fast on 59,434 updates, witness on 57,579,
and constant fast on all 65,536. The latch's extra 1,855 fast updates include
**855 neutral observations**, all with exactly zero current price difference
and **zero neutral releases**. The remaining impact is carried into later
odds. Indeed, observations with the same current latch/witness hazard sum to
+0.190928 bits per life; those with different hazards sum to −0.010175.
This partition does **not** identify the causal benefit of either set of
transitions: current prices already contain their prior histories.

## Concrete retained rows

The following rows are copied from the saved traces; neighbouring rows and
all four prices are retained in JSON.

- **Neutral hold works as specified:** world248, t8227 sets the latch with
  w′=−1.198464. At t8228 useful advice raises w′ to −0.334077. At t8229,
  candidate=cold, latch uses fast while witness uses slow, and both prices
  are exactly cold. At t8230 another bad advice event costs latch **0.000418
  bits less** than witness. Nothing was repriced to construct this example.
- **Useful withdrawal, still far from constant fast:** world252, t8610,
  candidate−cold=−1.954147; carried odds are latch +0.721612, witness
  +1.661580, fast −3.994727. Latch helps by **+0.302798** versus witness,
  yet loses **−0.829317** versus fast. All three use fast on this update;
  the quote difference comes from their carried states.
- **The cost when advice is useful:** world248, t9151,
  candidate−cold=+2.603745; latch/witness odds are −1.294191/−0.542980.
  Latch loses **−0.311717** to witness. Both currently use slow. Two preceding
  equal-price bytes have zero immediate difference; prior authority remains
  different.
- **Surface transfer has both signs too:** world254 moved, t9175, bad advice
  gives latch **+0.659973** versus witness. World255 moved, t9091, useful
  advice gives **−0.377727**; that truth clears the latch only for subsequent
  updates. Mean moved-tail latch−witness is −0.246711 bits, while latch still
  exceeds constant fast by +6.693739 and passes the moved bars.

## One question for the next hand

**How much of the remaining law-tail cost requires reducing confidence
already accumulated before the first negative evidence, versus preventing
returns to slow on brief positive bursts—while retaining useful transferred
advice when the old law survives a surface change?**

The current partition establishes that substantial loss remains after first
fast use, and identifies both carried confidence and actual returns. It does
not isolate their counterfactual contributions or select a new rule. No
claim about the impossibility of causal adaptation follows from this one
failed latch. The original transfer objective and all eight bars remain.
