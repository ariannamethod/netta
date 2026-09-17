# Sol turn 8 — reciprocal review and saved-state authority ledger

2026-09-17. This hand is diagnostic, not a new controller or a new material
experiment. Astra's world64–71 **AUTHORITY-CAP10 material FAIL remains FAIL**.
No source, threshold, target, policy, live Netta file, or canonical worktree
was changed. The turn-7 branch remains uncommitted and unpushed.

## Reciprocal review

I read the predata `PROTOCOL.md`, `NUMERICS.md`, `PREFREEZE_REVIEW.md`, C
wrapper, writer, reader, freeze, result, raw observations, and handoff. The
C wrapper completes the entire quote before reading the observed byte; the
only new outer rule caps the *next* source odds after that byte. It keeps
the admission-crossing byte cold, reuses the inherited candidate and local
row update, and uses the same C/S stream for all outer modes. The fixed
gate's thirteen booleans correspond to the written inequalities. The
prefreeze review describes the numerical correction and near-cap tie
convention before fresh generation. Hashes in the freeze and artifact
manifest were checked by the replay below.

I reran Astra's separately written probability-mass reader on her saved
artifacts with a new output path. It checked **2,359,296** outer forecasts,
all identities, admission and numerical bounds. Its output is byte-identical
to the sealed `turn7/VERIFY.json` (SHA-256
`e76c21570210ddc1df2e5993af1623e0aee3f28090b5578c81cb6273d05a1318`).
Maximum numeric error is `2.5082158572331537e-11` bit; cap/fast source
ordering has no violation. The reader's independent scope is *outer
probability-mass reconstruction from frozen component quotes*, not an
independent rebuild of tokenization or source books. This was a rerun of
existing data, not a fresh regeneration of the worlds or C traces.

Seven utility conditions pass, six fail. Cap keeps `202.946160` mean mosaic
bits, versus old `202.826644` and fast `183.490295`. On the changed tail
it gets `9.890760` versus old `10.759664` and fast `12.537713`: **only 1/8
tails beat old**. Full switched-life gain is `154.689617` versus old
`155.503365`: **only 2/8 beat old**. Positive gain relative to cold or fast
does not satisfy the registered old-relative conditions. No material PASS or
live integration follows from the mathematical loss bound.

## One bounded next question, on saved states only

`QUESTION.md` fixed the descriptive question and exclusions before my
ledger code was written. `ledger.py` reads Astra's existing paired outer
traces; `LEDGER.json` contains every world, unchanged mosaic and switched
life, both halves, and all three realized sign classes. It checks the exact
log-odds telescoping identity at each active byte, the switch, and the end,
plus price sums against the sealed `RESULT.json`. Sixteen lives pass these
checks. No new predictor was run.

For each active byte define `delta=log2(S(y)/C(y))` and the recorded
withdrawal `d=odds_before+delta-odds_after`. `d_fast-d_cap` sums to the
observed cap-minus-fast odds gap. The crossing byte is excluded. Means below
are per world; withdrawals are **log-odds units**, not predictive bits:

| Region of switched life | Equal-price events | Equal contribution | Favorable contribution | Adverse contribution | Net odds gap gained |
| --- | ---: | ---: | ---: | ---: | ---: |
| Before switch | 21,920/8 worlds | +51.190078 | −85.704363 | +41.458623 | +6.944338 |
| After switch | 23,368/8 worlds | +9.453685 | −10.095790 | +9.602486 | +8.960380 |

At the switch, cap has `6.944338` more source log-odds on average than fast
(individual worlds `5.602–8.133`). On a realized equal-price byte there is
**zero immediate price difference**, but fast withdraws and cap does not.
This clock contributes materially to the saved odds gap, alongside adverse
evidence. Favorable observations undo much of that gap because cap can clip
its own posterior near the ceiling. The three terms must be kept together;
the +51.19 neutral contribution is not a +51.19-bit predictive benefit.

Beside the ledger, the changed-tail *actual* paired cap-minus-fast price
is `+90.013547` bits/world on favorable observations and `−92.660500` on
adverse ones, net `−2.646953`; equal observations are zero. Mean pretruth
source weight on changed-tail adverse events is `0.433908` for cap and
`0.168763` for fast. The same retained authority helps on other bytes.
On the unchanged mosaic cap beats fast by `19.455866` bits/life. There is
no universal instruction to lower source weight on every disagreement.

This ledger answers *where the recorded odds gap was accumulated*, not what
would have happened if a clock transition were removed. Later posteriors
would change under such an intervention. Worlds64–71 are sealed explanatory
evidence, not a tuning set. The next causal question is whether a controller
can retain useful early influence and reduce exposure after genuine
candidate/cold disagreement **without** relying on a world-switch marker
or charging every neutral byte. It needs one predata rule, the same
whole-life and changed-tail comparisons, and fresh worlds; no such rule has
been chosen or tested in this hand.

## Boundaries and files

- `QUESTION.md`: question and exclusions recorded before the ledger code.
- `ledger.py`: read-only trace accounting; standard-library Python.
- `LEDGER.json`: all sixteen lives and their source-trace hashes.
- `turn7/VERIFY.json`, `RESULT.json`, `VERDICT.json`: Astra's unchanged
  outcome in her isolated worktree.

The diagnostic hand originally ended without a commit, push, merge or
canonical change. Oleg subsequently explicitly asked to publish it; this
copy is carried on isolated branch `sol/authority-ledger-20260917`.
Publication does not convert Astra's turn-7 FAIL into a PASS and does not
integrate a controller into live Netta.
