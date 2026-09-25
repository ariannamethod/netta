# Turn29: the full memory earns a local right to speak

Sol, 2026-09-25. Base Astra28
`dd6aeec368dd47bc6444a87508be7111e44a0848`; isolated branch
`sol/turn29-local-pooled`. Incoming audit plus one preregistered memory step on
one fresh batch.

**Material PASS: T1--T5 all pass. Independent verification PASS.**

Turn28 established that per-prefix choice was useful, but storing two complete
continuation vectors at every address halved coverage. Turn29 keeps the old
24-record, 528-byte pooled archive intact and gives each record one recipient
scalar: use current local P0 or use this pooled memory. No second source book
is stored.

That separation works on all eight new worlds. The local full archive beats
the same direct pooled archive early in 8/8 worlds, beats the shared-router
control in 8/8, rejects a full-address permuted control in 8/8, retains more
than all of the direct archive's intact full-life gain, and improves every
partial-change tail.

## Incoming Astra28 audit

Astra's published commit and handoff identities match. I ran her frozen reader
from preserved raw evidence into a new path. It reconstructed 2,688 source
counters and 3,276,800 forecasts; the new receipt is byte-identical to the
retained file, SHA-256
`627f29e24b32497b0a255f322e721829dd655f5b98efb0c70876062a4e0e5f81`.
Material FAIL T2/T3/T4 and verification PASS remain.

Code review found no result-changing defect. Quoting precedes truth, source
occurrences do not cross life boundaries, A+B rebuilds pooled counts, local
and global routers have the documented scope, and outer authority is separate
per arm. [Full audit](audit/AUDIT28.md).

## Construction

The portable source memory is the unchanged NETEI001 full pooled archive:
32 rules, 24 records, exactly 528 bytes in every world. At a matched record,
one local recipient weight forms

```
Q = (1-u) P0 + u S,        u0 = 7/8
u' = (1-2^-10) u S(x)/Q(x) + 2^-10 (7/8)
```

Each record owns its `u`. Updates follow the current quote and occur on every
matched visit, including equal prices and NEW. Weights reset between lives and
do not enter the portable archive. Source counts remain immutable. Exact equal
forecasts and NEW copy P0 bit-for-bit.

Five arms preserve the same outer prospective-32 admission and fixed slow
hazard: local full24, global full24, direct pooled full24, frozen Astra bank12
with P0/A/B local choice, and local full24 with permuted repeat counts. The
full and bank traces are emitted by separate C programs and must agree on
truth, P0, HEAD bindings and pretruth history.

Runtime recipient state for the candidate is 24 doubles = **192 bytes**. The
global control uses 8 bytes. The complete comparison process allocates another
192 bytes for the permuted routers. Astra's bank12 local routers use 288 bytes.
These are named separately from portable source memory.

## Frozen batch and verdict

Worlds 272--279, namespace `netta-local-pooled-permission-v1`; five 16KiB
recipients per world, with the same synthetic family and partial manipulation
as turn28. All protocol, code, binaries and dependencies were frozen before
generation. No parameter or gate changed afterward.

Mean received bits saved against the same local P0:

| regime/window | local full24 | pooled full24 | global full24 | Astra bank12 | permuted full24 |
|---|---:|---:|---:|---:|---:|
| intact first 4096 | 685.685 | 560.068 | 582.449 | 342.819 | 0.000 |
| intact full 16384 | 3412.973 | 2937.090 | 2961.855 | 1767.822 | 67.120 |
| partial final 8192 | 739.345 | 409.854 | 482.588 | 459.393 | -0.125 |
| partial full 16384 | 2258.197 | 1659.949 | 1756.790 | 1240.795 | -0.125 |
| switched final 8192 | 143.299 | 32.598 | 43.210 | 281.502 | 0.000 |
| moved-surface final 8192 | 1048.109 | 574.381 | 694.503 | 471.444 | 11.602 |
| unrelated full 16384 | 54.339 | 0.000 | 0.000 | 145.223 | 0.000 |

Preregistered conditions:

| condition | result |
|---|---|
| T1 early positive transfer | PASS: .167404 bit/byte, positive 8/8 |
| T2 local permission over direct pooled | PASS: +125.617 bits, wins 8/8 |
| T3 intact retention and coverage recovery | PASS: 116.203%; positive 8/8; +1645.152 over bank12, wins 8/8 |
| T4 partial-change adaptation | PASS: +329.491 tail, wins 8/8; +598.247 whole |
| T5 local scope and correspondence | PASS: +103.236 over global and +685.685 over permuted, wins 8/8 each |

Every individual comparison required by the gate wins; this is not a mean
rescued by one world. Per-world values and all other regimes are retained in
[TABLES.md](TABLES.md) and `RESULT.json`.

## What the result establishes

Turn28's apparent dilemma was not fundamental in this synthetic family. Netta
does not need to duplicate a source continuation at every remembered prefix in
order to decide locally whether that memory applies. Keeping all addresses and
attaching local permission is both cheaper in recipient state and materially
better here.

The gain is not merely delayed rejection. On intact lives local permission
improves the direct archive's complete mean by 475.883 bits. On partial lives
it improves both the changed tail and the whole life. The global binary router
also improves the direct archive slightly on intact full lives, but loses to
per-record state early in every world: evidence from one relation should not
decide authority for all relations.

This does **not** show that distinct source histories are useless. Turn28 has
real within-covered-context A/B witnesses. Turn29 shows that, under this fixed
budget and family, preserving twice as many addresses plus local P0 permission
is more valuable than storing both complete continuations at half the
addresses. A matched bank12 binary-router control is still needed to isolate
the residual value of A/B selection on identical addresses.

The strongest single saved help and harm rows are both retained. At one
partial-tail byte local permission saves 4.881 bits over direct pooled; at
another it loses 5.256 bits because a record it had learned to reject becomes
correct again. Local revision is useful, not clairvoyant. [RAW.md](RAW.md).

## Independent reader and boundaries

The reader independently recounts source continuations, reconstructs pooled
and full-permuted archives, replays binary and three-way routers, cross-checks
the two C traces, replays all five outer authorities and rebuilds the verdict.
It checked **3,276,800 forecasts**, 2,688 source counters, maximum numeric
difference `6.320988177321851e-11`. All validity checks and T1--T5 pass.

Worst full-prefix loss is -0.999854 bit and maximum drawdown 15.286337 bits,
inside the frozen -1/16 laws. `unrelated` local full admits in 6/8 worlds and
all six final gains are positive, but these worlds retain the same role/emitter
machinery and are not called IID or semantically unrelated.

Inherited greedy grammar selection and HEAD256 P0/bindings remain hash-pinned
supplied boundaries. This is exact learned-prefix transfer in a synthetic
family, not functional similarity, autonomous case discovery, natural
language, or a fifty-life curriculum. No mouth, Body 0, mycelium or live Netta
integration occurs.

## Next hand

Next recipient: **Don**, under Sol -> Don -> Astra -> Sol. Audit exact source
identity, both trace programs, the full-permutation control, reader receipt and
the interaction between local routers and their separate outer authorities.

Then take one bounded step. The clean unresolved attribution is on equal
addresses: compare turn28's P0/A/B router against a P0/(A+B) binary router on
the same 12 prefixes and fresh worlds. That says whether keeping distinct past
continuations adds anything after local permission is already present. Only
after that result should a sparse alternative encoding spend portable bytes.

This turn remains isolated and uncommitted. Commit, push, merge and live
integration require Oleg's explicit instruction.

## Identities

- PROTOCOL: `3174e44eb930042d2018aab6108dcc558d4d00d4426f5fdd8af1247dc001166c`
- FREEZE: `10e5fa044b78355fbb8c01217d43346f04bb909eb51ce4a60aaf472e6763e4dd`
- RESULT: `e1d4f14de7791b5a2bebce826d9ff36087f3f3d5ba7bf5dafc0dc20c121b81a3`
- VERIFY: `1333d5b57349a3557bd8d821c449ed26136e17a37c5a0974c9f7ed0543c74b34`
- candidate C: `9a64800440d06e70bb7f29772517ff2af443db32d9603c91c00715c1c3d36a2e`
