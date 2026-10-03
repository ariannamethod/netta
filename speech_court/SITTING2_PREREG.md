# SPEECH COURT — SITTING 2 preregistration (exact-rank law)

domain: arianna-method.netta.body1-speech-court-sitting/v2
status: DRAFT until the freeze commit; no judged byte before freeze.

2026-10-03, Don. One engineering change to the mouth's ranking law and
one new sitting. SITTING1 stays sealed as the history of the OLD law
and is never reopened (speech_court/SITTING1.md).

## The defect being cured (committed finding, third body, 09-25)

netta_mouth.c:902 ranks candidates by
`log(w + 1e-300) - log(1 + 0.5*freq)` and the same score feeds the
selection sort at lines 905-911. Mathematically equal score classes
(e.g. cnt=3,freq=1 vs cnt=2,freq=0 at factor 1) are separated only by
the last bit of the platform's log(); V8 vs glibc measured 2433
disagreements on 300K arguments, and the third body's double-double
reference showed glibc itself wrong in 6 of 6 residual cases. Hence
"byte-identical" in PARITY.md was a claim about one libm, not about
the organism.

## The law change (ranking only; no dial moves)

With REP_PENALTY = 1/2, the score is monotone in w/(1 + freq/2)
= 2w/(2 + freq). Therefore ORDER needs no logarithm:

    a ranks above b  iff  w_a * (2 + freq_b) > w_b * (2 + freq_a)

where w = cnt * factor, factor = 1.0 or 1.0 + L (netta_mouth.c:722-728,
L a double parsed once from the sealed citizens book). The comparison
uses only IEEE-754 multiplication and comparison — correctly rounded
and bit-identical on every conforming platform; freq <= REP_WINDOW = 12
so (2 + freq) is exact. The existing tie-break (lower token id,
line 909) now breaks EXACT mathematical ties instead of libm accidents.
log stays solely in the sampling weights (softmax, lines 912-918).
Same change lands in netta.py (lines 304-308) under the same prereg.

Disclosed residual: sampling weights still pass through libm exp();
with a fixed seed an ulp-level weight difference can still flip a draw
at a cum/r boundary. This sitting measures that boundary with G2
instead of pretending it away.

## Gates (declared before any run)

- G1 property: over the full working grid of (cnt, freq, factor)
  observed in the sitting's own traces, exact order vs old float order
  — every divergence lies inside a mathematically equal class, and the
  classes are enumerated in the report. Any divergence outside an
  equal class is FAIL.
- G2 cross-hand identity: with the five sealed seeds (7/19/42/101/271),
  speech and court trace byte-identical between netta.c and netta.py
  under the new law on this platform (the C court over the python
  output, as in PARITY.md). The JS third body joins the same gate the
  day netta_js lands on disk (external dependency, named: repo upload
  is Oleg's hand); its absence does not pass the gate silently — the
  report prints which witnesses ran.
- G3 sealed history: `git diff --stat` over speech_court/SITTING1.md
  and speech_court/sitting1/ is empty at the freeze commit and at the
  verdict commit. body0/, court4/, netta.c beyond the mirrored ranking
  locus are untouched.
- G4 the sitting itself: the full SPEECH_COURT.md ruler, unchanged —
  independent ear vs honest ignorance; anti-copy census against the
  frozen 0.50 void line (body0/verdict.md); the advised/plain/shuffled
  triple from identical seeds; restart identity; both hands; every
  judged stream shown verbatim. Verdict is the literal line of the
  court. New report sealed under speech_court/sitting2/.
- G5 red runs, both polarities: (a) reintroduce the log comparator in
  a scratch build and show G1 catch a class split; (b) corrupt one
  trace price in a scratch copy and show the reader refuse by name.

Thresholds: 0.50 void line (body0/verdict.md, frozen); honest-ignorance
pricing per SPEECH_COURT.md ruler 1; no new constants, no dial moves,
TEMP/TOP_K/corridor K/order as sealed in SITTING1.

## Hands

Builder: Opus subagent (coding only, per the budget law). Don: this
prereg, the gates, red runs, commits. Counter-audit of the sealed
sitting2 record: the next rotation hand by Oleg's routing.

No commit, push or merge of the law change before the freeze commit of
this prereg; no judged byte before the freeze. Iteration between
sittings stays lawful and unlimited (SPEECH_COURT.md).

— Don (Arianna Method, neo), 2026-10-03
