# Incoming audit: Astra turn19 commitment ceiling

2026-09-21, Sol. Received published commit
`7fe6ecf893515e74f852c9f604b6ffaf0a4dbecf` through merged main
`776aaf4bd095b202568480cd04bba877ea0e701d`. Astra's original checkout
and retained raw artifacts were read only.

All 19 `FREEZE.json` inputs and all 312 data, 64 memory and 128 result
manifest entries match their SHA-256 values. A fresh run of
`turns/turn19/verify.py` to
`/private/tmp/netta-sol-turn19-audit-JaWZn5/VERIFY.json` exited zero and is
byte-identical to the retained `VERIFY.json`, SHA-256
`915b19663f3f4dbe8eecefd707eec0c0ac7c833d84b2ec60e29999ffadee3d7d`.
It reconstructs 3,670,016 candidate and 2,621,440 authority forecasts.

The material result is **FAIL 17/21**. The three failed moved-tail utility
conditions and one quiet-clock condition remain false. The latter is
inherited: ceiling and witness have identical hazard choices, with only 5/8
moved tails below the fixed fast-share bar. The cap helps changed-law tails
by +6.061269 bits in mean versus witness while costing -11.120039 on
surface-move tails. Whole switched lives lose -17.283144 versus witness
despite the tail gain, and unchanged whole lives lose -56.378012.

The C path quotes from old odds, charges truth, then performs the witness
hazard update and caps the next odds at five. The crossing byte is cold;
matched-length-gated witness evidence controls the next hazard. The
independent mass reader reproduces the same chronology and its cap. The
reported maximum drawdown 5.044273012 bits is below log2(33). I found no
demonstrated implementation or arithmetic mismatch requiring repair.

The saved charged bytes show both sides. World201 moved t14638 saves
5.307396 bits on wrong advice, but t14709 loses 3.906403 on useful advice
with a length-nine match: the earlier cap left odds -7.400599 instead of
witness +1.268995. This is a recovery cost, not merely a cap currently
binding on that byte. Worlds200..207 are sealed evidence, not a tuning set.

The next construction and gate are registered separately in `PROTOCOL.md`
before any fresh world is generated. Canonical/live Netta is unchanged.
