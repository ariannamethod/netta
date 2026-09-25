# Turn28: local choice helps, storing every alternative loses too much coverage

Astra, 2026-09-25. Base `48e36938f887ec57b26e4ac20b83f58dd5d149ab`;
isolated branch `astra/turn28-memory`. Incoming audit plus one implemented
memory intervention, one fresh batch, then return to Sol.

**Independent verification PASS. Material gate FAIL: T2, T3, T4.**
The new local selector beats a single global selector on all eight early
comparisons. But spending twice as much per stored relation halves coverage,
and the result loses to the old archive of the same portable size.

## Incoming turn27 and the C performance change

The repaired Don27 reader was run once against his preserved raw artifacts:
3,145,728 forecasts, max difference 1.19485e-10, new receipt byte-identical
to his sealed VERIFY. All39 freeze pins match. His C1/C3/C6 FAIL stands.
The disclosed repair adds the missing `c7` reporting entry and changes no
forecast or verdict. [Audit and actual false alarms](audit/AUDIT27.md).

Two corrections are supported by the implementation and raw trace:

- Don's aggregate floor -1 indeed cannot bound a witness delta. However this
  finite uint16/KT predictor has a conservative delta floor -19.58495149 bits;
  it is not literally unbounded below. A bounded increment can still cross
  a cumulative threshold after several adverse observations.
- In the cited world263 example the first crossing follows the second large
  negative event, t2092; the third at2098 comes after the alarm. The code's
  prospective timing is correct. See [mechanism analysis](audit/MECHANISM.md).

Sol26's merged optimization is in `netta_mouth.c`: retained pair counts and
a heap replace repeated whole-stream recounting. Her old-C/new-C receipt
reports 16.05x on its fixed primary workload, at +40.59MiB peak RSS; Don's
incoming acceptance confirms output identity. This hand did not repeat that
performance benchmark. It leaves the mouth, canonical Netta and mycelium alone.

## Restriction addressed

The old source learner adds all lives' continuation counts at a prefix in
[turn13/experiment.py](../turn13/experiment.py:208). The recipient therefore
cannot distinguish two past cases which produced different continuations.
The longest-match predictor then uses the sum
[episode.c](../turn13/episode.c:312), with one outer authority per archive.

This hand stores both histories in [build_memory](experiment.py:133), and
adds local three-way selection in [bank.c](bank.c:143). The prior and fixed
share are inherited mathematics from turn5; this is their application to
compact episode prefixes, not a claim to invent local mixtures.

## Exact construction and cost

The same source-only grammar and greedy insertion list supply the addresses.
Source-list entries0/1 form A; entries2/3 form B. Overlapping immediate
continuations are counted inside each source life. No target or hidden
generator label participates in source selection or recipient choice.

One shared32-rule dictionary plus12 paired records:

    header16 + rules128 + records12*32 = 528 portable bytes

Each paired record contains owner/prefix length, reserved zero and two
seven-element uint16 vectors. All eight banks actually occupy528 bytes.
The full pooled control occupies528 bytes with24 records; the same-prefix
pooled control occupies336 with12. Source exposure is identical, four16KiB
lives per world. The source cost is explicit: alternatives displace addresses.

For each matching record, recipient weights choose P0, A or B. Prior is
(1/8,7/16,7/16); posterior update is followed by fixed share2^-10 on visits
to that record, including equal-price/NEW visits. No match does not tick it.
Weights begin learning before the outer gate and reset for each new life.
Source counts remain immutable. Three doubles cost288 recipient bytes for
12 local records; the global control uses24. Full vectors precede input.

Every arm retains its own prospective32-bit admission and fixed slow outer
HMM. There is no new hazard or latch. Protected NEW and equal-source prices
remain exactly P0. [Protocol](PROTOCOL.md), [interface](INTERFACE.md),
[build and allocation](BUILD.md), [code](bank.c).

## Frozen experiment

Worlds264..271, namespace `netta-episode-alternatives-v1`, generated once
after freeze. Five16KiB recipients per world: intact recombination, complete
law change, changed byte surface, unrelated commands, and partial change.
In partial, after8192, commands from components0/1 become independent uniform
draws; components2/3 retain their original commands. The initial8192 raw bytes
remain identical. These labels are generator-only. Preserved commands do not
guarantee preserved downstream BPE contexts.

Five paired arms: local bank, same bank/global router, full pooled archive,
small pooled archive with the bank's contexts, and local/permuted bank.
All outputs, including harm and null admissions, are retained.

## Received result

Means in bits saved against the same local P0; positive is better.

| Observation | Full pooled24 | Small pooled12 | Global bank12 | Local bank12 |
|---|---:|---:|---:|---:|
| Intact first4096 |657.845|369.911|272.845|423.355|
| Intact full16384 |3492.956|1949.241|1264.469|2049.375|
| Partial-change final8192 |490.549|200.644|412.186|528.157|
| Partial-change full16384 |2018.955|1054.744|978.989|1474.810|
| Complete-change final8192 |1.593|14.614|54.838|262.973|
| Moved-surface final8192 |1002.829|503.813|299.608|669.437|
| Unrelated full16384 |0.000|-0.116|2.224|153.895|

[TABLES.md](TABLES.md) contains every arm/world/horizon, admissions and losses.

| Preregistered condition | Outcome |
|---|---|
| T1 positive early transfer |PASS: .10335812 bit/byte, positive8/8|
| T2 early advantage over full528-byte baseline |FAIL: -234.489814 bits, wins0/8|
| T3 retain95% full intact benefit, positive8/8 |FAIL:58.671642% retained; positive8/8|
| T4 partial-tail >1bit, wins>=5/8, whole-life>=baseline |FAIL:+37.607951tail, wins4/8, whole -544.144199|
| T5 beat global/permuted early |PASS:+150.510359/+419.117768 bits; wins8/8 each|

The eight partial-tail differences against the full archive are
`+129.464,+154.498,-204.511,+206.687,-50.522,-185.287,+258.426,-7.892`.
The positive mean does not hide the four losing cases.

Unrelated lives admit local memory in6/8 and gain153.895 bits on average;
two remain exactly P0. Global admits3/8, including one -0.989-bit life.
The smaller pooled control admits once and loses0.927 bits. Full pooled and
permuted admit0/8 there. The generator's unrelated commands still use the
same repeat-role alphabet and emitter; their label does not imply absence
of every reusable statistical property. On intact lives the permuted arm
also obtains260.020 full-life bits. This control is not universally silent.

## What the controlled contrasts establish

The source coverage ablation gives an exact early accounting on the paired
batch:

    local - full = (local - small) + (small - full)
                = +53.443876 - 287.933690 = -234.489814 bits

On full intact lives the corresponding terms are +100.134367 and
-1543.715941. On partial tails they are +327.512946 and -289.904995.
The first term combines distinct source channels AND their local P0 router;
this experiment does not isolate which of those two additions earns it.
Local versus global does isolate the scope of router state on the same bank.

The histories really differ: source-only counts and conditional distribution
contrasts are recorded in [DIAGNOSIS.md](DIAGNOSIS.md). Difference alone does
not establish that storing all those alternatives is worth their bytes.
The new memory may reject an unhelpful source relation while retaining another,
but common outer authority still couples its received influence.

## The phenomenon in actual bytes

[RAW.md](RAW.md) displays histories, counts, weights, bytes and quotes.
At world266:t13978, the short record `23` has P0 weight .98031 and the local
prediction nearly follows the local learner. It saves5.5175 bits against the
full archive's erroneous longer record. That example particularly shows
the local cold option, not proof that book A/B selection caused the gain.

At world267:t15369, local weights give B .993825 at prefix `2000`.
That retained confidence makes the actual byte4.92048 bits more expensive
than the full pooled control. Both the useful choice and its lag are visible.
These are raw-byte predictions, not natural-language generation samples.

## Verification and exact scope

Independent source recount checks2688 counts and reconstructs all four
archives per world. Decimal probability-space replay checks **3,276,800
forecasts**, router states, matches, all metrics and every gate. Max numeric
difference **1.0982148523908108e-10**; all validity checks pass.
Lowest complete-prefix gain -0.999608238; greatest drawdown15.226724256,
within the unchanged -1/16 bounds. No post-freeze repair or retuning occurred.

HEAD256/P0 and inherited source greedy selection are supplied boundaries;
the new counts, archive projection, matching, probabilities and state changes
are independently reconstructed. [READER.md](READER.md), [VERIFY.json](VERIFY.json),
[reproduction](REPRODUCE.md). This does not demonstrate approximate semantic
matching, autonomous grouping of source lives or the50-life learning curve.

## Next hand: Sol

The most informative next comparison is a local **P0 versus pooled-record**
selector on the full24-record archive, keeping528 portable bytes. Collapse
the inherited three-way prior to (1/8,7/8), keep share2^-10 and the same outer
law. When A=B this collapse is mathematically exact. A fresh paired experiment
can determine how much of this turn's benefit needs two stored continuations
and how much needs only locally choosing when to use one. It also tests the
coverage cost directly. This is a proposal for the next hand, not an extra
candidate evaluated here. Sol should audit this result then choose her step.

All new work is under turns/turn28 in the isolated clone. No commit, push,
merge or live integration was performed. Previous evidence remains sealed.

## Identities

- PROTOCOL: `46608fc29be30f9087acefa4795d3495bb6e827f3c2f2760c37bc20c036220f2`
- FREEZE: `2d26262c7f00369f8259e3983dbb234e7cbb765734948294e2a06eb5f74bd869`
- RESULT: `cdbe197b1bc9c50f5a51f994c09659dae68c4bbdc90a74e9117508b230ca83a8`
- VERIFY: `627f29e24b32497b0a255f322e721829dd655f5b98efb0c70876062a4e0e5f81`

## Publication follow-up — 2026-09-25

After receiving the completed local result, Oleg explicitly requested commit
and push, returning the hand to Sol. Publication uses `astra/turn28-memory`;
the exact published commit is recorded in the shared handoff. The preceding
local-completion status records the state before that instruction. All frozen
code, measurements and verdicts remain unchanged. Large raw artifacts stay
at the documented local paths with their committed manifests and reproduction
instructions.
