# PARITY — `pyport/netta.py` against the compiled C organism

Verdict: **BIT PARITY.** On the same island with the same seed and the same
dials, `netta.py` emits speech byte-identical to `netta_mouth.c`, token traces
byte-identical to it, and a court report byte-identical to `netta_mouth_check.c`.
Measured 2026-09-21 on an Apple A18 Pro (macOS 26.4.1, Apple clang 21.0.0,
CPython 3.14.4). Everything below is a command that was run, with its rc.

## Identities

| thing | sha256 |
|---|---|
| `netta.txt` (island) | `02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb` |
| `pyport/netta.py` (573 lines) | `f3ef71d3164d895653c4bc9f521ab89557870d1d039b98574b218f0e95fb0278` |
| `netta.c` | `439e2ac3fbae25dcac2fa97f892a0360a71d313685841c62efd1a4cd7e5f3fc0` |
| `netta_mouth.c` | `9cc831657e6bdb3984b55d7f17dcac7040ad877c540d3d579393c642106d6dd6` |
| `netta_mouth_check.c` | `fc477bfcf4961e658fdae86741dd8c7e0b150670388a71c7fc710284024b7097` |
| island 2 (`head -c 89509 netta.txt`) | `cea9def76874bea382e21940fbeb1cd300ecb59ed5c260787aa6a9e4513e9db1` |

## Build and frozen invariant

```
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta            netta.c            -lm   # rc=0
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta_mouth      netta_mouth.c      -lm   # rc=0
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta_mouth_check netta_mouth_check.c -lm  # rc=0

./netta_mouth       netta.txt --out c_k1 --corridor 1                                # rc=0
./netta_mouth_check netta.txt --dir c_k1 --report c_k1.txt --corridor 1              # rc=0
diff c_k1.txt speech_court/sitting1/report_plain.txt                                 # rc=0
```

The invariant was frozen before a line of Python existed: the C at corridor K=1
reproduces the committed Sitting-1 record byte for byte, sha256
`3d06d1a13779553aa11e1cbba785ba5c254a4db4ba54219db2ccde3a49861079`. Parity is
measured against that, not against a checklist written afterwards.

## Bit parity, `netta.txt`, corridor K=1, default dials

```
python3 pyport/netta.py netta.txt --out py_k1 --report py_k1.txt --time          # rc=0
for s in 7 19 42 101 271; do cmp py_k1/speech_$s.bin c_k1/speech_$s.bin; done    # rc=0 x5
for s in 7 19 42 101 271; do cmp py_k1/trace_$s.tsv  c_k1/trace_$s.tsv;  done    # rc=0 x5
cmp py_k1.txt c_k1.txt                                                           # rc=0
```

Python stderr and C stderr agree on the organism itself:
`world 447545 B | lived stream 92853 units | merges 4096 | V 4352 (avg 4.82 B/unit) | order 4`.

| seed | bytes | sha256 of speech (C and Python) |
|---|---|---|
| 7 | 956 | `07ec0c0a9f224adabf428e4845398bfde1c45f9b3537ffe3f815f499f60e0259` |
| 19 | 956 | `8218e7318f5186dc084fdc9bd03cae0dd1820f0f9c1078d534b6a4f04df0932c` |
| 42 | 956 | `98728747db5395140c525f66cecf7a0c4b7deb9335360b06a5fcc25466ddfc00` |
| 101 | 956 | `ce7ce2ad6c15aebe7bb5ae926d2e058077cfb4b3d4bca710e63f2e325af5637e` |
| 271 | 961 | `58c2cf93fd3a73be270a49cd3878e392c56f22d89fd94f9bb1c0d0e94fbe9542` |

Python's own court report, the C reader's report, and the committed Sitting-1
record are one file: sha256 `3d06d1a1…`, three ways.

## The C's own second hand, pointed at the Python mouth

`netta_mouth_check.c` shares no implementation with the mouth. It rebuilds the
BPE inventory with a separately written pair counter, reconstructs every spoken
byte from the trace, rescans the lived stream at every choice, and replays the
corridor law and the A3 veto by its own hand.

```
./netta_mouth_check netta.txt --dir py_k1 --report creader_on_py.txt --corridor 1   # rc=0
cmp creader_on_py.txt speech_court/sitting1/report_plain.txt                        # rc=0
```

    SPEECH PASS: the mouth speaks below ignorance and above copying

The C accepts the Python's speech and trace as lawful, and prints the frozen
verdict line. This is the load-bearing receipt: the second hand is genuinely
foreign code, not a second pass of the same file.

## Not tuned to one setting

Each row: C and Python run with identical flags, all 5 seeds, speech **and**
trace compared byte for byte.

| dials | speech+trace identical | note |
|---|---|---|
| `--corridor 3` (the C's compiled default) | 5/5 | both report SPEECH FAIL, reports `cmp` rc=0 |
| `--corridor 0` (corridor law off, A1 behaviour) | 5/5 | both FAIL |
| `--corridor 7` | 5/5 | both FAIL |
| `--corridor 1 --order 3` | 5/5 | |
| `--corridor 1 --merges 1024` | 5/5 | |
| `--corridor 1 --merges 8000 --min-pair 2` | 5/5 | |
| `--corridor 1 --temp 1.7 --topk 40` | 5/5 | |
| `--corridor 1 --temp 0.12 --topk 3` | 5/5 | |
| `--corridor 1 --bytes 4000` | 5/5 | |

Wider seed set, `--seeds 1,2,3,5,8,13,21,34,55,89,144,233,377,610,987,1597,2584,4181,6765,10946`:
**20/20** streams byte-identical, speech and trace; C reader on the Python
output rc=0.

FAIL-path parity is checked too, not only the happy path: at K=3 the Python
report and the C reader report are `cmp` rc=0, both ending
`SPEECH FAIL: one or more streams reached frozen anti-copy coverage 0.50`.

## Second island

`head -c 89509 netta.txt > island2.txt` — the first 20% of the same world, so
the unit inventory, the merge count and the stop condition all differ.

```
./netta_mouth       island2.txt --out c_isl2 --corridor 1                        # rc=0
python3 pyport/netta.py island2.txt --out py_isl2 --report py_isl2.txt --time    # rc=0
./netta_mouth_check island2.txt --dir py_isl2 --report c_isl2.txt --corridor 1   # rc=0
cmp py_isl2.txt c_isl2.txt                                                       # rc=0
```

Both organisms report `world 89509 B | lived stream 21309 units | merges 2089 |
V 2345 (avg 4.20 B/unit)`. The budget was 4096 and both stopped at 2089: the
`MIN_PAIR` floor, hit at the same round in both implementations. 5/5 speech and
traces identical; report sha256 `eeffeebdf2285bda2b88eb468e5677306e905537dc215dccecf8ae4a99441f12`,
twice. `SPEECH PASS`.

## Restart identity

```
python3 pyport/netta.py netta.txt --out restart_a
python3 pyport/netta.py netta.txt --out restart_b
diff -r restart_a restart_b                                                      # rc=0
```

## Red probes — the gates fail when they should

A gate that never refuses is decoration.

| probe | result |
|---|---|
| swap one emitted token for one outside its lawful support | court refuses: *emitted token is outside lawful lived support* |
| feed a 956-byte verbatim chunk of the world as "speech" | longest 956, coverage 1.000000, void = True |
| the same census on real speech (seed 7) | longest 37, coverage 0.088912, void = False |
| corridor law at K=1 vs K=0 on seed 7 | 76 A3 vetoes vs 0 |

And the parity comparison's own teeth — mutate `netta.py`, re-run, count seeds
still matching the C:

| mutation | seeds still identical |
|---|---|
| xorshift `<< 13` → `<< 12` | **0/5** |
| drop the Amendment-3 veto (`admitted = list(alt)`) | **0/5** |
| `REP_WINDOW` 12 → 11 | 3/5 |
| `REP_PENALTY` 0.5 → 0.4 | 4/5 |
| `cum > r` → `cum >= r` | 5/5 — not discriminated |
| multiply each softmax weight by 1+1ulp | 5/5 — not discriminated |

Named limit of the probe set: these seeds do not discriminate at the sampling
boundary or at the last ulp of the softmax weights. Bit parity here is measured
over 25 streams on two islands across nine dial settings; it is not a proof that
no float ordering anywhere could ever diverge. The RNG and the corridor law are
proven load-bearing; the last-ulp question is open and named.

## Wall clock, A18 Pro, 8 GB

Interleaved A/B, five rounds, `/usr/bin/time -p`, real seconds. The C figure is
the two binaries the Python file replaces — mouth then independent reader.

| round | C mouth | C reader | C total | `netta.py` (speaks **and** judges) |
|---|---|---|---|---|
| 1 | 3.89 | 4.70 | 8.59 | 4.63 |
| 2 | 3.82 | 4.79 | 8.61 | 4.95 |
| 3 | 3.79 | 4.61 | 8.40 | 4.94 |
| 4 | 5.09 | 5.97 | 11.06 | 5.30 |
| 5 | 4.24 | 4.91 | 9.15 | 4.84 |

Cold start to `SPEECH PASS`, 447545-byte island, 5 sittings: **4.6–5.3 s**
typical, 6.3 s worst observed under load, 3.0 s best observed on a quiet box.
Second island (89509 B): **0.80–0.92 s**.

This is not "Python beats C". Where the file is faster it is because of an
algorithmic choice, not a language one, and the difference is named: the C
reader rescans all 92853 lived units for every support query, while `netta.py`
answers from the same sorted n-gram tables the mouth uses. Where the two do the
same work, the C is faster. Phase breakdown of the Python run:

```
units     1.89 s   BPE, 4096 merges
tables    0.09 s   uni / bi / tri / quad
speak x5  0.01 s   0.002 s per sitting
court x5  1.08 s   ear + census
total     3.07 s
```

Speaking costs two milliseconds. Growing the units costs everything else, and
the court costs more than the voice — which is the correct shape for an organism
that is judged harder than it is generated.

## Named residuals — what this file does not carry

1. **The Court-4 citizens adapter (MOUTH_PROTOCOL §4) is not implemented.**
   It authenticates a 36101-byte sealed capsule by SHA-256; a single
   self-contained file cannot carry it. The C's own default is `--citizens-mode
   none`, so the parity runs above are exactly what the C does by default, and
   the plain mouth is the one Sitting 1 passed. The trace columns
   `advice_book_row` and `advice_factor` are emitted as the constants the plain
   mouth produces (`0` and `1`) — which is why the C reader accepts the trace.

2. **Default `--corridor` differs on purpose: 1 here, 3 in `netta_mouth.c`.**
   K=3 fails the anti-copy census on this world (measured above); Sitting 1
   pinned K=1 and that is the court-passing dial. Pass `--corridor 3` to get the
   C's compiled default; parity holds at both, and at 0 and 7.

3. **Body 0's five-arm pricing is not ported.** `netta.c`'s Hebbian field,
   permuted-field control and word-trigram baseline are absent. Body 0's own
   verdict deleted the field; the mouth does not resurrect it, and neither does
   this file. What is ported from `netta.c` is its merge law and its xorshift64.

4. **The BPE is the same law by a different route.** The C rescans the whole
   stream every round; `netta.py` maintains the same pair counts incrementally
   over a linked list with a lazy heap, so the pair chosen in round *n* is the
   pair the C chooses in round *n*. This is not asserted, it is measured: same
   merge count, same lived-stream length, same inventory size, byte-identical
   speech on two islands, and the C reader's independently written pair counter
   accepts the result.

5. **The census `longest_match_bytes` can differ in one unreachable case.**
   The C's `longest_at` indexes the world by 4-byte key, so when the true
   longest match at a position is 1–3 bytes and ≥4 bytes remain, it reports 0.
   `netta.py` reports the true length. This can only change the *maximum* if no
   position in the stream matches 4 bytes — impossible on any run where the
   opening three tokens are a lived run. Measured identical on both islands, at
   every dial setting, in every report diff above.

6. **One file cannot be two hands.** `netta.py`'s court re-derives every
   support from the token sequence alone and refuses a token that was never a
   lived continuation (red probe above), but it shares the tables with the
   mouth. Independence is supplied from outside, by `netta_mouth_check.c`, and
   that is the receipt that counts.

7. **`sorted(key=(-score, id))` in the sampler is redundant on these runs** —
   candidates already arrive in ascending id and Timsort is stable, so removing
   the tie-break changed nothing on 5/5 seeds. It stays because the C's law
   should be visible in the code rather than resting on two unstated invariants.

## Reproduce

```
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta_mouth       netta_mouth.c       -lm
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -o netta_mouth_check netta_mouth_check.c -lm

./netta_mouth netta.txt --out c_k1 --corridor 1
PYTHONDONTWRITEBYTECODE=1 python3 pyport/netta.py netta.txt --out py_k1 --report py_k1.txt --time

for s in 7 19 42 101 271; do cmp py_k1/speech_$s.bin c_k1/speech_$s.bin; done
./netta_mouth_check netta.txt --dir py_k1 --report creader.txt --corridor 1
cmp creader.txt py_k1.txt
cmp creader.txt speech_court/sitting1/report_plain.txt
```

All five `cmp` calls and both binaries exit 0.

— built and measured by Claude (Opus, neo), 2026-09-21
