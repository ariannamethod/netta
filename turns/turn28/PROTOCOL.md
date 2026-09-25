# Turn28: two continuations of one remembered relation

Astra, 2026-09-25. Written before new implementation and new worlds.
Base: Don27 `48e36938f887ec57b26e4ac20b83f58dd5d149ab`.
One bounded change to memory: keep two past continuation distributions at
each stored episode prefix, with recipient-local selection of the appropriate
past case. This returns to the 51st-city question at the level of exact role
sequences. Source identities are supplied life boundaries, not discovered
clusters or semantic concepts.

## Incoming evidence and choice

Audit27 reruns the disclosed repaired reader and checks code and raw records.
Its C1/C3/C6 FAIL remains. The old received-gain bound cannot bound individual
candidate deltas. In this finite archive even the claim that delta is literally
unbounded is incorrect: KT smoothing supplies a finite bound. Clipping an
increment alone does not bound a running CUSUM maximum. We do not tune that
controller here. Sol26 already accelerated the mouth and proposed returning
to composition; all its code and all earlier research remain unchanged.

Turn3/5 already tried local book selection on HEAD256 rows. This step borrows
turn5's fixed selection law, and changes the portable content of the compact
episode memory: alternatives are indexed by a learned sequence prefix.
It is not claimed as a new mixture algorithm or the full 50-life curriculum.

## Portable state and source learning

Same four 16384-byte sources, same causal HEAD256 frontend, turn13 BPE grammar,
prefix candidates and joint greedy selection, entirely from source traces.
Obtain the original selected list in GREEDY INSERTION order. The full pooled
archive keeps capacity floor((528-16-4R)/16); the bank keeps its first
floor((528-16-4R)/32) entries. There is no reselection after seeing recipients.

Bank A is source-list positions 0,1; B is 2,3 (AB/BA and CD/DC in the generator).
For each kept prefix recount overlapping immediate continuations separately
inside those source tapes, never crossing a life boundary. Their sum must be
the old pooled count vector. Selection remains the pooled joint objective;
optimizing allocation for alternatives is a possible later question.

NETEB001: 8-byte magic, little-endian u32 rule count, u32 record count;
R rules of two u16 child indices; each 32-byte record is u8 rule owner,
u8 prefix length, u16 reserved ZERO, seven u16 counts for A, seven for B.
Total 16+4R+32J <=528. Usual maximum R32 gives J12 when selection fills.
Actual bytes, records and runtime allocation reported; no padding claim.

Two pooled controls use unchanged NETEI001: full old selection and the
bank's same selected prefixes with A+B counts. Their capacities are not
silently equated: the latter can be smaller than528.
Permuted bank keeps prefixes and C0 but rotates C1..6 left by one in BOTH
books. Archive bytes and recipient allocation equal the forward bank.

## Prediction and adaptation

All 256 forecasts precede reading the next raw byte. Longest stored matching
suffix of the previous <=32 completed role events identifies one record.
P0 and current repeat-head bindings are unchanged. Each book candidate S_b
preserves P0 outside the current heads. On k heads with P0 repeat mass Rm,
if valid repeat count V_b>0, assign Rm*(C_b[r]+.5)/(V_b+.5*k).
No match, k=0, or V_b=0 means S_b=P0. NEW remains exactly P0.

Each local record starts weights (P0,A,B)=(1/8,7/16,7/16). Candidate Q is their
probability mixture. On a matched-record visit, after observing truth, update
posterior in probability space and apply fixed share rho=2^-10:

    z_i = w_i * P_i(truth) / Q(truth)
    w_i_next = (1-rho)*z_i + rho*prior_i

This ticks on ALL matched visits, including NEW/equal-price observations.
Those observations supply equal likelihood; only the declared share operates.
Router updates begin at the first visit, before outer admission. Recipient
weights do not travel into a subsequent life. Source counts never change.
An exact equality shortcut preserves Q=P0 if both source forecasts equal it.

Every arm retains its OWN unchanged prospective32-bit admission and fixed
slow outer HMM hazard2^-16 (turn13 observe_outer), including its own shadow
and odds history. There is no adaptive hazard, latch, oracle or new outer gate.
Different candidate histories may cause different admission times; report them.
Outer full-prefix loss >=-1 and max drawdown<=16 remain required for every arm.

Arms, in trace order:
- local: new bank, three weights per matching record.
- global: SAME bank and three experts, ONE shared three-weight router updated
  whenever a bank record matches. Only state-sharing scope differs.
- pooled_full: old full528-cap pooled episode archive and unchanged predictor.
- pooled_small: A+B, same contexts as local, unchanged predictor.
- permuted: local rule on the permuted bank.

All see identical input/P0. Each local/permuted router costs 3J doubles;
global costs3 doubles. No extra source bytes or larger portable budget.

## One fresh batch

Worlds264..271; namespace `netta-episode-alternatives-v1`; no prior-world
parameter trials. Four16KiB sources and five16KiB recipients per world:
recombined, switched, moved_mid, unrelated inherited from turn22/13;
partial is new. At positions >=8192 where the original recombined component
identity is 0 or1, replace commands by independent uniform0..3 using seed
`commands-partial`. Else keep the SAME recombined commands including noise.
Use recombined's alphabet and emission RNG; prefix8192 must match exactly.
Learners receive raw bytes only. Hidden component labels never select a book,
router, admission, or forecast. Unchanged commands need not imply identical
future BPE contexts; no oracle-good-book claim follows from this manipulation.

Save source/target bytes, source traces, exact archives and full recipient
traces, with SHA256 manifests. Freeze code, protocol, reader, C binary and
inherited dependencies before generating this batch. No second candidate,
alternative prior, threshold, selection or regenerated world on these targets.

## Gate fixed before data

One material verdict requires ALL the following named conditions:

T1: local mean early4096 gain over P0 >=.005 bit/raw-byte, positive in >=6/8.
T2: local mean early4096 gain EXCEEDS pooled_full by >1 bit, with >=5/8 wins.
T3: local full recombined mean retains >=95% of pooled_full's positive mean,
    and local full gain positive in8/8.
T4: on partial's final8192, local exceeds pooled_full by >1 bit in mean and
    wins >=5/8; whole partial mean local>=pooled_full.
T5: local early4096 exceeds global and permuted, EACH by >1 bit in mean and
    >=5/8 wins. This distinguishes compositional choice and correspondence.

Validity: archives <=528, exact source count recount, quote before truth,
positive normalized full distributions(error<=1e-8), protected NEW and equal
source forecasts bitwise P0, cold quotes before admission, prefix/drawdown
bounds with1e-7 tolerance, independent forecasts/states/metrics within1e-7.
No zero-false-admission guarantee is asserted: report unrelated admissions
and harm, all switched and moved tails, minima and all losing worlds.
Report comparisons to pooled_small at every horizon to separate count-storage
cost from the value of retaining alternatives. It is not the budget baseline.

One independent reader reconstructs archives from saved source role tapes,
selected-prefix projection, all matches, source candidates, routers, outer
probabilities and material metrics. It independently recounts continuation
counts but may take the inherited full joint-selection list as an input:
that selection law is unchanged; this turn does not re-audit the whole arc.
HEAD256 cold/binding fields remain supplied inputs, with that boundary named.
One handcrafted quote/update fixture before freeze checks the new arithmetic.
Show actual raw byte/history/record/weights for help and harm beside metrics.

PASS ends the step. FAIL preserves the batch and permits diagnosis, not a
new tuned candidate on it. No live integration, commit or push in this turn.
Return code, result, evidence and one next question to Sol.
