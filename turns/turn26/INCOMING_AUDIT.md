# Incoming audit: Astra turn25

Sol, 2026-09-25.  Received after Oleg routed Astra's published hand back into
the Sol -> Don -> Astra -> Sol rotation.

## Publication identity

- checkout: `/Users/ataeff/arianna-codex/repos/netta-astra-turn25-20260922`
- branch: `astra/turn25-witness-latch`
- local HEAD: `71389502e514a39bd1a643fb2e5992de9677253c`
- remote head: `71389502e514a39bd1a643fb2e5992de9677253c`
- received worktree: clean
- parent: Don24 `798085be9a2bf8b6c41860238d4f38450558e126`

## Independent repeat

The retained reader was run from Astra's checkout with a new output path:

```sh
python3 -B turns/turn25/verify.py --output /tmp/sol-turn25-audit.XXXXXX/VERIFY.json
```

It reconstructed all four regimes in worlds 248--255, **2,097,152 forecasts**.
The new output and the published `turns/turn25/VERIFY.json` compare byte for
byte and share SHA-256:

`a3367a839f2e5e3c78e1adc1f84e85c1789a6b4d4e7cfa16008632ffc7c83fa3`.

The repeat reports:

- validity PASS: admission, archive, bounds, exact quotes, latch chronology,
  normalization;
- independent verification PASS;
- maximum numerical difference `1.3847056834492832e-10`;
- material FAIL, unchanged.

The failed material boundary remains the law-tail result: turn25 passes seven
of eight inherited conditions and does not solve the withdrawal problem.  The
saved diagnosis distinguishes carried confidence from the subsequent return
to slow updates but does not causally separate them.  No defect requiring an
incoming repair was found.  No turn25 artifact was edited.

## Routed additional obligation

Astra correctly retained Oleg's separate question about the measured Python
port and the older C mouth.  Don24 contains no C performance correction.
Turn26 therefore takes that engineering question alone; it does not reuse the
turn25 worlds or reinterpret its FAIL.
