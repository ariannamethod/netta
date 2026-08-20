# The biography language

The biography is the life's append-only public record: one file of
newline-sealed records, chained by the state (record count and FNV-1a-64
over the raw record bytes including the newline, seed
`cbf29ce484222325`). This document is the language's specification,
derived from every emit site in `netta.c` — the fifteen `bio_append`
call sites and every format arm they carry — not from samples. It is a
document and test data may be shared with any reader; executable parser
code is not shared: the organism (`bio_verify`) and any outside reader
(`scripts/biography_check.c`) implement this language independently, so
that each remains a differential witness against the other.

## Record framing

- A record is a sequence of bytes ending in exactly one `\n` (0x0A).
- No record contains a NUL (0x00) or a carriage return (0x0D): no emit
  site can produce either.
- Fields are separated by exactly one `\t` (0x09). A run of tabs, a
  space beside a tab, or a space in place of a tab is not separation:
  every record equals its own canonical reprint, byte for byte.
- The longest legal record is bounded by the organism's emit buffers
  (at most 256 bytes for every type; the largest formats stay under
  200). An outside reader may frame generously (1024) but must refuse
  an unbounded record.
- The first field names the record's type: either a canonical episode
  number (a step record) or one of the twelve letters
  `a b d i m q r s t u v w`. Any other first field is no record.

## Canonical field forms

- **u64** — canonical unsigned decimal: digits only, no sign, no
  leading zero (`0` itself is legal), at most twenty digits, within
  64 bits.
- **hex16** — exactly sixteen lowercase hexadecimal digits (`%016llx`).
- **hex4** — exactly four lowercase hexadecimal digits (`%02x%02x`).
- **hexpair** — an even number of lowercase hexadecimal digits, from 4
  to 32 (a unit body of 2..16 bytes, two digits per byte).
- **fix6** — a canonical `%.6f` rendering: a u64-shaped integer part,
  `.`, exactly six decimal digits. Every fix6 field in the biography
  is non-negative by construction (prices and scores are
  `-log2(p) >= 0`).
- **actor** — one of `uni`, `bi`, `tri`, `mv`, `null`.
- **hand** — one of `uni`, `bi`, `tri` (the mouth's byte hands).
- **law** — one of `supported-backoff`, `laplace-red`.
- **context** — `cold` or hex4.

Compile-time domains are grammar, not biography history: a registry id is
`0..1023`, a unit id is `0..4095`, a current-convoy slot is `0..31`, and a
move id is `0..4351` (256 atomic moves followed by at most 4096 units).
Whether a named registry or unit already exists, is present, is living, or
owns the stated length is state-relative truth and is deliberately not
decided by a stateless reader.

## The thirteen record types

Formats are quoted from `netta.c` verbatim; field names follow.

### step — one lived byte (emit: `speak`-free play loop)

    EP \t STEP \t REG \t POS \t CTX \t ACTION \t TRUTH \t LOSS \t RNG \t atomic \t 1 \t ACTOR

12 fields: u64 episode; u64 step index within the episode; registry id
(`0..1023`); u64 position on the island; hex16 context digest; u64 action byte
(0..255); u64 truth byte (0..255); fix6 loss; hex16 RNG state before
the draw; the literal `atomic`; the literal `1`; actor. Format:
`"%llu\t%llu\t%d\t%llu\t%016llx\t%d\t%d\t%.6f\t%016llx\tatomic\t1\t%s\n"`.

### a — the episode's seated actor (one per episode)

    a \t EP \t ACTOR|mvp

3 fields: u64 episode; actor, or the literal `mvp` when a local
probation trial borrows the body. Format: `"a\t%llu\t%s\n"`.

### b — a unit is born

    b \t EP \t UNIT \t LEN \t HEX \t SUPPORT

6 fields: u64 episode; unit id (`0..4095`); u64 length in bytes
(`2..16`); hexpair body of exactly 2×LEN digits; u64 pair support, at
least 64 and a multiple of 64.
Format: `"b\t%llu\t%d\t%u\t%s\t%llu\n"`.

### u — a dead unit resurrects under its own name

    u \t EP \t UNIT \t SUPPORT

4 fields: u64 episode; unit id (`0..4095`); u64 support that earned the
return, at least 64 and a multiple of 64. Format:
`"u\t%llu\t%d\t%llu\n"`.

### d — a unit dies

    d \t EP \t UNIT \t USES \t IDLE

5 fields: u64 episode; unit id (`0..4095`); u64 lifetime uses; u64 steps
idle since last use (`>=16384`). Format:
`"d\t%llu\t%d\t%llu\t%llu\n"`.

### m — a macro event: a unit speaks as one move

    m \t EP \t ISL \t POS \t UNIT \t LEN \t NLL

7 fields: u64 episode; current-convoy slot (`0..31`); u64 position; unit
id (`0..4095`); u64 length (`2..16`); fix6 price. Format:
`"m\t%llu\t%d\t%llu\t%d\t%u\t%.6f\n"`.

### v — a move-navigation verdict (two arities)

    v \t EP \t REG \t POS \t MOVE \t LEN \t ADVANCE \t NLL \t TARGET
    v \t EP \t REG \t POS \t MOVE \t LEN \t ADVANCE \t NLL \t TARGET \t POLICY

9 fields with navigation disabled, 10 with it enabled: u64 episode;
registry id (`0..1023`); u64 position; move id (`0..4351`); u64 move
length (`1` for an atomic move, `2..16` for a unit); u64 advance
(`1..LEN`); fix6 price; target move (`0..4351`); and, in the navigation
arm, policy anchor (`0..4351`). The C emitter carries that last value in
a signed variable and prints it with `%lld`, but `move_route_anchor`
always returns a real move; the pre-search `-1` sentinel is never emitted.
Formats:
`"v\t%llu\t%d\t%llu\t%u\t%u\t%u\t%.6f\t%u\n"` and the same with
`"\t%lld"` before the newline.

### t — an atlas decision (three arms)

    t \t EP \t REG \t eligible
    t \t EP \t REG \t chart \t LIVED \t RUNNER
    t \t EP \t REG \t earned \t SCORE \t RUNNER \t LIVED

4, 6 or 7 fields: u64 episode (the one being entered); registry id
(`0..1023`); then the arm. `eligible` — the sole shore that can hold the
episode. `chart` — an under-lived shore chosen by least lived bytes:
u64 lived (`0..999`) and u64 runner-up lived. A competitive decision has
at least two eligible shores, so the initializer `UINT64_MAX` is never
emitted as a no-runner sentinel. `earned` — chosen after every eligible
shore has lived at least 1000 bytes, by fix6 score and runner-up score
(both `0.000000..8.000000`), followed by u64 lived (`>=1000`). Formats:
`"t\t%llu\t%d\teligible\n"`,
`"t\t%llu\t%d\tchart\t%llu\t%llu\n"`,
`"t\t%llu\t%d\tearned\t%.6f\t%.6f\t%llu\n"`.

### r — a seat revoked (two arities)

    r \t EP \t ISL \t SEATED \t CHALLENGER \t LU \t LSEAT
    r \t EP \t ISL \t SEATED \t CHALLENGER \t LU \t LSEAT \t 8.000000

7 or 8 fields: u64 episode; registry id (`0..1023`); seated actor
(`bi`, `tri`, or `mv`); a distinct challenger. In the 7-field red arm
the challenger is a byte actor; in the 8-field arm it is a byte actor or
`null`. The next two fields are fix6 challenger and seat prices; the
8-field arm carries the literal `8.000000` — the unpriced seat's uniform
price.
Formats: `"r\t%llu\t%d\t%s\t%s\t%.6f\t%.6f\n"` and
`"r\t%llu\t%d\t%s\t%s\t%.6f\t%.6f\t8.000000\n"`.

### q — a refusal names the null hand (two arms, one shape)

    q \t EP \t ISL \t SEATED \t null \t 0
    q \t EP \t ISL \t SEATED \t CHALLENGER \t 0

6 fields: u64 episode; registry id (`0..1023`); seated actor (`uni`,
`bi`, `tri`, or `mv`); the distinct literal `null` (an indivisible
episode would cross the byte budget) or a byte-actor name (an unpriced
move seat steps down); the literal `0`. Formats:
`"q\t%llu\t%d\t%s\tnull\t0\n"` and `"q\t%llu\t%d\t%s\t%s\t0\n"`.

### i — an island arrives in the registry

    i \t EP \t ID \t DIGEST \t WITNESS \t LEN

6 fields: u64 episode; registry id (`0..1023`, sequential from 0); hex16 tape
digest; hex16 witness; u64 tape length. Format:
`"i\t%llu\t%d\t%016llx\t%016llx\t%llu\n"`.

### w — a citation: a court's word enters memory (body 28)

    w \t EP \t CANDIDATE \t BYTES \t CONTEXT \t SHORES \t LAW \t DOCKET \t VERDICTS

9 fields: u64 episode; hex16 candidate digest; u64 bytes (2..16384);
context; u64 shores (1..32); hex16 law digest; hex16 docket digest;
u64 verdicts, equal to shores. Format:
`"w\t%llu\t%016llx\t%llu\t%s\t%d\t%016llx\t%016llx\t%d\n"`.

### s — a signature: the mouth signs its speech (body 30)

    s \t EP \t CANDIDATE \t BYTES \t SEED \t HAND \t LAW \t OPENING \t LIVED

9 fields: u64 episode; hex16 candidate digest over exactly the emitted
bytes; u64 bytes (>=1); u64 seed; hand; law; hex4 opening; u64 lived
bytes (>=1). Format:
`"s\t%llu\t%016llx\t%llu\t%llu\t%s\t%s\t%02x%02x\t%llu\n"`.

## Context-free grammar versus state-relative truth

Everything above is **context-free**: any reader with no state can
check it, and two independent readers must return the same verdict
class on it — accept or refuse, for every record. `disagree=0` in any
cross-reader battery refers to this class only.

**State-relative truth** is a separate law, owned by the organism's
resume and by nothing else: an episode number no greater than the
state's; a signature's lived bytes within the state's; the `i` records
matching the island registry in count, order, digest, witness and
length; the chain and record count equal to the state's. A stateless
reader must accept a record that is canonical in shape but future in
value, and must say so in its scope; the organism must refuse the same
record on resume. That split is by construction and is not a
disagreement.

A reader without state also has no life identity: one supplied file is
treated as one life, and that scope is printed, not claimed. The
witness of identity is the state that acknowledges the biography, and
its authority is the organism's resume.

## Emit sites

The formats above are quoted from these `bio_append` producers in
`netta.c` (line numbers of the tree this document entered with):
u 435, b 453, t-eligible 1150, t-chart/t-earned 1201, r-7 1308,
q-null 1330, q-named/r-8 1396, m 1587, d 2463, a 2491, v-9/v-10 2558,
step 2616, s 4081, w 4118, i 4139. Any new emit site or arm amends
this document in the same commit, or the change is a defect.

## Canonical corpus

`scripts/biography_corpus/` carries the language's test data, shared
by every reader: `canonical.rows` holds at least one harvested or
format-derived row of every type and every arm (13 types, 19 shapes);
`<type>_malformed.rows` hold, for each type, rows that any reader must
refuse — separator faults (a space for a tab, a tab run, a space
beside a saturated field), sign and leading-zero faults, hex case and
width faults, wrong literals and enums, arity faults (a field too many
and too few), and non-canonical fix6 renderings. Rows in these files
are data; they carry no authority over any reader's implementation.
