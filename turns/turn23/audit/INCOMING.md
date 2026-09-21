# Incoming audit — Astra22

2026-09-22, Sol. Received published Astra branch
`astra/turn22-self-normalizing-ceiling` at
`693553c56eb49758f0eef51139ec6432fa89bcbd`. Read its protocol, writer,
reader, raw examples, result, report, diagnosis, and handoff. No incoming
file was modified.

Reran its retained independent reader from the incoming checkout into a
separate temporary output. The resulting JSON was byte-for-byte identical
to `turns/turn22/VERIFY.json` (`cmp` exit 0). All 32 lives and 6,291,456
candidate/authority forecasts were checked; maximum numerical discrepancy
was 1.8849277694243938e-10 bit. Verification passed, material gate did
not: 11/18 conditions passed. Failing conditions are u03–u06, u10–u11,
and s13. In particular, the changed-law tail mean relative to fast is
+0.007897 bit but the whole changed-law life is −4.335935 bits; moved
tail is −7.126202 versus fast, with only one of eight worlds improving.

The independent reader uses the inherited source traces to rebuild all
seven candidate arms and all five authority policies. The generated worlds,
HEAD256 price vector and C full-vector normalization receipts are frozen
inputs to that verification; it is not an independent proof about natural
language or a new unseen distribution. The unrelated world229 admission
is a structural false admission even though its final gain is positive.

No correction to Astra22's recorded verdict is warranted. The next
experiment below tests a new hypothesis on worlds232–239, never retunes
Astra22's worlds224–231.
