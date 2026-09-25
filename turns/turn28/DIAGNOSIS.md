# Turn28: the cost of breadth and the value still to isolate

**Post-hoc, saved forecasts only.** Run `python3 -B turns/turn28/diagnose.py`.
It reads 40 retained recipient traces, source BOOKS, and RESULT; it does not
construct another predictor or rerun the independent reader. DIAGNOSIS.json
contains input hashes, all 1/4/8/16 KiB and tail decompositions, per-world
partitions, source differences, and selected raw records.

## The preserved result

T1/T5 pass; T2/T3/T4 fail. Local memory has positive full recombined gain in
8/8, and beats global and permuted in early gain in 8/8. It loses early to
the full-budget pooled archive in 8/8 and retains only 58.671642% of that
archive's full mean gain. Partial-tail mean local−full is +37.607951 bits,
but only 4/8 tails improve and whole partial mean loses 544.144199 bits.

## Exact additive accounting

For every saved byte, `local−full = (local−small)+(small−full)`. All values
below are mean **received** bits across eight lives. Maximum aggregate
identity error is 1.71e−12 bits.

| Regime / window | Local − small | Small − full | Local − full |
|---|---:|---:|---:|
| Recombined, first 4096 | +53.443876 | −287.933690 | −234.489814 |
| Recombined, whole | +100.134367 | −1543.715941 | −1443.581574 |
| Recombined, tail | +7.580764 | −869.410188 | −861.829424 |
| Partial, whole | +420.066548 | −964.210748 | −544.144199 |
| Partial, tail | +327.512946 | −289.904995 | +37.607951 |
| Switched, whole | +340.912680 | −661.284574 | −320.371894 |
| Switched, tail | +248.359077 | +13.021179 | +261.380256 |
| Moved, whole | +258.177897 | −1173.322346 | −915.144449 |
| Moved, tail | +165.624294 | −499.016593 | −333.392299 |
| Unrelated, first 4096 | +70.713185 | −0.115847 | +70.597338 |
| Unrelated, whole | +154.011013 | −0.115847 | +153.895166 |

All four recipients with the shared first 8192 bytes have the same early
prices. The bank improves on the smaller pooled control, but that gain is
much smaller than the sacrificed coverage on unchanged lives.

**Identification boundary:** local−small combines separate A/B contents,
local P0 selection, and fixed-share revision. It is not the isolated causal
value of preserving two different books. Small−full changes the selected
coverage and consequently admission/outer history. Both are actual saved
controls; this is not a replay with new weights or modified records.

## Where the coverage price occurs

Classify by matches formed before truth: full has a match but bank does not;
full has a longer match; both select the same prefix; neither matches.

| Early recombined partition | Mean bytes | Local − small | Small − full | Local − full |
|---|---:|---:|---:|---:|
| Bank missing | 597.750 | 0 | −171.725716 | −171.725716 |
| Full longer | 251.500 | +20.204419 | −84.239947 | −64.035528 |
| Same prefix | 2284.875 | +33.239457 | −31.968026 | +1.271431 |
| Neither | 961.875 | 0 | 0 | 0 |

Pooled-small and pooled-full **candidate** prices are exactly identical on
every same-prefix row (maximum error 0). Their live prices there can differ
because earlier coverage changed admission and authority. Candidate early
small−full loses 307.312036 bits, entirely on missing/longer contexts.

Over the complete recombined life, bank-missing rows alone lose 1026.668249
received bits to full; full-longer rows add 377.718644 local−full loss. On
the partial tail, same-prefix rows instead give local **+233.393795** bits
over full, while missing/longer rows cost 127.402589 and 68.383255. Thus a real
within-covered-context improvement coexists with the breadth loss.

## Raw forecasts: selective use and its limits

All examples below are recombined, with zero-based byte positions. Complete
histories, heads, counts, weights and prices are stored in JSON.

- **Missing experience:** world 270, t3392, truth 66, rank 1. The previous role
  history ends `322`; full record 14 stores prefix `[3,2,2]`, counts
  `[857,1167,67,66,0,0,0]`. Bank has no match. Local and small charge
  log2 P=−6.454607; full charges −0.247683. Actual loss: **−6.206924 bits**.
- **A on one prefix:** world 271, t11050, record 8, prefix `[0,2,1,3]`, truth 73.
  Counts A=`[17,997,27,25,0,0,0]`, B=`[20,10,4,0,0,0,0]`.
  Pretruth local weights `(P0,A,B)=(.000277,.997900,.001823)` favour A;
  A/B truth prices are −1.415637/−1.902829. Local charges −1.423997 versus
  global −1.819307. The full pooled archive is already close at −1.427430.
- **B on another prefix in that same life:** world 271, t8242, record 2,
  prefix `[1,0,3]`, truth 84. A=`[47,17,17,27,0,0,0]`,
  B=`[53,95,2114,45,0,0,0]`; local weights
  `(.002643,.004422,.992935)`. A/B prices −2.636664/−.882117;
  local −.891198, global −4.612934, full −.910830.
  These rows show different useful local selections. Their very large
  margins against small also include its absence of admission in this life;
  they must not be presented as pure gains from distinct count vectors.
- **Wrong confident advice:** world 271, t10642, record 10,
  prefix `[2,2,0,2]`, truth 73 as rank 2. Weight on B is .999103, but its counts
  `[28,33,14,998,0,0,0]` favour rank 3. Local charges −6.289768 versus
  P0 −1.069900: **−5.219868 bits**. Full also errs at −6.214673. Local
  selection can confidently choose an inapplicable continuation.

## The source arrays are usually different

Across 96 bank records, no pair has identical count vectors. Seven have an
empty case. For the 88 records with repeat support in both cases, total
variation between their source-only KT distributions on ranks 1..6 has
median **.408677**, quartiles **.176500/.616994**, range **.003453–.940809**.
For 89 records with both nonempty all-role distributions, empirical TV has
median .456381. These are descriptions of source data, not target selection
scores or proposed cutoffs.

Therefore this batch does not support the simple explanation that uniform
duplication mainly stores nearly identical distributions. Difference also
does not establish usefulness: recipient k can remove source-supported
roles, local P0 may be better, and retained prefixes may be less valuable
than omitted ones. Sparse exception storage is an untested economic idea,
not a consequence already demonstrated by this diagnosis.

## Unrelated admits and the next question

Local admits on 6/8 unrelated lives; all six final gains are positive,
48.227886–353.823716 bits. Worlds 267/268 remain exactly cold. There are no
negative final local worlds; the worst local whole-prefix minimum is
−.995436. Global admits 3/8, including world 264 ending −.989082. Small admits
only world 271 and ends −.926774; full and permuted never admit.

“Unrelated” replaces commands with independent uniform 0..3, while retaining
the same emitter/HEAD machinery (`turn13/experiment.py:89–99`,
`turn13/episode.c:397–409`). It does not make raw bytes independent or remove
all shared repeat constraints. The 153.895166-bit local mean and 262.972990
switched-tail mean merit measuring what the local calibration itself buys.

**One next question:** does per-prefix calibration of the existing full 24-record
pooled archive retain these adaptation gains without sacrificing half its
coverage? A precise fresh-data ablation is the collapsed A=B law:

```
Q = (1-u)*P0 + u*P_pooled,             initial u=7/8
u_next = (1-2^-10)*u*P_pooled(truth)/Q(truth) + 2^-10*(7/8)
```

Use all existing pooled prefixes, the same outer law, and matched-visit
clock. Compare actual live results with bank12 and unchanged full24. This
keeps the 528-byte source archive and needs one local scalar per record; it
tests whether the useful change is largely recipient calibration. It has
**not** been run here. Choosing sparse storage for distinct alternatives
should follow evidence that those alternatives provide additional value.
