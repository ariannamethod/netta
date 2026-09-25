# Turn28 independent reader

2026-09-25, written before fresh worlds. `verify.py` was implemented from
PROTOCOL.md and INTERFACE.md without reading/importing the turn28 writer or
new C implementation. The earlier turn13 reader was consulted for the
unchanged archive convention, role history and fixed slow outer law.

## Reconstructed independently

- Four source tapes per world, each exactly 16,384 bytes/events: raw truth,
  supplied head bindings, role index and source-life boundaries agree.
- Rule DAG expansion and unique selected-prefix addressing. The supplied
  inherited greedy insertion list is taken as the selection boundary.
- Immediate continuation counts for every selected prefix, independently
  recounted with rolling 3-bit suffix integers. A is AB+BA; B is CD+DC;
  pooling, projection and repeat-rank rotation are exact.
- All four serialized archive byte strings, including reserved zero,
  uint16 counts, record order and the 528-byte limit. Runtime router payload
  is reported separately from portable bytes.
- All recipient pretruth role histories, longest suffix matches, component
  distributions and local/global/permuted three-expert states. Posterior and
  fixed share operate in Decimal probability space. A matched NEW or equal
  price observation still clocks share; an unmatched event does not.
- Five separate prospective admissions and fixed slow HMM trajectories,
  with Decimal source/cold masses, exact cold-before-admission and bounded
  prefix loss/drawdown. All 3,276,800 predictions on the 8×5×16,384 batch
  must be present, in order, with no surplus rows.
- Every life statistic, declared table/quantity/T1…T5, unrelated cases and
  raw help/harm extraction. Extreme witnesses use the first strict extremum
  after t=8192 and raw window `[max(0,t-16):t+17]`.
- Frozen code and all four stage manifests. The reader writes only a new
  receipt with `open('x')`; it never overwrites inputs or a previous receipt.

## Supplied boundary

This reader does not repeat source grammar mining or joint greedy selection.
HEAD256 bindings and cold probabilities are hash-pinned inputs. It checks
their consistency with raw observed truth and reconstructs the entire changed
probability support, but does not rebuild the learned-unit frontend. The C
all-256 normalization/NEW fields are retained implementation evidence and
checked in addition to independent support normalization. Generator code and
data are pinned; recipient raw-prefix identity is checked. This is not an
independent regeneration of the corpus or hidden partial-component labels.

The one declared C fixture checks the new arithmetic before freeze:
8 observations, 136 numerical comparisons, maximum error
`1.7763568394002505e-15`. It includes learned local/global states, matched NEW,
equal forecasts, and no match after a changed global state.
`FIXTURE_READER.json` records the exact tested reader hash. Subsequently the
source-split metadata assertion was aligned to the declared list-of-two-lists
format; no arithmetic changed. No fresh recipient worlds were executed by
this hand before freeze.

## Invocation

```sh
python3 -B turns/turn28/verify.py --output /absolute/new/VERIFY.json
```

A material FAIL is a valid verified outcome. Any numerical, archive,
chronology or schema disagreement stops the reader with an artifact/field
name and leaves the requested receipt absent. No threshold is adjusted.
