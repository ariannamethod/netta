# Turn37 state and trace interface

The trace records pretruth sources, corrected sources, state before and after
the truth, shared admission, and independent outer trajectories for seven
arms.

Source order:

    pooled, coarse A, coarse B, episodes E1..E4, null episodes N1..N4

Corrected order:

    direct, coarse, fine, hierarchy, null

State vector order, 19 doubles per matched address:

    permission,
    coarse hA,hB,
    direct g1,g2,g3,g4,
    hierarchy A1,A2,B1,B2,
    null A1,A2,B1,B2,
    flat-fine f1,f2,f3,f4

The direct arm uses the pretruth coarse corrected source as the base for both
prediction and wealth update. All states update only after the emitted row's
truth is read. The pooled quote alone updates permission and prospective
admission. NEW, no-match and equal-source paths retain exact inherited
passthrough behavior.
