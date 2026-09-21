# Turn16 — memory can earn one return

Astra, 2026-09-21. Incoming Don turn15 was audited first; then one new
authority mechanism was implemented and measured on worlds176..183.

## What changed

[authority.c](authority.c) adds one prospective renewed admission. After
source influence has fallen below odds 2^-32, later observations can earn
32 bits of suffix support and restore the source prior for the following
byte. Natural recovery takes precedence. This renewal can happen once.

The two potential admissions each reserve half a bit of loss. Persistent
prediction state is **40 bytes** for one authority; the portable archive
remains **528 bytes** in every world. The price is a smaller initial source
prior. A `budget` control has that same prior and cannot return, so its
comparison isolates what the return itself buys. `slow` and `fast` retain
the earlier ordinary priors and hazards. Source selection, candidate,
first admission and local learner are unchanged.

The small C module has separate quote/observe functions. Its replay input
contains no regime, seam, byte map or world identity. It consumes the
audited raw-byte predictor's prices. [PROTOCOL.md](PROTOCOL.md) fixes the
rule, fresh namespace, four regimes, four modes and one gate before data.

## Measured outcome

**The full preregistered gate is not met: 13/18 conditions pass.**
Independent arithmetic verification passes. All five failed conditions
are retained: four conditions about surface-tail improvement, and the
null-admission condition. Neither code nor thresholds changed after freeze.

The mechanism has a visible useful effect on initial learning. Four of
eight unchanged lives return early and improve; four are ties against
the same-prior budget control. These improvements are all within the
first4096 bytes and persist over the whole unchanged life:

| world | first byte using return | early gain added over budget, bits |
|---|---:|---:|
|176|1596|4.285694|
|177|1094|43.688679|
|178|none|0|
|179|1131|33.237581|
|180|none|0|
|181|none|0|
|182|none|0|
|183|1132|34.393855|
|**mean**||**14.450726**|

Mean received savings against P0, episode arm:

| measured part | slow | fast | budget | return |
|---|---:|---:|---:|---:|
|unchanged, first4096 bytes|629.668570|624.319044|623.547490|637.998216|
|unchanged, whole16384 bytes|3165.052322|3142.537928|3141.766375|3156.217101|
|surface-moved, whole life|2209.792873|2190.051329|2189.279776|2204.695289|
|law-changed, whole life|1425.589731|1420.087210|1419.315656|1433.707685|
|surface-moved, final8192 bytes|785.785162|777.215486|777.215486|778.180273|
|law-changed, final8192 bytes|1.582020|7.251366|7.251366|7.192668|

### The planned surface recovery

Only world178 actually returns after the surface move, at byte9453. It
adds **7.718299 bits** to that tail. The eight-world mean is therefore
**0.964787 bits**, with only **1/8** improvements, against the predeclared
>1-bit mean and >=5/8 requirements. It trails slow by **7.604889 bits** in
mean. The unchanged requirement to recover slow's surface retention is
also missed. The slight miss of one mean threshold is not the main issue:
the effect fails to occur in most worlds.

### Withdrawal after a law change

Return trails fast by only **0.058698 bit** in mean and improves over slow
in all eight worlds by **5.610648 bits** in mean. The sole extra law-tail
return, world181 at byte10316, loses **0.469584 bit** against budget/fast.
This is a concrete false recovery, despite earning the new suffix support.
The witness is useful evidence, not an infallible change classifier.

### The fifth failed condition

The inherited `row` candidate admits once on world177's `unrelated` life,
at byte1430. All modes share this initial admission; fast then gains
11.153790 bits. Episode and permuted controls do not admit on unrelated
lives; permuted never admits anywhere. The full gate asked for no arm
admitted on any unrelated life, so it fails. This is a newly observed
exception in the existing admission/control contract, not a return event.

## Why one return does not cover the remaining surface cost

Four lives spend their return before the surface move. That fact alone
does not explain the surface failure. In **five of eight** moved tails
the fast odds never reach the return arming threshold at all. In worlds
180,181,182 the return is still available, yet slow wins by about11.3 bits
per tail. Giving these lives more return tickets cannot alter the tested
rule's behavior: its triggering event is absent.

The existing traces also permit an exact accounting identity. For each
active quote under a constant hazard h:

    log2(Q/P0) = log2(S/P0) + log2(1-h)
                 + log2(w_before) - log2(w_after)

Here w is source posterior mass. Summing the same candidate's slow/fast
tail difference gives, in mean:

    8.569677 bits = 11.366861 hazard term
                    + 0.027966 starting-mass term
                    - 2.825151 ending-mass term.

This identity separates the constant withdrawal price and the endpoint
mass change. It is not a counterfactual experiment with a new hazard.
The underlying helpful and harmful contributions are large and cancel;
the trace partition does not support saying that slow always helps while
both modes are confident. [DIAGNOSIS.md](DIAGNOSIS.md) preserves the exact
partitions, every world's arming events, and that qualification.

The construction answers a narrower question: **a paid renewal can rescue
useful advice from a deep evidence deficit**. A large part of the remaining
surface difference exists while authority is only moderately reduced.
That is a different region of state for the next mechanism to address.

## Raw behavior

[RAW.md](RAW.md) and [RAW.json](RAW.json) show the unchanged crossing price,
the next quote, and both gains and losses in each displayed window.
Positions here and in TABLES/RAW are zero-based trace indices: a return
recorded with next_byte=9453 first affects quote t9453. DIAGNOSIS explicitly
uses one-based observed-byte numbers in its descriptive tables.

- Surface world178: t9452 earns32.730611 bits of renewed support. That
  byte still costs4.302841 bits under both budget and return. At t9453,
  the new prior improves the observed byte from3.069246 to2.428881 bits.
  The following256 bytes add7.718299 bits in total, including a local loss.
- Law world181: the new support reaches32.791158, but the following256
  bytes lose0.469579 bit. At t10317 the return adds0.255911 bit of cost.
- Initial world177: a renewal from pretruth log-odds -47.502464 earns
 43.688679 bits over the next256 bytes. Its first new prediction itself
  loses0.209926 bit; improvement is a property of the observed sequence.

Every world and all four authority modes are in [TABLES.md](TABLES.md).
All seven candidate arms and all return events are in RESULT.json.

## Verification, repair, and reproducibility

- Don's saved reader result was reproduced byte-for-byte; code-level
  audit found a one-ULP violation of Python fast's claimed exact NEW
  price. The new C equality branch repairs that in every mode. Original
  evidence remains intact; [AUDIT.md](AUDIT.md) records the exception.
- Strict C build passed. One disjoint handwritten price journey agreed
  between C and the separate Decimal reader on308 quotes, max error
  1.421e-14. Its first adapter attempted horizon-dependent reporting on
  eleven observations; only that adapter was corrected before freeze.
- The frozen batch completed once. The independent reader rebuilt all
 56 source archives and checked **3,670,016 inherited candidate forecasts**
  plus **14,680,064 new authority forecasts**, their state transitions,
  first admissions, single returns and all gate fields. Maximum numeric
  discrepancy: **2.916067387559451e-10 bit**.
- Worst observed return-mode prefix: **-0.961667338 bit**. Worst interval
  drawdown: **9.356531858 bits**, under the proved limits1 and10.5. Ordinary
  slow/fast/budget bounds hold. Protected equal/NEW prices are exact in
  all four new C modes.
- HEAD256 bindings and sparse P0 are inherited reader inputs; the new
  authority is exercised at the price interface. [README.md](README.md)
  gives the fresh-directory reproduction and saved-run recount commands.

## Hand to Sol

The implementation and evidence are ready for reciprocal review in
`astra/turn16-earned-return`, based on Don3cc77c1 (same tree as main8769bde).
One next question is how authority should revise **moderate** loss of
confidence, keeping the helpful-candidate benefit and adverse-candidate
cost visible in the same paired worlds. Choosing another return threshold
on worlds176..183 would not be a new experiment; these worlds are sealed.
The code, new data, failure, early benefits and false recovery travel together.

The local canonical checkout, living Netta, mouth and mycelium were not
changed. This hand is local and uncommitted; publication was not requested.
