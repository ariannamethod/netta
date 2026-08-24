# NETTA Body 0 — Independent verdict

Written by the independent verifier (`netta_check.c`), built from
`PROTOCOL.md` alone. `netta.c` was never opened, read, or grepped
during this verification. Every number below comes from
`netta_check` tool output in this session; none is copied from the
builder's stderr or NETTALOG0.md.

## World hashes (refuse-if-mismatch, checked first)

```
shasum -a 256 netta.txt
  02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb  (matches PROTOCOL.md)
shasum -a 256 /Users/ataeff/arianna-datasets/miller/combined.clean.txt
  76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2  (matches PROTOCOL.md)
```

## Train segmentation reproduction (gate before trusting anything)

`netta_check` replayed `merges.tsv` on the train-byte slice of each
world (train = first floor(0.9·len) bytes) and compared the result to
`train_tokens.u32` byte-for-byte:

- World A: 101976 tokens, **byte-identical** to `body0/A/train_tokens.u32`.
- World B: 359234 tokens, **byte-identical** to `body0/B/train_tokens.u32`.

## Recomputed evidence vs builder's evidence

For every position of every arm (a, b, c, e, d) on both worlds,
`netta_check` independently rebuilt unigram/bigram/trigram counts
from train only, rebuilt the field H (window 8, symmetric,
max-normalized) from the train unit stream, rebuilt the Fisher–Yates
permutation (seed `0xC0FFEE`) for arm e, recomputed P(truth) under
the exact recursive backoff chain with the cold-boundary staging
(position1→P1, position2→P2, position3+→full law), checked
Σ_u P(u) = 1 at every position (word-arm OOV escapes excluded, since
that escape sits outside the P1/P2/P3 mixture by construction), and
compared its own P against the builder's printed P at 1e-9 relative
tolerance.

Result, both worlds, all five arms:

- **P-mismatch (>1e-9 relative): 0**
- **truth_id mismatch (byte/unit arms, exact id law): 0**
- **byte_offset/byte_len mismatch: 0**
- **Σ=1 sum-check failures (>1e-6): 0**
- **builder evidence file length vs my token count: 0 discrepancies**

One defect was found and fixed during construction of the verifier
itself, before any of the above numbers were accepted: the first
draft of the field builder double-counted the window contribution
when two tokens in the window shared the same id (`H[a*V+b] += val;
H[b*V+a] += val;` write the same cell twice when `a==b`). This
produced 10505/11362 P-mismatches on arm c and 6434/11362 on arm e,
World A. Fixed by adding the value once on the diagonal. After the
fix, all five arms on both worlds show zero mismatches. This is a
finding about the verifier's own draft, not about the builder.

## Held-out bits/byte (verifier's own recomputed P, `Σ −log2 P(truth) ÷ test_bytes`)

`body0/results_A.tsv`:

| arm | positions | test_bytes | bits/byte |
|---|---|---|---|
| a | 44755 | 44755 | 2.7317707717 |
| b | 11362 | 44755 | 2.3144808577 |
| c | 11362 | 44755 | 2.3132963630 |
| e | 11362 | 44755 | 2.3145216624 |
| d | 17015 | 44755 | 2.2189100235 |

`body0/results_B.tsv`:

| arm | positions | test_bytes | bits/byte |
|---|---|---|---|
| a | 130131 | 130131 | 2.9444842201 |
| b | 40424 | 130131 | 2.8383545886 |
| c | 40424 | 130131 | 2.8371485867 |
| e | 40424 | 130131 | 2.8383939199 |
| d | 55684 | 130131 | 2.4803454041 |

## Verdict against the frozen PASS/FAIL rules (PROTOCOL.md + Amendment 5)

**Field gate.** Rule: arm c must beat BOTH arm b and arm e on BOTH
worlds by at least 0.01 bits/byte on EACH world separately, or the
field is deleted regardless of sign.

- World A: c beats b by 0.0011844947 bits/byte; c beats e by
  0.0012252994 bits/byte.
- World B: c beats b by 0.0012060019 bits/byte; c beats e by
  0.0012453332 bits/byte.

Direction is correct (c is lower/better than both b and e on both
worlds) but the margin is roughly **8x too small** on every
comparison, on both worlds, against the frozen 0.01 bits/byte bar.

**VERDICT: FIELD GATE FAILS. The field does not earn PREDICTIVELY
ADMITTED status. Per Amendment 5, the field is deleted from Body 0
— direction of victory does not matter, only material margin does,
and the margin was not met on either world.**

**Unit vs byte baseline.** "Unit model beats byte-trigram baseline"
(architectural result, no causal claim to units): holds on both
worlds — b beats a by 0.4172899140 bits/byte on World A and
0.1061296315 bits/byte on World B.

**Word-level baseline.** Arm d beats arm c on both worlds — by
0.0943863395 bits/byte on World A and 0.3568031826 bits/byte on
World B. Per the frozen rule, this is stated plainly: **the
word-level baseline is better than the unit+field arm on both
worlds**, by a much larger margin than the field's own (failed)
victory over b/e.

No coherence claim is made anywhere in this document. All bits/byte
numbers measure predictive compression only.

## Machine gates (cmp/shasum, separate from netta_check.c)

- **A′ test-independence**: `body0/A/merges.tsv` and
  `body0/Aprime/merges.tsv` are byte-identical (cmp rc=0, same
  SHA-256 `34c0ecc2b83143e110014c0531b8c4aa68da3196df851f92b7d7c3a97371178a`).
  `body0/A/train_tokens.u32` and `body0/Aprime/train_tokens.u32` are
  byte-identical (cmp rc=0, same SHA-256
  `71ed3b32bc43f67e3a3eeea580ec2088bfbff1488822e070da8907f6ec9b8f10`).
  Machine proof the reversed test suffix never entered the model.
- **β=0 ablation identity**: within `body0/A_beta0/`, `evidence_c.tsv`
  is byte-identical to `evidence_b.tsv` (cmp rc=0), and all five
  `speech_c_<seed>.bin` are byte-identical to the matching
  `speech_b_<seed>.bin` (cmp rc=0, seeds 7/19/42/101/271). Machine
  proof `--beta 0` ablates exactly the field term.

## MANIFEST.tsv verification

Every listed file in `body0/A/MANIFEST.tsv`, `body0/B/MANIFEST.tsv`,
`body0/Aprime/MANIFEST.tsv`, `body0/A_beta0/MANIFEST.tsv` was
re-hashed with `shasum -a 256` and re-sized with `wc -c`; all match
the manifest exactly (the `MANIFEST.tsv` self-row, written before its
own hash could be known, is the standard `e3b0c4...` empty-string
placeholder and is excluded from the check by convention, not a
finding).

## Anti-copy (verifier-computed, longest train match + coverage ≥32B)

World A (`netta.txt`), all under the 50% coverage void-threshold:

| speech file | longest match (bytes) | coverage ≥32B |
|---|---|---|
| speech_b_7 | 57 | 0.427824 |
| speech_b_19 | 53 | 0.440042 |
| speech_b_42 | 41 | 0.403344 |
| speech_b_101 | 62 | 0.428870 |
| speech_b_271 | 49 | 0.334728 |
| speech_c_7 | 51 | 0.497388 |
| speech_c_19 | 46 | 0.435737 |
| speech_c_42 | 60 | 0.335917 |
| speech_c_101 | 49 | 0.392334 |
| speech_c_271 | 49 | 0.333682 |

World B (`combined.clean.txt`), all well under threshold:

| speech file | longest match (bytes) | coverage ≥32B |
|---|---|---|
| speech_b_7 | 35 | 0.186111 |
| speech_b_19 | 27 | 0.000000 |
| speech_b_42 | 34 | 0.037820 |
| speech_b_101 | 30 | 0.000000 |
| speech_b_271 | 29 | 0.000000 |
| speech_c_7 | 36 | 0.050847 |
| speech_c_19 | 42 | 0.053097 |
| speech_c_42 | 40 | 0.141558 |
| speech_c_101 | 38 | 0.092640 |
| speech_c_271 | 43 | 0.124493 |

No file crosses 50% coverage; no generative-void condition triggers.
No coherence claim follows from this — anti-copy only rules out
verbatim reproduction of train.

## Summary

Reproduction: clean (0 mismatches, 0 sum-check failures, both
machine gates hold, both worlds, all artifacts hash-verified).

Field: **FAILS its frozen gate on both worlds** — right direction,
margin roughly 8x under the required 0.01 bits/byte bar on every one
of the four b/e comparisons. Field is deleted from Body 0.

Unit-vs-byte: unit model beats byte-trigram baseline on both worlds
(architectural result, no causal claim).

Word-level: beats the unit+field arm by a wide, gate-clearing margin
on both worlds — stated plainly per protocol.
