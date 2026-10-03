# Reproduce turn33

Use an isolated checkout at base
4dedb745ff044bd735dfa55f5a0e87ad9780d298 plus the turn33 source files.
The build needs a C11 compiler, make and the Python standard library.

From turns/turn33, before any new worlds:

    make all
    python3 -B fixture_check.py --output /absolute/new/FIXTURE.json
    python3 -B experiment.py freeze

Then execute exactly one declared batch:

    python3 -B experiment.py generate
    python3 -B experiment.py extract
    python3 -B experiment.py learn
    python3 -B experiment.py evaluate

The original frozen reader is intentionally retained as failed evidence. For
the sealed artifacts use the disclosed separate reader:

    python3 -B verify_repair.py --output /absolute/new/VERIFY.json

The output path must not exist. Every stage after freeze checks all original
freeze pins and its incoming manifests. data/, memory/ and results/ must be
absent at freeze time; all writes are creation-only.

Do not replace verify.py, FREEZE.json, RESULT.json or the original refusal
records. READER_REPAIR.md gives the complete repair boundary. The repaired
reader verifies retained artifacts; it does not generate worlds, change a
candidate or tune any value.

Ignored evidence directories are retained locally and pinned by:

- DATA_MANIFEST.json: 280 files;
- EXTRACT_MANIFEST.json: 344 files;
- MEMORY_MANIFEST.json: 32 files;
- RESULTS_MANIFEST.json: 80 files.

The compressed target traces occupy about 200 MiB in this checkout. Scientific
identity is the manifest content and decompressed rows, not filesystem
timestamps. A compiler or platform may change the executable hash; a new
reproduction must record its own freeze rather than editing this one.

This experiment is not a live Netta installation. Reproduction must not attach
the organ to the mouth, Body 0, mycelium or any canonical runtime.
