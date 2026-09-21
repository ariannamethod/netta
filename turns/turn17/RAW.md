# Charged bytes of the prospective clock

All rows are retained observations from world184, not generated examples.
Trace index `t` is zero-based. C and S are log2 probabilities of the actual
raw byte; A and F are charged adaptive and fixed-fast quotes. A-F is the
one-byte saving or cost of the new clock. The full traces live in ignored
`results/world184/*.tsv.gz` and `results/authority/world184/*.tsv.gz`, with
their SHA-256 values in `RESULTS_MANIFEST.json`. The tables below are a
readable window, not a substitute for the complete trace.

| Life | t | Actual byte / rank | C | S | F | A | A-F | Evidence before / clock |
|---|---:|---|---:|---:|---:|---:|---:|---|
| moved_mid helpful | 8429 | `0x85` / 1 | -6.162923 | -1.991599 | -5.822269 | -2.764743 | +3.057526 | +16.021083 / slow |
| moved_mid harmful | 12283 | `0x0f` / 4 | -3.662634 | -15.628091 | -7.779586 | -13.382932 | -5.603347 | +38.270004 / slow |
| moved_mid protected equal | 8192 | `0x62` / NEW | -15.598964 | -15.598964 | -15.598964 | -15.598964 | 0 | +54.739440 / slow |
| switched helpful | 8414 | `0x2f` / 1 | -6.578194 | -1.036912 | -4.793109 | -1.358081 | +3.435028 | +21.289505 / slow |
| switched harmful | 8319 | `0x3d` / 2 | -1.051255 | -4.546400 | -1.438473 | -4.046830 | -2.608357 | +33.508318 / slow |
| switched protected equal | 8192 | `0xef` / NEW | -6.128578 | -6.128578 | -6.128578 | -6.128578 | 0 | +54.739440 / slow |

The new law pays both sides, not just the favorable ones. On the moved tail
it gains 10.186155 bits over fast in world184 despite the displayed
5.603347-bit loss on a single wrong candidate. On the switched tail it
loses 4.425604 bits in world184, even though the displayed helpful byte
gains 3.435028 bits.

The previous evidence can remain strongly positive at the seam. The first
fast-clock quote after t8192 on switched world184 is t9289: before that,
the adaptive clock has already exposed memory under a slow hazard. For all
eight switched worlds the first fast-clock byte lies between t8445 and
t9289. Later fast updates do not refund the earlier charged loss. No
regime or seam label was given to the clock.
