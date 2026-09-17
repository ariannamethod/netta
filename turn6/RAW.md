# Two bytes from the frozen new batch

These are selected after the full run to illustrate the mechanism, not to
choose worlds or satisfy the material gate. All eight worlds are in
[REPORT.md](REPORT.md) and [RESULT.json](RESULT.json). The shared pretruth
candidate is identical in old and fast. A positive `fast - old` log2 price
means the fast controller predicted the observed byte better.

## Help: withdraw from a wrong source

World 56, `mosaic_then_unrelated`, raw byte index `t=9183` (after the fixed
switch), observed byte `225` / `0xE1`. The six already learned context units
were `a9,45,e1,3f,3f,3a`; canonical HEAD256 row `012334`, observed group 2.
The frontend had trained on 8192 preceding bytes.

| Pretruth quantity | Value |
| --- | ---: |
| Local P0 log2 price | -8.3912570995 |
| Shared revised candidate log2 price | -10.3180597897 |
| Old outer source weight | 0.9816381675 |
| Fast outer source weight | 0.3406842227 |
| Old live log2 price | -10.2456768104 |
| Fast live log2 price | -8.8083743163 |

Fast gains **1.4373024941 bits on this byte** because it gives much less
authority to the candidate that prices this truth worse than P0.

## Harm: withdraw from a useful source

World 59, same changed regime, `t=9056`, observed byte `192` / `0xC0`.
The context units were `ab,c0,abab,c0c0,ab,12`; row `010102`, observed
group 1. The frontend had trained on 8192 preceding bytes.

| Pretruth quantity | Value |
| --- | ---: |
| Local P0 log2 price | -6.1493865929 |
| Shared revised candidate log2 price | -4.5856026824 |
| Old outer source weight | 0.8077265858 |
| Fast outer source weight | 0.0254703860 |
| Old live log2 price | -4.7819366807 |
| Fast live log2 price | -6.0792346305 |

Fast loses **1.2972979498 bits on this byte**. It is less willing to use
past advice even when that advice is locally right. This is the local cost
behind its full-life tradeoff, not an omitted exception.

The matching records are under
`results/outer/world56/mosaic_then_unrelated.tsv.gz` and
`results/outer/world59/mosaic_then_unrelated.tsv.gz`, with the C pretruth
records under `results/c/` and learned-unit expansions under `traces/`.
