# Turn 4 mechanism audit: better candidate, worse received tail

2026-09-16, Astra mechanism reader. Incoming merged main is
`cdcd00f560aabebbfb0980399d6192016e1d0600`, containing Sol `8f267f9`.
Source/code references below are relative to this repository. Saved raw
artifacts were read without modification from
`/Users/ataeff/arianna-codex/repos/netta-sol-local-cold-20260916/turn4`.
Only this report is written by this bounded audit. No new model, target,
parameter trial or fixture was run. The separate measurement reader owns
full regeneration/reproduction of the batch.

## Current implementation and repair chain

No demonstrated defect found in the final prediction code.

- `turn4/experiment.py:25` fixes priors `(1/8,7/16,7/16)`. `:113` computes
  prior-weighted capitals in log space. Rounded-zero diagnostic weights do
  not remove an arm from the actual log-space quote.
- `:205` selects a row/global capital pair, `:208` quotes from its previous
  state, `:213` prices through the outer mechanism, and only `:218` updates
  capitals. Current truth selects its price; it does not select prior weights.
- `:133` retains the old causal outer gate/HMM, including a cold crossing
  byte and odds 1 on the next byte. Both source components have exactly P0's
  NEW price, so their relative-capital update is zero on NEW.
- `turn4/verify.py:55` reconstructs normalized weights from absolute sequence
  capitals, independently of the evaluator's P0-relative representation.
  `:114` rebuilds component group laws, `:161` checks the full candidate
  group law, `:172` prices before `:180` updates the independent HMM, and
  `:186` updates row capitals after charging the observation.

Both recorded freeze links match the actual predecessor-file hashes:

```
CODE_FREEZE.json        88c2cad79b2d17250bf34375448ce2a1e73028228c1c4e8e04d75f8adad240db
CODE_FREEZE_REPAIR.json 7d529a8bc84b67b86399108c5430821c50edf3b610ad1d7d872bb1538d9347cd
CODE_FREEZE_VERIFY.json 7beeea10374aeea12054beb338d18629ffe734a8d04fd9d6542f50527937c2da
```

Every final source/binary hash in CODE_FREEZE_VERIFY matches the Sol worktree.
The recorded underflow repair is consistent with the final log-weight
implementation. The second verifier repair skips an absent compact baseline
activation field (`verify.py:219`); baseline admission is still reconstructed
at `:139`. Neither changes the final predictor's mathematical law. These
checks establish the retained chain and final artifacts, not an independent
observation of the historical wall-clock moment when a repair was made.

## Exact algebra of the two candidates and their live influence

Let S2 be BANK2-ROW's candidate for the current row, and let A/B denote the
same P2 component probabilities used by both arms. Since the three-way
prior gives A and B equal masses, its conditional A/B posterior is exactly
the two-way posterior. Let beta=1−w_cold be its combined source mass.
Then, for each possible byte x,

```
S3(x) = (1-beta)*P0(x) + beta*S2(x).
```

For a row's accumulated two-way likelihood ratio Z2 versus P0, the same
identity gives beta=7*Z2/(1+7*Z2). This weight remembers the row's entire
past without a revision transition. Neither old confidence in sources nor
old confidence in cold is discounted.

Let rho2/rho3 be the respective outer HMM source weights before the byte
(zero before admission). The two actual live laws are therefore

```
L2(x) = (1-rho2)*P0(x) + rho2*S2(x)
L3(x) = (1-rho3*beta)*P0(x) + rho3*beta*S2(x).
```

The relevant effective exposures are e2=rho2 and e3=rho3*beta. There is no
ordering between them: the better candidate changes its own outer posterior
history. Even with equal outer weights, decreasing exposure helps on bytes
where S2<P0 and loses upside on bytes where S2>P0.

This is an identity of the saved laws, not a newly tested alternative. A
read-only decomposition of all eight changed tails agrees with recorded
candidate/live prices within **1.116e-14 log2 bits**. For r=S2(truth)/P0(truth),
the paired live increment is exactly

```
log2(1 + e3*(r-1)) - log2(1 + e2*(r-1)).
```

## What the saved tails actually show

Each row below sums bytes 8192..16383 in its own recorded life. Positive
values favor the local-cold arm. Four exposure columns partition its paired
live difference by the sign of S2/P0 and the change in effective exposure.

| World | Candidate advantage | Live advantage | Less exposure, S2>P0 | More exposure, S2>P0 | Less exposure, S2<P0 | More exposure, S2<P0 |
| ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| 40 | 76.917515 | 1.821673 | -8.072248 | 10.988758 | 13.111812 | -14.206649 |
| 41 | 65.031743 | -3.846855 | -16.433728 | 0.010681 | 12.597221 | -0.021028 |
| 42 | 115.251377 | -2.325619 | -11.944394 | 2.290495 | 9.918918 | -2.590638 |
| 43 | 81.418104 | -4.748820 | -11.027388 | 2.318338 | 6.925601 | -2.965370 |
| 44 | 119.609043 | -0.240890 | -3.851952 | 13.446364 | 8.141372 | -17.976673 |
| 45 | 88.641342 | -1.384828 | -6.466479 | 0.167454 | 5.185707 | -0.271510 |
| 46 | 60.823656 | -1.790390 | -6.429434 | 6.696695 | 7.225253 | -9.282904 |
| 47 | 140.390278 | -0.743932 | -0.970486 | 16.344380 | 5.279626 | -21.397452 |
| Total | — | **-13.259661** | **-65.196110** | **52.263164** | **68.385510** | **-68.712225** |

Values within 1e-12 of zero for the candidate log ratio or exposure difference
were placed in a separate numerically-flat category; its summed live effect
is 2.3e-12 bits. It is not omitted from the reported live advantage.

The net loss on bytes where S2 was useful is **−12.932946 bits**. The net
difference on bytes where S2 was harmful is **−0.326715 bits**. Consequently
"the new arm stays exposed to harmful advice longer" is only part of the
explanation. Lost useful exposure matters greatly, particularly worlds
41/42/43/45. An old belief that a row deserves cold can also become obsolete.

Longer outer exposure is directly visible in world 47: before byte 8703,
rho2=0.00000173836 while rho3=0.49490048. The paired live advantage had been
+4.279537 bits by byte 8319; by byte 9215 it had become −0.737722. This is a
second concrete mechanism, alongside suppression of useful rows. The
candidate ledger continues to count price differences even when the outer
process has very little remaining influence; at the final byte both source
weights are minute in every world. Candidate totals do not carry the
corresponding time/exposure weights.

## Counterexample to "more cold is always safer": world 43, byte 8225

Saved row `010102`, observed byte 64 (`0x40`), repeat group 1. These are
pretruth log2 probabilities and pretruth state:

| Field | Value |
| --- | ---: |
| P0 | -7.851443274236022 |
| P2_A | -5.928651629498919 |
| P2_B | -5.007165596452516 |
| S2 | -5.794390939595784 |
| S3 | -6.913886500055384 |
| L2 | -5.821494016773718 |
| L3 | -6.919023001054136 |
| Previous A capital relative to P0 | -3.269112112639626 |
| Previous B capital relative to P0 | -6.298928384869127 |
| Local cold weight | 0.7104776923725784 |
| rho2 | 0.9755018873402791 |
| rho3 | 0.9925630002076865 |
| rho3*beta | 0.2873691302857264 |

Both books improve the current observed price over P0. Historical relative
capitals nevertheless give cold most of the row's weight. The received
prediction loses **1.097528984280418 bits** against the two-book arm on this
byte. This is not a lookup, causal-order or numerical bug; it is the stated
stationary Bayesian update doing what its retained history implies.

Raw references (under the Sol turn4 root named above), event `t=8225`:

```
results/c/world43/mosaic_then_unrelated-row.tsv.gz
SHA256 aa745d5e45b66b16b61ed169f5865686fed46abed67dac965ce205f2b5114401
results/events/world43/mosaic_then_unrelated.tsv.gz  [arm=row3]
SHA256 d9958df94b3271976580ac13634ab34fad57913bf0271fd0028e33e46106f1a7
```

## Bounded next hypothesis selected by the parent, not tested here

Revise row confidence in both directions by a fixed-share transition on the
three posterior masses. Keep both source books, P0, component formulas,
priors and the outer gate/HMM. After pricing and Bayes observation of each
complete-row event, apply

```
w_next[j] = (1-rho)*w_posterior[j] + rho*prior[j],  rho=2^-10.
```

This changes the row's confidence, not its archived history or local counts.
It leaves two independent relative log weights per row (3248 bytes). After
each transition, cold mass is at least 2^-13 and each source mass at least
7*2^-14, allowing recovery from old cold confidence as well as old source
confidence. NEW still has P0's exact price and zero likelihood evidence;
the declared row clock subsequently applies its prior-directed transition.

The unchanged outer bounds of 1 bit per full prefix and 16 bits per interval
remain valid for a normalized causal candidate with this adaptive state.
They do not imply improved changed-tail utility. The fresh comparison must
measure the received live tail, preserve useful-mosaic gain, and show the
effective exposure alongside candidate prices. This report neither evaluates
rho on worlds40..47 nor predicts that it will pass a new material gate.
