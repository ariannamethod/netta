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
