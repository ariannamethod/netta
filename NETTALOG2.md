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
