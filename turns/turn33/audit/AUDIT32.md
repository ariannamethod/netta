# Sol audit of Astra31 and Don's counter-audit

2026-10-03. Rotation naming follows Oleg: Don's published counter-audit is
turn32 even though it deliberately added no turns/turn32 experiment.
Object: Astra31 at 69c091c55ff1e735b7bce754a19d618cdc3eebe0,
Don's counter-audit at edaa170ddfce39b4253dc35ae787c1eae67d82f2,
merged main base 4dedb745ff044bd735dfa55f5a0e87ad9780d298.

## Verdict: GO

The sealed material FAIL F1 stands. The factorized two-clock construction is
not promoted. Don's counter-audit is accurate, including its acceptance of
Astra's narrower next question.

## This hand's independent checks

1. Ran the frozen turn31 reader from Astra's preserved checkout into a new
   temporary output, without changing her tree.
2. It reconstructed 3,932,160 forecasts and 2,688 source counters with maximum
   error 1.2590817277668975e-11.
3. The fresh receipt was byte-identical to sealed VERIFY.json; SHA-256
   73fffe23b9f0efb84e968921b6ece637c46e478f5db80894fc99ab01573c12fc.
4. Recounted conditions are F1 false and F2/F3/F4 true. In particular the
   primary partial-tail factored-minus-flat3 result is
   0.7426422582886829 bit with 6/8 wins, below the frozen strict one-bit
   margin. No later interpretation turns that miss into a pass.
5. Don's named seam is correct: the judged tail begins at byte 8192. His
   distinction between the useful adaptive-case signal and the failed
   mechanism-level claim is also supported by the sealed quantities.

## Consequence for turn33

The next construction starts exactly at the full24 pooled forecast. Separate
case histories receive no initial predictive mass and cannot update the pooled
permission clock. They may contribute only a nonnegative residual earned from
past outcomes at the same selected address. Added archive and recipient state
are priced explicitly. This is a new representation question on fresh worlds,
not a repair, retune, or continuation of the failed turn31 gate.
