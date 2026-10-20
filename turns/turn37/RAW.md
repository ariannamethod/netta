# Turn37 raw switched-tail behavior

The first pair is selected mechanically as the largest received help and harm
of `direct-coarse` on the switched tails. The second pair is selected inside
the narrower set where both coarse wealths are nonpositive, at least one
direct episode wealth is positive and direct differs from coarse. Complete
rows remain in `VERIFY.json`; source counts below are in NEW, repeat1..6 order.

## Largest received help: world328, t8911

Truth 128, repeat rank3, record2. Direct beats coarse by **+3.928259710**
received bits.

Raw neighborhood:
`674bfc674b808087b74b874b808087e180878afc87fcfc8787fc8a878787fc8afc`

Both coarse wealths are negative `[-1.204874,-1.993332]`; all four direct
wealths are also negative `[-0.607568,-2.608997,-2.950561,-1.040603]`.
Direct and coarse candidates are therefore equal at `−3.012389692`. Their
different inherited outer histories make the received prices diverge:
direct `−3.012675942`, coarse `−6.940935652`. This byte is a path effect from
earlier direct activity, not a current residual.

Episode counts:

    E1 [576,37,34,56,0,0,0]   E2 [583,50,29,68,0,0,0]
    E3 [1106,40,61,72,0,0,0] E4 [1137,37,58,57,0,0,0]

## Largest received harm: world328, t8830

Truth 128, repeat rank2, record7. Direct loses **−4.100237635** received bits.

Raw neighborhood:
`12125cc9255c5c5c747474856280858580805a80d7d790d715d71590151544f415`

Coarse wealths `[-0.053743,-9.714941]` and direct wealths
`[-0.219084,-0.128486,-9.647828,-9.539224]` are all nonpositive, so both
candidates equal `−6.576155075`. Direct's prior outer state receives
`−6.569013726`; coarse receives `−2.468776092`. The largest local harm is also
an accumulated path effect and remains beside the PASS.

Episode counts:

    E1 [28,508,10,9,0,0,0] E2 [22,487,10,6,0,0,0]
    E3 [0,0,1,1,0,0,0]    E4 [0,0,0,1,0,0,0]

## Parent-veto escape help: world328, t10021

Truth 18, repeat rank3, record20. Both coarse wealths are negative
`[-0.143345,-5.702760]`, so coarse returns to pooled. Direct E2 has earned
`+0.946128`; the other direct wealths are
`[-0.136287,-6.410807,-1.863909]`. Direct changes the source price from
`−5.973222716` to `−3.399016234` and beats coarse by **+2.387916448 candidate
bits** and **+2.377459970 received bits**.

Raw neighborhood:
`1f6289aa4d4d894d1289252512ea7878124444d19191d1f491f4d191f4f46b9ef4`

Episode counts:

    E1 [500,13,10,8,0,0,0] E2 [493,7,8,10,0,0,0]
    E3 [7,11,478,5,0,0,0] E4 [17,5,510,13,0,0,0]

This is the phenomenon turn36 could not express: a useful episode speaks
while its broader family is silent.

## Parent-veto escape harm: world333, t11957

Truth 62, repeat rank1, record10. Coarse wealths are
`[-7.443397,-1.472030]`. Direct E3/E4 wealths are positive
`[+0.207348,+3.320991]`, while E1/E2 are negative. Their residual is wrong for
this truth: the source price moves from `−1.145486376` to `−3.735764901`.
Direct loses **−2.214492468 candidate bits** and **−2.205317152 received bits**.

Raw neighborhood:
`3e3e8af7f78a8a8af7f73ed6b7b73e3e3e3e39483e92929292484892489248d6d6`

Episode counts:

    E1 [16,518,17,14,0,0,0] E2 [17,510,8,8,0,0,0]
    E3 [0,0,21,1,0,0,0]   E4 [0,1,18,0,0,0]

Direct authority is earned and revocable, yet it can still be wrong at a
particular byte. The negative event is part of the measured mechanism.
