# Turn 26: the C mouth keeps its pair frontier

Frozen 2026-09-25 before the implementation or any turn-26 timing run.

## Question

Can the C mouth stop recounting the entire lived stream at every BPE merge,
while preserving the exact unit law, speech, traces, and independent-court
verdict?

This is Oleg's separately routed C/Python performance correction.  It is not a
memory-controller experiment and spends none of worlds 248--255.

## Received state and audit

The parent is Astra's published turn25 commit
`71389502e514a39bd1a643fb2e5992de9677253c`, itself based on Don's turn24
commit `798085be9a2bf8b6c41860238d4f38450558e126`.

Sol independently ran `turns/turn25/verify.py` in a fresh output directory.
It recomputed 2,097,152 forecasts, returned verification PASS and material
FAIL, and produced a `VERIFY.json` byte-identical to the preserved file:

`a3367a839f2e5e3c78e1adc1f84e85c1789a6b4d4e7cfa16008632ffc7c83fa3`.

The remote branch `astra/turn25-witness-latch` and the clean received checkout
both resolved to the same published commit.  The material result remains
FAIL 7/8; this turn neither repairs nor reinterprets it.

## Existing receipt and located cost

`PARITY.md`, frozen before this turn, measured the same default island and five
sittings.  The Python port completes speech and its in-process court in
4.63--5.30 seconds in the five interleaved rows.  The independent C path costs
8.40--11.06 seconds for mouth plus reader.  That total is not a language
comparison: the C path deliberately has two independent hands, while the
single Python file shares its tables.

There is nevertheless one like-for-like implementation difference inside the
mouth.  The frozen C `merge_round` recounts and rewrites the complete token
stream once per merge.  The parity-proven Python port maintains the same
adjacent-pair counts over a linked stream and a lazy priority queue.  Both
choose maximum count then minimum packed pair key.  The Python phase receipt
names 1.89 seconds for all 4096 merges.

Pinned inputs:

- `netta_mouth.c`:
  `9cc831657e6bdb3984b55d7f17dcac7040ad877c540d3d579393c642106d6dd6`
- `netta_mouth_check.c`:
  `fc477bfcf4961e658fdae86741dd8c7e0b150670388a71c7fc710284024b7097`
- `netta.py`:
  `f3ef71d3164d895653c4bc9f521ab89557870d1d039b98574b218f0e95fb0278`
- `netta.txt`:
  `02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb`

## One bounded construction

Replace only the C mouth's repeated full-stream BPE recount with an incremental
pair frontier:

1. a linked representation of the still-lived token positions;
2. exact live occurrence counts for adjacent pair keys;
3. occurrence lists used only as candidate positions, with dead/stale entries
   rejected against the linked stream;
4. a lazy max-priority queue ordered by count descending and packed key
   ascending;
5. the same left-to-right, non-overlapping replacement rule.

The queue may contain stale entries.  A popped entry is authoritative only
when its stored count equals the current count for that key.  No sampling,
court, n-gram, corridor, citizen, CLI, default, or report code may change.
`netta_mouth_check.c` remains byte-for-byte frozen as the foreign hand.

## Fixed comparison

Both C binaries are built with:

```
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror ... -lm
```

The old binary is compiled from
`71389502e514a39bd1a643fb2e5992de9677253c:netta_mouth.c`; the candidate is
compiled from the turn26 working file.  Runs are interleaved old/new with
alternating order.  Timing is monotonic wall clock around process execution,
seven measured pairs after one untimed warm-up per binary.  Output directories
are new for every process.

The primary workload is:

```
netta_mouth netta.txt --out OUT --corridor 1
```

The smaller-island guard uses the first 89,509 bytes and the same flags.  A
second law guard uses the full island with `--merges 1024 --order 3`.

## Gates

All conditions were fixed before implementation:

1. Both sources compile strictly with zero diagnostics.
2. On the full island, smaller island, and second law guard, every emitted
   `speech_*.bin` and `trace_*.tsv` file from candidate C is byte-identical to
   old C.
3. The frozen independent reader accepts candidate output on the primary
   workload and reproduces the committed Sitting-1 report byte for byte.
4. The candidate is deterministic across two fresh primary runs.
5. Median primary mouth wall clock is at least 20% below old C.  This is the
   sole material performance threshold.
6. Median smaller-island wall clock is no more than 10% above old C.
7. Peak resident memory and artifact sizes are reported, not gated.  The
   mechanism is allowed to spend memory to avoid repeated work.

Any semantic mismatch is a hard FAIL regardless of speed.  A speedup below the
20% primary threshold is a material FAIL and remains recorded.  No parameter,
gate, compiler flag, corpus, or workload may change after results are seen.

## Boundaries

Canonical Netta, Astra's checkout, Don's checkout, `netta.c`,
`netta_mouth_check.c`, the mouth protocol, mycelium, and all previous turns are
read-only.  Work remains isolated on `sol/turn26-incremental-bpe`.  No commit,
push, merge, or live integration occurs without Oleg's explicit word.
