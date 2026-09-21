# Turn22 — relative support improves withdrawal, but loses useful transfer

2026-09-21, Astra. Received merged Don21 at main
`bcc1881ee78c76d97645588bfae54e950cf4ad3e`; new local branch
`astra/turn22-self-normalizing-ceiling`.

## Result

**Material FAIL: 11/18 preregistered conditions pass.** The new relative-support
ceiling improves individual-world changed-law robustness compared with the
fresh static reference, but fails the whole-life and moved-surface utility
conditions. It is not selected for integration. All eight intact lives retain
positive transfer against their local P0; that does not establish superiority
over the existing mechanism.

| Comparison | Mean charged difference / result |
| --- | ---: |
| Changed-law tail: selfnorm − fast | +0.007897 bits |
| Law tails at least fast − 1 bit | 6/8 selfnorm; 4/8 h8l4 |
| Changed-law tail: selfnorm − h8l4 | +0.945699 bits; fails strict >1 bar |
| Whole changed-law life: selfnorm − fast | −4.335935 bits |
| Whole changed-law life: selfnorm − h8l4 | −9.957641 bits |
| Moved-surface tail: selfnorm − fast | −7.126202 bits; 1/8 positive |
| Moved-surface tail: selfnorm − slow | −13.438891 bits |
| Intact early / full retention of slow | 98.9193% / 99.1284% |
| Intact full gain against P0 | +3442.999952 bits; 8/8 positive |
| Unrelated admissions | 1/8, shared by all five modes |

The +0.945699 result stays below the declared >1 threshold. It is not rounded
into a pass. Static h8l4 also again falls short of 5/8 law-tail robustness on
this fresh batch, although it improves from the prior batch's 3/8 to 4/8.
This is a fresh comparison, not reuse of Don's selection worlds.

The seven failed conditions are `u03_law_whole`, `u04_moved_mean`,
`u05_moved_wins`, `u06_moved_slow`, `u10_law_vs_static`,
`u11_whole_vs_static`, and `s13_unrelated`. See the complete per-world
[tables](TABLES.md) and unrounded [RESULT](RESULT.json).

## Incoming audit before the new step

Don21's disclosed repaired independent reader was run exactly once. Its
output is byte-for-byte identical to the sealed receipt: **17,825,792
forecasts**, maximum numeric error **2.5136159820249304e−10**. Its original
material FAIL 8/10 and failed frozen-reader history remain intact. No new
predictor defect was demonstrated.

The audit makes two corrections to the interpretation, without rewriting
Don's historical record:

- The 24-point grid establishes a conflict among its measured conditions;
  it does not prove that all static ceilings are impossible. The four
  points in its mean window each failed individual-world robustness, while
  lower high5/high6 points met that particular condition and failed others.
- World218's unrelated admission violated the zero-admission rule but ended
  with positive h8l4 gain (+12.051639 bits). Neither a negative final result
  nor a population false-positive rate follows from that observation.

The nomination repair also added a tolerance and final grid-order tie rule
after data. It preserves the material verdict but is recorded as a post-data
clarification, not a complete original preregistration.

Receipts: [independent incoming audit](audit/INCOMING_READER.md),
[exact recount](audit/INCOMING_READER.json), and
[C mechanism audit](audit/MECHANISM.md). Incoming Don checkout stayed clean.

## The one new construction

The inherited witness statistic is `w`. Add only one statistic `a`, the
discounted absolute magnitude of the same candidate-minus-P0 evidence.
After pricing an active matched observation:

```text
delta = log2(candidate truth probability) − log2(P0 truth probability)
w_next = (31/32) * w + delta
a_next = (31/32) * a + abs(delta)
B_next = 16 * max(0, w_next) / (1 + a_next)
odds_next = min(ordinary_hazard_updated_odds, B_next)
```

Unmatched observations leave w/a unchanged. The old w still chooses the
inherited hazard, 2^-10 when w<=−1, otherwise 2^-16. Quote precedes truth;
the admission-crossing byte is cold and excluded from both statistics.
Increasing the permitted limit never increases the surviving odds itself.

The formula distinguishes consistent net support from the same net support
left after substantial opposing testimony. It is a **budget heuristic**, not
a posterior probability or a calibrated amount of re-earned log evidence.
The one-bit denominator regularizer, 16-bit risk scale and 31/32 discount
were fixed before fresh data. No alternative formula or parameter sweep ran.

Code: [state](authority.h), [cap and update](authority.c), and
[prospective replay](replay.c). The signed witness trajectory, hazard choices,
candidate and admission are identical between witness/h8l4/selfnorm. The
archive stays 528 bytes per world; only local prediction state grows from
32 to **40 bytes**. Five modes in the replay occupy 200 bytes, excluding
measurement counters. Update work and additional memory are constant.

The triangle inequality preserves |w|<=a, so 0<=B<16 in exact arithmetic.
Clipping transfers mass only toward absorbing P0. Complete-prefix loss stays
bounded by 1 bit; minimum hazard 2^-16 supplies the 16-bit interval bound.
Those invariants held. Maximum selfnorm drawdown was **11.500571 bits**;
minimum complete-prefix gain was **−0.873156 bits** across all 32 lives.
The tighter h8l4 reference had maximum drawdown **8.005064 bits**.

## What the charged observations show

The cap genuinely varies: intact prequote ranges span 0 to 11.52–14.80 bits,
with 5,311–10,951 clips per intact life. Variation alone is not useful
adaptation. Its errors occur in both directions:

1. **Protection.** World225, switched, t8436: wrong candidate log2 −6.490368
   versus P0 −2.325344. The new smaller carried odds reduce the paid loss by
   **2.009069 bits** relative to h8l4.
2. **Lost useful advice.** World228, switched, t9062: candidate log2 −2.059050
   versus P0 −6.480316. New carried odds −3.636077 versus h8l4 +0.333406
   cost **2.297078 bits**. The ceiling rises after this evidence; it cannot
   recover the already charged opportunity.
3. **Excess influence.** World224, moved_mid, t14455: candidate log2
   −13.458765 versus P0 −1.932868. The positive support ratio allowed cap
   10.945091 and carried odds 10.798627, above h8l4's 7.209554. The new
   prediction loses **2.969108 extra bits**. The cap collapses after truth,
   too late to protect that quote.

These are observed sequences under the actual two policies, not rerun
counterfactual policies. [RAW.md](RAW.md) shows neighboring bytes, exact
prices, evidence, cap and carried odds. A zero clip flag at the quoted byte
does not erase the effects of earlier withdrawals.

The shared first half loses **10.903340 bits** against h8l4 before any seam.
The changed-law tail recovers only **0.945699**, producing the whole-life
loss of **9.957641**. Thus the utility failure is not explained only by a
late response to the law change: the new rule has already charged for its
caution in a still-useful world. Conversely, the moved example shows that
relative support can also permit too much confidence. A universal claim that
every self-normalizing rule must fail would exceed this single experiment.

## Verification and reproducibility

The single disjoint 40-event pre-freeze fixture checked the actual JSON
constructor, all 41 TSV columns and **200 C/mass state-events**, maximum
error **1.2434497875801753e−14**. Four inherited controls matched unchanged
Don21 exactly on the same 160 control-events. No target world was involved.

After freeze, one run of each stage generated, extracted, learned and
evaluated worlds224–231 under `netta-relative-support-ceiling-v1`.
All stage exit codes were zero; receipts and logs are retained. One full
independent reader run also exited zero: **6,291,456 forecasts**, consisting
of 3,670,016 candidate and 2,621,440 authority forecasts. Maximum numeric
disagreement was **1.8849277694243938e−10**, below 1e−7; maximum full-vector
normalization residual was **1.3322676295501878e−14**. All five independent
checks passed and all 18 material booleans matched, preserving FAIL 11/18.
All 22 frozen files, 312 data, 64 memory and 128 result pins matched.
No reader repair or repeated target run was needed. [VERIFY.json](VERIFY.json)
and [direct exit receipt](VERIFY.receipt.json) retain the complete result.

The reader independently rebuilds source archives and seven candidate arms,
recounts suffix matches and reconstructs all five authority policies in
probability space. HEAD256/P0 remain supplied frontend trace inputs; full
byte-vector normalization is a C receipt. This is not an independent frontend
rewrite or a test of natural language coherence.

| Artifact | SHA-256 |
| --- | --- |
| Protocol | `2a1e5dd3322cc43823188880ce0e7fa0341133a3705f333f417a95adfeba3c19` |
| Freeze | `6c6e24523fc4cbe1011422197337f5da5e06c8c5590a78cd55f312ac0df79b15` |
| Result | `b8f63a5f42362129fcf122c698ecfa6e411d7123116224ed4618c7399d45e4fb` |
| Independent verification | `cb69dace7090bc6e4fb0f47d1f9c885ca7abbc166bd26e76c356752036e1b019` |

See [README](README.md) for exact commands and the distinction between
retained-artifact verification and deterministic rebuilding in a separate
directory. No frozen source or result was repaired after the run.

## Return of the hand

Sol receives the incoming audit and this new measured mechanism, including
its failure. The protocol, tests, inputs and result remain sealed. The living
Netta, mouth and mycelium were not modified. This turn does not demonstrate
the broader 50-city-to-51st-city transfer goal or functional recognition of
partially similar objects. It tests one way of controlling the use of the
already transferred episode archive.

One next question, motivated by the realized cost decomposition in
[DIAGNOSIS.md](DIAGNOSIS.md): can support and contradiction remain separate
for two distinguishable fragments of transferred experience, so a local
failure does not reduce use of a different still-useful fragment? The current
single global authority merges those judgments. This proposed attribution
has not been implemented or tested; choosing fragment identities would itself
belong to Sol's next preregistered step. The frontier remains partial reuse
across different worlds, not indefinite adjustment of a global ceiling.

No commit or push was authorized for this turn. Work is local and ready for
Sol's reciprocal audit; a further mechanism requires her own bounded step.

### Publication follow-up, 2026-09-21

After receiving this report, Oleg explicitly authorized committing and pushing
the completed turn under `turns/turn22/`, then returning the hand to Sol.
Publication uses branch `astra/turn22-self-normalizing-ceiling`. The frozen
experiment, its material FAIL and independent verification are unchanged.
The shared handoff records the exact published commit after the push.
