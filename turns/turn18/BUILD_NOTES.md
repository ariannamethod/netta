# Turn 18 build notes

Every ambiguity in `PROTOCOL.md` resolved minimally, every departure from a
literal reading, and every place the implementation had to choose. The
protocol itself was not edited and hashes to
`f6eb3c3bb156ba0a181dc66d811c4f99518b41f0453bad740fc12052188be6b6` in
`FREEZE.json`, in `verify.py`'s `PROTOCOL_SHA` literal and in `RESULT.json`.
Builder hand: Opus. Acceptance, pristine reruns and integrity probes are
Don's.

Workspace: `/Users/ataeff/arianna/netta-don-turn18-20260921/`, an isolated
clone on `sol/turn17-prospective-hazard-20260921` at `3aaeec9`. All work
lives in `turn18/`. No git operation of any kind was run. No network.
Canonical netta, `~/arianna-codex`, `~/arianna-shared` and every sealed
world range were not touched.

## 0. The terminated hand's leftover

`turn18/authority.h` was on disk when this hand started, left by a builder
that an infrastructure error killed early. It was treated as untrusted and
diffed against `turn17/authority.h`. It carried the two intended additions
(`AR_WITNESS`, the `matched` parameter) and one change beyond them: the
state field `evidence` renamed to `clock`. A rename is not an addition, so
the file was rewritten from `turn17/authority.h` with additive changes only
and the inherited field name kept. Nothing else from that hand survives;
no other leftover existed.

Keeping one scalar rather than adding a second is not a convenience. The
protocol says "one signed wrongness state `w`, initially 0, one 32-byte
state record", and `double shadow + double odds + double evidence + ARMode
+ unsigned` is exactly 32 bytes. A separate `w` field would make it 40 and
contradict the declared record. The modes are mutually exclusive per
`ARState`, so the field carries turn17's evidence EMA under `AR_ADAPTIVE`
and turn18's `w` under `AR_WITNESS`. `authority_replay` prints
`state_bytes=32` on every run.

## 1. How turn18's authority differs from turn17's

`authority.c` is turn17's file with three additions and nothing else
changed — same `logadd`, same `valid_prices`, same one-way odds update,
same first-admission branch, same error discipline:

1. `AR_WITNESS` added to the hazard selector:
   `(state->mode == AR_WITNESS && state->evidence > -1.0)` chooses slow.
   The protocol says `h = 2^-10 if w <= -1 bit, otherwise 2^-16`, so slow
   is the strict complement `w > -1.0`. A `w` landing exactly on -1.0 takes
   the fast branch, which is the literal reading of `<=`.
2. The witness clock update, gated on the vote:
   `if (state->mode == AR_WITNESS && matched >= 1) next.evidence =
   (31.0/32.0)*state->evidence + delta;`
   `matchedL == 0` leaves `w` untouched. Both constants are written as
   literals at their single use site and are never read from anywhere
   configurable; nothing scans them.
3. `matched < 0` joins the argument validation in `ar_observe`.

The adaptive branch is byte-for-byte turn17's, including its unconditional
`(255.0/256.0)` update on every active byte. A pre-freeze mutant confirmed
that the witness vote gate does not leak into it (section 9).

The first-admission branch already zeroed the clock field, which is
turn17's "the crossing observation does not enter the clock"; that rule now
covers `w` for free, exactly as the protocol restates it.

## 2. `replay.c`: a fourth field on the price tape

The tape gained `matchedL`, so a row is `t cold candidate matched` and
`sscanf` demands four fields. Column layout changed shape: turn17 printed
one `live`/`odds_before` pair per mode and then a single adaptive clock
block. Turn18 prints, per mode, `live`, `odds_before`, `slow_used`,
`odds_after`, and then both clock blocks
(`adaptive_e_before/e_after`, `witness_w_before/w_after`) — 27 columns.

`slow_used` and `odds_after` are now emitted for all four modes rather than
for the adaptive mode alone. The protocol's condition 9 asks the reader to
rebuild "every quote, state, hazard choice and clock trajectory for all
four modes"; with turn17's layout the reader could only compare its
`slow`/`fast` hazard decisions against nothing. Now every hazard decision
in the batch — 2,097,152 of them — is a stored column the reader checks.

Two runtime guards were added beyond turn17's:

- `if (!before[m].active && live[m] != cold) fail("exact inactive price")`
- `if (slow_used[AR_SLOW] == 0 || slow_used[AR_FAST] == 1)
   fail("fixed control hazard")`

Both are assertions about the fixed controls, not new law. They fire never
in a correct run and turn a silent control drift into an immediate
non-zero exit.

The interval-drawdown assertion inside `charge()` is turn17's unchanged:
10 bits for `AR_FAST`, 16 for the others. See section 7 for why the gate
test uses 16 everywhere while this internal bound stays tighter.

## 3. Where `matchedL` comes from, and how the reader gets it independently

The writer takes `episode_matchedL` from the retained C predictor trace
(`turn13/episode.c:464` emits it; the price tape's fourth field is that
column verbatim) and asserts the replay echoed it back unchanged.

The reader does not trust that column. `turn18/verify.py:verify_authority`
keeps its own 32-deep history of observed ranks and calls
`v13.suffix_match` against the episode table that `v13.verify_books`
rebuilt from the source tapes with the inherited reader's own code. It
compares its own length against the trace column and against the replay
tape's `matched` field, and it is its own value that drives its Decimal
witness clock. Both comparisons are hard equality on integers. So the
matchedL sequence that gates the clock is reconstructed, not adopted.

History bookkeeping mirrors `turn13/verify.py:verify_life` exactly: the
match is taken before the byte is priced, the observed rank is appended
after, and the deque is trimmed at 32.

## 4. Condition 5: "within 256 bytes after the t=8192 seam"

Implemented as: let `first_fast_after_move` be the smallest `t >= 8192` at
which the witness clock held 2^-10; the world counts if that value exists
and `first_fast_after_move - 8192 < 256`, i.e. the window is bytes
8192..8447 inclusive, 256 bytes. The gate is `sum >= 6` over the eight
switched lives.

`first_fast_t` — the first fast byte anywhere in the life — is recorded
separately for every mode and life so the reading is auditable: if a clock
had already been fast before the seam, the condition would be measuring
something else, and the two fields together show it. In this batch no
switched life is fast at the seam; the eight offsets are 40, 28, 21, 65,
32, 311, 120, 37.

## 5. Condition 6: "at most 25% of the final 8192"

`tail_fast_count / 8192 <= 0.25`, where `tail_fast_count` counts bytes
`t >= 8192` at which the witness held 2^-10. The denominator is the literal
8192 bytes of the final segment, not the number of active bytes in it. The
two differ only before admission, which happens near byte 100 in every
world, so no final-8192 byte is inactive anywhere in this batch and the
choice is not load-bearing here. It is recorded because it would be if a
life ever admitted late.

## 6. Condition 8: recorded, never raised

Turn17's reader raised on an equal-price violation. In turn18 exactness is
a *gate condition*, and the protocol says a FAIL is preserved with a
diagnosis. A reader that raises would turn a FAIL into a crash and destroy
the evidence, so both hands record `equal_price_exact`, `new_exact` and
`inactive_exact` per life as booleans and let `c8_*` report False. The
comparison is `stored != cold` on the IEEE doubles read back from the
trace — hard equality, no tolerance — in both hands, for all four modes and
all 32 lives. The C hand additionally refuses at run time
(`replay.c`: `"exact equal price"`, `"exact inactive price"`), so a
violation could only reach the gate through a path the C guard does not
cover.

`inactive_exact` is folded into `c8_equal_price_exact` rather than given a
tooth of its own: an inactive quote is the same "quote exactly C" claim
that protected NEW and equal prices make, and the protocol names two
classes, not three. Both flags are stored separately in
`RESULT.json:lives[*].exactness` so the distinction survives.

## 7. Condition 7's drawdown bound

The protocol says "interval drawdown at most 16+1e-7 in every mode". The
gate test `c7_drawdown_bound` is exactly that: 16 for all four modes,
including `fast`. Turn17's gate used the tighter `10 if fast else 16`.

The tighter bound did not disappear: it survives as a runtime assertion in
`replay.c:charge` and in `verify.py:Authority.step`, where it is an
arithmetic consequence of the hazard floor (`-log2(2^-10) = 10`), not a
performance claim. So the gate is the protocol's, while a fast-mode
drawdown above 10 bits would still abort both hands rather than pass
quietly. Observed maximum over all 32 lives and four modes: well inside
both bounds.

## 8. Condition 9 lives in `VERIFY.json`, not in `RESULT.json`

`RESULT.json` is written by the experiment before the reader has run — it
carries `independent_reader_pending: true`, turn14's convention that
turn15 and turn17 both kept. A `c9_*` boolean in that file would be a
claim about an event that has not happened, so:

- `RESULT.json:tests` carries the twenty named booleans of conditions 1–8
  and `gate_pass` is their conjunction.
- `verify.py` rebuilds those same twenty independently, requires
  `claim['tests'] == tests`, and writes them again in `VERIFY.json:tests`
  alongside `VERIFY.json:reader_tests`, which holds the condition 9 family:
  `c9_all_candidate_arms_rebuilt`, `c9_all_four_modes_replayed`,
  `c9_agreement_within_tolerance`, `c9_retained_digests_intact`.
- `VERIFY.json:nine_family_gate_pass` is the conjunction of all nine
  families. `VERIFY.json:gate_pass` is conditions 1–8, so it can be
  compared directly with the writer's.

This is the one structural departure from a literal "all nine as named
booleans in RESULT.json", and it is made so that no file claims something
it cannot have observed.

## 9. Gate test names

Twenty named booleans, each prefixed by the protocol condition it
implements: `c1_*` (3), `c2_*` (3), `c3_*` (1), `c4_*` (3), `c5_*` (1),
`c6_*` (1), `c7_*` (6), `c8_*` (2). Both hands assert `len(tests) == 20`,
so a tooth cannot be dropped silently. Condition 9 is `VERIFY.json` itself
(section 8).

`c7_unrelated_never_admits` checks all four modes rather than the witness
alone; admission is shared by construction, so this is the stronger form
and would catch a broken sharing.

## 10. Clock trajectories, extrema and the raw-sample rule

`RESULT.json:lives[*].clocks` holds, for `adaptive` and `witness`, the
minimum and maximum of the clock value *entering* each byte with the byte
index at which it occurred, the value entering t=8192, and the value
leaving t=16383. Extrema use strict `<` / `>`, so the first occurrence
wins and the index is deterministic.

The writer reads those values from the C columns; the reader computes them
from its own Decimal replay. The two sequences agree to ~1e-13 and the
recovered indices matched everywhere in this batch, but they are in
principle separable: if two bytes ever produced clock values within 1e-13
of the extremum, the two hands could name different indices while both
being right. The stronger independence was kept and the fragility is
recorded here rather than engineered away.

The raw samples are deliberately not built that way. `raw_guilt_saving` is
the byte in a switched or moved tail with the largest `witness_live -
slow_live` among bytes the witness held at 2^-10; `raw_hedge_cost` is the
byte with the most negative `witness_live - fast_live` among bytes it held
at 2^-16. Both hands take that argmax over the *stored* trace columns, so
they read identical IEEE doubles and cannot disagree on which byte is
meant; the recorded field values are then compared like any other number.
The two comparison partners follow the question: guilt is a deviation from
the slow default, and the hedge is what retention costs against fast.

## 11. `evaluate` additions over turn17

`RESULT.json` gained `tables` (per regime, per mode, means of gain / tail /
early-4096), `window` (256) and `fast_share_bound` (0.25). The reader
recomputes `tables` and compares it element by element, and refuses if the
two thresholds in `RESULT.json` differ from its own constants — a
mechanism threshold cannot be edited in one hand only.

`freeze()` additionally asserts `memory/` does not exist; turn17 checked
only `data/` and `results/`.

## 12. What `FREEZE.json` pins

Twenty-eight files: the five inherited repository sources
(`byte_recurrence`, `portable_recurrence`, `court4` core), `turn13`'s
episode/experiment/verify, `turn15`'s surface law, `turn16`'s authority
law, `turn17`'s seven code and protocol files, this turn's seven files, and
the two binaries built here. Every inherited digest was checked against
`turn17/FREEZE.json` before a line was written and all of them matched;
`INHERITED_SHA256.txt` records them.

`turn18/Makefile` is byte-identical to `turn17/Makefile` (`diff` rc 0), and
`turn18/episode` hashes to
`784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10`, the
value `turn17/FREEZE.json` pins for `turn17/episode`. The build is
reproducible across turns on this machine.

## 13. Pre-freeze probes

All of the following ran before `freeze` and before any byte of worlds
192..199 existed, in the scratchpad, not in `turn18/`. A tooth that cannot
fail is decoration, so each was broken and watched go red.

**The C law against an independent recount.** A 3000-row synthetic price
tape exercising both hazards (2494 fast / 461 slow active bytes), both
match regimes (1971 voting / 984 silent), 248 equal-price bytes and exactly
one admission, recomputed by a float implementation written from the
protocol text. Agreement 1.563e-13 on every column; malformed tapes
(negative match length, three fields, non-chronological `t`) all rejected
non-zero. Then three mutants of `authority.c`, each rebuilt and rerun:

| mutant | what broke | probe |
|---|---|---|
| drop the `matched >= 1` gate | silence enters `w` | red at t=51, `silence moved w` |
| `w < -1` instead of `w > -1` for slow | threshold inverted | red at t=45, `witness hazard 0 != 1` |
| `255/256` instead of `31/32` | wrong decay | red at t=46, `witness update` |

The unmutated build is the green counter-probe (rc 0, all checks green).

**The reader against the C hand.** `verify.py`'s Decimal `Authority`
stepped the same 3000-row tape for all four modes: 12000 hazard decisions
and every live / odds / clock column, maximum absolute error 2.103e-11.

**All twenty gate teeth.** A synthetic `lives` tree that passes all twenty
was perturbed 23 times, once per tooth plus three second variants
(condition 5 broken both by `None` and by an offset of exactly 256;
condition 7's normalization broken both by `max_norm_error` and by
`column_new_exact`; condition 8's equal-price broken both by the equal and
the inactive flag). Every targeted tooth went False, both hands agreed on
the full twenty in all 24 configurations, and the archive-cap tooth was
broken with a real 529-byte file and then restored green. Collateral
damage was recorded where it occurred and is structural in both cases
(making the witness tail track fast also moves it relative to slow; zeroing
one world's gain also moves the retention ratio).

**The whole pipeline, end to end, on throwaway worlds.** `evaluate` is the
irreversible stage — `results/` is written once with `open('x')` and the
stage refuses to start if it exists — so the pipeline was run in full on
worlds 900..901 under namespace `netta-witness-clock-smoke`, in the
scratchpad, loading `experiment.py` and `verify.py` verbatim and rebinding
only `HERE`, `WORLDS` and `NAMESPACE`. Generate, extract, learn, predict,
replay, `summarize`, `verify_books`, `verify_life`, `verify_authority` and
a full life-tree `compare` between the two hands: rc 0, reader/writer
trees agreeing, maximum authority error 1.370e-10. Nothing was written
inside the repository and worlds 192..199 did not yet exist.

## 14. Order of operations, and what was irreversible

`make` (rc 0) -> C law probe (rc 0) -> three C mutants (all red) -> gate
probes (rc 0) -> dry run (rc 0) -> `freeze` (rc 0) -> `generate` (rc 0) ->
`extract` (rc 0) -> `learn` (rc 0) -> `evaluate` (rc 0) -> `verify.py`
(rc 0). `freeze` ran with no `data/`, `memory/` or `results/` present,
verified by `ls` immediately before.

Once `generate` completed nothing was regenerated, retried or tuned: one
run of each stage, one gate, one reader. `FREEZE.json`, `RESULT.json`, the
three manifests and `VERIFY.json` are all written with `open('x')`;
`evaluate` refuses to start if `results/` exists; `verify.py` refuses if
its output file exists, which was probed directly — `--output VERIFY.json`
and `--output RESULT.json` both exit 1 with `verification output already
exists`. No file pinned by `FREEZE.json` was touched after the freeze.

## 15. The reader's refusals, probed

Condition 9 requires the reader to refuse by name on retained-artifact
digest drift. Probed without modifying anything on disk: `verify.py` was
loaded as a module against the real tree and its own `digest` function was
perturbed for one named path at a time. `identity()` passed on the
unperturbed baseline and refused by name on a frozen inherited source
(`frozen file turn13/episode.c`), a frozen turn18 source (`frozen file
turn18/authority.c`), the protocol (`frozen protocol identity`), and each
of the three retained classes (`retained artifact
results/world195/switched.tsv.gz`, `retained artifact
memory/world192/episodes.bin`, `retained artifact
data/world199/moved_mid.bin`). A destructive one-digit probe on a real
artifact belongs to Don's hand and was not run here.

## 16. One error this hand made and corrected

The mean rows of `RAW.md`'s tail tables were first typed by hand. A
check that re-derived every table cell from `RESULT.json` found the moved
`adaptive` mean written as 775.982 against an actual 776.233 — an
arithmetic slip — plus three cells truncated rather than rounded at the
third decimal. All four were corrected and the check now reports 18/18
means and 144/144 table cells matching, with all 26 raw-byte field values
present verbatim. No number in any deliverable is typed from recall.

## 17. What the run found

`gate_pass` is **false**. Seventeen of the twenty teeth pass; the three
that fail are the law-tail family, and they fail in an informative way.

The mechanism conditions both pass, and pass well. The witness clock
reaches 2^-10 within 256 bytes of the seam on 7 of 8 switched lives
(offsets 21, 28, 32, 37, 40, 65, 120, and 311 in world 197), where turn17's
adaptive clock needed 253 to 1097 on its own batch (`turn17/RAW.md`: the
first fast-clock byte lay between t8445 and t9289 across its eight switched
worlds). On the moved tails the witness stays at 2^-16 for at least 75% of
the final 8192 in 6 of 8 worlds. Silence and guilt are
therefore distinguishable signals and a 32-byte clock fed only by witnessed
wrongness does separate them, which is what the protocol asked.

It did not buy the bits. On the switched tails the witness beats slow by
0.194 bits in mean (7/8 wins, gate wanted >1) and beats the adaptive
incumbent by 0.241 bits (8/8 wins, gate wanted >1), while trailing fast by
5.068 bits (gate allowed 1). The diagnosis is in `RAW.md` and it is not
"the clock was too slow". Splitting each switched tail at its own first
fast byte: witness trails fast by 2.449 bits *before* it reacts and by a
further 2.619 bits *after* it is already at 2^-10. The reason is the odds.
At t=8192 the witness stands at 11.002 bits of commitment to memory, fast
at 4.975 — a gap of 6.03 bits in every one of the eight worlds, to within
0.05 bits, which is `log2(2^-10 / 2^-16)`. A hazard clock sets the rate at
which commitment decays, not the level it has already reached. The witness
walks its odds down to fast's seam level in 11 to 124 bytes and pays for
the whole descent; fast never had to pay because it was never that
committed.

So the turn does answer its one question, and the answer is: withdrawing
fast enough is not sufficient, because the debt is a level and the clock is
a rate. Condition 4 is untouched by all of this — the witness retains
99.90% of slow's early-4096 and 99.97% of its full-life gain on the
unchanged recombined lives, positive in 8/8 — so the protection is free
where nothing changed. On the moved tails the witness beats fast by 7.084
bits in mean, 8/8, and trails slow by only 1.920, which is condition 1
passing comfortably: the hedge is correct where the memory has merely gone
silent.

No condition was retried, no policy was run twice on this batch, and the
FAIL is preserved exactly as measured.

## 18. Not this turn's business

No commit, push, merge, branch or live integration. `RESULT.json` still
carries `independent_reader_pending: true`: the writer's file records that
it was written before the reader ran, and `VERIFY.json` is the reader's own
word. Acceptance, a pristine rerun and destructive integrity probes are
Don's hand. `turn18/.gitignore` matches turn17's so the batch, the memory
and the binaries stay out of the index; adding it is not a git operation
and it is not pinned by `FREEZE.json`.
