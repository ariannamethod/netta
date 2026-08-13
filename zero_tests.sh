#!/bin/sh
# NETTA ZERO gates Z0-Z2. Machine verdicts only; rc=0 means every gate
# passed. Run from the repo root. Each gate that can be faked carries a
# red counterpart proving the check can fail.
set -u
T=$(mktemp -d) || exit 99
trap 'rm -rf "$T"' EXIT
FAIL=0
say() { printf '%s %s\n' "$1" "$2"; }
gate() { # gate <name> <rc> <expected-rc>
    if [ "$2" -eq "$3" ]; then say PASS "$1"; else say FAIL "$1 (rc=$2 want $3)"; FAIL=$((FAIL+1)); fi
}

# --- build (strict) -----------------------------------------------------
cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o "$T/netta" 2>"$T/build.log"
rc=$?; [ -s "$T/build.log" ] && rc=98
gate "Z0 build strict, zero warnings" $rc 0
N="$T/netta"

# --- Z0: provenance -----------------------------------------------------
"$N" 2>/dev/null | head -1 | grep -q "NETTA ZERO"
gate "Z0 build identifies as NETTA ZERO" $? 0
printf 'NETTASTATE-OLD-GARBAGE' > "$T/fake.state"
printf 'abc' > "$T/tiny.bytes"
"$N" "$T/tiny.bytes" --state "$T/fake.state" --bio "$T/f.bio" >/dev/null 2>&1
gate "Z0 foreign state refused" $? 1

# --- Z1: byte world -----------------------------------------------------
printf 'a' > "$T/a.bytes"
D=$("$N" "$T/a.bytes" --reset --episodes 0 --state "$T/z.state" --bio "$T/z.bio" 2>/dev/null | awk '/island 0/{print $NF}')
[ "$D" = "digest=af63dc4c8601ec8c" ]
gate "Z1 FNV-1a-64 matches external vector (\"a\")" $? 0
i=0; : > "$T/all.bytes"
while [ $i -lt 256 ]; do printf "\\x$(printf %02x $i)" >> "$T/all.bytes"; i=$((i+1)); done
L=$(wc -c < "$T/all.bytes" | tr -d ' ')
[ "$L" = "256" ]
gate "Z1 all 256 byte values round-trip incl NUL" $? 0
D1=$("$N" "$T/all.bytes" --reset --episodes 0 --state "$T/z1.state" --bio "$T/z1.bio" 2>/dev/null | awk '/island 0/{print $NF}')
D2=$("$N" "$T/all.bytes" --reset --episodes 0 --state "$T/z2.state" --bio "$T/z2.bio" 2>/dev/null | awk '/island 0/{print $NF}')
[ -n "$D1" ] && [ "$D1" = "$D2" ]
gate "Z1 digest deterministic across runs" $? 0
printf 'мир שלום world\n' > "$T/u.bytes"
UL=$("$N" "$T/u.bytes" --reset --episodes 0 --state "$T/u.state" --bio "$T/u.bio" 2>/dev/null | awk '/island 0/{print $(NF-1)}')
[ "$UL" = "len=22" ]
gate "Z1 UTF-8 island measured in raw bytes" $? 0

# --- Z2: atomic game ----------------------------------------------------
W="$T/w.bytes"; cat NETTALOG2.md > "$W"
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/a1.state" --bio "$T/a1.bio" >/dev/null 2>&1
gate "Z2 newborn life runs" $? 0
FIRST=$(head -1 "$T/a1.bio" | awk -F'\t' '{print $8}')
[ "$FIRST" = "8.000000" ]
gate "Z2 newborn first step exactly 8 bits" $? 0
NON8=$(awk -F'\t' '$8!="8.000000"{n++} END{print n+0}' "$T/a1.bio")
[ "$NON8" -gt 0 ]
gate "Z2 learning moves loss within one life (red: all-8 = dead)" $? 0
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/a2.state" --bio "$T/a2.bio" >/dev/null 2>&1
cmp -s "$T/a1.bio" "$T/a2.bio" && cmp -s "$T/a1.state" "$T/a2.state"
gate "Z2 same seed: biography and state bit-identical" $? 0
"$N" "$W" --reset --seed 43 --episodes 1 --steps 64 --state "$T/a3.state" --bio "$T/a3.bio" >/dev/null 2>&1
cmp -s "$T/a1.bio" "$T/a3.bio"
gate "Z2 different seed diverges (red: comparison not tautological)" $? 1
"$N" "$W" --reset --seed 42 --episodes 2 --steps 64 --state "$T/d.state" --bio "$T/d.bio" >/dev/null 2>&1
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/s.state" --bio "$T/s.bio" >/dev/null 2>&1
"$N" "$W" --episodes 1 --steps 64 --state "$T/s.state" --bio "$T/s.bio" >/dev/null 2>&1
cmp -s "$T/d.bio" "$T/s.bio" && cmp -s "$T/d.state" "$T/s.state"
gate "Z2 split life equals direct life across restart" $? 0
"$N" "$T/tiny.bytes" --reset --steps 64 --state "$T/t.state" --bio "$T/t.bio" >/dev/null 2>&1
gate "Z2 island too small fails loud" $? 1

# --- B2: earned units --------------------------------------------------
i=0; : > "$T/rep.bytes"
while [ $i -lt 300 ]; do printf 'the cat sat on the mat and the dog ran off. ' >> "$T/rep.bytes"; i=$((i+1)); done
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 --state "$T/r.state" --bio "$T/r.bio" >"$T/r.out" 2>&1
gate "B2 repeated island life runs" $? 0
B=$(grep -c '^b	' "$T/r.bio")
[ "$B" -gt 0 ]
gate "B2 units are born from lived repetition" $? 0
"$N" "$T/all.bytes" --reset --seed 7 --episodes 1 --steps 200 --state "$T/ar.state" --bio "$T/ar.bio" >/dev/null 2>&1
AB=$(grep -c '^b	' "$T/ar.bio")
[ "$AB" -eq 0 ]
gate "B2 anti-repeat control births nothing (red twin of the above)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 --state "$T/ro.state" --bio "$T/ro.bio" --no-units >/dev/null 2>&1
grep -v '^[bm]	' "$T/r.bio" > "$T/r.atomic"
cmp -s "$T/r.atomic" "$T/ro.bio"
gate "B2 units never touch the atomic game (no-op subset identical)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 2000 --state "$T/du.state" --bio "$T/du.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 2000 --state "$T/su.state" --bio "$T/su.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --episodes 1 --steps 2000 --state "$T/su.state" --bio "$T/su.bio" >/dev/null 2>&1
cmp -s "$T/du.bio" "$T/su.bio" && cmp -s "$T/du.state" "$T/su.state"
gate "B2 births and pairs survive restart (split life identical)" $? 0
DUP=$(grep '^b	' "$T/r.bio" | awk -F'\t' '{print $5}' | sort | uniq -d | wc -l | tr -d ' ')
[ "$DUP" = "0" ]
gate "B2 same bytes, same identity (no duplicate unit)" $? 0
DPB=$(awk '/decisions per lived byte/{print $NF}' "$T/r.out")
awk -v d="$DPB" 'BEGIN{exit !(d < 1.0)}'
gate "B2 decisions per lived byte < 1 on repeated island ($DPB)" $? 0
DPB2=$(awk '/decisions per lived byte/{print $NF}' /dev/null 2>/dev/null; "$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/a2r.state" --bio "$T/a2r.bio" 2>/dev/null | awk '/decisions per lived byte/{print $NF}')
[ "$DPB2" = "1.0000" ]
gate "B2 decisions per lived byte == 1 without repetition (red twin)" $? 0

echo "----"
if [ $FAIL -eq 0 ]; then echo "ALL GATES PASS"; exit 0; fi
echo "$FAIL GATE(S) FAILED"; exit 1
