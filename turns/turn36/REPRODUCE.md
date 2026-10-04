# Reproduce turn36

Use an isolated checkout containing the parent main snapshot and this turn.
Requirements: C11 compiler, make, Python standard library. Source/recipient
raw inputs, archived counts and traces are retained locally under `data/`,
`memory/`, `results/`; committed manifests identify them.

Build from the repository root:

    make -C turns/turn36 all

The build stages only renamed entry points of frozen parents. The emit/trace
frontend and controls are included from those parents. Each `make` staging
recipe checks its exact expected diff.

The independent fixture is creation-only:

    python3 -B turns/turn36/fixture_check.py --output /absolute/new/FIXTURE.json

To replay retained raw artifacts without changing the sealed receipt:

    python3 -B turns/turn36/verify.py --output /absolute/new/VERIFY.json

For a full regeneration, use a fresh working directory for the turn, preserving
the original freeze, manifests, results and logs as evidence. Compile and run
the fixture first; then `experiment.py freeze`, `generate`, `extract`, `learn`,
`evaluate` and `verify.py` sequentially. Generation is namespace-seeded. Never
reuse the sealed directory as scratch space or overwrite its first result.

`run_trial.py` records those stages into creation-only stdout/stderr files.
The reader imports only frozen readers, independently recounts source lives,
rebuilds all four archives, and checks all six prediction streams. It treats
HEAD256/P0 and greedy source selection as named inherited supplied boundaries.

The prior-world preflight uses Don's local source path, recorded in
`preflight.py`, and is an integration check before fresh generation. It is
not required to read or reproduce the judged fresh batch.
