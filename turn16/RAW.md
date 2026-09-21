# Turn16: observed returns and their prices

Rows come from retained candidate and C authority TSVs. Event t changes the
following quote t+1. Delta is return minus the identical-prior budget control.
Selection is post hoc: sole post-seam surface return; sole post-seam law
return; largest early benefit. Each displayed window includes losses.

## World 178, moved_mid

Return after byte t=9452; next byte 9453.
Support 32.730611460 bits; pretruth odds -12.757189522; next odds -1.271553303.
Next 256 bytes: +7.718298854 extra bits versus budget.

| row | t | truth hex | rank | matched L | cold log2 | candidate log2 | budget log2 | return log2 | delta |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|
| crossing | 9452 | 6e | 3 | 2 | -4.304642492 | -1.033885673 | -4.302840983 | -4.302840983 | +0.000000000 |
| next | 9453 | 6e | 1 | 4 | -3.073097159 | -1.526225952 | -3.069246110 | -2.428880584 | +0.640365526 |
| help | 9478 | f9 | 3 | 2 | -5.212854132 | -2.658226679 | -5.005936702 | -2.785394455 | +2.220542247 |
| harm | 9465 | bc | 1 | 1 | -0.823416550 | -1.556346276 | -0.834404059 | -1.417170939 | -0.582766880 |

Crossing history: 30020112312330020112212330020112.
Crossing stored votes for NEW, ranks 1..6: 319,126,182,2349,0,0,0.
Crossing source price is still charged under old authority; the next quote uses the renewed prior.

## World 181, switched

Return after byte t=10315; next byte 10316.
Support 32.791157717 bits; pretruth odds -6.680516627; next odds -1.271553303.
Next 256 bytes: -0.469579263 extra bits versus budget.

| row | t | truth hex | rank | matched L | cold log2 | candidate log2 | budget log2 | return log2 | delta |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|
| crossing | 10315 | 80 | 1 | 1 | -2.855179654 | -1.870994588 | -2.841618144 | -2.841618144 | +0.000000000 |
| next | 10316 | 07 | 2 | 0 | -4.042369611 | -4.042369611 | -4.042369611 | -4.042369611 | +0.000000000 |
| help | 10433 | be | 1 | 3 | -4.938922831 | -2.143652734 | -4.938123417 | -4.921894760 | +0.016228657 |
| harm | 10317 | 66 | 3 | 3 | -2.195231582 | -3.471834306 | -2.211319263 | -2.467230622 | -0.255911359 |

Crossing history: 01111330221101003022210200122230.
Crossing stored votes for NEW, ranks 1..6: 1816,6391,4058,4157,0,0,0.
Crossing source price is still charged under old authority; the next quote uses the renewed prior.

## World 177, recombined

Return after byte t=1093; next byte 1094.
Support 32.006578174 bits; pretruth odds -47.502463939; next odds -1.271553303.
Next 256 bytes: +43.688679426 extra bits versus budget.

| row | t | truth hex | rank | matched L | cold log2 | candidate log2 | budget log2 | return log2 | delta |
|---|---:|---|---:|---:|---:|---:|---:|---:|---:|
| crossing | 1093 | 2b | 2 | 2 | -3.597327890 | -1.553687110 | -3.597327890 | -3.597327890 | +0.000000000 |
| next | 1094 | 03 | 3 | 3 | -1.610622205 | -2.505878590 | -1.610622205 | -1.820548666 | -0.209926462 |
| help | 1124 | 55 | 2 | 2 | -6.311257636 | -1.573196579 | -6.311257631 | -1.589804618 | +4.721453013 |
| harm | 1162 | 53 | 3 | 1 | -2.654358914 | -6.034796013 | -2.654358924 | -5.837271949 | -3.182913025 |

Crossing history: 01320201231301320201131301320201.
Crossing stored votes for NEW, ranks 1..6: 1397,210,3470,1382,0,0,0.
Crossing source price is still charged under old authority; the next quote uses the renewed prior.

