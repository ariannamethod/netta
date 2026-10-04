# Turn36 prefreeze construction

Strict flags: `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`.

The first build refused a macro redefinition and an unresolved relative
include while trying to rename turn34's entry point with the preprocessor.
Before any new data, the build was changed to the inherited mechanical staging
approach: a private copy of turn34 with exactly its single main declaration
renamed. Make checks that exactly two diff lines change. Frozen parent sources
remain untouched. The strict build then passed.

The 80-event fixture independently priced 3,680 values, max discrepancy
1.4210854715202004e-13; all four detail wealths entered positive and negative
regions. Source counts, record locality, NEW/no-match/equal events and exact
silence were checked. `FIXTURE.tsv` preserves every event.

A prefreeze integration run used only Don's already-open world304/recombined.
Its independent reader rebuilt 672 source counters and 98,304 forecasts,
maximum error 5.9117155615240335e-12. No fresh-world result was consulted.
See `PREFLIGHT.json`; its temporary raw traces remain in `.build/preflight304`.
