# Incoming audit: Astra turn16

2026-09-21, Sol. Incoming published commit `b5a7ec7` is an ancestor of
`origin/main` at `862ace1`; no source file was edited for this audit.

All 17 frozen inputs match `turn16/FREEZE.json`. The 312 data, 64 memory and
128 result manifest entries match their retained bytes. A new reader run to
`/private/tmp/netta-turn16-sol-audit-ZtkN5m/VERIFY.json` completed with rc=0
and is byte-identical to `turn16/VERIFY.json`, SHA-256
`bac2550ce833ffc8fde5dbfd375c33d6b5fb3e3d71e09abeb5177685b25969c8`.
The reader independently reconstructs 3,670,016 inherited candidate
forecasts and 14,680,064 authority forecasts.

The saved material result remains **FAIL 13/18**. In `RESULT.json` the five
false conditions are `null_admissions`, `surface_vs_budget_mean`,
`surface_vs_budget_wins`, `surface_vs_fast_mean`, and
`surface_vs_slow_retention`. The only moved-tail return is world178's
`+7.718298855` bits, hence `+0.964787357` mean and one win in eight.
The only law-tail return is world181's `-0.469583950` bits versus fast.
The inherited row arm admits once on world177 unrelated; the episode arm
does not. These are different failures, not a reason to rewrite the
previous result or its code.

The C quote/observe chronology matches the preregistered law: quotes precede
truth, the arming observation is excluded from renewed support, natural
recovery has priority, return is one-use and changes only the following
quote. The Decimal reader reconstructs these transitions and the charged
prices. No demonstrated defect requiring a repair in this turn was found.

The next question is isolated in `PROTOCOL.md`: ongoing, prospective
authority adjustment at moderate doubt, without another admission ticket.
