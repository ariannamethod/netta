# Turn30 independent reader

`verify.py` imports only the frozen turn28 reader helpers, not the turn30
writer and not the new C source. It independently:

- reads the raw source lives and recounts immediate role continuations;
- rebuilds turn28's bank/pooled_full/pooled_small/permuted archives and this
  turn's `permuted_small.bin` and `permuted_full.bin` byte-for-byte from those
  recounted source tapes, and checks that each null archive costs exactly what
  the archive it mirrors costs;
- verifies both C trace schemas and their exact agreement on the causal P0
  chronology — one price tape for every arm;
- reconstructs all twelve three-way router trajectories with Decimal state,
  renormalizing every posterior, and all binary pooled/null/full24 routers;
- recomputes the life's shared admission from the incumbent `pooled2`
  candidate stream and requires every gated arm to carry the same
  `active_before`, `activated_after` and activation step;
- normalizes the complete changed support of both the three-way and the binary
  mixtures, protects NEW, and replays five outer laws;
- rebuilds every horizon, admission, bound, comparison, quantity and gate
  tooth, and refuses any RESULT.json that certifies D6 or `gate_pass` itself.

Supplied, hash-pinned boundaries remain the inherited greedy grammar selection
and HEAD256 P0/bindings. The reader does not independently mine a new grammar
or rebuild the frontend's internal state from raw bytes. Of the turn29
`full24` trace it recomputes the `local_full` arm, which is the only arm this
turn exposes; the other three columns of that trace stay audit evidence.

Command:

```
python3 -B verify.py --output NEW_ABSENT_PATH
```

The original run checked 3,276,800 forecasts and 2,688 source counters with
maximum numeric error `3.392131020518718e-11`, finished in 141 seconds with
exit 0, and reported `verification_pass` true, `material_pass` false and
`gate_pass` false: D1 and D3 fail on the numbers, D2, D4, D5 and D6 hold.

An integrity probe on a copy of the tree changed one digit inside
`results/world280/recombined.turn30.tsv.gz` (manifest pins `2171a4df…`, the
altered file digests `9b26d714…`). The reader refused by name —
`AssertionError: manifest …/results/world280/recombined.turn30.tsv.gz` — with
direct rc=1 and no receipt written. Restoring that one file and rerunning the
same copy completes with exit 0 and a receipt identical to the sealed one,
which is what makes the refusal attributable to the digit.
