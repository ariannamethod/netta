# A local right to decline both remembered cases

2026-09-16, Sol. Frozen before implementation, generation, or measurement.
This is one isolated step after Astra's BANK2-ROW hand at `672ffc1`.
Canonical Netta, the mouth, mycelium, and the existing turn3 evidence are
read-only. A failure stays a failure; no parameter search on this batch.

## Question and fixed predictor: BANK2-COLD-ROW

Can a recipient retain the useful, row-specific combination of two past
HEAD256 books while using its own P0 prediction on rows where both books are
locally misleading? Keep the two immutable NETBANK1 books, the same recipient
frontend/local learner and support law, and the same joint 32-bit admission
and absorbing-cold HMM with hazard 2^-16. No new byte representation or
source-selection oracle is supplied.

For each complete canonical row p, begin with prior masses
`w(P0)=1/8`, `w(A)=7/16`, `w(B)=7/16`. Store two log2 capitals relative to P0,
`a[p]=b[p]=0`. Before truth, normalize these three prior-weighted capitals
and quote the mixture of P0, P2_A and P2_B. After charging that quote, update
only the observed row:

    a[p] += log2(P2_A(truth)/P0(truth))
    b[p] += log2(P2_B(truth)/P0(truth))

This update runs before and after joint admission, without reading future
bytes. Incomplete contexts return P0 and update nothing. All three branches
have identical NEW probability, so NEW gives no selection evidence. The
resulting three-way quote is one candidate for the unchanged outer HMM.
There is no per-row admission, reset, oracle, or temperature parameter.
Additional recipient state is 406 doubles = 3248 bytes. The bank stays
14080 bytes and the source budget stays four 16-KiB lives = 64 KiB.

The original BANK2-ROW quote, P0, and a three-way mixture with the same
prior but one global capital pair are fixed comparisons. The main paired
control is BANK2-ROW on exactly the same raw bytes and source books. The
global three-way control tests whether local choice, not merely adding P0,
matters. For every arm the current byte must be priced before any posterior
or HMM update. No arm may use hidden generator routes.

## New data and evidence

Use Astra's frozen raw recurrence generator and HEAD256 frontend with world
IDs 40..47, but replace the seed namespace by `netta-cold-v1`. The laws,
source/target lengths, alternate A/B raw mosaic, independently sampled source
trajectories, byte renamings, and three target regimes remain unchanged.
No redraw, world choice, or tuning after inspection. The switched target
shares its first 8192 raw bytes with the mosaic target, then follows an
unrelated grammar. Save hashes, books, raw streams, learned-unit traces, and
C-produced pretruth component-price traces. The frozen turn3 C reader is the
source of P0/P2 component prices; the new mixture and outer HMM are evaluated
separately. An independent reader must reconstruct their state from those
prices rather than importing the new evaluator.

## One declared gate

PASS requires every item below on the new eight-world batch:

1. Exact stream chronology and book identity; pretruth probabilities finite,
   positive and normalized within 1e-8; all NEW bytes equal P0 within 1e-10.
   A separate reader matches every candidate/live price and posterior within
   1e-8 bits. Every full prefix stays above -1-1e-7 bit and every interval
   drawdown stays below 16+1e-7 bits, for both real predictors and the global
   control.
2. On preserved mosaics BANK2-COLD-ROW gains at least 0.0075 bit/raw byte
   on average over P0, remains positive in all eight worlds, and retains at
   least 70% of the paired BANK2-ROW mean gain. These are material thresholds,
   not a claim of superiority to BANK2-ROW on the unchanged mosaic.
3. On the changed target's last 8192 bytes, the new arm improves the paired
   mean tail gain over BANK2-ROW by more than 1 bit per world, improves at
   least five of eight individual tails, and improves the worst tail by more
   than 1 bit. A gain here must not be bought by violating item 2 or the
   prefix bounds.

Report 1/4/8/16-KiB mosaic gains, admission times, posterior cold weights on
useful and harmful rows, all eight paired changed tails, unrelated behavior,
first-half identity, minimum prefix, maximum drawdown, and a concrete event
where both books were worse than P0. Preserve negative results and include
the first mechanistic explanation if the gate fails. Nothing here authorizes
integration with live Netta or an inference about semantic similarity,
natural-language speech, automatic case grouping, or fifty-life learning.
