# Turn 5 design: revisable row selection under the same outer HMM

Design only, 2026-09-16. Incoming merge:
`cdcd00f560aabebbfb0980399d6192016e1d0600`.
Read turn4 protocol, report and evaluator, and turn3's C bank mechanism.
No new data, parameter sweep, or alternative scoring on worlds 40–47 was run.
The turn4 material FAIL remains unchanged.

## What the code currently remembers

`turn4/experiment.py:25` fixes the row prior to
`pi = (1/8, 7/16, 7/16)` for `(P0, A, B)`.
`posterior` at line113 normalizes that prior times two accumulated capitals.
Lines219–220 add `log2(P2_A/P0)` and `log2(P2_B/P0)` on every complete-row
observation. These are lifetime sums: the code provides no revision clock or
explicit cap on the influence of old evidence. A once-useful book must lose
its accumulated relative evidence before the local cold option can dominate.

The second memory of success and failure is separate:
`Outer.step`, `turn4/experiment.py:133`, updates the one-way source-to-cold HMM.
The row candidate is its single source expert. The HMM can substantially
discount that expert even when an internal row subsequently improves.
There is no cold-to-source transition replenishing the surviving source path.
Its posterior can recover through new favorable evidence, but there is no
uniform bound on how much recovery evidence will be needed.

The C boundary is suitable for a local change:
`turn3/bank.c:104` completes all distributions before `fgetc`, and line245
updates the observed row's inner state only after pricing. Both source books
share recipient counts and the same causal frontend. Keep that boundary.

## Candidate improvement and live improvement are different claims

Write `C=P0`, `S` for a row candidate, `w` for the pretruth outer source
weight, and `r=S(y)/C(y)` for the actual byte. The live relative price is

```
P_live(y)/C(y) = 1-w+w*r.
```

At the same `w`, a larger `r` improves this price. Across two evolving
predictors, their outer weights and admission times differ. For example,
`r_old=.1, w_old=.001` gives relative price `.9991`; the substantially better
candidate `r_new=.5` at `w_new=.9` gives only `.55`. These are possible
normalized binary prices, not measurements from Netta.

Conversely, a locally repaired candidate may be almost inaudible if its
outer source weight is tiny. If old outer log-odds are `-K` and subsequent
candidate evidence is at most `b` bits per event, overcoming that deficit
requires roughly `K/b` favorable events; changing only the local router
cannot erase it. A candidate-tail gain is therefore a diagnostic, never a
substitute for the declared live endpoint.

## Simple alternatives

| State/update | Useful property | Cost or limitation |
|---|---|---|
| Existing lifetime capitals | Exact mixture over one fixed expert per row | Old support and old rejection persist indefinitely; adding a cold branch did not itself create revision. |
| Finite window of row likelihoods | Old evidence disappears after a known number of row contacts | Needs a ring of two scores per contact, roughly `203 * window * 16` bytes; hard window edges and window choice are additional assumptions. |
| Exponentially discounted capitals | Same two scalar states per row; explicit decay time | Still normalized and causal, so outer bounds survive; the old sequence-capital identity and static-expert regret bound no longer apply unchanged. |
| Row-local one-way withdrawal to cold | Gives an explicit way to abandon an old useful source locally | A withdrawn source has no replenishment path; this is a weaker answer when a previously rejected book becomes useful again. |
| Fixed-share row posterior, selected below | Finite evidence barrier both to withdrawing old advice and to reconsidering a book | Pays a continuing revision prior and can retain some bad advice; live improvement remains empirical. |

Keep the actual source books and recipient count learner unchanged. This
step concerns memory of **which source to trust**, not rewriting the source
observations, discovering case boundaries, or resetting local experience.

## Selected smallest construction: fixed share inside each existing row

Use the existing three experts `C`, `P2_A`, `P2_B`, existing prior `pi`,
and one fixed revision probability

```
rho = 2^-10.
```

This is a design choice made before new data. Its justification is an explicit
budget: fewer than23.1 bits of worst-case static-path price on a16-KiB life,
while limiting the predictive odds of old row advice. It is not asserted to
be an optimal hazard, and no neighboring value is to be tried on this batch.

For the current row let `w=(w0,wA,wB)` be its predictive weights, initially
`pi`. Quote the complete candidate distribution before observing the byte:

```
S(x) = w0 C(x) + wA P2_A(x) + wB P2_B(x).
```

Price the actual byte through the unchanged outer gate/HMM. Only then update
the observed row:

```
q_j = w_j P_j(y) / S(y),               j in {0,A,B}
w_j(next row contact) = (1-rho) q_j + rho pi_j.
```

All other rows retain their state. The shared recipient count is incremented
once, after the quote has been consumed. Row selection learns before and
after outer admission. Incomplete contexts predict as before and do not
advance a row clock.

The clock is **every observed complete-row event**, including NEW. All three
experts still price NEW identically, so its Bayes update supplies zero
selection evidence. The subsequent fixed-share transition nevertheless
moves the weights toward `pi`. That movement is the declared prior about
possible change, not newly earned evidence that either book is correct.
Changing to a repeat-only or byte-global clock would be a different proposal.

An implementation can retain two doubles per row, for example actual log
ratios `u=log2(wA/w0)`, `v=log2(wB/w0)`, initialized to `log2(7/2)`.
Compute the three posterior log weights and apply the share with log-sum-exp;
do not overwrite a source archive or feed the current outcome into its quote.
The router state remains3248 bytes. Archive size remains14080 bytes. Work is
constant for the one observed row, with two source quotes as before.

## Precisely what is bounded

### Predictive row weights

After a transition, and also at the initial prior,

```
w0 >= rho/8 = 1/8192
wA,wB >= 7rho/16 = 7/16384.
```

Hence the aggregate remembered-to-cold odds never exceed8191. Each
individual source-to-cold ratio is at most8187.5. A previously rejected book
also has a positive predictive floor: it is not required to erase an
arbitrarily large lifetime rejection before new evidence can affect its
weight. None of this guarantees that the book will deserve a high weight.

The P0 floor implies at most13 bits of excess loss **on one event**. It does
not imply a constant13-bit bound over an arbitrarily long row subsequence:
local cold can now transition away. For `m` contacts beginning at a predictive
row state, its all-cold path gives instead

```
excess_loss_to_C <= 13 + (m-1) [-log2(1-7rho/8)].
```

At the first-ever contact that initial13 can be replaced by3, from `pi0=1/8`.
The unchanged external HMM supplies the stronger whole-recipient bound below.

### Candidate versus the previous static row3

The transition matrix on a row is

```
T_ij = (1-rho) 1[i=j] + rho pi_j.
```

Consider the latent path that keeps the same expert on all `n_r` contacts
of row `r`. It has initial mass `pi_j` and continuation probability
`T_jj^(n_r-1) >= (1-rho)^(n_r-1)`. Summing these three paths lower-bounds
the fixed-share likelihood by `(1-rho)^(n_r-1)` times the static row3 mixture
likelihood on the same history. Therefore, for any complete prefix,

```
loss_fixed_share_candidate - loss_static_row3_candidate
    <= sum_r max(n_r-1,0) [-log2(1-rho)]
    <= N [-log2(1-2^-10)]
    < 23.1 bits, if N <= 16384.
```

All expert predictions may adapt through the common observed history. The
argument requires only that this history and those predictions are the same
for every latent router path. No source labels or generator routes enter it.

This is a **cumulative candidate** comparison from life start. It is not a
23.1-bit live comparison, an arbitrary-suffix comparison, or a guarantee of
improvement after an actual change. The outer gate can admit the two
candidates at different times and the outer posterior can evolve differently.
The old static three-capital identity is no longer the new predictor's
likelihood identity: an independent check needs a three-state forward pass.

### Unchanged external guarantees

The revised candidate is a normalized causal distribution. Keep the outer
hazard `h=2^-16`, the once-only prospective32-bit admission, and the rule that
the crossing byte is still cold and following-event odds start at one.

The outer all-cold path starts at mass1/2 and is absorbing, so complete-prefix
excess loss against `P0` remains at most1 bit. Before every later active
prediction the absorbing-cold mass is at least `h`, giving at most16 bits of
loss on every interval. Local fixed share does not multiply either budget
by203 rows or by three experts. It does not restore global source influence
after that influence has become tiny.

Do not reset outer odds when an inner row changes and do not introduce a
free cold-to-source outer transition as part of this local step. Resetting
would spend the same cold-path budget repeatedly. A symmetric outer HMM
without a separately proved safeguard loses the absorbing-cold proof: its
all-cold path shrinks with interval length. Such coupling is another mechanism.

## Counterexamples the new step must retain

1. **Stable misleading advice:** the share puts some mass back on bad books
   after every row event, even when static row3 would put almost all mass on
   cold. This can cost preserved-world gain or make the outer reject useful
   rows together with harmful ones.
2. **Rare rows:** a change may be impossible to recognize before the row
   reappears sufficiently often. A row clock creates no new observations.
3. **Outer rejection:** a much better current candidate can remain almost
   absent from live predictions because of its old outer evidence deficit.
4. **Changed confidence:** by keeping outer source influence larger, a locally
   improved candidate may produce more live loss on its remaining mistakes.
5. **Age of the source prior:** the source pseudo-count contribution still
   scales as `32/(B_r+32)`. Reconsidering the source identity does not reset
   that decay or give a mature recipient the behavior of a newborn.

No normalized predictor can dominate a different normalized predictor on
every possible next outcome: increasing some probabilities decreases others.
There is no smaller unconditional live-improvement trick being withheld here.
The proposed change is a falsifiable trade between retained evidence and
reconsideration, with the existing safety bounds retained.

## One next question, measured in live prices

On independently generated recipients, does this fixed revision clock improve
the **live changed-tail** result over unchanged row3 while retaining the
preserved-life gain required by the root's pre-data gate?

Use the same component predictions and raw bytes for the two arms. Expose
candidate gain, pretruth outer weight, live gain, and local row weights
separately at each relevant event. This permits three distinct findings:
revision failed locally; revision helped locally but did not reach live
predictions; or revision improved live behavior. Keep admission times and
preserved/unrelated costs beside the changed tails.

The mathematical floor and static-path price can be checked on every prefix;
they must not replace the live material criterion. No source rewriting,
outer restart, alternate hazard trial, or next experimental phase is included
in this note.
