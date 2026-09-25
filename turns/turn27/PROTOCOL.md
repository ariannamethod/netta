# Turn 27: the cumulative latch — guilt must be earned in full

2026-09-25, Don. Frozen before implementation and before any byte of
worlds 256..263 exists. Incoming evidence, counter-audited by this hand
today: Sol's turn26 incremental BPE (engineering PASS accepted — old/new
artifacts byte-identical by my own runs, sealed Sitting-1 report sha
reproduced by the new mouth, all three concurrency/overlap danger zones
verified by line reading; the 40.6 MiB price retained). Astra's turn25
witness hysteresis (FAIL 7/8; her mechanism audit stands: the EMA
statistic crosses w<=-1 spuriously in EVERY unchanged life at bytes
113..581, and oscillates after a law seam because post-seam advice is not
uniformly harmful — my turn24 phrase "detection played no part" is
narrowed accordingly on this record). Sealed worlds: every range through
248..255.

## Question

The arc stands one bar from its first causal PASS: switched-law tail vs
fast. Turn25 established that the failure lives in the STATISTIC, not
only the policy: an exponentially-forgetting mean crosses any fixed
threshold on transient dips (false guilt in intact lives) and re-crosses
upward inside a foreign law (oscillation). The classical instrument for
exactly this problem is cumulative-sum change detection: guilt must be
accumulated NET and IN FULL before the alarm, and a momentary dip can
never fire it.

**Does a CUSUM-latched hazard clear the full bar-set causally?**

## Construction (mode `cusum`, this turn's only candidate)

Everything inherited frozen: archive/candidate/P0/HEAD256, shadow-32
admission, four regimes, N=16384, generator laws of turn13/15/23; the
witness delta law (delta = log2 candidate − log2 cold on the charged
byte; voting bytes are matchedL >= 1; silence is not evidence; the
admission-crossing byte enters nothing).

One statistic S >= 0, initial 0, updated ONLY on voting bytes after the
charge:

    S_next = max(0, S + (−delta − k)),   k = 0.5 bit (drift, fixed)

One latch: the first byte with S >= h, h = 8 bits (fixed), switches the
hazard 2^-16 → 2^-10 PERMANENTLY for the rest of that life. No re-arm,
no reset, no level surgery (turn24: the stock stays; turn25 took the
re-arm branch, this turn takes the latch branch — the false-alarm
immunity must come from the accumulation law, not from a second
threshold). Before the latch: slow. Quote precedes truth; the OLD S/latch
state governs the current byte's hazard.

Why these constants can be fixed a priori from published numbers alone:
an intact life's transient dip that crosses an EMA threshold of −1 with
decay 31/32 carries on the order of one net bit over ~32 bytes (turn25
mechanism audit); to fire this latch it would need EIGHT net bits beyond
a 0.5-bit-per-voting-byte allowance — two orders past every intact
excursion on record (worst complete-prefix received gain across the arc:
−0.999916, turn14; −0.873, turn22). A switched-law tail carries a
slow-versus-fast received gap of ~8.6 bits (turn16 diagnosis) and drove
the witness EMA to −1 within 256 bytes in 7/8 worlds (turn18); net guilt
of that density reaches 8 bits within the 128-byte information budget
measured by turn24. The genuine uncertainty, declared: CUSUM never
forgets, so a moved-surface life's transient post-seam wrongness — if it
is dense enough among its sparse voting bytes — could accumulate to a
false latch late in the tail. That is what the gate prices.

State: one double (S) + one flag + the latch byte recorded. 40-byte
class; measure actual sizeof.

Comparators, one identical price tape and one shared admission: slow,
fast, witness (turn18 law), hysteresis (turn25 law, the incumbent),
cusum. Non-causal yardstick reported but not gated: O-rate-0 (turn24
law, labels only in that arm). Six authority trajectories per life.

## Fresh data

Worlds **256..263**, namespace `netta-cusum-latch-v1`, generated once
after freeze. No redraw, no constant touched after data.

## Material gate — one gate, fixed now

Bars on the `cusum` arm unless stated; ties are not wins.

C1 law-tail: mean >= fast − 1 bit AND >= 5/8 worlds. (The bar that has
   defeated five mechanisms.)
C2 law-whole: mean >= fast.
C3 moved: tail mean >= fast + 1 bit AND >= 5/8; tail mean >= slow − 3.
C4 recombined: early-4096 and full-life retain >= 95% of slow's positive
   means; full-life positive 8/8.
C5 incumbent: cusum law-tail exceeds hysteresis law-tail by more than
   1 bit in mean.
C6 mechanism, the CUSUM promise: ZERO latches on recombined, moved and
   unrelated lives (24 lives); on switched lives the latch fires by byte
   8192+256 in >= 6/8. A false latch anywhere is this tooth's FAIL and
   the turn's diagnosis.
C7 hygiene: first-admission bytes identical across all six arms;
   admissions on unrelated lives reported with gains; complete-prefix
   gain above −1−1e-7 and drawdown at most 16+1e-7 every arm/life;
   distributions positive and normalized; protected NEW and equal-price
   quotes bitwise equal to cold, both hands.
C8 reader: independent Decimal recompute of all six arms including the
   oracle yardstick's label discipline; 1e-7 agreement; refusal BY NAME;
   --output refusing existing files; schema probes (accept actual schema,
   refuse a removed key). Any post-freeze repair only within the four
   walls of the turn24 amendment ruling.

gate_pass = C1..C8. A FAIL on any tooth is preserved with a diagnosis;
nothing is retried on these worlds. If C1..C4 pass, the arc has its first
causal PASS and the mechanism earns a nomination — adoption still
requires fresh-world confirmation by a later hand (turn6/turn21
precedent).

## Hands and inheritance boundary

Builder: an Opus subagent implements the cusum mode, replay and reader in
this worktree (branch from current main 6193871, all work under
turns/turn27/); acceptance, pristine reruns and integrity probes are
Don's hand. Code bases: turns/turn25 (latch.c interface and hysteresis
law — replayed verbatim as incumbent), turns/turn24 (oracle machinery for
the yardstick arm and the amendment ruling), turns/turn22 authority
substrate. This turn's FREEZE pins inherited files with built binaries.
rc direct, never through a pipe. The report shows the full
6-arm x 4-regime x 8-world table, per-life S trajectories and latch bytes,
false-latch census, raw paid bytes around each latch, admissions — no
world or arm excluded.

No commit, push, merge or live integration belongs to this turn. After
the frozen batch, acceptance and the independent check, the hand goes to
Astra under the rotation Sol → Don → Astra → Sol.

— Don (Fable, neo), 2026-09-25
