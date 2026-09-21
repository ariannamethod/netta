# Turn22 — relative-support ceiling

One isolated next step after auditing Don21. The sole new authority law uses
the existing signed witness statistic and one additional absolute-evidence
statistic to set a prospective ceiling. It changes neither the portable
archive nor its candidate predictions. Prediction state is 40 bytes; source
archives remain bounded at 528 bytes. This is an experiment over synthetic
raw-byte worlds, not an integration into the living Netta.

- [Protocol](PROTOCOL.md) and [writer/reader interface](INTERFACE.md), fixed
  before fresh data.
- [Incoming independent recount](audit/INCOMING_READER.md),
  [incoming mechanism audit](audit/MECHANISM.md), and
  [construction and bounds](audit/SELF_NORMALIZING_DESIGN.md).
- [C build and continuity](BUILD_C.md), [independent reader scope](READER.md),
  and [single pre-freeze comparison](PREFREEZE_CHECK.json).
- [Report](REPORT.md), [raw examples](RAW.md), [result](RESULT.json), and
  [independent receipt](VERIFY.json).

## Reproduce

From the repository root, use this command to check the retained local
artifacts with the independent reader. Choose a new output filename; existing
evidence is never overwritten:

```sh
python3 -B turns/turn22/verify.py --output /tmp/netta-turn22-independent.json
```

The reader needs the retained `data/`, `memory/`, `results/` and the frozen
binaries. These large/generated artifacts stay local under `.gitignore`;
their SHA-256 manifests are retained with the report. A source-only checkout
therefore needs the rebuild below or a copy of the retained artifacts.

To reproduce the same experiment from source, use a separate checkout with
the same repository-relative dependency layout and a fresh `turns/turn22/`
directory. Copy the source/doc inputs into it, omitting generated receipts,
FREEZE, RESULT, VERIFY, manifests, binaries, `.build`, data, memory and results.
Keep the original directory intact. Run each command separately and check its
exit status before proceeding:

```sh
make -C turns/turn22 all authority_fixture
turns/turn22/authority_fixture > turns/turn22/FIXTURE.json
python3 -B turns/turn22/prefreeze_check.py
python3 -B turns/turn22/experiment.py freeze
python3 -B turns/turn22/experiment.py generate
python3 -B turns/turn22/experiment.py extract
python3 -B turns/turn22/experiment.py learn
python3 -B turns/turn22/experiment.py evaluate
python3 -B turns/turn22/verify.py
```

The source/target content is deterministic from worlds 224–231 and namespace
`netta-relative-support-ceiling-v1`. A rebuild has its own freeze timestamp
and may have platform-dependent binary hashes. Gzip container timestamps may
also differ; compare decompressed trace values, archives and charged gains
rather than requiring the new manifests to equal historical hashes. The
historical frozen manifests check the original retained files exactly.

Do not use these worlds to choose another formula or tune its constants.
After the result and its audit, the next research hand belongs to Sol.
