# Reproduce turn28

Base commit: `48e36938f887ec57b26e4ac20b83f58dd5d149ab` plus this turn's files.
Use an isolated checkout. C11 compiler, make and Python standard library only.
Commands run from `turns/turn28`. Original outputs refuse overwrite.

Build and preflight, before new data:

```sh
make all
./bank --fixture
python3 -B source_check.py
python3 -B verify.py --help
```

Run the fixed batch ONCE in a fresh copy without generated stage outputs:

```sh
python3 -B experiment.py freeze
python3 -B experiment.py generate
python3 -B experiment.py extract
python3 -B experiment.py learn
python3 -B experiment.py evaluate
python3 -B verify.py --output VERIFY.json
```

The freeze requires absent data/memory/results directories. For reproduction,
copy only the new source/protocol/interface/Makefile/source-check/reader files
into turn28 of a fresh base checkout; omit FREEZE and all original stage
outputs. Never delete or overwrite the original evidence to repeat a run.
The inherited `turns/turn13` and `turns/turn22` modules are read-only
dependencies. The Makefile supplies `episode` as the same executable as `bank`
for the inherited emitter and source trace commands.

Original FREEZE pins compiler-produced bytes on this machine. A new compiler
may make different binary hashes; a reproduction's freeze explicitly records
that fact. Source raw bytes, archives and scientific numbers are deterministic.
Gzip wrappers and stderr allocation/timing details need not be byte-identical.
Large raw data/archives/recipient traces remain locally under data/, memory/,
results/; their manifests are retained for source and result verification.

The independent reader takes HEAD256 cold probabilities/bindings and the
inherited full selected list as supplied inputs. It independently reconstructs
the new two-book counts, serialized archives, matching, mixtures, evidence,
received prices, metrics and decision. This boundary is explicit in READER.md.

Do not use a reproduction as an opportunity to alter priors, select prefixes
against these recipients, or replace individual worlds. Every losing world is
part of the result. Research outputs do not install this organ in live Netta.
