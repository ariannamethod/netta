# Turn31: preserve the conditional case while withholding its permission

Astra, 2026-09-26. Base `bbd8dbb41f88d5a6697ca03533176bdbbfa6ee49`.
Isolated branch `astra/turn31-case-mixture`. Incoming audit, one new C organ,
one preregistered fresh batch, independent reconstruction, then return to Sol.

**Material FAIL F1. F2/F3/F4 PASS; independent verification PASS.**
The changed conditional revision law helps six of eight partial tails by
0.742642 bits on average, below the declared >1-bit margin. It is a small
measured effect and does not warrant replacing the incumbent.

## Audit of Don30

One pristine run of his independent reader against retained source/recipient
artifacts reconstructed 3,276,800 forecasts and 2,688 counts. The receipt is
byte-identical to the sealed VERIFY; all 28 freeze pins match. His D1/D3 FAIL
is unchanged. The separate [audit](audit/AUDIT30.md) records exact scope.

Three explanations require narrowing:

1. D1's partial early4096 lies before the8192 manipulation. Don disclosed
   this. The actual24 paired trace prefixes through8192 are identical.
2. A/P0/B starts with total source mass7/8, exactly the pooled binary prior.
   A proposed pooled-half prior7/16 changes that total; it does not isolate
   discrimination from weaker commitment. Equal source forecasts collapse
   the three-way update to the binary7/8 update exactly.
3. The largest helpful byte shows retreat to P0, the harmful byte shows a
   wrong case preference. Those two events do not decompose the entire
   switched/unrelated benefit or close distinct histories as a family.

The inherited result is numerically sound; there was no demonstrated code
mismatch to repair. No old report, protocol, reader or raw result was edited.

## Restriction and new mechanism

In [Don's flat update](../turn30/router3.c#L55), independent source weights
receive a fixed share after each matched observation. Re-express them as
u=wA+wB (permission) and v=wA/u (conditional preference). Their implicit
conditional reset is lambda=rho*u0/u_next. As permission collapses, lambda
can approach1, largely erasing which old case was comparatively preferable.
The derivation and costs are in [DESIGN.md](audit/DESIGN.md).

The new [C organ](case_router.c#L203) keeps u and v separately:

```
S = v*PA + (1-v)*PB
Q = (1-u)*P0 + u*S
u_next = (1-rho)*u*S(x)/Q(x) + rho*(7/8)
v_next = (1-rho)*v*PA(x)/S(x) + rho/2
rho=2^-10; initial u=7/8,v=1/2
```

Only the conditional reset law changes relative to flat3. Each prefix owns
its state; after-truth updates use both OLD values, including NEW/equal
visits. No match does not tick. State resets between lives, immutable source
archives do not absorb recipient observations. All256 quotes precede truth.
When permission is high this reset can be slightly stronger than flat3's;
this is decoupling, not uniformly slower forgetting.

Same 12 source prefixes and 528-byte two-case archive. The candidate uses 192
recipient bytes against flat3's 288. Static-balanced and pooled each use 96;
pooled's single-vector archive costs 336. Full24's 528-byte Sol29 incumbent
is retained and not rerun/replaced in this narrower same-address study.

## Controlled comparisons

Six simultaneous arms consume the same causal HEAD256/P0/history tape:

- factored: new two-clock organ;
- flat3: unchanged Don30 three-state law;
- balanced: fixed equal average of NORMALIZED PA/PB distributions, binary
  local permission with the same7/8 prior/share; initial predictive shape
  matches flat3, then adaptive A/B selection is absent;
- pooled: turn29 binary local permission on the same12pooled records;
- permuted: factored with independently rotated repeat slots in both books;
- cold: P0, with zero source influence.

All six share admission from the pooled candidate's32-bit shadow, effective
on the next byte. Each then earns its own slow outer-HMM history. This fixes
entry timing; it does not establish independent early admission or assume
shared admission must favour one arm. Protected NEW remains exactly P0.

Protocol and code frozen before worlds288..295 under
`netta-conditional-case-v1`. Four16KiB source lives and five16KiB recipients
per world. Source grouping, grammar, greedy record selection and all five
regime generators are inherited unchanged. Post-change claims use bytes
[8192,16384). No parameter, world or gate was changed after generation.

## Result

Means of received bits saved against P0; full table and every world in
[TABLES.md](TABLES.md).

| Regime/window | factored | flat3 | balanced | pooled12 |
|---|---:|---:|---:|---:|
| intact first4096 |411.166452|410.774001|324.993377|417.769898|
| intact full |1896.255342|1895.409492|1494.302932|1913.009110|
| partial tail |477.374630|476.631987|467.681088|527.677850|
| partial full |1375.474157|1374.123942|1171.653631|1430.349392|
| complete-change tail |294.425984|294.605514|309.713817|275.140332|
| moved-surface tail |616.963455|616.458835|517.587211|635.936964|
| unrelated full |136.424671|135.435248|139.954876|133.135024|

| Declared condition | Actual result |
|---|---|
| F1 partial-tail mean>1 and wins>=5/8 against BOTH controls |FAIL: vs flat3 +0.742642,6/8; vs balanced +9.693542,5/8|
| F2 intact/whole partial retention>=95%,8/8intact positive |PASS: intact early98.4194%,full99.1242%;partial full96.1635%|
| F3 factored whole mean>permuted in each regime |PASS; null influence and losses retained|
| F4 causal/numeric/source reconstruction and inherited bounds |PASS, independent reader|

Partial-tail factored-minus-flat3 by world288..295:
`+1.583473,+1.672810,-0.135724,+1.241599,+0.176262,+3.943158,+0.677171,-3.217612`.
The two losing worlds remain; the1-bit boundary remains where it was.
Compared with pooled, factored loses50.303221bits on mean partial tail and
54.875236on whole partial life. Passing retention is not superiority.

The matched balanced control resolves an important part of the incoming
question. Flat3 beats it by401.106560bits on intact whole lives and85.780624
early, while its changed-law tail is15.108303bits worse and unrelated whole
4.519628worse. Thus adaptive case selection does have measured value relative
to a fixed equal-case mixture in this family. Count pooling is a different
predictive shape and remains stronger on ordinary ground. This does not
show that storing all distinct records earns its added portable bytes.

## What happens in actual predictions

[RAW.md](RAW.md) preserves the mechanically selected largest help and harm,
with actual bytes, prefix counts, quotes and pre/post states. Both occur in
world292/recombined:

- At t1456, both routers prefer B although A better predicts truth. Factored
  has permission .4074, flat3 total memory .9709. Lower permission saves
  **3.271381bits** on the charged forecast.
- At t1518, B now predicts truth well, and both conditional preferences favour
  it. Factored permission is only .00836, flat3 total memory .2042. The
  withheld useful advice costs **2.550876bits** relative to flat3.

These events show the downstream permission/recovery tradeoff. They do not
by themselves assign the entire mean to a single mechanism. The saved-trace
[diagnosis](DIAGNOSIS.md) examines the declared low-permission and
both-source-worse subsets without evaluating another predictor.

That diagnosis confirms that the conditional preferences actually diverge.
On the 216 partial-tail rows where flat permission is at most 7/4096,
mean absolute preference difference is 0.412566, but each arm gives memory
only about 0.0013 of the live forecast. The large state difference has little
immediate influence there. Across the entire partial tail, candidate gain
is +0.733971 bits and received gain +0.742642; absolute bytewise received
differences sum to 96.337897 bits per life. Help and harm largely cancel.
The outer HMM is not hiding a large positive candidate result in this trial.

## Verification, reproduction and exact limits

Strict C11/O2/Werror build. One handcrafted-price fixture: 20 events,
360 independent checks, maximum 3.55e-15. One new batch, all stages rc0,
no post-freeze repair. Independent reader rc0 in 165.22 seconds:
**3,932,160 forecasts**, **2,688 source counters**, exact reconstruction of
all 32 archives; maximum numeric discrepancy **1.2590817277668975e-11**.
Lowest prefix gain −0.999553968; greatest drawdown 13.564007050, within the
unchanged −1/16 limits. [Reader](READER.md), [receipt](VERIFY.json),
[reproduction](REPRODUCE.md), [build](BUILD.md).

The reader inherits greedy source selection and HEAD256 cold/bindings as
pinned supplied boundaries. It independently recounts continuations,
reconstructs archives and matching, all router/source/received prices,
shared admission and metrics. Functional recognition, automatic discovery
of source families, natural text transfer and the50-life curve are still
outside this experiment. Canonical Netta, mouth and mycelium are unchanged.

## Next hand: Sol

Audit this committed-when-authorized step and choose one bounded next move.
The two-clock change is not adopted: sub-bit partial gain is insufficient,
and pooled remains the practical same-address baseline. No further rho/prior
sweep was run or is implied by this FAIL.

A useful next question is whether distinct histories can be introduced as
an earned correction to the pooled forecast, with INITIAL PREDICTIONS exactly
matching that strong baseline. Current equal-case initialization and
count-pooling differ in source-support weighting and KT smoothing before
any recipient case choice occurs. That distinction should be explicit in a
new construction and fresh comparison. Preserve the full-address pooled
incumbent and charge any additional archive bytes. This is a proposal for
Sol, not another implemented or evaluated candidate in turn31.

Complete locally, not committed or pushed. All new scientific work under
turns/turn31; no live integration. Rotation Sol -> Don -> Astra -> Sol.

## Identities

- PROTOCOL.md: `42a30609026a65c92b0e4e41eef948762dfd8ca13c09b581e8575c72556efde2`
- FREEZE.json: `75c2250000fa6588e9900eb5859689d671597fd393e0016d011028911542fb42`
- RESULT.json: `18ae447a51de12cf1470c07a73f3d254beff2bb17869842d3e7e931d9b802841`
- VERIFY.json: `73fffe23b9f0efb84e968921b6ece637c46e478f5db80894fc99ab01573c12fc`
- case_router.c: `e94d42455f653e46b6ef206fcc6dea22b4df66fc27a63d9db61cdce6450b354e`
- verify.py: `b60bfb410499dc3a5e3f0b4c3d8661375fb16e21892a69b5ee0737b62c059869`

## Publication follow-up — 2026-10-03

Oleg explicitly requested commit and push of this completed turn. The local
completion statements above record the September 26 state. Before publication,
all 22 frozen files and the RESULT/VERIFY identities were checked unchanged.
The publication branch is `astra/turn31-case-mixture`; Oleg will merge it.
Oleg now routes turn32 to Don, who is already working on it, superseding the
original next-hand routing to Sol. This publication adds no new experiment.
