# Turn33 protocol: a remembered case begins as a silent residual

Sol, 2026-10-03. Written before implementation, fixture execution, generation
or inspection of worlds 296--303. Base `4dedb745ff044bd735dfa55f5a0e87ad9780d298`,
which contains Astra31 and Don's published counter-audit. Oleg calls Don's
counter-audit rotation turn32; it adds no `turns/turn32/` experiment. This is
the next scientific hand and is therefore recorded as turn33.

## Incoming result and question

Astra31 separated permission `u` from conditional A/B preference `v`. Its
primary partial-tail gain over flat3 was 0.742642 bit, below the frozen one-bit
margin: material FAIL F1. Don independently recounted the gate, source counts,
reader receipt and three refusal probes and accepted the sealed FAIL.

Their useful unresolved question is narrower than another reset-rate trial:
can distinct histories add a predictive correction while the first forecast
at every remembered relation remains **exactly** the pooled incumbent? The
candidate below changes the representation, not rho, admission, hazard,
generator, source selection or outer law.

## Portable memory and price

Four 16 KiB source lives are inherited. The first two form case A, the last
two case B. Greedy selection is performed once on their pooled evidence and
retains the full 24-address archive admitted by the inherited 528-byte limit.

For those same addresses:

- `pooled_full.bin` stores one summed seven-count vector per address;
- `bank_full.bin` stores A and B seven-count vectors separately;
- `permuted_full.bin` rotates repeat-role counts inside each A/B vector while
  retaining the same addresses and NEW counts.

The A+B vectors must reconstruct `pooled_full.bin` exactly. The two-case bank
is expected to cost 384 portable bytes more than the 528-byte pooled archive;
the actual difference is measured and must be identical in all eight worlds.
No source record is selected from recipient outcomes.

## One candidate: earned residual cases

At each address let `P` be the pooled KT continuation distribution and
`PA,PB` the separately normalized case distributions. Keep the inherited
binary pooled permission `u`, initially 7/8, with fixed share rho=2^-10.
Add two local log2 wealths `hA,hB`, initially exactly zero.

Before a quote define nonnegative excess wealth

```
e(h) = max(2^h - 1, 0)
S = (P + e(hA) PA + e(hB) PB) / (1 + e(hA) + e(hB))
Qearned = (1-u) P0 + u S
Qpooled = (1-u) P0 + u P
```

Thus `Qearned == Qpooled` byte-for-byte whenever both wealths are nonpositive,
including the first visit to every address. Case distinctions have no prior
predictive mass. After truth x, using the pretruth state:

```
hA' = log2((1-rho) 2^hA PA(x)/P(x) + rho)
hB' = log2((1-rho) 2^hB PB(x)/P(x) + rho)
u'  = (1-rho) u P(x)/Qpooled(x) + rho*(7/8)
```

The permission update always uses the pooled shadow, never the corrected
forecast. Case wealth is therefore an added residual and cannot rewrite the
incumbent permission clock. Fixed share returns wealth toward one and permits
recovery; it is the inherited rho, not a fitted constant. States are local to
an address, update only after a matched causal quote, reset between lives and
never modify immutable source counts. NEW and exact-common forecasts copy P0
bit-for-bit.

Recipient state for the candidate is one pooled-permission double plus two
wealth doubles per address: 24 bytes/address. The pooled incumbent uses eight.
Both portable and recipient-state deltas are reported separately.

## Simultaneous arms

- `earned`: the candidate above on the real A/B bank;
- `pooled`: the unchanged full24 pooled-permission incumbent;
- `permuted`: the same earned-residual law, but only the case residuals use
  permuted A/B counts; its pooled base and permission are the real incumbent;
- `flat3`: the frozen immediate P0/A/B three-expert law on all 24 addresses,
  retained as a non-material contextual control;
- `cold`: local P0 with no transferred influence.

All arms consume one raw-byte/P0/history tape. Admission is inherited and
shared: only the pooled incumbent's cumulative candidate-minus-P0 shadow can
open at 32 bits, effective on the next byte. Each arm then maintains its own
unchanged one-way outer HMM and its own charged gain. Wealth learns in shadow
before admission, just as the pooled router learns; no candidate controls its
own entry time.

## Fresh recipients

Worlds 296--303, namespace `netta-earned-case-residual-v1`. Five inherited
16 KiB regimes: recombined, partial, switched, moved_mid and unrelated. The
partial/switched/moved manipulation begins at byte 8192; all post-change claims
use [8192,16384). Source generators, grammar growth, greedy pooled address
selection, causal HEAD256/P0 bindings and command transformations remain
inherited and hash-pinned. These worlds still share the synthetic role/emitter
family and are not natural-language or semantic-similarity evidence.

## Gate fixed before data

All values are received log2 probability gain relative to the same P0. A win
is a strictly positive paired difference. Let `extra` be the measured portable
bytes of `bank_full.bin - pooled_full.bin`; it must equal 384 in every world.
The priced margin is `0.01 * extra = 3.84` bits per life.

E1 — earned correction pays after partial change: on [8192,16384),
earned-minus-pooled mean exceeds the priced margin and wins at least 5/8;
earned-minus-pooled over the whole partial life is nonnegative.

E2 — the silent start preserves ordinary ground: on recombined early4096 and
whole16384, earned retains at least 99% of pooled's positive mean gain; all
eight earned whole-life gains are positive.

E3 — correspondence rather than generic extra state: earned-minus-permuted
exceeds the priced margin and wins at least 5/8 on both recombined whole life
and the partial post-change tail.

E4 — causal and numeric validity: A+B reconstructs pooled counts at all 24
addresses; first visits and every nonpositive-wealth quote are earned==pooled
byte-for-byte; updates follow truth; all distributions normalize and remain
positive; NEW/equal/no-match behavior is exact; common admission is identical;
every arm/life has complete-prefix gain >= -1-1e-7 and drawdown <=16+1e-7.
An independent probability-space reader must reconstruct all archives,
forecasts, wealth/permission trajectories, outer states, metrics and gates
within1e-7. The writer cannot certify E4 itself.

Material PASS requires E1--E4. Otherwise retain FAIL. No threshold, source
budget, arm, world or law may change after generation. A technical defect may
be repaired only under the established disclosed-amendment rule and never by
reusing the material worlds for a second candidate.

## Evidence and stop

Before generation: strict C11/O2/Werror build and an independent Decimal
fixture containing first visits, positive and negative case wealth, recovery,
record locality, exact equality, NEW and no-match events. Freeze protocol,
interfaces, implementation, reader, fixture and inherited sources.

Retain every arm/world/regime early, whole and tail gain, activation, minimum,
drawdown, portable bytes and recipient bytes. Mechanically select the largest
help and harm earned-minus-pooled bytes on the partial tail and the first
earned divergence at each world. Preserve raw neighborhoods, source counts,
pretruth wealth, permission, forecasts and truth. Report how often residuals
remain silent, activate and later fall silent.

After one batch and one independent replay, stop. Do not connect this organ to
the mouth, Body 0, mycelium or live Netta. No commit, push or merge without
Oleg's separate instruction. Next rotation recipient is Don.
