# Turn 15 build notes

Every ambiguity in `PROTOCOL.md` resolved minimally, every departure from a
literal reading, and every place where the implementation had to choose. The
protocol itself was not edited. Builder hand: Opus. Acceptance, reruns and red
probes are Don's.

Workspace: `/Users/ataeff/arianna/netta-don-turn15-20260919/`, an isolated clone
of canonical netta at `2392228ad502aaa752c756c92d5585878eaef876`. All work lives
in `turn15/`; `git status` on the clone reports exactly one line, `?? turn15/`.
No git operation of any kind was run. No network.

## 1. The byte bijection pi_w and its seed

The protocol asks for a per-world uniform byte bijection drawn "from the
namespace+world seed using the same seeding discipline the turn13 generator
uses", suggesting `hash(namespace|world|"surface")`. Implemented exactly that,
reusing turn13's own seed function and its own alphabet-drawing idiom:

    seed(w, 'surface') = int.from_bytes(
        sha256(f'netta-surface-move-v1|{w}|surface'.encode()).digest()[:8], 'big')
    order = list(range(256)); random.Random(seed(w, 'surface')).shuffle(order)
    pi_w = bytes(order)          # pi_w[b] is the image of byte b

`seed` is turn13/experiment.py:41-42 with `NAMESPACE` overridden to
`netta-surface-move-v1`; the shuffle idiom is turn13/experiment.py:95, where the
generator draws each life's 64-byte alphabet the same way. The stream name
`surface` is dedicated: it collides with none of turn13's names (`components`,
`commands-*`, `alphabet-*`, `emit-*`).

`verify.py` rebuilds pi_w from the namespace and world alone with its own
seeding and shuffle code (`verify.py:surface_map`), never calling
`experiment.py`. It checks the map is a permutation and injective, and only
afterwards compares it with the `surface_map` field recorded in the world's
hidden `GENERATOR.json`. Order matters: the reader reproduces first and
compares second, so a wrong recorded map is caught rather than adopted.

pi_w is written only into `data/world*/GENERATOR.json`, which holds turn13's
hidden generation labels and carries the same scope note. No learner, archive,
trace or command file contains it. The learners read only `*.bin` byte streams
on stdin.

## 2. pi_w acts on emitted bytes, not on the command tape

"every byte of the recombined stream passed through pi_w" is implemented on the
**emitted byte stream** `recombined.bin`, not on the 4-symbol command tape that
drives the emitter. Reasons: pi_w is a permutation of 0..255 while commands are
0..3; gate condition 1 requires `moved_mid`'s first 8192 bytes to be
byte-identical to `recombined` and at least 90% of later positions to differ,
which is exactly what renaming an emitted stream produces; and the equivariance
theorem is about "a target life renamed through a fixed byte bijection", i.e.
the life's bytes. Renaming commands and re-emitting would produce an unrelated
life with no bijection between the two, making condition 2 meaningless.

So, per world:

    moved_whole.bin = bytes(pi[b] for b in recombined.bin)
    moved_mid.bin   = recombined.bin[:8192] + bytes(pi[b] for b in recombined.bin[8192:])

Both are derived files. They have no `.alphabet.bin`, `.commands.bin` or
`.emit.log` because they are not emitted — nothing was drawn for them, they are
the recombined life read back and renamed. `verify.py` recomputes both identities
byte for byte from `recombined.bin` and its own pi_w.

## 3. Why `generate_world` is reimplemented and nothing else is

The inheritance rule is to import the frozen turn13/turn14 code, never to
copy-and-edit it. That holds for everything except the generator entry point:

- `turn13.generate_world` cannot be reused. It loops over
  `SOURCES+REGIMES` with turn13's own `REGIMES`, and its last act
  (turn13/experiment.py:100) asserts `recombined.bin[:8192] == switched.bin[:8192]`.
  Turn 15 does not generate `switched` at all, so that assert would fail on a
  missing file. `turn15.generate_world` therefore reimplements the generator,
  keeping turn13's law verbatim — same component draws, same cycles, same 48/12
  schedule, same 8% noise rate, same `commands-unrelated` stream, same
  `alphabet-<body>` and `emit-<body>` seeds with `recipient` as the body for
  `recombined` — and dropping only the `switched` command tape and its assert.
  The emission itself goes through `turn13.emit`, so the bytes come off the
  frozen binary through frozen code.

- Everything else is called, not copied: `turn13.seed`, `turn13.emit`,
  `turn13.save`, `turn13.manifest`, `turn13.extract_world`,
  `turn13.learn_world`, `turn13.run_target`, and `turn14.replay_fast` with
  `turn14.Outer` for the paired authority replay. On the reader side,
  `turn13/verify.py`'s `verify_books` and `verify_life` and
  `turn14/verify.py`'s `MassOuter`, `verify_fast` and `compare` are used as
  they stand.

Since `moved_whole` and `moved_mid` are ordinary `*.bin` files in the world
folder, `turn13.run_target` drives the C predictor over them with no change.

## 4. Module loading under private names

`turn14/experiment.py` and `turn14/verify.py` each load turn13 under the fixed
`sys.modules` names `frozen_turn13_experiment` / `independent_turn13_reader` and
then mutate that module's `HERE`, `WORLDS` and `NAMESPACE` to turn14's values.
If turn15 used the same names, the two configurations would fight over one
entry. Turn 15 therefore loads its own turn13 handle under
`turn15_frozen_turn13_experiment` / `turn15_independent_turn13_reader`, and
turn14 under `turn15_frozen_turn14_experiment` /
`turn15_independent_turn14_reader`. Each module object keeps its own
configuration; turn14's inner turn13 handle stays pointed at `turn14/` and is
never used for filesystem work (`replay_fast` and `verify_fast` touch only
`HERE`, which turn15 overrides, and `compare_tree`, which is pure).

## 5. Raw samples: turn14's sampler does not fire here

`turn14.replay_fast` collects its help/harm examples only under
`regime == 'switched'` (turn14/experiment.py:154), a literal that turn15 cannot
change without editing a frozen file. Under turn15's regimes it returns
`{'help': None, 'harm': None}`, which is discarded. Raw samples are instead
collected in a separate pass, `raw_scan`, over the two retained traces
(`results/world*/moved_mid.tsv.gz` and `results/outer/world*/moved_mid.tsv.gz`),
restricted to `t >= 8192`. `verify.py` has its own `raw_scan` and the recovered
samples and extrema are compared field by field.

The sample's "helps / harms" measure is `received = fast_live - logcold`: what
the admitted candidate actually bought at that byte under the fast law. Turn 14
instead compared fast against slow; turn15's question is whether memory helps
or harms after the move, so the comparison is against cold.

## 6. "Received gain minus cold" (condition 3)

In this machinery `gain` is already defined as the running sum of
`live - cold` (turn13/experiment.py:419, turn14/experiment.py:81). So the
condition 3 quantity, "episode changed-tail received gain minus cold", is the
episode arm's `tail` field read directly — no second subtraction. This is a
different tooth from turn14's, where the changed-regime comparison was episode
minus the `row` arm; the turn15 protocol says "minus cold", and cold is what
`gain` is already measured against. The `row` arm is still reported in full in
every table.

Condition 4, "at least 0.005 bit/raw-byte", is implemented as
`mean_over_worlds(gain)/16384 >= 0.005`, matching turn13's `full_gain` idiom
(turn13/experiment.py:448).

## 7. "Above -1-1e-7" implemented as `>=`

Condition 6 says the complete-prefix gain must be "above -1-1e-7". Implemented
as `minimum >= -1-1e-7`, matching the inherited assertions it mirrors —
turn13/experiment.py:432 and turn13/verify.py:494 both use `>=`. A value landing
exactly on the bound would pass here and pass there; using `>` would make
turn15 stricter than the machinery whose bound it is quoting.

## 8. Equivariance admission is exact equality, including the never-admitted case

Condition 2 asks that "admission bytes match exactly between the two regimes
for every arm that admits". Implemented as exact equality of the activation
byte across `moved_whole` and `recombined`, which also requires that an arm
admitting in one regime and not the other counts as a failure, and that two
never-admitting arms (`None == None`) count as a match. This is the stronger
reading; the weaker one would let an arm admit under renaming and stay silent
without renaming.

## 9. Normalization maximum kept out of the compared summary

The writer records the C predictor's own `max_norm_error` column maximum per
life (`RESULT.json:norms`). The reader's normalization figure is the maximum of
that same column **and** its own reconstructed candidate normalizations
(turn13/verify.py:684-710), so it is greater than or equal to the writer's and
the two cannot be compared for equality in general. `worst_norm_error` was left
out of `summary.bounds`, which the reader compares element by element; instead
each hand computes `c6_normalized_new_exact` from its own measurement, and the
reader additionally bounds every value in the writer's `norms` by 1e-8. Both
hands must still agree on the boolean, which they do.

Observed in this batch the two coincide: the writer's maximum over 32 lives is
1.3322676295501878e-14 and the reader's `maximum_normalization_error` is the
same value, so here the C column dominates the reader's own reconstructions.
That is a measurement, not a guarantee — the inequality is what the structure
gives. Both are against a 1e-8 bar.

## 10. Gate test names

Sixteen named conditions, prefixed by the protocol condition they implement:
`c1_*` (four: prefix identity and renaming identity, mid divergence, whole
divergence and renaming identity, bijection), `c2_*` (three: full-life,
4096-horizon, admission), `c3_*` (two: mean, count), `c4_*` (two: rate,
positivity), `c5_*` (one), `c6_*` (four: prefix bound, fast drawdown, slow
drawdown, normalization and protected NEW). `gate_pass` is their conjunction
with the five inherited identity claims. Condition 7 is not a `tests` entry —
it is `VERIFY.json` itself.

## 11. FREEZE pins the regime list too

`FREEZE.json` carries `regimes` alongside `namespace` and `worlds`, and both
`check_freeze` and the reader assert it. Turn 14 pinned only namespace and
worlds; since turn15's whole question is which regimes exist, the regime tuple
belongs in the freeze.

## 12. Pre-freeze probes

Before `freeze` ran and before any byte of worlds 160..167 existed, a probe
suite was run on synthetic fixtures in the scratchpad (not in `turn15/`, and
not a deliverable): 47 checks, rc 0. It established that both hands' `surface_map`
agree for all eight worlds and are namespace-bound; that both hands'
`summarize` produce trees that pass `compare` in both directions, which is the
structural precondition for the later verification; that both hands'
`manipulation` and `raw_scan` agree on real byte arrays and real trace files;
and — the part that matters — that **every one of the sixteen teeth goes red
when broken**: a 1e-6 equivariance leak, a moved prefix, an identity renaming, a
thinned divergence, a non-bijection, a thinned recovery tail, a 4/8 count, a
sub-rate whole life, one negative world, a single permuted admission, a prefix
gain below -1, a drawdown over 10 and over 16, and a 1e-7 normalization. A tooth
that cannot fail is decoration. One green counter-probe confirmed the
equivariance tooth survives a 1e-9 wobble, so it is not merely always-false.

## 13. Order of operations, and what was irreversible

`make` (rc 0) -> probes (rc 0) -> `freeze` (rc 0) -> `generate` (rc 0) ->
`extract` (rc 0) -> `learn` (rc 0) -> `evaluate` (rc 0) -> `verify.py` (rc 0).
`freeze` ran with no `data/` or `results/` directory present, verified by `ls`
in the same command. Once `generate` completed, nothing was regenerated,
retried or tuned: there was one run of each stage and one gate. `RESULT.json`,
`FREEZE.json`, the manifests and `VERIFY.json` are all written with `open('x')`,
so none of them can be silently replaced; `evaluate` additionally refuses to
start if `results/` already exists, and `verify.py` refuses if its output file
exists.

Two edits were made to `experiment.py` after it was first written and **before**
the freeze: moving `worst_norm_error` out of `summary.bounds` (note 9), and
changing a bare `next(saved)` in `raw_scan` to `next(saved, None)` with an
explicit assert so an exhausted trace fails with a readable message. Both are in
the frozen digest `71c35aca0a58bf5f0e8438bc18232448af926827c887a37b52acdc974a08f418`.
Nothing was edited after the freeze.

## 14. The binary

`turn15/Makefile` is a byte-identical copy of `turn14/Makefile` (`diff` rc 0);
it builds `episode` from the frozen `turn13/episode.c` plus the frozen frontend
and recurrence sources. The resulting binary hashes to
`784d549d01f04bc02b3ae64a8ee5a2e69079ace3c9b085bd5076ef8581fb5b10`, which is the
value pinned for `turn14/episode` in `turn14/FREEZE.json`. The build is
reproducible across turns on this machine; all thirteen files pinned by turn14
were confirmed unchanged before any work began.

## 15. What the run found

Equivariance did not merely pass within tolerance — it held **exactly**. The
worst full-life difference between `moved_whole` and `recombined` is 0.0 bits,
and the worst 4096-horizon difference is 0.0 bits, across all seven arms, both
authority laws and all eight worlds, with every admission byte identical. The
protocol predicted equivariance from the merge law's `(count, first position,
key)` ordering; the measurement is bit-level identity, which is stronger than
the 1e-7 the gate asked for.

The recovery tooth passed and is the interesting one. The `moved_mid` prefix
horizon difference against `recombined` is exactly 0.0 in every world, so the
whole effect is the seam. Across the seam the fast episode changed tail falls
from a mean of 1591.303 bits (`recombined`) to 592.793 bits, positive in 8 of 8,
against a gate asking for +1 bit mean and 5 of 8. World 163 is the thin case:
1.691 bits of recovery under the fast law against 16.305 under the slow one —
recovery happened, barely. `unrelated` admits nothing in any arm and receives
exactly zero, and `permuted` is never admitted in any regime, life or mode.

## 16. Not this turn's business

No commit, push, merge, branch or live integration. `turn15/RESULT.json` still
carries `independent_reader_pending: true`, which is turn14's convention: the
writer's file records that it was written before the reader ran, and
`VERIFY.json` is the reader's own word. Canonical netta, the living organism,
mouth, mycelium and every sealed world range were not touched.
