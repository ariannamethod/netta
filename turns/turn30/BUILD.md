# Turn30 C build and state cost

Both programs built strict with Apple clang 21.0.0:

```
-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -lm
```

`router3.c` includes the frozen turn13 predictor law and adds the three-way
local router, the turn29 binary router unchanged, the NETEB001 loader and the
A+B/rotation address checks transcribed from turn28's `bank.c`, and the
inherited outer law with admission supplied from outside instead of from the
arm's own shadow. The `full24` yardstick is produced by turn29's own
`calibrate` binary, rebuilt here from frozen source: it digests
`b970060ef87e4968bfe1615c5ba731b71a69501c5d246dc83b040aa4143bc8f1`, byte-equal
to the hash turn29's FREEZE pins, as does turn28's `bank`
(`21b0dc1157338a565426ab3ec9948b6f2bb9c7d66323662849a550ba8dc07ed8`), which
that build pulls in.

For the 12 selected records one `R3Three` is 24 bytes and one `R3Two` is 8:
candidate three-way state 288 bytes, pooled comparison state 96, null
comparison state 96, total local-router state 480 bytes. Four outer states
occupy 256 bytes. None of that is portable; the portable cost is the archive
alone — 528 bytes for `bank.bin` against 336 for `pooled_small.bin`, a
difference of exactly 192 bytes, one 16-byte count vector per address.

The predataset fixture uses no generated world. Forty-eight handwritten
observations check the three-way posterior/renormalization/share arithmetic,
two separate records, the binary routers on the pooled and rotated stories,
no-match silence, the all-cold visit, NEW truths, and the shared admission
firing once for all four arms on the same step (step 33 of the fixture).
Maximum scalar error against the reader's Decimal classes was
`2.842170943040401e-14`.
