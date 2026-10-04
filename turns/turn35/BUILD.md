# Turn35 build and prefreeze receipts

Date: 2026-10-04. Parent: `3c780af57b11e716d8d7c127917f52302618a959`.

- Strict build passed under `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`.
- The Makefile stages turn33 and turn34 with entry-point renames only. It
  asserts exactly four diff lines for turn33's two textual mains and exactly
  two diff lines for turn34's one main.
- `hybrid_router --fixture34` is byte-identical to the parent
  `sel4_router --fixture`, SHA-256
  `515b44220375d1fb6c4cf452d1189b1e0faa289adb2531f332a8ce22a42f8f9d`.
- The independent hybrid fixture checked 40 events and 520 numerical values,
  maximum error `7.105427357601002e-15`. It witnessed fine and coarse
  records, first/silent/recovery/locality, NEW, no-match, exact-common,
  positive and negative residuals; all four fine residuals crossed both
  signs.
- Before freeze, the complete writer and independent reader were exercised
  on the eight already-open turn34 worlds (`304..311`) in a temporary tree.
  This was interface testing, not evidence for turn35. The independent
  reader replayed 3,932,160 forecasts, rebuilt all archives and selector
  positions, and agreed with the writer to maximum error
  `3.40634187523392e-11`. One prefreeze type defect found by this dry run
  (source ranks arrived as a list where SHA-256 requires bytes) was corrected
  before the freeze and before any turn35 world existed.

## Post-freeze retained batch

- Protocol SHA-256:
  `275bc7c2e565895625e26d3946932b764477bf3cf5d8307a46f28ba0878f7965`.
- Freeze SHA-256:
  `86726aa5774084b3b857d0dbacd2dba293dfc5fed95d5be502bb96cba1c10cda`;
  29 frozen identities.
- Exactly one fresh batch, worlds 312–319, was generated and retained.
- Writer result SHA-256:
  `61dfe316196e340099f4512a535208538c6547c483b038bceb256623c4fa0251`.
  As required, the writer left G5 and the material gate false.
- Independent receipt SHA-256:
  `d19a2c38c021f3e359dc8dc2f21a8ba681c892c8eba71db7cdbdec6c13d2a311`.
  It replayed 3,932,160 forecasts to maximum error
  `3.5456082514429e-11`, set G5 true, and retained the material FAIL.
- The creation-only reader red probe refused an existing output path at rc=1;
  the receipt hash before and after was identical.
- A final pristine rerun after report assembly reproduced both fixture and
  reader receipts byte-for-byte at the same two hashes.
- Final gates: G1 FAIL, G2 PASS, G3 FAIL, G4 PASS, G5 PASS.
