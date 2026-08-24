# NETTA TRANSFER COURT 3 — B-ONLY PREREGISTRATION (2026-08-25)

Court 2 and its diagnostics are closed history (protocols in git
under their hashes; the carrier's ghost failure and the matcher
autopsy are recorded and are not rewritten). This court asks one
question: does the frozen recognizer B, converted to a hard partial
map, turn recognition into useful carried memory. Any change after
the first measured run is a new experiment, named aloud.

## Standing laws

1. **Match is not authority.** The greedy B-map only PROPOSES a
   correspondence. A matched pair receives no live influence; all
   carried cargo is priced in shadow and earns local prospective
   authority under the verbatim court-2 earn/revoke law: ledger in
   bits, EARN 32, enter at L = 0.05, live clamp [0.01, 0.5], eta
   0.05, revoke below 16 with hysteresis. Global experience is never
   local authority.
2. **The oracle never enters construction.** The map is built from
   alive source/destination sets and B-scores only. The
   oracle-matchable set and the true permutation are evaluation
   instruments; they touch neither matching nor cargo. The oracle is
   an upper bound for the mechanism, never a guaranteed win of the
   past-world model.
3. **Remapping semantics: map-epoch authority.** The map is
   recomputed at every chunk boundary from the lived prefix, never
   inside a chunk. Authority is arm-level (per-pair ledgers remain a
   named candidate for a later court), so the no-inheritance promise
   is enforced by epochs: ANY change in the matched-pair set at a
   boundary (a remap, a new match, a dropped match) opens a new map
   epoch, and at the epoch boundary the traveller arm's live weight
   L resets to 0 and its shadow ledger resets to 0 — the old ledger
   values remain in the evidence as history only, and earning starts
   from zero under the standing earn/revoke law. A boundary where
   the matched-pair set is unchanged continues the current epoch.
   Epoch openings, remap counts and map sizes are telemetry.
4. **Development-selected map needs fresh confirmation.** Greedy
   1-to-1 was chosen after seeing oracle diagnostics on W-iso, so
   every transfer-G on the development worlds is DEVELOPMENT
   evidence. The learned-transfer claim requires, after builder and
   independent verifier are both frozen and hashed (court-2 A7
   blindness order verbatim, including hash-driven world selection),
   one untouched confirmatory cipher/isomorphic world. The final
   claim is scoped to the confirmatory class actually drawn.

## The recognizer (frozen, earned in diagnostics)

B[s][d] = -( JS(sortdesc P_A^R(.|s), sortdesc P_D^R(.|d))
           + JS(sortdesc P_A^L(.|s), sortdesc P_D^L(.|d)) ), bits;
KT rows (cnt+0.5)/(N+128) on both sides; destination side computed
from the lived prefix only. Machine laws L2 (synthetic self-max) and
L4 (permutation equivariance) are carried as fixture probes.

## The map (frozen)

Greedy global one-to-one partial matching over alive_src x
alive_dst: repeatedly take the highest B[s][d] among unassigned
alive pairs; total order: B descending, then smaller s, then smaller
d. No Sinkhorn anywhere in the recognition path (it is retained only
as a diagnostic control artifact). No soft transport: the 2^B
row-softmax is rejected as transport law (ranking equivalent,
probability mass too diffuse).

## The cargo and the prior (frozen)

Cargo = the full byte backoff model of A-train, orders 0..3, exactly
as court 2 froze it. Transport through the hard partial map m:

- A destination context byte b maps source-ward to m^-1(b) when
  matched; an unmatched context byte terminates the context at that
  point (the prior backs off to the shorter order).
- P_prior(d | ctx) = P_S(m^-1(d) | c_S) for matched d; all unmatched
  d (alive or dead) share Sigma_{s unmatched} P_S(s | c_S) uniformly.
  Sums to 1 exactly; the verifier checks 1e-6 per position.
- Priced per unit position as the per-byte product over the unit's
  span (court-2 law); shadow/earn/revoke as Standing Law 1;
  P_final = (1-L)P_local + L*P_prior.

**Immutable dependency (repaired: "from this file alone" was false
as written, and the earlier reference pinned the wrong pre-A9
hash).** The local model, worlds (iso/plain/ghost/half/ff from
miller + the causality pair), run length 131072, chunk 1024, R2
newborn floor, R5 total-order BPE, evidence format, A1 fixtures, A2
causality gate, A3 derived null and A8 first-chunk law are inherited
verbatim from the court-2 final freeze, preserved as the pinned
snapshot file `COURT2_SNAPSHOT.md`, SHA-256
`c5a60f6643f55ef60d21a037388d63d307351d795cb9729d038cb1258857716b`
(14218 bytes; byte-identical to git 52b9744:TRANSFER_PROTOCOL.md).
The independent verifier is written from EXACTLY TWO protocol files
— this file and that snapshot — and from no code. Nothing inherited
is re-litigated here.

## Arms

1. **cold** — no past.
2. **B-traveller** — cargo through the greedy B-map.
3. **shuffled-B** — identical machinery; at every boundary the
   B-columns are permuted WITHIN the alive-destination submatrix:
   the frozen single-cycle order sigma (seed 0x0DDBA11, cycle over
   all 256) sends each alive d to the next alive symbol along the
   cycle — a derangement on the live set whenever K >= 2, preserving
   the active alphabet exactly (the earlier all-256 null is repaired
   by this law: dead-row profiles can no longer enter live
   candidates).
4. **oracle** — control, out of competition: m = pi restricted to
   alive pairs, same schedule, same pipeline, differs only in the
   map object.

## Rulers and verdict table (frozen wording)

G_N at horizons {1024, 4096, 16384, 65536}. Let M = MARGIN(16384) =
163.84 bits; all decisions read G at the deciding horizon N = 16384.
Ghost line 520 bits. Every boundary belongs to exactly one side.

**Blocking labels** (computed first, independently, per arm a and
world w; the boundary belongs to the label):
- interference(a, w) := G_a(w) <= -M;
- surface_leak(a) := G_a(ff) <= -M (a special case of interference,
  named separately).
A blocking label on arm B forbids the learned-transfer verdict for
this run; labels are always recorded alongside the primary verdict
(multi-label evidence, single primary verdict).

**Arm formulas (literal, frozen):**
- B-PASS :=
    G_B(iso) >= M  AND  G_B(plain) >= M
    AND (G_B - G_sh)(iso) >= M  AND  (G_B - G_sh)(plain) >= M
    AND |G_B(ghost)| <= 520
    AND G_B(half1) >= M  AND  |G_B(half2)| < M
    AND G_B(ff) > -M
    AND no blocking label on B.
- ORACLE-PASS (control ceiling; the oracle has no shuffled pair and
  no shuffled term) :=
    G_o(iso) >= M  AND  G_o(plain) >= M
    AND |G_o(ghost)| <= 520
    AND G_o(half1) >= M  AND  |G_o(half2)| < M
    AND G_o(ff) > -M
    AND no blocking label on oracle.
- GENERIC(w) := G_B(w) >= M AND (G_B - G_sh)(w) < M, evaluated on
  iso and plain.

**Primary verdict — first matching row, top to bottom (mutually
exclusive by construction):**

| condition | frozen primary verdict |
|---|---|
| B-PASS | **learned transfer earned (development)** — the final claim still requires the untouched confirmatory world of Standing Law 4 |
| not B-PASS and ORACLE-PASS and not GENERIC(iso) and not GENERIC(plain) | **recognition without transport** — the loss lies between the map and the cargo, not in B |
| GENERIC(iso) or GENERIC(plain) | **generic carrier effect** — frequency/smoothing, not learned correspondence |
| otherwise | **transfer not detected in this form** |

Blocking labels (interference, surface leak) are reported with the
primary verdict in every case.

Telemetry (never PASS criteria): map size and churn per boundary,
shadow ledgers and earn/revoke events per arm, unmatched-context
truncation counts, oracle-matchable set sizes.

## Two hands

Builder emits raw artifacts and grades nothing; the independent
verifier is written from exactly this file and COURT2_SNAPSHOT.md
and from no code, reconstructs worlds and
maps, reprices everything, and owns the verdict. All artifacts
SHA-256-pinned in manifests; the log cites hashes only.
