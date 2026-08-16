# Warrant-receipt audit and the sitting docket

Date: 2026-08-17.  Don's untouched `81bc616` body-26 commit and all 234
inherited gates were reproduced before its diff was read.  The findings below
were then measured against a fresh strict build of that commit.  The body-27
law and predictions in this file were written before its implementation.

## What survived

The receipt construction is deterministic and independently reproduced.  The
organism and `scripts/warrant_check.c` share no source or linked object; each
implements FNV-1a-64 from the public offset and prime.  A canonical two-shore
transcript is accepted, all original red arms fire, and the published limit is
correct: this is a transcription witness, not a signature or a secret.

The law and line receipts solve the problem they actually name.  If an operand,
verdict, receipt, or law text is edited without rebuilding its witnesses, the
external hand refuses the line.  They do not yet establish that a transcript
is a complete, ordered account of one hearing.

## Independent audit findings

### A receipt line is not a sitting

A single body-26 invocation over two shores printed one law header followed by
two individually valid verdict lines.  Keeping the same valid lines and
receipts, the external hand accepted every one of these as a transcript:

```
dropped:    warrant accepted: 1 verdicts under law f2cc2274cb3c752b
duplicated: warrant accepted: 2 verdicts under law f2cc2274cb3c752b
reordered:  warrant accepted: 2 verdicts under law f2cc2274cb3c752b
spliced:    warrant accepted: 2 verdicts under law f2cc2274cb3c752b
```

The splice joined one valid line from a 300-byte literal candidate with one
valid line from a different all-`z` candidate under the same law.  No receipt
failed because no receipt names the candidate or the membership and order of
the sitting.  The checker can verify each leaf while accepting a fabricated
book.

### The external hand has no canonical grammar

Because fields are found by substring and numbers are parsed without checking
what was consumed, a public re-sealing hand produced four non-canonical lines
that body 26 accepted:

- `gap-micro=garbage` was parsed as zero and accepted as ABSTAIN;
- P, Q, matched16, and G could all be removed while the line was accepted;
- `matched-bytes=0/0` was accepted as satisfying the REPLAY half law;
- two `gap-micro` fields were accepted, with only the first governing the
  verdict.

These are not cryptographic attacks.  Re-sealing is deliberately possible
because FNV is public.  They show that the external hand authenticates
arbitrary strings while claiming they are canonical Netta warrants.

The hand also accepts any replacement law text when its public FNV and line
receipts are recomputed, even though verdict re-derivation remains hardcoded to
body 25's thresholds.  It accepted both a lone `pattern-court law v999` and a
transcript mixing a real v2 sitting with that different law; its final success
line attributed both verdicts to the later digest.  An independent
implementation must carry the exact law whose semantics it implements.

## Body 27 preregistration: the sitting docket

Body 27 does not let a court act and does not yet write its verdict into a
life.  It first makes one complete hearing citable.  A warrant is a sequence,
not a bag of valid leaves.

Each invocation will have this strict frame:

1. the existing exact canonical v2 law header;
2. one sitting header naming candidate digest and byte length, context, shore
   count, and law digest;
3. exactly that many canonical verdict lines, with court ids 0 through n-1;
4. one close line naming the count and a docket digest.

The candidate digest is FNV-1a-64 over the exact heard bytes under the same
public witness law already used for island identities.  It is identity for
this transcript, not collision-resistant authentication.  The docket starts
from the public FNV seed and consumes, in order, the complete sitting header
and every complete verdict line including its receipt, with one newline byte
after every record.  The close line publishes the resulting digest.  The
header's shore count and sequential ids make deletion, duplication, and
reordering explicit; the candidate identity makes cross-candidate splicing
explicit.

`scripts/warrant_check.c` will become a strict state machine.  Concatenated
complete sittings remain legal, each with its own `NETTA ZERO`, law, header,
lines, and close.  Unknown, overlong, partial, duplicated, out-of-order, or
out-of-frame records are refused by line.  The hand will carry an independent
copy of the exact v2 law text and refuse any other text even when its FNV is
self-consistent.  Court fields must occur once in canonical order and consume
the whole line.  Numeric tokens must be complete and in range; candidate byte
count, context, exact match denominator, court id, and count must agree with
the sitting header.  The human decimal witnesses must be finite and consistent
with their public integer operands at the printed resolution.

Before the body is run, the predictions are:

- Don's honest body-26 calibration transcripts, now emitted inside the new
  frame, remain accepted and their line receipts stay unchanged;
- dropping, duplicating, reordering, or cross-candidate splicing any valid
  verdict line is refused by the docket even though every leaf receipt is
  still valid;
- a different self-consistent law, malformed integer, zero denominator,
  duplicated field, missing human operand, unknown record, overlong record,
  premature EOF, or court line outside a sitting is refused;
- CRLF transport and concatenation of independently complete sittings remain
  accepted;
- candidate digest changes when the heard bytes change and is stable across
  the same candidate heard by different convoys;
- all earlier verdicts, thresholds, invocation refusals, no-write law, state
  v20, and sanitizer behavior remain.

This docket is the last missing mechanical prerequisite before a biography can
honestly cite a court sitting as an event.  It still supplies no independent
review, signature, requested power, revocation, or activation.  Those social
and constitutional facts cannot be manufactured by a stronger checksum.

---

## Results

The body-27 transcript retained every body-26 leaf byte for byte.  On the
two-shore audit sitting, for example, the REPLAY receipt remains
`fd8b7974e021a5a9` and the ABSTAIN receipt remains `e98c2a2a2aedfae9`.
The new frame around them is:

```
court sitting: candidate-digest=ca0d16aadc050d91 bytes=300 context=cold shores=2 law-digest=f2cc2274cb3c752b
...
court close: verdicts=2 docket=e2898cdce3dfbe2d
```

The fixture hand independently recomputed `ca0d16aadc050d91` from the exact
candidate bytes.  The same candidate kept that identity with one or two
shores, while the all-`z` candidate changed it.  The external hand accepted
the complete two-shore sitting and a concatenation of two complete sittings;
the same book transported as CRLF also remained valid.

All preregistered red predictions landed.  Removing either leaf, duplicating
one, reversing their order, or replacing one with a receipt-valid leaf from
the other candidate was refused.  A repeated law inside an unfinished frame
was refused, while repeated laws belonging to separately closed sittings were
accepted.  A self-consistent v999 law could no longer borrow the checker's
hardcoded v2 semantics.

The public fixture then rebuilt both receipt and docket around each malformed
record that body 26 had accepted.  The strict hand refused
`gap-micro=garbage`, an overflowing integer, missing P/Q/matched16/G,
`matched-bytes=0/0`, and a duplicated gap operand even though every public FNV
was valid.  It also refused short and non-hex receipts, an unknown record, a
1023-byte record, and EOF before the close.  Thus the grammar gates are
independent of the receipt gates, exactly as preregistered.

All 255 gates pass.  Strict C11 builds for the organism, verifier, and public
red constructor are warning-silent.  ASan/UBSan are silent both through the
organism's pattern court and through the external hand on honest and crafted
transcripts.  State remains v20; the court still writes nothing, elects
nobody, and grants no action or speech authority.
