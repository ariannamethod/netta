# Joint selection: stronger transfer at the same memory size

Astra, 2026-09-19. This additional hand was explicitly requested by Oleg
after turn12. The prior audit and sealed result were received by hash;
this hand implements a new source-selection mechanism and runs one fresh batch.

**The inherited 17-condition gate passes. Independent verification passes.**
In the direct paired comparison, joint selection increases full recombined
gain over the predecessor in all eight worlds, at the same 528 portable
bytes. Changed tails remain a real tradeoff: six of eight worsen against
that predecessor. This is a measured transfer improvement, with that limit.

## The implementation

[experiment.py:243](experiment.py) adds `joint_select`. Previously, every
record competed on its information score in isolation. The new source
learner adds a record only for its contribution to the memory already
selected. Positions already predicted by an equal or longer stored match
receive no new credit. Other source positions are repriced under the
candidate's empirical conditional distribution, grouped by their previous
winner; exact grouped gain minus128bits determines the next addition.

The greedy procedure stops at the same capacity or when no positive addition
remains. It preserves full occurrence counts, rather than replacing them
with residual counts. All selected records, marginal contributions, affected
source observations and cumulative source gains are retained in BOOKS.
An independently implemented source recount reproduces the selection path.

This source objective uses all seven event roles, including protected NEW.
Its empirical probabilities are distinct from the recipient KT/P0 quote.
Joint selection can improve the final source predictor without guaranteeing
a better changed-world conditional or a globally optimal archive.

Source grammar, prefix candidate pool, archive formats and 528-byte cap
are unchanged. So are the causal HEAD256 frontend, P0 recipient, 32-event
history, longest stored match, protected NEW, prospective32-bit admission
and outer hazard2^-16. The C change adds the `isolated` predecessor slot to
the existing arrays/CLI/traces; its 24 helper functions remain byte-identical.
See [BUILD_C.md](BUILD_C.md), [INHERITANCE.json](INHERITANCE.json).

## One frozen experiment

[PROTOCOL.md](PROTOCOL.md) and [FREEZE.json](FREEZE.json) were fixed before
worlds128–135, namespace `netta-joint-prefix-v1`, were generated. There are
four16KiB source lives and three16KiB target lives per world. Source pairing,
target recombination and noise follow the unchanged generator. The learner
receives emitted raw bytes; hidden commands/component labels are not inputs.

Seven paired arms use the same observations and P0: joint episode, isolated
predecessor, frequency, reversed tree with joint selection, permuted joint
counts, compact flat and the larger7032-byte pooled row. No target was reused
to adjust the learner, and no second candidate or batch was tried.

All forward joint and isolated archives contain32 rules and24 records:
16-byte header +128 rule bytes +384 record bytes = **528 bytes** each.
Between12 and19 selected prefixes are shared per world. The difference
therefore changes the chosen contents, without increasing record count.

## Actual paired result

Values are mean received bits saved relative to the same cold P0. The
predecessor below is relearned on these SAME source tapes and run on the
SAME target bytes; these are not comparisons between two world batches.

| Measurement | Isolated predecessor | Joint selection | Difference | Joint wins |
|---|---:|---:|---:|---:|
| First4096, recombined | 268.575687 | 420.813357 | +152.237670 | 7/8 |
| Full16384, recombined | 1788.349126 | 2579.673905 | +791.324779 | 8/8 |
| Whole switched16384 | 642.595405 | 1024.647910 | +382.052506 | 8/8 |
| Changed final8192 | 4.745151 | 2.221263 | **-2.523889** | **2/8** |

Full recombined saving is44.2489% larger than the predecessor's saving,
not44% better total prediction quality. Joint gain is0.102737636bit/raw-byte
at4096 and0.157450800 over16384. All eight early and full recombined gains
against P0 are positive. The one paired early loss is world128:−21.676390bits;
its full recombined comparison still gains6.879500bits.

Every life, every arm, candidate and received price, admission, loss bound
and1/4/8/16KiB horizon is retained in [TABLES.md](TABLES.md) and RESULT.json.

### Same17 material conditions, all shown

| Condition | Observed | Required | Outcome |
|---|---:|---:|---|
| Early mean gain/byte | .102737636 | ≥.005 | PASS |
| Early positive worlds | 8/8 | ≥6/8 | PASS |
| Full mean gain/byte | .157450800 | ≥.005 | PASS |
| Full positive worlds | 8/8 | 8/8 | PASS |
| Whole switched episode−row | 759.101256bits | ≥0 | PASS |
| Whole switched wins over row | 8/8 | ≥5/8 | PASS |
| Changed-tail episode−row | 2.670542bits | ≥−1 | PASS |
| Early advantage over frequency | 269.462643bits;8/8 | >1;≥5/8 | PASS/PASS |
| Early advantage over row | 232.579931bits;7/8 | >1;≥5/8 | PASS/PASS |
| Early advantage over reverse | 343.932262bits;8/8 | >1;≥5/8 | PASS/PASS |
| Early advantage over permuted | 420.813357bits;8/8 | >1;≥5/8 | PASS/PASS |
| Early advantage over flat | 199.984250bits;8/8 | >1;≥5/8 | PASS/PASS |

No unrelated arm admits in any of eight worlds. Permuted memory never
admits in any of24 lives. These controls test correspondence on this family.

## The tail result must not be misread

This fresh batch also lets the **isolated predecessor** meet the tail-versus-row
condition: its mean advantage is5.194431bits. Therefore turn12's failed tail
condition has not been causally repaired simply because turn13 passes that
condition on different worlds. The direct paired tail comparison is worse
for joint selection. The improvement supported by this intervention is early
and complete-life transfer.

Five joint changed tails still lose to P0. Joint loses to the isolated
predecessor on six tails; world132 is the worst paired case, −35.066543bits.
Full switched lives nevertheless improve in8/8 because the earlier benefit
more than offsets that price. Both parts remain visible.

One concrete missed benefit occurs in world132 at offset8210. The actual
byte is decimal50 (`32` hex), rank3 among heads `[85,170,50,68,81]`, after
role history `11230232312011030122022230020003`. Joint has no stored match
and charges P0's log2 price−4.323263497. The isolated arm's matching two-role
record has counts `[299,1345,2390,1665,0,0,0]`; its received price is
−3.654419832, saving0.668843665bits that joint misses. The source-coverage
criterion still does not guarantee that broad reusable relations survive.

## Raw behavior and memory cost

[RAW.md](RAW.md) exposes actual byte windows, source counts, chosen marginal
scores, current bindings, and cold/candidate/received probabilities. For
example, world128 offset533 observes byte `f0` as rank3. Joint gives
probability.063409232, versus P0.029730903 and isolated.033858186: a received
gain of1.092730bits over P0. At offset140 the same life also shows actual
joint harm of−2.133921bits. These posthoc excerpts explain behavior; the full
traces and all-world outcomes determine the reported result.

The same jointly selected records occupy474–492bytes when written as
explicit strings; their matches and quotes are independently identical.
The current tree encoding still has no measured compression advantage here.
One forward arm uses1568bytes of C array payload for rule/expansion/record
caches. Shared history40, P0 state7064, quote7288, seven outer states448 and
candidate/live vectors28672bytes are recorded in C logs. These are named
payloads, not total runtime RSS; frontend allocation and other storage remain.

## Verification, provenance and return

The one independent reader reconstructs all56 archives and all
**2,752,512 forecasts**, including joint paths and the paired predecessor.
Maximum numeric difference: **1.316494e-10bits**. Maximum normalization
error:1.332268e-14. Worst full-prefix gain:−0.999049bits; maximum observed
drawdown15.218301bits, within the inherited−1/16 bounds. The same17 material
decisions agree. HEAD256 bindings and sparse P0 remain supplied inputs;
[READER.md](READER.md) states that independence boundary.

The previous turn12 files and FAIL remain sealed. The current PASS has its
own protocol, sources, implementation and paired evidence. No numerical
repair, tolerance change or target-driven choice was needed after freeze.
RESULT's historical `independent_reader_pending` field stays as written;
VERIFY.json and VERDICT.json record the completed verification.

The code is ready locally for Sol to audit and then make her own next step.
Reproduction commands are in [README.md](README.md). This hand does not
integrate into live Netta and does not establish approximate functional
similarity or the full51-city learning sequence. The narrower, now measured
advance is greater transferred benefit from the same portable memory budget.

Work remains uncommitted in the existing isolated checkout/branch
`netta-astra-turn12-20260919`, `astra/turn12-information`, under `turn13/`.
No commit, push, merge, mouth or mycelium change. The passed step ends here;
the hand returns to Sol with both the gain and its counterexamples.
