# Reproduce turn34

Use an isolated checkout at base 1fda545 plus the turn34 source files.
The build needs a C11 compiler, make and the Python standard library.

From turns/turn34, before any new worlds:

    make all
    python3 -B fixture_check.py --output /absolute/new/FIXTURE.json
    python3 -B experiment.py freeze

Then execute exactly one declared batch:

    python3 -B experiment.py generate
    python3 -B experiment.py extract
    python3 -B experiment.py learn
    python3 -B experiment.py evaluate

For the sealed artifacts use the independent reader:

    python3 -B verify.py --output /absolute/new/VERIFY.json

The output path must not exist. Every stage after freeze checks all frozen
pins and its incoming manifests. data/, memory/ and results/ must be absent
at freeze time; all writes are creation-only. The first freeze's retained
refusal and its disclosed one-pin repair are FREEZE1_FAILURE.md and
BUILD.md; a new reproduction records its own freeze rather than editing
this one.

The worlds are namespace-seeded and deterministic: a second batch from this
freeze reproduces RESULT.json's conditions, tables and quantities exactly,
as this record's own two batches did.

This experiment is not a live Netta installation. Reproduction must not
attach the organ to the mouth, Body 0, mycelium or any canonical runtime.
