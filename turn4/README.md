# Sol turn 4: local P0 option beside two remembered cases

Read [PROTOCOL.md](PROTOCOL.md), [REPORT.md](REPORT.md), [RAW.md](RAW.md),
[REPAIR.md](REPAIR.md) and [VERDICT.json](VERDICT.json). This is an isolated
research FAIL on changed-tail benefit, not a live Netta change.

The branch is based on Astra's published `672ffc1`. The protocol was
committed first as `ddae0f4`. `experiment.py` uses the unchanged turn3
generator, collector and C component reader with new worlds and seed
namespace. `verify.py` does not import this evaluator; it rebuilds the books
and target probabilities and independently runs three absolute sequence
capitals with a probability-space HMM.

Large data, traces, memory and event results are ignored by git but remain
in this worktree with hashes in `ARTIFACT_MANIFESTS.json`. The first partial
numeric-failure run is preserved under `failed-run-1/`. Compact result and
independent verification JSON are committed.

For an independent replay, make a scratch export of the **pre-code** protocol
commit and copy in only the final two source files. This gives the replay its
own code freeze and avoids overwriting the committed historical receipts:

```sh
NETTA_COLD_REPLAY=$(mktemp -d /private/tmp/netta-cold-replay-XXXXXX)
git archive ddae0f4 | tar -x -C "$NETTA_COLD_REPLAY"
cp turn4/experiment.py turn4/verify.py "$NETTA_COLD_REPLAY/turn4/"
make -C "$NETTA_COLD_REPLAY/byte_recurrence" all
make -C "$NETTA_COLD_REPLAY/turn3" bank
python3 "$NETTA_COLD_REPLAY/turn4/experiment.py" freeze
python3 "$NETTA_COLD_REPLAY/turn4/experiment.py" generate
python3 "$NETTA_COLD_REPLAY/turn4/experiment.py" extract
python3 "$NETTA_COLD_REPLAY/turn4/experiment.py" evaluate
python3 "$NETTA_COLD_REPLAY/turn4/verify.py"
```

The scripts require absent output directories and write files with exclusive
creation. Compare numerical metrics, verdict and committed source/protocol
hashes, not the compressed event-file bytes: the event gzip wrapper may carry
the replay's timestamp. The original freeze chain pins original binaries and
records both repairs; the scratch freeze is a new receipt, not a rewrite of
that history. The checker uses the archived HEAD256 trace contract; it does
not independently retrain the byte-pair frontend. Neither result certifies
semantic similarity or speech.
