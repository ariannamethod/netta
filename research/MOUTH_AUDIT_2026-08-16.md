# Mouth audit and supported-backoff result

Date: 2026-08-16. Incoming branch: `don/zero-turn-6` at `454d3d7`.
The untouched 146-gate suite passed before the implementation was read.

## The arena's second hand

The C corpus-preparation hand reproduced the published Gutenberg files, but
an adversarial boundary separated it from the canonical Python hand.  With
one START and two following END marker lines, Python refused two ends while
C stopped counting after the first and accepted the body.  The C shuffle
also formed `n - 1` before testing its loop condition, so an empty input
underflowed `size_t` instead of matching the defined empty Python shuffle.

The repair counts every END after START and writes the first position only;
the descending shuffle now iterates by a positive span.  Executable vectors
cover CR at EOF, CRLF, a marker in the middle of a line, END without a final
newline, duplicate END, empty shuffle, and the independent sixteen-byte
SplitMix64/Fisher-Yates result
`0902000c08030d010504060f070e0b0a`.

## The eighteenth body under attack

The central claim survived.  Speech leaves state and biography byte-exact
under ordinary use, reset, Atlas, every architectural law flag, successful
and refused invocations.  A direct continuation and the same continuation
with an interleaved speech call finish with identical state and biography,
which independently locates speech outside the life's RNG stream.  Prompt
prefixes with the same last two bytes produce identical byte-hand speech;
an mv seat falls back to a named byte witness.

Three CLI boundaries needed repair.  `--speak 0` used zero itself as the
mode bit and therefore was not a mouth invocation.  A persisted state with
zero lived bytes counted as "resumed" and could emit uniform noise despite
having lived nothing.  `--speak-seed` and `--prompt-file` without `--speak`
were silently ignored by ordinary life.  Speech now has an explicit request
bit, a lived-byte floor, and speech-only controls are refused outside speech.
State remains v20: none of these laws adds persisted memory.

## Independent reconstruction of the first words

The pinned Dracula raw was fetched again: 890348 bytes, SHA-256
`96cd16eacdbfebae8fdda5591f66e0cc8ee76be18e0cd1aca02bc00615782d28`.
The repaired second hand produced the published 855114-byte body with
SHA-256 `a7786a4c81df95265b33d8c24dbbcaee80ab531d7d266a9782d52301718ce7c7`.
Lives were regrown with seed 160816 in 8000-byte episodes; prompt `The ` and
speech seed 7 were held fixed.  The old law is now named
`--speak-laplace`.

The four byte streams reproduce the tracked transcripts exactly.  The
published percentages also reproduce once their actual ruler is stated:
"displayable" here means ASCII `0x20..0x7e` plus LF, not C `isprint`.

| life / hand | bytes | displayable | fraction |
|---|---:|---:|---:|
| 64000, elected bi | 200 | 117 | 58.5% |
| 384000, elected tri | 300 | 119 | 39.7% |
| 384000, locked bi | 300 | 269 | 89.7% |
| 384000, locked uni | 300 | 293 | 97.7% |

## Body 19: supported generative backoff

The failure has a narrower cause than "tri cannot speak".  Laplace
smoothing is an honest prequential price: external truth may be a byte the
life has never seen, so every byte needs nonzero probability.  Generation is
a different act.  Sampling all 256 pseudo-counts treats ignorance as if it
were memory and can jump into an unseen context whose next row is another
uniform lottery.

The default mouth now samples observed continuations at the deepest context
available to its chosen byte hand: tri, then bi, then the lived unigram.
An empty row backs off causally; it never fabricates a deeper record.  The
old Laplace mouth remains an explicit red arm and neither law has memory or
authority.

On the fixed period-3 life, the old law emitted 113 bytes outside the lived
alphabet in a 120-byte sample.  Supported backoff emitted zero outside in
600 bytes and stayed on the exact `cacbab` cycle.  On the independently
regrown Dracula lives, the displayable census moved as follows:

| life / hand | old Laplace | supported backoff | support used |
|---|---:|---:|---|
| 64000, elected bi | 58.5% | 98.5% | bi 200/200 |
| 384000, elected tri | 39.7% | 100.0% | tri 300/300 |
| 384000, locked bi | 89.7% | 98.0% | bi 300/300 |
| 384000, locked uni | 97.7% | 97.7% | uni 300/300 |

This is a physiology verdict, not an eloquence verdict.  Printability still
cannot appoint a speaking hand; the unigram counterexample survives, and no
speech court or speech authority exists.  A future court still needs an
external ruler fixed before candidate speech is read.
