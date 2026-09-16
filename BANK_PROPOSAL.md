# Proposed bank: smallest two-book test and larger alternative

Design only, 2026-09-16. No new targets were generated or inspected for this
proposal. This is not a protocol, implementation, or empirical transfer claim.
The preceding HMM result remains a result about one source law per recipient.

## Selected scope after root critique: two books, no inner cold component

Root correctly identified that the four-book-plus-cold proposal below can pay
three bits to select a source on each row, whereas the preceding known-source
gain was only a few bits per row on average. The smaller proposed first step
uses **two** source books, with equal initial weights on each row and no extra
inner cold expert. The four-book construction below remains an unimplemented
alternative, not the selected experimental construction.

Both books produce their existing causal projected experts `S_A` and `S_B`
with shared local recipient counts. An unavailable source is exactly `P0`
through its existing `pr_quote` mask. The bank's source flag is the OR of
the two existing selected flags. For each row store one local log-odds value,
initialized to zero at recipient birth:

```
ell_r = log2(weight_B / weight_A)
bank(c) = [S_A(c) + 2^ell_r S_B(c)] / [1 + 2^ell_r]

after pricing the byte y from the pretruth quote:
ell_r <- ell_r + log2(S_B(y)/S_A(y)).
```

This update occurs on every observation of that row, including before outer
admission. NEW cannot change these weights because both projected experts
give every NEW byte exactly the same probability. If both sources are
unavailable, both equal `P0` and the weights likewise remain unchanged.

The entire bank is still one normalized expert for the unchanged once-only
32-bit gate and outer HMM-16. Its whole-life one-bit and all-interval 16-bit
loss guarantees relative to `P0` follow exactly as below. They do not charge
one such budget per book or per row.

### Exact internal comparator, and what it does not say

Fix any realized recipient sequence. On the subsequence of observations of
one row, ordinary posterior multiplication gives

```
product_t bank_t(y_t)
    = (1/2) product_t S_A,t(y_t) + (1/2) product_t S_B,t(y_t).
```

Each source expert may adapt to the entire preceding recipient history;
the identity remains valid because both use that same realized history and
the quote precedes the outcome. It requires no fixed categorical distribution.
Consequently, for the hypothetical continuously used, ungated bank,

```
loss_bank(row) <= min(loss_A(row), loss_B(row)) + 1 bit,
loss_bank(all) <= loss_best_fixed_source_for_each_row(all) + 203 bits.
```

Rows where the experts always coincide need no selection penalty; 203 is
the simple universal bound. The comparator picks one source for each entire
row subsequence, not the better source after seeing each individual outcome.
The comparator is an analysis oracle only, never input to the learner.

This is **not** a live-versus-oracle 203-bit guarantee. Before gate entry,
live predictions are cold, and the early gains of the hypothetical ungated
bank were not realized. After entry, the external HMM changes the live weight
of the bank. Resetting or switching inner experts would also change the
stated per-row comparator and incur additional selection costs.

### Scope, comparators, and cost

The two-book mechanism can choose A on one row and B on another. When both
available source experts are wrong on a row, it has no dedicated local cold
choice; the outer HMM can still go cold globally. Thus this first step asks
whether selecting parts helps in an A/B mosaic. It does not yet solve
independent rejection of every incompatible row while retaining all good rows.

A global A/B mixture with the same experts and initialization is the clean
comparator for row-local selection. A pooled count archive
`a_pool = a_A + a_B`, with unchanged prior strength 32, is a useful comparison
using the same total source evidence. Pooling can change support eligibility
and KT smoothing; it is not a pure ablation of the weight-update location.

Two books occupy `2 * 7032 = 14064` bytes plus hashes/manifest. The added
recipient selection state is `203 * 8 = 1624` bytes, in addition to the
existing common count array and one outer HMM state. Only the current row
needs two source quotes and one scalar likelihood update per byte.

No source book, recipient target, predictor implementation, or prior result
was changed in making this proposal. Numeric experiment gates belong in the
root's protocol before fresh data, rather than being improvised here.

---

## Larger alternative retained: four books plus an inner cold option

## Exact boundary

Keep the existing HEAD256 observation law, depth six, 203 equality rows,
877 source count cells, source support 32, prior strength 32, and once-only
32-bit table admission. Keep the HMM hazard `h = 2^-16` per active raw byte.

The new object is **four separate immutable source books**, and recipient
likelihood weights that can select different books on different equality rows.
No book's identity denotes its intended recipient or a generator law. Book
identities are hashes and provenance; their ordering has no predictive meaning.

Current implementation anchors, in `netta-byte-hmm-20260916`:

- `portable_recurrence/recurrence.h:13`: dimensions, count archive, and hazard.
- `portable_recurrence/recurrence.c:187`: shared local counts before truth.
- `portable_recurrence/recurrence.c:199`: source support and local contact mask.
- `portable_recurrence/recurrence.c:201`: existing cold/source construction.
- `portable_recurrence/recurrence.c:219`: projection preserving cold NEW mass.
- `portable_recurrence/recurrence.c:248`: prices from the existing quote.
- `portable_recurrence/recurrence.c:266`: shadow, HMM, and admission chronology.

The single-archive interface is presently `pr_quote(life, archive, ...)`.
The proposed bank changes the candidate distribution passed to the same outer
decision mechanism. It need not create four outer gates or four outer HMMs.

## Three constructions compared

| Construction | Preserves distinct histories | How a recipient selects | Limitation for a recipient assembled from several laws |
|---|---|---|---|
| One global mixture over complete books | Yes | A single likelihood weight per book, using all recipient observations | A book that is correct on one subset can be suppressed by its errors elsewhere; it cannot assign a different book to each row. |
| Separate likelihood mixtures per row, selected below | Yes | Fresh likelihood evidence on that row, with a local cold option | Selection has a cost on every informative row. On rare rows the recipient may not identify useful history before local learning catches up. |
| Pooled or hierarchical KT carry | Pooled counts lose alternatives; an explicit mixture of source priors can retain them | One pooled source mean, or a richer posterior over source priors and row parameters | Simple pooling can average conflicting laws into an uninformative mean. Retaining the latent alternatives requires explicit mixture state, approaching the selected construction; a learned hierarchy would add a separate shrinkage hypothesis. |

Choose the second construction for one bounded test. It directly asks whether
preserving alternatives and making the choice locally adds useful transfer.
It does not also change source confidence, the observation law, or the HMM.

## Source format and life boundary

Use four existing `NETHD256` books, each containing its own 877 `uint64` counts.
A small bank manifest records the observation-law version, archive hashes,
and source-life hashes. Do not average their count arrays. Do not transfer
source-side posterior weights, gate status, HMM odds, or source labels.

Collection should follow the same prequential HEAD256 frontend as before.
Each completed source life contributes actual observations to its own book.
If a life is later added to an existing history, write a new immutable book
and manifest version; retain the preceding version and provenance. This first
test holds the bank fixed throughout each recipient life.

For a controlled first test, all books have the same collection budget and
are included before any recipient data exist. Do not choose them by recipient
score. Exact duplicate archive hashes should not silently count as independent
books. Distinct source histories need not be identifiable as distinct laws;
the recipient receives no assertion that they are.

## Candidate distributions

Let `r` be the current equality row, with `k` repeated classes and one NEW
class. Let `b_rg` be recipient counts and `a_jrg` be source-book `j` counts.
All counts here precede the byte being predicted.

The common local recurrence law and each source prior are unchanged:

```
r0_g = (b_rg + 1/2) / (B_r + (k+1)/2)
rA_jg = (a_jrg + 1/2) / (A_jr + (k+1)/2)
r1_jg = (b_rg + 32 rA_jg) / (B_r + 32)
```

Let `p(c)` be the causal HEAD256 base distribution and `m_g` its group mass.
Define the same cold expert `C` and intermediate source expert `U_j`:

```
C(c)   = p(c) [1/2 + r0_g   / (2 m_g)]
U_j(c) = p(c) [1/2 + r1_jg / (2 m_g)]
```

Each source expert preserves the exact cold novelty mass:

```
S_j(c) = C(c),                                      if c is NEW
S_j(c) = U_j(c) [1-C(NEW)]/[1-U_j(NEW)],              otherwise.
```

The existing mask remains: `S_j = C` before one local row contact, when source
support is below 32, for incomplete context, or when `k <= 1`.
Here `C(NEW)` and `U_j(NEW)` mean the corresponding sums over all NEW bytes.

These are the existing `P0` and `P2`, applied separately to each book using
the **same** local count array and the same current base model.

## Local selection without imported authority

For each row, establish the fixed source-only eligible set
`J_r = {j : A_jr >= 32}`. Let `M_r = |J_r|`.
If `M_r = 0`, the bank predicts `C` on that row.

Otherwise initialize recipient weights with a cold expert and equal source
alternatives:

```
prior_r0 = 1/2
prior_rj = 1/(2 M_r),  j in J_r
L_rj = 0
```

Immediately before a prediction, calculate

```
Z_r = 1/2 + sum_j [2^L_rj / (2 M_r)]
pi_r0 = (1/2) / Z_r
pi_rj = [2^L_rj / (2 M_r)] / Z_r
B(c) = pi_r0 C(c) + sum_j pi_rj S_j(c).
```

Use log-domain normalization in an implementation. The `L_rj` are
recipient-local likelihood ratios, not source confidence or authority.
After pricing the actual byte `y`, update only its observed row:

```
L_rj <- L_rj + log2(S_j(y)/C(y)).
```

Then increment the one common local count. The selected source distributions
in this update must come from the pretruth quote; recomputing them with the new
count would leak the current outcome. Update these local weights on all row
observations, including before outer admission and while outer cold weight is
large. This is ordinary local learning from past observations.

Because every source expert gives NEW exactly the same probability as `C`,
NEW contributes zero to every source likelihood ratio. On repeat bytes the
ratio is exactly the ratio of the conditional WHICH-repeat probabilities.
Labels, byte names across lives, and a generator's source assignment never
enter this rule.

The local cold option allows a row for which all sources are wrong to become
cold while a different row still uses history. It does not require a row gate.
The outer gate admits the joint bank hypothesis; it does not certify each
row or each source as a Court 4 citizen.

## The unchanged outer HMM and its guarantees

Set `delta_t = log2(B_t(y_t)/C_t(y_t))`. Before admission price `C_t`, accumulate
prospective shadow evidence, and admit only after the first 32-bit crossing.
The crossing byte remains cold; the next prediction starts with odds one.

After admission, using log-odds `ell` before truth:

```
P_live(c) = [C(c) + 2^ell B(c)] / [1 + 2^ell]
z = ell + delta_t
ell_next = log2(1-h) - logadd2(-z, log2(h)),  h = 2^-16.
```

This is the same source-to-cold HMM with `B` as its single source expert.
The bank is a normalized, causal, adaptive predictor, so the previous path
argument applies without multiplying the guarantee by books or rows:

1. At admission the all-cold path has mass one half and stays cold. The entire
   recipient's cumulative excess log-loss versus `C` is at most one bit.
2. Before every later active prediction, cold path mass is at least `h`.
   Its all-cold continuation lower-bounds the probability of every interval
   by `h` times that of `C`; maximum drawdown is at most 16 bits. An interval
   starting before admission is cold until entry, followed by a cold path
   with mass one half, so it satisfies the same bound.
3. Against the static outer mixture using this **same adaptive bank**, same
   admission, and same recipient observations, the additional HMM loss for
   `T` active predictions is at most `(T-1)[-log2(1-h)]`.

All inner books and local weights learn from the common observed history,
irrespective of which latent outer path is considered. If a future design
lets hidden paths train different local models, this proof must be revisited.
The stopping time causes no future access: both the bank quote and entry
decision for the next byte are determined by the already observed prefix.

These bounds constrain regret against local cold. They do not guarantee
positive transfer, fast identification of a source, or recovery of an old law
after a long incompatible period.

## State and work

- Four serialized source books: `4 * 7032 = 28128` bytes, plus manifest/hash
  metadata. Four hashes alone add 128 bytes.
- One existing recipient count array: `877 * 8 = 7016` bytes.
- New relative log-weights: at most `203 * 4 * 8 = 6496` bytes. Ineligible
  entries may remain unused; no separate cold log-weight is needed.
- The existing outer shadow, gate, and HMM use constant state.
- Per byte, evaluate at most four source laws for the one current row and
  normalize at most five weights: `O(M*k)` arithmetic, `M=4`, `k<=6`.
  No search across all rows or cross-life byte dictionary is required.

## The price of selecting partial memories

For the subsequence of one row, this is an ordinary sequential mixture over
adaptive experts. Its total loss is at most one bit above the cold expert,
or at most `log2(2 M_r)` bits above any one eligible source expert on that row.
Both comparisons use each expert's actual causal predictions on the same
history. Summing over rows gives a comparator that may choose a different
source for each row, with the corresponding sum of selection costs.

For four books, a source choice can cost up to three bits per visited row;
203 rows would give a loose 609-bit oracle penalty. This is material relative
to the gains measured for a known matching book. It is the principal risk,
not a hidden advantage of the construction. A global book mixture pays the
source-selection cost once but has a much weaker class of comparators.

Also, `32/(B_r+32)` decays as local observations accumulate. The competing
source experts eventually approach each other. Their likelihood weights are
not promised to identify the true source: useful prediction may cease to
depend on that identity, and weak differences can remain unresolved. If the
recipient must see as many examples to select history as it needs to learn
the row locally, this bank will not produce an early gain.

## One future falsifying experience

Freeze one fresh mosaic experiment before generating its data:

- Construct four independent source laws and collect their books through
  HEAD256. Each source has the same prespecified collection budget.
- A new recipient law chooses different sources' conditional repeat choices
  on different raw equality patterns, with the assignment fixed by the
  generator's independent seed. No complete source law is the recipient law.
  Include a prespecified subset of recipient rows with independent new choices
  if the question includes rejecting history that matches no source.
- Keep this assignment entirely in the generator. The learner receives only
  the four source books and chronological recipient bytes. Learned HEAD256
  rows are not guaranteed to coincide with the generator's raw patterns;
  report this scope rather than providing an oracle map.
- Compare the row bank to cold, to the same books with one global set of
  likelihood weights, and to the existing conditional-row permutation null
  applied to every book. All outer gates and HMM parameters remain identical.

The informative claim is early gain over cold **and** the global bank on
fresh mosaic recipients, with genuine history outperforming the null bank.
Fix any numeric material threshold and replication count before generating
those recipients; this note does not itself declare another gate.

Expose row-wise gains and losses and examples in which different rows favor
different books or cold. Those are diagnostics of the mechanism, not a right
to drop difficult rows. A failure would mean that, at this evidence budget,
selecting reusable parts costs more than this memory supplies. It would not
invalidate the preceding single-source or HMM evidence.

The probability-path interpretation follows the same primary sources used
for the preceding HMM proposal:
[Koolen and de Rooij, Combining Expert Advice Efficiently](https://arxiv.org/abs/0802.2015)
and [Koolen and van Erven, Switching Between Hidden Markov Models using Fixed Share](https://arxiv.org/abs/1008.4532).
The particular bank, its counts, and its guarantees above are an explicit
construction for this proposed Netta step; no cited paper establishes its
empirical benefit here.
