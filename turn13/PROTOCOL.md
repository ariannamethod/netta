# Joint source allocation of continuation memory

Astra, 2026-09-19. Oleg explicitly retained the next hand with Astra after
turn12. One new source-allocation mechanism; previous evidence stays sealed.
Written before implementation and before fresh worlds are generated.

## Question

Can a fixed-size memory retain complementary broad and specific relations
by scoring each addition against the records already selected, instead of
ranking each record's information in isolation?

Turn12's information selection helped early transfer, but its complete
17-condition gate failed on changed tails. Its saved-price diagnosis found
useful omitted short prefixes. That observation does not prescribe particular
prefixes to retain and is not a target dataset for choosing a new selection.

## Exactly what changes

Keep the inherited source BPE grammar and distinct proper-prefix candidate
pool, immediate-continuation counts, exact uint16 records, 528-byte cap,
longest-stored-match prediction, KT redistribution of repeat mass, protected
NEW, local recipient, prospective admission32 and outer HMM hazard2^-16.
Keep the same raw-byte generator, noise, source/target lengths and families.
No recipient archive update, new authority law, new prefix proposal or
representation change. The only learned mechanism changes source selection.

An extra `isolated` arm retains turn12's information selection on the same
source tapes, so its difference from the new `episode` arm is directly
measured. The C quote law changes only its array/arm count for this comparison.
All current bindings and P0 are shared by the seven arms.

## Selection rule

Let four separately observed source role tapes be T_j; roles are0..6.
For each candidate content x, recount C_x(r) for every overlapping occurrence
with an immediate successor in the SAME tape. n_x=sum_r C_x(r). Only candidates
with n_x>=16 may be chosen. Let p(r) be unconditional frequency across ALL
source events, including NEW/incomplete0 and each life's first events.

For this source objective only, the distribution assigned by selected record
x is q_x(r)=C_x(r)/n_x. A source event with no selected matching prefix uses
p(r); otherwise it uses the longest selected matching suffix of completed
source history. Equal-length distinct contents cannot match the same history.
This full-seven-role empirical criterion deliberately preserves turn12's
information definition. It is not the recipient's KT/P0 price, and information
about protected NEW may still fail to become received benefit.

Start with no records selected. At each iteration, for every unselected
eligible x, consider only its observed successor positions where length(x)
is greater than the current winning selected prefix length. Adding x changes
only those positions. Group those positions by incumbent selected-record
index g (or -1 for unconditional) and observed role r. Compute:

    gain(x | selected) = fsum(N[g,r] * log2(q_x(r) / q_g(r))) - 128

Terms are evaluated in ascending g then ascending r. q_-1=p. Selected-record
indices are append order. Counts N are exact integers. Observed terms always
have positive numerator/denominator; zero-count terms are omitted. Use
Python binary64 division/log2 and math.fsum. No score quantization or hidden
epsilon. Ties: gain descending, prefix length descending, content lexical
ascending. The canonical rule owner remains the lowest owning rule ID.

Append the highest positive-gain candidate, update the winning records at its
affected source positions, and repeat until no positive-gain candidate remains
or the inherited capacity floor((528-16-4*rule_count)/16) is reached. No swap,
second greedy start, target adjustment, reserved short slots or parameter sweep.
Serialize the original ALL-occurrence C_x counts, not residual-only counts.
Save the chosen sequence, marginal gains, affected counts, and final source
objective so the independent reader can recount the complete selection path.

The first addition has exactly the isolated information-minus128 objective.
Later additions are credited only for their marginal effect. In particular,
an unselected broad prefix may cover positions left unserved by specific ones.
This is a greedy construction, not a claim of globally optimal memory.

## Arms and portable formats

Order in C and traces:
`episode`, `isolated`, `frequency`, `reverse`, `permuted`, `flat`, `row`.

- episode: new joint source selection on the forward tree.
- isolated: turn12 positive isolated-information-minus128 selection, unchanged.
- frequency: inherited support-frequency ordering, unchanged.
- reverse: recursively swap tree children, recount on actual sources, apply
  the NEW joint selection rule under the same cap. This is an order control.
- permuted: same new episode records/order, preserve C0 and rotate C1..6 left
  one slot as before, without reselection.
- flat: turn12 exact-uint16 independent-context allocation, unchanged; full
  528-byte cap irrespective of episode's actual occupancy.
- row: unchanged pooled877 uint64 counts, 7032 bytes.

Every small arm uses the same hard cap528; report actual occupancy without
padding. Tree format NETEI001:16-byte header,4-byte child-pair rules,
16-byte records (owner uint8, prefix length uint8, seven uint16 counts).
Flat format NETFI001 is unchanged:16-byte header, length byte, context bytes,
seven uint16 counts. No full-rule support is serialized. The proved source
count maximum65532 and all existing load/prediction checks remain unchanged.

## Fresh fixed batch

Worlds128..135, namespace `netta-joint-prefix-v1`. Exactly four16KiB source
lives and three16KiB recipients per world, same generation function bodies.
Source AB/BA/CD/DC, recombined ADBC, unrelated iid command roles, switched
shares recombined's first8192 raw bytes then changes the command law.
Generator component identities/commands are never supplied to the learner.

Freeze this protocol, implementation, independent reader, build and inherited
dependencies before generation. Previous worlds40..47,56..63,64..71,80..87,
96..103,112..119 remain sealed evidence. No alternative rule may be evaluated
on them or selected by the new targets after the first result.

## Gate and observations

Keep turn12's SAME17 material conditions with the NEW episode as candidate:

1. Mean received gain/P0 at4096 >=.005 bit/raw-byte.
2. Positive early gain in >=6/8 recombined worlds.
3. Mean full16384 gain/P0 >=.005 bit/raw-byte.
4. Positive full recombined gain in8/8.
5. Mean whole-switched episode−row >=0 bits.
6. Whole-switched episode beats row in>=5/8.
7. Mean changed8192-tail episode−row >=−1 bit.
8–17. For EACH frequency,row,reverse,permuted,flat: mean early4096
   episode advantage >1 bit AND positive advantage in>=5/8.

All17 are required for this construction to meet the inherited contract.
The `isolated` paired comparison must be shown at every horizon, full life
and tail; a gate PASS alone cannot establish improvement over that predecessor.
Report any lost early/full benefit explicitly. No additional threshold is
introduced after observing results.

Use one independent source/forecast reader, extended from the completed
turn12 reader. It must reconstruct the joint selection, exact seven archives,
matches, source quotes, admission/HMM and all2,752,512 forecasts within1e-7
bits. Inherited HEAD256/P0 inputs retain their documented independence limit.
Normalization1e-8, protected NEW, prefix gain>=−1−1e-7 and interval drawdown
<=16+1e-7 remain mechanism requirements. Verification and utility are distinct.

Show raw help AND harm, every world's1/4/8/16KiB horizons, candidate versus
received prices, all changed tails and null admissions. Source-selected
records can be explained from their saved marginal history. Label posthoc
diagnosis; no causal claim follows from a saved-price partition.

One fixed batch. A failure is preserved with its concrete remaining limit;
a pass ends the step. Keep verification proportional. No commit, push, merge
or live Netta/mouth/mycelium integration. Return the completed hand to Sol.
Partial functional similarity and diverse long-life accumulation remain the
larger goal; this step still studies exact role strings and their reuse.
