# THE COURT OF THREE CHARACTER LAWS

Preregistered before any run. The desktop hand found the breach, both
building hands confirmed it against the sources: the prototype hashes
Unicode code points (`mycelium.py:31` folds `ord(c)`; `:50` cuts
three-CHARACTER windows), body 1 hashes UTF-8 bytes (`mycelium.cpp:95`
folds `unsigned char`; `:111` cuts `substr(i, 3)` — three BYTES). On
ASCII the two are identical by construction. On Hebrew every letter is
two bytes with a near-constant `0xd7` lead, and pointed text adds
`0xd6`-lead vowel marks between letters: a byte trigram spans about a
letter and a half and half its mass is constant prefix. That is a
different topology of resemblance. The 105 green gates prove the
internal honesty of the new Latin-visible system, not the inheritance
of the prototype's any-script property. The claim "the byte law LOST
quality" is a hypothesis, not a fact — this court exists to make it one
or kill it.

## The three laws on trial

- **L-cp — codepoint law** (the prototype's, faithfully): decode UTF-8
  strictly; each code point is one atom folded as its integer value
  (`h ^= value; h *= 16777619`, 32-bit wrap — exactly the prototype's
  arithmetic, which is why L-cp and L-byte are bit-identical on ASCII).
  An invalid byte is DROPPED, exactly as the prototype drops it
  (`errors="ignore"`, mycelium.py:220) — which is why pure inheritance
  was never on the table: the prototype's own law cannot eat Netta's
  byte soup without silently deleting evidence. (Amended before the
  instrument was written: the first draft gave L-cp the keep-as-byte
  fallback, which would have made L-cp and L-u8b mechanically one law
  and left the court with two arms instead of three.)
- **L-byte — byte law** (body 1 as it stands): every byte is an atom.
- **L-u8b — utf8-or-byte law** (the candidate): a strict RFC-3629
  decode — 2..4-byte sequences, valid continuations, no overlongs, no
  surrogates, max U+10FFFF — yields codepoint atoms; any invalid lead
  or continuation byte is consumed as a single byte atom. No locale, no
  normalisation, no case beyond the existing ASCII lower. On valid
  UTF-8 it IS L-cp; on arbitrary bytes it degrades gracefully instead
  of dropping evidence.

Everything else is held constant across all three arms: segmentation,
tokenisation, stops, the strip set, DIM, the trigram window of three
ATOMS framed by `^` and `$`, the DIM-96 expansion. The court varies
exactly one thing — what an atom is. The neighbouring English biases
(the STOPS list, the ASCII strip set, the byte-counted length floor)
are real and are NOT on trial here; one law per verdict.

## Corpora (pinned at preparation, digests recorded in the report)

- Hebrew: two independent work-blocks cut from the Ben-Yehuda corpus
  carried by pitomadom (public-domain Hebrew literature; blocks split
  at the corpus's own `***` separators), prefix to the last newline
  within the first 120000 bytes of each block; `shoresh.txt` held as a
  reserve third source.
- English: `dracula.txt` and `frankenstein.txt` prefixes under the same
  cut law.
- The court binary takes corpus paths as arguments and embeds none.

## Metrics, sealed (computed per language, never mixed)

For each language, a two-source field is built under each law; then:

1. **Attribution**: every fragment's cut clause becomes a prompt, the
   origin fragment is excluded, per-source cap and k as in unfold;
   metric = share of prompts whose top-1 answer comes from the origin's
   source.
2. **Self-retrieval**: the full fragment text as prompt, nothing
   excluded; metric = share whose top-1 is the fragment itself.
3. **Gram diversity**: distinct trigram atoms-windows divided by total
   window instances over the field's tokens.
4. **Similarity profile**: mean pairwise cosine over all fragment
   pairs, and the share of pairs above 0.5 — the collapse detector.

## Sealed predictions

- English: all three laws agree to within noise; on a pure-ASCII
  fixture L-byte and L-u8b are BIT-IDENTICAL (a battery gate, cmp).
- Hebrew: L-byte shows collapsed gram diversity and an inflated
  similarity profile against L-cp; attribution and self-retrieval
  degrade. L-u8b tracks L-cp exactly on valid text.

## Decision rule, sealed

The byte law loses if, on Hebrew, (attribution(L-cp) −
attribution(L-byte)) >= 0.10 or (self-retrieval(L-cp) −
self-retrieval(L-byte)) >= 0.10. If it loses, the field law bumps to
`body1-utf8-or-byte-v2`: the grave's bytes stand untouched, every
field replays under the new atom law, the proposer and the school
recompute as the derived organs they are, and the whole battery reruns
under both hands with the second hand auditing the bump as its own
turn. If it holds within margin, the difference stays a named
experimental boundary and the court's report stands as its measure.
Either way the verdict is printed by the machine from the sealed rule,
not chosen after the numbers are seen.

## The instrument

`mycelium/unicode_court.cpp` — a read-only C++17 instrument, built
strict like both hands; it writes no state, prints one deterministic
report, and carries its own gates in the battery: the pure-ASCII
bit-identity of L-byte and L-u8b, decoder edge rows (overlong, bare
continuation, truncated sequence, surrogate range, U+10FFFF boundary),
determinism across two runs, and a word-level parity probe of L-cp
against the prototype's own embed on sample words (one Python
invocation under Oleg's standing word for this arc's fixtures).
