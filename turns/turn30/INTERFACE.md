# Turn30 writer / C / reader interface

The material contract is `PROTOCOL.md`. New prediction command:

```
router3 predict BANK POOLED_SMALL PERMUTED_SMALL < RAW
```

`router3 emit` and `router3 trace compact` delegate to frozen turn13 code. The
frozen turn29 binary supplies the `full24` yardstick unchanged:

```
turn29-calibrate predict POOLED_FULL PERMUTED_FULL < RAW
```

Per world memory contains `bank.bin`, `pooled_full.bin`, `pooled_small.bin`,
`permuted.bin` (turn28's four, built by turn28's frozen builder), plus
`permuted_small.bin`, `permuted_full.bin` and `BOOKS.json`. The two new
archives are NETEI001 with every pooled record's repeat counts rotated 1->6
while NEW count 0 is unchanged. `permuted.bin` is turn28's two-book null; this
turn does not read it, and it is kept because the inherited builder emits it.

`bank.bin`, `pooled_small.bin` and `permuted_small.bin` address the identical
12 records: same rules, same rule_id/prefix_len per record, pooled counts
exactly A+B, null counts exactly the rotation of the pooled counts. `router3`
refuses to run if any of that fails.

## Arms

- `bank3` — three-way local router over {P0, stored A, stored B} per record;
  weights start at (0.125, 0.4375, 0.4375), which is the turn29 prior mass
  0.875 split equally between the two histories; posterior with leak share
  2^-10 back to that prior vector, renormalized after every observe; quote is
  the log-domain three-way mixture, copied from P0 byte-for-byte wherever both
  stored sources equal P0.
- `pooled2` — turn29's binary router verbatim on the same 12 addresses with
  A+B pooled counts.
- `permuted2` — the same binary router on the rotated pooled counts; the null.
- `cold` — P0 itself, the zero of every gain.
- `full24` — turn29's `local_full` arm replayed by its own frozen binary on
  the 24-record pooled archive. Reported, not gated.

## One price tape, one shared admission

Both binaries consume the same raw life and the same frozen frontend, so every
common trace field agrees byte-for-byte; the writer and the reader both check
that. Within `router3` the four gated arms share one admission: the shadow of
the **incumbent** `pooled2` candidate stream is the only admission clock, and
when it first reaches 32 bits all four arms activate on that same step. The
candidate arm cannot open its own door — any `bank3` advantage is priced after
an admission it did not cause. `full24` keeps turn29's own per-arm admission,
which is why it is reported and not gated; its activation is disclosed.

## Trace

`results/world{w}/{regime}.turn30.tsv.gz` carries
`t,k,heads,cold_heads,truth,rank,history,logcold,matched,record,source_pooled,
source_a,source_b,source_permuted,max_norm_error,new_exact,
admission_shadow_before,admitted_before`; then `bank3_w_before/after` as a
comma-joined triple and the scalar `pooled2_*`/`permuted2_*` weights; then
seven outer fields for each of `bank3`, `pooled2`, `permuted2`, `cold`.

`results/world{w}/{regime}.full24.tsv.gz` is the unmodified turn29 interface;
only its `local_full` arm is exposed here, as `full24`.

Every candidate and live vector is fixed before stdin yields truth. Routers
update after truth only, on matched visits only, including NEW and equal-price
visits. A missing record prints prior state and does not update. Weights reset
between lives and never enter the portable archive; source counts never change.

## Verdict fields

`RESULT.json` carries `conditions` D1..D6 with `D6` false and
`independent_reader_pending` true: the writer cannot certify the independent
reader, so its `gate_pass` is false by construction and `material_pass` is
D1 AND D3 AND D4 AND D5. The final `gate_pass` — D1 AND D3 AND D4 AND D5 AND
D6 — is the reader's, in `VERIFY.json`. The reader refuses any RESULT.json
that claims D6 or gate_pass for itself.
