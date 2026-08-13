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
D=$("$N" "$T/a.bytes" --reset --episodes 0 --state "$T/z.state" --bio "$T/z.bio" 2>/dev/null | awk "/^island 0: /{print \$NF}")
[ "$D" = "digest=af63dc4c8601ec8c" ]
gate "Z1 FNV-1a-64 matches external vector (\"a\")" $? 0
i=0; : > "$T/all.bytes"
while [ $i -lt 256 ]; do printf "\\x$(printf %02x $i)" >> "$T/all.bytes"; i=$((i+1)); done
L=$(wc -c < "$T/all.bytes" | tr -d ' ')
[ "$L" = "256" ]
gate "Z1 all 256 byte values round-trip incl NUL" $? 0
D1=$("$N" "$T/all.bytes" --reset --episodes 0 --state "$T/z1.state" --bio "$T/z1.bio" 2>/dev/null | awk "/^island 0: /{print \$NF}")
D2=$("$N" "$T/all.bytes" --reset --episodes 0 --state "$T/z2.state" --bio "$T/z2.bio" 2>/dev/null | awk "/^island 0: /{print \$NF}")
[ -n "$D1" ] && [ "$D1" = "$D2" ]
gate "Z1 digest deterministic across runs" $? 0
printf 'мир שלום world\n' > "$T/u.bytes"
UL=$("$N" "$T/u.bytes" --reset --episodes 0 --state "$T/u.state" --bio "$T/u.bio" 2>/dev/null | awk "/^island 0: /{print \$(NF-1)}")
[ "$UL" = "len=22" ]
gate "Z1 UTF-8 island measured in raw bytes" $? 0

# --- Z2: atomic game ----------------------------------------------------
W="$T/w.bytes"; cat NETTALOG2.md > "$W"
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/a1.state" --bio "$T/a1.bio" >/dev/null 2>&1
gate "Z2 newborn life runs" $? 0
FIRST=$(grep -v '^a	' "$T/a1.bio" | head -1 | awk -F'\t' '{print $8}')
[ "$FIRST" = "8.000000" ]
gate "Z2 newborn first step exactly 8 bits" $? 0
NON8=$(awk -F'\t' 'NF>=11 && $8!="8.000000"{n++} END{print n+0}' "$T/a1.bio")
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

# --- B3: shadow unit-LM ------------------------------------------------
LM=$("$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/b3a.state" --bio "$T/b3a.bio" 2>/dev/null | tr -d ',' | awk '/unit-LM/{print $6, $8}')
U=$(echo "$LM" | awk '{print $1}'); A=$(echo "$LM" | awk '{print $2}')
[ -n "$U" ] && [ "$U" = "$A" ]
gate "B3 no units: unit-LM identical to atomic (prequential twin, $U)" $? 0
LMR=$("$N" "$T/rep.bytes" --reset --seed 42 --steps 4000 --state "$T/b3r.state" --bio "$T/b3r.bio" 2>/dev/null | tr -d ',' | awk '/unit-LM/{print $6, $8}')
UR=$(echo "$LMR" | awk '{print $1}'); AR=$(echo "$LMR" | awk '{print $2}')
awk -v u="$UR" -v a="$AR" 'BEGIN{exit !(u < a)}'
gate "B3 units predict: unit-LM $UR < atomic $AR on repetition" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 2000 --state "$T/b3d.state" --bio "$T/b3d.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 2000 --state "$T/b3s.state" --bio "$T/b3s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --episodes 1 --steps 2000 --state "$T/b3s.state" --bio "$T/b3s.bio" >/dev/null 2>&1
cmp -s "$T/b3d.bio" "$T/b3s.bio" && cmp -s "$T/b3d.state" "$T/b3s.state"
gate "B3 shadow counters survive restart (split life identical)" $? 0

# --- B4: context oracles ------------------------------------------------
i=0; : > "$T/ab.bytes"
while [ $i -lt 2000 ]; do printf 'ab' >> "$T/ab.bytes"; i=$((i+1)); done
i=0; : > "$T/const.bytes"
while [ $i -lt 4000 ]; do printf 'a' >> "$T/const.bytes"; i=$((i+1)); done
mdl() { grep "^model $2 " "$1" | awk '{print $NF}'; }
"$N" "$T/ab.bytes" --reset --seed 5 --steps 2000 --state "$T/ab.state" --bio "$T/ab.bio" > "$T/ab.out" 2>&1
AU=$(mdl "$T/ab.out" atomic-uni); BB=$(mdl "$T/ab.out" byte-bi)
awk -v a="$AU" -v b="$BB" 'BEGIN{exit !(a - b >= 0.5)}'
gate "B4 context wins on ab-world: byte-bi $BB vs atomic $AU (gap>=0.5)" $? 0
"$N" "$T/const.bytes" --reset --seed 5 --steps 2000 --state "$T/c4.state" --bio "$T/c4.bio" > "$T/c4.out" 2>&1
AUC=$(mdl "$T/c4.out" atomic-uni); BBC=$(mdl "$T/c4.out" byte-bi)
[ "$AUC" = "$BBC" ]
gate "B4 twins converge on constant world ($BBC == $AUC)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --steps 4000 --state "$T/r4.state" --bio "$T/r4.bio" > "$T/r4.out" 2>&1
UU=$(mdl "$T/r4.out" unit-uni); MB=$(mdl "$T/r4.out" move-bi)
awk -v u="$UU" -v m="$MB" 'BEGIN{exit !(m < u)}'
gate "B4 move context helps: move-bi $MB < unit-uni $UU on repetition" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 2000 --state "$T/b4d.state" --bio "$T/b4d.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 2000 --state "$T/b4s.state" --bio "$T/b4s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --episodes 1 --steps 2000 --state "$T/b4s.state" --bio "$T/b4s.bio" >/dev/null 2>&1
cmp -s "$T/b4d.state" "$T/b4s.state"
gate "B4 oracle counters survive restart (split state identical)" $? 0

# --- B5: the earned right to act ---------------------------------------
"$N" "$T/ab.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/e5.state" --bio "$T/e5.bio" > "$T/e5.out" 2>&1
grep -q '^a	.*	bi$' "$T/e5.bio"
gate "B5 the right is earned mid-life (bi acts after the threshold)" $? 0
"$N" "$T/ab.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/l5.state" --bio "$T/l5.bio" --actor-lock uni > "$T/l5.out" 2>&1
E5=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/e5.out")
L5=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/l5.out")
awk -v e="$E5" -v l="$L5" 'BEGIN{exit !(e < l)}'
gate "B5 earned actor lives cheaper: $E5 < locked-uni $L5" $? 0
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z5e.state" --bio "$T/z5e.bio" >/dev/null 2>&1
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z5l.state" --bio "$T/z5l.bio" --actor-lock uni >/dev/null 2>&1
cmp -s "$T/z5e.bio" "$T/z5l.bio"
gate "B5 zero intervention where the right is not earned (bio identical)" $? 0
"$N" "$T/ab.bytes" --reset --seed 5 --episodes 3 --steps 600 --state "$T/s5s.state" --bio "$T/s5s.bio" >/dev/null 2>&1
"$N" "$T/ab.bytes" --episodes 3 --steps 600 --state "$T/s5s.state" --bio "$T/s5s.bio" >/dev/null 2>&1
cmp -s "$T/e5.bio" "$T/s5s.bio" && cmp -s "$T/e5.state" "$T/s5s.state"
gate "B5 actor elections survive restart (split life identical)" $? 0
F5=$("$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z5f.state" --bio "$T/z5f.bio" --actor-lock bi 2>/dev/null | awk -F': ' '/^bits per raw byte/{print $2}')
[ "$F5" = "8.000000" ]
gate "B5 forced ignorance stays uniform: locked-bi exactly 8.0 on virgin rows ($F5)" $? 0

# --- B6: the trigram floor ---------------------------------------------
i=0; : > "$T/p3.bytes"
while [ $i -lt 700 ]; do printf 'abcacb' >> "$T/p3.bytes"; i=$((i+1)); done
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/p3.state" --bio "$T/p3.bio" > "$T/p3.out" 2>&1
BB6=$(awk '/^model byte-bi /{print $NF}' "$T/p3.out")
TT6=$(awk '/^model byte-tri /{print $NF}' "$T/p3.out")
awk -v b="$BB6" -v t="$TT6" 'BEGIN{exit !(b - t >= 0.5)}'
gate "B6 trigram sees period-3: byte-tri $TT6 vs byte-bi $BB6 (gap>=0.5)" $? 0
grep -q '^a	.*	tri$' "$T/p3.bio"
gate "B6 the seat passes to tri on the period-3 world" $? 0
E6=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/p3.out")
LB6=$("$N" "$T/p3.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/p3b.state" --bio "$T/p3b.bio" --actor-lock bi 2>/dev/null | awk -F': ' '/^bits per raw byte/{print $2}')
LU6=$("$N" "$T/p3.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/p3u.state" --bio "$T/p3u.bio" --actor-lock uni 2>/dev/null | awk -F': ' '/^bits per raw byte/{print $2}')
awk -v e="$E6" -v b="$LB6" -v u="$LU6" 'BEGIN{exit !(e < b && b < u)}'
gate "B6 the ladder of power: earned $E6 < locked-bi $LB6 < locked-uni $LU6" $? 0
"$N" "$T/ab.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/ab6.state" --bio "$T/ab6.bio" > "$T/ab6.out" 2>&1
BB7=$(awk '/^model byte-bi /{print $NF}' "$T/ab6.out")
TT7=$(awk '/^model byte-tri /{print $NF}' "$T/ab6.out")
[ "$BB7" = "$TT7" ]
gate "B6 tri collapses to bi where contexts are in bijection ($TT7)" $? 0
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z6e.state" --bio "$T/z6e.bio" >/dev/null 2>&1
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z6l.state" --bio "$T/z6l.bio" --actor-lock uni >/dev/null 2>&1
cmp -s "$T/z6e.bio" "$T/z6l.bio"
gate "B6 zero intervention still holds with three candidates" $? 0
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 3 --steps 600 --state "$T/p6s.state" --bio "$T/p6s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --episodes 3 --steps 600 --state "$T/p6s.state" --bio "$T/p6s.bio" >/dev/null 2>&1
cmp -s "$T/p3.bio" "$T/p6s.bio" && cmp -s "$T/p3.state" "$T/p6s.state"
gate "B6 trigram counters and elections survive restart" $? 0

# --- B7: the second island ---------------------------------------------
i=0; : > "$T/wA.bytes"; : > "$T/wB.bytes"
while [ $i -lt 300 ]; do
  printf 'the cat sat on the mat and the dog ran off. ' >> "$T/wA.bytes"
  printf 'the dog sat on the log and the cat ran off. ' >> "$T/wB.bytes"
  i=$((i+1))
done
od -An -v -tu1 "$T/wB.bytes" | tr -s ' ' '\n' | grep -v '^$' | awk 'BEGIN{s=12345}{a[NR]=$1}END{n=NR; for(i=n;i>1;i--){s=(s*1103515245+12345)%2147483648; j=(s%i)+1; t=a[i];a[i]=a[j];a[j]=t} for(i=1;i<=n;i++)printf "%c", a[i]}' > "$T/wBs.bytes"
tl() { grep "^this-life model $2 " "$1" | awk '{print $NF}'; }
"$N" "$T/wA.bytes" "$T/wB.bytes" --reset --seed 11 --episodes 4 --steps 800 --island 0 --state "$T/tr.state" --bio "$T/tr.bio" >/dev/null 2>&1
"$N" "$T/wA.bytes" "$T/wB.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --state "$T/tr.state" --bio "$T/tr.bio" > "$T/trB.out" 2>&1
"$N" "$T/wA.bytes" "$T/wB.bytes" --reset --seed 12 --episodes 2 --steps 800 --island 1 --state "$T/nb.state" --bio "$T/nb.bio" > "$T/nbB.out" 2>&1
TRB=$(tl "$T/trB.out" byte-bi); NBB=$(tl "$T/nbB.out" byte-bi)
TRT=$(tl "$T/trB.out" byte-tri); NBT=$(tl "$T/nbB.out" byte-tri)
awk -v a="$TRB" -v b="$NBB" -v c="$TRT" -v d="$NBT" 'BEGIN{exit !(b-a >= 0.5 && d-c >= 0.5)}'
gate "B7 kin experience transfers: bi $TRB vs $NBB, tri $TRT vs $NBT (gap>=0.5)" $? 0
"$N" "$T/wA.bytes" "$T/wBs.bytes" --reset --seed 11 --episodes 4 --steps 800 --island 0 --state "$T/sh.state" --bio "$T/sh.bio" >/dev/null 2>&1
"$N" "$T/wA.bytes" "$T/wBs.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --state "$T/sh.state" --bio "$T/sh.bio" > "$T/shB.out" 2>&1
"$N" "$T/wA.bytes" "$T/wBs.bytes" --reset --seed 12 --episodes 2 --steps 800 --island 1 --state "$T/shn.state" --bio "$T/shn.bio" > "$T/shN.out" 2>&1
SHB=$(tl "$T/shB.out" byte-bi); SHN=$(tl "$T/shN.out" byte-bi)
awk -v a="$SHB" -v b="$SHN" 'BEGIN{d=b-a; if(d<0)d=-d; exit !(d < 0.1)}'
gate "B7 shuffled world kills the transfer: $SHB vs $SHN (|gap|<0.1)" $? 0
"$N" "$T/p3.bytes" "$T/wB.bytes" --reset --seed 11 --episodes 4 --steps 800 --island 0 --state "$T/dc.state" --bio "$T/dc.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/wB.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --state "$T/dc.state" --bio "$T/dc.bio" > "$T/dcB.out" 2>&1
DCB=$(tl "$T/dcB.out" byte-bi)
awk -v kin="$TRB" -v don="$DCB" 'BEGIN{exit !(don - kin >= 0.5)}'
gate "B7 kinship is measurable: kin donor $TRB beats alien donor $DCB (gap>=0.5)" $? 0
DA1=$(grep '^island 0: ' "$T/trB.out" | awk '{print $NF}')
DA2=$(grep '^island 0: ' "$T/nbB.out" | awk '{print $NF}')
DB1=$(grep '^island 1: ' "$T/trB.out" | awk '{print $NF}')
DB2=$(grep '^island 1: ' "$T/nbB.out" | awk '{print $NF}')
[ -n "$DA1" ] && [ "$DA1" = "$DA2" ] && [ -n "$DB1" ] && [ "$DB1" = "$DB2" ]
gate "B7 no life mutates a world: island digests identical across lives" $? 0
UR=$(grep 'units recognisable on island 1' "$T/trB.out" | awk '{print $6}')
[ -n "$UR" ] && [ "$UR" -gt 0 ]
gate "B7 exact forms are recognised on the kin island ($UR units)" $? 0

echo "----"
if [ $FAIL -eq 0 ]; then echo "ALL GATES PASS"; exit 0; fi
echo "$FAIL GATE(S) FAILED"; exit 1
