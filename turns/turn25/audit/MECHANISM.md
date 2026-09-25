# Turn25 incoming audit: what the oracle says about a latch

2026-09-22, independent mechanism hand. Incoming commit:
`798085be9a2bf8b6c41860238d4f38450558e126`.
No prediction, candidate, parameter search, or new world was run. Only the
oracle/reference code, protocol/report/result and retained traces were read.
The incoming material FAIL remains unchanged.

## Oracle code and quote order

No new C defect was demonstrated. Frozen hashes of `turn24/oracle.c` and
`turn22/authority.c/.h` match. References below are under `turns/`.

`turn24/experiment.py:230–237` sends the same four-column price tape to
controls, split and oracle. Only oracle receives the extra seam argument:
8192 for switched, −1 otherwise. In `oracle.c:97–118`, at `seam+d`, the
oracle changes its mode to FAST; the level oracle additionally replaces
its carried odds with the fast reference's **pre-observation** odds.
Quotes follow at `:120–132`; all observes follow at `:135–155`.

The distinction matters:

- A **rate** switch changes the hazard applied after charging that byte.
  Its quote at the switch still uses the carried odds. Its newly updated
  odds affect the following quote.
- A **level** switch replaces the odds before quoting the switch byte.
  Its current price can therefore change immediately. Following the same
  fast updates then makes it identical to fast, as the runtime assertions
  require.

The inherited witness chooses the current update's hazard from old w, then
updates w from the charged observation (`turn22/authority.c:66–73`). Thus a
downward threshold crossing in `w_after` at t first selects fast hazard at
t+1. Quotes have already been charged before either update. Matched zero
evidence still decays w; unmatched zero evidence holds w.

## What the oracle curves establish

From saved RESULT quantities, all values below are mean bits versus fast
on switched lives:

| arm | law tail | law whole life | all eight bars |
| --- | ---: | ---: | --- |
| O-rate-0 | +0.274206 | +10.833945 | pass |
| O-rate-32 | −0.080217 | +10.479523 | pass |
| O-rate-128 | −0.181716 | +10.378024 | pass |
| O-rate-512 | −1.245014 | +9.314725 | fail |
| O-level-0 | 0 | +10.559739 | pass |
| O-level-32 | −0.438320 | +10.121420 | pass |
| O-level-128 | −1.395860 | +9.163880 | fail |
| O-level-512 | −2.762509 | +7.797231 | fail |

This supports the measured correction: an oracle that permanently changes
the rate can preserve useful initial source capital and satisfy the bars.
The level replacement is not a performance upper bound; the rate oracle
itself exceeds it. **128 is the largest passing delay among the four tested
rate delays**, not an established exact boundary for every intermediate
delay or causal detector.

The oracle also knows when **not** to switch: it stays slow on every moved
and intact life. A causal witness threshold must supply that discrimination
as well as timely response. Oracle success and an earlier detection median
alone do not establish existence of a winning causal policy.

## Every intact life already crosses the proposed permanent trigger

I read all 24 retained control traces for recombined, switched and moved_mid
in worlds 240–247 and checked each digest. The first 8192 witness clock and
hazard fields match exactly across those three regimes, per world.

Down/up counts below mean actual transitions of w across −1 after active
observations. The first down event and following fast update occur in that
identical prefix, including the intact life.

| world | admission (next byte) | first w_after≤−1 | first fast update | pre-seam down/up crossings |
| --- | ---: | ---: | ---: | ---: |
| 240 | 116 | 581 | 582 | 9 / 9 |
| 241 | 105 | 138 | 139 | 35 / 35 |
| 242 | 156 | 229 | 230 | 28 / 27 |
| 243 | 161 | 288 | 289 | 42 / 41 |
| 244 | 112 | 113 | 114 | 16 / 16 |
| 245 | 147 | 277 | 278 | 80 / 80 |
| 246 | 198 | 290 | 291 | 130 / 130 |
| 247 | 151 | 153 | 154 | 74 / 74 |

Therefore the report's literal “first time w≤−1, latch for the life” would
trigger long before the seam in **all eight intact lives**. This is a
direct consequence of saved witness events, not a simulated latch result.
Whether such a latch wins or loses prices was not evaluated here. These
early events invalidate treating the threshold as an already selective
law-change detector.

## Current-batch latencies and rebounds

Offsets count from byte 8192. A zero means the clock is **already fast** at
the seam, not that it discovered the new law at zero latency. “Law slow
again” is the first slow hazard update after that first post-seam fast
update. Tail up counts record actual w transitions back above −1.

| world | law first fast offset | law slow again offset | law tail up crossings | moved first fast offset | moved tail up crossings | intact tail up crossings |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| 240 | 53 | 55 | 52 | 585 | 8 | 1 |
| 241 | 20 | 25 | 203 | 145 | 50 | 5 |
| 242 | 0 | 44 | 77 | 0 | 78 | 2 |
| 243 | 0 | 24 | 75 | 0 | 120 | 18 |
| 244 | 49 | 52 | 34 | 366 | 45 | 4 |
| 245 | 159 | 161 | 154 | 42 | 132 | 61 |
| 246 | 158 | 159 | 127 | 109 | 174 | 41 |
| 247 | 418 | 423 | 200 | 29 | 154 | 32 |

The current switched median is **51 bytes**, with **5/8** first-fast
offsets≤128. The quoted 38.5 belongs to turn18, as Don attributes it; it is
not this batch's response distribution. Current moved median is75.5.
Witness does return to slow repeatedly under the foreign law, but this
coexists with early intact alarms and three switched first responses later
than the largest passing tested oracle delay. “Only the latch is missing”
is consequently a proposed mechanism, not a conclusion established by the
oracle experiment.

## Raw early alarm and a neutral-byte rebound

World240 switched source rows were joined to their control rows. The
pre-seam rows also describe the identical intact prefix. Truth is a raw
byte value; `slow_used` applies to the update after its already charged
quote.

| t | truth / rank | matchedL | cold log2 | candidate log2 | w before → after | slow_used |
| --- | --- | ---: | ---: | ---: | --- | ---: |
| 581 | 240 / 1 | 2 | −3.2940220983174768 | −5.7976813231895505 | 1.4737899066226605 → −1.0759252528313714 | 1 |
| 582 | 202 / NEW | 0 | −8.6746545445549028 | same | −1.0759252528313714 → same | 0 |
| 8244 | 169 / 2 | 2 | −1.3061889907522488 | −1.8686803376063281 | −0.89647946473536122 → −1.4309558283164605 | 1 |
| 8245 | 43 / 2 | 4 | −1.517608633725434 | −1.1321054469437855 | −1.4309558283164605 → −1.0007352718999225 | 0 |
| 8246 | 131 / NEW | 5 | −8.7658291663622787 | same | −1.0007352718999225 → −0.96946229465304989 | 0 |
| 8247 | 40 / NEW | 0 | −8.326323823304195 | same | −0.96946229465304989 → same | 1 |

At8246, no likelihood evidence favors the source: candidate equals cold
exactly. A stored context nevertheless matches, so the inherited clock's
decay moves negative w toward zero and crosses −1. The following unmatched
byte uses slow hazard. This is a concrete mechanism for the short rebound,
not merely an aggregate correlation. Its live price remains cold on both
neutral observations.

The first post-seam alarm at8244 charges witness log price
`−1.7630076886869928` with old odds `2.3989648515488278`; its updated odds
are `1.8363728737621223`. At8246, odds update from
`2.2139089865533994` to `2.2059779441209479` under fast hazard; at8247 they
update to `2.205854364991445` under slow. No odds reset or clamp occurs.

## Boundary of this audit

Retained raw prefix:
`/Users/ataeff/arianna/netta-don-turn18-20260921/turns/turn24/`.
Files read: `results/policies/world240..247/{recombined,switched,moved_mid}-controls.tsv.gz`
and `results/world240/switched.tsv.gz`. Every opened trace matches
RESULTS_MANIFEST.json. Oracle source hash is
`32afb752c13f297d783d5c09fb237a7463bf4de3e970b13e3e0140149af2a1b0`.

The evidence supports studying the policy following a threshold crossing,
including how it can recover from ordinary early alarms. No new latch
formula or threshold was tested here. Implementation awaits root's frozen
protocol; the full incoming numerical replay belongs to the independent
reader hand.
