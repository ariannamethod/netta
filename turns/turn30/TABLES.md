# Turn30 retained tables

All values are received bits saved against the same local P0 on the same life.
Positive is better; the `cold` arm is the zero by construction. These are
frozen writer values, independently reconstructed by the reader. No world, arm
or regime is excluded.

## Portable byte accounting

| arm | archive | bytes per world | records |
|---|---|---:|---:|
|bank3|`bank.bin` (NETEB001, A and B distinct)|528|12|
|pooled2|`pooled_small.bin` (NETEI001, A+B)|336|12|
|permuted2|`permuted_small.bin` (NETEI001, rotated A+B)|336|12|
|cold|none|0|0|
|full24|`pooled_full.bin` (NETEI001, A+B)|528|24|

Distinctness costs exactly **192 extra bytes** in every world — one 16-byte
count vector per address — and nothing is equalized to hide it.

## D1, the question: early(4096) on `partial`

| world | bank3 | pooled2 | bank3 − pooled2 |
|---:|---:|---:|---:|
|280|411.642|425.814|−14.172|
|281|777.437|804.274|−26.836|
|282|377.693|431.058|−53.366|
|283|375.749|405.006|−29.257|
|284|297.759|304.932|−7.173|
|285|658.642|659.093|−0.450|
|286|429.340|435.379|−6.039|
|287|362.539|476.959|−114.420|
|mean|461.350|492.814|**−31.464**|

Distinctness loses the early comparison in 8 worlds out of 8. D1 fails.

Because `partial`, `switched` and `moved_mid` share the first 8192 raw bytes
with `recombined` by construction, their early(4096) values are bit-identical
to `recombined`'s — the writer and the reader both assert that identity
(`shared_prefix_identity`). The column above is therefore also the early
column of every other prefix-sharing regime.

## D3, the tax on the unchanged: `recombined`

| world | bank3 early | pooled2 early | bank3 life | pooled2 life |
|---:|---:|---:|---:|---:|
|280|411.642|425.814|1944.579|1949.786|
|281|777.437|804.274|3548.064|3583.435|
|282|377.693|431.058|1619.130|1848.600|
|283|375.749|405.006|1775.538|1874.537|
|284|297.759|304.932|1568.979|1587.772|
|285|658.642|659.093|2739.832|2804.686|
|286|429.340|435.379|2169.142|2167.347|
|287|362.539|476.959|1597.276|2158.342|
|mean|461.350|492.814|2120.317|2246.813|

Retention 0.93615 early and 0.94370 full-life, both under the 0.95 floor. D3
fails. Only world286 keeps its full life above the pooled arm, by 1.8 bits.

## Where distinctness does pay

| world | `switched` life diff | `switched` tail diff | `unrelated` life: bank3 / pooled2 / full24 |
|---:|---:|---:|---|
|280|+49.712|+47.269|0 / 0 / 0|
|281|+150.277|+178.999|0 / 0 / 17.166|
|282|+177.083|+294.835|314.255 / 73.517 / 31.913|
|283|+44.650|+61.441|450.530 / 270.268 / 280.271|
|284|+202.002|+204.594|394.929 / 192.047 / 122.440|
|285|+89.255|+84.376|0 / 0 / 0|
|286|−1.022|−1.809|0 / 0 / 0|
|287|−92.684|+151.493|396.417 / 262.475 / 202.546|

On `switched` bank3 wins the life in 6 worlds of 8 (mean +77.4 bits) and on
`unrelated` it beats both pooled2 and the 24-address yardstick in every world
that was admitted at all. The partial tail is +11.3 bits on average with 4
wins of 8 — a split, not a win.

## D4, the null

| regime | permuted2 life | pooled2 life | excess |
|---|---:|---:|---:|
|recombined|93.177|2246.813|−2153.636|
|partial|38.058|1532.778|−1494.720|
|switched|25.551|1271.976|−1246.425|
|moved_mid|53.430|1790.264|−1736.834|
|unrelated|−0.440|99.788|−100.228|

The rotated archive never approaches the pooled one on any regime. It is not
worthless, though: it earns up to 236.485 bits in a single life (world280,
`recombined`), because rotation keeps each address's total attestation and its
NEW count while scrambling which repeat role they belong to. Early and tail
excesses are reported in `RESULT.json` as `d4_permuted2_excess` and are
negative on every regime as well.

## Unrelated-regime admissions (D5 disclosure)

Four of eight `unrelated` lives were admitted. The gated arms share one
admission step per life; `full24` keeps turn29's own.

| world | shared step | bank3 | pooled2 | permuted2 | cold | full24 (own step) |
|---:|---:|---:|---:|---:|---:|---|
|281|—|0|0|0|0|17.166 (2270)|
|282|1913|314.255|73.517|−0.989|0.000|31.913 (2322)|
|283|272|450.530|270.268|−0.971|0.000|280.271 (295)|
|284|558|394.929|192.047|−0.587|0.000|122.440 (2777)|
|287|918|396.417|262.475|−0.972|0.000|202.546 (1045)|

Worlds 280, 285 and 286 admitted nothing on `unrelated`. The label changes
commands while preserving repeat-role and emitter structure; these admissions
are disclosed, not read as semantic generalization.

## Safety extrema over all 40 lives

| arm | worst prefix gain | maximum drawdown | admitted lives |
|---|---:|---:|---:|
|bank3|−0.999791|14.165859|36/40|
|pooled2|−0.999505|14.246819|36/40|
|permuted2|−0.999238|13.924693|36/40|
|cold|0.000000|0.000000|36/40|
|full24|−0.915375|14.110823|37/40|

Every arm stays inside the preregistered −1 prefix and 16 drawdown bounds. The
four gated arms are admitted in exactly the same 36 lives, on exactly the same
step in each; `cold` is admitted with them and still pays exactly zero, which
is what makes the shared admission visible rather than assumed.
