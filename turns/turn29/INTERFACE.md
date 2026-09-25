# Turn29 writer / C / reader interface

The material contract is `PROTOCOL.md`. New prediction command:

```
calibrate predict POOLED_FULL PERMUTED_FULL < RAW
```

`calibrate emit` and `calibrate trace compact` delegate to frozen turn13 code.
The separate frozen turn28 binary supplies `bank_local` traces:

```
turn28-bank predict BANK POOLED_FULL POOLED_SMALL PERMUTED_BANK < RAW
```

Per world memory contains `bank.bin`, `pooled_full.bin`, `pooled_small.bin`,
`permuted.bin`, `permuted_full.bin`, and `BOOKS.json`. The first four preserve
turn28's exact formats. `permuted_full.bin` is NETEI001 with every selected
pooled record and repeat counts rotated 1->6 while NEW count 0 is unchanged.

`results/world{w}/{regime}.full.tsv.gz` contains common fields
`t,k,heads,cold_heads,truth,rank,history,logcold,matched,record,source,
perm_source,max_norm_error,new_exact`; scalar before/after weights for local,
global and permuted; then seven outer fields for each of `local_full`,
`global_full`, `pooled_full`, `permuted_full`.

`results/world{w}/{regime}.bank.tsv.gz` is the unmodified turn28 interface.
The reader requires exact agreement between both traces for chronology, truth,
P0, HEAD bindings and role history. Only its `local` arm is exposed in the new
result as `bank_local`; the other retained fields remain audit evidence.

Every current candidate/live vector is fixed before stdin yields truth. Weights
update after truth only. A missing record prints prior local/permuted state and
does not update; global state is printed unchanged. All archives, traces and
source material are hash-manifested.
