# Turn34 build receipts

2026-10-04, Don, base `1fda545`, protocol frozen at `dc55922`
(PROTOCOL.md sha256 `74fac529…`, pinned in verify.py and FREEZE.json).

- Build: `make` under `-O2 -std=c11 -Wall -Wextra -Wpedantic -Werror`, rc 0.
  `sel4_router` sha256 `0dd69fff40e6…`; `episode` is its symlink.
- Frozen-core staging: the Makefile stages `turns/turn33/earned_router.c`
  into `.build/turn33_core.c` with exactly the two `int main` lines renamed
  (one inside the `#if 0` turn31 provenance block, one live) and fails the
  build on any further drift. Provenance probe: `sel4_router --fixture33`
  reproduces the freshly built turn33 `earned_router --fixture` output at
  the same sha256 (`4d67e152…`, both hands, this session).
- Fixture gate, prefreeze: C versus independent Decimal over the 28-step
  handcrafted sequence — 364 numeric checks, max error `2.66e-15`, all
  witnesses including per-episode positive AND negative wealth for all four
  episodes. The green probe ran before generation into the session
  scratchpad; the committed FIXTURE_READER.json was produced after the batch
  from the byte-identical deterministic fixture (same checks, same error).
- Red probe of the fixture checker: one wealth value in a copy of the
  fixture output moved by +0.5 → the Decimal reader refuses by name with the
  exact delta printed, rc 1.
- One schedule correction before the freeze: the first draft of the fixture
  never let episode 3 reach positive wealth; truth at t=0 was changed to 7.
  This happened before `experiment.py freeze` and before any world existed.
- Freeze defect, disclosed: the first FREEZE.json missed the
  `turns/turn30/verify.py` pin; the independent reader refused at identity
  and wrote no receipt. See FREEZE1_FAILURE.md; the defective batch is
  retained whole in the session scratchpad. The second freeze added the one
  pin; the second batch reproduced the first batch's conditions, tables and
  quantities exactly, as determinism requires.
- File identities at the second freeze: experiment.py `2442b1bc…`,
  verify.py `146a3d75…`, fixture_check.py `2ba96a29…`,
  sel4_router.c `ecc17ba0…`; the full pin set is FREEZE.json (55 lines).
