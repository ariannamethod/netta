# Question and structural-twin audit

Date: 2026-08-16.  This protocol was written on `sol/zero-turn-9` after
reproducing the untouched body-22 suite and before implementing body 23 or
running any structural-twin hearing.  Results belong below the sealed line;
failed predictions stay in the record.

## Body-22 repair protocol

The mouth and contextual ear currently implement the same two-byte question
law in separate loops.  The untouched binary will be challenged with a
two-byte regular file, a longer file with the same suffix, a FIFO, `/dev/stdin`,
an eight-megabyte file, and an underspecified one-byte FIFO.  All complete
questions with the same final two bytes are predicted to be equivalent; the
short stream must be refused after EOF.  A regular file is a stronger case:
because the documentation calls it immutable, an in-place change of its
relevant tail must not silently produce an unwitnessed mixture.

The repair is accepted only if mouth and ear call one question reader.  A
regular file must take a stable snapshot of its final two bytes without
scanning an irrelevant prefix; a changed length or changed tail during that
snapshot is refused.  A non-regular input remains a stream: its question is
sealed only by EOF and its final two observed bytes are the context.  Thus a
pipe is supported, not accidentally seek-dependent, while an unterminated
pipe honestly has not finished asking.  The old error names, state silence,
and the rule that every prefix before the final two bytes is irrelevant must
survive.

`--prompt-file` and `--ear-context` remain distinct public verbs.  One warms a
living mouth and the other a stateless measurement; merging their invocation
names would erase that jurisdictional difference.  They share a byte-level
question law internally instead.

## Body-23 preregistration: every shore's structural twin

Body 23 may add `--ear-twin` only as an explicit, powerless ear instrument.
For each named island it will copy the immutable bytes and apply the already
sealed Gutenberg permutation: descending Fisher-Yates, SplitMix64 state
`0x4e45545441475241`, and `draw mod span` at each step.  This deliberately
reuses the historical algorithm rather than inventing a favourable new null.
The twin must conserve length and all 256 byte counts.  Its digest and the
number of positions changed from the true shore must be printed; a degenerate
same-byte shore must therefore say that zero positions changed rather than
pretending that structure was destroyed.

The candidate will be priced under the same cold or explicit two-byte context
first by the true shore and then by that shore's twin.  Exact-match length and
matched-16 coverage remain measurements against the true shore only.  The
external `scripts/garena_prep.c shuffle` hand must independently produce the
same twin and the same price on an arbitrary binary vector.  The flag must
open no state, write no file, enter no election, and be refused outside an ear
invocation.

Before measurement, the predictions are:

1. A literal slice of the existing period-3 shore will retain 100 percent
   matched-16 coverage and price lower on the true shore than on its shuffled
   twin.
2. A constant-byte shore will report an identical twin and equal prices.  It
   is a falsification arm for the phrase “shuffle destroys structure.”
3. A deterministic uniform-byte shore can make a supported trigram mouth
   replay a long literal passage even when its local price beats lower-order
   mouths.  The generator is SplitMix64, seeded
   `0x534f4c554e49464f`, one low byte per draw, 4096 bytes.  A life sees the
   302 transitions from fixed offset 16; prompt bytes are offsets 14 and 15;
   locked uni, bi, and tri mouths each emit 300 bytes at speech seed 7.
   The prediction is that tri has the lowest true-shore price and 100 percent
   matched-16 coverage.  A collision that breaks replay is a failed
   prediction, not grounds to choose another seed.

No price difference, match percentage, or twin result receives a threshold,
court, candidacy, or authority in this body.  The output merely completes the
three-coordinate ruler preregistered by body 22: true-shore price,
true-shore matched-16 coverage, and shuffled-twin price.

---

## Results

Not run at preregistration time.

Preflight amendment, still before any generated speech or hearing: the first
draft named fixed offset zero, but Netta correctly refused it because every
episode requires the preceding sixteen-byte causal context.  The protocol is
therefore repaired above to the earliest legal offset, 16, with its actual
two-byte prompt at offsets 14--15.  The seed, world length, episode length,
speech seed, mouths, and predictions are unchanged.

### Body-22 audit

The untouched `14ae402` binary reproduced all 194 gates before its diff was
read.  Exact-two, longer equal-suffix, FIFO, `/dev/stdin`, and eight-megabyte
questions all produced byte-identical 128-byte speech at seed 19; the same
FIFO equivalence held at the contextual ear.  A one-byte FIFO was refused by
the promised name after its writer closed.  The predictions passed.

Code inspection found that this behaviour rested on two copied `fgetc` loops
and that an ordinary file had no tail-stability witness.  One shared reader
now takes and rechecks the final two-byte snapshot of a regular file while its
open identity and length remain stable.  Non-regular sources retain their
EOF-sealed streaming semantics.  Permanent gates repeat the regular, long,
FIFO, stdin, eight-megabyte, short-stream, and ear cases.  Mouth and ear keep
their different public verbs, as preregistered.

### Body-23 results

The independent `garena_prep.c` hand and `--ear-twin` produced identical
twin digests and prices on the arbitrary period-3 binary shore.  Its literal
200-byte slice priced at 0.457849 bits per byte on the true shore and 2.136573
on the shuffled twin, with longest match 200 and matched-16 coverage 100
percent.  The first prediction passed.  On a 4096-byte all-`a` shore the
instrument reported `twin-changed=0/4096`, and true and twin prices were both
0.087149.  The falsification arm passed: the null names its degeneracy.

The uniform generator's first sixteen bytes are fixed by the permanent vector
`14a06a602648d491483f08d4faee0afe`; the complete 4096-byte shore has SHA-256
`8bf24111cf50bd943c8088991aa8799e8be06b0143fc1ed08e2f8e687139db8e`.
The three 300-byte speech SHA-256 witnesses are:

| hand | speech SHA-256 |
| --- | --- |
| uni | `3747c4f2432a9833ae3a308b5646e30f4a777e143b418dbe9a7d749ae62c9fc1` |
| bi | `935fa31b8b089eaad9070fc118a8c496aa052937678370cd58b41337687103c4` |
| tri | `37fad94dff1181b8a5a1791590ba55106044be9aea1dd0e12881bc309de655b5` |

| hand | true bits/byte | longest match | matched-16 | twin bits/byte |
| --- | ---: | ---: | ---: | ---: |
| uni | 8.000412 | 2 | 0.0% | 8.000262 |
| bi | 7.405849 | 10 | 0.0% | 8.000300 |
| tri | 7.005793 | 300 | 100.0% | 7.997098 |

The third prediction passed exactly: all 300 tri draws used trigram support,
and all 300 emitted bytes form one literal shore substring.  Its lower price
is a replay mechanism, not evidence of abstraction.  The twin coordinate
separates that ordered replay from the local low price, while the match census
names the literal fact directly.  Nothing in the triple appoints a speaker.

The shipped suite now contains 206 gates.  Strict build and all inherited
gates pass; the new question sources, independent twin hand, literal passage,
degenerate shore, invocation and no-write laws, portable uniform generator,
replay red, and ASan/UBSan twin hearing all pass.  State remains v20.
