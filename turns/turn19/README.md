# Turn19: commitment ceiling

One fixed five-bit ceiling, applied after the inherited witness update. The
incoming archive, candidate and first admission are unchanged. Persistent
authority state is 32 bytes per mode; the portable source archive is 528 bytes.

Start with [PROTOCOL.md](PROTOCOL.md), the incoming
[audit](audit/INCOMING_READER.md), and the
[mathematical review](audit/CAP_DESIGN.md). Implementation and the preserved
34-column trace schema are in [BUILD_C.md](BUILD_C.md). The independent
reader's scope is in [READER.md](READER.md).

## Read the retained run

The material result and its independent reconstruction are `RESULT.json` and
`VERIFY.json`. `FREEZE.json` pins code, protocol and binaries before generation.
The three manifests pin data, source books and raw forecasts. `logs/` records
each actual stage's exit code and timestamps. Raw directories and binaries
are retained locally and ignored by Git; summaries and manifests are durable.

From this repository's root, one new reader receipt can be obtained without
regenerating any target:

```sh
python3 -B turns/turn19/verify.py --output /tmp/netta-turn19-reader-new.json
```

The output path must not already exist. The original receipt is never replaced.
This reader uses saved HEAD256/P0 inputs, independently reconstructs seven
candidate arms and replays all five authority modes in probability space.
It does not independently reconstruct all 256 probabilities of local P0.

## Reproduce in a new directory

Run this Python block from this repository's root. It copies only the frozen
source dependencies into a fresh temporary directory, builds there, generates
the fixed batch and checks it once. It does not change retained evidence.

```python
from pathlib import Path
import json, shutil, subprocess, tempfile

src = Path.cwd()
dst = Path(tempfile.mkdtemp(prefix="netta-turn19-reproduce-"))
freeze = json.loads((src / "turns/turn19/FREEZE.json").read_text())
for name in freeze["files"]:
    if name in ("turns/turn19/episode", "turns/turn19/authority_replay"):
        continue
    target = dst / name
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src / name, target)
print(dst, flush=True)
subprocess.run(["make", "-C", str(dst / "turns/turn19"), "all"], check=True)
for stage in ("freeze", "generate", "extract", "learn", "evaluate"):
    subprocess.run(["python3", "-B", str(dst / "turns/turn19/experiment.py"), stage], check=True)
subprocess.run(["python3", "-B", str(dst / "turns/turn19/verify.py")], check=True)
```

Compare material booleans and numerical tables, plus uncompressed forecast
values. A new freeze has a new timestamp; gzip headers and compiler-dependent
binary hashes can differ, so a fresh `RESULT.json` is not promised to have
the original file hash. The new reader pins that reproduction's actual build.
The original build used Apple clang 21, arm64, with strict C11 warnings.

The small disjoint implementation fixture is `FIXTURE.json`; its independent
mass-space reconstruction is `FIXTURE_READER.json`. It precedes fresh data and
is not part of the material result. No additional target set or ceiling sweep
is part of this turn.
