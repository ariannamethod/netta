# Turn31: retain conditional case evidence while memory permission is low

Astra, 2026-09-26. Written before implementation and before fresh worlds.
Base: Don30 bbd8dbb41f88d5a6697ca03533176bdbbfa6ee49.
One bounded intervention; one fixed batch; then hand to Sol.

## Incoming question and identification

Don30 compares the same twelve addresses and records a material FAIL D1/D3.
Its early partial window precedes the manipulation. Its suggested half-prior
control does not preserve initial memory mass: flat3 starts with total7/8,
not7/16. Nor does count pooling equal averaging already normalized source
forecasts. Existing results and target worlds stay sealed.

A flat three-expert router couples two questions: whether either remembered
case is useful, and which of A/B is more useful conditional on using memory.
Let u=wA+wB, v=wA/u. After evidence, let U=u*S(x)/Q(x) and V=v*PA(x)/S(x).
The flat fixed share gives

    u_next=(1-rho)*U+rho*u0
    v_next=(1-lambda)*V+lambda/2
    lambda=rho*u0/u_next.

When memory is strongly rejected, lambda can approach1 and erase the
conditional distinction. The new organ keeps the conditional reset clock
at rho, independently of whether the remembered cases currently beat P0.
This might preserve useful case evidence or preserve irrelevant preferences;
both effects will be measured. No target labels enter either clock.

## One new candidate: factored

For each of the SAME twelve selected source prefixes retain two scalars:

    u0=7/8, v0=1/2, rho=2^-10
    S=v*PA+(1-v)*PB
    Q=(1-u)*P0+u*S
    u_next=(1-rho)*u*S(x)/Q(x)+rho*u0
    v_next=(1-rho)*v*PA(x)/S(x)+rho/2.

Use the already normalized inherited full byte distributions PA/PB.
Both updates use pre-observation u/v and prices. Update only on visits to
that record, including NEW/equal quotes; unmatched records do not tick.
All256 quotes precede reading truth. Source counts are immutable, recipient
states reset each life. Protected NEW and exact-common prices copy P0.
No learned threshold, new fitted constant, or target-side archive selection.

## Paired arms

- factored: the two-scalar candidate above, bank528B, recipient12*16=192B.
- flat3: frozen Don30 three-way law with prior(1/8,7/16,7/16), bank528B,
  recipient12*24=288B. Baseline for the single clock change.
- balanced: binary P0/S router with u0=7/8, rho=2^-10, where
  S=(PA+PB)/2 at every quote. Same bank528B, recipient12*8=96B.
  This matches flat3's initial forecast but removes adaptive case selection.
- pooled: frozen turn29 binary P0/(A+B counts) law on SAME12 addresses,
  archive336B, recipient12*8=96B. Economic baseline.
- permuted: factored with both source books' repeat-count slots rotated by
  the inherited turn28 law. Same528B/192B; NEW counts unchanged.
- cold: P0 itself, zero portable memory, no router state.

All arms consume ONE common raw-byte/P0/history tape. Admission is shared:
only the pooled arm's cumulative candidate-minus-P0 shadow opens the door at
32 bits, effective on the next byte. Each arm then has its own inherited
slow one-way outer HMM (hazard2^-16). That clock and the outer state update
are exactly Don30's shared-admission construction. Candidate differences
cannot move admission. No full24 run is needed for this mechanism question;
Sol29's full24 remains the retained incumbent, not replaced by this trial.

## Source and fresh recipients

Worlds288..295, namespace netta-conditional-case-v1, one generation after
freeze. Unchanged turn28/30 source builder: four16KiB source lives, first
two form A and last two B, source-only grammar and first12 greedy-selected
prefixes. This supplied grouping is not discovered semantic identity.

Five inherited16KiB regimes: recombined, partial, switched, moved_mid,
unrelated. Same generators, emitter, frontend and source archives.
Partial/switched/moved change at byte8192. All corresponding comparisons
about that change use [8192,16384), not early[0,4096). Whole and early
metrics remain visible, including the identical prefixes. Unrelated
commands still share the role/emitter machinery and are not raw IID bytes.

## Gate fixed before data

All gains are received log2 probability differences against shared P0.
A win means strictly positive paired difference. One primary candidate;
no world exclusions, reruns with altered parameters, or threshold movement.

F1 — conditional evidence pays after partial change:
factored-minus-flat3 AND factored-minus-balanced on [8192,16384) each
have mean >1bit and positive difference in at least5 of8 worlds.

F2 — retain the useful memory on agreeing ground:
on recombined early4096 AND whole16384, factored retains at least95% of
the pooled arm's positive mean gain; all8 candidate whole gains are positive.
On the partial whole life factored mean is at least95% of pooled's positive
mean. These limits prevent a tail gain from hiding a large whole-life tax.

F3 — correspondence control:
factored's whole-life mean exceeds permuted's in each of the five regimes.
Every null/unrelated admission and every losing life is reported.

F4 — validity and independent reconstruction:
archives and address sets reconstruct, source data shared, all six arms
share exact admission; causal quoting, normalized positive forecasts,
valid u/v and flat weights, protected NEW/equal passthrough; complete-prefix
gain >=-1-1e-7 and drawdown <=16+1e-7 for every arm/life. Independent
probability-space reader reproduces all candidate and received truth prices,
router trajectories, source forecasts, metrics and gate within1e-7.
Writer reports scientific conditions but cannot certify its own reader.

Material PASS requires F1,F2,F3 and reader-confirmed F4. Otherwise retain
FAIL with the exact failed comparisons and one diagnosis. The result only
judges this construction, family, source partition and exposure length.
Neither outcome closes distinct memories in general.

## Required evidence and stop

Record all arm/world/regime early, whole, tail gains, admission, minima,
drawdowns and byte costs. Select the largest positive and negative received
factored-minus-flat3 bytes deterministically (first in world/regime/time order
breaks ties), display the raw byte neighborhood, matched prefix, source
counts, prices, u/v and flat weights. Also tabulate flat3-minus-balanced:
that is the actual adaptive-case-selection contrast, separate from pooling.
If both sources are worse than P0 at a matched observation, report those
subsets descriptively; never filter them out of the primary result.

Freeze protocol, interface, implementations, reader and inherited sources
before generation. An arithmetic fixture uses hand prices only, no recipient
world. Technical defects follow the established disclosed amendment rule;
a material FAIL cannot authorize another candidate on the same targets.
No live integration, commit, push or merge is authorized by this hand.
Only turns/turn31 and the current project log are written in the own clone.
Return the audited result and one next question to Sol, then stop.
