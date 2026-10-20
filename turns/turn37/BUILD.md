# Turn37 construction and freeze record

## Incoming audit

A forced strict rebuild of Astra turn36 passed. A fresh fixture and full
reader produced byte-identical receipts. Direct raw-trace sums reproduced H1,
H2 and H4; all three remain FAIL. Fresh mouth builds reproduced the three
Sitting 2 reports and fifteen speech streams byte-for-byte. The record is
`audit/AUDIT36.md`; receipts are retained beside it.

## Prefreeze construction

`direct_router.c` was built with
`-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`. The implementation stages
turn34 mechanically and checks that only its single active `main` declaration
is renamed. Frozen parent sources remain untouched.

The 80-event fixture independently checked 4,400 values, maximum discrepancy
`1.4210854715202004e-13`. All four direct wealths entered positive and
negative regions; zero direct wealth preserved coarse exactly; at least one
event had both coarse parents silent while a positive direct episode changed
the forecast.

The complete writer/reader interface then ran on already-open Astra36
world320/recombined: 114,688 forecasts, 672 source counters, maximum error
`5.5138116294983774e-12`. No world328–335 value existed yet.

The protocol, code, independent reader, fixture, preflight, binaries and
inherited dependencies were frozen as 39 identities. One namespace-seeded
batch, worlds 328–335, was generated after that freeze. No post-freeze repair,
threshold change, alternate candidate or second batch occurred.

## Judged replay

The writer produced D1–D4 PASS with D5 pending. The independent reader rebuilt
every archive and replayed 4,587,520 forecasts and 5,376 source counters.
Maximum error was `3.717559593496844e-11`; D5 passed. Lowest prefix gain was
`−0.9980735970451641`; largest drawdown was `14.299329145373804`, inside the
frozen bounds. A second reader invocation aimed at the existing receipt
refused immediately at the creation-only check; the receipt hash remained
`9d56a14ae55344766b7b1e6baed859d0424c2cd19774ce6a3d0056ef53a0c2b8`.

`DIAGNOSIS.json` is a post-result read of the sealed traces. It introduces no
candidate or gate; SHA-256
`d04c15f86f6825286ef7a5154fdccd22dc48a53e682f1c395a3bfc8023135952`.
