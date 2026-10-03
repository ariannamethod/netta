# Turn33 raw witnesses

All rows are selected mechanically from the saved partial traces after the
8192-byte seam. Help/harm is the received earned-minus-pooled truth price.
First strict extrema win ties. Quotes and states precede truth.

## Per-world extrema and first candidate divergence

| World | Help t / bits | Harm t / bits | First divergence t | First candidate delta |
|---:|---:|---:|---:|---:|
| 296 | 11757 / +2.421533 | 15363 / -2.547993 | 13 | +4.44e-16 |
| 297 | 14056 / +2.483366 | 8557 / -3.690046 | 24 | +0.002661 |
| 298 | 9340 / +2.505710 | 14813 / -3.292941 | 9 | +0.079239 |
| 299 | 9867 / +3.185873 | 10010 / -1.922361 | 10 | +0.004854 |
| 300 | 13865 / +3.187382 | 10256 / -3.508533 | 14 | -0.000824 |
| 301 | 11033 / +4.256103 | 11618 / -2.914569 | 6 | +4.44e-16 |
| 302 | 9506 / +2.657449 | 13921 / -2.204768 | 12 | +0.010248 |
| 303 | 8854 / +2.245851 | 10849 / -1.663316 | 28 | +0.006152 |

All first divergences occur only after at least one prior visit. In worlds296
and301 the wealth printed as hA=hB=5.4188424580825512e-16 after a neutral
double-precision update; the resulting candidate delta is one floating-point
ulp. First visits themselves remain exactly pooled.

## Largest help: world301 partial, t11033

Truth decimal210, repeat rank2. Matched record15 at prefix 2,2,1,2,1 after
128 prior visits. Pretruth history:

    33222101203133022101203133022121

Raw bytes t[max(0,t-16):t+17]:

    a4a44646a4364646a436d236d2d23636d2043636d204a6a6a604a6d2d204a6d204

Source continuation counts, ordered NEW and repeat ranks1..6:

| Book | Counts |
|---|---|
| pooled | [29, 960, 38, 15, 0, 0, 0] |
| case A | [29, 959, 22, 15, 0, 0, 0] |
| case B | [0, 1, 16, 0, 0, 0, 0] |

| Quantity | log2 value |
|---|---:|
| P0 truth | -5.080489300800 |
| pooled source | -5.845441354070 |
| case A source | -6.595994679788 |
| case B source | -1.290733991279 |
| corrected earned source | -1.388904221455 |
| earned candidate | -1.506182745539 |
| pooled candidate | -5.762494814116 |
| earned received | -1.506310787129 |
| pooled received | -5.762413705196 |

Permission u=0.915375459670; hA=-4.251360704916 and hB=3.863413263295.
Case B had earned voice and correctly assigned much more mass to the truth.
Paid help: **+4.256102918066 bits**.

## Largest harm: world297 partial, t8557

Truth decimal137, repeat rank3. Matched record18 at prefix 3,1,3,1 after
142 prior visits. Pretruth history:

    20322331100100322332321201303131

Raw bytes t[max(0,t-16):t+17]:

    a29f039fa29f9fa261619f8961619f9f89618989ef6189618937377c0a9c9c0a7a

Source continuation counts:

| Book | Counts |
|---|---|
| pooled | [52, 23, 1025, 255, 0, 0, 0] |
| case A | [29, 22, 995, 255, 0, 0, 0] |
| case B | [23, 1, 30, 0, 0, 0, 0] |

| Quantity | log2 value |
|---|---:|
| P0 truth | -5.347236967494 |
| pooled source | -3.513779460442 |
| case A source | -3.479094931241 |
| case B source | -7.205518969281 |
| corrected earned source | -7.205518932389 |
| earned candidate | -7.204397519913 |
| pooled candidate | -3.514086858898 |
| earned received | -7.204189897083 |
| pooled received | -3.514144171020 |

Permission u=0.999703854440; hA=-0.991214948758 and hB=28.796419119729.
Accumulated B wealth was extremely confident and wrong on this byte. Paid
harm: **-3.690045726063 bits**.

These examples expose the mechanism's real trade: earned local distinctions
can sharply improve or sharply damage one event. The preregistered paired
life totals, not either anecdote, decide the result. Every world's full rows,
raw windows, states and first divergence remain in VERIFY.json and the
manifest-pinned results/world*/partial.turn33.tsv.gz traces.
