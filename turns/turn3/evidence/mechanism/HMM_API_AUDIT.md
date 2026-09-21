# Independent HMM implementation audit

2026-09-16, Astra mechanism reader. Scope: Sol source commit
`98d723b605bcc167bfe0b69abc4242dbeeb873fa` in
`/Users/ataeff/arianna-codex/repos/netta-byte-hmm-20260916`, read only.
Compared the recurrence C/header delta against the preceding Astra byte copy.
Read the current handoff, HMM protocol, report, driver, reader and API tests.
No historical Astra blind audit or Court4 reviewer traces were opened.

## Result

No demonstrated implementation defect in the HMM delta. Full saved-series
reproduction belongs to the parent reader; this report does not self-certify
those empirical claims.

References below are relative to the read-only Sol repository named above.

- `portable_recurrence/recurrence.h:15`: exactly representable hazard `2^-16`.
  `recurrence.c:146` initializes a fresh local life; HMM initialization only
  enables its mode. The carried archive still contains counts alone.
- `recurrence.c:163`: P0/P2 construction is unchanged. `:172` records the mode
  in a quote and `:233` rejects owner/event/mode/posterior mismatch before any
  mutation. Earlier checked-support and owner-binding repairs remain intact.
- `recurrence.c:248` prices the byte using the previous posterior; `:267`
  applies the transition after its evidence. That branch is outside row,
  support, repeat-class and truth-group conditions. Therefore every active
  observation consumes one hazard step, including zero-evidence observations.
- `recurrence.c:274` leaves the gate crossing byte cold and starts odds 1 on
  the following event, with no hazard step at admission.
- `byte_recurrence/live.c:153` completes all distributions before the input
  read at `:158`; recurrence observation at `:162` precedes frontend update
  at `:169`. The HMM flag does not change frontend, archive or source scores.

## Formula and bounds

Write `o` for source/cold odds, `r=P2/P0` for the just-observed likelihood
ratio and `h=2^-16`. The transition in `recurrence.c:268` is

```
z = o*r
o_next = (1-h)*z / (1+h*z)
log2(o_next) = log2(1-h) - logadd2(-log2(z), log2(h))
```

This is Bayes observation followed by source→cold transition. Its logarithmic
implementation avoids exponentiating the odds; `log1p(-h)` computes the small
survival cost accurately. Next predictive cold mass is at least `h`.

The bounds apply to the shared causal P0/P2 processes even though their local
counts adapt to observed history:

1. Admission starts cold mass 1/2, and cold is absorbing. Its always-cold path
   contributes at least `(1/2)*product(P0)`, so every full-prefix gain is ≥−1.
2. At the start of any active interval cold mass is ≥h (1/2 immediately after
   admission). Its always-cold path gives interval gain ≥log2(h)=−16. Thus
   maximum cumulative-gain drawdown is ≤16.
3. For T≥1 active observations, the surviving all-source path contributes
   `(1/2)*(1-h)^(T-1)*product(P2)`. Together with the always-cold path, HMM
   likelihood is at least `(1-h)^(T-1)` times static-mixture likelihood.
   Additional loss is ≤`(T-1)*[-log2(1-h)]`. With no active events the loss
   difference is zero. The final after-observation transition does not enter
   that prefix's likelihood, which explains T−1.

Narrow reader coverage note: `byte_recurrence/HMM_PROTOCOL.md:35` describes
prefix checks; `hmm16.py:233` checks extra loss against static only at the final
endpoint. The first two bounds are checked inside its loop. The third bound
is valid for every prefix by the argument above; the parent's independent
reader can inspect it inside its event loop. This is a coverage observation,
not an observed probability or bound violation.

## One disjoint API probe

`hmm_null_evidence.c` checks admission timing and four zero-evidence cases:
incomplete context, one repeat class, unsupported row, and selected NEW.
The fixture uses gate 0 solely to expose transition timing; measured gate 32
and all production files are unchanged. Each case has P2=P0; independently
evolving the source probability as `p_next=(1-h)*p` agrees with C odds.

Raw output: `hmm_null_evidence.txt`. PASS, four active observations,
maximum log2-odds error **6.4293188828390413e-17**. Strict compilation succeeded.

Compiler: Apple clang 21.0.0 (clang-2100.0.123.102), arm64-apple-darwin25.4.0.
Run from this evidence directory:

```sh
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -I /Users/ataeff/arianna-codex/repos/netta-byte-hmm-20260916/portable_recurrence hmm_null_evidence.c /Users/ataeff/arianna-codex/repos/netta-byte-hmm-20260916/portable_recurrence/recurrence.c -lm -o hmm_null_evidence
./hmm_null_evidence > hmm_null_evidence.txt
```

Audited file SHA-256 values:

```
1c14a567b30d376d515c22f6280872e98811956a691958057cfc7ef4b48739d3 portable_recurrence/recurrence.c
282f79c3a90680a2bf6fa0b86624995d83a292c4edf5d33393c95d3d1b56b958 portable_recurrence/recurrence.h
83969c627d6e3a64b8bb4aa33bb49ecec413e786cbcafa3a1c4ceff855dd7b0b byte_recurrence/live.c
```

No production edits, parameter changes, new target generation or commits.
