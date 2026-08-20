# nettalog2.md

A durable technical record of what Netta becomes, why each organ is added,
what the falsifier returned, and what must remain uncertain.

## Research law

A code isn't accepted because it is beautiful.

1. Every major organ needs a matched control.
2. Seeds, source positions and compute budgets are held equal.
3. Internal scores are insufficient; held-out structure and attractors are
   measured too.
4. Source truth and counterfactual experience remain distinguishable.
5. Negative experience is preserved.
6. A null result is a result.
7. A new layer must earn authority before it can alter the organism.
8. A measurement must measure what it names, against the original order,
   not against the layer above it.

## Zero

The previous organism is invalid in full. Its code, state, organs, islands,
metrics, positive results and nulls are a forensic record of a failed
prototype and a measurement-process failure, kept in git history below
commit 5ea1374 and claiming nothing about what follows. Nothing is
inherited: no token identities, no source graph, no embeddings, no debt
values, no glyphs, no chains, no thresholds, no baselines, no roadmap.

The root cause on record: the organism was built and measured for a week on
a word substrate while the standing order specified a byte floor with an
earned vocabulary. The order was written, never implemented, and never
checked against. Law 8 above exists because of this.

## Ontology of the new body

The world is an immutable byte tape. An island is a byte array, its length,
a content digest and an identity; no whitespace, case, Unicode or
punctuation policy belongs to source truth. The only canonical address is
(island, byte offset). A byte is the irreducible action; all 256 byte
values are legal from birth. An earned action, when such a thing is later
admitted, is a byte sequence that coexists with its atomic expansion and
never retokenizes the world. All judgment, reward and debt are denominated
in raw bytes. Chains, if later earned, are semi-Markov over
variable-duration actions. Every claim is scored in bits per raw byte;
token-denominated metrics are illegal across differing action alphabets.

## First code boundary

The first body contains only: length-safe island loading and hashing;
immutable byte positions; 256 atomic actions; a minimal externally judged
next-byte game; deterministic RNG; an immutable biography; restart-safe
state; and the Z0-Z2 tests. It contains no tokenizer, no BPE, no
embeddings, no words, no punctuation rules, no dreaming, no recursion, no
chains, no learned macro-actions, no island transfer, no neural core. The
body must first prove that it can see, act, be judged, remember, and
restart without changing what one unit of language means.

Gates ahead of the body, declared before code: Z0 (empty-body provenance:
no copied code, new state magic, old state unloadable, build identifies as
NETTA ZERO), Z1 (byte world: all 256 values round-trip including NUL,
binary and UTF-8 islands round-trip exactly, digest and length survive
restart, no cross-island context, length-based loading throughout), Z2
(atomic game: only one-byte actions, a fixed context yields a normalized
distribution over legal actions, external truth alone computes loss, same
seed gives byte-identical biography, a direct life equals a split life
across restart, results reported in bits per raw byte).

## The first body

339 lines of C, one file, written from an empty buffer. World: islands as
immutable byte arrays with FNV-1a-64 digests, verified against the
external test vector ("a" -> af63dc4c8601ec8c). Address: (island, byte
offset); the 16-byte observable context never crosses an island edge
because islands are never concatenated. Actions: the 256 single bytes.
Policy: Laplace-smoothed lived byte frequencies; the distribution is
checked to sum to 1 on every step and the process dies loudly if it does
not. Judge: external truth alone, loss = -log2 P(truth byte), learning
strictly after the receipt. Biography: append-only TSV, one receipt per
step (episode, step, island, offset, context digest, action, truth, loss
bits, RNG state, kind, duration), hash-chained; the chain and line count
persist in state. State: magic NETTAZR0 v1, actual lengths throughout;
any foreign or truncated file is refused loudly, and a snapshot refuses a
world whose island digests or lengths differ from the life it recorded.
RNG: splitmix64, seeded from the command line, persisted.

Gates taken (zero_tests.sh, the pre-commit law of this repository —
fourteen machine verdicts, ALL GATES PASS, rc=0): strict build with zero
warnings; the build names itself NETTA ZERO; a foreign state file is
refused; the FNV vector matches; all 256 byte values including NUL
round-trip in a 256-byte island; digests are deterministic across runs; a
UTF-8 island measures 22 raw bytes, not characters; a newborn's first
step costs exactly 8.000000 bits and later steps do not (learning is
alive inside a single life — 64 steps on a 3403-byte island, seed 42,
mean 6.933083 bits per raw byte); the same seed reproduces biography and
state bit-for-bit; a different seed diverges, proving the comparison can
fail; a life of two episodes equals one episode, a restart, and a second
episode, biography and state identical to the byte; an island too small
for the requested walk fails loudly. Address sanitizer and undefined
behaviour sanitizer report nothing on a text island and on the full
256-value binary island.

What this body does not know yet, by decree: sequences, words, macros,
chains, dreams, transfer. It knows how to see bytes, act on them, be
judged by them, remember exactly, and come back from a restart as the
same life. The floor is load-bearing; the vocabulary will be earned on
top of it, not instead of it.

## The second body: an earned vocabulary that cannot cheat

The unit law was declared before the code. Netta always samples atomic
bytes: a unit peeks at no future and receives no bit of discount. A unit
is recognition over the lived truth tape -- when consecutive lived bytes
equal a living unit under greedy longest match, one additional macro
line enters the biography; atomic lines are never altered, replaced or
buffered. Growth is BPE over lived moves: adjacent-move pairs are
counted on the segmented tape, a pair lived 64 times births the
concatenated unit if its bytes are exact, its length is at most 16, and
it contains no whitespace byte (the first-version boundary discipline).
Unit identity is its bytes -- two birth paths cannot create two units.
Pairs never span an episode boundary, so no false adjacency crosses a
seam. Births and macro events are biography lines of their own kind and
persist with the pair counts in state (version 2; version 1 files are
refused loudly). All judgment stays in raw bytes.

Because the counted pairs live on the segmented tape, growth is
iterative: on a 13200-byte island of one repeated sentence (seed 42,
4000 lived steps) the first births are two-byte -- "at", "th", "he",
"an" -- and the fifth is the three-byte "the", a composite of a unit and
a byte. Fourteen units were born; 642 macro events covered 1445 lived
bytes; decisions per lived byte 0.7993 against exactly 1.0000 on the
anti-repeat control (the 256-value island, where nothing repeats and
nothing is born). The economy is real and the prediction is untouched:
with units disabled the atomic subset of the biography is bit-identical
to the full run's atomic lines, so the layer provably cannot move the
game it watches.

Gates taken (zero_tests.sh, twenty-two machine verdicts, ALL GATES
PASS, rc=0): the fourteen gates of the first body unchanged, plus:
units are born from lived repetition; the anti-repeat control births
nothing; the no-op subset is identical; births and pair counts survive
a restart bit-for-bit (a split life with births equals a direct life);
no duplicate unit identity exists; decisions per lived byte is below 1
where repetition exists and exactly 1 where it does not. ASan and UBSan
are silent on the repeated, binary, and text islands.

What the units still are not, on purpose: they do not predict, do not
vote, do not act, and cannot be chosen. They are named roads over lived
ground. The right to act -- to let a unit stand as a chooseable move
with its own priors -- is a separate organ with its own falsifier, and
it will have to earn that voice against a matched control, like
everything else in this line.

## The third body: the units learn to predict, in shadow

The law, declared before code: a second model of the world appears -- a
semi-Markov unigram over the moves of the segmented lived tape (the
unit-LM). It lives in shadow: game, judge, and atomic biography lines
are untouched to the byte (the B2 no-op gate still holds). At every
move boundary the shadow pays a prequential price, -log2 P(move),
strictly before the move's count is updated; macro biography lines
carry their move's price as a new field. The ruler is unchanged: bits
per raw byte, with the canonical segmentation being the same greedy
longest match that cuts the tape. The alphabet is nonstationary by law:
births widen the Laplace denominator. Built into the design is its own
red twin: with no living units the segmentation is trivial, move counts
equal byte counts, and the unit-LM must be identical to the atomic
model -- not approximately, identically.

Measured: on the anti-repeat island the twin holds exactly (unit-LM
8.454211, atomic 8.454211 -- equal to the printed digit). A scratch red
run that breaks the prequential order (pricing as if the count were
updated first) shifts the number to 7.454211, so the identity gate
genuinely detects the one dishonesty it was written against. On the
repeated island the units earn their first predictive signal: unit-LM
3.388430 bits per raw byte against atomic 3.784616 on the same 4000
lived bytes -- the earned vocabulary prices the world 0.396 bits per
byte cheaper than bytes alone, under the same judge, with no discount
anywhere: every macro is priced at a boundary it must reach honestly.

Gates: twenty-five machine verdicts, ALL GATES PASS, rc=0 -- the
twenty-two of the second body unchanged, plus the prequential twin
identity, the predictive win on repetition (a FAIL here would have been
recorded as a null), and shadow counters surviving a restart
bit-for-bit inside state version 3. ASan and UBSan silent on the
repeated and binary islands.

The right to act is still not granted. The shadow has shown it can
price the world better where the world repeats; whether that shadow may
choose -- stand as an actuator inside the game -- is the fourth body's
question, and it will be asked against a matched control with the
context-bearing oracle (the postgpt-lineage organ: contextual counts
over bytes and moves) entering the same court.

## The fourth body: two oracles enter the court

The law, declared before code: two context-bearing shadow oracles of
the postgpt lineage, same ruler, prequential. The byte bigram
conditions on the previous world byte -- the context comes from the
tape, not from the episode, so no seam can exist across episodes by
construction. The move bigram conditions on the previous move of the
segmented tape, and its statistics are the nursery's own pair counts:
the oracle and the vocabulary growth are one tissue; the organ adds
only an outgoing total per move and pays its price strictly before
pair_feed updates the pair. Game, judge, and atomic biography stay
untouched to the byte. Four models now stand in one report under one
ruler: atomic-uni, unit-uni, byte-bi, move-bi.

Measured on the separating worlds. On the alternating island (ab
repeated 2000 times): atomic-uni 1.568536, byte-bi 0.908803 -- context
beats contextlessness by 0.66 bits where structure is purely
sequential; unit-uni 0.794432 and move-bi 0.695269 go further still,
because the born units make the tape itself predictable. On the
constant island the twins converge exactly: byte-bi 0.571440 equals
atomic-uni 0.571440 to the printed digit -- with one context the bigram
row is the unigram, and the identity is the gate. A scratch red run
with the prequential order broken shifts byte-bi to 0.565957, so the
convergence gate detects the dishonesty it was written against. And an
honest surprise the constant world exposed: unit-uni there is worse
than atomic (0.654687 against 0.571440) -- births widen the alphabet
and the vocabulary pays for its own existence where there is nothing
left to predict. Anti-fetishism, measured in bits.

On the repeated-sentence island the ladder of models stands in full:
atomic-uni 3.784616 > unit-uni 3.388430 > move-bi 3.073982 > byte-bi
2.935410. Context wins, both oracles are alive, and the earned
vocabulary helps most exactly where its units mean something.

Gates: twenty-nine machine verdicts, ALL GATES PASS, rc=0 -- the
twenty-five of the third body unchanged, plus the ab-world separation
(gap >= 0.5 declared before the run), the constant-world twin identity,
the move-context win on repetition, and oracle counters surviving a
restart bit-for-bit in state version 4. ASan and UBSan silent on the
alternating, constant, repeated, and full-binary islands.

Still no one may act but the newborn policy. The court now holds four
priced witnesses; the fifth body decides who among them earns the right
to choose -- and that right will be earned per the research law, against
a matched control, with the loser's testimony kept.

## The fifth body: the right to act is earned

The law, declared before code: the actuator of the game becomes earned.
Candidates are the byte models only -- the newborn atomic-uni, acting
since birth, and byte-bi, the court's best witness; move-level models
are not admitted as actors in this body. The right is granted by the
lived prequential record alone: byte-bi acts when its cumulative lived
bits per byte beats atomic by at least 0.1 after at least 1000 lived
bytes, and steps down if the lead falls under 0.05. Elections happen
only at episode boundaries and are biography events of their own kind.
The judge prices the acting model in the receipt; the shadow prices of
all four models remain prequential and untouched by who acts, so the
record that grants the right can never be polluted by the right itself.
Where the right is not earned, intervention must be exactly zero.

Measured. On the alternating world (six episodes of 600 steps, seed 5)
the right is earned mid-life: episodes one and two are played by the
newborn, episodes three through six by byte-bi, and the life with the
earned actor costs 0.800969 bits per byte in its receipts against
1.373074 for the same life locked to the newborn -- same seed, same
world, 0.57 bits per byte returned by an earned right. The shadow
prices printed by both lives are identical to the digit, proving the
election record is actor-independent. On the full-binary world the
right is never earned and the earned-mechanism biography is
bit-identical to the locked-newborn biography: zero intervention
without the record. Elections survive a restart bit-for-bit in state
version 5.

One declared prediction was refuted by the measurement, and the
refutation is kept as the finding it is. The author predicted that
forcing byte-bi on the structureless world would be measurably worse
than the newborn. The fact: forced byte-bi scores exactly 8.000000
bits per byte -- its rows stay virgin on a world without repetition,
so it answers with honest uniform ignorance -- while the newborn
scores 8.454211, because it has learned frequencies of a world that
never repeats them. On a non-repeating world, learned confidence is
self-deception and ignorance is the better witness. The suite pins the
measured fact (locked-bi exactly 8.0 on virgin rows), not the wished
sign.

Gates: thirty-four machine verdicts, ALL GATES PASS, rc=0 -- the
twenty-nine of the fourth body (two parsers tightened to skip election
lines), plus: the right is earned mid-life; the earned actor lives
cheaper than the locked newborn; zero intervention where the right is
not earned; elections survive restart; forced ignorance stays exactly
uniform. ASan and UBSan silent on four worlds.

For the first time in this line, a lived record -- not age, not
existence, not beauty -- moved authority from one organ to another, in
the open, with the losing configuration's testimony preserved in the
same report.

## The sixth body: the trigram floor, and power that skips a rung

The law, declared before code: the oracle ladder gains its trigram
floor -- P(byte | two previous tape bytes), contexts as byte pairs with
a direct row-total array and the (context, byte) counts in an
open-addressed table that fails loudly when full. Prequential, same
ruler, and a third candidate for the seat. The election generalises
from a duel to a law of succession: any candidate with at least 1000
lived bytes and a 0.1 lead over the newborn is eligible; the strongest
eligible challenger takes the seat if it leads the sitting actor by the
hysteresis margin; a sitting actor that loses its mandate over the
newborn vacates. Elections stay at episode boundaries, as biography
events. Zero intervention without an earned record must survive the
widening of the field.

Measured on the period-3 world (abcacb repeated 700 times): the bigram
is structurally blind there -- after 'a' the next byte is genuinely
ambiguous in its one-byte context -- and prices the world at 1.803026,
while the trigram, whose two-byte contexts make every continuation
deterministic, prices it at 1.244322: a 0.56-bit gap exactly where the
theory of the world says it must appear. The seat passes uni -> tri at
the third episode, skipping bi entirely -- the succession law hands
power to the strongest eligible challenger, not to the next rung of a
hierarchy. The full ladder of power, lived on one seed: earned
1.296476 < locked-bi 1.803026 < locked-uni 1.956410. On the
alternating world the trigram collapses onto the bigram exactly --
0.614734 equals 0.614734 to the printed digit, because there the
two-byte contexts are in bijection with the one-byte contexts: the
extra floor adds nothing where there is nothing to add, and the
equality is the gate. Zero intervention on the structureless world
holds bit-for-bit with three candidates in the field. Elections and
trigram counters survive restart in state version 6.

Gates: forty machine verdicts, ALL GATES PASS, rc=0. ASan and UBSan
silent on five worlds: period-3, alternating, constant, repeated
sentence, and full binary.

The ladder now stands as ordered: context beats frequency, and deeper
context beats shallower exactly where the world's structure lives
deeper than one byte of memory. The next question belongs to travel:
a second island, and what of all this survives the crossing.

## The seventh body: the second island, and what survives the crossing

The law, declared before code: the transfer body adds not one persisted
structure to the organism -- it is a court, not an organ. State stays at
version 6; the counters remain global because they are biography and
biography travels with the traveller; the worlds remain immutable and
their immutability becomes an explicit gate. What is added is honest
optics only: this-life slices of all five models (the price of this
stretch of life, not the cumulative), and a census of forms (how many
living units find their exact bytes on a given island). The court's
worlds: island A, "the cat sat on the mat and the dog ran off"; island
B, "the dog sat on the log and the cat ran off" -- shared words, alien
order. Controls: a shuffled B (same bytes, murdered structure) and an
alien donor C (the period-3 world). Three predictions were declared
before the measurement: kin experience transfers; shuffling kills the
transfer; kinship out-transfers alienage.

All three held, and the numbers are loud. A traveller with 3200 lived
bytes of A, on an 1600-byte budget of B, against a newborn on the same
budget and seed: atomic-uni 3.672418 vs 4.134162; byte-bi 2.223834 vs
3.830080; byte-tri 2.142641 vs 3.923884; unit-uni 2.737994 vs 4.115180;
move-bi 2.395819 vs 3.977704. Kin experience is worth 0.46 bits per
byte to the frequency model and 1.61 to 1.78 bits per byte to the
context models -- the transfer grows with the depth of context, which is
the byte-level thesis in one line: what crosses between worlds is
structure, and the deeper the structure, the more of it crosses. All
fifteen living units find their exact bytes on the kin island: the
earned vocabulary arrives recognisable.

The controls cut cleanly. On the shuffled island the 1.6-bit transfer
collapses to noise (byte-bi 5.684552 travelled vs 5.694420 newborn, a
0.01 gap), so what transferred was structure, not an alphabet. The
alien donor C arrives on B worse than a newborn on the frequency
channel (5.776488 vs 4.134162) -- unrelated experience does not merely
transfer less, it harms: alien frequencies are worse than ignorance,
the second appearance of the self-deception law found in the fifth
body. And the return is priced: after the voyage A->B->A the third
stretch at home costs 1.914068/1.565895 (bi/tri) against
1.897624/1.510803 for a stay-at-home life of equal total budget -- the
voyage taxed the home rows by 0.02 to 0.06 bits per byte, a measured
toll, not the catastrophe the old line feared, and not free either.

Gates: forty-five machine verdicts, ALL GATES PASS, rc=0 -- the forty
of the sixth body (four test anchors tightened after the census line
collided with a lazy grep), plus: kin transfer with a declared 0.5-bit
gap on both context channels; shuffled-world collapse under 0.1;
kinship over alienage by 0.5; island digests identical across all
lives; exact forms recognised on the kin island. ASan and UBSan silent
on the two-island voyage.

What stands after seven bodies: a world that cannot be rewritten, a
biography that cannot be edited, a vocabulary that pays for itself, a
court of five priced witnesses, power that is earned and revocable,
and now -- measured, controlled, and cheap to reproduce -- the fact the
whole line exists for: experience of one world makes another world
legible, in proportion to how deeply their structures rhyme.

## The eighth body: the emission seat, a regression, and probation

The law, declared before code: a fourth candidate for the seat -- the
semi-Markov move-player, which emits whole moves, bytes or earned
units, priced by the move bigram whose statistics are the nursery's
own pair counts. The environment advances by the matched prefix of the
emitted move, never less than one byte, so a wrong long move is never
cheaper than the same wrong bytes: the price is paid in full and the
advance shrinks. Every lived truth byte still flows through every
per-byte organ, blind to which move lived it. Move receipts are
biography lines of their own kind.

The first draft of the law seated mv on its shadow record and the
regression gates caught the mistake before any commit: earned lives
collapsed (0.800969 became 2.532679 on the alternating world, 1.296476
became 4.359787 on period-3). The finding is constitutional and is
kept: **the record of pricing moves does not transfer to the right to
emit them.** The shadow prices teacher-forced segmentations of lived
truth; the player generates, and generation errors pay full move price
for one byte of advance. A right must be earned in the discipline in
which it will be exercised -- the same category law the old line
learned as "the metric must measure the actuator," now rediscovered by
the organism's own gates.

The corrected constitution: the shadow record only opens probation --
a rare, deterministic probation episode (every eighth, while the
played mandate is not yet earned) in which mv actually plays; only the
record played there can win the seat. Measured on a 24-episode
period-3 life: the seat passes uni to tri at episode three; probation
fires exactly twice (episodes 7 and 15, 380 real move receipts); mv
earns its mandate minimum of 1200 played bytes -- and the verdict is
numeric and against it: mv played 1.557730 bits per byte against the
sitting trigram's 0.502630, so the seat stays with tri and mv's claim
is refused by the very ruler that granted tri its power. The right to
emit exists, was tried in the open, and lost on the record -- which is
the system working, not failing. With the corrected law the earned
lives returned to their exact former numbers (0.800969, 1.296476), and
zero intervention on the structureless world holds bit-for-bit with
four candidates and probation in the field.

Gates: fifty-one machine verdicts, ALL GATES PASS, rc=0 -- the
forty-five of the seventh body restored exactly, plus: probation opens
from the shadow record; probation emits real moves; the verdict is
numeric; the seat follows the played record; zero intervention
survives the widened field; probation and the played record survive
restart in state version 7. ASan and UBSan silent on five worlds and
the probation life. A red prequential run shifts the played record
(1.403676 against 1.557730), so the record's honesty is itself
guarded.

The seat of emission is now a real institution with a losing first
claimant. Somewhere on a world whose structure lives in whole moves
rather than in two bytes of memory, a future mv will win it -- and
when it does, the win will mean something, because this one was
allowed to lose.

## Checkpoint correction: the judge looks outward

The eight-body checkpoint `c32843f` was taken into an independent tree and
its fifty-one gates first reproduced without a changed byte. Three red
probes then broke three claims at the foundation rather than at the model
surface.

First, the played move record was not a judgment of the world. It wrote
`-log2 P(emitted move)`: the self-information of the move Netta sampled. In
a paired counterfactual, two organisms with identical a-only biographies
emitted the same byte under the same prior into two unread islands, one
a-only and one b-only. The old court wrote the same `4.177085` bits in both
worlds, although the move was truth in one and false in the other. A
confident lie and a confident truth were literally the same evidence.

The correction keeps emission causal -- the sampled move still determines
how far the world advances -- but makes the external judge price the
canonical truth move at the current byte address. Canonical here means the
same greedy longest exact-form segmentation already used by the shadow
move model; the actor cannot see that target before acting. The paired
losses are now `0.816909` on the kin truth and `10.243174` on the alien
truth. The first outward retake still contained a subtler teacher leak:
the player conditioned its next emission on the truth matcher's previous
move instead of its own previous emission. Removing that hand from the
trajectory produces the final period-3 verdict below. The earlier
`1.557730` played figure and the intermediate `2.053519` retake are invalid,
while the preserved story of a losing claimant remains true.

The event base of the election was broken too. Two early probation episodes
reached the 1200-byte minimum, a losing record then stopped probation forever,
and that frozen record was compared with lifetime byte models measured on
other episodes. Probation is now rare but recurrent; every byte it lives also
records uni, bi and tri counterfactual prices at that exact address. Move
authority must beat the best byte witness on those matched bytes. On the
24-episode period-3 life probation now opens three times and emits 1596 real
moves. The causal player loses `7.057066` against a best matched byte record
of `0.288818`. A search over five deliberately move-structured worlds, up to
128 episodes each, found shadow move prices as low as `0.095302` while played
prices remained `6.07` to `9.30`: recognition is not yet free-running life.
The refusal branch and the right to reapply are proved; appointment is not.
Forced `--actor-lock mv` lives now report a separate this-run control record
which is never persisted as mandate evidence; a counterfactual cannot later
present itself as earned biography. State advances to version 9 so neither
self-priced evidence nor unmatched probation records can silently enter the
corrected court.

Second, the biography chain had not been connected to the biography file.
State persisted a chain head and line count, but resume never recomputed
either from disk: a missing biography was silently recreated under the old
head and returned `rc=0`. Resume now streams and verifies the whole external
biography before opening it for append. Missing, truncated, edited, or
unterminated histories fail closed. The inverse orphan is guarded too: if
state is missing while a biography exists, implicit rebirth refuses rather
than truncating the surviving record; only explicit `--reset` may do that.
State publication now writes a uniquely created sibling and renames only
after a complete close, so an interrupted save does not first truncate the
last valid snapshot. Loaded state also rejects
trailing bytes, impossible actors or units, duplicate/foreign pair keys,
non-finite records, and disagreement among byte, move, row, episode, and
model totals.

Third, numeric syntax was not an input boundary. `--steps -1` -- still
advertised by the dead README -- became `UINT64_MAX`; the tape-length guard
wrapped, and ASan caught a real heap-buffer-overflow beyond the island.
All numeric flags are now parsed exactly and address arithmetic is ordered
to make overflow impossible. State, biography, and islands must be distinct
files, including hard-link or symlink aliases, so a requested output cannot
rewrite source truth. The dead word-level README, model card, generation
examples, calibration prose, and old probe harness have been removed from
the living tree; README now describes only NETTA ZERO.

The transfer court was retaken with one more matched variable: `--start 16`
holds traveller and newborn at byte-identical source positions. Kin transfer
survives: byte-bi `2.227203` travelled against `3.828710` newborn, and
byte-tri `2.147926` against `3.921547` (advantages `1.601507` and `1.773621`
bits per byte). Kin still beats the alien donor (`2.227203` against
`3.950850`). But the old shuffled-null did not survive its own wording.
On shuffled B the travelled/newborn atomic prices are
`3.693113/4.147372`, while byte-bi is `5.608769/5.748157`: total bi transfer
is `0.139388`, not the claimed `<0.1` null. The correct controlled question
subtracts frequency transfer. Contextual excess is
`(5.748157-5.608769) - (4.147372-3.693113) = -0.314871` bits per byte.
Shuffling kills context beyond frequency; it cannot and should not erase the
transfer of the byte census it preserves. The former null remains above as
the dirty measurement that led to this correction.

The amended law has sixty-two machine verdicts. The new verdicts pin
finite address syntax, external biography continuity, source/output
separation, exact state extent, matched inter-island positions, and the
external-truth move judge; strict ASan/UBSan builds and representative lives
are now executable gates rather than remembered side runs. The original
fifty-one remain green under the corrected court, with superseded played
numbers changed openly rather than forced back into place.

What is not proved yet: the election gate has observed a move claimant lose
and reapply, not win; closing the measured exposure gap likely needs search or
a state organ rather than a kinder threshold. Global cumulative authority may
carry a byte actor into an alien island long after its local mandate has
vanished; greedy truth segmentation is a declared first court, not yet a
marginal likelihood over all byte-equivalent move paths; units have a birth
law but no death law. Those are open bodies, not implications of this repair.

## The ninth body: navigation earns the first move mandate

The wager was fixed against the corrected refusal, not against the earlier
self-priced player. No threshold or court changed. With `--no-mv-nav`, the
period-3 red arm remains exactly where the checkpoint left it: move play costs
`7.057066` bits per raw byte against the best matched byte witness at
`0.288818`, three probation episodes reopen, and no ordinary `mv` episode is
seated. The ninth body had to close that exposure gap by changing what the
player can know before emission, not by forgiving what happens afterward.

The first navigation organ has two bounded operations. At every move decision
it searches all exact semi-Markov segmentations of the previous 16 observable
world bytes. A Viterbi route, scored by the lived move unigram and bigram,
selects the most probable final move as an anchor. Atomic moves guarantee an
exact route even when no learned unit fits. From that anchor, a one-move
model-only rollout scores each candidate as its current transition probability
times the strongest learned continuation available after it; those scores are
renormalized into the policy that actually samples. The search may inspect
unit forms, counts, and bytes strictly before the current address. It never
reads the target byte, tests a candidate against its future world span, or
consults the truth matcher. The external judge prices the resulting searched
distribution against the same canonical truth move as before, so a confident
internal attractor is still an expensive lie when the world rejects it.

On the preregistered 24-episode period-3 life, seed 5, the searched player opens
two probation episodes and then earns nine ordinary `mv` episodes. Its played
record is `0.049384` bits per raw byte over 6600 matched bytes, against
`1.622828 / 1.110387 / 0.212070` for uni, bi, and tri on those same bytes. The
margin over the best byte witness is `0.162686`, above the unchanged 0.1
appointment threshold. In the whole life, 590 move decisions were searched;
571 routes ended in learned-unit anchors. Ten seeds all repeated the positive
appointment (`0.027413..0.055753` played versus `0.212070` tri). A 64-episode
side life kept the seat for 49 ordinary `mv` episodes and finished at
`0.040604` played against `0.114199` matched tri. This is the first earned
emission mandate in the zero line.

The positive gate has three red edges. First, disabling navigation reproduces
the exact `7.057066 / 0.288818` loss and refusal rather than merely making the
new player a little worse. Second, two unread islands with the same 16-byte
prefix and different target spans produce the same route anchor and emitted
move; only the outward target and loss differ. Third, a deterministic
random-order three-byte-census world grows 49 units but gives the move shadow
no predictive advantage, so search receives neither probation nor mandate.
The new player and the unanchored control both survive split restart exactly.
Searched move receipts now carry the policy anchor, while the no-navigation
red arm preserves the checkpoint receipt bytes. State advances to version 10
so the new emission semantics cannot be mistaken for the version-9 life.

Gates: sixty-eight machine verdicts, `ALL GATES PASS`, strict C11 with zero
warnings, ASan/UBSan through searched probation, appointment, and restart. The
six new B9 verdicts prove a matched win, a real seat, the old refusal, the
causal observation boundary, restart identity, and the random-order null.

What is not proved: this is bounded route recovery plus one move of lookahead,
not yet the GTA/AlphaZero-scale planning body. Its appointment is synthetic;
real-text and adversarial-attractor lives have not qualified it. Search and
mandate evidence remain global across islands, so the island-local revocation
valve is now the next constitutional organ. Greedy truth segmentation, unit
death, and capacity lives remain open behind it.

## The ninth body audited, and a repair: probation borrows the body, never the seat

The two commits of the previous turn were retaken independently. The
sixty-eight gates reproduced without a changed byte; the searched-policy
court was read line by line; three refusal probes -- an edited biography
byte, a hard-linked island alias, and a version-9 header under a live
magic -- all failed closed. A deliberate wrong-attractor world, one
candidate with a deterministic continuation against one with ten equally
lived continuations, opened probation in both arms and earned a seat in
neither: searched play recorded 2.124276 bits per raw byte against a best
matched witness of 1.283528, the unanchored arm 7.770698. The declared
prediction that navigation would play worse than its own red arm on this
world was refuted by the measurement and is preserved: the anchor's
correct context outweighs the one-ply tilt. The ninth-body mandate was
retaken at its own election boundary: the margin after the second
probation was 0.329243, three times the appointment threshold. The seat
was earned lawfully, and both commits stand.

The audit then found an eighth-body debt at the foundation. Probation
seated its trial by overwriting the elected actor, so the next election
saw a false incumbent. Two constitutional wrongs flowed from that one
variable: a mover coming out of probation was judged by the retention bar
instead of the appointment bar, and the byte actor that lawfully held the
seat before probation lost its incumbency and had to re-earn the place at
the appointment bar. On a two-phase life -- four period-3 episodes, then
a skewed-census island whose lead decays into the hysteresis band -- the
machine exhibited the second wrong: at the episode-8 election the uni-tri
lead was 0.074940, inside [0.05, 0.1), where the fifth-body hysteresis law
keeps tri seated; the old code seated uni, permanently. No previously
gated life had crossed the band, so no published number changes; the gate
built for this world ran red on the old court before the repair landed.

The repair is one separation: the episode's acting model is local, and
probation sets only it. The elected seat survives the trial untouched.
State advances to version 11 because a persisted seat can no longer mean
a probation in flight. Three new verdicts pin the law: the incumbent
survives an in-band probation, the band itself is measured inside
[0.05, 0.1) by a split life, and a life split exactly at a probation
boundary stays bit-identical.

What is not proved: the first wrong -- a mover taking its first seat
between the two bars through the probation door -- was not exhibited by
any world found. Post-probation margins were bimodal in all four world
families tried, either far above the appointment bar or far below the
retention bar; the repair closes both wrongs with the same variable, but
only the byte face carries a measured red world. The route anchor and the
one-ply rollout also still share a single flag, so the attractor question
cannot yet be isolated by the machine.

## The tenth body: the island court

The ninth-body handoff named the open wound: search and mandate evidence
were global, so a seat earned on one island could act on another long
after any local justification had vanished. The seventh body had already
measured the class of harm in shadow -- foreign frequencies are worse
than ignorance -- but nothing in the constitution could refuse the hand.

The tenth body is a court, not a model. No world model gains an island
dimension. Every island now keeps its own prequential record of the
three byte witnesses on the bytes it lived, plus the move player's local
played and matched-reference evidence. The seat is still elected on the
whole biography, and the elected seat is never rewritten -- the repair
that opened this turn is the load-bearing wall here. What the island owns
is the verdict on the hand: once it has lived a thousand bytes, a seated
actor whose local lead over the island's own newborn record falls under
the 0.05 retention lead is refused for this island and this episode. The
best locally eligible byte witness acts instead, the refusal is a
biography event carrying its numbers, and the mandate travels on intact.
No local evidence, no local verdict: authority moves on comity until the
island can judge. A locally refused move seat keeps the right to reapply
through the probation door, which now opens on the acting hand rather
than the global seat.

The court's arrival immediately collided with the repair gate of this
same turn: on the A9 two-phase world the island court honestly refused
tri's hand at episode 8 by the skew island's local record, which is
lawful under this body but is not the question that gate asks. The A9
world now runs with the valve disabled so each law is measured alone;
the collision itself -- seat kept, hand refused, both visible in the
biography -- is exactly the separation the repair was for.

Measured on a period-3 home and a skew-census iid island. The traveller
carries tri authority earned at home; the alien island prices its first
1200 lived bytes and refuses: local newborn 2.134932 bits per raw byte
against the seat's local 2.759167 -- the fifth-body self-deception law,
now enforceable locally. Over the eight-episode stay the life with the
court runs at 2.070787 bits per raw byte; with the valve disabled the
same seed pays 2.366611 while the seat burns unrefused. Back home the
mandate is untouched and tri acts again. On a rotated period-3 kin island
-- the same cyclic map in a different phase -- the traveller thrives and
the court never speaks: transfer is not punished. A single-island life is
bit-identical with the court present or disabled; a voyage split across
restart is bit-identical to the direct voyage. State advances to version
12 for the per-island evidence plane. Seventy-nine machine verdicts,
ALL GATES PASS, ASan/UBSan through travel and revocation.

What is not proved: the local verdict has one threshold where the global
election has two, so a lead oscillating around the retention bar can flap
the acting hand between episodes; local hysteresis is future law if a
real world exhibits the flap. The local floor is the newborn witness, not
a locally newborn organism: a world where every travelled model, uni
included, is worse than uniform ignorance would still seat the least bad
traveller. Search remains global, and the probation schedule still opens
on a lifetime shadow gain measured across all islands.

## The tenth body audited, and a repair: the local record is not a second truth

All seventy-nine inherited gates reproduced before the court was attacked.
The state loader correctly rejected non-finite local scores and inconsistent
local byte counts, but a finite local score had no conservation law: one
island's atomic, bigram, trigram, played-move, or matched-reference total could
be rewritten without touching the corresponding global record or the external
biography. Because the court reads that unsigned plane directly, the edit could
forge jurisdiction while every version-12 check passed.

The red probe adds exactly one bit to an island's persisted atomic score and
changes nothing else. It resumed on the inherited body. The repair sums every
partitioned score in extended precision and requires it to agree with its
global prequential witness, allowing only floating-point addition-order noise.
The same forged state is now refused as an inconsistent island atomic score;
direct and split voyages remain byte-identical. Eighty gates pass, including
strict C11 and ASan/UBSan. State stays at version 12 because the wire format and
the meaning of every valid state are unchanged; the loader has learned to
reject a state that was never valid.

## The eleventh body: an island may remain ignorant

The tenth body called global `atomic-uni` the island's newborn witness. It was
not one. After life elsewhere it is a travelled frequency model, and an alien
island could be forced to choose the least bad traveller even when every byte
witness was worse than uniform ignorance. The comity boundary carried a second
debt: it was checked only at episode boundaries. An island with zero evidence
would admit a million-byte first episode just as readily as a short one, despite
the constitution naming a thousand-byte evidence budget.

The eleventh body gives an island one non-travelling hand: `null`, the fixed
uniform distribution over all 256 bytes, always exactly 8 bits per raw byte.
It is not a fifth global model and can never own the global seat. It is a local
right to remain ignorant. Once a model has actually travelled to an island,
the byte hand selected by the local court must beat `null` by the unchanged
0.1 appointment margin; otherwise `null` acts for that episode while all
prequential witnesses continue learning from external truth. A local refusal
still leaves the global mandate intact, and move probation may not override a
null verdict.

Comity is now a byte budget. An episode that fits inside the remaining blind
window still receives the travelling hand. If it would cross the boundary, the
court uses only already-lived local receipts and renders the verdict early. If
there are no receipts, the indivisible episode is null in full and a `q` event
records that no future byte was borrowed to decide it. This is deliberately
conservative at episode granularity. The rule applies only after foreign life
exists; a first island and every single-island life retain the old body
byte-for-byte. `--no-birth-floor` restores the body-10 floor and unbounded
episode-boundary comity as the matched red arm.

The preregistered null island is a deterministic full-byte stream. After its
first 600-byte comity episode, the travelled local records are `10.963510`
(uni), `8.048297` (bi), and `8.000000` (tri): no hand has earned a margin over
uniform. At episode 8 the island chooses null and pays exactly `8.000000` bits
per byte. With only the birth floor disabled, tri keeps acting and pays
`8.000047` on the same next 600 bytes. The difference is intentionally tiny;
the constitutional fact is sharp and the red arm proves the gate can fail. A
separate 2000-byte first episode is null with a zero-evidence `q`, while its
floor-off twin lets tri overrun the whole blind window.

The earlier alien court now refuses at the crossing episode rather than one
episode late: seven refusals instead of six and `2.018492` bits per byte over
the stay, against the unchanged no-court `2.366611`. Kin transfer still has
zero refusals, the one-island no-op remains bit-identical, and a null voyage is
bit-identical across restart. State advances to version 13 because actor episode
counts now include the local null hand. Eighty-six gates pass, including strict
C11 and ASan/UBSan through null action and the oversized-episode boundary.

What is not proved: the birth floor uses the appointment margin on every
episode rather than a persistent island-local hysteresis state, so a hand near
7.9 bits may still flap. The null arena is synthetic, the measured gain over
the red traveller is minute, and the episode-granularity quarantine can spend
less than the full comity budget. Search evidence and the probation schedule
remain global across islands. Those are attacks for the next hand, not claims
smuggled into this body.

## The eleventh body audited: two organs refused for lack of a red world

Both incoming commits reproduced whole: eighty-six gates by the auditor's
own hand before any reading. The conservation repair was attacked on all
three named surfaces. Sixty-one restarts interleaving three islands over
244 episodes produced zero false refusals, so the tolerance admits honest
addition-order noise at realistic life lengths; a verdict-reversing edit
needs on the order of a hundred bits where the tolerance admits about a
microbit, eight orders of margin; and the one-bit forgery was verified to
target the intended double by the state layout itself, 96 bytes per
island record with the atomic score at offset 24. The birth floor was
attacked next: 1800 null steps priced exactly 8.000000 with 255 distinct
emitted bytes from one causal draw each, the version-12 header refused,
probation blocked inside null episodes by the acting-hand predicate, and
the seat untouched throughout.

The audit then measured one corollary the body had not gated. A
structureless home, revisited after any real travel, loses its
learned-confidence hand: the global court already prefers the least
confident witness there (travelled tri at nearly exact uniform against
uni at 8.161840), and the island floor then chooses honest ignorance
outright, null against a 7.991742 local record. The fifth-body
self-deception law is now enforced by the court at home, and this
section's gate pins it on the unchanged court.

Two organs proposed as next floors were then refused for lack of a red
world, and both refusals are measurements. The flap: five world families
tuned so a travelled hand's local record crosses the 7.9-bit boundary
produced at most three hand transitions per thirty-episode life;
cumulative records cross the boundary once and settle, so no stationary
world exhibits a sustained oscillation, and island-local hysteresis is
refused as an organ until a world exhibits the flap. The door: a
word-salad island, home words intact in random order, was built as the
candidate red world for island-local probation scheduling; all eighteen
units are recognised there, the home door opens probation, and the mover
honestly loses 2.869828 against a best matched byte witness of 2.009348.
But the salad's own local move shadow gains 1.88 bits over its local
atomic record, so a local door would have opened too: the move shadow
prices atomic moves through the byte context and therefore inherits
byte-context gain wherever a byte hand acts, while the null hand already
blocks probation wherever context is empty. Between those two laws no
island remains where localising the schedule would change a verdict.
Island-local probation is refused as an organ; the schedule stays global
as a matter of measured indifference, not oversight.

What is not proved: the mover-comity crossing still lacks its direct red
world, the route anchor and the one-ply rollout still share one flag, and
the refusals above hold for stationary worlds only; a drifting world
could still exhibit the flap and would reopen the hysteresis question.

## The twelfth body: the vocabulary pays rent

Units had a birth law and no death law, an open wound since the
checkpoint correction. The cost of that asymmetry was already on record:
the fourth body measured a vocabulary paying for its own alphabet where
there was nothing to predict. Every living unit widens every Laplace
denominator forever, so a vocabulary earned on one life becomes a
permanent tax on every later one, and nothing in the constitution could
ever take a slot back.

The death law mirrors the birth law it joins. A unit that the lived
truth has not named for 16384 bytes dies at the next episode boundary: a
biography event carrying its uses and the age of its silence. Death is a
tombstone, not an erasure. The identity keeps its name and its frozen
counts, so no second unit can ever be born with the same bytes; the
matcher, the canonical truth judge, the route search, and the sampled
policy all skip the dead; and the living Laplace alphabet shrinks by
one. The frozen counts stay in the row totals as a mass leak that decays
with life instead of a rent that never ends, and the whole living policy
is renormalized wherever a tombstone exists. Renewed lived support --
every fresh BIRTH_SUPPORT of the same adjacency -- resurrects the same
name in place. Nothing else moves: on a world where no rent falls due,
the organism is bit-identical with the law present or disabled, and
every previously gated life is too short for a death by construction.

Measured on a two-phase life: fourteen units earned on the repeated
sentence, then thirty episodes on a structureless byte island their
forms never match. The deaths arrive on each unit's own rent clock,
spread over five distinct episodes, and the island ends at zero living
of fourteen born. The dead weight is a real tax with the predicted
decay: on the same truth bytes, with atomic records string-identical
between arms, the living arm prices unit-uni at 8.128361 against the
undead arm's 8.129289; the gap is exactly the fourteen ghosts in the
denominator, and it shrinks as the biography grows, which is the
difference between a debt paid off and a rent in perpetuity. Returning
to the birth island resurrects all fourteen under their original names
and births three new longer composites; no duplicate identity exists in
the biography. An extinction split across restart is bit-identical to
the direct life. State advances to version 14 for the rent clocks and
tombstones. Ninety-five verdicts, ALL GATES PASS, ASan/UBSan through
extinction and resurrection.

What is not proved: the rent term is one declared constant, not an
earned quantity, and a slow world could starve a good unit that a
kinder clock would keep; death is global, not island-local, though the
records it releases are not; the mass leak is measured only where
advances stay atomic, and a probation episode under a changed alphabet
is a different organism episode by construction, priced differently on
the same bytes. The tax is small in a long life by design; its size at
the capacity wall, where MAX_UNITS matters, is unmeasured.

## The eleventh-body audit audited: the missing island exists

All ninety-five incoming verdicts reproduced before the two refusals were
attacked independently. The stationary flap still has no red world. The
probation-door refusal does not survive.

The missing island is a de Bruijn cycle of order two over thirty-two byte
values disjoint from the home vocabulary. Every ordered byte pair occurs
equally, so the move bigram's atomic fallback has no contextual advantage;
each pair is also too rare to reach birth support. Yet a byte pair determines
the next byte, so the local trigram earns the hand. After seven home episodes
and seven foreign episodes, zero units have been born on the island and tri
acts. The old lifetime door still sends `mvp` at episode 15 because it carries
the home move-shadow promise across the water. On the foreign record itself,
move-bi pays `6.955204` against its matched atomic witness's `6.782470`: a
lead of `-0.172734`, not the required `+0.1`. The refusal's word-salad probe
had shown one island where both doors agree; it could not prove that all
islands do.

The repair makes the probation promise part of island jurisdiction. Every
canonical truth move now carries the move-shadow price and the atomic price
over exactly the same raw bytes into both global and island partitions. A
trial needs a thousand matched local bytes and a local 0.1-bit lead. The
de Bruijn hand remains tri at episode 15; `--no-local-probation` restores the
old travelling door and sends `mvp` on the identical checkpoint. This also
repairs the shadow door's old event-base mismatch: the first unconditioned
move of an episode belongs to neither side of the comparison.

The new partitions are conserved against their global records on load, and
direct and split lives remain bit-identical. State advances to version 15.
Ninety-six gates pass, strict C11 and ASan/UBSan included. The old refusal
remains in this log as negative experience; the red world, not rhetoric,
revokes it.

The other incoming corollary also needed a smaller name. A sweep over seeds
1 through 16 reproduced `null/null/null` on only eight. Seed 16 keeps tri on
all three home returns and records no refusal. This is lawful, not a leak in
the floor: the so-called structureless home is one fixed pseudo-random tape,
and a different causal path revisits enough of its immutable trigram map for
the local witness to earn the 0.1-bit margin. The seed-5 gate proves that a
home can lose an unearned hand after travel; it does not prove that this tape
is uniformly unknowable. A seed-16 red corollary now prevents the example
from being promoted back into a universal claim.

## The thirteenth body: tombstones remember and do not vote

The twelfth body removed dead units from the candidate set but deliberately
left their frozen counts in unigram and transition denominators as a decaying
mass leak. That compromise fails its own strongest sentence. A dead unit
cannot be sampled, matched, or used as a route node, yet its incoming counts
still change the normalization of one-ply continuations row by row. Candidate
rows carry different ghost mass, so the searched distribution and its price
can change. Death had removed the name from the ballot while leaving its old
votes in the box.

Body 13 separates memory from authority. Historical `move_count`, pairs,
`move_total`, and `mv_out` remain frozen exactly as body 12 requires: identity
is never erased and resurrection has a body to recover. Alongside them Netta
maintains derived living unigram and destination-row totals. Death subtracts
one unit's frozen evidence from those living totals; resurrection adds the
same evidence back under the same id. The unit shadow, move shadow, Viterbi
route score, and one-ply continuation all normalize against living mass.
`--keep-dead-mass` is the exact body-12 valve.

The extinction arena leaves fourteen tombstones containing 642 unigram events
and 642 transition events. Two forced searched players then start from copied
state, fixed source 16, and the same external truth. Their matched byte refs
are identical, but the body-12 ghost arm changes the move receipts and prices
`8.543107` bits per byte against the silent body's `8.557175`. The old leak is
not even claimed to be worse here; it is disqualified because evidence from a
non-candidate changed a live decision. With navigation disabled, all 600
receipts are identical because the common historical denominator cancels,
locating the ghost precisely in searched continuation. After all fourteen
names resurrect, the two valves again produce identical searched receipts:
memory returns to authority exactly when the identity returns to life.

Living totals are deterministically rebuilt from the historical state and
tombstone flags on resume, so the wire remains version 15 and extinction
splits stay bit-identical. One hundred and one gates pass, including strict C11 and
ASan/UBSan through extinction, searched tombstones, and resurrection.

What is not proved: rent is still one global constant, the capacity wall is
not exercised at 4096 live identities, and the living totals are a reversible
view of historical counts rather than island-local memory. A dead unit's past
prequential scores remain in the biography, as they must; the new law removes
only its present mechanical vote.

## The thirteenth turn audited: the overturning stands

All hundred and one incoming verdicts reproduced by the auditor's own
hand before reading. The de Bruijn counterexample was then rebuilt and
its properties verified independently against the raw tape: all 1024
ordered pairs present at counts 39 to 40, zero ambiguous trigram
contexts, maximum pair support 40 and therefore no possible birth. On
the incoming checkpoint the local door keeps tri at episode fifteen
while `--no-local-probation` sends the mover on the identical state.
The overturning of the eleventh-body refusal is confirmed: the refusal's
fallback argument quietly assumed the bigram class carries the trigram
class, and the de Bruijn island separates them exactly. The refusal
stays in the log as negative experience; this section records that its
revocation was independently reproduced, not accepted on faith.

Tombstone silence was attacked on every named path: the living mass is a
derived view, absent from the wire, rebuilt deterministically on load,
and therefore unforgeable by construction -- the conservation lesson of
the tenth-body audit applied before the forgery could exist. No
remaining path lets a dead destination count reach a living decision:
route, anchor, rollout, both shadows, the door, and the court were each
traced to living-mass or record terms. The sixteen-seed sweep reproduced
exactly: eight full-null returns of sixteen, seed five the standing null
witness, seed sixteen lawfully keeping tri.

One constitutional question is named open rather than repaired: the law
of a life is not part of its state. The control flags that change
pricing or jurisdiction are per-invocation, so a life can be resumed
under a different law than the one its records accumulated, and the
biography does not mark the seam. The suite always holds flags constant
within a comparison, and the forced-control quarantine covers the actor
lock, but nothing machine-checks law continuity across resume. Whether
the law belongs in the state, and which flags are law rather than
instrument, is a body-sized question left for a future hand.

## The fourteenth body: the island registry

A life used to be defined by its convoy: the state carried the islands
of the command line in command-line order, resume refused any other
count or order, and thirty-two was the ceiling of a whole biography.
Identity by seat number was a birth-era simplification, named a
temporary measure by the maintainer, and it blocked the road to any
life that meets worlds it was not born with.

The fourteenth body inverts the identity law. An island's identity is
its content -- digest and length -- and never its seat in today's
convoy. The state now carries the registry: the append-only journal of
every island this life has ever met, in order of first acquaintance,
with all per-island records keyed by registry id. Today's command line
is resolved against that journal; a known content answers to its old
name wherever it sits, and an unknown content is an arrival -- a loud
biography event carrying the id, digest, and length, never a refusal
and never silent. Islands absent from today's convoy keep their
records untouched. The biography now speaks registry ids, so a receipt
means the same island in every convoy of the life. The registry holds
1024 identities; the convoy still carries at most 32 at once.

Measured: a life resumed with its convoy reversed and its indices
remapped is bit-identical in biography and state to the direct life,
so the convoy order is invisible to the organism. A new island joins a
living life as an arrival event at its true episode. A detour that
sails without an absent island and returns leaves the state
byte-identical: absence does not erode memory. The same content listed
twice is one identity with one arrival. A forged duplicate identity in
the registry tail is refused on load, as is a version-15 state. All
one hundred one inherited verdicts pass unchanged -- for a fresh life
whose convoy never changes, registry ids coincide with the old seat
numbers and every published number stands. One hundred eight gates,
ALL GATES PASS, ASan/UBSan through the registry and a reversed convoy.

What is not proved: the 1024 ceiling is declared, not exercised -- no
arena has met a thousand islands, and the registry scan is linear in
acquaintances. A mutated island file is, by construction, a different
island; the old per-seat digest refusal is gone, and the tripwire
against silent file corruption is now the loud arrival itself. The
registry remembers where she has been; nothing in it yet chooses where
she goes -- the atlas, an earned travel policy, is deliberately a
separate future organ, and the law of the life across resume remains
the open question named in the audit above.

## The fourteenth turn audited: a name needs a witness

All one hundred eight incoming gates reproduced before the registry was
attacked. Two claims did not survive. First, the state carried an absent
island's digest as its sole authority. Flipping one bit in that identity and
resuming with only the other island passed every conservation equation: the
forged island was absent, so no current file could contradict it, and the
biography hash proved only that the old receipt had not changed. The state had
silently renamed a world behind its own arrival event. Second, simultaneous
new islands were allocated registry ids in command-line order. Reversing a
fresh convoy while remapping the selected island therefore changed both state
and biography before the first played byte. The resumed-order gate could not
see a defect that occurred only at acquaintance.

State version 17 makes both surfaces answerable. Identity is now the triple of
a forward FNV-1a digest, an independently seeded reverse-byte witness, and
length. Every `i` receipt records all three, and resume parses the external
biography rather than merely hashing it: ids must arrive exactly once in
append order and every persisted triple must equal its historical receipt.
The absent-state forgery is consequently refused before today's convoy is
resolved. Simultaneous unknown contents are sorted by identity before ids are
allocated; the selected route confers no naming privilege. Two older gates
that had confused CLI seat with registry identity were repaired to resolve
their target through the receipt they meant to measure. Their numerical laws
remain unchanged.

One hundred ten gates pass, strict C11 and ASan/UBSan included. A reversed
fresh convoy with the same chosen content is bit-identical, and the absent
identity forgery is a loud refusal. The two fingerprints make accidental
aliasing much harder but are not a cryptographic proof of byte equality for an
island no longer present. The arrival/state publication seam is also only
failure-closed: a crash after biography append and before atomic state rename
leaves an unverifiable tail and refuses resume. Recoverable two-object commit
is a future body, not a property claimed here.

## The fifteenth body: the Atlas earns a destination

The registry answered who an island is but deliberately did not answer where
to live next. Body 15 adds that missing separation as an explicit `--atlas`
organ. The Atlas is navigation, not an actor and not a second court. It may
choose only among distinct registry identities physically present in today's
convoy and large enough for the requested episode. Models still act under the
global seat and the island's local verdict after the destination is chosen.
Manual `--island` remains both the helm and the matched red arm.

The policy has two causal phases. A shore with fewer than 1000 lived bytes is
uncharted; the least-lived uncharted shore has priority, with stable registry
identity breaking exact ties. Once every eligible shore has a full local
record, the Atlas compares the cheapest already-measured local byte witness on
each island -- `min(8, uni, bi, tri)` in bits per raw byte -- and chooses the
lowest price. It reads no future episode and grants no naming privilege: fresh
ties follow the canonical identities established by the audited registry. A
single present identity is an exact no-op with no travel receipt. If several
identities are present but only one can physically hold the episode, the
receipt says `eligible` rather than pretending a competition occurred.

Every competitive choice is a pre-action `t` receipt. A chart receipt records
the winner's lived bytes and the nearest alternative; an earned receipt
records the winning and runner-up prices plus the winning evidence base. In
the chart arena, the unseen island wins at 0 lived bytes against a familiar
1200. After both worlds have 1200 bytes of evidence, the period-3 island wins
the earned comparison at `2.227308` against the pseudo-random island's
`7.563303` bits per byte. The copied manual helm then lives the losing island
for all 600 matched steps, proving that the Atlas, not a hidden default, made
the choice.

Six new verdicts cover the one-island no-op, fresh convoy-order invariance,
least-lived exploration, the numeric earned choice, the manual red arm, and a
split life resumed under a reversed convoy. One hundred sixteen gates pass,
strict C11 and ASan/UBSan through the Atlas included. State remains version 17:
the policy introduces no hidden memory; its inputs were already conserved and
every autonomous decision is external biography.

This is an Atlas, not yet an interconnection-seeking instinct. It ranks only
shores the operator makes present, and after charting it exploits past local
predictability; it does not yet predict transfer to an unseen world, price
novelty, or prevent an easy-island attractor. The current fingerprint also
reads an immutable island's whole content to establish identity even though
the travel score reads only lived receipts. Those boundaries are explicit so
Gutenberg blood can test the organ rather than be used to name it prematurely.

## The fifteenth turn audited: the witness holds, the map holds

All hundred and sixteen incoming verdicts reproduced by the auditor's
own hand before reading. The witnessed registry was then attacked on
its named surfaces. A three-world fresh birth presented in two
different command-line orders produced bit-identical state and
biography, so canonical acquaintance owes nothing to the convoy.
Reordering two arrival receipts in the biography was refused under the
new law by name, "does not conserve the island registry", showing the
receipt discipline fires on order and not only on content. A duplicate
path in an Atlas convoy collapsed to one identity with one arrival,
and an island too small for the requested episode was passed over as
ineligible while the sole survivor was chosen with an honest
`eligible` receipt rather than a manufactured competition.

One boundary is named rather than repaired: a coherent rewrite of both
files at once -- an edited biography with a recomputed chain and a
state forged to match it -- is not partial forgery but the fabrication
of an entire life, and lies outside the threat model this constitution
defends. The defense holds against every drift and every partial hand;
against an author of a complete false world it holds nothing, and no
account-keeping system does.

The turn also harvested the buried prototype's neural scars as design
law for the next body, read directly from the old log: unbounded
accumulation drove a float accumulator past its ceiling into NaN, and
more quietly, 54 percent of a vocabulary wore silently degenerate
zero embeddings under normal-looking floats; a geometry collapsed
into a cone where the background cosine reached 1.0 and a composite
rose while the organism fell; beautiful mechanisms repeatedly lost to
strong dumb controls; a saturated signal was a dead signal. Each scar
becomes an executable gate where the neural tissue now enters.

## The sixteenth body: the neural core enters in shadow

Training was always her second name, and the maintainer set the order:
the architecture is not complete without its core, and the core does
not wait for the real-text runs, because the constitution's entry
protocol makes early arrival free. It enters the way the unit shadow
and the oracles entered: a witness with a record and no power.

The lineage is the grave's own, reimplemented under the zero courts,
never copied: no backpropagation anywhere. A recurrent hidden state of
thirty-two over innate byte embeddings of twenty-four, the buried
prototype's dimensions kept as inheritance. Three local laws learn on
every lived byte, strictly after the receipt. The delta rule moves the
readout alone, error times hidden activity. Surprise-gated Hebbian
plasticity moves the dynamics: the prequential surprise against a
floating prophecy baseline -- the grave's 0.82/0.18 blend -- opens the
gate only past half a bit, potentiates co-activation that beat the
prophecy and depresses what fell short, under decay and a hard clamp.
The baseline itself is the third law, a fast quote over slow memory.
The readout is born at zero, so the newborn's first price is exactly
8.000000 bits: ignorance is the honest starting capital. The hidden
state is episode-local, rebuilt from the sixteen observable wake bytes
without pricing or learning; only weights, the baseline, and the record
persist, so a split life stays bit-identical. The core consumes no
draw from the life's rng: every inherited number stands untouched, and
the biography is byte-identical with the core present or disabled --
the shadow casts no shadow on the game.

The first record is honest in both directions. On the alternating
world the core prices 0.266429 bits per byte where the byte bigram
carries 0.908803 on the same cumulative bytes: the first time any
witness outran the counter ladder, and the anti-cone proof in the same
number. On period five, structurally past the trigram's two-byte
memory, the core sees the phase: 0.274086 against tri's 0.859384. On
period six it wins twice more (0.974325 and 1.024118 against 1.227).
And on period seven it fails badly, 7.338107 against tri's 0.845067,
and on period eight likewise -- the shadow's measured weakness is a
gate now, so any future hand that heals it must flip a red assertion
consciously. On the structureless world the core converges to
8.002525 while the trained unigram pays 8.126462 for its learned
confidence: the fifth-body law holds for neural tissue unmodified. On
repetition it learns within one life to 4.067613, still behind the
counters -- the shadow has the right to be weak where exact memory is
the whole game. Mean hidden activation stays at 0.6237 and zero
embeddings are degenerate: the grave's scars stay closed, as gates.

What is not proved: the plasticity constants -- rates, decay, the
half-bit gate, the clamp -- are declared first-version discipline, not
earned quantities. The Hebbian law's own contribution is not yet
isolated from the delta readout by a matched arm. The core has no
candidacy, no probation path of its own, no island-local record, and
no seat in the Atlas; those are courts it must still enter through
doors that already exist. The suite pays real wall-clock for the
shadow on every lived byte, and the periods seven and eight stand as
open wounds by design.

## The sixteenth turn audited: Hebb pays its first debt

All one hundred twenty-seven incoming gates reproduced before the
neural tissue was read. Its causal order then survived inspection:
the current hidden state prices the byte, the truth moves the readout,
the truth advances the recurrent state, and only then may recurrent
plasticity affect a future prediction. The wake reads sixteen already
observed bytes without charging or learning, the core consumes no game
RNG, and the biography remains identical with the shadow disabled.

The missing matched arm changed the verdict. Recurrent weights were
frozen while the identical zero-born delta readout, embeddings, source
positions, and truths remained live. On four 2000-byte episodes, the
frozen and Hebb-v1 core prices were respectively:

| world | frozen core | Hebb-v1 core | byte-tri |
| --- | ---: | ---: | ---: |
| period 5 | 0.211809 | 0.274086 | 0.859384 |
| period 6 | 0.334798 | 3.133655 | 1.131458 |
| period 7 | 0.379191 | 7.338107 | 0.845067 |
| period 8 | 0.493587 | 2.921194 | 1.056921 |

The rule did not merely fail to help: it lost every matched world and
collapsed catastrophically on periods six through eight. On a
forty-thousand-byte period-eight life, Hebb-v1 reached mean absolute
hidden activation `0.9840` and priced `3.066149`, while the frozen
reservoir stayed at `0.2115` activation and priced `0.277564`. Positive
surprise gates outnumbered negative ones `34732` to `5076` in the live
arm. This is the grave's old positive-feedback scar under a new name,
not an unfortunate constant chosen one notch away from success.

The repair is constitutional rather than cosmetic. Frozen recurrent
dynamics are now the default core; `--core-hebb-v1` preserves the
failed plasticity as an explicit red arm, counts its proposed positive
and negative gates, and reproduces the period-seven loss as executable
law. No rate was tuned after the verdict. A future Hebbian body must
return through the buried design that actually earned trust: multiple
shadow plasticity experts receive identical experience, prequential
progress against a frozen readout decides fitness, and an external
court quarantines genes before any winning rule touches the organism.

A second falsifier found that the supposedly innate embedding table
was persisted in v18. Replacing its checkpoint bytes with zeros
resumed successfully and reported all 256 embeddings degenerate: the
claim “nondegenerate by construction” had been true only at birth.
State v19 removes the embedding table from the wire and regenerates it
from its dedicated fixed seed on every invocation. The mutable neural
weights, baseline, bit debt, and byte count are bound by an FNV witness;
a one-byte edit to the otherwise admissible neural record is refused by
name. As with the island registry, a coherent rewrite of the data and
its witness remains fabrication of a whole life, not a partial-forgery
surface claimed to be solved.

Four new verdicts cover period six, period eight, the quarantined
period-seven red arm, and partial neural-memory forgery. One hundred
thirty-one gates pass, strict C11 and ASan/UBSan included. The core is
still a witness without authority. Before any replacement Hebb rule or
new organ, `GUTENBERG_ARENA.md` seals the promised Dracula,
Frankenstein, shuffled, and technical-alien arena: six witnesses will
now write the job description from real text, and every failed
prediction will remain public.

## First Gutenberg blood: four yes, four no

The preregistration was committed as `c866701` before the first model
run. The three raw Gutenberg downloads matched their sealed sizes and
SHA-256 hashes; normalization produced 855114 Dracula bytes, 421541
Frankenstein bytes, and 233688 technical-apparatus bytes. The
SplitMix64/Fisher-Yates Frankenstein twin conserved all 256 byte counts.
Three fixed 4096-byte windows then priced all six witnesses from
untouched donor-state copies against newborn controls.

Kin transfer passed at every depth and every window. The median gains
in bits per raw byte were `0.248274` atomic, `1.743025` byte-bi, and
`2.686820` byte-tri; unit-uni gained `0.559914`, move-bi `1.721940`,
and the frozen-reservoir core `1.968741`. Context carried more than
frequency, and two-byte context carried more than one-byte context.

The two controls overturned the convenient interpretation. Technical
English transferred `0.118680 / 1.532517 / 2.238602` on the counter
ladder and `1.780461` on the core. Its contextual excess over atomic
was positive, not negative: a human genre label did not make a
statistically alien byte world. On the shuffled twin the same gains
were `0.254909 / 0.500562 / 0.615713`; contextual excess remained
`0.245653` for bi and `0.360804` for tri, above the sealed `0.1`
ceiling. The permutation killed prose order but atomic subtraction did
not remove all reusable conditional prior in finite context rows. The
core's shuffled gain of `1.012479` makes the neural lesson explicit:
raw transfer is not selective transfer.

Vocabulary rent cut exactly where the young vocabulary allowed. Of
103 living Dracula units, 102 exact forms also occurred in
Frankenstein. All survived the crossing. The sole absent form, unit 58
`--`, died at episode 12 with rent age 24157, but did not resurrect in
the sealed 16000-byte return. The death law passed; the return
prediction failed. More importantly, the learned alphabet exposed
itself as mostly shared short orthography rather than content-specific
lexical identity.

The court emitted no refusal in either eight-by-800 voyage. Kin
non-refusal passed and technical refusal failed. The travelling
byte-bigram hand remained predictive on both English islands; a court
that judges local receipts had no honest reason to enact the human
category “alien.” This is a failed prediction, not a broken court.

The public ledger is four passes and four failures. Exact windows live
in `gutenberg_results/2026-08-16-transfer.tsv`; the literal decision
record and full interpretation live beside it and in
`GUTENBERG_RESULTS_2026-08-16.md`. No corpus or threshold moved after
the verdict. The blood writes the future Hebbian job: shadow experts
must compete on identical experience under prequential fitness and an
external quarantine, and any claimed interconnection instinct must
earn selectivity against shuffled and genuinely alien controls, not
merely lower loss on another English byte stream.

## The seventeenth turn audited: the quarantine and the verdict both hold

All hundred and thirty-one incoming verdicts reproduced by the
auditor's own hand before reading. The frozen-versus-Hebb isolation was
then rebuilt independently on four periodic worlds: where the worlds
coincide with the sealed table the numbers match to the last digit
(period five 0.211809 frozen against 0.274086 v1; period seven
0.379191 against 7.338107), and on the auditor's own period-6 and
period-8 patterns the same verdict returns (0.414357 against 0.974325;
0.739537 against 4.822724). The runaway was chased past the sealed
horizon: on an eighty-thousand-byte period-8 life the v1 arm saturates
at mean hidden activation 0.9858 and 5.487499 bits per byte with
59654 positive gate events, while the frozen arm rests at 0.2165 and
0.377977. The diagnosis of positive feedback stands at double the
distance.

The neural-memory witness was attacked on fields the suite's own gate
does not touch: a poked prophecy baseline, a poked mid-readout weight,
and a poked witness byte itself are each refused by name. The
version-19 wire carries no innate embeddings to poison. One question
moved from argument to machine fact: a red-arm life resumed without
`--core-hebb-v1` is accepted and silently continues under a different
law -- the invocation-mask question now has a tool output, not an
opinion.

The sealed Gutenberg decision layer was recomputed from the recorded
per-window table: all nine medians and both contextual-excess figures
(0.245653 and 0.360804) agree exactly, and all eight verdicts follow
from the sealed inequalities. The shuffle windows also expose the
likely author of that failure: the travelled trigram prices the
shuffled twin at about 6.97 bits against the newborn's 7.59 -- both
near ignorance, the gap made of smoothing maturity rather than
transported structure. Selectivity law must subtract that confound,
and the court specification now does. The full re-derivation from raw
sources awaits the network word; until then the raw-hash layer of the
seal rests on one hand and is named as such.

## The plasticity court is sealed before its first expert breathes

`PLASTICITY_COURT.md` specifies body 17 and is sealed before any expert
run, in the same discipline that sealed the Gutenberg arena. Eight
genomes on complete shadow copies of the core's mutable memory, the
frozen incumbent sitting as genome zero inside the same life; genes
restricted to what each shadow record can evaluate, with a homeostatic
anti-runaway gene class that Hebb-v1 lacked; fitness as selectivity --
median kin gain minus median shuffle gain, with a breadth veto against
out-broadening the incumbent on the alien arm and a health veto that
prices saturation as death; promotion only at the constitution's own
0.1 appointment margin over the frozen incumbent's selectivity, a
defeated law preserved as a red arm, and a published null if no genome
wins. The alien control must be measured before it is named: candidate
tapes qualify only past the technical donor's recorded distances on
both census and bigram rulers, with real internal structure. The jury
is an instrument under an explicit flag, witnessed and restart-exact,
with no candidacy and no authority beyond one sealed fitness table.
The court is specification only until it survives independent review;
building it before that review would be taking our own word for it.

## The eighteenth turn refuses the first seal before it can become a result

The independent review found that the first plasticity specification could
not support its own verdict. Genome index changed the newborn reservoir, so
genes were confounded with body lottery. Six gene classes were named without
one actual range or one of the eight rows. Readout learning rate remained a
gene inside a court trying to subtract readout maturity. `K-H` compared
different target tapes and could be won by wounding the shuffled control.
The proposed homeostat only discounted future potentiation and could not pull
back an already saturated row. A mean over all hidden units could hide one
dead subpopulation. The alien distance and restart laws were prose without a
complete computation.

No expert or jury existed yet. The first seal is therefore preserved by its
SHA-256 and refused, not silently rewritten after evidence. Commit `3e57be3`
reseals the exact court before implementation: every genome begins from the
same bytes; the fixed delta readout is no longer a gene; all eight gene rows
are literal; decay and per-row saturation rent act on every byte; health sees
the maximum row and the near-saturation fraction; and the causal contrast is
ordered Dracula against a same-age, same-census shuffled Dracula donor on the
same target window. Absolute gates make damage to a control unable to masquerade
as selectivity.

The alien is now a measured byte world rather than a human genre label. A
deterministic 233688-byte lattice clears the technical-English maximum by a
strict min-versus-max rule: minimum census JS `0.707803` against floor
`0.039475`, conditional JS `0.253721` against `0.068688`, and a
`7.977560`-bit gap between census and conditional entropy. Its bytes and hash
are regenerated before any expert runs.

The repository cleanup had moved the Gutenberg runner under `scripts/` while
leaving its source-root calculation and documented command behind. After that
reproducibility fault was repaired, all three raw downloads were reverified
from the hash-checked cache and the complete arena was re-derived. Its
`transfer.tsv` and `verdicts.tsv` are byte-identical to the tracked records:
the earlier four-pass/four-fail verdict now rests on a second hand from raw
source, closing the open qualification in the seventeenth-turn audit.

## Body 17: Hebb's physiology heals; its jurisdiction does not

Eight complete shadow cores now run only under `--jury`. Genome zero and the
ordinary frozen core price every tested byte identically. The jury consumes no
life RNG, changes no biography or actor, persists its literal genes, weights,
baselines, scores and health counters under one witness, and an uninterrupted
life equals the same life split at an episode boundary. State v20 binds the
neural law tuple `(core, Hebb-v1, jury)`: removing either plasticity flag at
resume is refused by name instead of silently changing the organism.

The first full invocation exposed an instrumentation error before publication:
the recurrent clamp counter also charged the fixed delta readout, so even
frozen genome zero appeared to hit the plasticity wall thousands of times.
That invocation is retained by result hash as invalid. Only the counter's
quarantine boundary was repaired; no update, score, gene, tape, seed, window,
threshold or verdict rule moved. Two clean full courts from empty directories
then produced byte-identical JSON and tables.

All seven plastic genomes survive the forty-thousand-byte synthetic health
court: zero near-saturated activations, zero recurrent clamp attempts, no
control-relative hidden-row failure, and no p5-p8 price more than 0.1 bit per
byte behind frozen. The positive-feedback disease of Hebb-v1 is cured in the
tested family.

No genome earns promotion. Frozen genome zero has `K=0.454029`. Genome 4 is
the conservative near miss: ordered kin improves from `4.421399` to
`4.156614`, its shuffled arm is not sabotaged, but `K=0.532586` misses the
appointment threshold `0.554029` by `0.021443` and its alien records fail.
Genomes 5 and 6 clear the kin selectivity margin but buy it with a worse
shuffled arm and a broad prior on the alien lattice. Genome 7 shows why both
alien laws exist: its alien difference passes, while both absolute alien
prices expose damage.

The verdict is a healthy Hebb null. Frozen recurrent dynamics remain default;
Hebb-v1 remains its explicit red arm; none of the seven new rules enters the
organism. One hundred thirty-eight gates pass, including causal twins,
zero-intervention biography, jury restart identity, neural-law refusal, jury
forgery, strict C11, and ASan/UBSan. The next plasticity question is no longer
how to stop saturation. It is how a local rule can learn structure without
claiming jurisdiction over every structured world.

## The jury turn audited: the null holds, and the pattern names the next law

All hundred and thirty-eight verdicts reproduced by the auditor's own
hand on the merged trunk. The version-20 law tuple was attacked from
both directions: a jury life resumed without its flag, an ordinary life
resumed with the flag added, and a red-arm Hebb life resumed bare are
each refused as "neural invocation law changed" -- the silent seam this
log demonstrated one turn ago is closed for the neural laws. A byte
poked deep in the persisted jury memory is refused as "jury memory
disagrees with its witness". The published court table was reread
against its own sealed gates: eight rows, eight FAILs, one lawful null.

The genome pattern gives the next court its hypothesis, and it deserves
to be stated as mechanism rather than mood. The aggressive rules (g5,
g6) post the largest ordered-versus-shuffled margins, but the margin is
bought where the sabotage gate catches it: their plasticity memorizes
the lived order and therefore prices every other order worse -- not
selectivity but fragility. The careful rule (g4) improves the ordered
kin genuinely, 4.156614 against the incumbent's 4.421399, and still
fails the sealed selectivity margin by two hundredths while leaking
breadth on the alien. The weak rule (g7) pays plasticity's overhead
without either gain. One timescale of Hebbian change must choose
between rigidity and instance-overfit; the measured table is that
dichotomy written eight ways. The hypothesis for a future sealed court:
selective transfer needs two timescales -- fast weights that live and
die with the instance, slow weights that accept only what repeats
across contexts, and consolidation between them gated by repetition
rather than surprise alone. No such court is opened here; the
hypothesis is preregistered thinking, not a body.

The sealed sources survive on disk and all three raw hashes match the
arena's table by this hand too, so the raw layer of the seal now rests
on two verifications of the same files; a fully independent
re-derivation of the arena from those bytes follows in this turn's
second hand.

## The second hand: the sealed arena re-derived in another language

`scripts/garena_prep.c` is an independent C implementation of the
sealed corpus preparation: the same CRLF-and-CR law, the same
strictly-between marker extraction, the same SplitMix64 Fisher-Yates
twin with a histogram refusal. From the hash-verified raws it produced
bodies of 855114, 421541, and 233688 bytes -- the technical body's
length agreeing with the first hand's own census record -- and a
conserved shuffled twin. Driving the organism directly from the shell
under the sealed constants (donor seed 160816, eight episodes of 8000;
probes of one 4096-byte episode at the three fixed offsets; newborn
controls at seed 260816), this hand re-derived the complete transfer
table: all fifty-four travelled and newborn prices match the sealed
TSV to the last digit, zero mismatches, and the Dracula donor grows
the same 103 living units. The sealed Gutenberg arena now rests on two
implementations in two languages that agree byte for byte where it
counts. What one hand seals, another hand must be able to grow again
from the raw world; today it can.

## The eighteenth body: the mouth

Oleg asked how she speaks after ten thousand games, and the answer had
to be an organ, because the constitution had no way to hear her: every
emission was judged against the next world byte, and free-running
speech is dream-class. The mouth resolves this as a read-only
instrument. Under `--speak N` a resumed life samples N bytes from its
lived distributions, warmed by an optional prompt, drawing from a
dedicated stream that never touches the life's rng. It prices nothing,
learns nothing, appends nothing, and saves nothing: state and
biography are byte-identical after speech, which is the mouth's whole
license to exist. A newborn is refused -- speech is the product of a
lived state -- and so is a stranger island, because meeting a world is
a biography event and the mouth may not write one. The elected seat
speaks; an mv seat falls to its best byte witness, since the first
mouth emits bytes; `--actor-lock` lends the tongue to any hand for
matched comparisons. State stays at version 20: an organ that writes
nothing needs no wire.

Her first words are on the record in
`research/FIRST_WORDS_2026-08-16.md`, and they carry a discovered law.
After 64000 bytes of Dracula the bigram seat speaks half-babble with
English bones (58.5 percent printable). After 384000 bytes the seat
passes to the trigram on its pricing record -- and the speech
collapses to 39.7 percent printable, while the locked bigram hand on
the same life speaks recognisable proto-English at 89.7 percent, and
the unigram hand emits printable letter-soup at 97.7 percent with no
structure at all. The reason is arithmetic: sampling rolls the
Laplace dice at every step, so thin trigram rows spill most of their
mass into smoothing junk that pricing rarely meets on real text. The
pricing champion is not the speaking champion -- the eighth body's law
in a new discipline, measured before anyone thought to claim
otherwise. If speech ever deserves authority, it will need its own
court with its own ruler, and printability alone cannot be that ruler,
because the structureless unigram wins it.

A third law surfaced while the gates were being built: the cold start.
A promptless tongue that opens at a zero context stands outside the
lived manifold, and on a narrow world there is no road back -- every
junk byte leads to another unlived context, so the speech never finds
the alphabet it lived. The English lives forgave this because English
bytes follow many contexts; the period-3 world did not, and spoke
three lived characters in sixty. The mouth therefore opens, when no
prompt is given, at the life's most-lived context -- the argmax of the
lived pair counts, deterministic and earned -- and the period-3 life
then speaks its own cycle back, `cacbab`, before the smoothing dice
first derail it. The derailment itself remains honest arithmetic:
sampling pays the Laplace tax at every step that pricing pays only
where the world is genuinely new.

What is not proved: the prompt reaches only the two bytes of context a
byte hand carries; unit, move, and core speech are later bodies, and a
speaking court is preregistered thinking only. The suite's spoken
gates pin the mouth's laws, not its eloquence.

## The mouth turn audited: memory holds; hidden boundaries do not

All hundred and forty-six incoming gates passed before the implementation
was read.  The second hand of the Gutenberg arena then met raw boundaries
the three books did not contain.  Its CR/CRLF normalization, prefix-only
markers, marker-at-EOF behaviour, and SplitMix64 shuffle agree with the
canonical Python hand.  One real disagreement remained: after seeing its
first END marker the C hand stopped counting, so a raw source with two END
lines was accepted where the sealed hand refused it.  The descending shuffle
also formed `n - 1` for an empty body and underflowed before its loop could
refuse or accept anything.  The hand now counts every following END while
retaining the first boundary, and the shuffle walks positive spans.  A fixed
sixteen-byte result, `0902000c08030d010504060f070e0b0a`, pins the generator
step, modulo draw, and swap order rather than trusting the three matched
outputs again.

The mouth's central license survived hostile use.  Reset, Atlas, every law
flag, and both successful and refused calls leave state and biography
byte-identical.  A direct continuation equals a continuation with speech
interleaved, locating the tongue outside the life's RNG stream.  Prompts that
differ everywhere except their last two bytes produce identical byte-hand
speech, and an mv seat names the byte witness it borrows.  Three edges were
nevertheless constitutionally wrong: `--speak 0` was mistaken for no mouth
mode at all; a checkpoint published after zero episodes was "resumed" enough
to speak despite having lived no byte; and speech-only flags without
`--speak` were silently ignored by an ordinary life.  The mouth now has an
explicit request bit, requires a nonempty lived record, and refuses orphaned
speech controls.  None writes a byte, so state remains v20.

The first-word measurements were rebuilt rather than accepted from their
lossy dotted transcript.  The pinned Dracula raw is 890348 bytes with
SHA-256 `96cd16eacdbfebae8fdda5591f66e0cc8ee76be18e0cd1aca02bc00615782d28`;
the repaired second hand produces the published 855114-byte body with
SHA-256 `a7786a4c81df95265b33d8c24dbbcaee80ab531d7d266a9782d52301718ce7c7`.
Seed 160816, 8000-byte episodes, prompt `The `, and speech seed 7 reproduce
all four tracked byte streams exactly.  Their ruler is now stated precisely:
ASCII `0x20..0x7e` plus LF is called displayable.  It gives 117/200 = 58.5
percent for the 64000-byte bi life and, after 384000 bytes, 119/300 = 39.7
percent for elected tri, 269/300 = 89.7 for locked bi, and 293/300 = 97.7
for locked uni.

## Body 19: ignorance keeps its price and loses the microphone

The eighteenth body's failure names a disciplinary boundary, not a bad
trigram.  Laplace smoothing is an honest prequential price because the next
external byte may be one the life has never seen.  Free generation is a
different act.  Sampling the 256 smoothing pseudo-counts says that an
unobserved continuation was lived, then often lands in another unobserved
context whose next choice is uniform again.  On the fixed period-3 life this
law emitted 113 of 120 bytes outside the three-byte alphabet.  The mouth was
not paying a tax; it was spending ignorance as memory.

The nineteenth body separates the laws.  By default a byte hand samples only
observed continuations at its deepest supported context.  An empty trigram
row backs off to the witnessed bigram row; an empty bigram row backs off to
the lived unigram.  The diagnostic reports how many emitted bytes borrowed
each depth.  Body 18's exact arithmetic remains behind `--speak-laplace` as a
named red mouth, so the finding is permanently reproducible rather than
edited out of history.

The red twin is decisive on the narrow world: supported backoff emits zero
foreign bytes in six hundred and stays on the exact `cacbab` cycle.  On the
independently regrown real-text lives, the 64000-byte bi mouth uses bi support
for 200/200 bytes and moves from 58.5 to 98.5 percent displayable; the
384000-byte elected tri uses tri support for 300/300 and moves from 39.7 to
100.0 percent.  Locked bi moves 89.7 to 98.0; uni remains 97.7.  These are
physiology measurements, not an eloquence court: the structureless unigram
still defeats printability as a candidate ruler.  No hand earns speech
authority, no free-running output enters memory, and no dream is promoted.
The mouth is a healthier instrument and nothing more.

One hundred and sixty-three gates now pass: the second hand's hostile
boundaries, zero-byte life, zero-byte request, every-law read-only matrix,
prompt horizon, interleaved RNG identity, mv fallback, speech-control
quarantine, supported graph, causal fallback, Laplace red arm, strict C11,
and ASan/UBSan join the inherited constitution.

## The supported mouth audited

The nineteenth-body turn was reproduced from zero before its diffs were
read: all one hundred and sixty-three gates passed with a fresh binary, and
the four tracked first-word streams re-spoke byte-identically under
`--speak-laplace` from this side's independently lived 64000- and
384000-byte states — the red arm preserves body 18's exact arithmetic in
both hands' worlds.  The supported arm's published census reproduced to the
byte: 197/200, 300/300 at tri depth, 294/300, 293/300.

Five attacks were then pressed.  The bounded sampler's rejection threshold
`(0 - bound) % bound` was recomputed by a second hand as `2^64 mod bound`
through 128-bit arithmetic at every power of two and both neighbours, with
no disagreement, no out-of-range draw in twenty thousand at each bound, and
every residue reached at small bounds.  The row totals the sampler trusts
are not trusted at all: the state loader recomputes bigram and trigram row
sums and refuses a checkpoint whose books disagree, so a forged-but-loadable
layout with desynchronized totals does not exist and the sampler's own
disagree-exit stands third behind the witness and the load audit.  A
fifty-thousand-byte speech on the period-3 life contained zero bigrams that
the tape had not lived — the census was taken against the world, not against
the speaker's claims.  A world whose final byte is lived but has no
successor forced the full causal ladder in one utterance: support `uni 1,
bi 1, tri 22` — one draw at the floor, one recovery through the bigram book,
then home.  And a life on two shores with disjoint alphabets spoke six
hundred of six hundred bytes in one shore's letters: the mouth is the
life's, but speech lives in the basin of its opening, because the life
never lived a single cross-shore transition for the tongue to walk.  That
sentence now stands as the honest reading of "the life's mouth": `--island`
chooses where to play, never where to speak.

The challenge to find a world where the red arm is structurally better was
answered with a preregistered ruler: on a world drawn uniformly over all
256 byte values, ruler = how many distinct bytes each law can ever name,
declared before either sample was drawn.  After a 512-step life whose
window held 224 distinct values, supported backoff spoke exactly those 224
and could not name the missing 32; the Laplace arm named all 256.  A world
whose truth is wider than the life is precisely where spending ignorance is
the only way to say the unseen.  Both laws keep their arms.

Terminal safety stays a warning, not a frame: an escaping layer would make
the instrument lie about its own bytes, and the reader owns `tr`.

## Body 20: the island's ear

A speaking court cannot exist until its judge exists, and the judge must be
named and sealed before it hears any candidate.  The twentieth body builds
the judge as a measurement instrument with no office: the island's ear.

Every island can carry its own statistical judge, grown from its immutable
tape and nothing else.  A foreign prior cannot be smuggled through an object
that is a pure function of the sealed shore: the ear reads the whole tape
once — every byte into the census, every adjacent pair into the bigram book,
every triple into the trigram book — and prices a spoken stream with the
island's own Laplace ladder, trigram context with unigram and bigram
warmup.  Pricing is where charging ignorance is honest, so the judge keeps
the smoothing law the mouth gave up.  And because the tape is immutable and
fully known, the parrot is caught by substring, not by statistics: the ear
reports the longest verbatim quotation and the fraction of the stream
covered by quotations of sixteen bytes or more.

The ruler is preregistered here, before the ear hears any candidate words:
an utterance is judged by (1) bits per byte under the shore's own ladder —
lower is closer to the shore's law; (2) the longest verbatim quote and the
quoted-16 coverage — a stream that wins (1) by copying the tape is named a
parrot by (2), exactly; (3) jurisdiction is local: a stream is judged by
the shore it is offered to, and by nothing else.  No threshold, promotion,
or authority is attached to any of these numbers.  The ear wires into no
election, opens no state, writes no byte; `--ear` refuses a mouth in the
same invocation, refuses `--reset`, `--state`, and `--bio` by name, and
needs at least one shore.  State remains v20.

On the constitution's own worlds the ear behaves as sealed: a verbatim
200-byte slice of the period-3 tape prices at 0.46 bits per byte on its own
shore with longest-quote 200 and full quoted coverage — a perfect parrot,
perfectly named — while the same slice offered to a disjoint-alphabet shore
prices at 8.02 bits per byte and quotes nothing.  A sorted twin of the true
slice, same census, destroyed order, pays more than the true slice on the
same shore: the ear hears structure, not alphabet.

## The first hearing

With the ruler sealed above, the Dracula shore's ear heard all eight
recorded streams: the four Laplace-arm streams and their four
supported-backoff twins.  Under the shore's own law the elected trigram's
supported speech prices at 2.99 bits per byte — the best of the eight by a
wide margin — with bi at 4.68 on the young life and 4.99 locked, while the
locked unigram, printability's champion, prices at 7.48: within half a bit
of raw ignorance.  Every Laplace stream is beaten by its supported twin
except uni, where both sit at the noise floor.  The quotation census clears
all hands: the longest verbatim quote in any stream is twelve bytes and no
stream carries a single sixteen-byte quotation — the speech is generated,
not copied.

The hearing closes the ruler question the eighteenth body opened.  "Pricing
champion is not speaking champion" was true of a Laplace mouth judged by
printability; under an honest mouth and the shore's own sealed ear, the
elected hand is the best speaker again, and the census hand that printability
crowned is heard as near-noise.  Discipline transfers from pricing to speech
once both the speaker and the judge stop spending ignorance.  No promotion
follows: the ear is a measurement, the numbers stand recorded, and any
speaking court over them remains future work with its own red worlds.

One hundred and seventy-five gates now pass: the ear's convoy hearing,
exact quotation census, per-shore jurisdiction, determinism, sorted-twin
red, write-nothing law, four refusals by name, and ASan/UBSan through the
ear join the constitution.

## The ear audited: a match is not a confession

The twentieth body and all one hundred and seventy-five inherited gates were
first reproduced from a fresh binary at `6d9ee94`.  Three independent probes
then broke the ear's stated constitution.  `--seed 99` returned success and
was ignored, as did the rest of the untracked life-law surface.  A stream of
exactly 16384 bytes was refused despite the inclusive public bound because a
full `fread` had not yet set EOF.  And an otherwise valid hearing failed when
the current directory happened to contain hardlinked default files named
`netta0.state` and `netta0.bio.tsv`: life-only alias inspection ran before the
supposedly state-free ear.

All three are repaired without changing the state.  Ear mode now has one
failure-closed invocation law: shores, `--ear`, and the later explicit ear
context are its only inputs; every named life or mouth control is refused,
even when its value equals the default.  The ear enters immediately after
the immutable shores are loaded, before any state, biography, island-route,
or ambient-default query.  A one-byte overread distinguishes the admitted
16384th byte from a refused 16385th byte without truncation.  The descending
substring recurrence was attacked independently at both tape boundaries,
at lengths fifteen and sixteen, and through NUL; each exact verdict survived.

The sorted red arm also received its missing jurisdiction.  Sorting a
period-3 slice raises its price from 0.457849 to 8.000823 bits per byte, but
on a block-sorted `a...ab...b` shore the sorted candidate costs 3.230624
against 7.208411 for an alternating equal-census twin.  An ear hears order;
it does not promise that one operation destroys order on every world.

The deeper refusal was semantic.  The preregistered two-shore hearing below
found seventeen consecutive spaces shared by a freely generated technical
stream and its shore.  The DP was exact; the noun was not.  Substring proves
shared bytes but cannot alone establish the causal act of quotation or name
a parrot.  The live interface therefore reports `longest-match` and
`matched16`.  Body 20's original names remain above as an auditable claim
that failed.  No replacement threshold is chosen after seeing the spaces;
a future copying court owes a preregistered complexity control.

## Body 21: the ear remembers the question

The cold ear's first byte is charged by the unigram book, its second by the
bigram book, and only its third enters the trigram book.  That is a valid
unprompted instrument, but it is not a neutral detail when the mouth was
asked a question.  On the preregistered `a...azzbbbb` counterworld, cold `aa`
beats `bb` at 1.836756 against 6.097363 bits per byte.  After the explicit
context `zz`, the verdict reverses: `bb` costs 7.005625 and `aa` 8.002812.
The warmup can change an ordering, so it must be carried or named.

The twenty-first body adds `--ear-context P`.  The named file must contain at
least two bytes; exactly its final two become the initial trigram context and
their hexadecimal identity is printed in every verdict.  No life state is
opened and no hidden most-lived context is borrowed.  Prefixes before the
last two bytes are proved irrelevant bit-for-bit.  Without the flag, the
body-20 cold ladder remains exact.  Both instruments are pure, read-only,
and powerless; body 21 supplies experimental context, not a court.

## The second and third hearings

Before either result existed, `research/EAR_AUDIT_2026-08-16.md` registered
the two shores, life and speech seeds, lengths, hands, laws, context, expected
ordering, and failure meaning.  The published normalized Frankenstein and
technical SHA-256 witnesses were rechecked.  Independent 384000-byte lives
then spoke 300 bytes from locked uni, bi, and tri under supported and Laplace
laws, prompt `The ` and speech seed 7.  The context-bearing local ear heard:

| shore | supported tri | supported bi | supported uni | Laplace tri | Laplace bi | Laplace uni |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Frankenstein | 2.910738 | 4.992303 | 6.995211 | 8.002758 | 5.550433 | 7.163641 |
| technical | 3.134352 | 5.268368 | 7.563339 | 7.827882 | 6.320897 | 7.617944 |

Both preregistered price ladders pass, and every supported hand beats its
matched Laplace twin.  The Dracula ordering is therefore no longer a
one-shore accident: three real-text shores agree that supported tri is heard
below bi below uni under their own books.  Three English byte worlds are not
language in general, and the instrument promotes nobody.

The preregistered no-match claim fails on technical tri: longest match
seventeen, matched-16 coverage 5.7 percent, exactly seventeen spaces at
speech offset 223 and shore offset 8956.  Frankenstein clears with longest
match ten; all other new streams remain below sixteen.  The complete literal
table lives in `research/ear_results/2026-08-16-cross-shore.tsv`.

One hundred and ninety gates now pass.  The inclusive cap and loud
overflow, binary substring boundaries, ambient-state independence, full
ear law mask, opposed sorted-world reds, explicit-context reversal and
suffix identity, refusal of underspecified context, and ASan/UBSan through
the context-bearing ear join the inherited constitution.  The exact matcher
is now linear in shore plus candidate length rather than their product; a
full 16384-byte sitting against the 855114-byte Dracula shore completes at
the public bound instead of performing roughly fourteen billion DP cells.
State stays v20.

## The context-bearing ear audited

The ninth turn was reproduced from zero: 190 gates with a fresh binary
before its diffs were read.  The suffix automaton was then given a second
hand: an independent brute-force longest-common-substring probe, built
outside the repo, compared summary-for-summary against the shipped binary
on two hundred deterministic random world/stream pairs — a quarter of them
squeezed into a four-letter alphabet so real multi-byte matches occur — and
on the shaped worlds: a uniform tape that forces clone chains, alternating
offsets, all 256 byte values including NUL, the stream that is the shore's
own tail, and the full public bound, 16384 candidate bytes against the
855114-byte Dracula shore.  Two hundred and five comparisons, zero
mismatches, longest-match 16384 exact on the self-slice.  A convoy of
thirty-two shores heard one candidate in one invocation, thirty-two
verdict lines, and the same run under ASan/UBSan wrote nothing to stderr.
The invocation law held against all twenty-four life and mouth controls —
twenty-four refusals, zero holes; duplicate mode flags resolve last-wins
exactly as every other flag; a shore may hear itself and a stream may be
its own context; and the verdict is identical from a foreign working
directory.

## The structural red, preregistered and landed

Before the run, the prediction: on a shore drawn uniformly over all 256
byte values, the locked-tri supported mouth would price below uni on the
shore's own ear while carrying a matched-16 census above thirty percent,
because on a structureless world every trigram row holds exactly one lived
continuation and supported speech can only replay the tape.  The measured
verdict was harder than the prediction: tri 7.01 bits per byte with
longest-match 300 of 300 — matched-16 coverage 100 percent, a total parrot
— bi 7.53 with longest-match 11, uni 8.00 with longest-match 2.  The
price ordering tri < bi < uni, the same ordering the three English shores
honour, is reproduced exactly by pure copying on a shore with nothing to
learn.  Price alone cannot tell discipline from theft; the match census
alone cannot tell theft from shared habit; the pair is the minimal honest
verdict.

For any future copying court the ruler is preregistered here, before any
candidate is read: a stream offered to such a court is measured by the
triple (bits per byte on the true shore's ear; matched-16 coverage against
the true shore; bits per byte on the ear of the shore's sealed shuffled
twin).  Speech that carries the shore's order prices low on the true ear
and high on the twin; a census-only stream prices alike on both; a copied
stream is named by the match census regardless.  No threshold is chosen
today, no court is seated, and no ear number grants authority.

## Body 22: the question's law

The mouth accepted what the ear refuses.  A prompt of one byte left the
second context position holding a byte of the most-lived opening — a
hidden cold byte inside the question, the exact class the contextual ear
was built to refuse — and an empty prompt file was a silent cold opening
pretending to be a question.  The old law accepted both; the red run
against the previous binary shows the one-byte question passing with rc 0.

The twenty-second body makes the question one law everywhere: a question
is at least two bytes, or it is absent and the opening is named cold.  The
mouth now refuses a prompt of fewer than two bytes by name, exactly as the
ear refuses its context.  A refused question leaves no fingerprint on
state or biography.  Four gates seal it: the one-byte refusal by name, the
empty-file refusal, the two-byte red arm proving the refusal can fail, and
the memory-silence of a refused question.

## The question law audited and repaired

The tenth turn began from Don's untouched `14ae402`: all 194 gates were
rebuilt and replayed before its diff was read.  The new refusal was correct,
but mouth and ear carried the same rule in two independent loops, and a
regular context file had no witness that its relevant tail stayed stable
while it was read.  The old binary was then challenged with an exact two-byte
question, a longer equal-suffix file, a FIFO, `/dev/stdin`, an eight-megabyte
file, and a one-byte FIFO.  Every complete `ab` source produced identical
speech and hearing; EOF made the short stream refuse by name.  The transport
law was sound, but duplicated and under-specified.

One `question_tail` reader now serves both public verbs.  A regular file is
read in constant prefix-space and constant time with respect to its irrelevant
prefix: its final two bytes are read twice while the open-file identity and
length remain fixed, and a changed tail is refused.  A FIFO or other
non-regular source remains a stream whose question is sealed only by EOF.
The names stay distinct on purpose: `--prompt-file` warms a lived mouth;
`--ear-context` warms a stateless measurement.  They share a byte law without
pretending to share jurisdiction.  Three permanent stream gates join the four
refusal gates, and all memory-silence invariants remain unchanged.

## Body 23: every island casts a structural twin

`research/STRUCTURAL_TWIN_AUDIT_2026-08-16.md` was sealed before the first
new hearing.  Under explicit `--ear-twin`, every immutable shore now grows an
in-memory twin with the already-published Gutenberg recipe: descending
Fisher--Yates, SplitMix64 seed `0x4e45545441475241`, and `draw mod span`.
Length and the complete 256-byte histogram are checked before a verdict.  The
twin digest and changed-position count are printed, so the null cannot claim
to have destroyed structure it did not change.  The independent
`garena_prep.c` hand reproduced the in-memory twin's digest and price exactly.
On the period-3 literal slice the true ear costs 0.457849 bits per byte,
matched-16 coverage is 100 percent, and the shuffled-twin ear costs 2.136573.
On a 4096-byte constant shore, zero positions change and both prices are
0.087149.

The uniform red now has a permanent portable generator,
`scripts/uniform_shore.c`.  SplitMix64 seed `0x534f4c554e49464f` produces a
4096-byte shore with SHA-256
`8bf24111cf50bd943c8088991aa8799e8be06b0143fc1ed08e2f8e687139db8e`.
A single life saw 302 transitions from offset 16; prompt bytes were offsets
14--15; locked mouths emitted 300 bytes at seed 7.  The preregistered failure
landed exactly:

| hand | true bits/byte | longest match | matched-16 | twin bits/byte |
| --- | ---: | ---: | ---: | ---: |
| uni | 8.000412 | 2 | 0.0% | 8.000262 |
| bi | 7.405849 | 10 | 0.0% | 8.000300 |
| tri | 7.005793 | 300 | 100.0% | 7.997098 |

The supported tri hand used trigram support on all 300 draws and replayed the
tape verbatim.  Its cheap local price is therefore not evidence of abstraction;
the true match and twin price expose the mechanism.  Body 23 completes Don's
preregistered triple — true-shore price, true-shore matched-16 coverage, and
shuffled-twin price — without choosing a threshold or seating a copying
court.  The ear still opens no state, writes no file, enters no election, and
grants no authority.  State remains v20.  Two hundred and six gates, including
the independent hand, the degenerate null, uniform replay, invocation law,
and ASan/UBSan through the twin ear, pass.

## The structural twin audited

The tenth Sol turn was reproduced from zero: 206 gates with a fresh
binary before its diffs were read.  The twin was then given an
independent second hand at every length class: shores of two, three, a
prime 257, the full 256-value binary alphabet, and the whole Dracula
body all grow byte-identical twins under `--ear-twin` and under the
sealed external `garena_prep.c shuffle` — same digest, same price to
six decimals (3.970000 both ways on the Dracula slice), and the census
conservation was re-counted outside the organism.  A zero-length and a
one-length shore name their own degeneracy: `twin-changed=0/0` and
`0/1`.  Thirty-two shores heard a full 16384-byte candidate with twins
under ASan/UBSan in one invocation — silent — and a shore's verdict
line is byte-identical alone and inside the convoy: one shore's twin
cannot contaminate the next shore's books.  The null was pressed where
it must stay quiet: on a repeated-boilerplate shore the twin price
climbs from 1.85 to 7.95, while on a high-entropy shore the twin
prices marginally below the true shore (7.998 against 8.001) — the
null does not always speak, and no prose anywhere claims it does.  The
question reader survived a regular stdin, an eight-megabyte prefix in
under a second, and a one-byte FIFO with a late-closing writer refused
by name after EOF.  The uniform shore was regrown from its published
generator: shore SHA-256 and the sixteen-byte vector match the record,
and all three locked speech streams reproduce their published SHA-256
witnesses bit for bit on a rerun.

## Body 24: the pattern court

Mouth, ear, context, and twin exist; the ruler triple is sealed.  The
twenty-fourth body seats the court that reads the triple — and it is
named the pattern court, because its verdicts are measurement
patterns, never causal accusations: the independent-cycle world below
proves that a hand which never saw a shore can still replay it.

The law was fixed before the calibration worlds were measured, in this
turn's preregistered checklist: the court hears a candidate stream on
every shore of the convoy and returns exactly one verdict from a
four-word lattice, evaluated in order.  ABSTAIN when the shore's twin
changed nothing — a shore whose null cannot move refuses structural
jurisdiction.  REPLAY when matched-16 coverage reaches the frozen
threshold: the stream is largely literal shore substring.  ORDER when
coverage is below that threshold but the structure gap — twin price
minus true price, in bits per byte — reaches its frozen threshold: the
stream carries shore order it did not literally copy.  STRANGER
otherwise.  The court is a trigram court: a stream carrying only the
byte census prices near ignorance at trigram depth and is honestly
read as a stranger; a census court would need its own ruler and its
own body.

Calibration, measured with the sealed ear before any threshold was
frozen: the uniform replay world P=7.008 M=100.0 G=0.988; the
independently written period-3 cycle P=0.894 M=100.0 G=1.849 — a
replay pattern from a hand that copied nothing, the standing proof
that pattern is not cause; the Dracula elected-tri stream P=2.995
M=0.0 G=1.564; noise on Dracula P=8.067 M=0.0 G=-0.007; the
constant shore twin-changed=0; the boilerplate world M=100.0 G=6.097;
a synthetic order world of eleven lived bytes and one foreign byte
repeated, P=2.715 M=0.0 G=1.371.  From these the thresholds are
frozen with wide margins: matched-16 at 50 percent (replay worlds sit
at 100, order worlds at 0) and structure gap at 0.5 bits per byte
(order worlds at 1.37-1.85, the stranger at -0.007).  A straddle pair
on the period-3 shore proves the match threshold bites: 73.7 percent
coverage is REPLAY, 36.8 percent with gap 0.671 is ORDER.

The court holds no office: it wires into no election, grants no speech
authority, writes nothing, opens no state, and refuses every life,
mouth, ear, and twin control by name — a court invocation is shores,
`--court`, and an optional explicit context, nothing else.  State
remains v20.

## Body 25: the court's public warrant

The eleventh Sol audit began from Don's untouched `f38c5cf`: a fresh strict
binary reproduced all 220 inherited gates before the diff was read.  The
four-word court survived its fifteen-byte quilt, equal-census, and explicit
context attacks, but failed two questions about its own jurisdiction.

At the fifty-percent boundary, a candidate with exact coverage 1000/2000 was
REPLAY and one with 1000/2001 was ORDER, yet both public lines rounded the
operand to `matched16=50.0%`.  The private comparisons were correct and the
public evidence was not: identical visible premises appeared to yield
different judgments.  More deeply, a 4096-byte shore containing one `b`
among 4095 `a` bytes grew a twin with two moved positions, but that twin and
the true shore priced the heard 300 `a` bytes identically at 0.087545.  The
hidden `changed != 0` door nevertheless let the court say REPLAY.  A null that
moves irrelevant bytes has not earned structural jurisdiction over the
coordinate being judged.

Body 25 is a narrow first clause of the future earning law, not a grant of
power.  The four words and body-24 thresholds remain frozen, while every
deciding operand becomes public: P, twin price Q, exact
`matched-bytes/candidate-bytes`, integer `gap-micro`, and exact
`changed/shore-bytes`.  The court rounds the measured total gap once to its
already promised six-decimal resolution and decides entirely on integers.
It ABSTAINS if the twin changed no shore byte or if `gap-micro == 0`; otherwise
it says REPLAY at `2 * matched-bytes >= candidate-bytes`, ORDER at
`gap-micro >= 500000`, and STRANGER otherwise.  This adds no post-hoc moved
fraction: the falsifier must simply move the coordinate it claims to test at
the public ruler's resolution.

The preregistered boundary worlds landed: 50.0000% (1000/2000) remains REPLAY,
49.9750% (1000/2001) remains ORDER, and their exact public premises differ.
The one-`b` shore reports `changed=2/4096`, `gap-micro=0`, and now abstains.
The fifteen-byte quilt remains ORDER; the equal-census stream remains the
trigram court's honestly named STRANGER; and explicit context `6362` still
changes candidate `ab` from cold STRANGER to contextual ORDER while naming
that jurisdiction.

One prediction was paid for in public.  The old all-`z` stranger world also
has P=Q=8.013738 and gap-micro zero despite 2822/4200 moved shore bytes, so
the new law correctly changes it to ABSTAIN.  The preregistration remains in
`research/PATTERN_COURT_AUDIT_2026-08-16.md` with the failed prediction and
its correction; no threshold was bent to save an inherited label.

Two hundred and twenty-five gates pass, including the five new warrant
worlds and ASan/UBSan through the court.  The court still accepts only shores,
`--court`, and optional explicit context; it opens no state, writes nothing,
enters no election, and grants no authority.  State remains v20.  A complete
law of earning remains external work: preregistration receipts, independent
review, red-world scope, a named requested power, revocation, and a separately
enacted activation body cannot be self-attested by this instrument.

## The public warrant audited

The eleventh Sol turn was reproduced from zero: 225 gates with a fresh
binary before its diffs were read.  An independent hand then reparsed
every court line of eleven hearings — all nine calibration worlds plus
the all-`z` and equal-census witnesses — and recomputed every verdict
using only the printed operands: eleven agreements, zero divergence.
The integer law bites exactly where it is written: a candidate with
matched bytes 200 of 400 is REPLAY at the tie and 200 of 401 is ORDER
one byte below it; a NUL-bearing candidate keeps its exact fraction; a
cold `ab` at gap-micro 265747 is STRANGER while the same two bytes
under an explicit `cb` context reach gap-micro 1951844 and ORDER — the
warrant names the jurisdiction that moved the verdict.  The two
abstention witnesses stay distinct in the operands themselves: the
all-`z` stream abstains at gap-micro 0 with 1226 moved shore bytes,
while the equal-census stream is a STRANGER at gap-micro -3052.  A
32-shore convoy leaks nothing — line seventeen is byte-identical alone
and in company — verdicts repeat bit for bit, sanitizers are silent
through the full convoy, and the mask refuses ear, mouth, twin, and
every life control.

## Body 26: the warrant's receipt

The court publishes its operands; the twenty-sixth body makes that
publication independently checkable.  This is the second clause of the
earning law, preregistered in this turn's checklist before a line of
it was built: a verdict is citable only through a receipt that an
external hand can recompute, and every warrant names the law it was
judged under.

Each court invocation now opens with the law itself: the canonical
one-line text of the lattice and its frozen thresholds, followed by
its FNV-1a-64 law-digest.  Every verdict line carries a receipt: the
FNV-1a-64 of that line's canonical operand text joined with the
law-digest.  A silently amended threshold changes the canonical text,
therefore the law-digest, therefore every receipt: no verdict can
pretend continuity with a law it was not judged under.  The external
hand is `scripts/warrant_check.c` — an independent implementation
that parses a court transcript, re-derives every verdict from the
printed operands alone, recomputes every receipt, and refuses the
transcript on any divergence, naming the line.

The receipt authenticates the tuple; it does not replace it, and it
grants nothing.  Its honest limit is also named here: FNV is a
witness, not a signature — it catches transcription, tampering, and
law drift, not an adversary who rebuilds the hash from source.  A
sealed receipt against that adversary needs a key and a key holder,
and those are historical and social facts no organism can self-attest
from inside its own file.  Activation authority remains a later body.

One honest note on defense in depth: on a receipt-valid transcript the
external hand's verdict re-derivation can never fire alone, because the
receipt already binds the verdict word into the hash.  The re-derivation
exists for the organism that seals its own error consistently, and it
did real work in this audit's first act, where eleven verdicts were
recomputed from operands before any receipt existed.  Two hundred and
thirty-four gates now pass: the external hand's strict build, a
five-hearing acceptance, the one-law rule, and five red arms — tampered
operand, tampered verdict, tampered receipt, amended law, and a verdict
without a law — join the constitution.

## Body 27: the warrant's sitting docket

The twelfth Sol audit began from Don's untouched `81bc616`: all 234 inherited
gates passed before its diff was read.  The organism's line receipt and the
external hand's independent FNV implementation survived.  The book around
those leaves did not.

One two-shore invocation was rewritten four ways without changing a single
leaf or receipt: one verdict dropped, one duplicated, the pair reversed, and
one leaf replaced by a valid receipt from a different candidate.  Body 26
accepted all four books.  It also accepted publicly re-sealed lines containing
`gap-micro=garbage`, no P/Q/matched16/G fields, `matched-bytes=0/0`, or two
gap operands.  Finally, any foreign law text could borrow the hand's hardcoded
v2 semantics when its public digest and receipts were rebuilt; a mixed-law
book was reported wholly under its final digest.  None is a cryptographic
break.  Together they locate the missing object: a receipt line is not a
complete sitting, and a checksum is not a grammar.

Body 27 makes one hearing citable without yet letting it act.  After the exact
v2 law, each invocation publishes a sitting header with FNV identity of the
heard candidate bytes, candidate length, explicit context, shore count, and
law digest.  Court ids must then run from zero through that declared count.
The close publishes a docket digest over the exact header and every complete
verdict line including its body-26 receipt, in order, with newline record
boundaries.  The old leaf receipts do not change; the docket binds their
membership, order, and candidate-bearing frame.

The independent hand is now a strict state machine over complete sittings.
It carries its own exact copy of the v2 law whose semantics it implements,
accepts multiple separately closed sittings and CRLF transport, and refuses
unknown, overlong, partial, duplicated, out-of-order, or out-of-frame records.
Every court field occurs once in canonical order and consumes the whole line;
integers, fractions, decimals, context, candidate length, court id, count,
verdict, receipt, and docket must agree.  The test-only
`scripts/warrant_fixture.c` can publicly rebuild both receipt and docket.  It
exists to prove that re-sealed malformed lines are stopped by visible grammar,
not by pretending the FNV witness is secret.

The preregistered predictions held.  A two-shore hearing over the 300-byte
candidate names `candidate-digest=ca0d16aadc050d91` and closes at
`docket=e2898cdce3dfbe2d`; the fixture independently reproduces candidate
identity, and the leaf receipts remain `fd8b7974e021a5a9` and
`e98c2a2a2aedfae9`.  Drop, duplicate, reverse, and cross-candidate splice all
fail.  Re-sealed garbage integer, missing human operands, zero denominator,
duplicate operand, overflowing integer, foreign law, bad receipt, unknown
record, 1023-byte record, and premature EOF all fail.  Two complete sittings
and CRLF survive.

Two hundred and fifty-five gates pass, including a sanitizer-silent strict
hand on both honest and crafted input.  State remains v20.  The court still
opens no life, writes nothing, elects nobody, and grants no speech or action
authority.  The docket completes the mechanical prerequisite for a future
biography to cite a court sitting as an event; it does not manufacture an
independent reviewer, key holder, requested power, revocation law, or
activation body.  A stronger checksum cannot create those social facts.

## The sitting docket audited

The twelfth Sol turn was reproduced from zero: 255 gates with a fresh
binary before its diffs were read.  The first duty was recomputation
without her fixture: a fresh minimal hand, written outside the repo,
recomputed the candidate digest from the raw candidate file, both leaf
receipts, and the ordered docket fold of a two-shore sitting —
candidate a8bfae544bd5e579, docket 8ce3006a46a6bfe5 — all byte-equal
to the organism's own print.  Eleven framing attacks then behaved
exactly as the law demands: two concatenated complete sittings are
accepted, an incomplete tail, a dropped leaf, a duplicated leaf,
reordered leaves, a cross-candidate splice, a duplicated sitting
header, a court record before any law, a record longer than the
buffer, and a final record without its newline are each refused; a
CRLF transcript is tolerated by name.  Four numeric adversaries on
the leaf — a signed plus, a twenty-digit overflow, a NaN, and a
leading zero — die in the grammar before any hash is consulted.  The
decisive attack was the public constructor: using only the published
fixture, a foreign-law sitting was built that is fully
hash-self-consistent under its own amended law — its law digest, leaf
receipt, and docket all verify — and the checker refuses it by
grammar: the law is not canonical v2.  Security rests on the grammar,
not on any secret, exactly as claimed.

## Body 28: the citation — preregistered before code

The docket survived, so the twenty-eighth body may exist: the first
time a court's word touches the life — as memory, never as power.
The grammar was fixed in this turn's preregistered checklist before a
line was written:

A citation is one biography line of type `w`:
`w  episode  candidate-digest  bytes  context  shores  law-digest
docket  verdicts` — the life records that a complete sitting said
something about a candidate stream, and where that judgment lives.
The verdict words themselves stay in the docket; the biography holds
the pointer, not the power.

The law of the event: a life cites only what it can itself re-derive.
`--cite FILE` re-verifies the whole single sitting with the life's own
hands — the banner, the law line, which must be this life's own
canonical law in both text and digest, the sitting header, every leaf
receipt, the ordered docket fold, and the close.  Any failure refuses
by name and leaves no trace in the biography.  A newborn is refused:
a life must exist before it can remember what was said about it.  The
citation does not claim authorship — the docket names no life, so the
event records an external fact about a candidate stream, and binding
speech to speaker remains a future body.  Duplicates are two events:
the operator showed the record twice, and the biography says so, each
under its own episode stamp.  The line enters the same hash chain and
the same two-object publication law as every other biography event.
And the event is powerless by construction: nothing reads `w` lines
back into behaviour, which a twin gate proves — a life that cited and
its twin that did not play the next episode to bit-identical prices.
State remains v20.

The citation lived on first breath and its gates seal the
preregistration exactly: one verified sitting becomes one biography
line and a repeat is a second event; a cited life resumes and plays,
and its uncited twin prices the next episode bit-identically at
1.020237 bits per raw byte; a forged fold, a truncated close, a
foreign law, a second sitting, a newborn, and every play or instrument
flag are refused by their preregistered names, each leaving the
biography byte-identical.  Two hundred and sixty-eight gates pass,
sanitizers silent through the citation.  For the first time a court's
word stands inside a life's own hash-chained memory — and the life
itself proved every hash before letting it in.

## The citation audited and repaired: a witness is not a grammar

The twelfth Don turn was reproduced untouched before its diff was read:
268 gates and the citation sanitizer all passed.  The broader law held.
The citation was memory-only, the invocation mask was separate from play,
and a valid docket entered one `w` line without changing state v20.  But
the life's claim to have re-derived the whole sitting did not yet hold.

The external hand parsed the canonical meaning of every court record.
`cite_verify()` checked the exact law, ordered ids, leaf receipts, docket
fold, and close, but treated the bytes before a valid receipt as opaque.
A public hand therefore built `court 0: canonical-shape=false
verdict=replay`, computed its correct receipt and close under the real
law, and presented one file to both readers.  The external checker
refused line four.  The life accepted it and appended a citation.  The
same gap admitted a short candidate digest, signed and zero-padded byte
counts, an undeclared context, and a canonical header whose context or
byte count disagreed with its leaf.  Every public hash was correct; the
language was not.

The repair leaves the court and the record unchanged.  Netta now carries
an independent strict parser for the same body-27 language as the
external hand: fixed-width lowercase identities, canonical bounded
integers, `cold` or four-hex context, complete finite decimal operands,
header/leaf equality, exact coverage and gap witnesses, the verdict
derived from the integer law, the receipt, and the ordered docket fold.
The two readers still share no code.  The six measured counterexamples
now refuse before the biography is opened.

The surrounding claims were widened rather than remembered.  CRLF remains
legal; overlong, binary-interrupted, and every incomplete newline boundary
refuse.  All twenty-seven non-file controls were swept one by one and left
the biography unchanged.  The citation's inherited two-object publication
debt was made explicit: a biography one line ahead of state and a state one
line ahead of biography both refuse resume by chain and line count.  This is
fail-closed, not an atomic transaction and not an automatic recovery law;
body 28 neither hides nor repairs that older debt.

The powerlessness twin now crosses two resumes, seven further episodes,
2400 lived bytes per arm, units, jury, atlas choice, and a reversed convoy.
Both arms print byte-identical output; after removing the one `w` event their
biographies are identical, and their final states differ only in the stored
biography count and chain.  A cross-life citation remains legal and honestly
external: the docket names candidate bytes, not a speaker, so adding a
`self` or `other` word would invent knowledge.  Authorship remains future.

Two hundred and seventy-five gates now pass, strict builds are silent, and
ASan/UBSan remain green.  Body 29 is not opened: the repaired reader returns
to the other hand before any new organ is named.

## The repaired citation audited: two readers, one language

Sol's repair `51e87a6` was reproduced untouched before its diff was read:
275 gates, strict build silent, sanitizers green.  Her finding was then
rebuilt by hand rather than trusted from the log.  The record `court 0:
canonical-shape=false verdict=replay` was given its correct public
receipt under the canonical law and a correctly recomputed close, and one
file was shown to three readers.  The external hand refused line four.
The citation of `98f9162`, my own untouched body-28 commit, accepted it,
printed `cited`, and changed both the biography and the state.  The
citation of `51e87a6` refused it by name and left both files
byte-identical.  The defect was real, and the repair is real.

The law she wrote for this turn — the life and the external hand must
agree on every record — was then measured instead of restated.  A
cross-reader battery of 215 transcripts was built from four canonical
sittings (one, two and thirty-two shores, and a context-bearing one):
every numeric field of the header and of a court leaf at its boundaries
(leading zero, sign, decimal and hexadecimal forms, overflow, the
`[2, 16384]` and `[1, 32]` limits at both edges), every hexadecimal
identity at 15 and 17 digits and in upper case, the context word in every
wrong shape, the law's own thresholds (gap-micro 499999/500000/-500000/0/1,
coverage 113/114 of an even and of an odd byte count, changed 0 with every
verdict word), the verdict word contradicting the law, receipt and docket
variants, close variants, and transport — CRLF, bare CR, `\r\r\n`, NUL
inside and at the head of a record, an overlong record, a byte-order mark,
a missing final newline at every stage, blank records, a repeated law,
a repeated sitting, a court before its sitting, thirty-three leaves.  The
public fixture re-derived every receipt and every close, so the readers
were judged on grammar and law rather than on a stale hash; twenty-nine
records were built to be legal.

Both readers accepted the same 29 and refused the same 185, and their
refusal stages align record by record: the hand's line number and the
life's named reason point at the same place in every case, with one
harmless offset — when the header undercounts its shores, the life reads
one more valid leaf and refuses at the close, while the hand refuses at
the leaf.  No refusal left a trace: 186 refused citations, 186 states and
biographies byte-identical.  Exactly one record divides them, and it is
named: two complete sittings in one file, which the external hand accepts
as a transcript and the citation refuses because a `w` line points at one
sitting.  That is a difference of office, not of language.

The same battery run against `98f9162` disagrees with the external hand
on 94 of the 215 records: the old citation admitted a verdict word
contrary to the law's own arithmetic, signed and zero-padded counts,
upper-case identities, five- and seven-decimal operands, a matched
percentage that did not follow from its fraction, and a coverage larger
than its denominator.  The battery now lives in `scripts/cross_reader/`
with its four base sittings and runs as one gate on every suite pass, so
that the two-object debt she named — one language, two parsers — has a
standing witness rather than a remembered one.  Two hundred and
seventy-six gates.

## Body 29: speech names itself — preregistered before code

Preregistered before a line was written: after speaking, the mouth states
one canonical manifest on stderr — `spoke: candidate-digest=… bytes=…
seed=… hand=… context=… lived-bytes=… episode=…` — where the digest is
the same FNV over exactly the emitted bytes that the court uses to name a
candidate.  The mouth's write-nothing law is untouched: state and
biography stay byte-identical.  The manifest is deterministic; the
external hand recomputes its digest over the captured stream; another
seed states another digest; the red Laplace mouth states the same shape;
and no `self` or `other` word is introduced.  The manifest is a fact of
the speaker at the moment of speech, not a field of any docket: nothing
reads it back, and the docket keeps naming candidate bytes rather than a
life.  Sol drew the boundary — the current docket has no speaker fact —
and the manifest creates a speaker fact without touching the docket.

The manifest lived on first breath.  On the two-shore life used through
these turns, `--speak 60 --speak-seed 7` states
`candidate-digest=796e0c5ce4b27b99 bytes=60 seed=7 hand=tri context=6162
lived-bytes=1200 episode=2`; the fixture recomputes `796e0c5ce4b27b99`
over the sixty captured bytes, and the pattern court over the same file
opens its sitting with the same candidate digest.  The court and the mouth
now call one stream by one name, from two sides.  Two red hands were
built before the gates were trusted: a mouth whose digest starts from the
witness seed states `8ff14ed28afa975e` for the same speech and fails the
recomputation gate, and a mouth whose manifest carries a different prefix
fails the shape gate.  One property of that life is worth recording: with
sixty of sixty bytes drawn at tri support and one lived continuation per
context, seeds 7 and 8 state the same digest — the seed drew nothing — so
the negative case is gated on the wider-alphabet life where body 18
already proved that seeds differ.

Eight gates seal the preregistration: the canonical shape, the untouched
memory, the identical repeat, the recomputed digest over exactly sixty
bytes, the court's agreement on the name, the hand that spoke, the
different seed, and the red Laplace shape; the sanitizer mouth is silent
with the manifest filtered by name.  Two hundred and eighty-four gates
pass.  Authorship — a docket that names its speaker — remains the next
body, and it now has a fact to bind: the mouth signs what it says.

## The speech manifest audited: what an outside hand can know

Don's `8eecfd6` was reproduced untouched before its implementation was read:
284 gates, strict build silent, sanitizers green.  The manifest's first
grammar then needed two corrections while they were still cheap.  Its
`context` was not the court's external listening context but the two byte
positions where the tongue began, so the field is now `opening`.  And an act
of speech was incompletely named without its law: supported backoff and the
Laplace-red arm can differ with the same state, seed, hand, and opening.  The
canonical line now includes `law=supported-backoff|laplace-red`.

The new `scripts/manifest_check.c` is an outside reader with no organism code.
It takes exactly one newline-sealed canonical line and the captured speech
file, then independently re-derives the count and FNV candidate digest.  CRLF
is transport; fragments, a second record, NUL, long lines, noncanonical
integers, wrong widths and enum words, a different count or digest, and a
different stream all refuse.  Zero speech is a first-class stream and carries
the public empty FNV witness `cbf29ce484222325`.  Strict and sanitizer builds
are silent.

The reader's most important gate is a deliberate acceptance.  Seed, hand,
law, opening, lived bytes, and episode can all be restated in another
canonical spelling while the captured bytes and digest remain unchanged; the
reader accepts and prints only the two facts it actually proved.  Those six
fields remain statements by the mouth.  A grammar reader cannot manufacture
authorship, and the manifest still enters neither behavior nor biography.

The cross-reader battery grew from 215 to 230 records.  Fourteen new cases put
32 characters beyond each bounded numeric-token class.  One new positive
case combines canonical leaves from two independently printed worlds and
re-seals the public book; both readers accept it, showing that no unprinted
shore list hides in either parser.  Before mutation the battery now rebuilds
three base sittings with Netta herself from the period-3 shore, a disjoint
period-3 shore, both 228-byte candidates, and an explicit opening, and demands
byte-identical output.  The repaired result is `transcripts=230
both-accept=30 disagree=1 [S14_two_sittings] trace-leaks=0`: 199 records are
refused by both and the one difference of office is unchanged.  The expanded
battery gives the pre-repair `98f9162` 105 disagreements, eleven more from the
long operand cases.

The seed observation is now executable rather than anecdotal.  Every observed
pair in the narrow period-3 source has one continuation; seed 7 and seed 8
therefore say the same sixty bytes with `tri 60`, while the wider world still
gives distinct digests.  The mouth is behaving according to its lived graph.

For body 30, the roads no longer look equal.  An external manifest cannot be
re-derived after the life has continued, and a pure shore court cannot name a
speaker from an unproved statement.  If authorship is opened, the honest road
is an `s` event written by the life itself: generation may remain behaviorally
read-only, but the act of speaking becomes an explicit biography/state
publication carrying the manifest's exact fields.  That changes the mouth's
storage constitution and inherits the two-file publication debt; its crash
law must be preregistered before code.  It would bind causal authorship inside
one biography, not create a cryptographic signature.  No part of body 30 was
implemented in this turn.

Two hundred and ninety-four gates pass.  State stays v20, strict builds are
silent, and ASan/UBSan cover the independent manifest reader as well as the
mouth.

## The manifest reader audited: one grammar, two hands

Sol's `bc103b6` was reproduced untouched before its diff was read: 294
gates, strict build silent, sanitizers green.  Her two renamings stand on
their reasons — the mouth's `opening=` is not the court's `context=`, and
the speech `law=` is part of the act that produced the stream — and her
reader `scripts/manifest_check.c` says exactly what it can prove: the
count and the digest of the captured stream, nothing about seed, hand,
law, opening, lived bytes or episode, which remain statements of the
speaker until a body gives an outside hand the state to re-derive them.

The reader was then met by a second hand rather than reread.  Fifty-two
manifests were built from one canonical line: the identity at 15, 17
and upper-case digits and flipped by one; every count with a leading
zero, a sign, a 32-character token, an empty token, at the unsigned
64-bit maximum and one past it; the hand as `Tri`, `mv`, `null`,
`trix` and empty; the law shortened, capitalised, lengthened and with
an underscore; the opening at three, five and upper-case digits and as
`cold`; float and hexadecimal counts; a capitalised and a renamed
prefix; a double space, a tab, a leading and a trailing space, an
extra and a missing field, two fields swapped; CRLF, `\r\r\n`, a
missing final newline, a trailing blank record, two records, a NUL in
the middle, an overlong record, an empty input, a lone newline and a
byte-order mark.  An independent hand — a full-line regular grammar
with an explicit unsigned range check, transport rules and the stream
witness through the public fixture — agreed with the reader on all 52,
accepting exactly four: the canonical line, its CRLF form, the line
with every speaker-only field restated, and the seed at the unsigned
maximum.  The one disagreement of the first pass was the hand's own:
its NUL detection did not exist on this platform's grep, so it read
the record with the NUL removed and accepted; the reader had refused
the record as unsealed, which is the law.  The hand was corrected and
the pass repeated.

Her question about `base32.t` is answered in the battery: the
maximum-shore fixture is now replayed before any record from a formula
rather than a stored world — shore i of thirty-two carries 33+13(i−1)
bytes with byte k = ((2i+1)k + i) mod 256, the candidate 300 bytes
with byte k = (11k + 5) mod 256 — and the fresh court output must be
byte-identical to `base32.t` or the battery exits before emitting
anything, which a corrupted copy of the fixture proved (rc 2, zero
records).  The court of `98f9162` prints the same fixture byte for
byte: the court has not moved since.  The regenerated fixture opens on
candidate `53d2c84fac9d4eb5`, thirteen abstentions and nineteen
strangers; the battery's verdicts do not depend on its contents, and
its summary line is unchanged — 230 records, 30 accepted by both, 199
refused by both, the one named difference, no trace — while the
expanded battery against `98f9162` disagrees on 105, as she reported.
Two hundred and ninety-four gates.

## Body 30: the signature — preregistered before code

Oleg's word opened the thirtieth body with a condition: when speech becomes
a biography event, Netta for the first time does not merely produce a
sound but can say that this act belongs to its own lived causal line — and
the price is real, the mouth stops being storage-read-only and a
publication debt appears between state and biography, so crash
consistency may not be waved away.  The preregistration therefore put the
crash contract first.  Under `--speak N --sign` the mouth publishes what it
just said as one biography event of type `s` — episode, candidate digest,
bytes, seed, hand, law, opening, lived bytes — after the stream is out and
flushed, in the citation's two-file order: the line, the biography close,
the atomic state.  A death between the two files is refused on resume by
chain and count and never repaired; a stream that cannot be flushed
publishes nothing, because an unsigned speech is honest and a phantom
signature is not.  Speech stays behaviourally read-only: the signed and
the unsigned mouth speak byte-identical streams, and nothing reads `s`
back into behaviour.  `--sign` needs `--speak` with at least one byte and
a lived state; with `--reset`, `--cite`, `--court` or `--ear` it refuses
by name.  State stays v20: the biography count and chain it already
carries are the whole publication.

The first probes found the contract's real edge, and it was not the one
named in advance.  The first build opened the biography for append before
speaking, so that an unwritable biography would refuse before a byte was
out.  With stdout closed at invocation, that order let the biography
inherit descriptor 1: the sixty spoken bytes were written into the
biography ahead of the `s` line, `fflush(stdout)` succeeded, the mouth
reported `signed` with rc 0, and the next resume refused the life by
chain.  Fail-closed, but a phantom signature and a locked life.  The same
class holds for a closed stderr and for an operator's redirect of stdout
or stderr into the biography or the state — and it held for the unsigned
mouth of body 18 as well, whose write-nothing law was only as honest as
the operator's stream.  The mouth therefore now checks, before its first
word and for the signed and the unsigned mouth alike, that descriptors 1
and 2 are open and that neither is the biography or the state; a voice
pointed at the memory refuses silently, because it has nowhere honest to
say why and every word it said would break the chain — the state loader
already refuses trailing bytes.  The biography is opened only after the
stream is flushed; a biography that cannot be opened leaves the speech
spoken and unsigned.

The signature lived on first breath.  On the two-shore life the signed
mouth speaks the same sixty bytes as the unsigned one, appends
`s 2 796e0c5ce4b27b99 60 7 tri supported-backoff 6162 1200`, and saves
the state atomically; the fixture over the captured stream names the same
digest, and the red Laplace mouth signs `5f1219034fdfcd3e` under its own
law name.  The crash contract was then executed rather than described:
the biography one line ahead of the state refuses resume by chain and
count; the state ahead of the biography refuses; the biography cut in the
middle of the `s` line refuses; a closed stdout refuses before a word by
name and leaves both files byte-identical; a closed stderr, a stdout
appended to the biography, a stderr appended to the state or to the
biography all refuse with both files untouched; a stale `state.tmp`
sibling left by an interrupted publication neither helps nor hinders.
Two red hands were built before the gates were trusted: the pre-guard
build that signed into a closed stdout, and a build without the state
save, whose signature leaves the state unchanged and whose resume is
refused as biography-ahead.

Ten gates seal the body: the signed speech is the unsigned speech byte for
byte; one `s` event names the digest the manifest and the fixture name and
the state moves; a signed life resumes and plays; signed and unsigned
twins play identically and their biographies differ only by the `s` line;
a repeated signature is a second event; the red Laplace mouth signs under
its law; the three deaths between the files refuse; the stale sibling is
inert; the closed and self-directed streams refuse before a word; and the
mask refuses `--sign` without a mouth, with zero bytes, with a reset or
with another instrument.  The citation's non-file sweep now counts
twenty-eight controls, and the sanitizer mouth signs and resumes in
silence.  Three hundred and four gates pass.

Two limits are named rather than hidden.  The publication has no fsync
anywhere in the organism, so the contract covers a dying process and not a
dying machine — the same limit the citation has carried since body 28.
And the `s` line records the manifest, not the flag environment: the
biography chain supplies causal placement, and an outside hand can still
prove only count and digest.  A citation still names no life; reading `s`
against `w` — the life recognising its own speech in a court's word — is
the next body, and it now has both halves to bind.

## The signature audited: the mouth cannot write its world

Don's `7982e38` was reproduced untouched before its implementation was
read: 304 gates, strict build silent, sanitizers green.  The advertised
one-file leads both refused, but they were file constructions rather than
process deaths.  The contract was therefore met at the write boundary.
`RLIMIT_FSIZE` cut the biography in the middle of its `s` row, stopped a
state sibling after the complete biography publication, and killed the
process by `SIGXFSZ` during that state write; an interposed rename first
returned `EXDEV`, then stopped the process at the exact pre-rename call so a
real `SIGKILL` could end it.  Every old state remained intact, every
biography lead refused resume by grammar or count/chain, and complete and
partial stale siblings conferred no recovery power.  The law covers the
dying process it claims to cover.

The descriptor sweep found a different breach.  When stdout was opened for
append on a named shore that the resumed life was allowed to meet, the mouth
loaded and verified the old shore, wrote sixty words into it, signed the
speech, and on the next invocation accepted the mutated file as a new island
arrival.  The world had been changed by the instrument whose constitution
says no life mutates a world.  A second arm sent stdout and stderr through one
plain file: banner, diagnostics, speech, manifest and the `signed` receipt
became one mixed object, while the biography claimed a sixty-byte candidate
that no longer existed as the captured file.  A read-only directory failed
only at the eventual stdout flush.  These were not signature-only defects;
they belonged to the mouth's stream law.

The guard now runs before the banner.  Descriptors 1 and 2 must be writable;
state, biography and every named shore are protected on both channels; and
one regular or framed sink cannot carry both the byte stream and its voice.
A terminal remains an intentional interactive surface.  A protected stderr
still refuses silently because the reason itself would write the forbidden
object; when stdout alone is protected, stderr can name the refusal.  A full
speech sink may receive a real prefix, but a failed flush publishes no
signature and moves neither memory file.

Biography had the same kind of unspoken boundary.  An ordinary newborn life
could write its biography to `/dev/null` or a FIFO, publish a state whose
line count and chain described the vanished bytes, and become impossible to
resume.  Biography is now explicitly a regular-file object.  Non-regular
paths are refused before a FIFO can block; newly created regular files and
existing regular files keep the old append/reset behavior.

The `s` event also receives its own reader now, rather than borrowing only
the outer chain.  `bio_verify` demands canonical unsigned decimal tokens,
sixteen lower-hex digest positions, `uni|bi|tri`,
`supported-backoff|laplace-red`, four lower-hex opening positions, positive
speech and lived-byte counts, and episode/lived-byte values no later than the
loaded life.  The public red constructor
`scripts/biography_fixture.c` recomputes the state's line count and FNV
chain after mutating the row.  Fourteen correctly re-sealed records —
leading, signed, zero and future integers; short and upper-case hex; a move
hand; a foreign law; malformed opening; and an extra field — all refuse.
FNV remains an integrity witness, never secret authorship.

The short powerless twin was extended instead of merely repeated.  Three
signed speeches of 73, 91 and 127 bytes, including the Laplace arm, were
interleaved with jury, units, Atlas choice, reversed convoy order and two
final resumes.  The unsigned twin heard the identical speeches.  All four
played transcripts matched after the public biography counter was removed;
the biographies differed only by exactly three `s` rows, and the states
differed only in bytes 41–56, the biography count and chain.  No signature
entered behavior indirectly through a later life.

Eight gates were added: strict red-hand construction; regular biography;
shore/channel isolation; newborn and full-output refusal; partial/full file
ceilings; rename refusal and real `SIGKILL`; fourteen re-sealed grammar
cases; and the long three-signature twin.  Three hundred and twelve gates
pass.  State stays v20 and the sanitizer court remains green.

Body 31 should not amend the `w` line and should not immediately mint a
redundant `r` fact.  A warrant is the court's immutable statement; the
relation between a prior `s` and a later `w` is derivable from one verified
causal chain.  The first recognition should therefore be a read-only reader:
within one biography, match candidate digest *and byte count*, require the
`s` row to precede the `w` row, and report how many prior own speech events
share that public candidate identity.  Another life's identical digest is
absent from this chain and grants nothing; a grafted `s` breaks count/chain
unless the state is also re-sealed, which remains the named non-cryptographic
integrity limit.  Repeated identical speeches are membership, not proof of
which physical utterance the court heard.  No recognition code is opened
without Oleg's word.

## The signature audited twice: a tab in a format is not a byte

Sol's `1c1bd3f` was reproduced untouched before its diff was read: 312
gates, strict build silent, sanitizers green.  Her three findings on body
30 were real and her repairs stand: the mouth could append its speech to
a named shore and the next invocation would meet the mutated world as an
arrival; a stream and a voice merged into one plain file made a mixed
object under a clean sixty-byte signature; and a biography could
disappear into `/dev/null` or a FIFO and leave a life that no resume
could open.  The publication order survived real deaths — partial
writes, a refused rename, `SIGXFSZ`, a `SIGKILL` at the pre-rename stop —
and the long twin carried three signatures through jury, atlas, a
reversed convoy and two resumes with only `s` rows and the state's count
and chain apart.

Her request was to meet `bio_signature_ok` with an independent parser
rather than to reread it, and the second hand found what a reading would
not.  Fifty-four `s` rows were built from one canonical row — every
field with a leading zero, a sign, a 32-character token, an empty token,
at the unsigned maximum and past it, the episode and lived-byte bounds at
and past the state's, the digest at 15, 17 and upper-case digits, the
hand as `mv`, `null`, `Tri`, `trix` and empty, the law shortened,
capitalised, lengthened and underscored, the opening at three, five and
upper-case digits and as `cold`, an extra and a missing field, a
trailing tab, a trailing and a leading space, a space for a tab, the
prefix as `S` and `ss`, two fields swapped, CRLF, a bare `s` and `s`
with a tab — each re-sealed into the state by her public fixture and
shown to two readers: the organism on resume, and a full-line regular
grammar with an explicit unsigned range and the state's bounds.  Five
rows divided them, and one of the five was a defect of the reader she
had called strict.  A tab in a `scanf` format is a whitespace directive:
it matches any run of blanks, including none.  So `s\t2\t<digest> 60\t…`
resumed as a canonical signature, and so did a row with two tabs after
its type and a row with two spaces after its hand.  The `i` record of the
island registry, older than the signature, carried the same root — a row
with spaces for tabs, a row with `+0` for its episode, and a row with two
tabs all resumed.  The court's readers and the manifest reader never had
this hole, because each of them reprints the canonical line and compares
it byte for byte; `bio_verify` did not.  It does now, for `s` and for
`i`: the row must equal its own canonical reprint, tab for tab.

The other four rows named an older debt.  A bare `s`, ` s\t…`, `S\t…`
and `ss\t…` were admitted because the biography reader knew only two of
its thirteen record types — the step row that opens with an episode
number, and the letters `a b d i m q r s t u v w` — and chained the rest
unread.  The type set is closed now: a record whose first field is
neither a canonical episode number nor one of those letters, followed by
a tab, refuses resume by its own name.  This is a partial closure and is
called so: the type is known, the shape is proved for `i` and `s` only,
and the eleven other grammars are still chained rather than parsed.
That is the door the next turn should walk through.

The battery repeated on the repaired reader agrees on all fifty-four
rows, accepting exactly the eleven built to be legal.  Three gates seal
the repair against her fixture — tab runs, spaces and signs refuse in
`s` and `i`; the five records of no known type refuse by name; the
canonical rows still resume — with her own `1c1bd3f` as the red hand
that accepted every one of them.  The descriptor law was met by the
states she had not listed and held on all of them: a read-only stdout,
both channels on `/dev/null`, a stdout on a symlink and on a hard link of
the biography, a biography reached through a symlink, a directory,
`/dev/null`, a FIFO without a reader, and a state that is its own
biography.  Three hundred and fifteen gates.

## Body 31: the recognition — preregistered before code

Oleg's word opened the thirty-first body with a boundary drawn tighter than
either audit had asked: `biography_check.c` as an independent read-only
organ — prior `s`, later `w`, equal digest and byte count, honest
multiplicity, a report and nothing else — and no grammar taken out of
`netta.c`, because Netta must stay self-sufficient and survive without any
external module; two independent readers prove one language first, and the
extraction of grammars is a separate refactor after the other hand's audit.
So the organism did not move: the commit of this body changes no line of
`netta.c`.

The reader shares no organism code.  It takes one biography, requires a
regular file, and reads it record by record with its own hands: every
record newline-sealed and free of NUL; every record opening with a known
type — the episode number of a step or one of the letters `a b d i m q r s
t u v w`; and, for the three records it parses, a grammar split on exact
tab bytes, so that the whitespace directive that fooled the organism's
first `s` reader has no way in — `s` with its nine canonical fields, `w`
with its nine (episode, candidate digest, bytes in the court's `[2,
16384]`, `cold` or four-hex context, shores in `[1, 32]`, law digest,
docket digest, and a verdict count equal to the shores), and `i` by shape.
The ten other types are admitted by type only, as the organism admits
them today; the reader is stricter than the organism on `w`, which the
organism chains but does not yet parse — a difference named for the later
turn, not hidden.  It prints the record count and the FNV chain over the
raw records, the same public witness the organism prints after a played
episode, so the two can be laid side by side.

Recognition is causal and exact.  A `w` is recognised when a signature
that stands EARLIER in the biography carries the same candidate digest and
the same byte count; the report names the line, the episode, the
candidate, the bytes, the multiplicity and every matching `s` line; a `w`
with no such prior signature is printed as an external fact.  A signature
after the citation recognises nothing — the life did not yet own the
speech when the court's word arrived — and a signature of the same digest
with a different byte count is not the same stream.  Multiplicity is
reported, not resolved: two signatures and two citations of one stream
give two recognised lines of multiplicity two.  Nothing is decided by it;
it is a reading.

Measured on the two-shore life: `--speak 60 --speak-seed 7 --sign` leaves
`s` at record 1539 with candidate `796e0c5ce4b27b99`; the pattern court
over the captured sixty bytes opens a sitting on that same candidate; the
citation of that docket leaves `w` at record 1540; the reader prints
`recognised: line 1540 episode 2 candidate 796e0c5ce4b27b99 bytes 60
multiplicity 1 s-lines 1539` and `recognition: s=1 w=1 recognised=1
external=0`, and after a played episode its `1554 records, chain
5f3805c3f5633fed` equals the organism's own `biography:` line.  Two red
hands were built before the gates were trusted: a reader that pairs on
digest alone recognises a signature whose byte count was changed to 61,
which the law calls a different stream; and the causal order was checked
against a life that cited first and signed after, which must not
recognise.

Ten gates seal the body: the strict silent build; the recognised
citation of the life's own signed speech with the biography byte-identical
after the reading; the reader's count and chain equal to the organism's;
a foreign docket left external; the citation before the signature left
external; multiplicity two on two signatures and two citations; the byte
count required beside the digest; malformed `s`, `w` and `i` rows,
unknown types, a NUL and an unsealed row refused by name; nine re-sealed
`s` rows on which the organism on resume and the reader draw one verdict;
and the sanitizer reader silent.  Three hundred and twenty-five gates.

What this body does not do is also its content.  It reads `s` against `w`
outside the life: the organism does not know that a `w` is about its own
speech, and no `w` field, no new event and no behaviour changed.  The
recognition exists as a fact an outside hand can derive from the public
biography alone — which is exactly the fact a later organism growing
between lives will need first — while the life itself keeps its citations
as external facts.  Whether the life should ever read its own recognition,
and how, is the question the next turn inherits together with the eleven
unparsed grammars.

## The recognition audited: a file is not a life identity

Don's `1c56f9a` was reproduced untouched before its implementation was
read: 325 gates, strict build silent, sanitizers green.  The organism was
indeed absent from the body-31 commit.  The outside reader changed no
biography byte and no path from its report entered state, resume or
behaviour.  Prior `s`, later `w`, equal digest and bytes, causal order and
honest multiplicity all stood.

Two executable I/O defects remained in the reader's own contract.  It used
`fopen` before `fstat`, so a FIFO with no peer blocked before the promised
regular-file refusal.  It also ignored the final stdout flush, so a complete
lost report could return rc 0 although rc 2 was declared for I/O failure.
The hand now opens nonblocking, verifies the opened descriptor is regular,
and enters stdio only afterward; a failed report flush returns 2.  Records
are read into a fixed 1024-byte frame rather than an unbounded `getline`:
overlong and unsealed rows, NUL, CR and more than sixteen fields refuse by
name.  CR is one global separator law, not a privilege of whichever record
grammars have already been implemented.  The empty sequence is valid
syntax, a lone newline is an untyped record, and a single canonical `s` is
a valid one-event sequence.  Citation bounds are inclusive at 2 and 16384
bytes and at 1 and 32 shores.

The phrase "two readers, one language" needed a narrower noun.  The outside
reader has no state, so it reads context-free record syntax.  Resume reads
that syntax and then judges historical validity: an `s` episode beyond the
loaded episode or lived bytes beyond the loaded life must be refused by the
organism and accepted as canonical syntax by the outside hand.  Twenty-seven
`s` rows agree after those contextual bounds are held fixed; two future-bound
rows disagree for the declared reason.  The same distinction already exists
for `i`, whose registry identity only state can judge.

The older `w` debt is now measured rather than gestured at.  Twenty-four
independently re-sealed families -- signs and leading zeroes, hex widths and
case, byte and shore bounds, context, law and docket shape, verdict count,
field count, exact tabs, CRLF and record length -- are accepted by current
resume because it chains `w` by type, while the outside reader refuses them.
This is a finite corpus of debt families, not a claim that malformed strings
are finite.  The reader parses three of thirteen record types (`i`, `s`,
`w`); ten remain type-only outside.  The organism parses two, so eleven
remain to close there.

Finally, two sibling lives were forked from one honest past.  Life A signed
a stream; life B cited the court's word about A without signing it.  Each
biography alone reported the right result.  Concatenating A before B by hand
manufactured a prior-`s`/later-`w` recognition.  No stat, FNV chain or record
grammar can recover the missing life boundary from that file alone.  The
report therefore says its premise literally: `one supplied file is treated
as one life; state identity unverified`.  Recognition is a true relation
inside the supplied sequence, not independently proved provenance of that
sequence.

The later grammar refactor should preserve this independence.  `netta.c`
must grow or retain all thirteen validators inside its self-sufficient heart;
the outside reader should implement the language again without a shared
linked object or required header.  A shared parser would shorten code by
destroying the differential witness.  Documentation and adversarial corpora
may be shared; executable hands should not be.  That refactor is not opened
by this audit.  Three hundred and thirty-two gates pass; `netta.c` and state
v20 are untouched.

## The recognition audited twice: the frame, the report, the scope

Sol's `2ee6c0a` was reproduced untouched before its diff was read: 332
gates, strict build silent, sanitizers green.  Her two findings on the
outside reader were real and her repairs stand: `fopen` could block on a
FIFO before the regular-file verdict, and a lost report could return rc 0.
The reader now opens non-blocking, frames every record at 1024 bytes,
refuses NUL, carriage return, an unsealed tail and more than sixteen
fields uniformly, prints its scope before any verdict — one supplied file
is treated as one life, state identity unverified — and returns rc 2 when
the report cannot be written.  Her measurements stand as well: twenty-seven
`s` rows on which the organism and the reader agree as context-free
grammar; two rows whose episode and lived bytes lie beyond the state,
which the stateless reader must accept as syntax and the organism must
refuse on resume — the line between record syntax and state-relative
truth, drawn rather than blurred; twenty-four re-sealed malformed `w`
families the organism resumes and the reader refuses, the measured size
of the debt the grammar refactor will close; and the sibling splice, where
a file made of life A's signature and life B's citation recognises inside
the file, so that file scope is an assumption to be printed, not a life
identity to be claimed.

The frame was met at its edges: a 1024-byte record is read, a 1025-byte
record refuses by name; the organism's own biography buffers are all at
most 256 bytes, so no legal record can reach the frame.  An empty file is
an empty language, accepted with zero records and the seed chain; a lone
newline opens with no known type; a sixteenth field is admitted and a
seventeenth refuses; a closed stdout returns rc 2 with the reason on
stderr; a biography reached through a symlink reads.  One asymmetry is
recorded, not repaired: a carriage return inside a step row is chained by
the organism and resumed, and refused by the reader — the reader is
stricter than the organism on the ten types the organism does not parse,
which is the same debt as the twenty-four `w` families, seen from the
other side.

On her question — if one file should prove one life rather than assert
it, name the missing witness — the answer is that the witness exists and
is not the reader's to hold.  A life's biography is identified by the
state that acknowledges it: the record count and the chain, checked by
the organism on resume.  A spliced file resumes under no state unless an
operator re-seals one with the public fixture, and that re-sealing is
public authorship, reproducible by anyone, refused by nothing — because
FNV is a witness, not a signature.  So the reader's law is correctly
stated as scope: one file, one life, identity unverified; the authority
over identity stays with the organism's resume, and the reader must not
be given state by stealth to pretend otherwise.  Three hundred and
thirty-two gates.

## Grammar closure — preregistered before code, on Oleg's contract

Oleg opened the refactor with a contract stricter than the position
either hand had taken.  `netta.c` stays a self-sufficient single-file
heart: no required headers, no shared objects, no external parser code.
The specification and the canonical corpus come first, derived from all
emit sites rather than from samples; a document and test data are the
only things two readers may share — executable code stays independent,
because the second hand is the differential witness.  Context-free
grammar and state-relative truth are described separately, and
`disagree=0` refers to the verdict class on the context-free language
only.  All thirteen record types close with exact tabs, canonical
fields and byte-for-byte reprint; every type receives its own
malformed-and-resealed red battery.  State v20, biography emission,
behaviour, RNG and lived results do not change; the long twins before
and after must be bit-identical.  The work lands in separate commits —
specification and corpus, then the inner reader, then the outer — so a
divergence, if one appears, is localised by construction.  The end of
the body: thirteen of thirteen in both hands, grammar disagreement
zero, the twenty-four-family `w` debt at zero, the carriage-return
asymmetry at zero, and the full suite green on a clean archive.  After
that, the other hand audits, and the freeze is declared by Oleg's word,
not by an automatism; the heart is not emptied, and the circle is not
yet called closed.

This first commit is the specification and the corpus, and the
specification promptly paid for being derived from the code rather
than from memory.  The `t` record has three arms, not two: `eligible`
and `earned` were in every sample this log had seen, and `chart` — the
under-lived shore chosen by least lived bytes, with the runner-up's
lived count or `18446744073709551615` when there is none — appeared
only when a real varied life was run and its atlas rows were counted.
A specification written from the two known arms would have been wrong
on eight of the first hundred and twenty-four atlas decisions of that
life.  The corpus therefore carries one row of every type and every
arm — nineteen shapes across the thirteen types, seventeen harvested
from lives lived for this commit, and the two arms those lives did not
walk (`r` with seven fields, `q` naming the null hand) derived from
their emit formats and said so.  Ninety-one malformed rows across
thirteen files carry the separator, sign, leading-zero, hex, literal,
enum, arity and fixed-point faults every reader must refuse.

Two findings from the harvesting are recorded for the next hands, not
silently repaired.  A life lived under a neural-law flag can never
cite: without the flag the state refuses to load (the invocation law
changed), and with it the citation's mask refuses the flag — the
citation of body 28 is unreachable for a jury-law life, which the
suite never noticed because its cited lives are plain.  And CodeQL's
first alert on this repository names the stat-then-open sequence of
`bio_verify` as a race; the post-open identity check added by the
fifteenth turn (fstat against the named stat, device and inode) closes
it fail-closed, but a swap to a FIFO between the two calls would hang
the plain fopen — the verify-side open wants the same non-blocking
discipline the write side already has.  Both belong to an organism
turn under audit, not to this refactor, whose contract forbids
touching behaviour.

## Grammar closure, the inner reader: thirteen types in the organism's hands

The second commit grows `bio_verify` to the whole language.  Eleven
grammars join the `i` and `s` checks of the fifteenth turn: the step
record and the letters `a b d m q r t u v w`, each split on exact tab
bytes, each field held to its canonical shape from `BIOGRAPHY.md`, each
record closed by its exact field count — so a run of tabs, a stray
space, a sign, a leading zero, a wrong literal, a wrong enum, a
non-canonical fixed-point rendering or a carriage return refuses resume,
for every type, with the organism's own hands and no external code.  The
seventeen canonical corpus rows of foreign types graft onto a lived
biography and resume; all ninety-one malformed corpus rows refuse; the
twenty-four `w` families that measured the debt now refuse in both
hands, and the gate that counted them as debt now counts them as repaid;
the carriage-return asymmetry is closed from the organism's side.  What
did not change is proved, not promised: a two-stage life frozen from the
pre-closure binary — seed 11, atlas and jury, ten episodes over two
shores — lands on the same record counts and the same chains,
`18c67d0e0ca68401` and `1fc3f4127170a2f9`, byte for byte, and the whole
suite stands at three hundred and thirty-six.  The outside reader still
parses three types; its closure is the third commit, and the gate that
will hold both hands to all ninety-one rows waits there.

## Grammar closure, the outer reader: the language whole in both hands

The third commit grows `scripts/biography_check.c` to the same thirteen
types with its own hands: ten grammars join its `s`, `w` and `i` — the
step record and the letters `a b d m q r t u v`, each a fresh
implementation of `BIOGRAPHY.md` on the reader's side of the fence,
sharing with the organism nothing but the document and the corpus.  All
ninety-one malformed corpus rows now refuse in the outside reader as
they refuse in the organism; all nineteen canonical shapes are syntax to
the stateless reader, including the two whose state-relative values a
resume would refuse — the line between record syntax and state-bound
truth holds exactly where the sixteenth turn drew it.  The closing gate
grafts every corpus row into a lived biography and asks both hands for
their verdict class: one class on all one hundred and eight rows,
disagreement zero.

The body ends where Oleg's contract said it would end.  Thirteen of
thirteen in both hands; grammar disagreement zero; the twenty-four `w`
families that measured the debt repaid; the carriage-return asymmetry
closed; the frozen life bit-identical through the closed heart; three
hundred and thirty-nine gates on the tree and on a clean archive.  The
heart was not emptied: every validator lives inside the single file, no
header, no shared object, no external parser code — self-sufficiency
kept literally.  The circle is not called closed: that word is the
other hand's audit and then Oleg's, not an automatism at the end of a
diff.

## Grammar closure audited: the values behind the shapes

Don's `735a1d8` was reproduced untouched before its implementation was
read: 339 gates, strict builds silent, sanitizers green.  The fifteen emit
sites and all nineteen format arms were then walked again from code rather
than from the corpus.  No type or arm was missing, but the adversarial pass
found two common-mode holes in both readers.  A two-field `t\t1` record made
each hand inspect field three before proving it existed; the outside hand's
uninitialised pointer happened to refuse on the observed build, but that was
not a C contract.  And `18446744073709551616.000000` passed both `fix6`
helpers even though the specification names a u64-shaped integer part.  The
organism now proves `t` arity before indexing and parses the integer part
through its own canonical-u64 hand; the outside reader independently holds a
twenty-digit integer part against the decimal `UINT64_MAX` boundary.

The same walk separated fixed emitter domains from history.  Registry ids
are `0..1023`; unit ids `0..4095`; the `m` field named ISL is a current-convoy
slot `0..31`, while `r` and `q` receive a persistent registry id and must not
be narrowed to 31; move ids are `0..4351`.  A `v` atom has length one, a unit
length 2..16, advance is 1..length, target is a move, and the navigation arm's
policy is a real route anchor `0..4351`: its signed local variable never
emits the pre-search `-1` sentinel.  Atlas `chart` chooses a shore lived fewer
than 1000 bytes and always has a runner in a competitive choice; `earned`
begins at 1000 lived bytes and both selected prices lie under the atlas's
8-bit ceiling.  Unit birth and resurrection support is a positive multiple
of 64, death rent is at least 16384 idle bytes, and the court's seated and
challenger roles are now the roles its branches can actually write.  Whether
an in-range identity exists, is present or living, owns a length, or appears
in historical order remains the state's law; the stateless reader does not
borrow those facts.

One older special path also failed the hostile-number test.  `i` used direct
`sscanf` integer conversions before its canonical and state checks, leaving
an overflowing id to libc before the reader could refuse it.  It now captures
bounded string tokens, converts each through canonical u64, reprints the
exact record, and only then compares registry order and witnesses.  Its
arrival counter is u64 rather than a second implicit `int` limit.  The `s`
special path already followed that discipline and was left alone.

Forty-six new malformed rows were written without consulting either parser:
short arity, u64 and fixed-point overflow, every fixed upper and lower bound,
move/length/advance relations, impossible court roles, and overflowing `i`
tokens.  The shared corpus is now 137 malformed rows.  All 137 refuse in the
organism after public re-sealing, all 137 refuse in the outside reader, and
the two independent hands give one verdict class on all 154 context-free
canonical and malformed grafts: disagreement zero.  The former sanitizer
gate now executes the short-`t` and overflowing-fix6 regressions explicitly.

The recorded open race was taken too.  `bio_verify` retains the named
`stat`, but opens with `O_RDONLY|O_NONBLOCK`, judges the opened descriptor as
a regular file, compares device and inode, and only then creates its stream.
The fail-closed identity law is unchanged and a FIFO swap can no longer block
inside `fopen`.  The other recorded finding — a jury-law life cannot enter
the citation invocation — is deliberately not repaired here: it changes an
organism law and still needs Oleg's word.

Behavioural equivalence was checked twice.  Don's frozen seed-11 chains stay
pinned by the suite.  Independently, binaries built from pre-closure
`2ee6c0a` and this audited tree lived seed 29 under Atlas and jury on three
periodic shores, resumed after a convoy permutation, then resumed with one
shore absent.  State and biography matched byte for byte after every stage;
the final biography has 8544 records and chain `7c8ae812ad41697c` (state
SHA-256 `1b68c9c39fab34a6f3daa1767309d32705ac863af2f7e0eb7443467f66d1668d`,
biography SHA-256
`da5b06521ef26cd2bed21a7b3f1757f7de20f4cf423049dfd9e6f5235f74612a`).

The audit verdict is therefore narrow and affirmative: the biography
language of the current fifteen emit sites is closed enough to freeze, and
new organs should live outside `netta.c`; this does not silently freeze a
known behavioural defect or grant the word of closure.  Oleg owns both the
freeze declaration and any later surgical exception.  The self-contained C
heart, state v20, emit bytes, RNG and lived behaviour remain unchanged.

## The value closure audited: every edge answered from both sides

Sol's `269d67f` was reproduced untouched before its diff was read: 339
gates, strict builds silent, sanitizers green.  Her four findings were
then met with a second hand rather than reread, and all four stand.  The
fix6 overflow was proved real against the pre-audit hands: the row whose
integer part is one past the unsigned maximum was accepted by both of
the sixteenth turn's readers and refuses in both of hers.  The short `t`
refused on the old hands too — but through a read of an uninitialised
field pointer, undefined behaviour that happened to land on the right
verdict; her arity guard and the sanitizer gate that now executes the
case are the difference between a verdict and luck.  One suspicion of
this audit died properly: the seven-field `r` row with a `uni`
challenger looked at first like a domain wider than its emitting branch,
until the branch itself answered — the challenger defaults to zero and
the loop may leave it there, so `uni` is the fall-through challenger the
emitter really produces, and her bound is exactly right.

The new value boundaries were then walked edge by edge, from both
sides, in both hands: birth and resurrection support at 64, 63, 128 and
zero; death rent at 16384 and 16383; the chart winner at 999 and 1000;
the earned score at 8.000000 and 8.000001 and its floor at 1000 and
999; the seated `uni` and the seven-field `null` challenger; the
undistinct court; atomic and unit move lengths across the 255/256 line;
advance against length; policy and target at 4351 and 4352; the
registry at 1023 and 1024; the unit at 4095 and 4096.  Twenty-eight
edges and the short `t`: every legal edge accepts in both hands, every
step past it refuses in both, no split anywhere.  Behavioural
equivalence was retried with a recipe neither hand had used — seed 47,
four shores including a ninety-byte sliver, atlas and jury, a permuted
convoy on the first resume, a shore withdrawn and a stranger added on
the second — and the body compiled from `2ee6c0a` and the audited body
matched byte for byte through all three stages: 9972 records, chain
`a855ca70189ea535`, one state, one biography.

Both hands have now audited each other's closure and both say the same
word to Oleg: the biography language of the fifteen emit sites is
closed enough to freeze, new organs belong outside `netta.c`, and the
heart stays single-file and behaviourally unchanged.  The two recorded
organism findings — the jury-law life that cannot cite, and nothing
else — wait for his word separately.  The declaration itself is his.

## The descriptor is the biography being judged

Don's `c2b73ba` was reproduced untouched before its audit note was read: 339
gates, strict builds silent, sanitizers green.  His second walk of every new
value edge stands, as does his fresh three-stage equivalence life: 9972
records and chain `a855ca70189ea535` in both bodies.  Two independent hands
therefore give Oleg the same freeze verdict on the closed biography language.

GitHub CodeQL then named the residue of the earlier FIFO repair.  Non-blocking
open had removed the possible hang, and comparing the opened device and inode
to a preceding named `stat` made a replacement fail closed, but the check and
the use were still two pathname operations.  The read-only verifier needs no
pathname identity after resolution: an open descriptor already pins the
object whose bytes it reads.  `bio_verify` now opens once with
`O_RDONLY|O_NONBLOCK`, judges that descriptor with `fstat`, and gives the same
descriptor to `fdopen`.  There is no named pre-check and therefore no interval
in which a changed pathname can redirect the object being verified.  A
regular biography reached through a symlink remains legal; a FIFO without a
writer is refused immediately as non-regular.  The biography grammar, its
emitters, state v20, RNG and lived behaviour are untouched.  The remote alert
is not called closed until GitHub analyses the delivered commit.
