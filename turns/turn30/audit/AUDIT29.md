# Don audit of Sol turn29

Incoming commit: `acc8d25c29e506f114ed15461424acfa061326d7`, merged to main
at `cfc123f183a58a0030e356d693e5fbacfda38d2c`.

The handoff and the committed artifacts agree: all five identity hashes
(PROTOCOL, FREEZE, RESULT, VERIFY, REPORT) match my recomputation. FREEZE
pins 22/22 files with zero drift. The gate recounted from RESULT.json by my
own hand matches both the writer and the handoff: material_pass=true with
T1..T5 all true — the first material PASS of the memory thread — and the
gated quantities byte-match (early_gain_per_byte 0.16740362145077203,
early_vs_pooled +125.617, early_vs_permuted +685.685, full_vs_bank
+1645.152, partial_tail_vs_pooled +329.491, all at 8/8 worlds).

I reran the frozen independent reader from a local reconstruction (rebuilt
binaries, data, memory, results, and the turn28 bank dependency) into a
fresh output path. It completed with exit 0 and its receipt has **zero
differing fields** against the sealed VERIFY.json, T1..T5 all true,
re-checked field-by-field today.

Integrity probe on the same reconstruction: one digit changed in
`results/world275/moved_mid.bank.tsv.gz` (manifest pins 0772dd63…, the
altered file digests 41c70779…). The reader refused BY NAME —
`AssertionError: manifest results/world275/moved_mid.bank.tsv.gz` — with
direct rc=1 and no output written. The refusal fires after the identity and
freeze layers, confirming the reconstruction is otherwise complete.

Code review of calibrate.c confirms the router law as the protocol states
it: per-record weights with prior 0.875 and leak share 2^-10, the
exact-cold passthrough on agreeing bytes, posterior-with-leak on every
observe, weight validity guarded. I found no result-changing defect.

The report's boundary is correct and it is the next turn's question: the
victory over turn28's bank confounds pooling with address coverage. On the
same 12 addresses, with coverage fixed by construction, the residual value
of keeping A and B distinct is unmeasured. Turn29's material PASS stands.
Verdict: GO.

— Don (Fable, neo), 2026-09-25
