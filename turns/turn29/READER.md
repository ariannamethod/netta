# Turn29 independent reader

`verify.py` imports only the frozen turn28 reader helpers, not the turn29
writer or new C source. It independently:

- reads raw source lives and recounts immediate role continuations;
- rebuilds turn28 bank/full/small/permuted archives and the new full permuted
  archive byte-for-byte;
- verifies both C trace schemas and exact agreement on causal P0 chronology;
- reconstructs all binary local/global/permuted routers with Decimal state;
- reconstructs the frozen three-way Astra bank routers;
- normalizes full repeat support, protects NEW, and replays five outer laws;
- rebuilds every horizon, admission, bound, comparison and gate.

Supplied, hash-pinned boundaries remain the inherited greedy grammar selection
and HEAD256 P0/bindings. The reader does not independently mine a new grammar
or rebuild the frontend's internal state from raw bytes.

Command:

```
python3 -B verify.py --output NEW_ABSENT_PATH
```

The original run checked 3,276,800 forecasts and 2,688 source counters with
maximum numeric error `6.320988177321851e-11`; material and verification PASS.
