# Turn30 DISTINCT HISTORIES — Don's acceptance record and verdict

domain: arianna-method.netta.turn30-distinct-histories-report/v1
2026-09-25. Builder: Opus subagent under the preregistered PROTOCOL.md.
Acceptance, the rulings, reruns and this record: Don's hand.

## Verdict

**Material FAIL: D1, D3 fail; D2, D4, D5 pass; D6 closed by the
independent reader.** Both hands agree on every tooth (writer's
RESULT.json holds D6 false with independent_reader_pending — the writer
cannot certify its own reader; the reader's VERIFY.json closes it:
3,276,800 forecasts, max error 3.392e-11). gate_pass = false. On the
same 12 addresses, with coverage fixed by construction, distinct A/B
histories carry no residual value under the frozen gate — they cost
192 portable bytes per world and tax the unchanged regime.

## The protocol defect — mine, named first

D1 gated the partial regime on the early(4096) window while partial
diverges from recombined at byte 8192. With one price tape, one
address set and one shared admission, every arm's trace is
bit-identical across those regimes up to t8191 — asserted by both
hands (shared_prefix_identity true) — so the gated tooth measured
distinctness on ordinary pre-divergence bytes and never saw the
regime it was aimed at. The FAIL stands as the frozen gate's lawful
verdict (D1 −31.464 bits, 0/8 — distinctness loses on ordinary ground,
which is itself an answer), but the tooth did not test what its name
claims. The window law for a seam regime is: the gated window opens at
the seam or the tooth is aimed at the prefix. Same error class as
turn27's threshold-variable lesson: the gate must be derived for the
bytes it actually reads. The ungated post-seam tail it missed:
partial tail +11.288 bits, 4/8 — not a win, not the gated claim.

## What the data says

- **Distinctness costs on agreeing ground.** D1: bank3 461.350 vs
  pooled2 492.814 on the shared prefix, 0/8. D3: recombined retention
  0.93615 early / 0.94370 full against the 0.95 floor. The price is
  structural: the three-way router spends probability mass holding two
  stories where one suffices.
- **Distinctness pays exactly where the target stops agreeing with the
  pooled story** — reported, not gated: switched +77.4 bits mean life
  (6/8), unrelated 194.5 vs pooled2's 99.8, above even the 24-address
  yardstick; per extra byte +0.4032 (switched) and +0.4934 (unrelated)
  against −0.66..−0.20 everywhere else.
- **But the mechanism is not discrimination.** The two mechanically
  selected extremes: the largest harm (world285 switched t9092,
  −4.614 bits) is the router committed to the WRONG history
  (weights 0.000194/0.999335/0.000471) — it bought a story, and pooling
  never buys one; the largest help (world282 moved_mid t14914, +5.505)
  is both history weights already near zero — the help is faster
  RETREAT to P0, not choosing the right history. Whether the
  switched/unrelated advantage is discrimination or mere dilution of
  commitment is exactly the next bounded question (below).
- **The material was there:** all 96 selected addresses have A ≠ B and
  all 96 rotated null records differ from their mirror. The FAIL is not
  an absence of divergence. D4 held everywhere (worst excess −100.228).
- **D5 hygiene complete:** admissions identical across the four gated
  arms in 40/40 lives, recomputed by the reader from the incumbent's
  candidate stream; worst prefix floor −0.999791; max drawdown 14.247;
  normalization 1.31e-14; protected NEW and equal-price quotes bitwise
  cold, both hands; the five unrelated-life admissions disclosed with
  their gains.

## Ruling on the admission clock

The protocol fixed "one shared admission per life" without naming whose
stream drives it — my under-specification. The builder drove it from
the incumbent pooled2's candidate stream and disclosed the decision in
INTERFACE.md: the candidate cannot open its own door, and any bank3
win would be priced after an admission it did not cause. Accepted as
the lawful conservative reading — conservative against the candidate,
which still lost on the gated teeth. full24 keeps turn29's own per-arm
admission law and is reported, not gated, so D5's identity tooth
scopes to the four gated arms. Recorded as precedent: a shared-event
law names its clock in the protocol.

## Acceptance performed (all rc direct)

- Gate recounted from RESULT.json by my hand: every tooth and quantity
  byte-matches the builder's report; both hands agree.
- FREEZE 28/28 re-digested, drift NONE. Identity hashes PROTOCOL
  fab5c1dc…, FREEZE 70df3daf…, RESULT e8535c84…, VERIFY 6dd86eff… match.
- router3.c read line by line: prior vector (0.125, 0.4375, 0.4375);
  exact-cold passthrough only when both sources equal cold; leak to the
  prior vector with renormalization and per-step validity checks;
  turn29's binary law verbatim for pooled2/permuted2; pooled = A+B and
  the rotation law enforced by exact equality at load; the three
  archives' address projections asserted identical at every step; all
  vectors precede truth.
- Pristine reader rerun to a fresh output: rc=0, **zero differing
  fields** against the sealed VERIFY.json.
- Integrity probe, my hand, a different artifact than the builder's:
  one bit changed in results/world283/switched.turn30.tsv.gz → refusal
  BY NAME with the full path, rc=1, no receipt written.
- Builder's preflight accepted: 11 tooth probes each reddening its
  target with zero collateral; schema probes both polarities (14 key
  removals refused by name); --output refusing existing paths; freeze
  refusing to run over existing data; C-vs-Decimal fixture agreement
  2.842e-14.

## Inherited-code observations (reported, nothing fixed)

turn28 bank.c eb_observe leaves its three-way posterior unnormalized
behind a 1e-10 guard where turn28's reader normalizes explicitly —
turn30 renormalizes in both hands; the rotation law exists twice
(t13.rotate_records, turn29 rotate_counts), identical; Apple clang
embeds the output path in the Mach-O, so frozen binary pins reproduce
only at the same absolute path — caveat recorded in REPRODUCE.md, the
inherited turn28/turn29 binaries rebuilt here byte-identical to their
pins.

## One bounded next question (a proposal, not this turn)

Dilution or discrimination. bank3's advantage where the pooled story
goes stale may come only from weaker commitment (each history starts
at 0.4375 < 0.875), not from telling A from B. Control arm on fresh
worlds: `pooled-half` — turn29's binary router with prior mass 0.4375
on the SAME pooled archive, one new constant, declared. If pooled-half
matches bank3 on switched/unrelated, distinctness is fully dead and
the whole effect is priced commitment; if bank3 stays ahead, distinct
histories discriminate and earn a design. Either way the pooled law
(turn29) remains the incumbent memory shape on ordinary ground — this
turn closed that branch.

— Don (Fable, neo). Rotation per Oleg's word: Sol → Don → Astra.
Worlds 280..287 sealed.
