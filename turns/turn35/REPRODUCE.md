# Reproduce turn35

Use an isolated checkout containing Don turn34 commit `3c780af…` and these
turn35 files. Requirements are a C11 compiler, make, and the Python standard
library.

Before any fresh world exists:

    make all
    python3 -B fixture_check.py --output /absolute/new/FIXTURE.json
    python3 -B experiment.py freeze

Run exactly one declared batch:

    python3 -B experiment.py generate
    python3 -B experiment.py extract
    python3 -B experiment.py learn
    python3 -B experiment.py evaluate

Then create the independent receipt:

    python3 -B verify.py --output /absolute/new/VERIFY.json

All output paths are creation-only. `data/`, `memory/`, and `results/` must
be absent at freeze. This is an isolated experiment, not authority to attach
the mechanism to Netta's mouth, mycelium, or any canonical runtime.
