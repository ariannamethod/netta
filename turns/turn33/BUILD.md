# Turn33 C build and execution receipt

2026-10-03. This receipt describes the isolated turn33 implementation. It
does not authorize or record attachment to the mouth, Body 0, mycelium or a
canonical runtime.

## Strict build

`make all` completed with rc=0 under Apple clang 21.0.0 using:

```
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Werror \
  earned_router.c .build/byte_recurrence/frontend.c \
  .build/portable_recurrence/recurrence.c -lm -o earned_router
```

The resulting executable is an arm64 Mach-O file. `episode` is a symlink to
that executable. The private `.build` layout points only to inherited frozen
sources and preserves turn13's relative include paths; no older turn was
edited.

## Implemented chronology

`earned_router.c` retains the pooled 24-address archive and its inherited
per-address permission clock. For the same addresses it loads separate A/B
source counts and adds two recipient-local log2 wealth coordinates. Before a
truth is observed, only positive excess wealth can add a case residual to the
pooled forecast. The wealth coordinates update after truth; the permission
clock continues to update from the pooled shadow, not the corrected forecast.

The implementation checks A+B reconstruction of pooled counts, archive
addresses, forecast positivity and normalization before use. NEW, exact-common
and unmatched events preserve the inherited exact paths. All five arms consume
one causal byte/history tape and one pooled prospective-admission decision.

At 24 records the candidate's portable case bank is 912 bytes versus 528 for
the pooled archive: 384 additional portable bytes. Candidate recipient state
is 576 bytes versus 192 for the pooled incumbent: another 384 local bytes.
Only the portable delta was charged by the preregistered gate, at 0.01 bit per
byte or 3.84 bits per life.

## Predataset fixture and freeze

The handcrafted 24-event fixture used two record identities and no generated
worlds. An independent Decimal implementation checked 384 numeric values with
maximum error `3.552713678800501e-15`. It witnessed first visits, silent and
active residuals, positive and negative wealth, recovery, record locality,
NEW, no-match and exact-common events.

The protocol, interface, implementation, inherited sources, original reader
and fixture were then frozen in `FREEZE.json` before any material world was
generated. The freeze contains 24 pins. Its SHA-256 is
`5f2fc32bbbabed4c845a9cb687da154c65fc505bdd4a6b01c71d72236e770cf0`.

## One material batch

Exactly one batch, worlds 296--303, was executed through the creation-only
stages `generate`, `extract`, `learn` and `evaluate`. No candidate, threshold,
world or predictor was changed after generation.

| Stage identity | Files | SHA-256 |
|---|---:|---|
| `DATA_MANIFEST.json` | 280 | `495558b7e9f3bdd675ef12055193452c0053255282c9d587729cd15cce8b3d25` |
| `EXTRACT_MANIFEST.json` | 344 | `e7f4424cb16d6cda9b54397d491f700a862ee11728d4c85608741d6e70c0fe48` |
| `MEMORY_MANIFEST.json` | 32 | `6d8ef5a36d1f81fbe7e92e7cdc83bde5f85d3bdf28379623182f1f13cd6f099e` |
| `RESULTS_MANIFEST.json` | 80 | `3d871a20badee9b12eafbed31902404cf92410f48a6f74eace8284d7c88fd5a0` |

The ignored evidence directories remain local and content-addressed by these
manifests. They are evidence, not intended source-tree bulk.

## Reader boundary

The original frozen reader independently rebuilt and replayed the first life,
then refused on a discontinuous strict-zero category count. It remains
unchanged. A first separate repair later refused on the analogous exact-equal
category. `verify_repair.py` changes only those descriptive classifications,
after independently reconstructed numeric coordinates have passed the frozen
tolerance. `READER_REPAIR.md` and both refusal receipts retain the complete
chain.

The final reader checked 3,276,800 forecasts and 4,032 source counters with
maximum error `2.9814373192493804e-11`. E1--E4 and the full material gate pass.

## Core identities

| File | SHA-256 |
|---|---|
| `PROTOCOL.md` | `aacfe4680944a967a3a2b54b7e3a2e11702a97809337f5cef6ba653008579d68` |
| `earned_router.c` | `92b9a19ab588a75ede0d876a9debdcc943c5fa72c2b52c61e505e269849d2fc8` |
| `earned_router` | `7ff0656936c0ab83b887e07c8a0dcbfc6a31670f1c0ebe2c6d893317e4bb6159` |
| `experiment.py` | `626e64da81469c8fd85d1a79e22ab848993bf9ca1801df8b71420ab6fe75cb6b` |
| frozen `verify.py` | `ffe21d585736f91f7ff2ff37d74ef29ec1d8828db32557631055d0ef9d6c60e5` |
| final `verify_repair.py` | `63fa0c427a961e7e19d1499caa0ca4b38b49a182852aba8170cc64f11a1aedc8` |
| `FIXTURE_READER.json` | `c00d6a309dc128028a081ba78a6e670c1cc7877a3987284ad030dfcefc51534f` |
| `RESULT.json` | `898aa35a89628e685b757eaa06336d0c40c165bb7b714de35730d53c04181a2b` |
| `VERIFY.json` | `1601e9bac1d8a135bcdd15aa70f70215681a2ad2248e493fc9dbc35dbc21bf8d` |

Complete locally. No commit, push, merge or live integration was performed.
