# Joint source selection — Netta turn13

One continuation explicitly requested by Oleg after turn12. Own existing
checkout: `netta-astra-turn12-20260919`, branch `astra/turn12-information`.
Previous turn12 is sealed; this directory contains the new work.

**Completed: the same17 material conditions PASS; independent verification
PASS.** Joint selection improves full recombined gain over the paired
predecessor in8/8 at the same528bytes. Its changed tail worsens in6/8; both
results are preserved.

- [Report](REPORT.md), [all outcomes](TABLES.md), [raw byte behavior](RAW.md).
- [Result](RESULT.json), [independent check](VERIFY.json), [verdict](VERDICT.json).

The only new learned rule scores each candidate record by its marginal
source contribution under the longest-match memory already selected.
The predecessor's isolated score is retained as a paired arm. Existing
format, budget, frontend, recipient and authority are unchanged.

- [Predata protocol](PROTOCOL.md), [received state](RECEIVED.json).
- [Source learner](experiment.py), [C quote implementation](episode.c),
  [independent reader](verify.py), [reader scope](READER.md).
- [C build and preserved functions](BUILD_C.md),
  [source function inheritance](INHERITANCE.json).

In BOOKS, each selected record retains its diagnostic `score` from the old
isolated information criterion. The new marginal score is explicitly in
`joint_forward`/`joint_reverse` step records as `marginal_gain`; these values
need not have the same sign. Counts remain full occurrence counts.

## Reproduce in an empty directory

After the retained freeze exists, run this from the repository root:

```sh
python3 - <<'PY'
from pathlib import Path
import json, shutil, tempfile
src = Path.cwd()
dst = Path(tempfile.mkdtemp(prefix='netta-joint-replay-'))
for relative in json.loads((src/'turn13/FREEZE.json').read_text())['files']:
    if relative == 'turn13/episode':
        continue
    target = dst/relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(src/relative, target)
print(dst)
PY
```

Change to the printed directory. Python3 with math.exp2, make and a C11
compiler are sufficient; no external numerical library is used.

```sh
make -C turn13
python3 turn13/experiment.py freeze
python3 turn13/experiment.py generate
python3 turn13/experiment.py extract
python3 turn13/experiment.py learn
python3 turn13/experiment.py evaluate
python3 turn13/verify.py --output turn13/VERIFY.json
```

Stages refuse to replace evidence. Build hashes, timestamps and gzip headers
can differ; compare raw streams, learned archives, decompressed quotes and
verified numerical outcomes. To recount only the retained run:

```sh
python3 turn13/verify.py --output /tmp/netta-joint-reader-unused.json
```

Choose an unused output file. The reader checks all seven source archives
per world, joint selection paths, matches, prices and inherited gates. Its
HEAD256/P0 independence boundary is stated in READER.md. Full local data and
traces are in ignored data/ and results/, pinned by manifests.
