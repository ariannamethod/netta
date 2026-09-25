# Turn29 retained tables

All values are received bits saved against the same local P0. Positive is
better. These are frozen writer values independently reconstructed by the
reader.

## Intact world-by-world

| world | local early | pooled early | global early | permuted early | local full | pooled full | bank12 full |
|---:|---:|---:|---:|---:|---:|---:|---:|
|272|526.581|406.039|420.346|0.000|2791.021|1985.542|1209.602|
|273|538.565|317.217|330.523|0.000|2703.460|1744.125|1475.976|
|274|632.044|610.778|613.627|0.000|3461.771|3389.929|2463.807|
|275|974.587|833.585|861.833|0.000|5076.612|4547.664|2103.735|
|276|611.060|378.500|477.773|0.000|2708.770|2119.754|1946.962|
|277|674.622|608.320|607.348|0.000|3179.314|2881.317|1679.038|
|278|864.444|774.667|797.012|0.000|4243.636|3925.551|1269.655|
|279|663.579|551.440|551.131|0.000|3139.202|2902.838|1993.797|

Local full wins every early comparison against direct pooled and global, and
every full-life comparison against bank12.

## Partial-change world-by-world

| world | local−pooled tail | local−pooled whole |
|---:|---:|---:|
|272|+398.376|+764.041|
|273|+229.426|+780.774|
|274|+287.119|+331.365|
|275|+314.888|+607.128|
|276|+211.559|+575.123|
|277|+257.953|+377.365|
|278|+393.125|+600.667|
|279|+543.480|+749.516|

No losing partial tail or whole partial life is hidden.

## Unrelated-world admissions

| world | local full gain / activation | pooled | global | bank12 gain / activation | permuted |
|---:|---:|---:|---:|---:|---:|
|272|98.346 / 1525|0 / —|0 / —|183.732 / 1660|0 / —|
|273|0 / —|0 / —|0 / —|210.870 / 617|0 / —|
|274|0 / —|0 / —|0 / —|130.838 / 1783|0 / —|
|275|3.464 / 2779|0 / —|0 / —|102.040 / 1706|0 / —|
|276|79.523 / 641|0 / —|0 / —|313.851 / 559|0 / —|
|277|166.777 / 1852|0 / —|0 / —|0 / —|0 / —|
|278|42.695 / 1438|0 / —|0 / —|192.871 / 919|0 / —|
|279|43.904 / 4836|0 / —|0 / —|27.580 / 4318|0 / —|

The label `unrelated` changes commands but preserves repeat-role and emitter
structure. These admissions are disclosed, not interpreted as semantic
generalization.

## Safety extrema over all 40 lives

| arm | worst prefix gain | maximum drawdown | admitted lives |
|---|---:|---:|---:|
|local full|-0.999854|13.900827|38/40|
|global full|-0.847813|12.856055|32/40|
|pooled full|-0.808331|15.286337|32/40|
|bank12 local|-0.981667|14.609254|39/40|
|permuted full|-0.998140|10.791941|11/40|

Every arm remains inside the preregistered -1 prefix and 16 drawdown bounds.
