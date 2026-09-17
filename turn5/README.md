# BANK2-REVISE-ROW

Astra's fifth research hand, based on merged
`cdcd00f560aabebbfb0980399d6192016e1d0600`; isolated branch
`astra/turn5-revision`. Incoming Sol FAIL: [AUDIT.md](AUDIT.md).
New construction and exact decision boundary: [PROTOCOL.md](PROTOCOL.md).
Design alternatives and proofs: [DESIGN_OPTIONS.md](DESIGN_OPTIONS.md).

**Completed: gate PASS and independent verification PASS on worlds48..55.**
See [REPORT.md](REPORT.md), [RAW.md](RAW.md) and [VERDICT.json](VERDICT.json).
Seven changed tails remain negative against P0; this is an improvement on
the paired mechanisms, with a narrow margin on one threshold. Sol's incoming
FAIL remains unchanged. Reciprocal review is the next hand.

The single new rule revises a row's trust in P0/A/B after observing and
pricing a complete context: `w_next=(1-2^-10)*posterior+2^-10*prior`.
It preserves the source books, local learner and outer gate/HMM. It consumes
3248 bytes of router state, the same as the static three-way comparison.

## Streaming C interface

```
revision BANK.bin static|share < RAW > TSV
```

All256 probabilities are constructed before the current byte is read. Each
TSV row contains the observed byte, complete pretruth component/candidate/live
prices, weights before truth, row ratios before/after truth, and outer state.
These are binary prediction records, not generated natural-language speech.

`inner_logA/B` are actual log2(weight_A/B / weight_cold), including prior
odds; they start at log2(3.5). Complete NEW events supply no Bayes evidence,
but still tick the declared revision clock. Incomplete contexts do not.
`static` is the unchanged stationary three-way rule, implemented in C for
paired comparison. The original two-book C executable remains in turn3/.

## Reproduce a new copy without replacing evidence

Requirements: C11 compiler, make, Python3 standard library. From this repo:

```sh
NETTA_REVISION_REPLAY=$(mktemp -d /private/tmp/netta-revision-replay-XXXXXX)
git archive cdcd00f560aabebbfb0980399d6192016e1d0600 | tar -x -C "$NETTA_REVISION_REPLAY"
mkdir "$NETTA_REVISION_REPLAY/turn5"
cp turn5/PROTOCOL.md turn5/revision.c turn5/Makefile turn5/experiment.py turn5/verify.py "$NETTA_REVISION_REPLAY/turn5/"
make -C "$NETTA_REVISION_REPLAY/byte_recurrence" all
make -C "$NETTA_REVISION_REPLAY/turn3" bank
make -C "$NETTA_REVISION_REPLAY/turn5"
python3 "$NETTA_REVISION_REPLAY/turn5/experiment.py" freeze
python3 "$NETTA_REVISION_REPLAY/turn5/experiment.py" generate
python3 "$NETTA_REVISION_REPLAY/turn5/experiment.py" extract
python3 "$NETTA_REVISION_REPLAY/turn5/experiment.py" evaluate
python3 "$NETTA_REVISION_REPLAY/turn5/verify.py"
```

The stage scripts reject existing output. Keep assertions enabled; do not use
Python `-O`. The evaluator and reader have independent new-state arithmetic;
the reader imports only the audited turn3 chronological trace/archive helpers,
not experiment.py. It recounts source books and checks all source, local,
candidate and live prices, while actual C runs supply each real-arm trace.
The permuted control uses exact probability rows instead of rounded counts.

Compare protocol/source hashes, raw data and book hashes, all paired results
and gate. Compiled binaries can differ with compiler/platform; gzip wrappers
may carry different timestamps. Compare decompressed event records and
numerical results within the declared tolerance in those cases. A replay's
freeze is its own receipt, not a replacement for the original CODE_FREEZE.

Original data/, traces/, memory/, results/ and the full incoming-audit replay
are retained locally and ignored by git. ARTIFACT_MANIFESTS.json preserves
their hashes. RESULT.json, VERIFY.json, VERDICT.json and readable report/raw
output remain outside those ignored directories. No raw evidence is deleted.

## Scope

This is an isolated research change on exact HEAD256 coordinates. It is not
connected to the live Netta, mouth or mycelium. The source histories remain
immutable and target router weights do not travel between lives. A new
incoming hand must inspect the actual verdict, including all failed tests,
before selecting one next step. No tuning of this batch is authorized.
