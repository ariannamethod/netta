# THE ROOT COURT: SUBWORD INHERITANCE

Preregistered after both Unicode sittings and before this court's
instrument exists. Those sittings measured source attribution, a
self-query, rank shape and corpse composition. They did not ask the
question that exposed the byte/codepoint split: does the field preserve
useful character-trigram resemblance among related non-ASCII word
forms? This court asks only that question. Body 4 does not open before
the answer is committed and independently audited.

## Material, sealed

The gold labels come from the corpus, not from a new Hebrew analyser.
`pitomadom.c/shoresh.txt` is pinned whole at FNV-1a-64
`05d2840d282a7f14` (71280 bytes). Its explicitly labelled extended-root
section states these eight families, 37 distinct surface forms:

```text
שלח: שלח שליח שליחות משלוח
כתב: כתב כתיבה מכתב כתובת
ספר: ספר סיפור ספרות מספר ספריה
למד: למד לימוד תלמיד תלמוד מלמד
בנה: בנה בניין מבנה בנאי תבנית
שמר: שמר שמירה משמר משמרת שמורה
פתח: פתח פתיחה מפתח פתיחות
חבר: חבר חיבור מחברת חברה חבורה
```

The future instrument takes the pinned file as input, verifies its
digest, strict-decodes the whole file, and verifies that the eight source
lines behind this table occur once. In each source line the first form
is bare and later forms carry Hebrew's coordinative U+05D5; the sealed
table above explicitly removes that one syntactic prefix and nothing
else. The instrument uses exactly the 37 table forms, not an inferred
parse. A digest, declaration or UTF-8 mismatch is refusal, not a partial
court.
Inspection of the labels was allowed to define this court; no embedding,
ranking or metric over them was run before this preregistration.

## Arms and constants

- **L-byte:** every UTF-8 byte is one atom.
- **L-u8b:** strict RFC-3629 scalars are atoms; an invalid byte is the
  tagged atom `0x110000 + byte`.

L-cp is absent because the pinned material must be valid UTF-8, where
L-cp and L-u8b are exactly one arm.

Everything after atomisation is identical to body 1: boundary atoms
`^` and `$`, windows of three atoms, FNV-32 folding and DIM-96 expansion,
sum followed by L2 normalisation, cosine comparison. Each listed form
is embedded directly. There is no sentence tokenizer, stop list,
segmentation, root extractor, gematria, training, or imported Pitomadom
logic. A fixed pure-ASCII fixture must make both arms bit-identical.
That fixture is, in this order: `raven`, `shalom`, `cafe`, `root`,
`rooted`, `roots`.

## Metrics, sealed

Each of the 37 forms queries the other 36. Scores sort by descending
cosine, with UTF-8 byte order as the deterministic tie-break.

1. **Root top-1:** share of queries whose first result has the same
   corpus-declared root.
2. **Root MAP:** mean average precision over all other members of the
   query's family. For a query with `r` relevant peers,
   `AP = sum(precision@k for relevant ranks k) / r`; root MAP is the
   arithmetic mean of the 37 AP values. This is the primary metric.
3. **Separation:** mean cosine for unordered within-family pairs and
   mean cosine for unordered between-family pairs.
4. **Fixed-cone classification:** at cosine >= 0.5, true-positive rate,
   false-positive rate, and balanced accuracy
   `0.5 * (TPR + 1 - FPR)` for the same-root label.

The instrument prints every aggregate at `%.17g`, decides from the
unrounded double aggregates, and emits every ranking row as
`law<TAB>query<TAB>rank<TAB>candidate<TAB>score<TAB>same-root`, in arm,
query-table and rank order, newline sealed. Its FNV-1a-64 row digest is
over those exact bytes. It writes no state. The later audit recomputes
MAP from the rows rather than trusting the summary alone.

## Prediction and decision rule, sealed

L-u8b is predicted to recover Hebrew morphological neighbourhoods and
raise root MAP over L-byte. The byte law loses if

```text
root_MAP(L-u8b) - root_MAP(L-byte) >= 0.10
```

Root top-1, separation and fixed-cone classification are diagnostics;
they cannot overturn the single primary rule after the numbers appear.
If the byte law loses, body 1 bumps to `body1-utf8-or-byte-v2`: graves
and ledgers remain evidence, fields replay, derived proposer and school
state is rebuilt, and every prior body is remeasured under the new law
before body 4. If the byte law holds, the parliament may open on L-byte
with both observed cone limits recorded. Either outcome is cheaper now
than discovering the wrong atom geometry after weights acquire power.

## Instrument boundary

The future `mycelium/root_court.cpp` is a read-only C++17 instrument,
standard library only, strict-built and sanitizer-clean. Its gates must
cover the corpus digest and declaration grammar, all 37 unique forms,
the expected 68 within-family and 598 between-family unordered pairs,
ASCII arm identity, invalid-byte tagging, deterministic output, ranking
row coverage, and independent recomputation of the printed verdict.
This file is the contract; the instrument deliberately does not land in
the same commit.
