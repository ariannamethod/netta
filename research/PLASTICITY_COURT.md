# The plasticity court: independently audited specification for body 17

Status: **RESEALED BEFORE ANY EXPERT RUN**
Date: 2026-08-16
Lineage: the buried prototype's plasticity genomes (old log, evolution
experiment 2), rebuilt under the zero courts.

The first seal, commit `c246fcf`, had SHA-256
`20b353a48246604c9fd7770cd6d17e7e25f99ff12b01b374cc4df76536b964a5`.
No jury implementation or expert score existed when independent review
refused that specification. The refusal and every reason for it are preserved
in `PLASTICITY_COURT_AUDIT_2026-08-16.md`. This replacement is the only
operative specification. No table entry, fitness rule, target, window,
threshold, or promotion law below may change after its commit and before the
sealed result is published.

## What is on trial

Hebb-v1 lost its matched frozen-reservoir control on every tested world and
remains quarantined behind `--core-hebb-v1`. Its diagnosed defect is positive
feedback: gated potentiation drives hidden saturation (`|h| 0.9840` at forty
thousand bytes), which drives more gated events (`+34732/-5076`). The frozen
reservoir with a delta readout is the reigning incumbent.

The court does not restore v1. It tests whether a smaller local recurrent rule
with continuous decay and per-row saturation rent can beat the frozen
incumbent. A published null is a successful court.

## Causal identity of the jury

Eight genomes own eight complete shadow copies of the mutable core memory:
`Wxh`, `Whh`, `Who`, prophecy baseline, score record, hidden-health record, and
clamp record. Innate embeddings are shared because they are deterministic and
immutable. All eight copies are born **byte-identical** from the current
incumbent core. Genome index selects only one sealed gene row; it never seeds or
perturbs weights. Every shadow receives the same truth byte in the same order,
prices it before updating, and consumes no draw from the life's RNG.

Genome zero is the exact frozen incumbent. Its readout is still plastic under
the accepted delta law, while `Wxh` and `Whh` never move. The ordinary core
continues separately under `--jury`, so the instrument cannot alter the life it
observes.

## The sealed genomes and update law

Readout learning rate is **not a gene**. It is `0.05` for every genome, because
varying witness maturity would confound the recurrent-plasticity trial. The
eight rows below are complete; there is no mutation, generation, sampling, or
hidden range.

| g | input eta | recurrent eta | surprise gate | mod clip | decay/byte | target | homeostasis/byte |
|---:|---:|---:|---:|---:|---:|---:|---:|
| 0 | 0 | 0 | 0.5 | 1.0 | 0 | 1.0 | 0 |
| 1 | 0.0003125 | 0.0003125 | 0.5 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 2 | 0.0006250 | 0.0006250 | 0.5 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 3 | 0.0012500 | 0.0012500 | 0.5 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 4 | 0.0006250 | 0.0003125 | 0.5 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 5 | 0.0003125 | 0.0006250 | 0.5 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 6 | 0.0006250 | 0.0006250 | 1.0 | 0.25 | 0.000001 | 0.35 | 0.000244140625 |
| 7 | 0.0006250 | 0.0006250 | 0.5 | 0.50 | 0.000001 | 0.25 | 0.000488281250 |

For each byte, prophecy, readout update, state advance, and baseline update keep
the body-16 order. For each hidden row `j`, after state advance define

```
rent_j = homeostasis * max(0, abs(h[j]) - target)
scale_j = max(0, 1 - decay - rent_j)
```

Every input and recurrent weight in row `j` is multiplied by `scale_j` on
**every byte**, whether or not surprise opens the Hebbian gate. If
`abs(baseline - nll) > gate`, the clipped modulation is then added with the
row's input or recurrent eta and the same local co-activations as v1. The sum is
checked finite and clamped to `[-4, 4]`. Thus homeostasis can remove accumulated
runaway after a gate closes; it is not merely a discount on new potentiation.

Genes may touch only their shadow core. Embeddings, readout-law form, alphabet,
life RNG, actors, biography, and ordinary core are frozen by constitution.

## Matched donor court: order, not age

The old `K-H` rule compared different target tapes and could reward a genome
for damaging its shuffled control. It is replaced by a paired donor-order
contrast. The runner is `scripts/plasticity_court.py`.

The normalized Gutenberg sources, compile flags, seeds, donor length, target
windows, and offsets are exactly those in `GUTENBERG_ARENA.md`. In addition to
ordered Dracula, the court makes a deterministic Fisher-Yates permutation of
the **Dracula donor** with SplitMix64 seed `0x4e45545441475241`. The two donors
therefore have identical bytes, histogram, age, and readout maturity; only
their order differs.

For genome `g` and a target window `w`:

- `R_g(w)` is its prequential price after ordered Dracula;
- `Q_g(w)` is its price after shuffled Dracula;
- `K_g = median_w(Q_g(w) - R_g(w))` on the three Frankenstein windows.

Positive `K` is advantage carried by donor order. Because both arms meet the
same target window, target difficulty and online target adaptation cancel
inside each pair.

A candidate may not manufacture contrast by injuring a control. Besides the
promotion margin below, it must satisfy both absolute gates:

- `median R_g <= median R_0 - 0.1` bit/raw-byte;
- `median Q_g <= median Q_0 + 0.1` bit/raw-byte.

The first demands an actual improvement on kin transfer. The second forbids a
win bought by making the matched red donor worse.

## Sealed alien world and breadth veto

Distances use unsmoothed empirical distributions on all byte values. Terms
with zero mass contribute zero. `JS1` is base-2 Jensen-Shannon divergence of
the byte census. `JS2` is Jensen-Shannon divergence of adjacent-byte joint
distributions. Conditional divergence is exactly `CJS = JS2 - JS1` on the
pair-prefix marginals. Conditional entropy is `H2 - H1(prefix)`.

The technical Gutenberg body is the old floor. Its maximum distance over
Dracula, Frankenstein, and tracked `netta.txt` is `JS1 0.039475`,
`CJS 0.068688`. A candidate must have its **minimum** distance over those same
three references exceed both maxima. It is a world only if census entropy
minus conditional entropy is at least `0.25` bit.

The sealed alien is the deterministic byte lattice of length `233688`:

```
x[i] = ((i % 251) * 73 + ((i // 251) % 17) * 19) % 256
```

Its SHA-256 is
`7a3f7a161e6777a9905db5e06bbd3560a82999cdab06a09f0e3258d401ce4f40`.
Its minimum distances are `JS1 0.707803`, `CJS 0.253721`; census entropy is
`7.999420`, conditional entropy `0.021860`, a `7.977560`-bit structure gap.
The runner must regenerate and remeasure all figures before an expert; any
failed hash, floor, or structure check aborts the court.

On the same three offsets define alien donor-order advantage
`A_g = median_w(Q_g(w) - R_g(w))`, now probing the lattice. Breadth vetoes are:

- `A_g <= A_0 + 0.1`;
- ordered-donor alien price `<=` genome zero's corresponding median `+ 0.1`;
- shuffled-donor alien price `<=` genome zero's corresponding median `+ 0.1`.

A candidate may neither broaden Dracula's structural prior nor hide damage to
the alien world in a difference score.

## Synthetic health and performance vetoes

Four independent newborn jury lives see period-5, -6, -7, and -8 tapes. Each
life receives five 2000-byte episodes: 10000 bytes per world, 40000 in the
published health table. On every world, every candidate must satisfy:

- core price no worse than genome zero by more than `0.1` bit/raw-byte;
- maximum over hidden rows of time-mean `abs(h[j])` no more than genome zero's
  value plus `0.1`;
- fraction of hidden activations with `abs(h[j]) >= 0.98` no more than genome
  zero's fraction plus `0.01`;
- zero non-finite values and zero attempted clamp crossings.

The row maximum prevents one saturated subpopulation from hiding inside an
innocent global mean. The near-saturation fraction names the diagnosed v1
failure directly; both thresholds are control-relative.

## Promotion law

A nonzero genome is eligible only if it passes every absolute, breadth, health,
finite, and clamp veto and

`K_g >= K_0 + 0.1` bit/raw-byte.

Ties keep genome zero. Among multiple eligible genomes, larger `K` wins; an
exact tie chooses the lower sealed genome index. Promotion is not performed in
this body: it requires a new body/version and a red arm preserving the defeated
law. If none wins, frozen remains default and the full null table is published.

## Instrument and restart law

`--jury` is explicit and off by default. It may emit stdout and state bytes but
never changes an actor, action, life RNG draw, ordinary-core byte, or biography
receipt. Its eight gene rows, weights, baselines, records, health counters, and
witness are persisted. An uninterrupted jury life and the same life split at
an episode boundary must have byte-identical state, biography, and jury output.

Body 17 also repairs the already demonstrated silent law change. State v20
binds the neural law tuple `(core enabled, core-hebb-v1 enabled, jury enabled)`.
A resume whose CLI tuple differs is refused by name before any new receipt.
`--reset` deliberately establishes a new tuple. Actor locks, Atlas navigation,
fixed offsets, and island choice remain declared interventions rather than
learned neural laws.

## What this court does not decide

The jury has no candidacy or biography authority. It does not grant the core an
actor seat, choose an Atlas destination, tune a source, or decide the future
genome table. It answers one question only: did any sealed local recurrent rule
earn the right to ask for a promotion body?
