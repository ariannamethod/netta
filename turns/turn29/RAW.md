# Turn29 raw help and harm

Post-result description of rows already selected mechanically in RESULT.json.
Zero-based byte positions; all forecasts were fixed before truth.

## Local full versus direct pooled: strongest partial-tail help

World272, t10673, truth136/rank3, full record11, prefix
`[1,2,0,3,3,1]`, pooled counts `[904,14,21,24,0,0,0]`.
Pretruth local memory weight `.9817206741`, after truth `.9992659222`.
Local candidate/live prices are `-2.898567/-2.911464`; direct pooled live is
`-7.792236`. Saved difference: **+4.880772 bits**.

This row includes different outer-authority histories. It demonstrates the
real live consequence of local permission, not an isolated candidate-price
identity.

Raw window hex:
`9b83ff9b83568356839b8356888356568856f05638f0f05638c2e6e614e6c2e614`

## Local full versus direct pooled: strongest partial-tail harm

World278, t8555, truth51/rank3, full record0, prefix `[2]`, pooled counts
`[5255,6414,657,4085,0,0,0]`. The local memory weight had fallen to
`.0042836166`; after this surprising truth it rose to `.1727675428`.
Local live price `-8.241738`; direct pooled live `-2.985282`.
Difference: **-5.256457 bits**.

The memory it had learned to distrust became useful on this byte. Revision
responds after paying the miss; it does not know future law changes.

Raw window hex:
`bb919168bb68bb919191bbbb3391bb9133339191bbbbbbbbbb9d9d91919d9dbb06`

## Full24 versus Astra bank12: strongest help and harm

Help: world272 t8270, record14/prefix `[3,0]`, truth86/rank3. Full24 live
`-1.906505`; bank12 live `-6.739508`; difference **+4.833002 bits**.

Harm: world274 t14068, record14/prefix `[0,1,1,3,2]`, truth173/rank1.
Full24 live `-6.976369`; bank12 live `-1.769138`; difference
**-5.207231 bits**.

Both exist alongside the aggregate 8/8 full-life advantage. Coverage recovery
does not dominate every individual prediction.
