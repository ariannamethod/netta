# Turn20: witnessed two-level commitment ceiling

2026-09-21, Sol. Written before implementation, freeze or generation of
worlds208..215. Incoming turn19 audit is `INCOMING_AUDIT.md`. One mechanism,
one fresh batch, one material gate. Rotation: Sol -> Don -> Astra -> Sol.

## Question and causal law

Can a memory influence limit remain loose while its witnessed advice is
useful, tighten after witnessed wrongness, and then recover from subsequent
useful evidence? Compare with Astra's fixed five-bit ceiling, uncapped
witness and the inherited slow, fast and adaptive controls on the same
candidate/P0 tape. The 528-byte joint-prefix archive, source selection,
HEAD256 frontend, local P0, seven candidates, shadow32 admission and
matched-length-gated witness evidence are unchanged.

Add one mode, `witnessed_ceiling`, with no new state field. At byte t:

1. Quote from the current source/cold odds z and current candidate and P0.
   Charge the observed byte under that quote.
2. Use the OLD witnessed evidence w to choose the old witness hazard:
   h=2^-10 if w<=-1, otherwise h=2^-16. Update odds by the inherited
   one-way Bayes/hazard law.
3. If the episode candidate matched a stored record (`matchedL>=1`), set
   `w_new=(31/32)w+(candidate_log2-P0_log2)`; otherwise leave w unchanged.
4. Set the NEXT quote's maximum odds to **5 bits if w_new<=-1**, otherwise
   **10 bits**: `z_next=min(z_hazard, cap(w_new))`.

The first admission and its crossing byte are identical to every control;
admission starts z=w=0 on the following byte. Equal candidate/P0 and
protected NEW quotes are exactly P0. No future truth, seam, regime label or
surface map enters the law. Cap10 is the pre-existing fast hazard's approx
attainable odds level; cap5 is Astra's measured fixed cap. These levels and
the -1 witnessed threshold are chosen from prior mechanisms, not scanned
on any fresh world. The mode may rebuild odds from NEW paid evidence after
w recovers; transferred capital is not refunded retroactively.

Clipping transfers source mass to absorbing cold mass, never the reverse.
The one-bit complete-prefix loss bound therefore remains. Odds never exceed
10 at an active quote, so every interval loss is bounded by
`log2(1025)=10.001408194` bits. During a negative-w next quote the stronger
local cold floor is 1/33; intervals crossing both cap states only have the
global 10-bit guarantee. These are mathematical bounds, not claimed utility.
Prediction state remains 32 bytes per mode; the archive remains <=528 bytes.

## Fresh batch and freeze

Worlds208..215, namespace `netta-witnessed-ceiling-v1`; four independent
16,384-byte source lives per world, and recombined, switched-law,
mid-life surface move and unrelated target lives. Changed targets share the
first 8,192 bytes; the move applies an independently seeded byte bijection
only to the recombined tail. All source/generator laws are inherited. No
target from turns<=19 is a tuning input. Protocol, C code, Python writer,
independent reader, inherited inputs and binaries are hashed before any
fresh byte is generated. Run generate -> extract -> learn -> evaluate once.
There is no rate/cap sweep, redraw or threshold edit after seeing data.

## One material gate: all conditions required

Gains are paid log2-probability savings relative to the identical P0.
Positive paired differences are wins; ties are not. All metrics below
refer to the new mode unless explicitly named. Its complete gate is:

1. **Surface tail:** mean new-fast >1 bit, >=5/8 wins, and mean new-slow
   >=-3 bits (three conditions). Mean new-witness >=-3 bits is a separate
   recovery check (one condition).
2. **Changed-law tail:** mean new-fast >=-1 bit, mean new-slow >1 bit and
   >=5/8 wins versus slow, mean new-witness >1 bit (four conditions).
   Mean new-fixed-ceiling >=-2 bits is a separate protection check (one).
3. **Unchanged recombination:** retain >=95% of slow's positive mean
   early4096 and whole-life gains; whole-life gain >0 in 8/8 (three).
4. **Mechanism:** the first admission and complete witnessed-evidence/hazard
   trajectory match uncapped witness; the upper10 and lower5 caps are never
   violated; each level clips in at least one related life (three).
5. **Structural controls:** no episode admission on unrelated targets;
   source archive <=528 bytes; all distributions positive and normalized;
   all inactive/equal-price and protected-NEW quotes exactly P0; complete
   prefix loss <=1+1e-7 and interval drawdown <=log2(1025)+1e-7 for the
   new mode (five).

The material gate therefore has **20 booleans**. A separately implemented
Decimal mass reader must independently rebuild source books and all seven
candidates, replay all six authority modes and recompute every boolean to
within 1e-7 bit. Its inherited explicit boundary is HEAD256/P0 input from
retained raw traces, as in turns13–19. A verifier failure cannot be counted
as material improvement. Prior clock quiet-share counts and changed-law
reaction times are retained as diagnostics, not repeated as material teeth:
this step does not change the clock and the prior quiet-share condition
already failed identically for witness and cap.

Report every world/regime/mode with early, whole and final8192 gain;
admission, odds, caps, clips, clock counts, minimum prefix and drawdown.
Show exact charged bytes where the new mode helps and harms against the
fixed cap and witness, plus whole-life costs beside tail benefits. A FAIL
is retained. Diagnose using the single saved batch without a second
policy run on it. Hand the result to Don, then stop. No live integration,
commit, push or merge is conveyed by this protocol itself.
