# Turn19: a five-bit ceiling on memory's commitment

2026-09-21, Astra. Written before implementation and before new worlds.
Incoming: Don turn18 at 5f3970a, followed by the byte-preserving relocation
29c384e (368 exact renames). Work is isolated on astra/turn19-commitment-ceiling.
Original artifacts remain sealed. Audit replay uses their original layout.
New work lives under turns/turn19. Rotation: Sol -> Don -> Astra -> Sol.

## One question and one change

Can an upper bound on source commitment preserve useful transfer through a
surface change while limiting its cost after a law change? Don's measured
witness clock distinguishes those cases but retains excessive source mass.
His eight worlds establish this limitation for the tested rules; they do
not exclude every possible rate rule. This experiment tests one fixed cap,
not a search over caps and not a verdict on an entire mechanism class.

Keep the joint-prefix source archive, selection, HEAD256 frontend, local P0,
candidate, shadow32 initial admission and witness clock exactly as inherited.
Add mode `ceiling` beside `slow`, `fast`, `adaptive`, `witness` on the SAME
candidate/cold tape. The ceiling mode follows witness with one addition:

    quote using current z (log2 source/cold odds)
    charge observed byte under that quote
    delta = candidate_log2 - cold_log2
    h = 2^-10 if OLD evidence <= -1, otherwise 2^-16
    z_hazard = log2(1-h) - log2(2^(-z-delta) + h)
    z_next = min(z_hazard, 5)
    if matchedL >= 1: evidence_next = (31/32)*evidence + delta
    else: evidence_next = evidence

The admission-crossing byte stays cold. Admission initializes z=evidence=0
for the following byte; it does not update the clock. Equal candidate/cold
and protected NEW quotes stay bitwise equal to cold. No seam, world label,
regime, surface map or future observation enters the module. No return,
lower floor, new learned statistic or source selection is added.

Five bits means source mass at most 32/33, cold mass at least 1/33. It is
fixed from the prior turn's ~4.975-bit mean fast commitment at the seam,
rounded to a simple five-bit cap. That mean varies across worlds; it is
not a universal equilibrium or a predicted pass. No candidate was run on
sealed data to choose C. No new-world parameter scan or replacement batch.

Clipping moves excess source mass into absorbing cold mass. It therefore
preserves the initial half-cold capital argument (prefix gain >= -1 bit).
The cold floor additionally bounds EVERY interval loss by log2(33), about
5.044394119 bits, for ceiling. Other modes retain their inherited bounds.
During zero-delta slow-clock silence the source mass decreases as
v_n = v_0*(1-2^-16)^n. Capping does not repeat while mass is decreasing.
Evidence remains fixed in silence; if already fast, the clock stays fast.
Persistent state remains 32 bytes per mode; five-mode replay uses 160 bytes
apart from measurement counters. Portable source archive remains <=528 B.

## Fresh fixed batch

Worlds 200..207; namespace `netta-commitment-ceiling-v1`; four independent
source lives of 16,384 bytes per world; targets recombined, switched,
moved_mid and unrelated, all 16,384 bytes. Inherited generators supply the
two seams at byte index8192. Worlds of all previous turns are evidence only.
Record all hashes and freeze protocol, code, inherited dependencies and
binaries before generate. Run generate -> extract -> learn -> evaluate once.
Technical repairs preserve the interrupted evidence and explain their scope;
no threshold or hypothesis change follows material results.

## One material gate, 21 conditions

Gains are log2 probability improvements over the same P0, in bits. Positive
paired differences are wins; ties are not. Replace turn18's tested mode by
ceiling and compare its law-tail improvement with the immediate predecessor
witness. Keep the earlier numerical utility thresholds. Report adaptive too.

1. Moved final8192: mean ceiling-fast >1 bit; wins >=5/8;
   mean ceiling-slow >=-3 bits. (3 conditions)
2. Switched final8192: mean ceiling-fast >=-1 bit;
   mean ceiling-slow >1 bit and wins >=5/8. (3)
3. Switched final8192: mean ceiling-witness >1 bit. (1)
4. Recombined: positive slow reference means; retain >=95% of slow's
   early4096 and whole-life mean gains; whole-life ceiling gain >0 in8/8. (3)
5. Ceiling first fast-clock offset after seam <256 in >=6/8 switched lives. (1)
6. Ceiling fast-clock share <=25% in >=6/8 moved tails. (1)
7. Shared initial admission across five modes; no episode admission on any
   unrelated life; archive <=528 B; prefix gain >=-1-1e-7 in all modes;
   drawdown <=log2(33)+1e-7 for ceiling, <=10+1e-7 fast, <=16+1e-7 others;
   positive normalized candidate/mixture distributions. (6)
8. Every inactive/equal-price quote exactly cold; every protected NEW quote
   exactly cold, independently checked with strict equality. (2)
9. The cap actually clips in all eight recombined lives, every ceiling
   prequote/postupdate odds <=5, and its clock and admission are exactly
   the witness clock and admission on every observation. (1)

An independent reader must reproduce source books, seven candidate arms,
matches and all five authority streams with maximum error <=1e-7 bit and
agree on all 21 material booleans. It uses mass-space arithmetic and its own
gate calculation. Its inherited limit is explicit: HEAD256 bindings and
P0 arrive from retained traces; candidate construction and new authority are
reconstructed independently. A verifier error is not a material improvement.

## Inspectable output and handoff

Keep per-world early, whole-life and tail gains for every mode; per-life
prefix minima, drawdown, admission, clip count, odds at seam and clock share.
Show exact charged bytes for the largest ceiling-witness help and harm in
each changed life, with truth byte, match, cold/candidate/live prices, prior
odds and following odds. Show the aggregate price on unchanged lives beside
any tail improvement. Do not call a tail-only gain a better whole life.

After one result, diagnose its actual limitations from preserved traces,
hand the result and one bounded open question to Sol, then stop. No live
Netta, mouth, mycelium or netta.code integration; publication only under
Oleg's explicit authority. The Python models suggest studying where acquired
experience can affect a choice; this hand tests only the C authority ceiling.
