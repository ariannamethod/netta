# Reproduce turn31

Base commit `bbd8dbb41f88d5a6697ca03533176bdbbfa6ee49`, plus turn31 sources.
C11 compiler, make, Python standard library. Use an isolated checkout.

From turns/turn31, build and inspect the arithmetic fixture before any worlds:

```
make all
./case_router --fixture
```

Run the declared batch once in a fresh reproduction folder/tree containing
PROTOCOL.md, INTERFACE.md, case_router.c, Makefile, experiment.py, verify.py
and inherited tracked dependencies. Exclude prior FREEZE/stage outputs and
ignored raw directories from that reproduction copy. Original evidence stays
where it is. Then from that copy's turns/turn31:

```
python3 -B experiment.py freeze
python3 -B experiment.py generate
python3 -B experiment.py extract
python3 -B experiment.py learn
python3 -B experiment.py evaluate
python3 -B verify.py --output VERIFY.json
```

Every stage checks frozen files. The freeze requires absent data/, memory/,
results/. Stage receipts use exclusive creation. Do not overwrite the original
outputs to repeat a run or try another constant on these sealed recipients.

The independent reader also supports checking the retained raw artifacts with
an absent --output path outside this directory. It does not generate data or
learn parameters. Read READER.md for its supplied boundaries.

Raw source bytes/traces, archives and recipient traces remain in ignored
`data/`, `memory/`, `results/`. Their committed manifests record exact content
hashes. Gzip metadata and allocation/timing stderr can vary between executions;
compare decompressed scientific rows and reconstructed numbers. Compiler/path
changes can change the binary digest; record the new build's own freeze rather
than altering the old one. All new source changes are scoped to turns/turn31.

This is a shared-admission research organ. It does not install itself in live
Netta or alter the source archive/recipient reset contracts of existing bodies.
