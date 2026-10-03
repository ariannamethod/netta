# Turn31 C / writer / reader interface

Protocol is PROTOCOL.md. Build executable `case_router`, symlink `episode`
for inherited emit/trace commands. Command:

    case_router predict BANK POOLED_SMALL PERMUTED_BANK < RAW

Bank format is frozen NETEB001. Pooled format is NETEI001. Address projection
must be identical; pooled counts=A+B; permuted bank independently rotates
slots1..6 in each book, slot0 unchanged. All source build laws inherited28.
No separate full24 trace. `--fixture` uses only handcrafted prices.

Arms in order: factored, flat3, balanced, pooled, permuted, cold.
Trace `results/worldW/REGIME.turn31.tsv.gz`, tab separated with header:

    t k heads cold_heads truth rank history logcold matched record
    source_pooled source_a source_b source_permuted_a source_permuted_b
    max_norm_error new_exact admission_shadow_before admitted_before
    factored_u_before factored_v_before factored_u_after factored_v_after
    flat3_w_before flat3_w_after
    balanced_u_before balanced_u_after pooled_u_before pooled_u_after
    permuted_u_before permuted_v_before permuted_u_after permuted_v_after

Then, for EACH arm, seven fields:
`ARM_candidate ARM_live ARM_shadow_before ARM_odds_before ARM_active_before
ARM_activated_after ARM_gain_after`.
Weights triplets are comma joined. Every float prints %.17g. Common fields
have frozen turn30 semantics; times zero based, activation records t+1.
No record=-1, weights show initial states and do not update. Empty history '-'.
Router states refer to the current matched prefix, not a global state.
All quoted source values are log2 probabilities of the actual observed byte;
the full256 distributions must already be fixed before that byte is read.
`new_exact` reports validation over every protected NEW byte, not only truth.
`max_norm_error` covers cold, source, conditional mixtures, candidates and lives.

Fixture output JSONL: each row has `t`, `record`, `cold`, `a`, `b`, `pooled`
(truth log2 probabilities), `factored_before` [u,v], `factored_after` [u,v],
`flat_before` [w0,wA,wB], `flat_after`, `balanced_before` scalar,
`balanced_after`, `pooled_before` scalar, `pooled_after`, plus candidate
truth-price fields `factored`, `flat3`, `balanced`, `pooled2`.
Includes unmatched and all-equal events, changing favored case and a period
when both histories are poor. At least two record identities. Scalar fixture
source forecasts come from normalized handcrafted full-byte distributions.

Writer RESULT schema: namespace, worlds, regimes, arms, protocol_sha256,
lives (world/regime, arms statistics, max_norm_error, exactness,
raw_help/raw_harm), tables, quantities, conditions(F1..F4), validity,
material_pass, independent_reader_pending, gate_pass.
Statistics: gain, early, tail, minimum, peak, drawdown, activation, horizons.
Horizons1024,4096,8192,16384. Schema and gate restated by reader, not imported
from writer. Writer F4/gate_pass false until external reconstruction; reader
reports F4 true only when source, predictions, metrics and gate all reproduce.
Data/source/archive/trace manifests use relative file names -> SHA256.
Archive files and BOOKS.json are inherited turn28 outputs; no new metadata
inside them, to avoid a second source-memory transformation.

Only new mechanism is conditional fixed share separated from memory permission.
Balanced is a fixed equal-case comparison; its u update and prior are pooled's.
Quote S copies P0 exactly wherever both A and B exactly equal P0; binary quote
also copies P0 on equality. Both candidate and live protected quotes exact.
Full source/P0 and grammar selection are documented inherited boundaries.

## Result details for independent reader

`exactness` keys: new, equal, inactive, history, shared_admission,
router_weights. All boolean. History checks last32 prior ranks.
`raw_help`/`raw_harm` are the original trace row string-valued dict plus
numeric `difference`=factored_live-flat3_live and `raw_hex` from raw bytes
[max(0,t-16):t+17]. First strictly larger/smaller wins, so first row wins ties.

`tables[regime][arm][early|gain|tail]` is the arithmetic mean over eight worlds.
`quantities` keys:

- comparisons[regime][flat3|balanced|pooled][early|gain|tail]:
  {mean, wins, per_world}; all are factored-minus-named-arm; per_world is
  world288..295 order. wins counts strictly positive differences.
- retention: recombined_early, recombined_full, partial_full: candidate mean
  divided by max(pooled mean,1e-300). Positive pooled means separately required.
- permuted_excess[regime]: whole mean permuted minus factored.
- flat_minus_balanced[regime][early|gain|tail]: paired mean flat3-balanced.
- portable_bytes[arm], recipient_router_bytes[arm]: sorted unique byte counts
  across worlds; exact mapping/cost is in protocol (cold=0).
- shared_prefix_identity: all arms' cumulative8192 gains exactly identical
  to recombined for partial/switched/moved in each world.
- unrelated_admissions: entries {world,arm,activation,gain}, world-major then
  arm order, only where activation is not null. Includes cold's shared event;
  that event never grants cold any source influence.
- recombined_positive: number of strictly positive factored whole-life gains.

`validity` boolean keys: archive(all inherited archives<=528), source_counts,
normalization(max error<=1e-8), exact_quotes(all exactness), bounds(-1/16 with
1e-7 tolerance), identical_admissions, shared_prefix, unrelated_disclosed.
Writer source_counts is an assertion of its A+B construction; reader must
recount/rebuild rather than trust it. The writer's material_pass combines
F1..F3 and validity, while F4=false, independent_reader_pending=true and
gate_pass=false. Reader independently closes F4 and final gate.

F1 uses the comparisons.partial.flat3.tail and .balanced.tail mean>1,wins>=5.
F2 requires the three positive pooled means, min(retention)>=.95 and
recombined_positive==8. F3 requires every permuted_excess strictly<0.
