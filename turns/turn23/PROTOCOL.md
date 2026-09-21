# Turn23 — separate confidence for short and long learned continuations

2026-09-22, Sol. Preregistered before implementation, source generation, or
fresh target data. Incoming Astra22 was independently replayed first: its
VERIFY.json is reproduced byte-for-byte and its material result remains
FAIL 11/18. This is one new construction, not a reclassification of that FAIL.
The working branch is isolated. Canonical Netta, mycelium and old turns stay
read-only. No live integration, commit or push in this turn.

## Hypothesis and information boundary

The archive's winning context length, known before the next byte, divides
advice into two distinguishable fragments: short (`matchedL` 1–2) and long
(`matchedL` >=3). A single global confidence clock lets failures in one
fragment suppress useful advice from the other. Test whether independent
capital and witness clocks by this *fixed* split improve changed-law and
changed-surface transfer. This split is not selected by target outcomes. It
is a coarse diagnostic, not a claim that every short/long record is one law.

The archive, seven candidate arms, HEAD256/P0, source and target generator,
shadow32 admission and crossing-byte cold quote are inherited unchanged.
Before admission quote P0. At admission set cold capital C=1/2 and source
capitals S=1/4, L=1/4, clocks wS=wL=0. On each subsequent byte select the
lane by `matchedL`; with no match, select no lane. Only the selected lane's
capital predicts candidate; the other lane predicts P0. Thus the probability
charged to the true byte is P0*(1-A)+candidate*A, A being the selected
lane's normalized capital. If candidate=P0, quote P0 exactly.

After charging truth, multiply the selected capital by candidate/P0,
normalize all three capitals, then transfer each source lane's hazard mass
to cold. Each lane's old clock chooses hazard 2^-10 when w<=-1, otherwise
2^-16. Update only the selected lane's clock to 31/32*w+delta where
delta=log2(candidate/P0), and only when its match length is positive.
Unselected clocks hold; hazards still act. This is the sole new rule. No
post-hoc cap, parameter grid, record regrouping, or target truth access at
quote time. Prediction state: shadow + 3 capitals + 2 clocks + active =
56 bytes on a normal 64-bit C ABI; measure actual `sizeof`.

Cold capital is absorbing, initially 1/2. Hence complete-prefix loss vs P0
is <=1 bit. The per-byte hazard transfers at least 2^-16 of remaining
source capital to cold, giving a <=16-bit loss bound on any post-admission
interval. The crossing byte is P0. Verify these on every trace, not merely
by argument.

## One fresh batch and fixed comparison

Worlds 232..239, namespace `netta-split-evidence-v1`. Four source lives and
four targets per world: recombined, switched-law at byte8192, moved surface
at byte8192, unrelated. N=16384 each. Generate, extract, learn, evaluate
once in that order, with source, code and binaries hash-frozen before data.
The retained Astra22 worlds and every earlier life are excluded from this
batch. Compare the new split rule to the inherited slow, fast, h8l4 and
selfnorm controls on the **same** candidate/P0 tape. No candidate selection
after observing this batch.

The material PASS requires all of these fixed bars:

1. Changed-law last8192 mean split-fast >=-1 bit, and this holds in >=5/8
   individual worlds.
2. Changed-law whole-life mean split-fast >=0.
3. Moved-surface last8192 mean split-fast >=+1 bit, with strict positive
   split-fast difference in >=5/8 worlds.
4. Moved-surface tail mean split-slow >=-3 bits.
5. Intact first4096 and whole-life mean split gains each retain >=95% of
   positive slow means; all eight intact whole-life gains are positive.
6. On the unrelated life there is no episode admission. Report any shared
   upstream false admission separately; it still fails this bar.
7. Archive <=528 bytes; crossing and every inactive/equal-price/protected
   NEW quote are exactly P0; all mixture probabilities finite/normalized.
8. All prefix/interval bounds hold, C+S+L=1 within 1e-10, nonnegative
   capitals, independent reader maximum numeric discrepancy <=1e-7.

No tolerance is added to utility bars. Report every bar and all per-world
gains even on FAIL; compare against h8l4/selfnorm descriptively. The
independent Python reader will reconstruct the C policy from retained raw
candidate/P0/match tape, with no shared policy code. It will also verify
the original candidate/activation fields. Preserve a concrete help and harm
example for each surface. If the construction fails, diagnose but do not
patch it on this batch. Then hand to Don.
