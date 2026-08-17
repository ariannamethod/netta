# Citation canonical-language audit

Date: 2026-08-17. Don's untouched body-28 commit `98f9162` and all 268
inherited gates were reproduced before its implementation diff was read.
This record describes the independent audit, the repair, and the remaining
boundary. Body 29 was not opened.

## Finding: a valid witness did not imply a valid court record

The external checker and the life disagreed about the language of a complete
sitting. `scripts/warrant_check.c` parsed the complete canonical sitting
header and every semantic field of every court record. `cite_verify()` checked
the banner, exact law, shore order, leaf receipt, ordered docket fold, and
close, but did not parse the court record behind the receipt.

A public construction made the disagreement executable. The record

```
court 0: canonical-shape=false verdict=replay
```

was given its correct public receipt under the canonical law and placed under
a correctly recomputed docket close. The external checker refused line 4 as a
noncanonical court record. `--cite` accepted the same file and appended a `w`
event to the biography.

The same gap admitted five independently recomputed header variants that the
external checker refused:

- a one-digit candidate digest;
- a signed byte count interpreted as `UINT64_MAX`;
- a zero-padded byte count;
- an undeclared context word;
- a canonical header whose bytes or context disagreed with its leaf.

The receipt and docket were doing their stated job: witnessing exact bytes.
The citation reader had mistaken witnessed bytes for canonical meaning.

## Repair

The life now carries its own strict parser for the body-27 language. It shares
no code with `scripts/warrant_check.c`, preserving two independent hands, but
requires the same invariants:

- exactly 16 lowercase hexadecimal digits for candidate, law, shore, receipt,
  and docket identities where applicable;
- canonical unsigned decimal counts, candidate bytes in `[2, 16384]`, and
  shores in `[1, 32]`;
- context exactly `cold` or four lowercase hexadecimal digits;
- a canonical court id, complete finite decimal fields, exact field order,
  and full-line consumption;
- equality between sitting bytes/context and every leaf;
- exact fractions and printed decimal witnesses derived from the integer
  operands;
- the canonical verdict derived from changed bytes, coverage, and gap;
- the original leaf receipt and ordered docket fold.

The court, its thresholds, the docket format, the biography line, and state
v20 did not change. Only the admission reader was repaired.

## Wider audit

### Input boundaries

The repaired reader refuses overlong records, embedded NUL, every incomplete
newline boundary, zero or 33 shores, noncanonical numeric spellings, a header
and leaf that disagree, a second sitting, and any invalid close. CRLF transport
remains accepted and produces the same canonical fold.

### Invocation boundary

All 27 controls outside `--cite`, `--state`, and `--bio` were swept
individually. Each was refused before biography mutation. The three admitted
file controls remain the whole citation surface.

### Publication boundary

Citation inherits the existing two-object publication law. If biography is
one line ahead of state, resume refuses on line count and chain. If state is
one line ahead of biography, resume refuses by the same law. This is
fail-closed and does not silently replay or erase the event, but it is not an
atomic two-file transaction and has no automatic recovery. Body 28 does not
worsen that debt; it also does not solve it.

### Powerlessness boundary

The original one-episode price twin was extended through:

- two resumed stretches;
- seven additional episodes and 2400 lived bytes per arm;
- units, jury, and atlas choice;
- reversed convoy order on the second resume.

The two arms printed byte-identical output. Their biographies became
byte-identical after removing the one `w` line. Their final state files differed
only in bytes 41-56, the persisted biography line count and chain; no behavioral
state byte differed.

### Cross-life meaning

A life may cite a sitting about bytes produced elsewhere. This remains legal
because a body-27 docket names a candidate, not a speaker, and the `w` event is
documented as an external fact rather than a self-claim. Adding a `self` or
`other` marker now would manufacture knowledge the record does not contain.
Speaker binding remains the smaller candidate for a future authorship body.

## Result

The repaired tree passes 275/275 gates. Strict C11 builds are warning-free,
shell syntax is clean, and ASan/UBSan are silent through the citation and the
whole inherited organism. Body 28 is repaired but should receive Don's fresh
independent reading before body 29 is preregistered.
