# Reproduce turn29

Base commit: `dd6aeec368dd47bc6444a87508be7111e44a0848` plus this turn's files.
Use a fresh isolated checkout. C11 compiler, make and Python standard library
only. Run from `turns/turn29`:

```
make all
./calibrate --fixture
python3 -B source_check.py
python3 -B experiment.py freeze
python3 -B experiment.py generate
python3 -B experiment.py extract
python3 -B experiment.py learn
python3 -B experiment.py evaluate
python3 -B verify.py --output VERIFY.json
```

For a true reproduction, copy only source/protocol/interface/reader files into
a fresh base and omit original FREEZE, manifests, data, memory, results and
receipts. Stage writers refuse overwrite. A different compiler may change
frozen binary hashes; source worlds and scientific numbers are deterministic.

Large raw bytes, archives and traces remain in ignored local directories and
are content-addressed by committed manifests. Do not use reproduction to try a
second prior, threshold, router or selected subset on worlds272--279.
