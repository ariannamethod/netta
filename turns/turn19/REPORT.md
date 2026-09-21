# Turn19 — a fixed commitment ceiling: protection and recovery cost

2026-09-21, Astra. **Material FAIL: 17/21 conditions pass. Independent
verification PASS.** One preregistered batch, no parameter sweep or repair.

## Incoming audit

Don's turn18 at `5f3970a` reproduces: fresh reader output is byte-identical
to its sealed verification, including its material FAIL17/20. Relocation
`29c384e` contains 368 exact renames, with no content edits. Audit execution
restored the original relative paths in an isolated temporary directory.
The full audit is [INCOMING_READER.md](audit/INCOMING_READER.md).

Two narrative corrections matter for the next question. The response offsets
have median **38.5 bytes**, not 36. The tested clocks do not exclude every
possible rate rule: an ordinary hazard itself bounds achievable source odds
by `(1-h)/h`. The observed roughly six-bit seam gap is a property of those
traces, not an identity for arbitrary histories. Don's evidence is preserved.
No demonstrated numerical or implementation defect required a repair.

## The one implemented change

In [authority.c](authority.c), new mode `AR_CEILING` follows Don's witness
mode and applies `odds = min(odds, 5)` after the observed byte and ordinary
hazard update. Thus the next quote gives source memory at most **32/33**
of the mixture and local P0 at least **1/33**. The cap is fixed from prior
evidence, before the new worlds. It adds no learned parameter or state field.

The 528-byte source archive, seven candidate arms, HEAD256 frontend, P0,
initial shadow32 admission, witness evidence and choice of hazard are
unchanged. The five-mode comparison uses the same candidate/cold price tape.
Each mode retains 32 bytes of prediction state; comparison state totals160.
The incoming `episode` executable was rebuilt byte-for-byte identically.

Clipping transfers source capital into absorbing cold capital. The initial
one-bit prefix bound survives. The cold floor bounds loss on **any interval**
by `log2(33) = 5.044394119` bits. The derivation and its assumptions are in
[CAP_DESIGN.md](audit/CAP_DESIGN.md). This guarantee is relative to P0;
it does not promise constant regret against the uncapped memory.

## Fresh measurement

Worlds200–207, namespace `netta-commitment-ceiling-v1`: four source lives
and four target regimes per world, each16,384 bytes. Changed targets share
the first8,192 bytes with the intact target. Source code, reader, protocol
and binaries were frozen before generation. The five stage receipts in
`logs/` all report direct exit0.

Mean differences in charged gain **ceiling minus witness**, in bits:

| Target | First8,192 bytes | Final8,192 bytes | Whole16,384 bytes |
| --- | ---: | ---: | ---: |
| Intact recombination | -23.344413 | -33.033599 | -56.378012 |
| Surface renamed | -23.344413 | -11.120039 | -34.464452 |
| Law changed | -23.344413 | +6.061269 | -17.283144 |
| Unrelated | 0 | 0 | 0 |

The law-tail improvement is positive in **8/8 worlds**, including an average
**+1.075716 bits over fixed fast** and **+6.315823 over fixed slow**. Ceiling's
law tail gains18.716522 bits against P0 in mean. This is a local success
inside the full failed experiment, not a better whole-life result.

Intact early and whole-life gains retain **98.0525% / 97.9615%** of slow;
all eight intact lives remain positive. The full material failures are:

| Failed condition | Measured | Preregistered requirement |
| --- | ---: | ---: |
| Surface tail ceiling−fast mean | -5.631177 bits | >1 |
| Surface tail ceiling−fast wins | 3/8 | >=5/8 |
| Surface tail ceiling−slow mean | -12.405970 bits | >=-3 |
| Surface tails with <=25% fast-clock bytes | 5/8 | >=6/8 |

The last failure is **identical in witness and ceiling**: the cap did not
alter the clock. Worlds202,206,207 exceed the fast-share threshold in both.
The law response condition passes7/8; the offsets are
`[24,15,137,560,54,77,95,15]`. There is no unrelated episode admission.
All exact NEW/equal-price conditions pass.

The cap clips on every intact life, between1,944 and4,891 times. Maximum
ceiling drawdown is **5.044273012** bits, within the analytic bound;
minimum complete-prefix gain is **-0.686860738** bits. All cap and clock
identity checks pass. Full per-world numbers: [TABLES.md](TABLES.md).

## The phenomenon in charged bytes

Log2 probabilities on observed bytes; less negative means more probability.
`Delta` is the actual ceiling−witness saving on that byte, not candidate advice.

| World/regime/index | Byte | Match length | P0 | Candidate | Witness | Ceiling | Delta bits |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 206/law/8803 | 139 | 3 | -1.655209 | -5.866547 | -5.322522 | -1.791887 | +3.530635 |
| 204/law/9809 | 64 | 3 | -7.129508 | -1.189589 | -2.598170 | -6.004224 | -3.406053 |
| 205/surface/14773 | 18 | 1 | -3.308345 | -15.358620 | -14.056753 | -7.179244 | +6.877509 |
| 201/surface/14709 | 227 | 9 | -5.926550 | -1.364209 | -1.839841 | -5.746245 | -3.906403 |

The last event is useful remembered structure with a nine-unit match, yet
ceiling's prior source odds are already **-7.400599** bits against witness's
**+1.268995**. There is no clipping on that byte. Earlier, at14638, the cap
saved5.307396 bits on wrong advice. The later good advice then starts with
much less influence. This identifies a recovery cost in addition to the
small cost repeatedly paid while useful source mass is at the ceiling.
All16 changed-life help/harm pairs and exact state fields are in
[RAW.md](RAW.md); trace accounting is in [DIAGNOSIS.md](DIAGNOSIS.md).

## Independent verification and boundary

A separately extended Decimal probability-space reader reconstructs source
books, seven candidate arms, matched lengths and all five authority modes:

- 3,670,016 candidate forecasts plus2,621,440 authority forecasts;
- every quoted price, update, clip, hazard choice and evidence value;
- all21 material booleans and all retained artifact digests;
- maximum numeric discrepancy **1.4267698134062812e-10 bits**;
- maximum normalization discrepancy **1.354472090042691e-14**.

Its explicit inherited boundary is HEAD256/P0 input from retained traces;
full256-vector normalization receipts come from C. Before fresh data, a
separate33-event fixture agreed over1,650 fields to1.07e-14. No second
experimental batch was needed. `RESULT.json` preserves its historical
`independent_reader_pending` flag; completed verification is in `VERIFY.json`.

This is synthetic structural transfer and authority measurement. It does
not establish natural-text semantics, recognition of approximately similar
objects, or progressive improvement over fifty lives. The cap does not
increase the archive's amount of experience. It tests how much influence
that experience receives while another part of its law becomes unreliable.

## Return the hand

Keep the failed five-bit construction and all its help/harm cases. The next
question for Sol is how a bounded memory influence can recover after new
evidence of usefulness, without charging the same permanent ceiling cost to
every useful continuation. [DIAGNOSIS.md](DIAGNOSIS.md) gives the concrete
trace boundary for selecting one prospective rule. No alternative cap,
clock, return or next batch was tried in this turn.

Work is local on `astra/turn19-commitment-ceiling`, based on29c384e, under
`turns/turn19/`. No commit or push was requested for this turn. Canonical
Netta, its mouth and mycelium, Don's checkout and netta.code were not edited.
Rotation remains **Sol -> Don -> Astra -> Sol**; the hand returns to Sol.

## Identities

| Artifact | SHA256 |
| --- | --- |
| PROTOCOL.md | `e00991a388a12b6ee3dbc803f1de613752c141944ed5ee30fe46ead7de53f71b` |
| FREEZE.json | `f2248eaf6877cb4b20c410a076f6300a90353fece28811d3aac43868ad9586d7` |
| RESULT.json | `527d9d00dd729b0241814b67a1207d7afb2a146c844678a1c26fb16b42eb4991` |
| VERIFY.json | `915b19663f3f4dbe8eecefd707eec0c0ac7c833d84b2ec60e29999ffadee3d7d` |

Reproduction and retained-data locations: [README.md](README.md).

## Publication update — 2026-09-21

After the completed local handoff, Oleg explicitly requested commit and push,
with this turn kept under `turns/turn19/`. Publication preserves the frozen
code, original FAIL17/21 and raw evidence. The exact published commit is
recorded in the shared handoff after the push. No new experiment is part of
this publication.
