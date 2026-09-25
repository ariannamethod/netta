# Turn25 — witness latch with evidence for return

2026-09-22, Astra. Written before implementation and before fresh worlds.
Domain: causal statistical prediction of synthetic byte streams.
Incoming commit: `798085be9a2bf8b6c41860238d4f38450558e126` (Don24).

## One question

Can a witness retain the fast withdrawal rate through neutral observations,
and return to the slow rate only on positive evidence, while preserving useful
transfer on unchanged and surface-renamed worlds?

The incoming oracle showed joint satisfaction of eight utility bars with a
hidden regime label. Its largest passing sampled rate delay was 128 bytes.
This is not a guarantee for a causal detector. In Don24's retained traces all
eight unchanged lives already cross the witness threshold before byte 8192
(first crossing at bytes 113–581). Therefore we choose the explicitly offered
re-arm variant, not a first-crossing permanent latch. No candidate prices have
been replayed on those worlds to select parameters.

## Fixed law and chronology

Retain the turn22 P0/candidate, shadow admission at 32 bits, signed witness
clock, and slow/fast source-to-cold transition probabilities 2^-16 / 2^-10.
The one new state is a bit `fast_latched`, initially zero. On each byte:

1. Quote from carried source/cold odds, before observing truth.
2. If active before this byte, charge that quote and update odds with the
   hazard selected by the OLD latch (zero = slow, one = fast).
3. If active and `matched >= 1`, update `w = (31/32)*w + candidate - cold`.
   Otherwise retain w. Candidate and cold here are log2 prices of the truth.
4. If updated w <= -1, set the latch. If updated w >= +1, clear the latch.
   Between these thresholds keep its previous value.

First admission initializes odds, w and latch to zero; its crossing byte is
still quoted by P0 and excluded from the clock. The latch cannot inspect byte
position, regime, seam, generator labels, or future data. No odds cap, reset,
additional admission, or new source archive. The two thresholds are a fixed
symmetric one-bit deadband, not fitted to any recipient outcomes. Only this
law is tested. The clock keeps running after a latch or return.

Comparators: the unchanged slow, fast and witness laws. All four receive the
same input price tape and have identical admission. The witness comparator
uses the old w threshold, so its first fast update has the same chronology as
the candidate; only permission to return to slow changes.

## Fresh data and retained substrate

Worlds 248..255, namespace `netta-witness-latch-v1`. Eight worlds, each with four
source lives and four recipient regimes: recombined, switched, moved_mid,
unrelated. Every life has 16384 raw bytes; the two constructed changes occur
at 8192. Use the unchanged turn13 generator, source learner, 528-byte episode
archive, and HEAD256 causal frontend; the surface bijection is the unchanged
turn22 generator. Predictors receive only raw bytes, then precomputed causal
price records for authority replay. The construction labels are used only in
analysis. All earlier worlds are sealed evidence, not a tuning set.

## Material gate — all eight inherited bars

Compute paired differences per world, then the arithmetic mean over eight.
Tail is bytes 8192..16383; early is bytes 0..4095; whole is 0..16383. Gain is
the sum of log2(P_live/P0) for the observed bytes. Higher is better.

1. Switched tail mean relative to fast >= -1 bit.
2. Switched whole-life mean relative to fast >= 0 bits.
3. Moved-surface tail mean relative to fast >= +1 bit.
4. Moved-surface tail improves on fast in at least 5/8 worlds (strict >0).
5. Moved-surface tail mean relative to slow >= -3 bits.
6. Recombined early mean retains >=95% of the positive slow mean.
7. Recombined whole-life mean retains >=95% of the positive slow mean.
8. Recombined whole-life gains are positive in all 8/8 worlds.

These are exactly Don24's eight bars. Law-tail individual wins and the
comparison with witness are reported without adding success conditions.
No adjustment of thresholds, data, or law after seeing the batch.

## Validity and measurement

- Shared admission; unrelated admissions reported with their gains, as in
  turn24. Their absence is not an extra material gate.
- Every source episode archive <=528 bytes.
- Every whole-prefix gain >=-1-1e-7; drawdown <=16+1e-7 (fast <=10+1e-7).
- Finite positive normalized distributions (frontend error <=1e-8); exact
  P0 quotes on inactive, equal-price and protected NEW observations.
- Latch transitions obey the fixed chronology; neutral observations cannot
  release a fast latch; no clamp/reset. Both set and release occur in a fixed
  handcrafted pre-freeze fixture. Check control equality against the unchanged
  turn22 implementation on that fixture.
- A separately written reader reconstructs four authority laws in probability
  space from the pinned input tape and compares every quote/state and all
  material metrics within 1e-7. It does not claim an independent rebuild of
  HEAD256 or source books. It checks the code and artifact manifests, refuses
  a named changed artifact, and writes only to a fresh requested output path.
  Check the actual writer result schema before freezing, including a missing
  required field. Freeze source, protocol, reader and binaries before data.

Report material and validity results separately. Overall acceptance requires
both. Preserve a failed batch and diagnose it; do not run a second candidate.

## Observable output and resources

Retain raw input bytes, source books, complete price and authority traces,
first set/return positions, counts and time spent fast before/after the seam,
per-world gains, and concrete rows showing help and harm. Report the first
crossing rather than substituting a post-seam median for it. A constant number
of floating-point operations and one bit per active life are added; report
actual C struct bytes (including padding) and serialized archive bytes.

## Scope and next hand

Work only in this isolated clone under turns/turn25. Canonical Netta, mouth,
mycelium and all older turns stay unchanged. No commit or push in this task
without Oleg's instruction. Finish with audit, raw result, verdict, and handoff
to Sol. Carry Oleg's separate request about C/Python speed into that handoff;
it is not a second experiment within turn25.
