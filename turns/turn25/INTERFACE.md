# Turn25 writer / reader interface

The reader may read this file and PROTOCOL.md, not the writer or new C law.

Constants: namespace `netta-witness-latch-v1`, worlds 248..255, regimes
`recombined switched moved_mid unrelated`, modes `slow fast witness latch`.
N=16384; seam=8192; early=4096; numerical tolerance=1e-7.

Files relative to this folder:

- `FREEZE.json`: namespace, worlds, regimes, modes, files mapping repository
  relative paths to SHA256. Reader must check those files.
- `DATA_MANIFEST.json`, `MEMORY_MANIFEST.json`, `RESULTS_MANIFEST.json`: mappings
  from turn25 relative artifact paths to SHA256.
- `memory/world{w}/episodes.bin`: inherited archive <=528 bytes.
- `results/world{w}/{regime}.tsv.gz`: inherited episode raw trace, containing
  t, truth, rank, logcold, episode_candidate, episode_matchedL,
  episode_active_before, episode_activated_after, episode_shadow_before,
  episode_live, max_norm_error, new_exact. Other inherited columns retained.
- `results/authority/world{w}/{regime}.tsv.gz`: new C trace. Common columns:
  t,cold,candidate,matched,shadow_before,active_before,admitted_after,
  w_before,w_after,latch_before,latch_after. For each of the four modes:
  {mode}_live,{mode}_odds_before,{mode}_slow_used,{mode}_odds_after.
  `slow_used` is -1 before admission, 1 slow, 0 fast. `w` is shared by witness
  and latch. latch_before/after refer to the candidate's flag only.

Result top-level keys: namespace,worlds,regimes,modes,protocol_sha256,lives,
bars,quantities,tables,validity,material_pass,independent_reader_pending.
Reader owns `check_schema(result)` (raise on missing fields) and CLI
`--output NEW_PATH` (default VERIFY.json, fail if destination exists).

Each life: world,regime,modes,diagnostics,exactness,max_norm_error,
archive_bytes,raw_help,raw_harm. Each mode's metrics: gain,tail,early,minimum,
peak,drawdown,activation,horizons,slow_count,fast_count,tail_fast_count.
activation is t+1 on first admission, null if absent. Horizons keys as strings
1024,4096,8192,16384. Counts count active hazard updates.

diagnostics: first_set,first_return,set_count,return_count,sets_before_seam,
returns_before_seam,first_set_after_seam,latch_at_seam. Positions use t of the
observation that UPDATED the flag (not first later fast update). Unseen first
positions null. latch_at_seam is flag BEFORE observation8192.

exactness boolean keys: admission,equal_price,inactive,new,clock,latch_law.
All must be true for validity. Reader derives them from raw inputs and state.

For each mode, bars has the eight names from Don24:
b1_law_tail_mean,b2_law_whole_mean,b3_moved_tail_mean,b4_moved_tail_wins,
b5_moved_tail_vs_slow,b6_intact_early_retention,b7_intact_full_retention,
b8_intact_full_positive.

For each mode, quantities: law_tail_vs_fast (8 numbers),law_tail_mean,
law_whole_vs_fast,law_whole_mean,moved_tail_vs_fast,moved_tail_mean,
moved_tail_wins,moved_tail_vs_slow,moved_tail_vs_slow_mean,
intact_early_retention,intact_full_retention,intact_full_gains.
tables[regime][mode] contains means gain,tail,early.

validity booleans: admission,archive,bounds,normalization,exact_quotes,latch.
material_pass is all eight candidate latch bars, independent_reader_pending
is true in writer output. Reader emits separate verification and material
verdicts. The reader checks all metrics/quantities/bars/validity plus every
authority quote and state; float comparisons allow1e-7, discrete exact.

raw_help/raw_harm are descriptive (not gate): greatest/least one-byte latch
minus witness log-price difference on the second half. They include t,truth,
rank,matched,cold,candidate,w_before,w_after,latch_before,latch_after and all
four live quotes / odds_before. Reader need not reconstruct those selections.
