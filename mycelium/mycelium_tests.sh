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
    -I/opt/homebrew/include -L/opt/homebrew/lib \
    "$ROOT/mycelium/mycelium.cpp" -o "$MYC" -lnotorch -framework Accelerate
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

def room_meals_bytes(name, texts, seed0):
    path = os.path.join(T, name)
    os.makedirs(path)
    rows = []
    for i, data in enumerate(texts, 1):
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

mintf = []
for i in range(1, 28):
    if i <= 2:
        mintf.append(f"The amber wolf sings beside the frozen granite river tonight {i:02d}. "
                     f"A copper courier delivers the quiet town ledger across morning {i:02d}.")
    elif i <= 10:
        mintf.append(f"The amber wolf sings beside the frozen granite river on evening {i:02d}. "
                     f"The amber wolf sings beside the silver granite bridge on evening {i:02d} as well. "
                     f"A copper courier delivers the quiet town ledger across morning {i:02d}. "
                     f"Another copper courier carries the town parcel across evening {i:02d}.")
    elif i <= 26:
        mintf.append(f"A copper courier delivers the quiet town ledger across morning {i:02d}. "
                     f"Another copper courier carries the town parcel across evening {i:02d}.")
    else:
        mintf.append(f"The amber wolf sings beside the frozen granite river tonight {i:02d}. "
                     f"A copper courier delivers the quiet town ledger across morning {i:02d}.")
room_meals("mint-field", mintf, 260)

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

parl = []
for i in range(1, 35):
    if i <= 2:
        parl.append(f"The quiet raven counts the copper coins tonight before dawn {i:02d}.")
    elif i <= 18:
        parl.append(f"The quiet raven counts the copper coins on evening {i:02d}. "
                    f"The quiet raven counts the silver ledger on evening {i:02d} once more.")
    elif i <= 26:
        parl.append(f"The quiet raven counts the copper coins on evening {i:02d}. "
                    f"The quiet raven sings beside the silver ledger on evening {i:02d} once more.")
    else:
        parl.append(f"The quiet raven sings beside the copper archive on evening {i:02d}.")
room_meals("parl-field", parl, 180)

little_nul = []
for i in range(1, 11):
    little_nul.append(
        b"alpha\x00hidden memory speaks through the silent byte on evening " +
        f"{i:02d}".encode() +
        b". Another ordinary fragment carries enough visible tokens for court."
    )
room_meals_bytes("nul-parl-field", little_nul, 250)

long_a = "a" * 300
long_b = "b" * 300
long = []
for i in range(1, 11):
    long.append(f"{long_a} {long_b} remembers the sealed record on evening {i:02d}. "
                f"Another ordinary fragment carries enough visible tokens for court.")
room_meals("long-parl-field", long, 270)
open(os.path.join(T, "long-token-a"), "w").write(long_a)
open(os.path.join(T, "long-token-b"), "w").write(long_b)

parl2 = []
for i in range(1, 27):
    if i <= 2:
        parl2.append(f"The amber wolf sings beside the frozen river tonight {i:02d}.")
    elif i <= 10:
        parl2.append(f"The amber wolf sings beside the frozen river on evening {i:02d}. "
                     f"The amber wolf sings beside the silver bridge on evening {i:02d} as well.")
    else:
        parl2.append(f"A numbered courier route {i:02d} delivers the quiet evening ledger across town.")
room_meals("parl2-field", parl2, 220)

shape = "quiet raven counts".encode()
uid = fnv(bytes([3, 0]) + shape)
open(os.path.join(T, "parl-unit-id"), "w").write(f"{uid:016x}")
for arity in (2, 3):
    uid = fnv(bytes([arity, 0]) + shape)
    open(os.path.join(T, f"parl-arity{arity}-unit-id"), "w").write(f"{uid:016x}")
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

# ---- the court of three character laws (UNICODE_COURT.md) ----
COURT="$T/unicode_court"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/unicode_court.cpp" -o "$COURT"
pass "the court builds strict and silent"

check_decode() {
    got=$("$COURT" --decode "$1")
    [ "$got" = "$2" ] && pass "decoder: $3" || fail "decoder: $3"
}
NL='
'
check_decode c0af "cp:${NL}u8b: 1114304 1114287" "overlong stays tagged evidence only for u8b"
check_decode 80 "cp:${NL}u8b: 1114240" "a bare continuation byte"
check_decode e282 "cp:${NL}u8b: 1114338 1114242" "a truncated sequence"
check_decode eda080 "cp:${NL}u8b: 1114349 1114272 1114240" "a surrogate is not a character"
check_decode f48fbfbf "cp: 1114111${NL}u8b: 1114111" "the last code point decodes"
check_decode f4908080 "cp:${NL}u8b: 1114356 1114256 1114240 1114240" "beyond the last code point"
check_decode 41d790 "cp: 65 1488${NL}u8b: 65 1488" "ascii beside an aleph"
check_decode e9c3a9 "cp: 233${NL}u8b: 1114345 233" \
    "an invalid byte cannot impersonate the same-valued Unicode scalar"

"$COURT" "$T/sea/speech" "$T/engine/speech" "$T/sea/speech" "$T/engine/speech" \
    >"$T/court-ascii.out"
ascii_b=$(grep 'L-byte' "$T/court-ascii.out" | sed 's/L-[a-z0-9]* *//')
ascii_c=$(grep 'L-cp' "$T/court-ascii.out" | sed 's/L-[a-z0-9]* *//')
ascii_u=$(grep 'L-u8b' "$T/court-ascii.out" | sed 's/L-[a-z0-9]* *//')
[ -n "$ascii_b" ] && [ "$ascii_b" = "$ascii_u" ] && [ "$ascii_b" = "$ascii_c" ] &&
    pass "on pure ASCII the three laws are one law" || fail "ascii identity"

"$COURT" "$T/sea/speech" "$T/engine/speech" "$T/sea/speech" "$T/engine/speech" \
    >"$T/court-ascii2.out"
cmp -s "$T/court-ascii.out" "$T/court-ascii2.out" &&
    pass "the court is deterministic" || fail "court determinism"

if python3 - "$COURT" <<'PY'
import math, subprocess, sys

COURT = sys.argv[1]

# the prototype's embedding formula, reproduced from mycelium.py:31-59
# (cited in UNICODE_COURT.md); no sibling import, the formula IS the law.
def fnv_vec(s):
    h = 2166136261
    for c in s:
        h ^= ord(c)
        h = (h * 16777619) & 0xFFFFFFFF
    v = []
    for _ in range(96):
        h ^= h >> 13
        h = (h * 1597334677) & 0xFFFFFFFF
        h ^= h >> 16
        v.append((h & 0xFFFF) / 32768.0 - 1.0)
    return v

def embed(word):
    w = "^" + word + "$"
    grams = [w[i:i + 3] for i in range(len(w) - 2)] or [w]
    vec = [0.0] * 96
    for g in grams:
        gv = fnv_vec(g)
        for i in range(96):
            vec[i] += gv[i]
    n = math.sqrt(sum(x * x for x in vec)) + 1e-10
    return [x / n for x in vec]

for word in ("raven", "שלום", "cafè"):
    mine = " ".join(f"{x:.17g}" for x in embed(word))
    theirs = subprocess.run([COURT, "--embed-cp", word],
                            capture_output=True, text=True).stdout.strip()
    if mine != theirs:
        raise SystemExit(f"parity broke on {word!r}")
PY
then
    pass "L-cp matches the prototype's own embedding, bit for bit"
else
    fail "prototype parity"
fi

# ---- the organ court (ORGAN_COURT.md) ----
ORGAN="$T/organ_court"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/organ_court.cpp" -o "$ORGAN"
pass "the organ court builds strict and silent"

"$ORGAN" "$T/sea/speech" "$T/engine/speech" "$T/sea/speech" "$T/engine/speech" \
    >"$T/organ-ascii.out"
ob=$(grep 'L-byte' "$T/organ-ascii.out" | sed 's/L-[a-z0-9]* *//')
ou=$(grep 'L-u8b' "$T/organ-ascii.out" | sed 's/L-[a-z0-9]* *//')
[ -n "$ob" ] && [ "$ob" = "$ou" ] &&
    pass "the organ court's two arms are one arm on pure ASCII" ||
    fail "organ ascii identity"

"$ORGAN" "$T/sea/speech" "$T/engine/speech" "$T/sea/speech" "$T/engine/speech" \
    >"$T/organ-ascii2.out"
cmp -s "$T/organ-ascii.out" "$T/organ-ascii2.out" &&
    pass "the organ court is deterministic" || fail "organ determinism"

uc_att=$(grep '^\[en\] L-byte' "$T/court-ascii.out" | grep -o 'att=[0-9.]*')
oc_att=$(grep '^\[en\] L-byte' "$T/organ-ascii.out" | grep -o 'att-cap=[0-9.]*' |
    sed 's/att-cap/att/')
[ -n "$uc_att" ] && [ "$uc_att" = "$oc_att" ] &&
    pass "the consistency row binds both courts to one field" ||
    fail "court consistency: unicode '$uc_att' vs organ '$oc_att'"

# ---- the root court (ROOT_COURT.md) ----
ROOTC="$T/root_court"
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/root_court.cpp" -o "$ROOTC"
pass "the root court builds strict and silent"

"$ROOTC" --ascii >"$T/root-ascii.out"
rb=$(grep '^L-byte ' "$T/root-ascii.out" | sed 's/^L-byte //')
ru=$(grep '^L-u8b ' "$T/root-ascii.out" | sed 's/^L-u8b //')
[ -n "$rb" ] && [ "$rb" = "$ru" ] &&
    pass "the root court's two arms are one arm on pure ASCII" ||
    fail "root ascii identity"

t1=$("$ROOTC" --atoms e9)
t2=$("$ROOTC" --atoms c3a9)
NL='
'
[ "$t1" = "L-byte: 233${NL}L-u8b: 1114345" ] &&
    [ "$t2" = "L-byte: 195 169${NL}L-u8b: 233" ] &&
    pass "a tagged invalid byte cannot impersonate a lawful character" ||
    fail "tag domain: [$t1] [$t2]"

"$ROOTC" --ascii >"$T/root-ascii2.out"
cmp -s "$T/root-ascii.out" "$T/root-ascii2.out" &&
    pass "the root court is deterministic" || fail "root determinism"

expect_fail "a foreign corpus is refusal, not a partial court" "digest mismatch" \
    "$ROOTC" "$T/sea/speech"

expect_fail "the independent root hand refuses an incomplete report" \
    "unknown record" python3 "$ROOT/mycelium/root_court_check.py" \
    "$T/root-ascii.out"

# ---- body 4: the parliament (writer stage) ----
feed() { # $1 room dir, $2 field dir, $3 label, $4 from, $5 to
    i=$4
    while [ $i -le $5 ]; do
        (cd "$1" && "$MYC" ingest "$3" "$T/$2/speech-$i" "$T/$2/bio" >/dev/null)
        i=$((i + 1))
    done
}

mkdir "$T/nul-parl"
(
    cd "$T/nul-parl"
    feed "$T/nul-parl" nul-parl-field nul 1 2
    "$MYC" enroll-hex 2 616c7068610068696464656e 6d656d6f7279 >/dev/null
    feed "$T/nul-parl" nul-parl-field nul 3 10
    "$MYC" examine >/dev/null
    "$MYC" propose >/dev/null
    "$MYC" petition 1 >pet.out
    grep -F 'verdict: PASS' pet.out >/dev/null &&
        pass "a NUL-bearing rent shape earns a full PASS ballot" ||
        fail "NUL parliament ballot: $(cat pet.out | tr '\n' ' ')"
    if "$CHECK" >check.out 2>check.err; then
        pass "the independent hand accepts a NUL-bearing rent ballot"
    else
        fail "NUL parliament reader: $(cat check.err | tr '\n' ' ')"
    fi
)

mkdir "$T/long-parl"
(
    cd "$T/long-parl"
    long_a=$(cat "$T/long-token-a")
    long_b=$(cat "$T/long-token-b")
    feed "$T/long-parl" long-parl-field long 1 2
    "$MYC" enroll 2 "$long_a" "$long_b" >/dev/null
    feed "$T/long-parl" long-parl-field long 3 10
    "$MYC" examine >/dev/null
    "$MYC" propose >/dev/null
    "$MYC" petition 1 >pet.out
    grep -F 'verdict: PASS' pet.out >/dev/null &&
        pass "a 601-byte rent shape earns a full PASS ballot" ||
        fail "long parliament ballot: $(cat pet.out | tr '\n' ' ')"
    if "$CHECK" >check.out 2>check.err; then
        pass "the independent hand accepts a 601-byte rent ballot"
    else
        fail "long parliament reader: $(cat check.err | tr '\n' ' ')"
    fi
)

mkdir "$T/parl"
(
    cd "$T/parl"
    feed "$T/parl" parl-field raven 1 2
    "$MYC" enroll 3 quiet raven counts >/dev/null
    feed "$T/parl" parl-field raven 3 10
    "$MYC" examine >/dev/null
    "$MYC" propose >/dev/null
    "$MYC" petition 1 >pet1.out
    grep -F 'verdict: PASS' pet1.out >/dev/null &&
        grep -qF "unit $(cat "$T/parl-unit-id")" pet1.out &&
        pass "a strong clean alive citizen is admitted with its pinned unit id" ||
        fail "honest PASS: $(cat pet1.out | tr '\n' ' ')"
    expect_fail "an admitted identity refuses a second petition" "already admitted" \
        "$MYC" petition 1
    "$MYC" petition-opaque 1 root-v1 0123456789abcdef >dark.out
    grep -F 'verdict: DARK' dark.out >/dev/null &&
        grep -cF 'J ' dark.out | grep -q 3 &&
        pass "a foreign recognizer is heard and lands DARK" || fail "opaque lane"
    expect_fail "a repeated opaque identity refuses without a new law" \
        "no new preregistered law" \
        "$MYC" petition-opaque 1 root-v1 0123456789abcdef
    arity2_id=$(cat "$T/parl-arity2-unit-id")
    arity3_id=$(cat "$T/parl-arity3-unit-id")
    [ "$arity2_id" != "$arity3_id" ] ||
        fail "arity-tagged opaque unit ids collided"
    "$MYC" petition-opaque 1 root-arity-v1 "$arity2_id" >dark-arity2.out
    "$MYC" petition-opaque 1 root-arity-v1 "$arity3_id" >dark-arity3.out
    grep -F 'verdict: DARK' dark-arity2.out >/dev/null &&
        grep -F 'verdict: DARK' dark-arity3.out >/dev/null &&
        pass "opaque identities isolate the same glyph bytes by arity-tagged unit id" ||
        fail "opaque arity isolation"
    expect_fail "a repeated arity-tagged opaque identity still refuses" \
        "no new preregistered law" \
        "$MYC" petition-opaque 1 root-arity-v1 "$arity2_id"
    "$MYC" petition 9 >sil.out
    grep -F 'verdict: SILENCE' sil.out >/dev/null &&
        grep -F 'J exam unheard' sil.out >/dev/null &&
        pass "a missing glyph lands SILENCE with unheard receipts" || fail "silence"
    expect_fail "a silent glyph refuses until it exists" "still silent" \
        "$MYC" petition 9
    "$MYC" enroll 2 quiet raven >/dev/null
    feed "$T/parl" parl-field raven 11 18
    "$MYC" examine >/dev/null
    "$MYC" enroll 2 quiet raven >/dev/null
    feed "$T/parl" parl-field raven 19 26
    "$MYC" examine >ex2.out
    grep -F 'verdict: hyp 3 pass' ex2.out >/dev/null || fail "pair retake did not pass"
    "$MYC" petition 2 >scar.out
    grep -F 'J record scarred:18' scar.out >/dev/null &&
        grep -F 'verdict: SCAR' scar.out >/dev/null &&
        pass "a failed-then-passed identity is scarred, never cherry-picked" ||
        fail "scar: $(cat scar.out | tr '\n' ' ')"
    expect_fail "a hot scar refuses before its boundary" "scar holds until meal 34" \
        "$MYC" petition 2
    "$MYC" petition-opaque 2 root-scar-v1 0badc0ffee0ddf00 >dark-scar.out
    grep -F 'verdict: DARK' dark-scar.out >/dev/null &&
        pass "a hot known scar does not poison a foreign opaque identity" ||
        fail "scar isolation: $(cat dark-scar.out | tr '\n' ' ')"
    expect_fail "the known scar still refuses after foreign darkness" \
        "scar holds until meal 34" \
        "$MYC" petition 2
    feed "$T/parl" parl-field raven 27 34
    cp .mycelium.ledger "$T/parl-ledger.before"
    cp .mycelium.proposals "$T/parl-props.before"
    cp .mycelium.school "$T/parl-school.before"
    "$MYC" petition 2 >weak.out
    grep -F 'verdict: WEAKEN' weak.out >/dev/null &&
        pass "a cooled scar enters only through WEAKEN, never by forgetting" ||
        fail "cooled scar: $(cat weak.out | tr '\n' ' ')"
    "$MYC" petition-opaque 2 root-v9 fedcba9876543210 >/dev/null
    cmp -s .mycelium.ledger "$T/parl-ledger.before" &&
        cmp -s .mycelium.proposals "$T/parl-props.before" &&
        cmp -s .mycelium.school "$T/parl-school.before" &&
        pass "the parliament wrote nothing into the three old chains" ||
        fail "no-authority"
    "$MYC" franchise >fr.out
    grep -F 'citizen 1: glyph 1' fr.out >/dev/null &&
        grep -F 'WEAKEN' fr.out >/dev/null &&
        pass "the franchise lists both admissions deterministically" || fail "franchise"
    if ! grep -F 'DARK' fr.out >/dev/null && ! grep -F 'root-' fr.out >/dev/null; then
        pass "DARK ballots carry no franchise power"
    else
        fail "DARK leaked into franchise: $(cat fr.out | tr '\n' ' ')"
    fi
    "$CHECK" >check.out
    grep -F 'parliament:' check.out >/dev/null &&
        pass "the independent hand recounts every ballot from the pins" ||
        fail "reader parliament: $(tail -1 check.out)"
)

mkdir "$T/parl2"
(
    cd "$T/parl2"
    feed "$T/parl2" parl2-field wolf 1 2
    "$MYC" enroll 3 amber wolf sings >/dev/null
    feed "$T/parl2" parl2-field wolf 3 10
    "$MYC" examine >/dev/null
    feed "$T/parl2" parl2-field wolf 11 26
    "$MYC" petition 1 >fr.out
    grep -F 'J rent starved' fr.out >/dev/null &&
        grep -F 'verdict: FREEZE' fr.out >/dev/null &&
        pass "a starved citizen is frozen, not admitted" || fail "freeze"
    expect_fail "a frozen identity refuses inside its window" "frozen until meal 34" \
        "$MYC" petition 1
)

mkdir "$T/parl3"
(
    cd "$T/parl3"
    feed "$T/parl3" parl2-field wolf 1 2
    "$MYC" enroll 3 amber wolf sings >/dev/null
    feed "$T/parl3" parl2-field wolf 3 10
    "$MYC" examine >/dev/null
    feed "$T/parl3" parl2-field wolf 11 25
    "$MYC" petition 1 >/dev/null
    cp .mycelium.parliament "$T/parl3-ref"
)
for cut in 2 3 4; do
    rm -rf "$T/parl3-cut$cut" 2>/dev/null || true
    cp -R "$T/parl3" "$T/parl3-cut$cut"
    (
        cd "$T/parl3-cut$cut"
        head -n $cut "$T/parl3-ref" > .mycelium.parliament
        "$MYC" ingest wolf "$T/parl2-field/speech-26" "$T/parl2-field/bio" >/dev/null
        "$MYC" franchise >/dev/null 2>fr.err
        cmp -s .mycelium.parliament "$T/parl3-ref" &&
            pass "an interruption after line $cut recovers the exact ballot despite the advanced clock" ||
            fail "recovery cut $cut: $(cat fr.err | tr '\n' ' ')"
    )
done

cp -R "$T/parl3" "$T/parl3-partial-j"
(
    cd "$T/parl3-partial-j"
    python3 - "$T/parl3-ref" .mycelium.parliament <<'PY'
import sys
raw = open(sys.argv[1], "rb").read()
lines = raw.splitlines(keepends=True)
assert len(lines) >= 3 and lines[0].startswith(b"Q\t")
assert lines[1].startswith(b"P\t") and lines[2].startswith(b"J\t")
open(sys.argv[2], "wb").write(lines[0] + lines[1] + lines[2][:9])
PY
    cp .mycelium.parliament "$T/parl3-partial-j.before"
    expect_fail "a partial first-J tail is not recovered by the writer" \
        "unsealed" "$MYC" franchise
    expect_fail "a partial first-J tail is refused by the reader" \
        "unsealed" "$CHECK"
    cmp -s .mycelium.parliament "$T/parl3-partial-j.before" &&
        pass "a partial first-J tail leaves the parliament digest untouched" ||
        fail "partial first-J tail was mutated"
)

for n in one two; do
    rm -rf "$T/parl-det-$n" 2>/dev/null || true
    mkdir "$T/parl-det-$n"
    (
        cd "$T/parl-det-$n"
        feed "$T/parl-det-$n" parl2-field wolf 1 2
        "$MYC" enroll 3 amber wolf sings
        feed "$T/parl-det-$n" parl2-field wolf 3 10
        "$MYC" examine
        "$MYC" petition 1
        "$MYC" petition 5
        "$MYC" franchise
    ) >"$T/parl-det-$n/stdout" 2>"$T/parl-det-$n/stderr"
done
if cmp "$T/parl-det-one/.mycelium.parliament" "$T/parl-det-two/.mycelium.parliament" &&
        cmp "$T/parl-det-one/stdout" "$T/parl-det-two/stdout"; then
    pass "clean-room parliament chain and stdout are byte-identical"
else
    fail "parliament clean-room determinism"
fi

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

src = os.path.join(root, "parl")
raw = open(os.path.join(src, ".mycelium.parliament"), "rb").read()
payloads = [line[:-17] for line in raw.splitlines()]
for name in ("parl-flip", "parl-trunc", "parl-finding", "parl-verdict",
             "parl-opaque-glyph", "parl-opaque-school-count"):
    path = os.path.join(root, name)
    shutil.copytree(src, path)

flipped = bytearray(raw)
flipped[len(raw) // 2] ^= 1
open(os.path.join(root, "parl-flip", ".mycelium.parliament"), "wb").write(bytes(flipped))
open(os.path.join(root, "parl-trunc", ".mycelium.parliament"), "wb").write(raw[:-1])

p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"J\t1\texam\t"):
        p[i] = b"J\t1\texam\tadequate"
        break
open(os.path.join(root, "parl-finding", ".mycelium.parliament"), "wb").write(seal(p))

p = payloads.copy()
for i, row in enumerate(p):
    if row == b"V\t1\tPASS":
        p[i] = b"V\t1\tWEAKEN"
        break
open(os.path.join(root, "parl-verdict", ".mycelium.parliament"), "wb").write(seal(p))

def mutate_first_opaque(name, edit):
    p = payloads.copy()
    for i, row in enumerate(p):
        fields = row.split(b"\t")
        if len(fields) == 11 and fields[0] == b"P" and fields[4] == b"root-v1":
            edit(fields)
            p[i] = b"\t".join(fields)
            break
    else:
        raise AssertionError("root-v1 opaque ballot not found")
    open(os.path.join(root, name, ".mycelium.parliament"), "wb").write(seal(p))

mutate_first_opaque("parl-opaque-glyph",
                    lambda fields: fields.__setitem__(2, b"999"))

school_lines = open(os.path.join(src, ".mycelium.school"), "rb").read().splitlines()
final_school_chain = school_lines[-1][-16:]
def edit_school_count(fields):
    fields[5] = b"999"
    fields[6] = final_school_chain
mutate_first_opaque("parl-opaque-school-count", edit_school_count)
PY

for spec in "parl-flip:parliament chain broken:chain does not fold" \
            "parl-trunc:unsealed:unsealed" \
            "parl-finding:false finding:false finding" \
            "parl-verdict:false verdict:false verdict" \
            "parl-opaque-glyph:names a glyph absent:opaque petition needs an existing glyph" \
            "parl-opaque-school-count:school prefix ends before requested record count:pinned school prefix"; do
    name=${spec%%:*}; rest=${spec#*:}; writer=${rest%%:*}; reader=${rest#*:}
    (
        cd "$T/$name"
        expect_fail "$name writer refusal" "$writer" "$MYC" franchise
        expect_fail "$name reader refusal" "$reader" "$CHECK"
    )
done

# ---- body 5: the mint (writer stage) ----
mint_flow() { # $1 room: feed 1-10, enroll, examine, petition -> PASS citizen
    mkdir -p "$1"
    feed "$1" mint-field wolf 1 2
    (cd "$1" && "$MYC" enroll 3 amber wolf sings >/dev/null)
    feed "$1" mint-field wolf 3 10
    (cd "$1" && "$MYC" examine >/dev/null && "$MYC" petition 1 >/dev/null)
}

mint_flow "$T/mintroom"
cp -R "$T/mintroom" "$T/mint-pre"
(
    cd "$T/mintroom"
    "$MYC" mint 1 >mint.out
    grep -F 'PASS budget 512' mint.out >/dev/null &&
        grep -F 'E LIT' mint.out >/dev/null &&
        pass "the first note is struck LIT on a full budget" ||
        fail "mint: $(cat mint.out | tr '\n' ' ')"
    blob=$(ls .mycelium.notes.d | head -1)
    [ "$(wc -c < ".mycelium.notes.d/$blob" | tr -d ' ')" = "388" ] &&
        pass "the weight blob is canonical body5-note-weight-v1" ||
        fail "blob size"
    expect_fail "a minted identity refuses a second mint" "already minted" \
        "$MYC" mint 1
    expect_fail "a retrain inside the cooldown refuses" "cooldown holds" \
        "$MYC" retrain 1
    expect_fail "a missing glyph cannot mint" "needs an existing glyph" \
        "$MYC" mint 9
)
# no-authority: mintroom holds a note, mint-pre does not; same field,
# every organ of bodies 1-4 must answer byte-identically
for room in "$T/mintroom" "$T/mint-pre"; do
    (
        cd "$room"
        "$MYC" unfold the amber wolf remembers >na-unfold.out
        "$MYC" propose >na-propose.out
        "$MYC" franchise >na-franchise.out
    )
done
if cmp -s "$T/mintroom/na-unfold.out" "$T/mint-pre/na-unfold.out" &&
        cmp -s "$T/mintroom/na-propose.out" "$T/mint-pre/na-propose.out" &&
        cmp -s "$T/mintroom/na-franchise.out" "$T/mint-pre/na-franchise.out"; then
    pass "the minted weight has no authority over bodies 1-4"
else
    fail "no-authority"
fi
feed "$T/mintroom" mint-field wolf 11 18
(
    cd "$T/mintroom"
    "$MYC" retrain 1 >re.out
    grep -F 'T steps 512' re.out >/dev/null &&
        pass "a cooled note retrains on the pinned field without drift" ||
        fail "retrain: $(cat re.out | tr '\n' ' ')"
)
feed "$T/mintroom" mint-field wolf 19 26
(
    cd "$T/mintroom"
    "$MYC" notes >rent.out
    grep -F 'R note 1' rent.out >/dev/null &&
        pass "a starved citizen's note dies into the morgue" || fail "rent R"
    expect_fail "a note in the morgue cannot retrain" "morgue" \
        "$MYC" retrain 1
)
feed "$T/mintroom" mint-field wolf 27 27
(
    cd "$T/mintroom"
    "$MYC" notes >res.out
    grep -F 'Z note 1' res.out >/dev/null &&
        pass "the same identity resurrects with its weights kept" || fail "rent Z"
)

# clean-room determinism: same pin, same binary, byte-identical weights
for n in one two; do
    mint_flow "$T/mintdet-$n"
    (cd "$T/mintdet-$n" && "$MYC" mint 1) >"$T/mintdet-$n/stdout" 2>&1
done
if cmp "$T/mintdet-one/.mycelium.notes" "$T/mintdet-two/.mycelium.notes" &&
        diff -r "$T/mintdet-one/.mycelium.notes.d" "$T/mintdet-two/.mycelium.notes.d" >/dev/null &&
        cmp "$T/mintdet-one/stdout" "$T/mintdet-two/stdout"; then
    pass "two mints from one pin strike byte-identical weights"
else
    fail "mint determinism"
fi

# recovery: cuts after M and after T complete to the reference chain
for cut in 2 3; do
    rm -rf "$T/mintrec-$cut" 2>/dev/null || true
    cp -R "$T/mintdet-one" "$T/mintrec-$cut"
    (
        cd "$T/mintrec-$cut"
        head -n $cut "$T/mintdet-one/.mycelium.notes" > .mycelium.notes
        "$MYC" notes >/dev/null 2>rec.err
        cmp -s .mycelium.notes "$T/mintdet-one/.mycelium.notes" &&
            pass "an interruption after line $cut recovers the exact mint" ||
            fail "mint recovery cut $cut: $(cat rec.err | tr '\n' ' ')"
    )
done

python3 - "$T" <<'PY'
import os, shutil, struct, sys

root = sys.argv[1]
SEED = 0xcbf29ce484222325
PRIME = 0x100000001b3

def fnv(data, h=SEED):
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

src = os.path.join(root, "mintdet-one")
raw = open(os.path.join(src, ".mycelium.notes"), "rb").read()
payloads = [line[:-17] for line in raw.splitlines()]
for name in ("notes-flip", "notes-trunc", "notes-lit", "notes-noblob",
             "notes-badblob", "notes-nanblob"):
    path = os.path.join(root, name)
    shutil.copytree(src, path)

flipped = bytearray(raw)
flipped[len(raw) // 2] ^= 1
open(os.path.join(root, "notes-flip", ".mycelium.notes"), "wb").write(bytes(flipped))
open(os.path.join(root, "notes-trunc", ".mycelium.notes"), "wb").write(raw[:-1])

# false verdict: hits lowered to the baseline while E still says LIT
p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"T\t"):
        fields = row.split(b"\t")
        fields[13] = fields[14]
        p[i] = b"\t".join(fields)
open(os.path.join(root, "notes-lit", ".mycelium.notes"), "wb").write(seal(p))

d = os.path.join(root, "notes-noblob", ".mycelium.notes.d")
for f in os.listdir(d):
    os.remove(os.path.join(d, f))

d = os.path.join(root, "notes-badblob", ".mycelium.notes.d")
for f in os.listdir(d):
    open(os.path.join(d, f), "wb").write(b"short")

# non-finite floats with a matching resealed digest
nan_blob = struct.pack("<97f", *([float("inf")] * 97))
nan_hex = f"{fnv(nan_blob):016x}".encode()
d = os.path.join(root, "notes-nanblob", ".mycelium.notes.d")
for f in os.listdir(d):
    os.remove(os.path.join(d, f))
open(os.path.join(d, nan_hex.decode()), "wb").write(nan_blob)
p = payloads.copy()
for i, row in enumerate(p):
    if row.startswith(b"T\t"):
        fields = row.split(b"\t")
        fields[15] = nan_hex
        p[i] = b"\t".join(fields)
open(os.path.join(root, "notes-nanblob", ".mycelium.notes"), "wb").write(seal(p))
PY

for spec in "notes-flip:notes chain broken" \
            "notes-trunc:unsealed" \
            "notes-lit:false verdict" \
            "notes-noblob:is missing" \
            "notes-badblob:not canonical" \
            "notes-nanblob:non-finite"; do
    name=${spec%%:*}; needle=${spec#*:}
    (
        cd "$T/$name"
        expect_fail "$name writer refusal" "$needle" "$MYC" notes
    )
done

mint_flow "$T/mint-nocit" >/dev/null 2>&1 || true
rm -rf "$T/mint-nocit"
mkdir -p "$T/mint-nocit"
feed "$T/mint-nocit" mint-field wolf 1 2
(cd "$T/mint-nocit" && "$MYC" enroll 3 amber wolf sings >/dev/null)
feed "$T/mint-nocit" mint-field wolf 3 10
(
    cd "$T/mint-nocit"
    "$MYC" examine >/dev/null
    expect_fail "a glyph without citizenship cannot mint" "is not a citizen" \
        "$MYC" mint 1
)

if [ ! -e "$T/.failed" ]; then
    printf '%s\n' '----' 'ALL MYCELIUM GATES PASS'
    exit 0
fi
printf '%s\n' '----' 'MYCELIUM GATES FAILED'
exit 1
