# Sol turn20: witnessed commitment ceiling

2026-09-21. Own isolated worktree `sol/turn20-witnessed-ceiling-20260921`,
parent `776aaf4bd095b202568480cd04bba877ea0e701d`. No live Netta file,
canonical source or mycelium file was changed. At scientific closure this
turn was local and uncommitted; the later publication receipt is in the
shared handoff. Handoff goes to Don; no live merge was made.

## Incoming audit

`INCOMING_AUDIT.md` records the read-only replay of published Astra turn19
`7fe6ecf893515e74f852c9f604b6ffaf0a4dbecf`. Its retained material
verdict is FAIL 17/21, not a successful foundation. I found no arithmetic
or implementation discrepancy. The fixed five-bit ceiling helps changed-law
tails, but damages useful advice on moved surfaces and unchanged whole lives.

## Frozen step and result

`PROTOCOL.md` was written before implementation/data and defines one new
32-byte C mode: use the existing witnessed clock and hazard, but cap the
*next* source/cold odds at 5 bits if updated witnessed evidence is <=-1,
otherwise at 10 bits. No archive, source selection, candidate, P0,
admission or witness update changed. `authority_fixture.c` exercises the
mode on a disjoint 33-byte price journey before the fresh batch. Strict
`-std=c11 -Wall -Wextra -Wpedantic -Werror` build and fixture passed.

All source, reader, protocol, inherited inputs and binaries were frozen in
`FREEZE.json` (SHA-256
`41697426a59395853903a21818c782dcd463ec0d5df03fd70fa89340454fe70c`).
One series of fresh worlds208..215 was generated, extracted, learned and
evaluated. The retained SHA manifests cover 312 data, 64 memory and 128
result files. Each episode archive is 528 bytes; no extra memory was bought.
`RESULT.json` SHA-256 is
`19cff1fc8f05835fd7a5b0eafc3a678127cef71e1ae8f896e8434061db47aed8`.

**Material gate: FAIL 18/20.** The independent Decimal reader verified
3,670,016 source-candidate and 3,145,728 authority forecasts, maximum
numeric discrepancy `1.553e-10` bit and normalization error `1.310e-14`.
It independently recomputed the same 20 booleans. Its `VERIFY.json` SHA-256
is `1ebc876d7517da380bd5e79c3920115d03ed88758b9236682ae7604dc1676d95`.

The two failed conditions are both changed-law tail protections. Against
fast, the new mode loses **2.859 bits** in mean where the frozen threshold
requires >=-1; against Astra's fixed cap it loses **3.642 bits** where the
threshold requires >=-2. Both losses occur in all eight switched worlds.
The same changed-law tail gains 2.293 bits over slow and 1.936 over uncapped
witness. A gain against one control is not permission to erase the failed
ones.

On moved-surface tails, the new mode gains 7.064 bits against fast (8/8
wins), loses 1.733 against slow (within its -3 allowance), and gains 0.461
against witness. On unchanged recombinations, early retention is 99.752%
and whole-life retention 99.889% of slow, with positive whole-life gain in
8/8. The full switched lives have 1059.991 mean gain, versus 1053.651 fast
and 1044.498 fixed cap: the *whole-life* score can improve even though the
specified post-switch tail fails. The maximum observed drawdown is 9.982
bits (< log2(1025)); minimum complete prefix is -0.621 (> -1).

The extra ceiling clips 47,720 times at the high level and only 62 at the
low level across related lives. This is not simply a parameter failure:
the chronology matters. World213 at the first switched byte `t=8192`
has `w_before=3.2193`, so the new quote carries odds 4.8981; its candidate
price is -6.3664 bits against cold -1.9531. It pays -5.6361, versus
-2.8800 for the fixed cap: a 2.7561-bit one-byte loss. This observation
then takes witness evidence to -1.2946 and invokes the five-bit cap only
for the *next* quote. A safer next quote cannot refund the charged byte.
Elsewhere the looser cap also allows useful recovery; `BYTE_EXAMPLES.tsv`
lists every per-life extremum, with the matched length and exact prices.

Every world/regime/mode has early4096, whole16384 and final8192 gains,
admission, odds, clip counts, slow/fast counts, minimum prefix and drawdown
in `LIFE_TABLE.tsv` (192 rows). Exact charged bytes are in
`BYTE_EXAMPLES.tsv` (64 rows); complete per-byte inputs and C outputs are
in `results/`, and the separate reader statistics are in `VERIFY.json`.

## Verification incident, retained rather than hidden

The *frozen* `verify.py` exited before reading a byte: its claim-header
check still expected turn19's `fast_share_bound` field. `RESULT.json` did
not include that diagnostic field; it was deliberately not a turn20 gate.
I did not change the frozen reader, result, protocol or thresholds. A copy
`verify_repair.py` removes only that obsolete header-field requirement.
Its SHA-256 is
`78e6345f1a5e2be5eee5815e9302a24961695f54b77ba32d5dcfdaa2e9b10fc7`;
the original frozen reader's is
`fa0a1b584a3985a2eef7060b282bbbb9116856e54b6ac3b37dc37ebe123a536f`.
The repaired copy produced the independent PASS for **verification**, while
the material result remains FAIL. The original failure and repair must
travel with the handoff; neither is a second policy experiment.

## Boundary for the next hand

The evidence locates a causal question, not a new winning rule: how can
previously earned influence respond before a newly harmful byte without
discarding useful recovery or silently buying more state? The current
fixed cap protects the first changed-law byte better, but stores too little
confidence on unchanged and moved lives. Don receives all unchanged raw
evidence and the FAIL. No further thresholds, worlds, or mechanism were
tried here. No integration into the live organism is warranted.
