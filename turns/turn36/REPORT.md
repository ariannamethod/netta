# Turn36: parent-relative detail preserves the coarse forecast and fails its price

Astra, 2026-10-05. Base main
`4c90cf36e50a8dfc5ae2f0ed59e34267667c4543`; branch
`astra/turn36-earned-granularity`. One fresh batch, worlds 320–327,
`netta-parent-relative-detail-v1`. **Material FAIL: H1/H2/H4 fail;
H3/H5 pass.** The candidate is not adopted. All thresholds and worlds
remain as frozen before generation.

## Incoming hand

Don34's accepted object reproduced: the reader receipt is byte-identical,
3,276,800 forecasts and 6,720 source counters. Directly summing the eight
switched traces gives 4.842001395211258 bits at 6/8 against 6.72, confirming
G4 FAIL. The staged turn33 source and its independently built fixture agree.
Recorded discontinuous labels follow independently reconstructed coordinates.

The fourth requested check remains unresolved: the original first defective
batch was not located, so its claimed equality to the accepted batch is not
certified. The surviving accepted batch is independently valid. Full details,
raw commands and receipts: [audit/AUDIT34.md](audit/AUDIT34.md).

The merged Sitting 2 mouth also replayed: all three C-reader reports and all
15 speech streams match the sealed bytes. Every stream is shown in
[audit/MOUTH_RAW.md](audit/MOUTH_RAW.md); the audit links its numerical reports.

## One mechanism

The two coarse cases keep their turn33 wealth relative to pooled memory.
Each of four episodes now earns a local detail wealth against its own parent
case. Inside A and B, positive detail wealth refines that parent's prediction;
the existing coarse-case wealth then combines those refinements with pooled
memory. Permission, admission, source selection, emitter and the slow outer
law remain inherited.

At nonpositive detail wealth the source prediction equals the coarse source
exactly. At nonpositive coarse wealth the entire case and its details fall
silent. All states reset at each life. Six simultaneous arms: hierarchy,
coarse two-case, flat four-episode, parent-preserving scrambled detail,
pooled and cold. The scrambled archive exchanges odd repeat-rank counts
inside each sibling pair, preserving both coarse parents exactly.

The single four-episode portable archive is 1,584 bytes, versus 912 coarse
and 528 pooled. Coarse archives are exact count projections checked by the
reader; the runner loads them as redundant comparison inputs. The candidate
adds 672 portable bytes over coarse, priced at 6.72 bits. Recipient router
state is 1,344 bytes, compared with 960 flat-fine and 576 coarse; shared
decoded counts and measurement scratch are separate.

## Frozen gates and measured outcome

All gains below are received log2 probability differences per life. Tails
cover bytes 8192–16383, after the declared manipulation.

| gate | measured result | decision |
|---|---|---|
| H1: switched hier−coarse >6.72, >=5/8, nonnegative whole | tail +0.724454, 5/8; whole +1.429676, 8/8 | FAIL |
| H2: switched hier−fine >1, >=5/8, equal portable bytes | −1.860739, 2/8 | FAIL |
| H3: >=99% ordinary retention, 8/8 positive, partial tail >=0 | early 100.000027%, whole 100.093899%; positive 8/8; partial +6.570554, 5/8 | PASS |
| H4: switched hier−scrambled >1, >=5/8, equal bytes/parents | −0.275153, 5/8 | FAIL |
| H5: independent source/count/trajectory/gate replay | 3,932,160 forecasts; 5,376 source counters | PASS |

Switched-tail hier−coarse, worlds 320–327:

    +0.529295, −0.006707, +1.129440, +2.409646,
    +0.885678, −0.063406, +0.929032, −0.017347

The unchanged flat four-episode control adds only 2.585193 bits over coarse
on this batch's switched tail, also below the 6.72-bit price. Hierarchy loses
to it by another 1.860739 bits. Partial change has a larger positive effect
over coarse, but the frozen question requires the switched-tail gate.

| regime | hierarchical whole gain vs P0 | hierarchical tail | coarse tail | flat-fine tail | scrambled tail |
|---|---:|---:|---:|---:|---:|
| recombined | 3878.433654 | 2165.084212 | 2162.151044 | 2168.202160 | 2166.277710 |
| partial | 2604.933377 | 891.583935 | 885.013381 | 898.545957 | 891.769583 |
| switched | 1866.427404 | 153.077962 | 152.353508 | 154.938700 | 153.353114 |
| moved surface | 3004.669335 | 1291.319893 | 1286.287107 | 1297.309095 | 1291.293048 |
| unrelated | 118.587911 | 43.374191 | 43.001634 | 42.711201 | 43.510585 |

Unrelated admission opened in four worlds: 320,322,324,326. All six arms
shared each admission, and all gains/losses remain in RESULT/VERIFY.

## The raw behavior

[RAW.md](RAW.md) shows mechanically selected largest help/harm bytes against
both coarse and flat-fine controls, with raw neighborhoods, source counts,
pretruth wealth, permission, candidate and received prices.

Against coarse, maximum help is world326/t9885, **+0.626669 bits**;
maximum harm is world322/t12529, **−0.982987 bits**. In the latter, parent B
has positive wealth 4.150, but detail E4's wealth 146.607 overwhelms E3's
27.199. E4 has only one repeat observation, at rank3; truth is rank1, where
the pooled parent contains three observations from E3. Its candidate price
falls from coarse −2.727214 to hierarchical −3.714309.

The largest loss against flat-fine is world326/t10766, **−2.830544 bits**.
Both coarse cases are silent (hA=−2.156728, hB=−2.584375), so the hierarchy
equals pooled despite positive detail wealth. Flat-fine has earned E4
wealth +1.175003 and predicts the true repeat rank3 much better:

    truth=106, rank=3, prefix=2031
    hierarchical candidate −4.869260888; received −4.866194028
    flat-fine candidate     −2.034745488; received −2.035649823

This byte exposes the parent veto directly: a useful episode can be silenced
with its broader family. It is one observed source of loss, with the full
loss distribution retained beside it.

The largest help against flat-fine, world323/t8976, **+2.522692 bits**, comes
from different outer histories: candidates are nearly equal and both poor
(−6.862263 versus −6.855835), while hierarchical outer log2 odds are −1.768711
versus flat-fine +4.049995. The inherited outer layer gives the former more
current local probability on that truth.

The saved-trace diagnosis prices no alternate predictor. On the entire
switched tail, candidate hier−coarse is +1.643557 bits, received +0.724454,
while absolute bytewise received differences sum to 19.859740 bits per life.
Help and harm compensate. The candidate-level total also falls below 6.72.
Positive detail sits behind at least one silent parent on 31,392 of 65,536
tail positions; those descriptive subsets overlap and are not an attribution
of total causal effect. [DIAGNOSIS.json](DIAGNOSIS.json) preserves the counts.

## Verification and next hand

Strict C11/Werror build; 80-event fixture, 3,680 checks; one integration
preflight on already-open Don34/world304; 39 frozen files; one fresh batch;
one independent complete replay. Maximum replay error
**3.2528646443097387e-11**. Lowest prefix −0.998065074, largest drawdown
13.923688480, inside the inherited −1/16 bounds. No post-freeze repair,
parameter change, second candidate or second fresh batch occurred.

Next: Sol audits this hand, especially the parent-preserving null, exact
silence, byte accounting and the raw parent-veto event. A bounded follow-up
question is whether an episode should earn a correction against the current
coarse forecast directly, retaining the coarse starting point while allowing
an individually useful episode to speak after its parent falls silent.
That construction has not been implemented or measured here. The negative
result also keeps source-defined episode boundaries and sparse-count wealth
as open questions.

This is exact-address synthetic transfer work. All speech shown is the
independently reproduced existing mouth; turn36 stays isolated from live
Netta. Complete locally, uncommitted and unpushed. Next hand: Sol.

## Identities

- PROTOCOL: `d661ec3964cc08aa0ae1c2987fa5e00743cd60663d9e382719114ec9fbcfd6d5`
- FREEZE: `d609245acf05a7c12e0f365ec97061c022b2a9194e2836a437ccc23fbbcad450`
- RESULT: `1304048b450bd714a4640ac7a91f7ae320eb9305cd29fbb718a344ee4bf4d2c0`
- VERIFY: `d87169b84d2114455adb0567d57f1ccf91788549e988e88b00809152a164cf5a`
- C: `8a59d69cad523fac9250b2b65e283b7926c1aa120b478fa9cb18e92699b9503f`
- reader: `6c671118aefb904696ed21765990a7a7e0d3566c692cd341b459d7e94cbc8fd1`

## 2026-10-05 publication and routing follow-up

After the local hand was complete, Oleg explicitly authorized its push and
assigned the next microphone to **Milla**. Publish this snapshot on
`astra/turn36-earned-granularity`; Milla receives the four-point incoming
audit previously addressed to Sol. The earlier completion status above
records the pre-publication state. All 39 frozen identities and the saved
RESULT/VERIFY hashes were rechecked unchanged. No new experiment or live
integration is part of this publication. The shared handoff records the
published commit after remote verification.
