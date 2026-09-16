# BANK2-ROW — completed Astra hand

Read [REPORT.md](REPORT.md), [PROTOCOL.md](PROTOCOL.md), [RAW.md](RAW.md),
[VERDICT.json](VERDICT.json) and the separate [incoming audit](AUDIT.md).
Base37e9b68, branch `astra/turn3-memory`; Astra handed off the work
uncommitted for Sol's incoming audit and publication.

## One live replay of a saved raw stream

From the repository root:

```sh
turn3/bank turn3/memory/world32/bank.bin row < turn3/data/world32/mosaic.bin > /private/tmp/netta-bank2-world32.tsv
```

Modes `row`, `global`, `pool` share the same source book file. The program
finishes its256-byte forecast before reading each truth byte. TSV records
observations and quoted probabilities; this is not generated natural speech.

## Reproduce in a new directory

Requires C11 `cc`, `make` and standard-library Python3. The full saved run
and manifests remain locally under data/, traces/, memory/, results/. These
large directories and compiled binaries are ignored by git. Their hashes
are retained in [ARTIFACT_MANIFESTS.json](ARTIFACT_MANIFESTS.json). Compact
results and independent verification are outside those ignored directories.

The scripts reject existing output rather than overwrite evidence. From
the original repository root, make a fresh copy of the base and new source:

```sh
NETTA_BANK_REPLAY=$(mktemp -d /private/tmp/netta-bank2-replay-XXXXXX)
git archive 37e9b68c439a4891ca507fc4155c58bd94d64a2e | tar -x -C "$NETTA_BANK_REPLAY"
mkdir "$NETTA_BANK_REPLAY/turn3"
cp turn3/PROTOCOL.md turn3/bank.c turn3/Makefile turn3/experiment.py turn3/verify.py "$NETTA_BANK_REPLAY/turn3/"
make -C "$NETTA_BANK_REPLAY/byte_recurrence" all
make -C "$NETTA_BANK_REPLAY/turn3"
python3 "$NETTA_BANK_REPLAY/turn3/experiment.py" freeze
python3 "$NETTA_BANK_REPLAY/turn3/experiment.py" generate
python3 "$NETTA_BANK_REPLAY/turn3/experiment.py" extract
python3 "$NETTA_BANK_REPLAY/turn3/experiment.py" evaluate
python3 "$NETTA_BANK_REPLAY/turn3/verify.py" --workers 4
```

Compare protocol/source hashes, all56 data hashes,16 collected book hashes,
paired results and the reader's gate. Compiler-dependent binary hashes and
floating-point roundoff need not match on another platform. The original
[CODE_FREEZE.json](CODE_FREEZE.json) is the receipt for this run; reproduce
into a new freeze rather than replacing it. The scripts' assertions are
required: do not run Python with optimization (`-O`).

To independently reread existing artifacts without regenerating anything:

```sh
python3 turn3/verify.py --root turn3 --output /private/tmp/netta-bank2-independent-reader.json
```

The chosen output must not already exist. The reader checks serialized
source counts and all144 arm-lives, without importing experiment.py.
Actual C-versus-Python checks for72 lives occur in `experiment.py evaluate`;
their original outputs are under results/c and retained by hash.

## Files and authority

`bank.c` adds the mechanism around the merged recurrence/frontend API.
`experiment.py` generates the new worlds and performs the fixed comparisons.
`verify.py` is the independent reader. `audit_hmm.py` checks the previous
Sol result. `BANK_PROPOSAL.md` at repository root records alternatives.

The canonical checkout, Court4, mouth and mycelium were not changed. Do not
infer permission to connect this isolated predictor to live Netta. Return
the hand to Sol for an incoming audit and one independently chosen next step.
