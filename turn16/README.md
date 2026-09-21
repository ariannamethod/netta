# One earned return

Turn16, Astra. Incoming review: [AUDIT.md](AUDIT.md). Exact construction,
fixed data and gate: [PROTOCOL.md](PROTOCOL.md). Measured outcome will be
in [REPORT.md](REPORT.md), with byte examples in [RAW.md](RAW.md).

The new portable-memory authority is [authority.c](authority.c), with the
quote/observe API in [authority.h](authority.h). It reserves half a bit
for each of two possible admissions and permits a single renewed admission
after fresh suffix evidence. Its persistent state is **40 bytes** per
mode; the deployed choice would use one state per candidate. The experiment
uses 7 candidate arms x 4 paired modes = 1120 bytes of authority state,
plus the unchanged source archives, local learner and measurement storage.
`budget` has the same initial prior as `return` but cannot return.

The module is exercised through its price interface. `replay.c` receives
only t, P0 and the seven candidate prices, quotes all modes before any
observation update, and writes the charged prices and state transitions.
The raw-byte candidate predictor is inherited byte-for-byte from turn13.
See [BUILD_C.md](BUILD_C.md) and [READER.md](READER.md) for the exact boundary.

## Reproduce in a fresh directory

Do not run generation over retained evidence. This copies the frozen code
into a new temporary tree; no original code/data is changed. On this host:

```sh
NETTA16_REPRO=$(mktemp -d /Users/ataeff/arianna-codex/repos/netta16-reproduction-XXXXXX)
python3 - "$NETTA16_REPRO" <<'PY'
import json, shutil, sys
from pathlib import Path
src = Path('/Users/ataeff/arianna-codex/repos/netta-astra-turn16-20260921')
dst = Path(sys.argv[1])
pins = json.loads((src/'turn16/FREEZE.json').read_text())['files']
for name in pins:
    if name in ('turn16/episode', 'turn16/authority_replay'):
        continue
    out = dst/name
    out.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src/name, out)
print(dst)
PY
cd "$NETTA16_REPRO"
make -C turn16 all
PYTHONDONTWRITEBYTECODE=1 python3 turn16/experiment.py freeze
PYTHONDONTWRITEBYTECODE=1 python3 turn16/experiment.py generate
PYTHONDONTWRITEBYTECODE=1 python3 turn16/experiment.py extract
PYTHONDONTWRITEBYTECODE=1 python3 turn16/experiment.py learn
PYTHONDONTWRITEBYTECODE=1 python3 turn16/experiment.py evaluate
PYTHONDONTWRITEBYTECODE=1 python3 turn16/verify.py
```

Check each command's exit code before continuing. Every stage creates new
outputs; the frozen source list is checked again before later stages.
Compare the generated raw bytes, seven archives, decompressed candidate and
authority traces, and numerical result. Gzip headers and the new freeze
timestamp need not match the retained original bytes.

To recount the saved run, use an unused output path:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 turn16/verify.py --output /tmp/netta16-independent-recount.json
```

Full raw streams and traces stay in local ignored `data/` and `results/`;
their manifests are retained. The source books and evidence summaries
are alongside the code. This turn is local until Oleg requests publication.
