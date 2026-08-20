#!/bin/sh
set -eu

ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
T=$(mktemp -d "${TMPDIR:-/tmp}/netta-mycelium-tests.XXXXXX")
MYC="$T/mycelium"
CHECK="$T/ledger_check"

pass() { printf 'PASS M1 %s\n' "$1"; }
fail() { printf 'FAIL M1 %s\n' "$1"; : >"$T/.failed"; }

expect_fail() {
    name=$1
    needle=$2
    shift 2
    if "$@" >"$T/out" 2>"$T/err"; then
        fail "$name (accepted)"
    elif grep -F "$needle" "$T/out" "$T/err" >/dev/null; then
        pass "$name"
    else
        fail "$name (wrong refusal: $(tr '\n' ' ' <"$T/err"))"
    fi
}

${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/mycelium.cpp" -o "$MYC"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/ledger_check.c" -o "$CHECK"
pass "both hands build strict and silent"

python3 - "$T" <<'PY'
import os, sys

T = sys.argv[1]
SEED = 0xcbf29ce484222325
PRIME = 0x100000001b3

def fnv(data, h=SEED):
    for byte in data:
        h ^= byte
        h = (h * PRIME) & 0xffffffffffffffff
    return h

def sig(data, seed=1):
    return (f"s\t1\t{fnv(data):016x}\t{len(data)}\t{seed}\tbi\t"
            "supported-backoff\t0000\t1000")

def case(name, data, line=None, newline=True):
    path = os.path.join(T, name)
    os.makedirs(path, exist_ok=True)
    open(os.path.join(path, "speech"), "wb").write(data)
    if line is None:
        line = sig(data)
    open(os.path.join(path, "bio"), "wb").write(
        line.encode() + (b"\n" if newline else b""))

sea_lines = [
    "The patient ocean remembers a silver current beneath the winter harbor.",
    "A salt wind carries the lantern signal across the sleeping water.",
    "Every tidal archive keeps a different moon inside its moving surface.",
    "The quiet vessel follows a blue channel toward the northern lighthouse.",
    "Cold waves return the names that vanished beyond the distant reef.",
    "An amber compass listens while the deep horizon changes direction.",
    "The harbor bell answers the river where the old maps become uncertain.",
    "Under the midnight pier a patient current gathers broken constellations.",
    "The coastal memory bends around each island and preserves its weather.",
    "A final tide carries the silver message through the open estuary.",
]
engine_lines = [
    "The copper engine measures a changing voltage across the silent rotor.",
    "A patient circuit carries the control signal through the narrow chamber.",
    "Every steel bearing remembers the pressure of the previous rotation.",
    "The calibrated governor closes a valve before the hot turbine accelerates.",
    "An electric sensor follows the pulse along the insulated winding.",
    "The workshop archive records each failure beside the repaired assembly.",
    "A cooling manifold returns the measured heat toward the outer radiator.",
    "The brass instrument compares the current phase with the reference clock.",
    "Under steady load the mechanical relay preserves its exact switching order.",
    "A final diagnostic carries the voltage reading through the control cabinet.",
]
sea = ("\n\n".join(sea_lines) + "\n").encode()
engine = ("\n\n".join(engine_lines) + "\n").encode()
case("sea", sea, sig(sea, 11))
case("engine", engine, sig(engine, 12))

bad = f"s\tbogus\t{fnv(sea):016x}\t{len(sea)}\tbad\tbad\tbad\tbad\tbad"
case("bad-s", sea, bad)
case("no-lf", sea, sig(sea, 11), newline=False)

double = os.path.join(T, "double")
os.makedirs(double)
open(os.path.join(double, "speech"), "wb").write(sea)
line = sig(sea, 11)
open(os.path.join(double, "bio"), "wb").write((line + "\n" + line + "\n").encode())

vt = b"alpha bravo charlie delta echo foxtrot\n\v\none two three four five six seven\n"
case("ascii-vt", vt)

nul = (b"alpha\x00hidden memory speaks through the silent byte and continues forever. "
       b"another ordinary fragment carries enough visible tokens to enter the grave.")
case("nul", nul)
nul2 = (b"alpha\x00hidden memory speaks through the silent byte and changes slowly. "
        b"another ordinary fragment carries enough visible tokens to enter the grave.")
case("nul2", nul2, sig(nul2, 14))
case("tiny", b"x", sig(b"x", 15))

dup_a = (b"luminous archive remembers every winter river and carries the patient signal.\n\n") * 12
dup_b = (b"copper engine follows every summer road and measures the turning voltage.\n\n") * 12
case("dup-a", dup_a, sig(dup_a, 21))
case("dup-b", dup_b, sig(dup_b, 22))

orphan = os.path.join(T, "orphan", ".mycelium.grave")
os.makedirs(orphan)
open(os.path.join(orphan, f"{fnv(sea):016x}"), "wb").write(sea)

rent = os.path.join(T, "renter")
os.makedirs(rent)
bio = []
for i in range(1, 20):
    if i in (1, 2, 19):
        text = (f"The crimson lantern flickers beside the granite tower on evening {i:02d}. "
                "Another calm sentence carries enough plain tokens for the grave tonight.")
    elif i == 17:
        text = ("The crimson lantern glows beside the granite tower on evening 17. "
                "Another calm sentence carries enough plain tokens for the grave tonight.")
    else:
        text = (f"A numbered courier route {i:02d} delivers the evening ledger across town. "
                "Another calm sentence carries enough plain tokens for the grave tonight.")
    data = text.encode()
    open(os.path.join(rent, f"speech-{i}"), "wb").write(data)
    bio.append(sig(data, 40 + i))
open(os.path.join(rent, "bio"), "wb").write(("\n".join(bio) + "\n").encode())

def room_meals(name, texts, seed0):
    path = os.path.join(T, name)
    os.makedirs(path)
    rows = []
    for i, t in enumerate(texts, 1):
        data = t.encode()
        open(os.path.join(path, f"speech-{i}"), "wb").write(data)
        rows.append(sig(data, seed0 + i))
    open(os.path.join(path, "bio"), "wb").write(("\n".join(rows) + "\n").encode())

school = []
for i in range(1, 19):
    if i <= 2:
        school.append(f"The quiet raven counts the copper coins tonight before dawn {i:02d}.")
    else:
        school.append(f"The quiet raven counts the copper coins on evening {i:02d}. "
                      f"The quiet raven counts the silver ledger on evening {i:02d} once more.")
room_meals("school-field", school, 70)

glory = []
for i in range(1, 11):
    if i <= 2:
        glory.append(f"The amber wolf sings beside the frozen river tonight {i:02d}. "
                     f"The amber wolf sings beside the silver bridge tonight {i:02d} as well.")
    else:
        glory.append(f"A numbered courier route {i:02d} delivers the quiet evening ledger across town.")
room_meals("glory-field", glory, 90)

opposite = []
for i in range(1, 19):
    if i <= 2:
        opposite.append(f"The quiet raven counts the copper coins before dawn {i:02d}.")
    elif i <= 10:
        opposite.append(f"The quiet raven counts the copper coins on evening {i:02d}. "
                        f"The quiet raven counts the silver ledger on evening {i:02d} once more.")
    else:
        opposite.append(f"The quiet raven counts the copper coins on evening {i:02d}. "
                        f"The quiet raven sings beside the silver ledger on evening {i:02d} once more.")
room_meals("opposite-field", opposite, 120)

carry = [
    "The steady signal returns beside the copper archive before dawn. "
    "Another steady sentence gives the repeated meal enough plain tokens."
] * 10
room_meals("carry-field", carry, 150)
PY

mkdir "$T/honest"
(
    cd "$T/honest"
    "$MYC" ingest sea "$T/sea/speech" "$T/sea/bio" >ingest-sea.out
    "$MYC" ingest engine "$T/engine/speech" "$T/engine/bio" >ingest-engine.out
    "$MYC" unfold "patient memory follows the changing signal" >unfold.out
    "$CHECK" >check.out
    "$MYC" ablate >ablate.out
)
grep -F "resonance beats the hash control on A and B" "$T/honest/ablate.out" >/dev/null &&
    pass "held-out resonance beats its sealed absence" || fail "honest ablation"
grep -F "(V 1, G 2, U 1)" "$T/honest/check.out" >/dev/null &&
    pass "the independent hand accepts the replayable ledger" || fail "honest ledger"

mkdir "$T/bad-field"
(
    cd "$T/bad-field"
    expect_fail "a malformed s-shaped row is not food" "no canonical newline-sealed s event" \
        "$MYC" ingest bad "$T/bad-s/speech" "$T/bad-s/bio"
)
mkdir "$T/nolf-field"
(
    cd "$T/nolf-field"
    expect_fail "an unsealed s row is not a biography event" "no canonical newline-sealed s event" \
        "$MYC" ingest nolf "$T/no-lf/speech" "$T/no-lf/bio"
)

mkdir "$T/double-field"
(
    cd "$T/double-field"
    "$MYC" ingest twin "$T/double/speech" "$T/double/bio" >one.out
    "$MYC" ingest twin "$T/double/speech" "$T/double/bio" >two.out
    grep -F "s#2" two.out >/dev/null && pass "a later matching s event is a second witness" ||
        fail "second witness"
    expect_fail "all eaten witnesses refuse by name" "already eaten witness" \
        "$MYC" ingest twin "$T/double/speech" "$T/double/bio"
    "$CHECK" >check.out
)

mkdir "$T/vt-field"
(
    cd "$T/vt-field"
    "$MYC" ingest ascii "$T/ascii-vt/speech" "$T/ascii-vt/bio" >out
    grep -F "2 fragments, 13 tokens" out >/dev/null &&
        pass "ASCII vertical-tab block splitting matches Python" || fail "ASCII parity"
)

(
    cd "$T/orphan"
    "$MYC" field >out 2>err
    grep -F "orphan grave blob" err >/dev/null &&
        pass "a blob-before-ledger crash leaves a loud powerless orphan" || fail "orphan report"
)

mkdir "$T/dup-field"
(
    cd "$T/dup-field"
    "$MYC" ingest a "$T/dup-a/speech" "$T/dup-a/bio" >/dev/null
    "$MYC" ingest b "$T/dup-b/speech" "$T/dup-b/bio" >/dev/null
    expect_fail "exact prompt twins cannot sit the ablation exam" "ABLATION FAILED" "$MYC" ablate
)

mkdir "$T/nul-field"
(
    cd "$T/nul-field"
    "$MYC" ingest nul "$T/nul/speech" "$T/nul/bio" >/dev/null
    "$MYC" unfold alpha >out
    python3 - out <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
raise SystemExit(0 if data.count(b"\0") >= 2 else 1)
PY
    pass "NUL survives corpse and lineage output"
    "$CHECK" >/dev/null
)

python3 - "$T/honest/.mycelium.ledger" "$T" <<'PY'
import os, shutil, sys

ledger, root = sys.argv[1:]
SEED = 0xcbf29ce484222325
PRIME = 0x100000001b3

def fnv(data, h):
    for byte in data:
        h ^= byte
        h = (h * PRIME) & 0xffffffffffffffff
    return h

def seal(payloads):
    out, h = bytearray(), SEED
    for payload in payloads:
        h = fnv(payload, h)
        out += payload + b"\t" + f"{h:016x}".encode() + b"\n"
    return bytes(out)

raw = open(ledger, "rb").read()
payloads = [line[:-17] for line in raw.splitlines()]
for name in ("mut-label", "mut-u", "mut-s", "mut-usort", "prefix", "missing",
             "unsealed"):
    path = os.path.join(root, name)
    os.makedirs(path)
    if name != "missing":
        shutil.copytree(os.path.join(root, "honest", ".mycelium.grave"),
                        os.path.join(path, ".mycelium.grave"))
    else:
        os.makedirs(os.path.join(path, ".mycelium.grave"))

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"G\tsea\t"):
        fields = row.split(b"\t")
        fields[1] = b"SEA"
        p[i] = b"\t".join(fields)
        break
open(os.path.join(root, "mut-label", ".mycelium.ledger"), "wb").write(seal(p))

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"U\t"):
        fields = row.split(b"\t")
        fields[3] = b"0"
        p[i] = b"\t".join(fields)
open(os.path.join(root, "mut-u", ".mycelium.ledger"), "wb").write(seal(p))

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"G\t"):
        p[i] = row.replace(b"\tsupported-backoff\t", b"\txxxxxxxxxxxxxxxxx\t", 1)
        break
open(os.path.join(root, "mut-s", ".mycelium.ledger"), "wb").write(seal(p))

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"U\t"):
        fields = row.split(b"\t")
        parts = fields[5].split(b",")
        assert len(parts) >= 2, "mut-usort needs a two-source U receipt"
        parts.reverse()
        fields[5] = b",".join(parts)
        p[i] = b"\t".join(fields)
        break
open(os.path.join(root, "mut-usort", ".mycelium.ledger"), "wb").write(seal(p))

lines = raw.splitlines(keepends=True)
open(os.path.join(root, "prefix", ".mycelium.ledger"), "wb").write(b"".join(lines[:2]))
open(os.path.join(root, "missing", ".mycelium.ledger"), "wb").write(raw)
open(os.path.join(root, "unsealed", ".mycelium.ledger"), "wb").write(raw[:-1])
PY

for spec in "mut-label:G field grammar:G label" \
            "mut-u:U field grammar:U k" \
            "mut-s:attested s line:attested s line" \
            "mut-usort:canonical order:U sources" \
            "missing:grave blob:grave blob" \
            "unsealed:unsealed:unsealed"; do
    name=${spec%%:*}; rest=${spec#*:}; writer=${rest%%:*}; reader=${rest#*:}
    (
        cd "$T/$name"
        expect_fail "$name writer refusal" "$writer" "$MYC" field
        expect_fail "$name reader refusal" "$reader" "$CHECK"
    )
done
pass "writer and independent reader share the crafted ledger language"

(
    cd "$T/prefix"
    "$MYC" field >field.out 2>field.err
    "$CHECK" >check.out
    grep -F "1 attested witnesses" field.out >/dev/null &&
        grep -F "orphan grave blob" field.err >/dev/null &&
        pass "a boundary prefix is self-consistent and later blobs are orphans" ||
        fail "boundary prefix"
)

for n in one two; do
    mkdir "$T/determinism-$n"
    (
        cd "$T/determinism-$n"
        "$MYC" ingest sea "$T/sea/speech" "$T/sea/bio"
        "$MYC" ingest engine "$T/engine/speech" "$T/engine/bio"
        "$MYC" unfold "patient memory follows the changing signal"
        "$MYC" propose
    ) >"$T/determinism-$n/stdout" 2>"$T/determinism-$n/stderr"
done
if cmp "$T/determinism-one/.mycelium.ledger" "$T/determinism-two/.mycelium.ledger" &&
        diff -r "$T/determinism-one/.mycelium.grave" "$T/determinism-two/.mycelium.grave" &&
        cmp "$T/determinism-one/.mycelium.proposals" "$T/determinism-two/.mycelium.proposals" &&
        cmp "$T/determinism-one/stdout" "$T/determinism-two/stdout"; then
    pass "clean-room grave, ledger and stdout are byte-identical"
else
    fail "clean-room determinism"
fi

# ---- body 2: the proposer ----
cp -R "$T/honest" "$T/no-props"
(
    cd "$T/honest"
    "$MYC" propose >propose-one.out
    "$MYC" propose >propose-two.out
    "$CHECK" >check-props.out
)
grep -F "(W 1, P 2)" "$T/honest/check-props.out" >/dev/null &&
    pass "the proposals chain is read by the independent hand" || fail "props chain read"
d1=$(grep -o 'snapshot [0-9a-f]*' "$T/honest/propose-one.out")
d2=$(grep -o 'snapshot [0-9a-f]*' "$T/honest/propose-two.out")
[ -n "$d1" ] && [ "$d1" = "$d2" ] &&
    pass "the snapshot digest is a pure function of the field" || fail "snapshot purity"

(
    cd "$T/honest"
    "$MYC" unfold "patient memory follows the changing signal" >unfold-after.out
    "$MYC" ablate >ablate-after.out
)
(
    cd "$T/no-props"
    "$MYC" unfold "patient memory follows the changing signal" >unfold-after.out
    "$MYC" ablate >ablate-after.out
)
cmp "$T/honest/unfold-after.out" "$T/no-props/unfold-after.out" &&
    pass "proposals grant no authority to unfold" || fail "unfold authority leak"
cmp "$T/honest/ablate-after.out" "$T/no-props/ablate-after.out" &&
    pass "proposals grant no authority to the ablation" || fail "ablate authority leak"

mkdir "$T/echo-field"
(
    cd "$T/echo-field"
    "$MYC" ingest echo "$T/dup-a/speech" "$T/dup-a/bio" >/dev/null
    "$MYC" propose >out
    grep -F "(nothing recurs across meals yet)" out >/dev/null &&
        pass "a one-meal echo proposes nothing" || fail "one-meal echo"
)

mkdir "$T/tiny-field"
(
    cd "$T/tiny-field"
    "$MYC" ingest tiny "$T/tiny/speech" "$T/tiny/bio" >/dev/null
    "$MYC" propose >out
    grep -F "(nothing recurs across meals yet)" out >/dev/null &&
        pass "a valid meal without fragments seals an empty snapshot" ||
        fail "empty-fragment meal"
    "$CHECK" >/dev/null
)

mkdir "$T/nul-props-field"
(
    cd "$T/nul-props-field"
    "$MYC" ingest nul-one "$T/nul/speech" "$T/nul/bio" >/dev/null
    "$MYC" ingest nul-two "$T/nul2/speech" "$T/nul2/bio" >/dev/null
    "$MYC" propose >out
    python3 - out <<'PY'
import sys
data = open(sys.argv[1], "rb").read()
raise SystemExit(0 if b"alive alpha\0hidden memory" in data else 1)
PY
    pass "NUL survives a proposed shape and its snapshot digest"
    "$CHECK" >/dev/null
)

mkdir "$T/rent-field"
(
    cd "$T/rent-field"
    i=1
    while [ $i -le 17 ]; do
        "$MYC" ingest renter "$T/renter/speech-$i" "$T/renter/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" propose >rent17.out
    grep -F "alive crimson lantern flickers support=2 meals=1,2" rent17.out >/dev/null &&
        pass "rent keeps a shape alive fifteen meals after its witness" ||
        fail "rent living edge"
    "$MYC" ingest renter "$T/renter/speech-18" "$T/renter/bio" >/dev/null
    "$MYC" propose >rent18.out
    grep -F "dead crimson lantern flickers support=2 meals=1,2 last=2" rent18.out >/dev/null &&
        pass "rent starves an unwitnessed shape after sixteen meals" || fail "rent death"
    grep -F "alive crimson lantern support=3 meals=1,2,17" rent18.out >/dev/null &&
        grep -F "dead crimson lantern flickers support=2 meals=1,2 last=2" rent18.out >/dev/null &&
        pass "pair survival and triple death are independent proposals" ||
        fail "pair and triple independence"
    "$MYC" ingest renter "$T/renter/speech-19" "$T/renter/bio" >/dev/null
    "$MYC" propose >rent19.out
    grep -F "alive crimson lantern flickers support=3 meals=1,2,19" rent19.out >/dev/null &&
        pass "a starved identity resurrects with its history grown" || fail "resurrection"
)

python3 - "$T" <<'PY'
import os, shutil, sys

root = sys.argv[1]
SEED = 0xcbf29ce484222325
PRIME = 0x100000001b3

def fnv(data, h):
    for byte in data:
        h ^= byte
        h = (h * PRIME) & 0xffffffffffffffff
    return h

def seal(payloads):
    out, h = bytearray(), SEED
    for payload in payloads:
        h = fnv(payload, h)
        out += payload + b"\t" + f"{h:016x}".encode() + b"\n"
    return bytes(out)

honest = os.path.join(root, "honest")
raw = open(os.path.join(honest, ".mycelium.proposals"), "rb").read()
payloads = [line[:-17] for line in raw.splitlines()]
names = (
    "props-flip", "props-trunc", "props-mono", "props-empty",
    "props-law", "props-before-w", "props-arity", "props-canon",
    "props-future", "props-main-prefix", "props-prefix-w", "props-prefix-p",
    "props-snapshot",
)
for name in names:
    path = os.path.join(root, name)
    os.makedirs(path)
    shutil.copytree(os.path.join(honest, ".mycelium.grave"),
                    os.path.join(path, ".mycelium.grave"))
    shutil.copy(os.path.join(honest, ".mycelium.ledger"),
                os.path.join(path, ".mycelium.ledger"))

flipped = bytearray(raw)
flipped[3] ^= 1
open(os.path.join(root, "props-flip", ".mycelium.proposals"), "wb").write(bytes(flipped))
open(os.path.join(root, "props-trunc", ".mycelium.proposals"), "wb").write(raw[:-1])

last_p = [p for p in payloads if p.startswith(b"P\t")][-1]
fields = last_p.split(b"\t")
fields[1] = b"1"
main_lines = open(os.path.join(honest, ".mycelium.ledger"), "rb").read().splitlines()
meals = 0
chain_at_one = None
for line in main_lines:
    payload = line[:-17]
    if payload.startswith(b"G\t"):
        meals += 1
    if meals == 1 and chain_at_one is None:
        chain_at_one = line[-16:]
assert chain_at_one is not None
fields[2] = chain_at_one
open(os.path.join(root, "props-mono", ".mycelium.proposals"), "wb").write(
    seal(payloads + [b"\t".join(fields)]))

open(os.path.join(root, "props-empty", ".mycelium.proposals"), "wb").write(b"")

p = payloads.copy()
law = p[0].split(b"\t")
law[2] = b"body2-props-v2"
p[0] = b"\t".join(law)
open(os.path.join(root, "props-law", ".mycelium.proposals"), "wb").write(seal(p))

open(os.path.join(root, "props-before-w", ".mycelium.proposals"), "wb").write(
    seal(payloads[1:]))

short = payloads[1].split(b"\t")[:-1]
open(os.path.join(root, "props-arity", ".mycelium.proposals"), "wb").write(
    seal([payloads[0], b"\t".join(short)]))

noncanon = payloads[1].split(b"\t")
noncanon[1] = b"02"
open(os.path.join(root, "props-canon", ".mycelium.proposals"), "wb").write(
    seal([payloads[0], b"\t".join(noncanon)]))

snapshot = payloads[1].split(b"\t")
snapshot[3] = str(int(snapshot[3]) + 1).encode()
open(os.path.join(root, "props-snapshot", ".mycelium.proposals"), "wb").write(
    seal([payloads[0], b"\t".join(snapshot)]))

future = payloads[-1].split(b"\t")
future[1] = b"999"
future[2] = b"0000000000000000"
future_raw = seal(payloads + [b"\t".join(future)])
future_dir = os.path.join(root, "props-future")
open(os.path.join(future_dir, ".mycelium.proposals"), "wb").write(future_raw)
open(os.path.join(future_dir, "proposals.before"), "wb").write(future_raw)

main_prefix = os.path.join(root, "props-main-prefix")
open(os.path.join(main_prefix, ".mycelium.ledger"), "wb").write(
    b"\n".join(main_lines[:2]) + b"\n")
open(os.path.join(main_prefix, ".mycelium.proposals"), "wb").write(
    b"\n".join(raw.splitlines()[:2]) + b"\n")

open(os.path.join(root, "props-prefix-w", ".mycelium.proposals"), "wb").write(
    b"\n".join(raw.splitlines()[:1]) + b"\n")
open(os.path.join(root, "props-prefix-p", ".mycelium.proposals"), "wb").write(
    b"\n".join(raw.splitlines()[:2]) + b"\n")
PY

for spec in "props-flip:proposals chain broken:chain does not fold" \
            "props-trunc:unsealed:unsealed" \
            "props-mono:not monotonic:not monotonic" \
            "props-empty:no law record:no law record" \
            "props-law:props law record:props law record" \
            "props-before-w:P before props law record:P before props law record" \
            "props-arity:P arity:P arity" \
            "props-canon:P field grammar:P after-meals" \
            "props-future:P main prefix:P main prefix" \
            "props-main-prefix:P main prefix:P main prefix"; do
    name=${spec%%:*}; rest=${spec#*:}; writer=${rest%%:*}; reader=${rest#*:}
    (
        cd "$T/$name"
        expect_fail "$name writer refusal" "$writer" "$MYC" propose
        expect_fail "$name reader refusal" "$reader" "$CHECK"
    )
done

cmp "$T/props-future/.mycelium.proposals" "$T/props-future/proposals.before" &&
    pass "a foreign future prefix refuses before the writer appends" ||
    fail "future prefix append"

(
    cd "$T/props-snapshot"
    expect_fail "the examiner re-derives a historical proposal snapshot" \
        "P snapshot drifted from the main prefix" "$MYC" propose
)

for name in props-prefix-w props-prefix-p; do
    (
        cd "$T/$name"
        "$CHECK" >/dev/null
        "$MYC" propose >/dev/null
        "$CHECK" >/dev/null
    )
done
pass "W-only and complete-P record-boundary prefixes resume honestly"

# ---- body 3: the school ----
mkdir "$T/school"
(
    cd "$T/school"
    i=1
    while [ $i -le 2 ]; do
        "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" enroll 3 quiet raven counts >enroll1.out
    grep -F 'enrolled hyp 1 arity 3' enroll1.out >/dev/null &&
        pass "an alive proposal enrolls as a hypothesis" || fail "enroll"
    expect_fail "a twice-enrolled shape refuses by name" "already-enrolled" \
        "$MYC" enroll 3 quiet raven counts
    expect_fail "an unproposed shape refuses by name" "not-proposed" \
        "$MYC" enroll 2 velvet moon
    i=3
    while [ $i -le 5 ]; do
        "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex1.out
    [ "$(grep -c "$(printf 'V\t')" .mycelium.school)" -eq 0 ] &&
        pass "no verdict before the window closes" || fail "early verdict"
    i=6
    while [ $i -le 10 ]; do
        "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex2.out
    grep -F 'verdict: hyp 1 pass' ex2.out >/dev/null &&
        grep -F 'glyph 1 minted for hyp 1' ex2.out >/dev/null &&
        pass "a recurring shape earns its glyph on the future stream" || fail "honest pass"
    expect_fail "a legalised shape refuses re-enrollment" "already-legalised" \
        "$MYC" enroll 3 quiet raven counts
    "$MYC" enroll 2 quiet raven >enroll2.out
    i=11
    while [ $i -le 18 ]; do
        "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex3.out
    grep -F 'verdict: hyp 2 fail' ex3.out >/dev/null &&
        pass "the legalised triple starves its own pair" || fail "marginal law"
    "$CHECK" >check.out
    grep -F '(S 1, H 2, R 3, O 16, V 2, L 1)' check.out >/dev/null &&
        pass "the school chain is read by the independent hand" ||
        fail "school counts: $(tail -2 check.out | tr '\n' ' ')"
)

mkdir "$T/glory-room"
(
    cd "$T/glory-room"
    i=1
    while [ $i -le 2 ]; do
        "$MYC" ingest wolf "$T/glory-field/speech-$i" "$T/glory-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" enroll 3 amber wolf sings >/dev/null
    i=3
    while [ $i -le 10 ]; do
        "$MYC" ingest wolf "$T/glory-field/speech-$i" "$T/glory-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex.out
    grep -F 'verdict: hyp 1 fail' ex.out >/dev/null &&
        pass "past glory buys nothing after the bell" || fail "past glory"
)

(
    cd "$T/nul-props-field"
    "$MYC" enroll-hex 2 616c7068610068696464656e 6d656d6f7279 >enroll-hex.out
    python3 - .mycelium.school <<'PY'
import sys
raw = open(sys.argv[1], "rb").read()
raise SystemExit(0 if b"H\t1\t2\t19\talpha\0hidden memory\t" in raw else 1)
PY
    "$CHECK" >/dev/null
    pass "a NUL-bearing proposal enrolls through the canonical hex lane"
)

mkdir "$T/carry-room"
(
    cd "$T/carry-room"
    i=1
    while [ $i -le 2 ]; do
        "$MYC" ingest carrier "$T/carry-field/speech-$i" "$T/carry-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" enroll 3 steady signal returns >/dev/null
    while [ $i -le 10 ]; do
        "$MYC" ingest carrier "$T/carry-field/speech-$i" "$T/carry-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex.out
    costs=$(awk '/^  O hyp 1/ { print $9 ":" $11 }' ex.out | sort -u | wc -l | tr -d ' ')
    [ "$costs" -gt 1 ] &&
        pass "Laplace state is carried across byte-identical meals" ||
        fail "school reset its pricing state per meal"
)

mkdir "$T/opposite-room"
(
    cd "$T/opposite-room"
    i=1
    while [ $i -le 2 ]; do
        "$MYC" ingest scholar "$T/opposite-field/speech-$i" "$T/opposite-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" enroll 3 quiet raven counts >/dev/null
    while [ $i -le 10 ]; do
        "$MYC" ingest scholar "$T/opposite-field/speech-$i" "$T/opposite-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >/dev/null
    "$MYC" enroll 2 quiet raven >/dev/null
    while [ $i -le 18 ]; do
        "$MYC" ingest scholar "$T/opposite-field/speech-$i" "$T/opposite-field/bio" >/dev/null
        i=$((i + 1))
    done
    "$MYC" examine >ex.out
    grep -F 'verdict: hyp 2 pass' ex.out >/dev/null &&
        grep -F 'glyph 2 minted for hyp 2' ex.out >/dev/null &&
        pass "a pair outside its legalised triple earns uncovered marginal gain" ||
        fail "opposite marginal case"
)

python3 - "$T" <<'PY'
import os, shutil, sys

root = sys.argv[1]
SEED = 0xcbf29ce484222325
PRIME = 0x100000001b3

def fnv(data, h):
    for byte in data:
        h ^= byte
        h = (h * PRIME) & 0xffffffffffffffff
    return h

def seal(payloads):
    out, h = bytearray(), SEED
    for payload in payloads:
        h = fnv(payload, h)
        out += payload + b"\t" + f"{h:016x}".encode() + b"\n"
    return bytes(out)

school = os.path.join(root, "school")
raw = open(os.path.join(school, ".mycelium.school"), "rb").read()
payloads = [line[:-17] for line in raw.splitlines()]
names = (
    "school-flip", "school-trunc", "school-vtot", "school-prefix",
    "school-shape", "school-h-len", "school-h-proposal", "school-o-order",
    "school-o-unarrived", "school-v-early", "school-l-missing",
    "school-l-delayed", "school-l-second", "school-overflow",
    "school-verdict-wrap", "school-r-prefix", "school-r-reason", "school-law",
)
for name in names:
    path = os.path.join(root, name)
    os.makedirs(path)
    shutil.copytree(os.path.join(school, ".mycelium.grave"),
                    os.path.join(path, ".mycelium.grave"))
    shutil.copy(os.path.join(school, ".mycelium.ledger"),
                os.path.join(path, ".mycelium.ledger"))

flipped = bytearray(raw)
flipped[3] ^= 1
open(os.path.join(root, "school-flip", ".mycelium.school"), "wb").write(bytes(flipped))
open(os.path.join(root, "school-trunc", ".mycelium.school"), "wb").write(raw[:-1])

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"V\t"):
        fields = row.split(b"\t")
        fields[3] = str(int(fields[3]) + 1).encode()
        p[i] = b"\t".join(fields)
        break
open(os.path.join(root, "school-vtot", ".mycelium.school"), "wb").write(seal(p))

h1 = next(i for i, row in enumerate(payloads) if row.startswith(b"H\t1\t"))
o1 = [i for i, row in enumerate(payloads) if row.startswith(b"O\t1\t")]
v1 = next(i for i, row in enumerate(payloads) if row.startswith(b"V\t1\t"))
l1 = next(i for i, row in enumerate(payloads) if row.startswith(b"L\t1\t"))
r_open = next(i for i, row in enumerate(payloads)
              if row.startswith(b"R\talready-enrolled\t"))
r_legal = next(i for i, row in enumerate(payloads)
               if row.startswith(b"R\talready-legalised\t"))

def write(name, rows):
    open(os.path.join(root, name, ".mycelium.school"), "wb").write(seal(rows))

p = payloads.copy()
f = p[h1].split(b"\t"); f[6] = b"0000000000000000"; p[h1] = b"\t".join(f)
write("school-prefix", p)

f = payloads[h1].split(b"\t")
f[3] = b"19"; f[4] = b"quiet  raven counts"
write("school-shape", [payloads[0], b"\t".join(f)])

f = payloads[h1].split(b"\t"); f[3] = b"17"
write("school-h-len", [payloads[0], b"\t".join(f)])

f = payloads[h1].split(b"\t")
f[2] = b"2"; f[3] = b"11"; f[4] = b"velvet moon"
write("school-h-proposal", [payloads[0], b"\t".join(f)])

p = payloads.copy(); p[o1[0]], p[o1[1]] = p[o1[1]], p[o1[0]]
write("school-o-order", p)

write("school-o-unarrived", payloads[:v1 + 1])
main = open(os.path.join(root, "school-o-unarrived", ".mycelium.ledger"), "rb").read().splitlines()
cut, meals = [], 0
for row in main:
    cut.append(row)
    if row[:-17].startswith(b"G\t"):
        meals += 1
    if meals == 2:
        break
open(os.path.join(root, "school-o-unarrived", ".mycelium.ledger"), "wb").write(
    b"\n".join(cut) + b"\n")

v = payloads[v1].split(b"\t")
first_o = payloads[o1[0]].split(b"\t")
v[2] = b"pass"; v[3] = first_o[4]; v[4] = first_o[5]
write("school-v-early", [payloads[0], payloads[h1], payloads[o1[0]], b"\t".join(v)])

write("school-l-missing", payloads[:v1 + 1])
p = payloads.copy(); p[l1], p[r_legal] = p[r_legal], p[l1]
write("school-l-delayed", p)
p = payloads.copy(); p.insert(l1 + 1, payloads[l1])
write("school-l-second", p)

zero_o = []
for pos, idx in enumerate(o1):
    f = payloads[idx].split(b"\t")
    f[4] = b"18446744073709551615" if pos == 0 else b"1" if pos == 1 else b"0"
    f[5] = b"0"
    zero_o.append(b"\t".join(f))
write("school-overflow", [payloads[0], payloads[h1]] + zero_o)

wrap_o = []
for pos, idx in enumerate(o1):
    f = payloads[idx].split(b"\t")
    f[4] = b"8000000" if pos == len(o1) - 1 else b"0"
    f[5] = b"18446744073709551615" if pos == len(o1) - 1 else b"0"
    wrap_o.append(b"\t".join(f))
v = payloads[v1].split(b"\t")
v[2] = b"pass"; v[3] = b"8000000"; v[4] = b"18446744073709551615"
write("school-verdict-wrap", [payloads[0], payloads[h1]] + wrap_o +
      [b"\t".join(v), payloads[l1]])

p = payloads.copy(); f = p[r_open].split(b"\t"); f[5] = b"0000000000000000"; p[r_open] = b"\t".join(f)
write("school-r-prefix", p)
p = payloads.copy(); f = p[r_open].split(b"\t"); f[1] = b"already-legalised"; p[r_open] = b"\t".join(f)
write("school-r-reason", p)
p = payloads.copy(); f = p[0].split(b"\t"); f[2] = b"body3-school-v1"; p[0] = b"\t".join(f)
write("school-law", p)
PY

for spec in "school-flip:school chain broken:chain does not fold" \
            "school-trunc:unsealed:unsealed" \
            "school-vtot:V totals:V totals" \
            "school-prefix:H main prefix:H main prefix" \
            "school-shape:H shape is not canonical:H shape is not canonical" \
            "school-h-len:H field grammar:H shape length" \
            "school-o-order:O outside the window's order:O outside the window's order" \
            "school-o-unarrived:O prices an unarrived meal:O prices an unarrived meal" \
            "school-v-early:V before the window closed:V before the window closed" \
            "school-l-delayed:must be legalised immediately:must be legalised immediately" \
            "school-l-second:L without a pass:L glyph is not sequential" \
            "school-r-prefix:R main prefix:R main prefix" \
            "school-r-reason:R reason does not match:R reason does not match" \
            "school-law:school law record:school law record"; do
    name=${spec%%:*}; rest=${spec#*:}; writer=${rest%%:*}; reader=${rest#*:}
    (
        cd "$T/$name"
        expect_fail "$name writer refusal" "$writer" "$MYC" examine
        expect_fail "$name reader refusal" "$reader" "$CHECK"
    )
done

(
    cd "$T/school-l-missing"
    "$CHECK" >/dev/null
    "$MYC" examine >recover.out
    grep -F 'recovered glyph 1 for hyp 1 after a V-boundary prefix' recover.out >/dev/null &&
        [ "$(grep -c "$(printf 'L\t1\t1\t')" .mycelium.school)" -eq 1 ] &&
        "$CHECK" >/dev/null &&
        pass "a sealed pass at EOF recovers its one mandatory L" ||
        fail "V-boundary recovery"
)

(
    cd "$T/school-h-proposal"
    expect_fail "the writer re-derives H proposal state at its pinned prefix" \
        "H shape was not an alive proposal" "$MYC" examine
)
(
    cd "$T/school-overflow"
    expect_fail "the reader refuses overflowing O totals" "O totals overflow" "$CHECK"
)
(
    cd "$T/school-verdict-wrap"
    expect_fail "the reader classifies gain without u64 wraparound" \
        "V verdict class" "$CHECK"
)

for n in one two; do
    mkdir "$T/school-det-$n"
    (
        cd "$T/school-det-$n"
        i=1
        while [ $i -le 2 ]; do
            "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio"
            i=$((i + 1))
        done
        "$MYC" enroll 3 quiet raven counts
        i=3
        while [ $i -le 10 ]; do
            "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio"
            i=$((i + 1))
        done
        "$MYC" examine
        "$MYC" enroll 2 quiet raven
        i=11
        while [ $i -le 18 ]; do
            "$MYC" ingest scholar "$T/school-field/speech-$i" "$T/school-field/bio"
            i=$((i + 1))
        done
        "$MYC" examine
    ) >"$T/school-det-$n/stdout" 2>"$T/school-det-$n/stderr"
done
if cmp "$T/school-det-one/.mycelium.school" "$T/school-det-two/.mycelium.school" &&
        cmp "$T/school-det-one/.mycelium.ledger" "$T/school-det-two/.mycelium.ledger" &&
        cmp "$T/school-det-one/stdout" "$T/school-det-two/stdout"; then
    pass "clean-room school chain and stdout are byte-identical"
else
    fail "school clean-room determinism"
fi

if [ ! -e "$T/.failed" ]; then
    printf '%s\n' '----' 'ALL MYCELIUM GATES PASS'
    exit 0
fi
printf '%s\n' '----' 'MYCELIUM GATES FAILED'
exit 1
