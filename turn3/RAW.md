# BANK2-ROW — raw behavior and all eight worlds

All gains are cumulative log2 probability ratios against P0; positive means fewer bits charged than the same local learner. No natural-language generation is claimed. Full traces and every arm are retained under results/, with hashes in ARTIFACT_MANIFESTS.json.

## Fixed batch, all paired gains

|World|Row|Global|Pool|Best null|Row−pool|
|---:|---:|---:|---:|---:|---:|
|32|130.340426|0.000000|0.000000|0.000000|130.340426|
|33|203.592238|0.000000|0.000000|0.000000|203.592238|
|34|143.354265|0.000000|0.000000|0.000000|143.354265|
|35|160.816670|0.000000|2.178681|0.000000|158.637989|
|36|189.482874|0.000000|0.000000|0.000000|189.482874|
|37|183.856930|0.000000|0.000000|0.000000|183.856930|
|38|164.351989|0.000000|0.000000|0.000000|164.351989|
|39|185.808543|0.000000|0.000000|0.000000|185.808543|

## Early horizons and changed tails

Admission is the number of observed bytes when the joint shadow first crosses32 bits. The crossing byte is still priced by P0. A tail is the last8192 bytes of the switched life; drawdown is the largest fall from any preceding gain peak.

|World|Admission|G1024|G4096|G8192|Changed tail|Changed minimum|Changed drawdown|
|---:|---:|---:|---:|---:|---:|---:|---:|
|32|568|1.931826|35.613447|82.790646|23.839955|-0.939046|10.334055|
|33|443|26.847855|106.485626|172.035685|-5.837505|-0.983469|9.239958|
|34|293|34.564709|86.813763|123.556956|2.331242|-0.450307|8.438764|
|35|952|-0.352212|32.990327|157.126137|-8.907355|-0.960080|9.285988|
|36|474|21.951492|72.093318|155.470232|-2.530992|0.000000|10.930658|
|37|431|14.903048|91.569696|175.856713|-6.924810|-0.139692|9.311723|
|38|359|15.946865|46.385433|103.410260|-6.239558|-0.678807|10.327068|
|39|516|1.726841|22.190768|136.054934|0.926234|-0.990796|9.624154|

All unrelated lives stay equal to P0 in every arm. Pool is admitted in world35 after8245 bytes on mosaic (gain2.178681), and after8222 bytes on the changed life (gain3.367961). Other global/pool/null lives in those regimes remain unadmitted. The changed-tail mean -0.417849 hides five negative tails offset partly by world32; all are shown.

## Actual routing exposure and source coverage

These are raw-last6 generator routes, independently recounted from actual bytes. They are provenance, not oracle labels for learned-unit HEAD256 contexts. First six bytes have no full raw context; A+B+shared=16378. Each source book contains two16KiB lives.

|World|A visits|B visits|Shared visits|A rows with support≥32|B rows with support≥32|
|---:|---:|---:|---:|---:|---:|
|32|4745|4412|7221|145|176|
|33|3923|4852|7603|151|164|
|34|3080|6746|6552|145|151|
|35|4819|3945|7614|155|163|
|36|3835|5129|7414|144|161|
|37|5369|3729|7280|147|155|
|38|3594|5363|7421|152|152|
|39|4193|4792|7393|175|155|

## Hindsight diagnostic, never supplied to the predictor

Candidate scores before outer admission/HMM. Best per row is an unavailable hindsight choice, not a live comparator.

|World|Best whole book|Best book per row|Actual inner mixture|Selection cost|Visited rows|
|---:|---:|---:|---:|---:|---:|
|32|-372.103343|297.360652|163.704525|133.656127|203|
|33|-419.511976|357.374931|232.842874|124.532057|201|
|34|-294.132144|293.603323|176.820031|116.783292|203|
|35|-295.000226|303.570796|177.405322|126.165474|203|
|36|-260.923123|345.445857|223.342939|122.102918|202|
|37|-301.460621|323.390401|197.758568|125.631833|203|
|38|-416.190503|321.398659|198.026556|123.372103|202|
|39|-332.684497|352.072133|219.752934|132.319199|202|

## Actual byte witnesses

Illustrations selected after scoring, without changing the gate: in the first world32, the first active positive-gain event at t≥1024 with log2(B/A)≤−3 for A and ≥3 for B. The harm example is the most costly row event after the switch in world35, the world with the worst endpoint tail. These examples illustrate behavior; the verdict uses every world and event. All t are zero based.

Each listed unit expansion is hexadecimal. All six expansions concatenate to an actually observed suffix before the truth byte. A HEAD row names equality of their first bytes, not equality of whole learned units. Counts list repeat groups0..k−1 followed by NEW. [Exact source records](evidence/RAW_WITNESSES.json).

### A: world32, mosaic, t=1027

Observed past units: `65,68,6f,0f,0f,11`. Learned inventory: 266 units from the first 1024 bytes. Context heads: `101,104,111,15,17`; row `012334`. Truth: decimal104 / hex68, group1.

Source counts A: `[7, 164, 4, 10, 14, 115]`; B: `[4, 25, 4, 57, 125, 94]`.

|P0(truth)|P2 A(truth)|P2 B(truth)|Bank(truth)|Live(truth)|
|---:|---:|---:|---:|---:|
|0.305115776|0.314993723|0.147685985|0.314991485|0.314679057|

B's weight before truth: 0.000013377306; after charging it: 0.000006272044. Inner log2 odds -16.189833569 → -17.282623730. Outer log2 odds before truth: 4.935910198. This event's live gain: +0.044524390 bits.

### B: world32, mosaic, t=1088

Observed past units: `ba,ba,78,67,ba,24`. Learned inventory: 266 units from the first 1024 bytes. Context heads: `186,120,103,36`; row `001203`. Truth: decimal103 / hex67, group2.

Source counts A: `[78, 9, 8, 9, 42]`; B: `[9, 9, 89, 8, 59]`.

|P0(truth)|P2 A(truth)|P2 B(truth)|Bank(truth)|Live(truth)|
|---:|---:|---:|---:|---:|
|0.239607746|0.083006220|0.269761614|0.269318486|0.266432375|

B's weight before truth: 0.997627224999; after charging it: 0.999268690812. Inner log2 odds 8.715781723 → 10.416175450. Outer log2 odds before truth: 3.216359796. This event's live gain: +0.153094854 bits.

### harm: world35, mosaic_then_unrelated, t=8417

Observed past units: `fc,fe,9b,fefe,fefe,fe`. Learned inventory: 463 units from the first 8192 bytes. Context heads: `252,254,155`; row `012111`. Truth: decimal254 / hexfe, group1.

Source counts A: `[16, 5, 4, 20]`; B: `[25, 3, 6, 20]`.

|P0(truth)|P2 A(truth)|P2 B(truth)|Bank(truth)|Live(truth)|
|---:|---:|---:|---:|---:|
|0.194614145|0.093662711|0.063077515|0.088001151|0.090730014|

B's weight before truth: 0.185107869666; after charging it: 0.132681724516. Inner log2 odds -2.138242867 → -2.708591841. Outer log2 odds before truth: 5.250531365. This event's live gain: -1.100964781 bits.

The first two records show one recipient choosing different source histories on different patterns. The third shows both available books are worse than P0: choosing the better of the two still hurts. The learned expansions fefe in that context also show why generator raw routes cannot be treated as exact labels for all frontend contexts.

Further independent recounts of admission peaks and all detailed tables: [measurement reading](evidence/RESULT_READING.md).
