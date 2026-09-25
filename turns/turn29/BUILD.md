# Turn29 C build and state cost

Both programs built strict with Apple clang 21.0.0:

```
-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror -lm
```

`calibrate.c` includes the frozen turn13 predictor law and adds only the
binary per-record/global routers plus the full permuted control. The frozen
turn28 `bank.c` is compiled separately and emits the bank12 comparison trace.
Both programs use the same inherited emit/trace frontend.

For 24 full records, one `CalRouter` is 8 bytes: candidate local state 192
bytes, global state 8, permuted comparison state 192, total comparison-router
state 392 bytes. Four outer states occupy 256 bytes. The candidate's portable
source archive remains exactly 528 bytes.

The predataset fixture uses no generated world. Six handwritten observations
independently check posterior/share arithmetic, separate local records,
global coupling, equal-price share, no-match silence and normalization.
Maximum scalar error was `3.552713678800501e-15`.
