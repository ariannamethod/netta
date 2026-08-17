# Speech manifest audit

Date: 2026-08-18. Don's untouched body-29 commit `8eecfd6` and all 284
inherited gates were reproduced before its implementation diff was read. This
record audits the manifest and cross-reader battery. Body 30 is not implemented.

## Finding: the manifest needed two names made precise

The first manifest used `context=` for the two byte positions from which the
mouth began. The court already uses `context=` for a different fact: external
ear priming supplied to a hearing. The values may be the same bytes, but their
causal roles are not. The manifest field is now `opening=`.

The manifest also omitted the speech law. Supported backoff and Laplace-red
can emit different streams from the same state, seed, hand, and opening. The
canonical line now names that choice explicitly:

```
spoke: candidate-digest=... bytes=... seed=... hand=... law=... opening=... lived-bytes=... episode=...
```

`hand` is exactly `uni`, `bi`, or `tri`; `law` is exactly
`supported-backoff` or `laplace-red`; identities are fixed-width lowercase
hex; unsigned integers have their canonical decimal spelling. Zero-byte
speech has the FNV empty-stream digest `cbf29ce484222325`.

## Independent reader and its limit

`scripts/manifest_check.c` shares no organism code. It reads exactly one
newline-sealed manifest from stdin, permits CRLF transport, consumes the full
line, reconstructs its canonical spelling, and independently reads the named
speech file to rederive byte count and FNV digest. It refuses fragments, extra
records, NUL, overlong records, invalid enum values, padded/signed/wide
integers, field drift, a different count or digest, and different captured
bytes. Its strict and ASan/UBSan builds are silent.

The reader deliberately prints only the two facts it can prove: byte count
and digest. A canonical manifest with restated seed, hand, law, opening,
lived-bytes, and episode is accepted when the stream witness still agrees.
That negative control matters: those fields are speaker statements. The
captured stream alone cannot independently rederive them, and syntax must not
be described as authorship.

No path reads the manifest into behavior. State and biography remain
byte-identical across speech.

## Cross-reader battery audit

The body-28 battery now contains 230 records. Fourteen new records put a
32-character token beyond every 31-character numeric scan class in the
sitting, court, and close records. A new positive control combines canonical
leaves from two independently printed worlds, preserves equal bytes/context
and sequential ids, and re-seals their public receipts and close. Both readers
accept it: the language binds its printed ordered leaves and has no hidden
reader-owned shore list.

Three static bases are now replayed before mutation from their small source
worlds: an 1800-byte `abcacb` shore, a disjoint 1800-byte `xyzxzy` shore, the
two 228-byte boundary candidates, and the `ab` opening. Their fresh court
output must be byte-identical to `base1.t`, `base2.t`, and `basectx.t`.
`base32.t` remains the independent maximum-shore grammar fixture.

Result on the repaired reader:

```
transcripts=230 both-accept=30 disagree=1 [S14_two_sittings] trace-leaks=0
```

Thus 30 records are accepted by both, 199 refused by both, and the sole named
difference remains office rather than language: the external checker accepts
two complete sittings as one transcript while one citation takes one sitting.
The expanded battery against pre-repair `98f9162` disagrees on 105 records;
11 new differences are long numeric operands the old citation admitted.

## Seed observation

The narrow period-3 source maps every observed byte pair to one continuation.
Both seed 7 and seed 8 therefore emit the same 60 bytes while all 60 choices
use trigram support. This is a property of the lived transition graph, not a
mouth fault. The wider NETTALOG world remains the negative control and yields
different digests for seeds 7 and 8.

## Position for body 30: authorship

Three roads were considered; none was implemented.

1. An external manifest plus later stream rederivation cannot bind authorship
   after the life continues, because the generating state has changed and the
   manifest is not in the biography chain.
2. A future `s` biography event is the honest road. Speech generation can
   remain behaviorally read-only, consuming no life RNG and changing no model,
   while the act of speaking becomes an explicit state/biography publication.
   The event should carry the exact manifest fields, including `law` and
   `opening`; the biography chain supplies causal placement. This is a real
   constitutional change: the mouth invocation will no longer be storage
   read-only and inherits the two-file publication debt. Its crash contract
   must be preregistered before code.
3. A pure shore court cannot add `speaker=` from a supplied manifest. It has no
   independent speaker fact and would turn a statement into a court finding.

Even the second road would establish causal authorship inside this life's
biography, not a cryptographic signature to an unknown external reader. FNV
remains a public integrity witness.

## Result

The tree passes 294/294 gates. Strict C11 builds are warning-free, shell
syntax is clean, and ASan/UBSan are silent through both the mouth and the
independent manifest reader. State remains v20. Body 30 remains unopened.
