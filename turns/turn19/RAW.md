# Turn19: actual charged bytes

These are the largest and smallest per-byte ceiling−witness differences on each changed tail, selected only after the fixed run for inspection. A positive difference means ceiling assigned more probability to the observed byte. Positions are zero-based. This is synthetic recurrence behavior, not generated natural-language speech.

Every record comes from `RESULT.json` and identifies the raw authority TSV and target byte. `cold`, `candidate`, and `*_live` are log2 probabilities; odds are log2(source/cold). A field called help/harm retains the extremum even if it is a tie.

## World 200 / switched

Trace: `results/authority/world200/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 46.168677557751465,
  "adaptive_live": -4.160833068946439,
  "adaptive_odds_after": 4.092611744113868,
  "adaptive_odds_before": 7.566351144727115,
  "candidate": -4.235396056745841,
  "ceiling_clipped": 0,
  "ceiling_live": -1.7287589040118834,
  "ceiling_odds_after": -3.261520455281829,
  "ceiling_odds_before": 0.2118456139944056,
  "ceiling_w_after": -1.7902932582964175,
  "ceiling_w_before": 1.7373403886307774,
  "cold": -0.7620542969633574,
  "delta": 2.432074164934556,
  "fast_live": -2.3897734416504193,
  "fast_odds_after": -1.9410501513120693,
  "fast_odds_before": 1.5340684947468375,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 2,
  "regime": "switched",
  "slow_live": -4.160833068946439,
  "slow_odds_after": 4.092611744113868,
  "slow_odds_before": 7.566351144727115,
  "t": 8215,
  "truth": 53,
  "witness_live": -4.160833068946439,
  "witness_odds_after": 4.092611744113868,
  "witness_odds_before": 7.566351144727115,
  "witness_w_after": -1.7902932582964175,
  "witness_w_before": 1.7373403886307774,
  "world": 200
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 34.59833457799858,
  "adaptive_live": -4.693310713442137,
  "adaptive_odds_after": 0.8881945619835181,
  "adaptive_odds_before": -2.153380622069178,
  "candidate": -2.87058108993397,
  "ceiling_clipped": 0,
  "ceiling_live": -5.898112060888784,
  "ceiling_odds_after": -6.481825638922329,
  "ceiling_odds_before": -9.522038233331832,
  "ceiling_w_after": -3.6147767013329473,
  "ceiling_w_before": -6.871137697981288,
  "cold": -5.912219033520396,
  "delta": -1.1633683908474675,
  "fast_live": -5.877328863909834,
  "fast_odds_after": -5.162086986114289,
  "fast_odds_before": -8.202275971638189,
  "hazard": "2^-10",
  "matchedL": 1,
  "rank": 3,
  "regime": "switched",
  "slow_live": -4.693310713442137,
  "slow_odds_after": 0.8881945619835181,
  "slow_odds_before": -2.153380622069178,
  "t": 8227,
  "truth": 26,
  "witness_live": -4.734743670041317,
  "witness_odds_after": 0.7949475893143862,
  "witness_odds_before": -2.2428318870229904,
  "witness_w_after": -3.6147767013329473,
  "witness_w_before": -6.871137697981288,
  "world": 200
}
```

## World 200 / moved_mid

Trace: `results/authority/world200/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 49.79375130208418,
  "adaptive_live": -15.56693705187935,
  "adaptive_odds_after": 2.593320096966052,
  "adaptive_odds_before": 14.458449476388731,
  "candidate": -15.788052394828398,
  "ceiling_clipped": 0,
  "ceiling_live": -8.955147912654237,
  "ceiling_odds_after": -6.864996710077225,
  "ceiling_odds_before": 5.0,
  "ceiling_w_after": -3.306294259823074,
  "ceiling_w_before": 8.834766707045832,
  "cold": -3.9230778875546743,
  "delta": 6.611789139225113,
  "fast_live": -12.24540134249985,
  "fast_odds_after": -3.419267066998442,
  "fast_odds_before": 8.44724884130254,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -15.56693705187935,
  "slow_odds_after": 2.593320096966052,
  "slow_odds_before": 14.458449476388731,
  "t": 13728,
  "truth": 0,
  "witness_live": -15.56693705187935,
  "witness_odds_after": 2.593320096966052,
  "witness_odds_before": 14.458449476388731,
  "witness_w_after": -3.306294259823074,
  "witness_w_before": 8.834766707045832,
  "world": 200
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 35.37521893410887,
  "adaptive_live": -1.9841508447591432,
  "adaptive_odds_after": 9.588754369440949,
  "adaptive_odds_before": 5.276895249605251,
  "candidate": -1.9492620308604183,
  "ceiling_clipped": 0,
  "ceiling_live": -4.538421655996616,
  "ceiling_odds_after": 1.4885142998458878,
  "ceiling_odds_before": -2.840334756841197,
  "ceiling_w_after": 4.325535634900177,
  "ceiling_w_before": -0.0035067960088594682,
  "cold": -6.2781948743941784,
  "delta": -2.5542708110757997,
  "fast_live": -3.2547422077878245,
  "fast_odds_after": 3.563977582769116,
  "fast_odds_before": -0.7467697978674102,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 2,
  "regime": "moved_mid",
  "slow_live": -1.9841508447591432,
  "slow_odds_after": 9.588754369440949,
  "slow_odds_before": 5.276895249605251,
  "t": 10473,
  "truth": 104,
  "witness_live": -1.984150844920816,
  "witness_odds_after": 9.588754362745307,
  "witness_odds_before": 5.276895242830002,
  "witness_w_after": 4.325535634900177,
  "witness_w_before": -0.0035067960088594682,
  "world": 200
}
```

## World 201 / switched

Trace: `results/authority/world201/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 50.75698727749909,
  "adaptive_live": -6.1669564488693815,
  "adaptive_odds_after": 0.9604542299085418,
  "adaptive_odds_before": 3.9913960618143114,
  "candidate": -6.677229288546213,
  "ceiling_clipped": 0,
  "ceiling_live": -3.7874317241678224,
  "ceiling_odds_after": -6.1048731433349746,
  "ceiling_odds_before": -3.073973829972958,
  "ceiling_w_after": -2.173253948927334,
  "ceiling_w_before": 0.8852882896864422,
  "cold": -3.646352308985138,
  "delta": 2.379524724701559,
  "fast_live": -3.91687462158975,
  "fast_odds_after": -5.079524161469569,
  "fast_odds_before": -2.0471959039725,
  "hazard": "2^-16",
  "matchedL": 4,
  "rank": 1,
  "regime": "switched",
  "slow_live": -6.1669564488693815,
  "slow_odds_after": 0.9604542299085418,
  "slow_odds_before": 3.9913960618143114,
  "t": 8206,
  "truth": 218,
  "witness_live": -6.1669564488693815,
  "witness_odds_after": 0.9604542299085418,
  "witness_odds_before": 3.9913960618143114,
  "witness_w_after": -2.173253948927334,
  "witness_w_before": 0.8852882896864422,
  "world": 201
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 47.15725477730489,
  "adaptive_live": -2.854663996514369,
  "adaptive_odds_after": 3.6390724907308494,
  "adaptive_odds_before": 0.9603245309958364,
  "candidate": -2.3677189278248645,
  "ceiling_clipped": 0,
  "ceiling_live": -4.939415117975404,
  "ceiling_odds_after": -3.430229341706229,
  "ceiling_odds_before": -6.107733203914827,
  "ceiling_w_after": 0.5737045022512259,
  "ceiling_w_before": -2.173253948927334,
  "cold": -5.046763193099445,
  "delta": -2.0825837508948544,
  "fast_live": -4.838953430541749,
  "fast_odds_after": -2.4050581801081483,
  "fast_odds_before": -5.082426591582606,
  "hazard": "2^-10",
  "matchedL": 1,
  "rank": 2,
  "regime": "switched",
  "slow_live": -2.854663996514369,
  "slow_odds_after": 3.6390724907308494,
  "slow_odds_before": 0.9603245309958364,
  "t": 8209,
  "truth": 187,
  "witness_live": -2.8568313670805496,
  "witness_odds_after": 3.612447155107724,
  "witness_odds_before": 0.9521650001737676,
  "witness_w_after": 0.5737045022512259,
  "witness_w_before": -2.173253948927334,
  "world": 201
}
```

## World 201 / moved_mid

Trace: `results/authority/world201/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 23.897737902709487,
  "adaptive_live": -13.223362015165598,
  "adaptive_odds_after": -1.252305259612326,
  "adaptive_odds_before": 8.346794945681234,
  "candidate": -14.976884936714974,
  "ceiling_clipped": 0,
  "ceiling_live": -7.4044321418494565,
  "ceiling_odds_after": -7.971148901826378,
  "ceiling_odds_before": 1.6279421501498141,
  "ceiling_w_after": -6.543510895657461,
  "ceiling_w_before": 3.154124443501246,
  "cold": -5.377815986415682,
  "delta": 5.683155147738093,
  "fast_live": -7.871781301895178,
  "fast_odds_after": -7.377947751352213,
  "fast_odds_before": 2.2225392476544537,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -13.223362015165598,
  "slow_odds_after": -1.252305259612326,
  "slow_odds_before": 8.346794945681234,
  "t": 16282,
  "truth": 31,
  "witness_live": -13.08758728958755,
  "witness_odds_after": -1.4423256084753153,
  "witness_odds_before": 8.156773456408455,
  "witness_w_after": -6.543510895657461,
  "witness_w_before": 3.154124443501246,
  "world": 201
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 17.3966451591276,
  "adaptive_live": -1.8100368795680688,
  "adaptive_odds_after": 5.940858013352332,
  "adaptive_odds_before": 1.3798928429393293,
  "candidate": -1.364209420001274,
  "ceiling_clipped": 0,
  "ceiling_live": -5.746244631371694,
  "ceiling_odds_after": -2.8382841548613644,
  "ceiling_odds_before": -7.400599195772201,
  "ceiling_w_after": 4.09771698660666,
  "ceiling_w_before": -0.47961098985753997,
  "cold": -5.926549553032426,
  "delta": -3.9064033205897637,
  "fast_live": -5.063032761852024,
  "fast_odds_after": -0.17428812272619215,
  "fast_odds_before": -4.733968365639287,
  "hazard": "2^-16",
  "matchedL": 9,
  "rank": 3,
  "regime": "moved_mid",
  "slow_live": -1.8100368795680688,
  "slow_odds_after": 5.940858013352332,
  "slow_odds_before": 1.3798928429393293,
  "t": 14709,
  "truth": 227,
  "witness_live": -1.8398413107819307,
  "witness_odds_after": 5.830060538272656,
  "witness_odds_before": 1.268995309061563,
  "witness_w_after": 4.09771698660666,
  "witness_w_before": -0.47961098985753997,
  "world": 201
}
```

## World 202 / switched

Trace: `results/authority/world202/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 14.944308514518987,
  "adaptive_live": -5.191482790800187,
  "adaptive_odds_after": -1.7169999170081207,
  "adaptive_odds_before": 2.8321711836647987,
  "candidate": -7.102038076709091,
  "ceiling_clipped": 0,
  "ceiling_live": -2.6078799250306854,
  "ceiling_odds_after": -9.169708223036688,
  "ceiling_odds_before": -4.6205437804215475,
  "ceiling_w_after": -5.186476732692847,
  "ceiling_w_before": -0.6578935145802997,
  "cold": -2.5528956862659093,
  "delta": 2.5683937267117294,
  "fast_live": -2.6885505962574645,
  "fast_odds_after": -7.82361355763187,
  "fast_odds_before": -3.2730553716741277,
  "hazard": "2^-16",
  "matchedL": 4,
  "rank": 3,
  "regime": "switched",
  "slow_live": -5.191482790800187,
  "slow_odds_after": -1.7169999170081207,
  "slow_odds_before": 2.8321711836647987,
  "t": 8340,
  "truth": 142,
  "witness_live": -5.176273651742415,
  "witness_odds_after": -1.740609053888234,
  "witness_odds_before": 2.808561938094171,
  "witness_w_after": -5.186476732692847,
  "witness_w_before": -0.6578935145802997,
  "world": 202
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 24.03694143841167,
  "adaptive_live": -1.7282018649771014,
  "adaptive_odds_after": 8.006726643431493,
  "adaptive_odds_before": 5.38047706481328,
  "candidate": -1.6995544915658247,
  "ceiling_clipped": 0,
  "ceiling_live": -3.501375328867111,
  "ceiling_odds_after": 0.12514196507267758,
  "ceiling_odds_before": -2.5067566951250857,
  "ceiling_w_after": 7.46814711537802,
  "ceiling_w_before": 4.992208962175912,
  "cold": -4.33149917483593,
  "delta": -1.7731734585730745,
  "fast_live": -2.7332462812016467,
  "fast_odds_after": 1.9799834268265093,
  "fast_odds_before": -0.6449776213289639,
  "hazard": "2^-16",
  "matchedL": 2,
  "rank": 3,
  "regime": "switched",
  "slow_live": -1.7282018649771014,
  "slow_odds_after": 8.006726643431493,
  "slow_odds_before": 5.38047706481328,
  "t": 8255,
  "truth": 157,
  "witness_live": -1.7282018702940363,
  "witness_odds_after": 8.006726373019033,
  "witness_odds_before": 5.3804767933353865,
  "witness_w_after": 7.46814711537802,
  "witness_w_before": 4.992208962175912,
  "world": 202
}
```

## World 202 / moved_mid

Trace: `results/authority/world202/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 28.675184495250676,
  "adaptive_live": -11.33245169454114,
  "adaptive_odds_after": 2.311019797292911,
  "adaptive_odds_before": 11.797661486849178,
  "candidate": -11.596882678790793,
  "ceiling_clipped": 0,
  "ceiling_live": -6.486905663517971,
  "ceiling_odds_after": -5.138790804802071,
  "ceiling_odds_before": 4.347742263996672,
  "ceiling_w_after": -0.6433312291221069,
  "ceiling_w_before": 9.128443046075889,
  "cold": -2.110372248782668,
  "delta": 4.845546031018653,
  "fast_live": -7.813134122089365,
  "fast_odds_after": -3.7044880110774945,
  "fast_odds_before": 5.7835401707299585,
  "hazard": "2^-16",
  "matchedL": 4,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -11.332451694546982,
  "slow_odds_after": 2.311019797327807,
  "slow_odds_before": 11.797661486884076,
  "t": 13795,
  "truth": 80,
  "witness_live": -11.332451694536624,
  "witness_odds_after": 2.3110197972659376,
  "witness_odds_before": 11.797661486822202,
  "witness_w_after": -0.6433312291221069,
  "witness_w_before": 9.128443046075889,
  "world": 202
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 18.378355535777303,
  "adaptive_live": -2.3624270447287525,
  "adaptive_odds_after": 5.912543728699432,
  "adaptive_odds_before": 2.356894974559549,
  "candidate": -2.128894146639613,
  "ceiling_clipped": 0,
  "ceiling_live": -5.469269568320026,
  "ceiling_odds_after": -2.475906877968716,
  "ceiling_odds_before": -6.032878317504619,
  "ceiling_w_after": 2.861862478706415,
  "ceiling_w_before": -0.7175586393808061,
  "cold": -5.685891557246184,
  "delta": -3.1068415158478255,
  "fast_live": -4.862167241122324,
  "fast_odds_after": -0.14469517874062648,
  "fast_odds_before": -3.699006776025779,
  "hazard": "2^-16",
  "matchedL": 2,
  "rank": 1,
  "regime": "moved_mid",
  "slow_live": -2.3624270447287525,
  "slow_odds_after": 5.912543728699432,
  "slow_odds_before": 2.356894974559549,
  "t": 8242,
  "truth": 193,
  "witness_live": -2.362428052472201,
  "witness_odds_after": 5.9125368801828,
  "witness_odds_before": 2.356888119742425,
  "witness_w_after": 2.861862478706415,
  "witness_w_before": -0.7175586393808061,
  "world": 202
}
```

## World 203 / switched

Trace: `results/authority/world203/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 6.509010677900153,
  "adaptive_live": -4.152492194129832,
  "adaptive_odds_after": -0.8883046258300986,
  "adaptive_odds_before": 3.299583804651745,
  "candidate": -5.524388860918921,
  "ceiling_clipped": 0,
  "ceiling_live": -1.4212830519634227,
  "ceiling_odds_after": -8.148235143495562,
  "ceiling_odds_before": -3.9603585285622303,
  "ceiling_w_after": 0.7955202839240693,
  "ceiling_w_before": 5.144128833356764,
  "cold": -1.336534337528625,
  "delta": 2.7305815418312953,
  "fast_live": -1.4286696444476896,
  "fast_odds_after": -8.024876759634386,
  "fast_odds_before": -3.835607251330509,
  "hazard": "2^-16",
  "matchedL": 4,
  "rank": 3,
  "regime": "switched",
  "slow_live": -4.491467436052336,
  "slow_odds_after": -0.23695901751133253,
  "slow_odds_before": 3.9509361996284955,
  "t": 10016,
  "truth": 172,
  "witness_live": -4.151864593794718,
  "witness_odds_after": -0.8894311663810478,
  "witness_odds_before": 3.298457254817521,
  "witness_w_after": 0.7955202839240693,
  "witness_w_before": 5.144128833356764,
  "world": 203
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 2.81786929122377,
  "adaptive_live": -2.1907807465932594,
  "adaptive_odds_after": 3.351378019781303,
  "adaptive_odds_before": 0.9707060692095947,
  "candidate": -1.7308163111269481,
  "ceiling_clipped": 0,
  "ceiling_live": -4.036595980962468,
  "ceiling_odds_after": -3.899358110396543,
  "ceiling_odds_before": -6.280253284276959,
  "ceiling_w_after": 3.1275782137062524,
  "ceiling_w_before": 0.770745342541978,
  "cold": -4.1117349742456595,
  "delta": -1.8455368181864378,
  "fast_live": -4.035822484611152,
  "fast_odds_after": -3.8854778464697803,
  "fast_odds_before": -6.264891513076913,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 3,
  "regime": "switched",
  "slow_live": -2.050339752185866,
  "slow_odds_after": 3.9974689300012924,
  "slow_odds_before": 1.6169239321219924,
  "t": 10092,
  "truth": 104,
  "witness_live": -2.19105916277603,
  "witness_odds_after": 3.3502586801220646,
  "witness_odds_before": 0.9695865552681515,
  "witness_w_after": 3.1275782137062524,
  "witness_w_before": 0.770745342541978,
  "world": 203
}
```

## World 203 / moved_mid

Trace: `results/authority/world203/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 31.664003657095265,
  "adaptive_live": -15.338388638436406,
  "adaptive_odds_after": 1.1317809265361176,
  "adaptive_odds_before": 11.356785078049297,
  "candidate": -15.880175762024093,
  "ceiling_clipped": 0,
  "ceiling_live": -9.374022946570621,
  "ceiling_odds_after": -6.604174502851596,
  "ceiling_odds_before": 3.6207816348080613,
  "ceiling_w_after": -2.4294230683117095,
  "ceiling_w_before": 8.046978920382385,
  "cold": -5.655241864591948,
  "delta": 5.964361820445086,
  "fast_live": -10.985816160591064,
  "fast_odds_after": -4.882778180839707,
  "fast_odds_before": 5.343613088554102,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -15.338388638436406,
  "slow_odds_after": 1.1317809265361176,
  "slow_odds_before": 11.356785078049297,
  "t": 12754,
  "truth": 145,
  "witness_live": -15.338384767015707,
  "witness_odds_after": 1.1317685566964892,
  "witness_odds_before": 11.356772707796047,
  "witness_w_after": -2.4294230683117095,
  "witness_w_before": 8.046978920382385,
  "world": 203
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 28.654644718868447,
  "adaptive_live": -1.0204579345172222,
  "adaptive_odds_after": 7.6335756292480585,
  "adaptive_odds_before": 4.13148671442938,
  "candidate": -0.9476314348769734,
  "ceiling_clipped": 0,
  "ceiling_live": -3.683100297984995,
  "ceiling_odds_after": -0.26659798015895864,
  "ceiling_odds_before": -3.7730467919464794,
  "ceiling_w_after": 3.0099794086803375,
  "ceiling_w_before": -0.5125261594927096,
  "cold": -4.454120560565873,
  "delta": -2.660854539087978,
  "fast_live": -2.7804904426423804,
  "fast_odds_after": 1.6006952326352002,
  "fast_odds_before": -1.9001008031973565,
  "hazard": "2^-16",
  "matchedL": 5,
  "rank": 3,
  "regime": "moved_mid",
  "slow_live": -1.0204579345172222,
  "slow_odds_after": 7.6335756292480585,
  "slow_odds_before": 4.13148671442938,
  "t": 14674,
  "truth": 125,
  "witness_live": -1.022245758897017,
  "witness_odds_after": 7.597610489262398,
  "witness_odds_before": 4.095413619294469,
  "witness_w_after": 3.0099794086803375,
  "witness_w_before": -0.5125261594927096,
  "world": 203
}
```

## World 204 / switched

Trace: `results/authority/world204/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 30.736139240900695,
  "adaptive_live": -5.826322551051471,
  "adaptive_odds_after": 2.429832322861419,
  "adaptive_odds_before": 6.213461303818492,
  "candidate": -6.052590130032871,
  "ceiling_clipped": 0,
  "ceiling_live": -2.843985865532295,
  "ceiling_odds_after": -4.64879800885308,
  "ceiling_odds_before": -0.8652867740524566,
  "ceiling_w_after": -3.8161351177484972,
  "ceiling_w_before": -0.03369989620529512,
  "cold": -2.2691017867332537,
  "delta": 2.9040583602728485,
  "fast_live": -3.234880330257812,
  "fast_odds_after": -3.633612158673686,
  "fast_odds_before": 0.1513993840299985,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 2,
  "regime": "switched",
  "slow_live": -5.826322551051471,
  "slow_odds_after": 2.429832322861419,
  "slow_odds_before": 6.213461303818492,
  "t": 8261,
  "truth": 9,
  "witness_live": -5.748044225805144,
  "witness_odds_after": 1.9534099837375916,
  "witness_odds_before": 5.737005601718465,
  "witness_w_after": -3.8161351177484972,
  "witness_w_before": -0.03369989620529512,
  "world": 204
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 2.610431704413189,
  "adaptive_live": -2.3929128698413433,
  "adaptive_odds_after": 5.502242322876547,
  "adaptive_odds_before": -0.4366559399980939,
  "candidate": -1.1895892021995143,
  "ceiling_clipped": 0,
  "ceiling_live": -6.0042238531992,
  "ceiling_odds_after": 0.29270137621029857,
  "ceiling_odds_before": -5.64716806058689,
  "ceiling_w_after": 11.980799572317292,
  "ceiling_w_before": 6.235748289581415,
  "cold": -7.129507618984811,
  "delta": -3.406053466629064,
  "fast_live": -6.259006671730544,
  "fast_odds_after": -0.23077160732061547,
  "fast_odds_before": -6.1680781598506345,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 3,
  "regime": "switched",
  "slow_live": -1.8597103561560226,
  "slow_odds_after": 6.658049611892089,
  "slow_odds_before": 0.7203780999421313,
  "t": 9809,
  "truth": 64,
  "witness_live": -2.598170386570136,
  "witness_odds_after": 5.148715763381244,
  "witness_odds_before": -0.7903994854059879,
  "witness_w_after": 11.980799572317292,
  "witness_w_before": 6.235748289581415,
  "world": 204
}
```

## World 204 / moved_mid

Trace: `results/authority/world204/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 21.76177535515496,
  "adaptive_live": -14.720338969551348,
  "adaptive_odds_after": -0.6024348642466667,
  "adaptive_odds_before": 11.644023260963891,
  "candidate": -16.052303121154655,
  "ceiling_clipped": 0,
  "ceiling_live": -8.40680389473931,
  "ceiling_odds_after": -7.699008654524606,
  "ceiling_odds_before": 4.547435077147225,
  "ceiling_w_after": -5.56606523976791,
  "ceiling_w_before": 6.895851738853703,
  "cold": -3.8058815093722203,
  "delta": 6.3135321635498975,
  "fast_live": -9.446608800390512,
  "fast_odds_after": -6.621438246485519,
  "fast_odds_before": 5.62640725906419,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -14.720338969551348,
  "slow_odds_after": -0.6024348642466667,
  "slow_odds_before": 11.644023260963891,
  "t": 9403,
  "truth": 245,
  "witness_live": -14.720336058289208,
  "witness_odds_after": -0.6024396954865188,
  "witness_odds_before": 11.644018429675484,
  "witness_w_after": -5.56606523976791,
  "witness_w_before": 6.895851738853703,
  "world": 204
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 10.354952104793702,
  "adaptive_live": -2.8640890555183343,
  "adaptive_odds_after": 8.702389349175474,
  "adaptive_odds_before": 4.803552129730517,
  "candidate": -2.816768853810105,
  "ceiling_clipped": 0,
  "ceiling_live": -5.732668751805258,
  "ceiling_odds_after": 0.18929756639821094,
  "ceiling_odds_before": -3.7187140769357923,
  "ceiling_w_after": 5.78519098281785,
  "ceiling_w_before": 1.9376848770462833,
  "cold": -6.724827611989368,
  "delta": -2.8503692532570613,
  "fast_live": -4.484553698714552,
  "fast_odds_after": 2.433410303405872,
  "fast_odds_before": -1.4656009220673485,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 1,
  "regime": "moved_mid",
  "slow_live": -2.8640890555183343,
  "slow_odds_after": 8.702389349175474,
  "slow_odds_before": 4.803552129730517,
  "t": 8871,
  "truth": 12,
  "witness_live": -2.882299498548197,
  "witness_odds_after": 8.224754740778964,
  "witness_odds_before": 4.32331873082333,
  "witness_w_after": 5.78519098281785,
  "witness_w_before": 1.9376848770462833,
  "world": 204
}
```

## World 205 / switched

Trace: `results/authority/world205/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 20.694025957423225,
  "adaptive_live": -4.668186068047536,
  "adaptive_odds_after": -0.207996343621494,
  "adaptive_odds_before": 4.427727424615866,
  "candidate": -5.710384026641928,
  "ceiling_clipped": 0,
  "ceiling_live": -1.2242886302865108,
  "ceiling_odds_after": -7.765508977951939,
  "ceiling_odds_before": -3.1284102309054984,
  "ceiling_w_after": -5.968840259958991,
  "ceiling_w_before": -1.3761626469858945,
  "cold": -1.0747013309505222,
  "delta": 2.836697840282193,
  "fast_live": -1.4210285711235955,
  "fast_odds_after": -6.443305565296342,
  "fast_odds_before": -1.8061970934523772,
  "hazard": "2^-10",
  "matchedL": 4,
  "rank": 2,
  "regime": "switched",
  "slow_live": -4.668186068047536,
  "slow_odds_after": -0.207996343621494,
  "slow_odds_before": 4.427727424615866,
  "t": 8435,
  "truth": 161,
  "witness_live": -4.060986470568704,
  "witness_odds_after": -1.2922051244905113,
  "witness_odds_before": 3.3454631024728534,
  "witness_w_after": -5.968840259958991,
  "witness_w_before": -1.3761626469858945,
  "world": 205
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 14.346255794286597,
  "adaptive_live": -2.150930640043203,
  "adaptive_odds_after": 7.215163684047352,
  "adaptive_odds_before": 3.0054752567737197,
  "candidate": -1.9912678671669473,
  "ceiling_clipped": 0,
  "ceiling_live": -5.457798864994033,
  "ceiling_odds_after": -0.424599974404524,
  "ceiling_odds_before": -4.637546724274864,
  "ceiling_w_after": 6.071574967064406,
  "ceiling_w_before": 1.918544311371137,
  "cold": -6.204253032590565,
  "delta": -3.1169260698850683,
  "fast_live": -4.845076203996142,
  "fast_odds_after": 0.8573466873031267,
  "fast_odds_before": -3.3516716718030635,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 1,
  "regime": "switched",
  "slow_live": -2.150930640043203,
  "slow_odds_after": 7.215163684047352,
  "slow_odds_before": 3.0054752567737197,
  "t": 8525,
  "truth": 3,
  "witness_live": -2.340872795108965,
  "witness_odds_after": 5.975521187369715,
  "witness_odds_before": 1.7639439009006346,
  "witness_w_after": 6.071574967064406,
  "witness_w_before": 1.918544311371137,
  "world": 205
}
```

## World 205 / moved_mid

Trace: `results/authority/world205/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 42.871615401839264,
  "adaptive_live": -14.056752985174834,
  "adaptive_odds_after": -0.5522466016657688,
  "adaptive_odds_before": 11.49806533546814,
  "candidate": -15.358619989953255,
  "ceiling_clipped": 0,
  "ceiling_live": -7.179244414009743,
  "ceiling_odds_after": -8.276553571775331,
  "ceiling_odds_before": 3.773743423585172,
  "ceiling_w_after": -7.517503338382623,
  "ceiling_w_before": 4.678990009846154,
  "cold": -3.3083450795321707,
  "delta": 6.877508571165091,
  "fast_live": -8.794902534611793,
  "fast_odds_after": -6.5823516051086255,
  "fast_odds_before": 5.469347592450401,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -14.056752985174834,
  "slow_odds_after": -0.5522466016657688,
  "slow_odds_before": 11.49806533546814,
  "t": 14773,
  "truth": 18,
  "witness_live": -14.056752985174834,
  "witness_odds_after": -0.5522466016657688,
  "witness_odds_before": 11.49806533546814,
  "witness_w_after": -7.517503338382623,
  "witness_w_before": 4.678990009846154,
  "world": 205
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 20.138678170771456,
  "adaptive_live": -1.9324181817936528,
  "adaptive_odds_after": 5.810546834511295,
  "adaptive_odds_before": 2.5447653963178487,
  "candidate": -1.729671430469124,
  "ceiling_clipped": 0,
  "ceiling_live": -4.7982699437186165,
  "ceiling_odds_after": -2.58016597899338,
  "ceiling_odds_before": -5.845560066003852,
  "ceiling_w_after": -0.7055691124106669,
  "ceiling_w_before": -4.100757279876157,
  "cold": -4.996710932938484,
  "delta": -2.86241335398533,
  "fast_live": -4.219988309489336,
  "fast_odds_after": -0.2074509964746413,
  "fast_odds_before": -3.4718590338426134,
  "hazard": "2^-10",
  "matchedL": 2,
  "rank": 2,
  "regime": "moved_mid",
  "slow_live": -1.9324181817936528,
  "slow_odds_after": 5.810546834511295,
  "slow_odds_before": 2.5447653963178487,
  "t": 10490,
  "truth": 40,
  "witness_live": -1.9358565897332864,
  "witness_odds_after": 5.708225244743692,
  "witness_odds_before": 2.5182763830777266,
  "witness_w_after": -0.7055691124106669,
  "witness_w_before": -4.100757279876157,
  "world": 205
}
```

## World 206 / switched

Trace: `results/authority/world206/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 15.26358250875005,
  "adaptive_live": -5.368315935694155,
  "adaptive_odds_after": 1.163102350398689,
  "adaptive_odds_before": 5.374512525242802,
  "candidate": -5.86654744948893,
  "ceiling_clipped": 0,
  "ceiling_live": -1.791886842186043,
  "ceiling_odds_after": -7.45412635952815,
  "ceiling_odds_before": -3.2427653580568565,
  "ceiling_w_after": 0.3076235299040738,
  "ceiling_w_before": 4.6647353722705445,
  "cold": -1.6552085875059142,
  "delta": 3.5306348811190724,
  "fast_live": -2.2655262146320245,
  "fast_odds_after": -5.013945787394904,
  "fast_odds_before": -0.801153707855957,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 3,
  "regime": "switched",
  "slow_live": -5.368315935694155,
  "slow_odds_after": 1.163102350398689,
  "slow_odds_before": 5.374512525242802,
  "t": 8803,
  "truth": 139,
  "witness_live": -5.322521723305115,
  "witness_odds_after": 1.0081303585797137,
  "witness_odds_before": 5.219535512242995,
  "witness_w_after": 0.3076235299040738,
  "witness_w_before": 4.6647353722705445,
  "world": 206
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 10.73296349900273,
  "adaptive_live": -2.298834602227849,
  "adaptive_odds_after": 5.575052727960831,
  "adaptive_odds_before": 2.999634964900693,
  "candidate": -2.158797288384484,
  "ceiling_clipped": 0,
  "ceiling_live": -4.534618835094051,
  "ceiling_odds_after": -2.4356492272556607,
  "ceiling_odds_before": -5.012112747250503,
  "ceiling_w_after": 4.64538011886482,
  "ceiling_w_before": 2.135628919558849,
  "cold": -4.735286891426669,
  "delta": -2.1323170886008853,
  "fast_live": -4.1645735927823235,
  "fast_odds_after": -0.6323793344287236,
  "fast_odds_before": -3.2065493057463264,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 3,
  "regime": "switched",
  "slow_live": -2.298834602227849,
  "slow_odds_after": 5.575052727960831,
  "slow_odds_before": 2.999634964900693,
  "t": 8522,
  "truth": 211,
  "witness_live": -2.4023017464931655,
  "witness_odds_after": 4.700000229538903,
  "witness_odds_before": 2.1241049466056587,
  "witness_w_after": 4.64538011886482,
  "witness_w_before": 2.135628919558849,
  "world": 206
}
```

## World 206 / moved_mid

Trace: `results/authority/world206/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 10.8792601425574,
  "adaptive_live": -4.291847158446146,
  "adaptive_odds_after": -0.015249883630005567,
  "adaptive_odds_before": 4.577509303329417,
  "candidate": -5.240277678510886,
  "ceiling_clipped": 0,
  "ceiling_live": -1.0165351880452183,
  "ceiling_odds_after": -6.293548248062408,
  "ceiling_odds_before": -1.699405309046376,
  "ceiling_w_after": -5.938140014930213,
  "ceiling_w_before": -1.3888254190709135,
  "cold": -0.6475622883056213,
  "delta": 2.8388652059930157,
  "fast_live": -0.7964026380677514,
  "fast_odds_after": -7.728141272831387,
  "fast_odds_before": -3.1340096612087365,
  "hazard": "2^-10",
  "matchedL": 4,
  "rank": 2,
  "regime": "moved_mid",
  "slow_live": -4.712821543978323,
  "slow_odds_after": 1.0909583088627837,
  "slow_odds_before": 5.683742607251713,
  "t": 11771,
  "truth": 84,
  "witness_live": -3.855400394038234,
  "witness_odds_after": -0.8558460537734097,
  "witness_odds_before": 3.739058343444332,
  "witness_w_after": -5.938140014930213,
  "witness_w_before": -1.3888254190709135,
  "world": 206
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 8.262415005289993,
  "adaptive_live": -2.890076294423778,
  "adaptive_odds_after": 8.085047561685096,
  "adaptive_odds_before": 3.5020426275453542,
  "candidate": -2.773329793054198,
  "ceiling_clipped": 0,
  "ceiling_live": -5.472893180680889,
  "ceiling_odds_after": 1.6766766532552575,
  "ceiling_odds_before": -2.9122481442205403,
  "ceiling_w_after": 3.8366723964490723,
  "ceiling_w_before": -0.7766139806592662,
  "cold": -7.362346983266934,
  "delta": -2.39647043210208,
  "fast_live": -5.312073531333429,
  "fast_odds_after": 1.9173241091342084,
  "fast_odds_before": -2.6649467961806406,
  "hazard": "2^-16",
  "matchedL": 1,
  "rank": 1,
  "regime": "moved_mid",
  "slow_live": -2.890076294423778,
  "slow_odds_after": 8.085047561685096,
  "slow_odds_before": 3.5020426275453542,
  "t": 8428,
  "truth": 48,
  "witness_live": -3.0764227485788087,
  "witness_odds_after": 6.607661535446175,
  "witness_odds_before": 2.020814827208121,
  "witness_w_after": 3.8366723964490723,
  "witness_w_before": -0.7766139806592662,
  "world": 206
}
```

## World 207 / switched

Trace: `results/authority/world207/switched.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 17.602115257694674,
  "adaptive_live": -5.273586961483812,
  "adaptive_odds_after": 4.094726938131003,
  "adaptive_odds_before": 7.814856147028183,
  "candidate": -5.349231488379329,
  "ceiling_clipped": 0,
  "ceiling_live": -1.9205558307689106,
  "ceiling_odds_after": -5.740565372402723,
  "ceiling_odds_before": -2.020811929850621,
  "ceiling_w_after": -2.118921479637211,
  "ceiling_w_before": 1.6524485545615943,
  "cold": -1.629500471510573,
  "delta": 3.3528607818216143,
  "fast_live": -3.4224527801414313,
  "fast_odds_after": -1.979479936924091,
  "fast_odds_before": 1.7420183098108084,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 2,
  "regime": "switched",
  "slow_live": -5.273586961483812,
  "slow_odds_after": 4.094726938131003,
  "slow_odds_before": 7.814856147028183,
  "t": 8680,
  "truth": 150,
  "witness_live": -5.273416612590525,
  "witness_odds_after": 4.091381831280576,
  "witness_odds_before": 7.811510168849011,
  "witness_w_after": -2.118921479637211,
  "witness_w_before": 1.6524485545615943,
  "world": 207
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 9.024369732329765,
  "adaptive_live": -1.7408632523608096,
  "adaptive_odds_after": 6.337963924552872,
  "adaptive_odds_before": 2.4465668722806218,
  "candidate": -1.5155714825438484,
  "ceiling_clipped": 0,
  "ceiling_live": -4.731168519353209,
  "ceiling_odds_after": -0.5739342995137083,
  "ceiling_odds_before": -4.467098476709647,
  "ceiling_w_after": 12.114350700895038,
  "ceiling_w_before": 8.486348099079192,
  "cold": -5.408772462455918,
  "delta": -2.7048831613771407,
  "fast_live": -4.430139662314612,
  "fast_odds_after": 0.15923244746520837,
  "fast_odds_before": -3.7309832757758685,
  "hazard": "2^-16",
  "matchedL": 2,
  "rank": 1,
  "regime": "switched",
  "slow_live": -1.6826868592095119,
  "slow_odds_after": 6.8028442935938855,
  "slow_odds_before": 2.9121253079603022,
  "t": 9497,
  "truth": 65,
  "witness_live": -2.026285357976068,
  "witness_odds_after": 4.982351422932924,
  "witness_odds_before": 1.089868511434978,
  "witness_w_after": 12.114350700895038,
  "witness_w_before": 8.486348099079192,
  "world": 207
}
```

## World 207 / moved_mid

Trace: `results/authority/world207/moved_mid.tsv.gz`.

### raw_cap_help

```json
{
  "adaptive_e_before": 12.322196650256398,
  "adaptive_live": -13.0466007852641,
  "adaptive_odds_after": -2.49528206546884,
  "adaptive_odds_before": 7.37750948274258,
  "candidate": -15.768761053747932,
  "ceiling_clipped": 0,
  "ceiling_live": -6.362668296974934,
  "ceiling_odds_after": -11.25932167995118,
  "ceiling_odds_before": -1.3865340270941071,
  "ceiling_w_after": -8.679708988309782,
  "ceiling_w_before": 1.231542339736133,
  "cold": -5.895995423818771,
  "delta": 6.6343676757825,
  "fast_live": -7.592083574369674,
  "fast_odds_after": -8.705548793453689,
  "fast_odds_before": 1.1686297847958917,
  "hazard": "2^-16",
  "matchedL": 2,
  "rank": 4,
  "regime": "moved_mid",
  "slow_live": -13.177407731206552,
  "slow_odds_after": -2.338923646913295,
  "slow_odds_before": 7.533868348229947,
  "t": 13050,
  "truth": 173,
  "witness_live": -12.997035972757434,
  "witness_odds_after": -2.5538807612440544,
  "witness_odds_before": 7.318910631560243,
  "witness_w_after": -8.679708988309782,
  "witness_w_before": 1.231542339736133,
  "world": 207
}
```

### raw_cap_harm

```json
{
  "adaptive_e_before": 5.546120943309364,
  "adaptive_live": -1.953873375289084,
  "adaptive_odds_after": 4.48574609771679,
  "adaptive_odds_before": 1.0484894817971715,
  "candidate": -1.4478677748719209,
  "ceiling_clipped": 0,
  "ceiling_live": -4.611842748019986,
  "ceiling_odds_after": -2.0878818134292403,
  "ceiling_odds_before": -5.525626560702717,
  "ceiling_w_after": 4.252996046383999,
  "ceiling_w_before": 0.8415216587005846,
  "cold": -4.885639714389729,
  "delta": -2.467814795793646,
  "fast_live": -4.709754853257226,
  "fast_odds_after": -2.790009656067879,
  "fast_odds_before": -6.22616810853314,
  "hazard": "2^-16",
  "matchedL": 3,
  "rank": 3,
  "regime": "moved_mid",
  "slow_live": -1.837685720111338,
  "slow_odds_after": 4.939750269167527,
  "slow_odds_before": 1.502676140116648,
  "t": 11825,
  "truth": 221,
  "witness_live": -2.14402795222634,
  "witness_odds_after": 3.8930011741073955,
  "witness_odds_before": 0.45557833350975396,
  "witness_w_after": 4.252996046383999,
  "witness_w_before": 0.8415216587005846,
  "world": 207
}
```
