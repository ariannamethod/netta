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

dup_a = (b"luminous archive remembers every winter river and carries the patient signal.\n\n") * 12
dup_b = (b"copper engine follows every summer road and measures the turning voltage.\n\n") * 12
case("dup-a", dup_a, sig(dup_a, 21))
case("dup-b", dup_b, sig(dup_b, 22))

orphan = os.path.join(T, "orphan", ".mycelium.grave")
os.makedirs(orphan)
open(os.path.join(orphan, f"{fnv(sea):016x}"), "wb").write(sea)
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
for name in ("mut-label", "mut-u", "mut-s", "prefix", "missing", "unsealed"):
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

lines = raw.splitlines(keepends=True)
open(os.path.join(root, "prefix", ".mycelium.ledger"), "wb").write(b"".join(lines[:2]))
open(os.path.join(root, "missing", ".mycelium.ledger"), "wb").write(raw)
open(os.path.join(root, "unsealed", ".mycelium.ledger"), "wb").write(raw[:-1])
PY

for spec in "mut-label:G field grammar:G label" \
            "mut-u:U field grammar:U k" \
            "mut-s:attested s line:attested s line" \
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
    ) >"$T/determinism-$n/stdout" 2>"$T/determinism-$n/stderr"
done
if cmp "$T/determinism-one/.mycelium.ledger" "$T/determinism-two/.mycelium.ledger" &&
        diff -r "$T/determinism-one/.mycelium.grave" "$T/determinism-two/.mycelium.grave" &&
        cmp "$T/determinism-one/stdout" "$T/determinism-two/stdout"; then
    pass "clean-room grave, ledger and stdout are byte-identical"
else
    fail "clean-room determinism"
fi

if [ ! -e "$T/.failed" ]; then
    printf '%s\n' '----' 'ALL MYCELIUM GATES PASS'
    exit 0
fi
printf '%s\n' '----' 'MYCELIUM GATES FAILED'
exit 1
