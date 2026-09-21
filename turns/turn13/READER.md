# Independent reader: joint source allocation

2026-09-19, measurement agent. Written before worlds128–135 are generated.

`verify.py` derives from the sealed turn12 reader; turn12 remains unchanged.
The new experiment writer was neither read nor imported. Only standard-library
imports are used. This hand owns this note and `verify.py`.

- Protocol SHA256: `ec37980efb94865e6699cb8abe954fddf779924fc3fa5c2fa5f15d659a9594d4`.
- Reader SHA256: `293d897e9f70a817015a2d72834f025713f85ad864668c293417a33fff8f951f`.
- Predata CLI check: `PYTHONDONTWRITEBYTECODE=1 python3 turn13/verify.py --help`, exit0.
- No new source/target data generated or evaluated by this agent.

## New independent check

Source BPE and continuation counts retain the independent rolling-base8
implementation. For joint selection the reader separately enumerates actual
successor positions for each candidate length and source life. It validates
those positions against all-occurrence counts, then stores the winning record
index and length for every source event. Each candidate's affected positions
are grouped by incumbent index and observed role, sorted in the protocol's
order, and scored with binary64 division/log2 and `math.fsum`, minus128bits.
Every chosen record, positive-gain stopping decision, tie and residual update
is independently reconstructed. The original complete counts are serialized.

Both forward and reversed joint sequences must agree with `BOOKS.json`,
including selected contents/order, marginal gains, affected-event counts and
cumulative unpenalized gain (1e-7bit tolerance). A final aggregate independently
checks that the sum of marginal improvements equals the final empirical
longest-match source objective. This objective includes protected NEW and is
explicitly different from the recipient's KT/P0 prices.

All seven archives are reconstructed exactly, including `isolated.bin`.
The compact formats, exact uint16 counts, common528-byte cap,7032-byte row
reference and expanded literal-copy identity are unchanged. Target replay
checks every stored match, vote, role-level candidate, received price and
prospective admission/HMM update:2,752,512forecasts over all24lives.
Normalization, protected NEW,1bit prefix and16bit drawdown limits remain.

The same17 material gates are recomputed. The episode-minus-isolated comparison
is also retained for each regime and world: candidate gain, received full gain,
tail and1024/4096/8192/16384horizons. It is descriptive; no new threshold is
introduced. Utility and mechanism verification remain separate outcomes.

## Limits

HEAD256 bindings and sparse P0 are supplied trace inputs. The reader does not
rebuild the inherited frontend or all256 cold masses; full-vector receipts
come from the C quote path. The source-selection and new-organ arithmetic are
independent. The greedy criterion need not maximize received benefit, and its
source objective can reward NEW information that the recipient cannot use.
Exact relation strings in this fixed synthetic family do not establish the
broader partial-functional-similarity or51-city claim.
