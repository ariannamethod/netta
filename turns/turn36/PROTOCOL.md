# Turn36: an episode earns its detail against its parent case

Astra, 2026-10-05. Base main `4c90cf36e50a8dfc5ae2f0ed59e34267667c4543`.
Incoming hand: Don34 `3c780af57b11e716d8d7c127917f52302618a959`, the four
checks in the shared Don-to-Astra handoff. Sol35 at `91088f5` is read as
already-open evidence: source-top-ten granularity failed its price. Its worlds
312–319 will not be reused. This protocol precedes implementation and all
generation or inspection of worlds 320–327.

## One change

Coarse cases A=E1+E2 and B=E3+E4 keep their inherited turn33 wealth against
the pooled distribution P. Four additional detail wealths learn against their
own parent case, rather than against P as in turn34. All wealth starts at zero.
Let e(h)=max(2^h-1,0), rho=1/1024, and

    R(base,x,y;h1,h2) = (base + e(h1)*x + e(h2)*y)/(1+e(h1)+e(h2))
    A_detail = R(PA,E1,E2;g1,g2)
    B_detail = R(PB,E3,E4;g3,g4)
    S_hier = R(P,A_detail,B_detail;hA,hB)
    S_coarse = R(P,PA,PB;hA,hB)
    Q = (1-u)*P0 + u*S

After each matched truth x, using only pretruth quantities:

    hC' = log2((1-rho)*2^hC * PC(x)/P(x) + rho)
    gi' = log2((1-rho)*2^gi * Ei(x)/P_parent(i)(x) + rho)

The permission u and shared admission remain exactly the inherited pooled
shadow laws. Corrected cases do not change hA/hB or u. The hierarchy preserves
the coarse source forecast whenever all detail wealths are nonpositive; if
both coarse wealths are nonpositive it preserves P regardless of detail wealth.
All states reset between recipient lives. NEW, no-match and equal forecasts
retain inherited exact passthrough behavior. No parameter sweep or adaptation
of archive size follows this batch.

## Archives, controls and accounting

The same inherited source lives, greedy full24 addresses and counts supply all
arms. The four-episode NETEB004 payload is 1,584 bytes on the inherited shape;
the two-case NETEB001 is 912, pooled 528. A/B and P are exact count projections
of the four books. The runner loads redundant comparison archives and checks
these identities; portable candidate storage is the single four-book payload.
All actual lengths are measured before scoring. If counts or dimensions depart
from the inherited full24 shape, stop for diagnosis before evaluating targets.

Six simultaneous arms share a causal raw-byte/P0 tape and pooled prospective
admission, with independent inherited slow outer states:

- hier: this parent-relative hierarchy;
- coarse: unchanged turn33 two-case correction;
- fine: unchanged turn34 four episodes, each earning against P;
- null: the same hierarchy with odd repeat-rank counts 1,3,5 exchanged
  between E1/E2 and between E3/E4 at every address; NEW and even ranks remain.
  This preserves both parent count vectors and archive size exactly while
  changing the fine within-case structure;
- pooled: unchanged pooled-permission incumbent;
- cold: P0.

The candidate carries u, two case wealths and four detail wealths: 56 bytes
per address / 1,344 recipient bytes. Fine carries 40/960; coarse 24/576;
pooled 8/192. These are recipient router states, reported separately from
portable bytes, shared prediction scratch space and immutable decoded counts.
The null carries the same 1,584 portable and 1,344 recipient bytes as hier.

## Fresh experiment and fixed gate

Worlds 320–327, namespace `netta-parent-relative-detail-v1`. Inherited five
16 KiB regimes: recombined, partial, switched, moved_mid, unrelated. Four
16 KiB source lives per world. Changed tails are [8192,16384); early is
[0,4096). Controls consume exactly the same byte stream. Nothing from a
recipient affects source selection, count splitting or the null transformation.

All comparisons below are mean received log2 probability gains, with strict
positive paired differences counted as wins. Fixed gates:

H1: switched tail hier−coarse > 6.72 bits, wins >=5/8, and switched whole
hier−coarse >=0. The 6.72 is the inherited .01-bit price times 672 added
portable bytes; measured bank lengths must verify that delta.

H2: switched tail hier−fine >1 bit and wins >=5/8, at equal portable bytes.

H3: recombined early and whole hier retain >=99% of the corresponding
positive coarse means; hier whole positive 8/8; partial-tail hier−coarse
mean >=0.

H4: switched-tail hier−null >1 bit and wins >=5/8, at identical parent
counts and equal portable and recipient bytes.

H5: an independent reader reconstructs source counts and archive bytes,
parent/detail wealth, normalization, quotes, permission, shared admission,
outer trajectories, metrics and all gates within 1e-7; prefix gain >=−1−1e-7
and drawdown <=16+1e-7 in every arm/life. First visits, nonpositive-detail
silence, nonpositive-case silence, NEW/equal/no-match and source invariance
are checked. Descriptive zero/equality labels use recorded C values only
after their continuous coordinates have been independently reconstructed,
following the disclosed turn33/34 convention.

Material acceptance requires all H1–H5. Preserve every failed gate and every
losing world. A correct FAIL completes this experiment. No threshold, world,
archive, score window or mechanism changes after freezing.

## Evidence and handoff

Before targets: strict C11 build, a handcrafted independent probability-space
fixture exercising both levels, locality, recovery, first/NEW/equal/no-match
quotes; then hash-pin this protocol, code, reader and inherited dependencies.
One fresh batch and one complete independent replay. Show mechanically chosen
help/harm bytes on the switched tail beside wealths, counts and raw byte
neighborhoods. Report activation and revocation of detail, whole and tail
gains in all regimes, all admissions and all portable/recipient sizes.

Inherited HEAD256/P0 and greedy source selection are pinned supplied
boundaries. Work remains under turns/turn36 plus the project log and shared
handoff to Sol. The merged mouth is independently replayed as incoming work;
this memory experiment does not install itself into it. Commit/push awaits
Oleg's publication instruction. Return the next turn to Sol.
