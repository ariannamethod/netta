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

## Sitting and second-hand audit

The instrument landed only after this contract became merged history.
On the pinned corpus it printed `HOLDS`: root MAP is
0.88118061721002894 for L-byte and 0.61121304918311958 for L-u8b,
delta -0.26996756802690935 against the sealed +0.10 loss threshold.
L-byte top-1 is 37/37; the 2664 rows are sealed by FNV-1a-64
`a1a2b9df758ddfe5`.

The later hand reconstructed all eight declarations from the sealed
table, checked the 68/598 pair arithmetic, reproduced 127/127 gates,
and independently verified every query, candidate, family label, rank,
score order, tie rule, aggregate, row digest and the verdict with
`root_court_check.py`. Reordered rows and a canonically reprinted forged
summary both refuse. The sealed prediction that L-u8b would recover the
neighbourhoods is falsified; the sealed verdict stands, and body 4 may
open on L-byte.

A repeated strict `-O2` build reproduced the report byte for byte. An
`-O1` ASan/UBSan build was silent and preserved every candidate order,
aggregate, MAP delta and the `HOLDS` verdict, but changed the final bits
of some printed cosine rows and therefore sealed those rows under
`abe9b9221eb13d8b` rather than `a1a2b9df758ddfe5`. The row digest names
one sitting under one floating-point build; it is not a portable digest
of the law. Rank, recomputed aggregates and verdict are the portable
claims established across the two builds. No numeric rule is changed
after the sitting to conceal that boundary.

The causal reading is narrower than "bytes are universally better".
On these Hebrew forms, a three-byte window alternates between carrier
grams like `d7 xx d7`, which behave as character unigrams, and transition
grams like `xx d7 yy`, which behave as character bigrams. Post-hoc
ablation gives root MAP 0.8601 from carrier grams alone and 0.7603 from
transition grams alone; codepoint unigrams plus bigrams reach 0.8578.
Thus the current byte law earned its verdict through a useful accidental
multiscale recognizer, not through undifferentiated prefix mass. This
Hebrew court does not prove the same geometry for every UTF-8 script,
and the recognizer remains versioned so a later multilingual court can
ask that new question without reopening this one.
