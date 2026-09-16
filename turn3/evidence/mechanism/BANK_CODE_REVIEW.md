# BANK2: code, wording and cost review

2026-09-16. Bounded reread of the final C implementation by its implementation
agent. This is not a second independent reproduction of the fresh results;
the parent and independent measurement reader own that comparison. No new
experiment, fixture or production edit was performed for this review.

Repository: `/Users/ataeff/arianna-codex/repos/netta-astra-turn3-20260916`.
`turn3/bank.c` SHA-256:
`2e1cd35f7e35e42149bf024a6dccf4c6757c3d2233271aff64e283cc367fb93e`.
Line references below are rooted at this repository.

## Findings

No concrete implementation defect found in the final dynamic-allocation
version. The following wording accurately describes its mechanism:

> The recipient learns which of two separately preserved histories predicts
> better on each exact HEAD256 coordinate, allowing different histories to
> contribute on different rows of a partly familiar world.

The exact coordinate remains the canonical equality pattern supplied by the
unchanged frontend (`bank.c:110`). Both P2 components use the same recipient
counts and local byte law (`:135`). The row index selects a locally learned
log-odds (`:151`); the bank mixes the two predictions (`:156`) and updates
only that row after pricing (`:242`). In global mode all rows use index 0.
No distance function, inferred correspondence between different rows, or
semantic matching was added. Case boundaries are supplied by the collector.
The generator's raw-pattern routing mask is never an input to this C learner;
it also need not coincide with every learned-unit HEAD256 coordinate.

Thus "partly applicable memories on exact HEAD256 coordinates" is defensible.
Claims of general functional similarity, discovery of case boundaries, or
transfer beyond these coordinates would require additional evidence.

## Memory accounting

| Quantity | Bytes in the current build | Meaning |
| --- | ---: | --- |
| Row router | 203 × 8 = **1624** | New recipient-local numeric state |
| Global router | **8** | One shared recipient-local log-odds |
| Pool router | **0** | No inner selector |
| Serialized two-case bank | **14080** | 16-byte bank header + two complete 7032-byte HEAD256 books |
| Two resident count payloads | **14032** | Two × 877 × 8, excluding headers and C object overhead |
| Additional pooled payload, pool mode only | **7016** | Exact integer sum retained alongside A/B for comparable component logging |

`bank.c:214` allocates exactly 203/1/0 doubles by mode. The allocation is
zero-initialized, checked, and released at `:267`. These weights are learned
inside the recipient and are not serialized or carried into its next life.
Calling the *whole memory* "1624 bytes" would be incorrect: that number is
only the additional row-router state, excluding immutable books, existing
local learner/HMM, allocator overhead, frontend and temporary quote vectors.
The file size is checked by `:15` and its structure by `:43`.

## Cost

Let J be the number of books and k the number of repeat classes (k≤6).
The group-level mechanism requires **O(J·(k+1))** work to quote/mix their
conditional group factors, and O(J) to update the observed row. For J=2
the update is one difference of log probabilities. Generalizing a row
selector to J cases would need 203·(J−1) independent log-odds; this code is
specifically the fixed two-book construction.

This is not the runtime complexity of the entire executable. The existing
`portable_recurrence/recurrence.c:39` finds a row by a linear search through
203 fixed six-digit names for each component quote. The diagnostic frontend
and driver also construct full 256-byte vectors: `bank.c:125` maps every
byte through up to k heads, and `:165`/`:185` price and validate the vectors.
Excluding frontend model work, a parameterized accounting is
O(J·R·depth + J·k + 256·k + J·256), with R=203, depth=6, J=2 fixed here.
The unchanged frontend's segmentation, projection and periodic model rebuild
are additional costs. Pooling costs O(J·877) once at load time.

## Boundary between inner choice and outer rejection

The inner selector has exactly two source-conditioned components. There is
no third, independently selectable cold expert. An unavailable component
already equals P0 through the unchanged `pr_quote` support rule, so it can
provide a cold fallback in that special case. The precise limitation is:

> When both source components are available and both harm a particular row,
> the inner selector cannot choose a separate P0 option for that row.

There is one outer shadow gate and one HMM for the complete bank, shared by
all rows (`bank.c:212`, `:235`). Local benefit and harm jointly affect that
global authority; there is no independently admitted/rejected authority for
each row. The inner weights continue learning before and after outer
admission. NEW contributes exactly zero inner evidence (`:243`), with its
bank probability copied from P0 (`:162`). Incomplete contexts return Pbase
and have no inner update.

The absorbing-cold HMM bounds still apply because the inner bank produces a
normalized causal candidate before truth. They bound live loss against P0;
they do not establish a per-row cold-rejection guarantee. Likewise the inner
Bayesian mixture's ≤1-bit regret to the best fixed donor on each visited row
is about the inner candidate process, not a bound on post-gate live gain.

## Final allocation and order checks

- Row accesses use a valid canonical row; global accesses only index 0.
  Pool sets `inner_index=-1` and never dereferences its null inner pointer.
- The pooled archive is allocated only in pool mode (`:63`). Cell addition
  overflow is rejected with the allocation freed (`:69`); normal exit frees
  it at `:268`. Existing per-row support/precision checks remain in pr_quote.
- Every valid archive fills both count payloads before prediction. Wrong
  outer/inner magic, size, book count or reserved field is rejected at load.
- Bank quotes preserve the common PRLife owner/event/HMM identity by copying
  a component quote. Only candidate group scales and diagnostic selection
  are changed (`:148`). All six probability vectors are completed and
  validated before `fgetc` (`:228` versus `:231`). Pricing uses that quote;
  inner and frontend updates follow the observed price (`:235`–`:248`).

No repair is indicated by this code review. Empirical PASS and its scope
remain conditional on the independent reader of the fixed fresh batch.
