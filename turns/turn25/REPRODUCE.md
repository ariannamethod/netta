# Reproduce turn25

Requirements: C11 compiler, make, Python 3.11+ standard library. No network or
third-party packages. The recorded run used Python 3.14.4 and the platform C
compiler. `RUN.json`, `PREFLIGHT.json`, `FIXTURE.json` and `VERIFY_RUN.json`
record execution and checks. Code and binaries were frozen before generation.

## Retained-artifact reader

Run from the repository root, with the ignored data, memory and results still
present in the original working copy:

```sh
python3 -B turns/turn25/verify.py --output turns/turn25/audit/SECOND_READER.json
```

The output path must not already exist. This reads the frozen artifacts and
recomputes four authority trajectories, every quote/state, and the material
gate. It does not rebuild the inherited frontend or source books.

## Full deterministic regeneration into a new directory

This copies only pinned source files into a new temporary root. The original
evidence remains in place. Source worlds, archives and prediction inputs are
regenerated using the same fixed seeds; this is reproduction, not another
candidate or a new selection batch. Run from the repository root:

```python
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

source = Path.cwd()
turn = source / "turns/turn25"
pins = json.loads((turn / "FREEZE.json").read_text())["files"]
destination = Path(tempfile.mkdtemp(prefix="netta-turn25-reproduce-"))
for name in pins:
    p = Path(name)
    if p.suffix not in (".c", ".h", ".py", ".md") and p.name != "Makefile":
        continue
    target = destination / p
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / p, target)
work = destination / "turns/turn25"
subprocess.run(["make", "all"], cwd=work, check=True)
subprocess.run([sys.executable, "-B", "preflight.py"], cwd=work, check=True)
for stage in ("freeze", "generate", "extract", "learn", "evaluate"):
    subprocess.run([sys.executable, "-B", "experiment.py", stage], cwd=work, check=True)
subprocess.run([sys.executable, "-B", "verify.py"], cwd=work, check=True)
print(destination)
```

Compare RESULT's material bars, validity, per-life metrics and quantities.
Raw `.bin` worlds and archives are deterministic. Gzip headers, timestamps,
absolute receipt paths and compiler-dependent binary hashes can differ in a
new run. The regenerated run makes its own freeze and retains its receipts;
do not replace this turn's original manifests with them.

The incoming Don24 rerun is a separate audit, documented under `audit/`.
`diagnose.py` (if present) only analyzes saved turn25 traces after the result;
it neither changes the controller nor generates data.
