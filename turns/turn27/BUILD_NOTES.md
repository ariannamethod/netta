# Turn27 BUILD NOTES — what was written, what was inherited, what was proved

Builder: Opus subagent, under `PROTOCOL.md` frozen before any byte of worlds
256..263 existed. Acceptance, pristine reruns and integrity probes are Don's
hand. No commit, push or merge belongs to this turn.

## Sources

Two new C files and one new replay. Everything else is linked or run unedited.

| file | origin |
|:-----|:-------|
| `cusum.h`, `cusum.c` | new: this turn's candidate law |
| `replay.c` | new: six-arm driver |
| `../turn25/latch.c`, `latch.h` | linked **unedited** — the hysteresis arm IS turn25's law, same object code |
| `../turn22/authority.c`, `authority.h` | linked **unedited** — the hazard/odds/admission substrate |
| `../turn13/episode.c` + `byte_recurrence` + `portable_recurrence` | built through the private `.build/layout` symlink tree, exactly as turn24 and turn25 do; no file outside `turns/turn27/` is written |
| `../turn24/oracle` | the retained binary, run as it stands for the equivalence probe |

`experiment.py` imports turn22's generator module (which imports turn13's) and
rebinds `HERE`/`REPO`/`WORLDS`/`NAMESPACE`, the same inheritance turn24 and
turn25 use. No generator law was touched.

## Diff against the incumbent law (`turn25/latch.c` → `cusum.c`)

`cusum.c` is `latch.c` with three substantive changes and nothing else; the
remainder of the diff is renames (`LRState`→`CSState`, `lr_`→`cs_`,
`fast_latched`→`latched`).

1. **Resting mode `AR_SLOW` instead of `AR_WITNESS`.** turn25 kept the core in
   witness mode so the inherited EMA clock kept running, and restored that mode
   after each observation. This law runs no inherited clock at all, so the core
   rests in `AR_SLOW` and `valid_state` asserts `core.evidence == 0.0` — the
   witness clock is provably unused rather than merely unread.
2. **The statistic.** `next.core.evidence = (31/32)*evidence + delta` becomes
   `raised = state->sum + (-(candidate - cold) - CS_DRIFT); next.sum = raised > 0 ? raised : 0`.
   The parenthesisation is the protocol's own, `S + (-delta - k)`, and the
   reader uses the identical expression in identical IEEE-754 doubles, so the
   latch byte cannot move by an ulp at the threshold.
3. **One-way.** turn25's `else if (evidence >= 1.0) fast_latched = 0;` release
   branch is deleted. `if (next.sum >= CS_THRESHOLD) next.latched = 1;` sets and
   never clears. The invariant `(latched || sum < CS_THRESHOLD)` in
   `valid_state` makes that a checked property of every state the law returns,
   not a comment.

The admission branch is unchanged in shape: `else if (first) { sum = 0; latched = 0; }`
— the crossing observation enters neither the sum nor the flag.

`CS_DRIFT 0.5` and `CS_THRESHOLD 8.0` are `#define`s in `cusum.h`. They were
written once, before the freeze, and never scanned, swept or fitted.

## Diff against the yardstick law (`turn24/oracle.c` → `replay.c`)

turn24's `oracle.c` carries a `main()` and eight arms (four delays × rate/level),
so it cannot be linked. The O-rate-0 arm is `oracle.c:104-114` with `DELAY=0`
and `LEVEL=0`, which reduces to: hold an `AR_SLOW` state, and at `t == seam` set
`mode = AR_FAST`. That is reproduced in `replay.c` together with turn24's own
runtime asserts — pre-switch identity to the slow reference in quote, state and
hazard; no second switch; no switch on a life with no law change; a declared
switch that must be present. The clamp machinery is absent because O-rate-0 has
no clamp. `argv[1]` carries the seam, as in turn24, and reaches no other arm.

The hysteresis asserts in `replay.c` are turn25's `replay.c:98-107` verbatim
(old-flag hazard chronology, the transition law, "neutral observation released
latch"). The `charge()` bound is turn25's: 10 bits for the fast arm, 16 for the
others.

## Measured state

From every production receipt, e.g. `results/authority/world259/switched.tsv.log`:

```
state_bytes ar=40 lr=48 cs=56 total=264
```

`sizeof(CSState)` is **56 bytes** — `ARState` (40) plus one double and one
unsigned, padded. The protocol's "40-byte class; measure actual sizeof" is
answered with 56.

## Equivalence — what is and is not claimed

**The hysteresis arm.** turn25's `episode`, `authority_replay` and `fixture`
binaries are listed in its `.gitignore` and are absent from this tree, so there
is no original binary to compare against. What was done instead: every turn25
*source* in its `FREEZE.json` was checked against the tree (0 diffs), and
`.build/turn25_replay` was rebuilt from those pinned sources
(`turn25/replay.c`, `turn25/latch.c`, `turn22/authority.c`). The claim is
therefore **bitwise equality against turn25's replay rebuilt from its frozen
sources** — not against turn25's own binary. `EQUIVALENCE.json` records both
shas and says so in its `scope`.

**The yardstick arm.** `turns/turn24/oracle` is present and its sha256 matches
`turn24/FREEZE.json` exactly, so it was run as it stands. That comparison is
against the original binary.

Results, on whole production lives, printed-field by printed-field:

| life | hysteresis vs turn25 (8 cols) | shared slow/fast/witness (12 cols) | orate0 vs turn24 (4 cols) |
|:-----|------------------------------:|-----------------------------------:|--------------------------:|
| world256/switched  | 0 | 0 | 0 |
| world256/recombined | 0 | 0 | 0 |
| world257/moved_mid | 0 | 0 | 0 |

16384 rows each. The same comparison on the handcrafted preflight tape (200
events, both seam settings) is also 0.

## Labels — naming

turn13's generator already writes the hidden `data/world{w}/GENERATOR.json`. The
labels stage reads it, pins its sha, and asserts the constructional fact that
makes the seam derivable: `emission_seeds['switched'] == emission_seeds['recombined']`
(both emit under the `recipient` body). It then checks the tapes themselves —
shared raw prefix, switched command tape equal to `recombined[:8192] + unrelated[8192:]`,
and that `moved_mid` has no command tape at all — and writes the derived file as
`labels/GENERATOR_LABELS.json`. The derived name is distinct from the
generator's own per-world `GENERATOR.json` so the two are never confused; the
derived file carries the sha of each generator file it read. `verify.py`
re-derives all of it from `data/` independently, including turn22's surface
bijection from the namespace seed.

## Label confinement, as a structural property

`verify.py`'s `Authority.__init__` refuses a seam for any arm but the yardstick
and refuses to construct the yardstick without one:

```python
need((seam is None) != (mode == YARDSTICK), ...)
```

A gated arm that had consulted the seam could not agree with a recompute that
cannot receive it. That is the reader's proof of confinement; the writer's is
the runtime assert in `replay.c` that the orate0 arm is identical to slow in
quote, hazard and odds on every byte where it is not armed.

## One pre-freeze change to the writer's shape

`pack_result` gained a `labels_path` argument so `preflight.py` can drive the
**real** packer against a stub label file under `.build/` before any labels
exist. Made before the freeze; `FREEZE.json` pins the resulting file.

## Defect found by the reader, after the freeze — NOT repaired

`verify.py` ran to completion over all 32 lives and then refused with
`KeyError: 'c7'`, rc=1, no `VERIFY.json` written.

Cause: `verify.py:547` builds `test_details` with keys `c1..c6`, while the
final loop iterates `GATED_TESTS` (`c1_law_tail .. c7_hygiene`) and asks for
`rebuilt['test_details']['c7']`. The writer's `experiment.py:490` has `c7=hygiene`;
the reader does not. One missing key in the reader's reporting dict.

What the reader had already completed before refusing, in order: schema
acceptance; `identity()` over every `FREEZE.json` pin and all five manifests;
every declared scalar including `tests.c8_reader is False`; independent
re-derivation of all eight world labels against `GENERATOR_LABELS.json` and
`RESULT.json`; `check_life` for all 32 lives with strict key-set comparison
against `RESULT.json` at 1e-7; the forecast count 8×4×16384×6 = 3,145,728;
and `compare()` of `quantities`, `bars`, `hygiene`, `false_latch_census`,
`table`, `life_table`, `cusum_trajectories`, `unrelated_admissions` and
`fast_own_drawdown_bound`. Inside the final loop it agreed with the writer on
the verdicts of **all seven** gated teeth and on the detail blocks of c1..c6.
The only comparison never performed is c7's detail block — which is the
`hygiene` dict, already compared in full one step earlier.

This was not repaired. The turn24 amendment ruling permits a post-freeze repair
only when **(a)** the defect is a crash, **(b)** no RESULT exists yet, **(c)** a
signed amendment records both shas, **(d)** the checker enforces
frozen-or-amended. (a), (c) and (d) are available here; **(b) is not** —
`RESULT.json` existed before the reader ran. Editing a frozen reader after
seeing the results is the situation that wall exists to forbid, and a gate is
answered with work, not by editing the gate. The fork goes to Don.

`check_freeze()` in `experiment.py` and `identity()` in `verify.py` already
implement the amendment-aware check, so if Don rules the repair lawful it needs
only `FREEZE_AMENDMENT.json` with the frozen and amended shas of
`turns/turn27/verify.py`. The frozen sha is in `FREEZE.json`.

Note what this does not change: `gate_pass` is already false on C1, C3 and C6
from the writer's own hand, and the reader agreed with the writer on every one
of the seven gated verdicts before it refused. C8 cannot rescue the gate.
