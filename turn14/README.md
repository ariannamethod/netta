# Netta turn14: joint memory with fixed faster withdrawal

This directory contains one frozen synthetic experiment. It composes Astra's
turn13 528-byte joint-prefix memory with Sol's previously fixed turn6
source-to-cold hazard. Live Netta is not modified.

- [PROTOCOL.md](PROTOCOL.md): question, exact change and predata gate.
- [REPORT.md](REPORT.md): result, price and boundary.
- [RAW.md](RAW.md): per-byte help/harm and every changed tail.
- [RESULT.json](RESULT.json): writer result.
- [VERIFY.json](VERIFY.json): independent source/candidate reconstruction and
  Decimal mass replay.
- [FREEZE.json](FREEZE.json): frozen source/build hashes.

The retained run used worlds 144..151 under namespace
`netta-joint-fast-withdraw-v1`. Raw data, learned archives and compressed
traces are local ignored artifacts pinned by `DATA_MANIFEST.json`,
`MEMORY_MANIFEST.json` and `RESULTS_MANIFEST.json`.

To reproduce in a clean copy with Python 3, a C11 compiler and `make`:

```sh
make -C turn14
python3 turn14/experiment.py freeze
python3 turn14/experiment.py generate
python3 turn14/experiment.py extract
python3 turn14/experiment.py learn
python3 turn14/experiment.py evaluate
python3 turn14/verify.py --output turn14/VERIFY.json
```

Each stage refuses to replace existing evidence. Build timestamps and gzip
headers may differ; raw streams, archive bytes, decompressed predictions and
the independently recovered numerical verdict are the reproducible objects.
