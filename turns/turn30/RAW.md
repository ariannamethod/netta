# Turn30 raw help and harm

Post-result description of rows already selected mechanically in `RESULT.json`
— each life records the byte where `bank3_live − pooled2_live` is largest and
the byte where it is smallest, over the whole life. Zero-based byte positions;
every forecast was fixed before its truth. Nothing here is chosen by hand
except which of the forty pre-selected extremes to narrate: the two largest.

## Distinctness helps most: retreat, not discrimination

World282, `moved_mid`, t14914, truth235/rank4, record1, prefix `[1]`.
Stored histories at that address: A `[3819,1460,1472,1416,0,0,0]`,
B `[2650,307,1499,3776,0,0,0]`, pooled `[6469,1767,2971,5192,0,0,0]`.

Cold price `-6.097549`. Every stored source was far worse than cold on this
byte: A `-14.060769`, B `-14.421016`, pooled `-15.251745`. The three-way
router had already retreated to P0 — weights `(0.900545, 0.053733, 0.045722)`
— and quoted `-6.248105`, while the binary router still held `0.983495` on its
one pooled story and quoted `-11.874985`. Live prices `-6.247195` against
`-11.752080`: **+5.504885 bits**.

The win is not A being preferred over B. Both history weights were already
near zero, within 0.008 of each other. Splitting one memory into two gives the
arm two weights to abandon independently, and here abandonment is what paid.

## Distinctness harms most: committing to the wrong history

World285, `switched`, t9092, truth207/rank2, record5, prefix `[1,0,3]`.
A `[53,93,4,37,0,0,0]`, B `[27,71,1101,24,0,0,0]`, pooled
`[80,164,1105,61,0,0,0]`.

The two histories flatly disagree about role 2: A saw it 4 times in 134 valid
continuations, B saw it 1101 times in 1196. The three-way router had committed
to A — weights `(0.000194, 0.999335, 0.000471)` — and quoted `-6.100105`
against cold `-1.479443`. The pooled arm, holding `0.939413` on counts that
contain B's evidence, quoted `-1.476375`. Live `-6.092988` against
`-1.479336`: **−4.613652 bits**.

This is the mechanism behind the 8-of-8 early loss. Pooling cannot pick the
wrong history because it never picks; the three-way router can, and on prefixes
where one source life dominates the other's count it does. Nothing recovers the
bits already paid; the leak share brings the weight back only after the miss.

## The genuine divergence was there to exploit

Across the eight worlds all 96 selected addresses have A ≠ B — the counts
differ at every single one — and all 96 rotated null records differ from the
pooled records they mirror. The failure is not an absence of material.

## Byte accounting

`bank.bin` is 528 bytes against `pooled_small.bin`'s 336 in every world, so
distinctness costs exactly 192 portable bytes, one 16-byte count vector per
address. Dividing the mean life difference by those bytes:

| regime | early | life | tail |
|---|---:|---:|---:|
|recombined|−0.163876|−0.658831|−0.397161|
|partial|−0.163876|−0.202877|+0.058793|
|switched|−0.163876|+0.403173|+0.664843|
|moved_mid|−0.163876|−0.526835|−0.265165|
|unrelated|+0.120737|+0.493375|+0.236895|

Bits per extra byte. The early column is one number repeated because the four
prefix-sharing regimes have bit-identical early gains; the regimes that pay for
the extra bytes are the two where the target stops agreeing with the pooled
story.

## The shared prefix identity

`partial`, `switched` and `moved_mid` reuse `recombined`'s first 8192 raw
bytes, so with one price tape, one archive set and one shared admission their
traces are identical up to t8191 and their early(4096) gains are equal to
`recombined`'s to the last bit — the writer and the reader both assert it
(`shared_prefix_identity: true`). D1 therefore measures the regime's shared
prefix, before any divergence: it asks whether distinctness pays on ordinary
bytes, and the answer is that it costs 31.464 bits of the 492.814 the pooled
arm earns. The post-divergence `partial` tail, which D1 does not gate, is
+11.288 bits with 4 wins of 8 — reported, not decisive.

## The null is not free

`permuted2` reaches +236.485 bits in world280 `recombined` and is admitted in
the same 36 lives as everything else, because the shared admission does not ask
the null's opinion. Rotation preserves each address's total attestation and its
NEW count while scrambling which repeat role owns it, and that residue is worth
real bits. It never comes near the pooled arm on any regime (worst margin
−100.228 bits on `unrelated`, −2153.636 on `recombined`), which is the form D4
takes here: the null loses to the incumbent everywhere, rather than being
worthless everywhere.
