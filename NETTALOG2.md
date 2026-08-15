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
