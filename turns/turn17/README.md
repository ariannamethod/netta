# Turn17 local reproduction

Start with `INCOMING_AUDIT.md`, then `PROTOCOL.md`, `REPORT.md`, `RAW.md`,
`TABLES.md`, `RESULT.json`, `VERIFY.json` and `VERDICT.json`. The result is
a material FAIL with an independent arithmetic PASS. The raw streams and
price traces reside locally in ignored `data/`, `memory/` and `results/`;
their complete SHA-256 inventories are the three `*_MANIFEST.json` files.

To recount the retained run without changing it, choose a new output path:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 turn17/verify.py \
  --output /private/tmp/netta17-independent-recount.json
```

For a fresh reproduction, use a *separate* checkout containing the frozen
turn13 source files and a copy of `turn17/PROTOCOL.md`, `experiment.py`,
`verify.py`, `authority.c`, `authority.h`, `replay.c` and `Makefile`. On the
same compiler/toolchain, build and run, checking each exit status:

```sh
make -C turn17 all
PYTHONDONTWRITEBYTECODE=1 python3 turn17/experiment.py freeze
PYTHONDONTWRITEBYTECODE=1 python3 turn17/experiment.py generate
PYTHONDONTWRITEBYTECODE=1 python3 turn17/experiment.py extract
PYTHONDONTWRITEBYTECODE=1 python3 turn17/experiment.py learn
PYTHONDONTWRITEBYTECODE=1 python3 turn17/experiment.py evaluate
PYTHONDONTWRITEBYTECODE=1 python3 turn17/verify.py
```

Do **not** run `generate` or `evaluate` over this retained run. Their stages
refuse existing outputs. Gzip headers and freeze timestamps can differ in
a fresh directory; compare decompressed traces, raw bytes, archive content,
the measured verdict and the independent reader's gate fields.
