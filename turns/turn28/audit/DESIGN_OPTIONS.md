# Turn28: separate remembered continuations under one byte budget

2026-09-25. Design review before new data or implementation. Incoming HEAD
`48e36938f887ec57b26e4ac20b83f58dd5d149ab`, branch `astra/turn28-memory`.
This note reads prior mechanisms and reports; it does not rescore sealed
targets. The root hand owns the incoming reproduction and final protocol.

## Two bounded choices

### 1. Clip the CUSUM contribution

Don proposes `S'=max(0,S+min(2,-delta)-0.5)`, latch permanently at 8.
This is implementable and leaves the existing authority bounds intact.
It caps a positive increment at 1.5; **six consecutive voting observations
with delta≤−2 still reach S=9**. Negative average drift and a reported
standard deviation do not prohibit such a cluster. They do not establish
that the intact maximum stays below 8 or that changed-law detection remains
within 128 bytes. The stated slow-versus-fast received gap also is not the
sum of candidate-versus-cold evidence consumed by this detector.

Likewise, “any unbounded statistic eventually crosses every threshold” needs
assumptions about its sequence law; it is not a deterministic consequence
of unbounded support. Turn27's 13 false latches are the actual counterexample
to its declared false-alarm premise. Its C5 gain supports that measured
candidate on that comparison, rather than establishing the correctness of
an entire detector family. Clipping remains a hypothesis, not a guaranteed
repair. It does not itself enlarge the contents of portable experience.

### 2. Keep two continuations per remembered episode prefix — recommended

The concrete restriction is in `turns/turn13/experiment.py:218–224`: a record
adds immediate continuation counts over every source tape. Its serialized
form at lines315–319 keeps only that sum. `episode.c:279–292` selects one
longest completed prefix; lines312–321 project its pooled counts onto the
recipient's current repeat heads. Source disagreement has already vanished
before the recipient can assess it.

Turn3 already demonstrated a local A/B selector over exact HEAD6 rows;
turn4's local cold option improved candidate prices without repairing live
tails; turn5's fixed-share revision then passed its own fresh gate. Reuse
that known selector on **sequence prefixes with separate continuation
histories**, paying for both channels in the existing 528-byte archive.
This is a development of that lineage, not a claim to invent local mixtures.

## Proposed format and source construction

- Header: magic8, `u32 nrules`, `u32 nrecords`: 16 bytes.
- Shared dictionary: at most32 `(left:u16,right:u16)` rules: 128 bytes.
- Each record: `rule:u8,prefix_length:u8,reserved:u16`, followed by
  two arrays of seven exact `u16` continuation counts: 32 bytes.
- At most12 records: total at full capacity **16+128+12×32=528 bytes**.
  Store actual counts if the source learner produces fewer rules/records.
- Preserve the inherited source-only BPE dictionary and the first12 entries
  in **greedy insertion order** returned by `joint_select` at lines243–306.
  Do not reinterpret “first” as sorted serialization order or reorder by
  target outcomes. This reuses the old selector; it does not claim that its
  old 128-bit selection penalty optimizes the new 32-byte record cost.
- Count case0 from source-list entries0–1 (`AB,BA`), case1 from entries2–3
  (`CD,DC`). Case grouping is supplied by life order, not discovered by Netta.
  Recount every immediate alternative, including overlaps, exactly as before.
  Do not count only the suffix that happened to complete a BPE rule.
- Source provenance lives in non-predictive metadata. No component label,
  recipient map, target regime, or future routing weight is portable input.

## Causal recipient law

Match the same longest stored prefix using only completed prior events. Let
its index be j, current repeat heads be `x_1..x_k`, and
`M=Σ_{r=1..k} P0(x_r)`. For source case b:

```
Q_b(x_r) = M * (c[j,b,r] + 1/2) / (Σ_{s=1..k} c[j,b,s] + k/2)
Q_b(x)   = P0(x)                       for every NEW byte
```

If no record matches, k=0, or that case has zero valid-repeat support,
its complete quote is P0. The seven source slots retain NEW in the archive;
the repeat projection does not replace the local NEW mass. Set the exact
equal/NEW branches directly to P0 in implementation.

For each stored record keep weights of `Q_0=P0,Q_A,Q_B`, initially
`pi=(1/8,7/16,7/16)`. Quote `S=Σ_i w[j,i] Q_i` before observing the byte.
After charging it, update only that visited record:

```
posterior_i = w[j,i] * Q_i(observed) / S(observed)
w_next[j,i] = (1−rho)*posterior_i + rho*pi_i,  rho=2^-10
```

This is turn5's existing prior/rate. The clock ticks on matched-record visits,
including NEW, which contributes no likelihood preference but does execute
the inherited revision step. No match means no inner update. Inner learning
also runs before outer admission. Recipient weights start afresh each life.
The selected implementation stores three probability weights per record,
so its router costs at most **288 bytes** for12 records, separate from portable
bytes, dictionary caches, local learner, vectors, and outer state. Two log
ratios could encode the same information in192 bytes, but are not the chosen
representation and must not be reported as its allocation.

Retain one prospective32-bit admission and the existing slow absorbing-cold
outer HMM for every arm. Each arm earns admission on its own candidate;
forcing the bank to inherit pooled admission would change the test. Only
source contents and their local use change, not the authority law.

For a fixed best expert b_j per visited record, the candidate's log loss is
at most that expert allocation's loss plus
`Σ_j -log2(pi[b_j]) + (N_matched−J_visited)*[-log2(1−rho)]`.
This follows by retaining each row's no-switch path in its fixed-share
mixture. Component prices may adapt causally through P0. The bound concerns
candidate prices; it does not imply that live gain improves. The unchanged
outer cold path still gives whole-prefix loss≤1 bit and interval loss≤16
bits against P0, independent of the candidate's local mixture.

## One fresh experiment and comparisons

Use the same total four source lives. Retain intact recombination, moved
surface, complete law change, and unrelated regimes. Add one partial-change
recipient: after the seam replace the A/B command laws while retaining C/D,
with the same source-independent recipient emission process. All hypotheses,
replacement construction and seed namespace must precede new bytes.

Compare on exactly the same fresh sources and recipients:

1. Local three-way episode bank above.
2. Global three-way bank: same archive, experts, prior and revision, one
   weight vector updated on every matched-record visit.
3. Pooled12: same selected contexts, counts summed, inherited candidate.
4. Pooled24: full original archive under the same528-byte cap.
5. Permuted local bank: one joint deterministic permutation of repeat slots
   in both channels, retaining NEW and the original context addresses.

Pooled12 isolates the loss of coverage; it is smaller and is not the equal
portable-budget incumbent. Global/permuted consume the complete bank.
Pooled24 prices the real exchange of breadth for distinct histories. The
existing compact flat arm can remain a reported528-byte comparator; a claim
of advantage over flat memory requires that actual comparison.

Recommended material claim: **earlier received reuse at the same portable
budget**, plus selective survival under partial change. Require positive
early gain over P0 and a paired first4096 mean advantage >1 bit over
pooled24, with at least5/8 strict wins. Require partial-tail advantage >1 bit
over both global and pooled24, again with at least5/8 strict wins. Compare
the permutation and retain full-change/unrelated losses explicitly. Root
must freeze the exact complete gate before data; these are proposed bars.

Show actual matched histories, two source count arrays, pretruth weights,
and candidate/live probabilities where one case helps and the other hurts.
Generator CD regions may support descriptive diagnosis, but retained CD
commands do not guarantee identical HEAD/BPE contexts after AB changes.
They are not an oracle declaring which archive channel must win.

## What a failure or success would resolve

Twelve prefixes may lose too much coverage to beat pooled24. A single prefix
may conceal two contexts requiring different cases; its router would then
average or lag. Both remembered cases can be wrong. A globally withdrawn
outer candidate can still suppress locally useful fragments. The local
router therefore does not eliminate global suppression by construction;
that is why received partial-tail prices are essential.

A positive result would establish useful preservation and selection of
distinct episode continuations at fixed portable cost. Coordinates still
require exact completed recency-role prefixes. It would leave approximate
functional recognition, automatic grouping of previous lives, and a
fifty-life scaling curve as separate questions. It directly advances the
traveller's use of pieces of different previous cities without claiming the
entire51-city objective complete.
