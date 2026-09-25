# Turn26 — the C mouth keeps its pair frontier

Sol, 2026-09-25.  Isolated branch `sol/turn26-incremental-bpe`, based on
Astra25 commit `71389502e514a39bd1a643fb2e5992de9677253c`.

## Result

**PASS.**  The C mouth now preserves adjacent-pair evidence across BPE rounds
instead of recounting the complete stream 4096 times.  On the 447,545-byte
island its median wall time fell from **2.851755 s to 0.177640 s**: **16.05x**
faster, a **93.77%** reduction, against a preregistered 20% gate.

The speed is not paid for with a different voice.  Across all three frozen
semantic guards, every speech file and every token trace is byte-identical to
the parent C.  The unchanged independent C reader accepts the candidate and
reprints the committed Sitting-1 report byte for byte, ending with the same
literal verdict:

`SPEECH PASS: the mouth speaks below ignorance and above copying`

## Incoming audit

Astra25 was accepted before this step.  Local and remote publication identities
match at `7138950`; its clean checkout's independent reader was run into a new
path.  All **2,097,152 forecasts** were reconstructed and the new
`VERIFY.json` is byte-identical to Astra's published receipt.  Verification
PASS and material FAIL 7/8 both remain.  Full receipt:
[INCOMING_AUDIT.md](INCOMING_AUDIT.md).

## What changed

The old C `merge_round` rescanned every live token to recount pairs, chose the
most frequent pair, then rewrote the stream.  The Python parity port had already
shown a different route through the same law: retain pair counts over a linked
stream and keep the current maximum on a lazy heap.

Turn26 implements that route in `netta_mouth.c`:

- token positions remain stable and live positions form a doubly linked stream;
- exact adjacent-pair counts live in an open-addressed table;
- occurrence vectors name candidate left positions and discard stale entries
  when their pair reaches the frontier;
- a lazy heap orders count descending and packed pair key ascending;
- replacement still walks valid positions left-to-right and is
  non-overlapping.

No sampler, n-gram table, corridor, citizen, CLI, default, report, RNG, reader,
or protocol line changed.  `netta_mouth_check.c` remains the foreign hand.

## Frozen gates and measurements

Seven old/new pairs followed one warm-up per binary.  Pair order alternated.
Both binaries used Apple clang 21.0.0 with identical strict C11/O2 flags and
wrote into fresh directories.

| Workload | Old C median | New C median | Change | Old peak RSS | New peak RSS |
| --- | ---: | ---: | ---: | ---: | ---: |
| 447,545 B, 4096 merges, K=1 | 2.851755 s | 0.177640 s | 16.05x / −93.77% | 12,861,440 B | 55,427,072 B |
| 89,509 B, stop at merge floor, K=1 | 0.227614 s | 0.033155 s | 6.87x / −85.43% | 9,060,352 B | 16,351,232 B |

The larger island spends **40.59 MiB** more peak resident memory.  This is the
explicit price of retaining positions, counts and stale heap evidence instead
of recomputing them.  Artifact bytes do not change: the primary ten speech and
trace files total 33,884 bytes in both arms.

All preregistered gates:

| Gate | Result |
| --- | --- |
| strict old/new/reader builds | PASS |
| artifacts identical on full island | PASS |
| artifacts identical on 89,509-byte island | PASS |
| artifacts identical at 1024 merges / order 3 | PASS |
| candidate restart identity | PASS |
| independent reader and committed report identity | PASS |
| primary speed reduction >=20% | PASS (93.77%) |
| smaller island no more than 10% slower | PASS (85.43% faster) |

The reader report SHA-256 is
`3d06d1a13779553aa11e1cbba785ba5c254a4db4ba54219db2ccde3a49861079`,
the committed Sitting-1 identity.

## Additional implementation checks

An ASan/UBSan build completed the full primary workload without a finding.
A supplementary deterministic suite compared old/new return codes and, on
successful mouths, every artifact in **48 configurations over 16 worlds**:
prefixes of the lived island, repetitive sentence worlds and seeded generated
text; 32/128/512 merge budgets, pair floors 2/3, orders 3/4.  All matched.
This broadens implementation coverage but does not replace the frozen gate.

The first full harness invocation completed its computations but failed before
writing a result because macOS `os.uname()` lacks Python's `_asdict` helper.
Only platform serialization in `run.py` changed.  Candidate C, protocol,
inputs, gates and compiler command remained frozen; the complete experiment
was then rerun from the beginning.  `RESULT.json` contains only that successful
second run.

## What the result means — and does not

The earlier receipt correctly warned that the C-total versus one-file Python
number was not a language contest: C used an independent second process while
Python's internal court shared tables.  Turn26 does not erase that distinction.
It removes one real algorithmic handicap inside the C mouth under a direct
old-C/new-C comparison with identical work and output.

The historical Python phase timing was not rerun as a new gate.  Thus the
measured statement here is exactly: **this C mouth is 16.05x faster than its
parent C mouth on the frozen primary workload**.  It is not a general C/Python
benchmark and says nothing about Body 0, which was not edited.

This step also does not enlarge transferable experience.  It makes the next
scientific turns cheap enough that performance is no longer a reason to keep
polishing the same small controller question.

## Next hand

Next recipient: **Don**, under Sol -> Don -> Astra -> Sol.  Audit the parent
identity, candidate hash, frozen reader and retained `RESULT.json`, then take
one own bounded step.

The substantive path returns to memory composition on fresh worlds: preserve
distinguishable fragments together with the conditions under which each was
useful, and test whether a new life can use one matching fragment without an
unrelated or harmful fragment weakening it.  Whole-bank, shuffled-condition,
flat-memory and no-memory controls should remain visible; turn25's worlds are
not a selection surface for a new rule.

Canonical Netta, `netta.c`, mycelium, Astra's checkout and all prior turn
artifacts remain untouched.  Oleg subsequently authorized commit and push of
this isolated turn.  Publication does not authorize live integration; the
exact published identity is recorded in the shared handoff.

## Evidence

- [protocol](PROTOCOL.md)
- [freeze](FREEZE.json)
- [machine result and raw timing rows](RESULT.json)
- [reproduction](REPRODUCE.md)
- [supplementary stress check](stress.py)
