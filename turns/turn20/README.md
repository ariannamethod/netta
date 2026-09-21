# Turn20 local reproduction and receipts

Worktree: `/Users/ataeff/arianna-codex/repos/netta-sol-turn20-20260921`.
Branch: `sol/turn20-witnessed-ceiling-20260921`, based on `776aaf4`.
Nothing here is integrated into live Netta. Raw `data/`, `memory/`, and
`results/` are retained locally and intentionally ignored by Git.

The completed sequence was strict `make all fixture`, then the five stages
`freeze`, `generate`, `extract`, `learn`, `evaluate` of `experiment.py`,
each once on worlds208..215. No new policy run follows this result. The
three manifests pin every raw stage; `FREEZE.json` pins inputs and binaries.

To check the retained data without regenerating it:

```sh
cd /Users/ataeff/arianna-codex/repos/netta-sol-turn20-20260921
shasum -a 256 turns/turn20/{FREEZE.json,RESULT.json,VERIFY.json}
python3 -B turns/turn20/verify_repair.py --output /private/tmp/netta-turn20-check-unique.json
```

Use a fresh output pathname on each independent check. `verify_repair.py`
is the disclosed reader-only correction to a stale turn19 receipt-field
lookup in the frozen `verify.py`; see `REPORT.md`. The original frozen
reader exits with `KeyError: 'fast_share_bound'` before reconstruction.
The repaired reader hashes and checks that frozen file without editing it.
It uses retained HEAD256/P0 trace inputs, independently rebuilds source
books, candidates, matched lengths and authority prices, then recomputes
all material gate booleans. The result is independent-verification PASS,
material-gate FAIL 18/20.

`LIFE_TABLE.tsv` contains all 192 world/regime/mode measurements.
`BYTE_EXAMPLES.tsv` contains 64 fully charged per-life extrema; their
complete dictionaries and every raw trace remain in `RESULT.json` and
`results/`. `render_tables.py` only formats frozen results; it does not
participate in evaluation or verification.
