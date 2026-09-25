# Reproduce turn26

Requirements: Apple/Clang-compatible C11 compiler, Python 3.11+ standard
library, and BSD `/usr/bin/time` with `-l -o`.  No network or third-party
package is used.

From the repository root:

```sh
python3 -B turns/turn26/run.py
```

The script refuses changed protocol, candidate, reader, Python receipt, or
world identities.  It extracts the old mouth from parent commit `7138950`,
strictly builds old C, candidate C and the frozen independent reader, creates
all output directories under one temporary root, then performs:

1. old/new artifact comparisons on the full island, 89,509-byte island and
   the 1024-merge/order-3 guard;
2. candidate restart identity;
3. the independent C reader against candidate output and the committed
   Sitting-1 report;
4. one untimed warm-up and seven alternating old/new timing pairs on each of
   the full and smaller islands.

It writes `turns/turn26/RESULT.json`.  Wall seconds are measured by Python's
monotonic performance clock around the process; maximum resident bytes come
from `/usr/bin/time -l`.  Normal scheduler/load variation is expected, so the
frozen gate is relative medians, not equality with the retained decimals.

Supplementary checks:

```sh
python3 -B turns/turn26/stress.py
```

This strictly builds an ASan/UBSan candidate and runs the full primary
workload, then compares old/new artifacts in 48 settings over sixteen
deterministic text worlds.  These checks broaden implementation coverage; the
material performance gate remains the one in `PROTOCOL.md`.
