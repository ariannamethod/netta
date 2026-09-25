# Reproduce turn30

Base commit: `cfc123f183a58a0030e356d693e5fbacfda38d2c` plus this turn's files.
Use a fresh isolated checkout. C11 compiler, make and Python standard library
only. Run from `turns/turn30`:

```
make all
./router3 --fixture
python3 -B source_check.py
python3 -B probes.py
python3 -B experiment.py freeze
python3 -B experiment.py generate
python3 -B experiment.py extract
python3 -B experiment.py learn
python3 -B experiment.py evaluate
python3 -B verify.py --output VERIFY.json
```

`source_check.py` and `probes.py` write their receipts to stdout;
`SOURCE_PREFLIGHT.json` and `PREFREEZE_PROBES.json` are those two streams,
captured before `freeze` and therefore before any world exists. The freeze
stage refuses to run once `data`, `memory` or `results` exist, and every later
stage re-digests all 28 frozen files before doing anything.

For a true reproduction, copy only source/protocol/interface/reader files into
a fresh base and omit the original FREEZE, manifests, data, memory, results and
receipts. Stage writers refuse overwrite. A different compiler may change
frozen binary hashes; source worlds and scientific numbers are deterministic.

The frozen binary hashes also depend on where the build runs. Apple clang
21.0.0 writes the output path into the Mach-O image, so the same source, flags
and output name compiled in another directory digests differently — measured
here, `395e639f…` at `turns/turn30/router3` against `e8a5b512…` for the same
bytes of source built elsewhere. Rebuilding turn29's `calibrate` and turn28's
`bank` in their own directories reproduced their pinned hashes exactly, which
is what makes an inherited-binary pin auditable at all. An auditor who
reconstructs the tree at a different absolute path should expect the three
binary pins to differ and check the sources and results instead.

Large raw bytes, archives and traces remain in ignored local directories and
are content-addressed by committed manifests. Do not use reproduction to try a
second prior, share, router shape or address set on worlds280--287. The gate
was fixed before the data existed and its result stands as it fell.
