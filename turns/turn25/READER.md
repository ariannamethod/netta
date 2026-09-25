# Independent turn25 reader

2026-09-22, Astra measurement hand. Incoming turn24 was audited first; its single fresh reader execution and amendment reconciliation are recorded in `audit/MEASUREMENT.md`.

`verify.py` was written from PROTOCOL.md and INTERFACE.md without reading or importing the new writer/C implementation. It imports only the standard library. The pinned P0/candidate/match tape is its input boundary; it does not independently rebuild HEAD256, source books or candidate probabilities.

## Independent calculation

- Four independent 50-digit Decimal source/cold mass trajectories: slow, fast, witness and latch.
- Carried prices precede the hazard update. OLD latch selects the rate; signed evidence is then updated on active matched observations; updated w sets at `<=-1`, clears at `>=+1`, and otherwise preserves the bit.
- Admission-crossing price is cold; admission resets odds, clock and latch. Equal-price and NEW observations retain exact cold prices. Neutral observations cannot clear a fast latch.
- Every quoted price, odds before/after, hazard, shared shadow/admission, clock and latch field is compared. Both input streams must contain exactly N chronological rows; extra or missing rows are rejected by filename.
- All horizons, prefix minima, drawdowns, active hazard counts, first-set/return diagnostics, per-world quantities, means, eight bars and separate validity flags are rebuilt. Comparisons to empirical utility bars have no extra tolerance. Numeric reproduction uses `1e-7`; discrete values are exact.
- Code/protocol/interface and retained artifact manifests are checked. Failures name the artifact; an existing requested output is refused. The reader writes only its fresh output file.

Expected full batch: **2,097,152 authority forecasts**, worlds248..255 × four regimes ×16,384 bytes ×four modes. Frontend normalization remains a retained C receipt; derived authority mass normalization is independently checked. Descriptive raw-help/harm selection is outside this reader, as stated in INTERFACE.md.

```sh
python3 -B turns/turn25/verify.py --output turns/turn25/VERIFY.json
```

## Prefreeze interface

`check_schema(result)` accepts the actual writer constructor's artificial output and names a missing required field. `check_artifact(path,wanted)` supports the single isolated temporary-file hash probe. `Authority(mode).step(t,cold,candidate,matched)` exposes all state/price fields needed for the disjoint C fixture. Short fixtures should use `step`; full-life `result()` requires the declared horizon observations.

`--help` passed. Own code review is complete. Root owns the actual schema/C fixture, freeze and one fresh batch; this hand has generated no target worlds.

- Protocol SHA256: `db4d93eb8835973330cbe6e4a356b9228d230e9251b7fbc7bb7f0bb074cdf730`.
- Interface SHA256: `3013801e74b4537ad177fcfdc817d9923101b352a0ae1aa44fd08e308a405ceb`.
- Reader ready-for-review SHA256: `845e2e09cedc1cb26457e18cdf8624884ec4e0299b008910d5319b4c5c7985aa`.
