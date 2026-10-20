# Turn37: an episode earns a residual against the current coarse forecast

Sol, 2026-10-05. Base Astra turn36
`235762ec7a06ce5df2925a649d1003f77b527bde`; research object
`a8ef6387e86db07036f82c58558c83e4ce5353cf`. This protocol precedes
implementation and all generation or inspection of worlds 328–335.

## Incoming result and one change

Turn36 showed a concrete parent veto. A fine episode could have positive
recipient wealth while both coarse cases were nonpositive, forcing the
hierarchy back to pooled. Turn37 keeps the same source archive, coarse
predictor, permission, admission, P0 and outer law. It changes only the
denominator against which an episode earns its influence.

Let `P` be pooled memory, `A` and `B` the two coarse case forecasts, `E1..E4`
the four episode forecasts, `rho=1/1024`, `e(h)=max(2^h-1,0)`, and

    R(base, forecasts, wealths) =
        (base + sum_i e(hi)*forecast_i) / (1 + sum_i e(hi))
    C = R(P, [A,B], [hA,hB])
    S_direct = R(C, [E1,E2,E3,E4], [g1,g2,g3,g4])
    Q = (1-u)*P0 + u*S_direct

The coarse wealths retain their inherited updates against `P`. Each direct
episode wealth updates against the pretruth current coarse forecast `C`:

    hC' = log2((1-rho)*2^hC * PC(x)/P(x) + rho)
    gi' = log2((1-rho)*2^gi * Ei(x)/C(x) + rho)

All four direct wealths begin at zero and reset between recipient lives. When
all are nonpositive, `S_direct` equals `C` exactly. A positive episode may
speak when both coarse wealths are nonpositive because its comparison is to
the resulting coarse forecast itself. The episode does not alter `hA`, `hB`,
`u`, admission or any other arm's state.

## Controls and accounting

Seven simultaneous arms share one causal byte/P0 tape and pooled prospective
admission, with independent inherited outer states:

- `direct`: the new direct-against-coarse residual;
- `coarse`: unchanged turn33 two-case correction;
- `fine`: unchanged turn34 four episodes earning against pooled;
- `hier`: unchanged turn36 parent-relative hierarchy;
- `null`: direct residual using the turn36 parent-preserving scrambled episode
  archive;
- `pooled`: unchanged pooled-permission incumbent;
- `cold`: P0.

The null exchanges odd repeat-rank counts inside E1/E2 and E3/E4, preserving
each coarse parent vector, every address and the full archive length. The
four-episode portable archive is measured before scoring and must remain
1,584 bytes; coarse is 912 and pooled 528. Direct therefore adds 672 portable
bytes over coarse, priced at 6.72 bits. Direct recipient state is permission,
two coarse wealths and four direct wealths: 56 bytes per address, 1,344 bytes
over 24 addresses. Portable and recipient bytes are reported separately.

`hier` is an equal-byte, equal-recipient-state mechanism control. `fine` is an
equal-portable-byte control with 960 recipient bytes. `null` has identical
portable and recipient dimensions to `direct`.

## Fresh experiment and fixed gates

Worlds 328–335, namespace `netta-direct-coarse-residual-v1`. Inherited five
16 KiB regimes: recombined, partial, switched, moved_mid and unrelated. Four
16 KiB source lives per world. Changed tails are `[8192,16384)`; early is
`[0,4096)`. Recipient observations never affect archive selection, counts or
the null transformation. No threshold, world, archive size or mechanism is
changed after freezing.

All comparisons are mean received log2 probability differences per life;
wins are strict positive paired differences.

**D1 payment:** switched-tail `direct-coarse` is greater than 6.72 bits,
wins at least 5/8, and switched-whole `direct-coarse` is nonnegative.

**D2 parent-veto removal:** switched-tail `direct-hier` is greater than 1 bit
and wins at least 5/8, at equal portable and recipient bytes.

**D3 episode structure:** switched-tail `direct-null` is greater than 1 bit
and wins at least 5/8, with identical coarse parents, addresses and byte
budgets.

**D4 retention:** recombined early and whole `direct` retain at least 99% of
the corresponding positive coarse means; direct whole is positive in 8/8;
partial-tail `direct-coarse` is nonnegative.

**D5 independent replay:** a reader sharing no turn37 writer or C code
recounts source lives, rebuilds all archives, replays every state, forecast,
permission, admission and outer trajectory within `1e-7`, checks all gates,
and verifies prefix gain `>=-1-1e-7` and drawdown `<=16+1e-7` for every arm and
life. It checks exact direct silence, inherited hierarchy silence,
NEW/equal/no-match passthrough, common prefixes and source invariance.

Material acceptance requires D1–D5. Every failed gate and losing world is
retained. A correct FAIL completes the turn.

## Evidence before and after the batch

Before fresh targets: strict C11/Werror build; an independent probability
fixture covering positive and negative direct wealth, exact silence and a
positive direct episode speaking while both coarse parents are silent; then
one complete writer/reader preflight on already-open turn36 world320. Freeze
the protocol, code, reader, fixture, preflight and inherited dependencies.

After the one batch: show mechanically selected help and harm against coarse,
hierarchy and null, including any event where direct influence survives a
silent coarse parent. Retain all regimes, admissions, activations, revocations,
portable sizes and recipient sizes. The memory experiment remains isolated
from the merged mouth. Return the completed hand to Don.
