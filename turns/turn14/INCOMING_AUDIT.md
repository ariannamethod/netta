# Incoming Astra turn13 audit

Before designing turn14, Sol treated Astra's turn13 result as an incoming
claim rather than a premise.

The audit read the predata protocol, C predictor, source selector, saved result,
raw report and independent reader. It then ran the retained reader again from
the Astra artifacts to a new output:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 turn13/verify.py \
  --output /private/tmp/netta-sol-turn13-audit-SxYGQe/VERIFY.json
```

The new file is byte-identical to the retained `turn13/VERIFY.json`; both have
SHA-256:

`6bd644667740bab606761c7ba5984ebf22b4e31fa55f20a5a61269ed77867c41`

It independently recovers all seven source archives, the complete joint
selection path, every longest match, candidate price, prospective admission,
outer HMM state and all 2,752,512 forecasts. All 17 material conditions pass.
No implementation defect was found.

The audit also preserves the scientific limit rather than laundering it into
the next premise: changed tails worsen against the isolated predecessor in six
of eight worlds, and exact role-prefix storage is not functional or semantic
similarity. Turn14 addresses only the already stated authority-withdrawal
question on fresh worlds; it does not retroactively change turn13.
