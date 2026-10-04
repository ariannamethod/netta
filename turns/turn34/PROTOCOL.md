# Turn34 protocol: distinct episodes earn their voice only where they apply

Don, 2026-10-03. Written before implementation, fixture execution, generation
or inspection of worlds 304--311. Base `1fda545`, which contains Sol's turn33
and Don's published counter-audit (AUDIT33, verdict GO). Oleg's rotation count
records that counter-audit as a turn without a directory; this is the next
scientific hand and is therefore recorded as turn34. Rotation after this hand:
Astra.

## Incoming result and question

Turn33 established that two separate case histories, starting completely
silent, can earn a reversible, local, positive-excess-wealth residual over the
full pooled archive: PASS E1--E4, partial-tail earned-minus-pooled
+43.475058565 bits at wins 7/8 against a 3.84-bit priced margin, recombined
retention 101.37%, earned-minus-permuted +60.057178651 at 8/8. Don recounted
the sealed reader byte-identically and accepted the PASS.

Sol's handoff names the open frontier: compositional memory — several
distinguishable episodes with applicability conditions, and whether a new life
can select the useful pieces without duplicating every record. Turn34 narrows
that to the smallest honest step: **granularity and selection.** When the same
four inherited source lives are remembered as FOUR separate episodes instead
of turn33's two coarse cases, does per-episode earned wealth select the
applicable episode where it applies — paying its own larger price — or does
the finer split dilute the correction that coarseness already earned? The
candidate changes the representation's granularity only: rho, admission,
hazard, generator, source selection, pooled permission and outer law are all
inherited untouched.

## Portable memory and price

Four 16 KiB source lives are inherited. Turn33 grouped them pairwise into
cases A and B; turn34 additionally keeps each life as its own episode:
E1,E2,E3,E4 (lives 1--4 in inherited order). Greedy selection is performed
once on pooled evidence and retains the full 24-address archive admitted by
the inherited 528-byte limit.

For those same addresses:

- `pooled_full.bin` stores one summed seven-count vector per address;
- `bank2_full.bin` stores the turn33 A and B vectors separately;
- `bank4_full.bin` stores the four episode vectors separately;
- `permuted4_full.bin` rotates repeat-role counts inside each episode vector
  while retaining the same addresses and NEW counts.

E1+E2 must reconstruct A, E3+E4 must reconstruct B, and A+B must reconstruct
`pooled_full.bin`, all exactly. The portable deltas
`extra2 = bank2_full - pooled_full` and `extra4 = bank4_full - pooled_full`
are measured and must each be identical in all eight worlds; the expectation
from turn33's layout is extra2 = 384 and extra4 = 3 x 384 = 1152 bytes, and
the binding value is the measurement, not the expectation. No source record
is selected from recipient outcomes.

## One candidate: earned selection among four episodes

At each address let `P` be the pooled KT continuation distribution, `Pi` the
separately normalized distribution of episode i, for i in {1,2,3,4}. Keep the
inherited binary pooled permission `u`, initially 7/8, fixed share
rho = 2^-10. Add four local log2 wealths `h1..h4`, initially exactly zero.

Before a quote define nonnegative excess wealth per episode:

```
e(h) = max(2^h - 1, 0)
S = (P + sum_i e(hi) Pi) / (1 + sum_i e(hi))
Qsel    = (1-u) P0 + u S
Qpooled = (1-u) P0 + u P
```

Thus `Qsel == Qpooled` byte-for-byte whenever every wealth is nonpositive,
including the first visit to every address: episode distinctions carry no
prior predictive mass, exactly as turn33's law demands. After truth x, using
the pretruth state:

```
hi' = log2((1-rho) 2^hi Pi(x)/P(x) + rho)    for each i
u'  = (1-rho) u P(x)/Qpooled(x) + rho*(7/8)
```

The permission update always uses the pooled shadow, never the corrected
forecast. Episode wealth is an added residual and cannot rewrite the
incumbent permission clock. States are local to an address, update only after
a matched causal quote, reset between lives and never modify immutable source
counts. NEW and exact-common forecasts copy P0 bit-for-bit. Descriptive
silent/active and exact-equality classifications follow the recorded C double
after the reader's independent reconstruction has agreed within the frozen
1e-7 tolerance — turn33's disclosed reader-repair convention, adopted here
into the frozen reader from the start rather than repaired after the fact.

## Simultaneous arms

- `sel4`: the candidate above on the real four-episode bank;
- `earned2`: turn33's two-case law verbatim on the real A/B bank — the
  incumbent correction and the direct ancestor control;
- `pooled`: the unchanged full24 pooled-permission incumbent;
- `permuted4`: the sel4 law, but the episode residuals use permuted episode
  counts; pooled base and permission are the real incumbent;
- `cold`: local P0 with no transferred influence.

All arms consume one raw-byte/P0/history tape. Admission is inherited and
shared: only the pooled incumbent's cumulative candidate-minus-P0 shadow can
open at 32 bits, effective on the next byte. Each arm maintains its own
unchanged one-way outer HMM and its own charged gain. Wealth learns in shadow
before admission; no candidate controls its own entry time.

Recipient state for sel4 is one pooled-permission double plus four wealth
doubles per address: 40 bytes/address, against 24 for earned2 and 8 for
pooled. Portable and recipient-state deltas are reported separately.

## Fresh recipients

Worlds 304--311, namespace `netta-earned-selection-v1`. Five inherited 16 KiB
regimes: recombined, partial, switched, moved_mid and unrelated. The
partial/switched/moved manipulation begins at byte 8192; all post-change
claims use [8192,16384). Source generators, grammar growth, greedy pooled
address selection, causal HEAD256/P0 bindings and command transformations
remain inherited and hash-pinned. These worlds still share the synthetic
role/emitter family and are not natural-language or semantic-similarity
evidence.

## Gate fixed before data

All values are received log2 probability gain relative to the same P0. A win
is a strictly positive paired difference. The priced margins follow the
inherited 0.01 bits-per-portable-byte rule: `m4 = 0.01 * extra4` (expected
11.52) and `m42 = 0.01 * (extra4 - extra2)` (expected 7.68), bound to the
measured byte deltas, not to the expectations.

G1 — selection pays after partial change: on [8192,16384),
sel4-minus-pooled mean exceeds m4 and wins at least 5/8; sel4-minus-pooled
over the whole partial life is nonnegative.

G2 — the silent start preserves ordinary ground: on recombined early4096 and
whole16384, sel4 retains at least 99% of pooled's positive mean gain; all
eight sel4 whole-life gains are positive.

G3 — correspondence rather than generic extra state: sel4-minus-permuted4
exceeds m4 and wins at least 5/8 on both recombined whole life and the
partial post-change tail.

G4 — composition pays its own price over coarseness: on the switched
post-change tail [8192,16384), sel4-minus-earned2 mean exceeds m42 and wins
at least 5/8. This is the turn's own question; a FAIL here is a finding of
equal rank that finer episodes do not earn their extra bytes on these worlds,
and it is not tuned away.

G5 — causal and numeric validity: E1+E2 reconstructs A, E3+E4 reconstructs B,
A+B reconstructs pooled counts at all 24 addresses; first visits and every
all-nonpositive-wealth quote are sel4==pooled byte-for-byte; updates follow
truth; all distributions normalize and remain positive; NEW/equal/no-match
behavior is exact; common admission is identical; every arm/life has
complete-prefix gain >= -1-1e-7 and drawdown <= 16+1e-7. An independent
probability-space reader must reconstruct all archives, forecasts,
wealth/permission trajectories, outer states, metrics and gates within 1e-7,
with the discontinuous-label convention declared in the candidate section.
The writer cannot certify G5 itself.

Material PASS requires G1--G5. Otherwise retain FAIL. No threshold, source
budget, arm, world or law may change after generation. A technical defect may
be repaired only under the established disclosed-amendment rule and never by
reusing the material worlds for a second candidate.

## Evidence and stop

Before generation: strict C11/O2/Werror build and an independent Decimal
fixture containing first visits, positive and negative wealth for every
episode, recovery, record locality, exact equality, NEW and no-match events.
Freeze protocol, interfaces, implementation, reader, fixture and inherited
sources.

Retain every arm/world/regime early, whole and tail gain, activation,
minimum, drawdown, portable bytes and recipient bytes. Per episode, report
how often its residual remains silent, activates and later falls silent, and
at every sel4-vs-pooled divergent quote record the excess-wealth shares of
all four episodes, so concentration of selection is readable from the record
rather than claimed. Mechanically select the largest help and harm
sel4-minus-pooled and sel4-minus-earned2 bytes on their gate tails and the
first sel4 divergence in each world. Preserve raw neighborhoods, source
counts, pretruth wealth, permission, forecasts and truth.

After one batch and one independent replay, stop. Do not connect this organ
to the mouth, Body 0, mycelium or live Netta. No commit, push or merge
without Oleg's separate instruction. Next rotation recipient is Astra.
