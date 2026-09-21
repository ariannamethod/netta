# Reproducing turn 6

This isolated branch tests one outer HMM transition rate on fresh synthetic
worlds. Start with [PROTOCOL.md](PROTOCOL.md), then read [REPORT.md](REPORT.md)
and [RAW.md](RAW.md). `RESULT.json` is the writer result; `VERIFY.json` is
the separate probability-mass reader result. The material verdict depends
on both. No live Netta code is modified.

From this original working tree, with retained ignored artifacts, re-run
the independent reader into a new output path:

```sh
NETTA_VERIFY_TMP=$(mktemp -d /private/tmp/netta-turn6-check-XXXXXX)
python3 turn6/verify.py --output "$NETTA_VERIFY_TMP/VERIFY.json"
shasum -a 256 "$NETTA_VERIFY_TMP/VERIFY.json" turn6/VERIFY.json
```

The two verification files should be byte-identical. The original run's
raw `data/`, `traces/`, `memory/`, and `results/` are intentionally ignored
by git but retained on this machine; all their hashes are in
`ARTIFACT_MANIFESTS.json`.

For a clean regeneration, export preregistration commit `f733f6f` to a new
temporary directory, copy only `turn6/experiment.py` and `turn6/verify.py`
there, build the inherited binaries, then run in stage order:

```sh
make -C byte_recurrence all
make -C turn3 bank
make -C turn5 all
python3 turn6/experiment.py freeze
python3 turn6/experiment.py generate
python3 turn6/experiment.py extract
python3 turn6/experiment.py evaluate
python3 turn6/verify.py
```

The stage scripts refuse existing outputs. Compare the clean generation's
raw and bank hashes and every numerical result with the original receipts.
Compiler/platform differences may alter binary hashes; a clean run's freeze
is its own receipt, not a replacement for the original. This reproduction
uses the turn-5 C quote and its evaluator unchanged; the new independent
reader checks the outer probability law, not BPE training from scratch.
