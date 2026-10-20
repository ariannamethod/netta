# Reproduce turn37

Requirements: C11 compiler, make and Python standard library. The ignored
`data/`, `memory/` and `results/` directories are pinned by committed
manifests and can be regenerated deterministically from the frozen namespace.

From the repository root:

    make -C turns/turn37 all
    python3 -B turns/turn37/fixture_check.py --output /absolute/new/FIXTURE.json
    python3 -B turns/turn37/verify.py --output /absolute/new/VERIFY.json

The fixture and reader receipts are creation-only. A second run must use a new
output path.

For a full regeneration, copy the frozen tracked turn into a fresh checkout,
leave `FREEZE.json` unchanged, and run:

    python3 -B turns/turn37/experiment.py generate
    python3 -B turns/turn37/experiment.py extract
    python3 -B turns/turn37/experiment.py learn
    python3 -B turns/turn37/experiment.py evaluate
    python3 -B turns/turn37/verify.py --output /absolute/new/VERIFY.json

`experiment.py freeze` is used only before the first batch. Do not overwrite
the retained batch or its first receipts. `diagnose.py` reads the sealed
switched traces after judgment and writes a creation-only descriptive receipt.
