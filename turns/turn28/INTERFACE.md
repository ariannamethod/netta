# Turn28 writer / C / independent reader contract

Read PROTOCOL.md for laws, thresholds and evidence boundary. Paths below are
relative to this directory. Reader must not import experiment.py or new bank.c.

C CLI: `bank predict BANK POOLED_FULL POOLED_SMALL PERMUTED < RAW`.
`bank emit ALPHABET SEED` and `bank trace compact` delegate to inherited
episode CLI. `bank --fixture` emits handcrafted new-router observations and
states as JSON lines (test-only, no data/world access).

Source files: `data/world{w}/source{AB,BA,CD,DC}.tsv.gz` inherited compact trace
has t,pattern,k,heads,truth,rank,logcold_truth. Raw files same prefix with .bin.
Recipient files: recombined,partial,switched,moved_mid,unrelated.bin.
Memory per world: `bank.bin`, `pooled_full.bin`, `pooled_small.bin`,
`permuted.bin`, `BOOKS.json` with keys rules,selected,bank_selected,source_split.
Rules are the inherited objects with left,right; selected is full greedy list
(each rule_id,prefix_len,prefix,counts,total,etc); bank_selected is its initial
bank-capacity projection with extra `book_counts` as two lists of7 integers.
source_split is exactly `[["sourceAB","sourceBA"],["sourceCD","sourceDC"]]`.

Trace `results/world{w}/{regime}.tsv.gz` mandatory common columns:
t,k,heads,cold_heads,truth,rank,history,logcold,matched,record,full_matched,
full_record,bank_a,bank_b,perm_a,perm_b,max_norm_error,new_exact.
Lists heads,cold_heads comma-separated or '-' if empty; history is role digits
or '-'. record/full_record are index in serialized record order, -1 no match.
matched/full_matched are lengths, zero no match. bank_a etc are log2 prices
on the observed raw byte, computed from full pretruth vectors.

For local/global/permuted: `{arm}_w_before`, `{arm}_w_after` comma-separated
three probabilities, pertaining to the matched bank record or shared global
router. When no match, print prior for local/permuted, global's state for
global; no router changes. Equal likelihood can move weights by fixed share.

For EACH of local,global,pooled_full,pooled_small,permuted:
`{arm}_candidate`, `{arm}_live`, `{arm}_shadow_before`, `{arm}_odds_before`,
`{arm}_active_before`, `{arm}_activated_after`, `{arm}_gain_after`.
NEW checks apply to ALL256 pretruth forecasts, not just realized truth.

Manifests: FREEZE.json has namespace,worlds,regimes,arms,files(repo-relative
path->SHA256); DATA_MANIFEST.json,EXTRACT_MANIFEST.json,MEMORY_MANIFEST.json,
RESULTS_MANIFEST.json map turn-relative path->SHA256 (stage snapshots).
DATA pins raw/command/generator files; EXTRACT pins data folder after traces.

RESULT.json: namespace,worlds,regimes,arms,protocol_sha256,lives,tables,
conditions,quantities,material_pass,validity,independent_reader_pending.
Each life has world,regime,arms,max_norm_error,exactness,raw_help,raw_harm.
Each arms[name] has gain,early,tail,minimum,peak,drawdown,activation,horizons.
activation is first influencing byte t+1, null if no admission; horizon keys
1024,4096,8192,16384. tail=gain-at8192. Gains sum live-logcold chronologically.
tables[regime][arm] has mean gain,early,tail.
conditions keys T1,T2,T3,T4,T5; definition exactly PROTOCOL.
quantities: early_gain_per_byte,early_positive,early_vs_full,early_wins_full,
full_retention,full_positive,partial_tail_vs_full,partial_tail_wins_full,
partial_whole_vs_full,early_vs_global,early_wins_global,early_vs_permuted,
early_wins_permuted. All mean comparisons are local minus named comparator.
full_retention = mean local full / max(mean pooled_full full,1e-300);
T3 additionally requires pooled_full mean>0 to avoid vacuous retention.
validity keys archive,normalization,exact_quotes,bounds,source_counts.
Each exactness has new,equal,inactive,history booleans.
raw_help/harm: extremal partial-life-tail local-minus-pooled_full observations,
all trace fields plus raw_hex window; other regimes retain same extraction.

Independent verifier CLI `python3 -B verify.py --output NEW_PATH` (never
overwrite; root can be own directory). Numeric tolerance1e-7, exact discrete
fields; recompute whole trace, metrics and all quantities/conditions. Receipt
verification_pass,material_pass,forecasts_checked,max_error,source_counts_checked,
conditions,result_sha256. Failure names artifact and field. No modifications
to source/inputs/results.
