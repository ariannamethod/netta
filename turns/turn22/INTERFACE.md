# Turn22 writer, C replay and independent reader contract

This file describes serialization only. The scientific law and comparisons
are fixed in PROTOCOL.md. The independent reader does not import the writer
or new C implementation.

## C trace

Five modes, in order: `slow fast witness h8l4 selfnorm`.
Input, without header: `t cold candidate matched`, chronological from zero.
Output is TSV with41 columns:

1. `t cold candidate matched shadow_before active_before admitted_after`.
2. For each mode: `<mode>_live <mode>_odds_before <mode>_slow_used <mode>_odds_after`.
3. For witness, h8l4, selfnorm: `<mode>_w_before <mode>_w_after`.
4. `selfnorm_a_before selfnorm_a_after`.
5. `h8l4_cap_before h8l4_cap_after h8l4_clipped`.
6. `selfnorm_cap_before selfnorm_cap_after selfnorm_clipped`.

All quotes precede all observations. Old clock chooses hazard; updated
clock/statistics choose cap for the next quote. An increasing cap does not
increase stored odds. Clip means this state's hazard-updated odds exceeded
its new limit. Slow-used is -1 when inactive,0 fast,1 slow. Cap values may
be emitted before admission, but per-life cap ranges count active quotes.

## Result

Top-level fields: `namespace worlds regimes modes protocol_sha256 lives
quantities tables tests manipulation gate_pass independent_reader_pending`.
`pack_result(lives, quantities, tables, tests, manipulation)` is the actual
writer constructor. The pre-freeze schema fixture calls this constructor
with explicitly empty fixture inputs and checks its keys with the independent
reader. Those inputs are not an experimental result and never reach RESULT.

Each life has `world regime modes clocks support exactness column_new_exact
raw_help raw_harm episode_matched_events episode_max_matched_length
source_trace_sha256 authority_trace_sha256 max_norm_error`.

Per-mode statistics: `gain tail early minimum peak max_drawdown activation
horizons slow_count fast_count tail_slow_count tail_fast_count tail_fast_share
first_fast_t first_fast_after_move clip_count max_odds_before max_odds_after
odds_at_move cap_min cap_max cap_at_move cap_bound_exact`.
No cap: null range/at_move, zero clips, cap_bound_exact true. Max odds start
at0. Cap ranges use active prequote limits; at_move is index8192 even if inactive.

`clocks` contains witness/h8l4/selfnorm with `minimum minimum_t maximum
maximum_t at_move final`. These read prequote w; final reads the final update.
`support` has `maximum_a a_at_move final_a` for selfnorm.
`exactness` contains `equal_price_bytes new_bytes equal_price_exact new_exact
inactive_exact clock_identical support_recurrence support_domain cap_formula`.

Raw samples select selfnorm minus h8l4 on active changed-tail observations.
Update the retained maximum only when delta>stored_delta+1e-7, minimum only
when delta<stored_delta-1e-7; otherwise keep the earlier observation. Fields:
`world regime t truth rank matchedL cold candidate delta`, all five modes'
`live odds_before odds_after`, plus selfnorm `w_before w_after a_before a_after
cap_before cap_after clipped` and h8l4 `cap_before cap_after clipped`.

Quantities: `law_vs_fast law_vs_static law_whole_vs_fast law_whole_vs_static
moved_vs_fast moved_vs_slow early_retention full_retention recombined_full
law_robust_worlds static_law_vs_fast static_law_robust_worlds
static_law_whole_vs_fast`. Tables: regime -> mode -> `gain tail early` means.

Material test names follow the numbered protocol conditions:
`u01_law_tail`, `u02_law_robust`, `u03_law_whole`, `u04_moved_mean`,
`u05_moved_wins`, `u06_moved_slow`, `u07_early_retention`,
`u08_full_retention`, `u09_intact_positive`, `u10_law_vs_static`,
`u11_whole_vs_static`, `s12_admission`, `s13_unrelated`, `s14_archive`,
`s15_bounds`, `s16_normalized`, `s17_exact_quotes`, `s18_support_cap`.
