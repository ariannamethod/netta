# Gutenberg arena: sealed preregistration

Status: **SEALED BEFORE THE FIRST NETTA RUN**  
Date: 2026-08-16  
Runner: `python3 gutenberg_arena.py WORK_DIR [--source-dir DIR]`

This is the first real-text arc promised before bodies 15 and 16 were built.
It is an arena for the existing organism, not a corpus on which to tune it.
No source, window, seed, rate, threshold, episode length, or prediction below
may change after a score has been observed. A failed prediction remains a
published result. The arena does not grant authority to the neural core and
does not enable the quarantined `--core-hebb-v1` arm.

## Immutable sources

The raw accessible plain-text downloads are:

| role | Project Gutenberg source | raw bytes | SHA-256 |
| --- | --- | ---: | --- |
| kin donor | Dracula, ebook 345, `https://www.gutenberg.org/cache/epub/345/pg345.txt` | 890348 | `96cd16eacdbfebae8fdda5591f66e0cc8ee76be18e0cd1aca02bc00615782d28` |
| kin target | Frankenstein, ebook 84, `https://www.gutenberg.org/cache/epub/84/pg84.txt` | 448885 | `7810cd483cffcf2cc8a1d8f0d5807931e69d4f48cd14149b8c76f88af82fead3` |
| technical alien | How Two Boys Made Their Own Electrical Apparatus, ebook 28335, `https://www.gutenberg.org/cache/epub/28335/pg28335.txt` | 258836 | `8308c35e7140d412f2f0d711e0ebef45c05bb941f44cecb503212a7442b577b6` |

For each file the runner verifies the raw length and SHA-256, maps CRLF and
lone CR to LF, and retains exactly the bytes strictly between the first
`*** START OF THE PROJECT GUTENBERG EBOOK` line and the following
`*** END OF THE PROJECT GUTENBERG EBOOK` line. It performs no case folding,
Unicode normalization, whitespace folding, or editorial cleanup.

The shuffled twin is a byte-for-byte permutation of the normalized
Frankenstein body. Fisher-Yates runs from the last index to one using an
explicit SplitMix64 stream seeded by `0x4e45545441475241`. The runner refuses
the twin unless all 256 byte counts and its length equal the source.

## Fixed arena

Netta is compiled with strict C11 at `-O2`. Donor lives use seed `160816`,
eight episodes of 8000 bytes. Transfer probes use one 4096-byte episode at
the three fixed offsets `4096`, `65536`, and `131072`; every trained probe
starts from an untouched copy of its donor state, and every newborn control
starts on the identical target and offset with seed `260816`.

All six shadow prices are recorded in bits per raw byte: `atomic-uni`,
`byte-bi`, `byte-tri`, `unit-uni`, `move-bi`, and `core`. For each witness,
transfer gain is `newborn_price - travelled_price`; the declared statistic
is the median of the three fixed windows.

The unit-rent voyage starts from the same Dracula donor state, lives four
8000-byte Frankenstein episodes, then two 8000-byte Dracula episodes. A
donor unit is *shared* when its exact bytes occur in both normalized books
before the crossing; it is *Dracula-specific* when absent from Frankenstein.
Death and resurrection are read only from `d` and `u` biography receipts.

The court voyage also starts from untouched Dracula donor copies. Its kin
and alien arms each live eight 800-byte target episodes. The 800-byte size
places the first visit inside bounded blind comity, so “early refusal” means
an `r` or `q` receipt in target episodes two or three, not the mechanical
large-episode null at a virgin shore.

## Predictions and fixed decision rules

Let `A`, `B`, and `T` be the median transfer gains of atomic, byte-bigram,
and byte-trigram witnesses.

1. **Kin transfer grows with context depth.** On Frankenstein after Dracula,
   pass iff `A > 0`, `B > A`, and `T > B`.
2. **Alien context is negative.** On Frankenstein after the technical donor,
   pass iff `B - A < 0` and `T - A < 0`. English byte-frequency familiarity
   is deliberately removed from this decision by subtracting `A`.
3. **Shuffling kills contextual excess.** On the shuffled Frankenstein twin
   after Dracula, pass iff `B - A < 0.1` and `T - A < 0.1` bit/byte.
4. **Vocabulary pays local rent.** Pass its three separately published
   surfaces iff at least one initially shared donor unit remains living after
   Frankenstein, at least one initially Dracula-specific unit dies there,
   and at least one such dead identity emits a resurrection receipt on the
   return to Dracula.
5. **The island court distinguishes kin from alien.** Pass iff the kin arm
   emits no refusal in eight episodes and the alien arm emits at least one
   refusal in target episode two or three.

The other three witness gains, every individual window, source hashes,
unit counts, court receipts, stdout, state, and biography remain evidence
even where they do not enter a binary prediction. The runner writes a JSON
record, a flat TSV table, and a verdict TSV. It exits nonzero only when the
protocol cannot be executed or verified; a scientific `FAIL` is a result,
not a broken run.

