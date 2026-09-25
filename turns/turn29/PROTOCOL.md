# Turn29: local permission without duplicated source records

Sol, 2026-09-25. Written before implementation and before worlds 272--279.
Base: Astra28 `dd6aeec368dd47bc6444a87508be7111e44a0848`.

One bounded change answers the separation exposed by turn28. Keep the full
24-record pooled episode archive and its 528-byte portable source budget.
Give each stored prefix a recipient-local choice between current P0 and that
one pooled continuation. Do not store a second source book. This tests local
permission without paying for duplicated record contents.

## Incoming evidence and audit boundary

Turn28 stored two source histories at each of 12 prefixes. Its local selector
beat the shared selector and permuted control early, but halving address
coverage lost to the old 24-record archive. Its result is material FAIL
T2/T3/T4. Before this turn, Sol independently reruns Astra's frozen reader;
the exact receipt and code review are recorded under `audit/`.

The new construction does not repair turn28, tune its result, or reuse worlds
264--271. Turn28's bank becomes a frozen comparison arm on fresh targets.

## Source memory and local router

Use the same four 16384-byte source lives, causal HEAD256 frontend, turn13 BPE
grammar, source-only joint greedy insertion order and 528-byte NETEI001 pooled
archive. No target byte or generator label selects a rule or record.

For each of the full archive's records, recipient state is one scalar `u`, the
weight of its pooled source forecast S. On that record's matched visits:

```
Q = (1-u) * P0 + u * S
u_initial = 7/8
u_posterior = u*S(truth) / Q(truth)
u_next = (1-rho)*u_posterior + rho*(7/8)
rho = 2^-10
```

The P0 posterior is the complementary mass. The update happens only after the
current forecast, on every matched visit including NEW or equal-price visits,
and before external admission. No-match visits do not tick a router. Recipient
weights reset between lives and never enter the portable archive. Source counts
never change.

Exact equality is law: if S equals P0, Q is copied from P0 byte-for-byte.
NEW and all bytes outside current repeat heads remain exactly P0.

## Arms

All arms see identical source exposure, target bytes and P0. Each retains its
own unchanged prospective 32-bit admission and fixed slow outer HMM with
hazard 2^-16.

- `local_full`: all 24 pooled records, one binary P0/S router per record.
- `global_full`: same archive and binary law, one router shared by all records.
- `pooled_full`: unchanged full pooled archive with no inner router.
- `bank_local`: frozen turn28 12-record two-book local P0/A/B mechanism.
- `permuted_full`: same 24 addresses and bytes as full, but repeat-role counts
  1..6 rotated left while NEW stays fixed; one binary local router per record.

The full and permuted portable archives each remain <=528 bytes. `bank_local`
also remains <=528 but retains only the first bank-capacity addresses. Runtime
recipient state, outer state and trace vectors are reported separately and do
not masquerade as portable source bytes.

## One fresh batch

Worlds 272--279, namespace `netta-local-pooled-permission-v1`. Four 16KiB
sources and five 16KiB recipients per world: `recombined`, `partial`,
`switched`, `moved_mid`, `unrelated`. Generation is exactly the turn28 family
with new namespace/world identities. The partial manipulation replaces the
commands of recombined components 0/1 after byte 8192 and preserves the raw
prefix exactly. Hidden components and regime labels are never predictor input.

Save raw bytes, source traces, all archives and full prediction traces with
manifests. Freeze protocol, writer, both C predictors, independent reader and
all inherited dependencies before generating the batch. No second prior,
share, threshold, record order or regenerated target may be tried on these
worlds.

## Gate fixed before data

One material verdict requires every condition:

T1: `local_full` mean early-4096 gain over P0 >= .005 bit/raw-byte and is
    positive in at least 6/8 worlds.

T2: `local_full` mean early-4096 gain exceeds `pooled_full` by >1 bit and wins
    in at least 5/8 worlds. This asks whether local permission itself helps.

T3: on intact full lives, `local_full` retains >=95% of positive
    `pooled_full` mean, is positive in 8/8, AND exceeds `bank_local` by >1 bit
    in mean with at least 5/8 wins. This asks whether restoring addresses
    recovers the coverage turn28 paid away.

T4: on the partial final 8192 bytes, `local_full` exceeds `pooled_full` by
    >1 bit in mean and wins at least 5/8; whole partial mean is not lower.

T5: early `local_full` exceeds both `global_full` and `permuted_full` by >1
    bit in mean and wins at least 5/8 against each. This tests local scope and
    source correspondence without changing address capacity.

Validity: every source archive <=528 bytes; exact source recount and
permutation; pretruth chronology; positive normalized 256-byte distributions;
protected NEW and equal forecasts bitwise P0; independent outer states;
full-prefix loss >=-1 and drawdown <=16 with 1e-7 tolerance; the two C traces
agree exactly on truth, P0, bindings and pretruth role history; independent
forecasts, routers, authority states, metrics and gates within 1e-7.

Report all worlds, all horizons, admissions, minima, drawdowns, unrelated
admissions, complete changed tails, partial losing tails and direct
`local_full-bank_local` comparisons. Preserve actual help and harm rows.
`unrelated` retains the shared emitter/role machinery and is not called IID.

The independent reader may accept inherited grammar selection and HEAD256
P0/bindings as hash-pinned inputs. It must independently recount source
continuations, rebuild the pooled and permuted archives, replay both router
laws and all outer laws, and reconstruct the verdict.

PASS ends the step. FAIL also ends it and preserves every counterexample. No
live integration, merge, commit or push follows without Oleg's explicit word.
Return audit, code, raw behavior and one next question to Don.
