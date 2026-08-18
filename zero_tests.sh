#!/bin/sh
# NETTA ZERO gates Z0-B30. Machine verdicts only; rc=0 means every gate
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
FIRST=$(grep -v '^[ai]	' "$T/a1.bio" | head -1 | awk -F'\t' '{print $8}')
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
"$N" NETTALOG2.md --reset --steps -1 --state "$T/neg.state" --bio "$T/neg.bio" >/dev/null 2>&1
gate "Z2 negative step syntax is refused before address arithmetic" $? 1
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/bc.state" --bio "$T/bc.bio" >/dev/null 2>&1
mv "$T/bc.bio" "$T/bc.withheld"
"$N" "$W" --episodes 1 --steps 64 --state "$T/bc.state" --bio "$T/bc.bio" >/dev/null 2>&1
gate "Z2 a missing biography refuses resume (chain is externally verified)" $? 1
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/orphan.state" --bio "$T/orphan.bio" >/dev/null 2>&1
cp "$T/orphan.bio" "$T/orphan.before"
mv "$T/orphan.state" "$T/orphan.withheld"
"$N" "$W" --episodes 1 --steps 64 --state "$T/orphan.state" --bio "$T/orphan.bio" >/dev/null 2>&1
rc=$?; cmp -s "$T/orphan.bio" "$T/orphan.before" || rc=98
gate "Z2 an orphan biography is preserved and refuses implicit rebirth" $rc 1
cp "$W" "$T/alias.bytes"
cp "$T/alias.bytes" "$T/alias.before"
"$N" "$T/alias.bytes" --reset --steps 64 --state "$T/alias.bytes" --bio "$T/alias.bio" >/dev/null 2>&1
rc=$?; cmp -s "$T/alias.bytes" "$T/alias.before" || rc=98
gate "Z2 an immutable island cannot alias writable state" $rc 1
"$N" "$W" --reset --seed 42 --episodes 1 --steps 64 --state "$T/trail.state" --bio "$T/trail.bio" >/dev/null 2>&1
printf 'x' >> "$T/trail.state"
"$N" "$W" --episodes 1 --steps 64 --state "$T/trail.state" --bio "$T/trail.bio" >/dev/null 2>&1
gate "Z2 state with trailing bytes is refused" $? 1

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
"$N" "$T/wA.bytes" "$T/wB.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --start 16 --state "$T/tr.state" --bio "$T/tr.bio" > "$T/trB.out" 2>&1
"$N" "$T/wA.bytes" "$T/wB.bytes" --reset --seed 12 --episodes 2 --steps 800 --island 1 --start 16 --state "$T/nb.state" --bio "$T/nb.bio" > "$T/nbB.out" 2>&1
TRB=$(tl "$T/trB.out" byte-bi); NBB=$(tl "$T/nbB.out" byte-bi)
TRT=$(tl "$T/trB.out" byte-tri); NBT=$(tl "$T/nbB.out" byte-tri)
awk -v a="$TRB" -v b="$NBB" -v c="$TRT" -v d="$NBT" 'BEGIN{exit !(b-a >= 0.5 && d-c >= 0.5)}'
gate "B7 kin experience transfers: bi $TRB vs $NBB, tri $TRT vs $NBT (gap>=0.5)" $? 0
TRD=$(awk '/^island 1: /{sub(/^.*digest=/, ""); print}' "$T/trB.out")
NBD=$(awk '/^island 1: /{sub(/^.*digest=/, ""); print}' "$T/nbB.out")
TRR=$(awk -F'\t' -v d="$TRD" '$1 == "i" && $4 == d {print $3}' "$T/tr.bio")
NBR=$(awk -F'\t' -v d="$NBD" '$1 == "i" && $4 == d {print $3}' "$T/nb.bio")
awk -F'\t' -v r="$TRR" 'NF >= 11 && $3 == r {print $4}' "$T/tr.bio" > "$T/tr.pos"
awk -F'\t' -v r="$NBR" 'NF >= 11 && $3 == r {print $4}' "$T/nb.bio" > "$T/nb.pos"
cmp -s "$T/tr.pos" "$T/nb.pos"
gate "B7 traveller and newborn are judged at identical source positions" $? 0
"$N" "$T/wA.bytes" "$T/wBs.bytes" --reset --seed 11 --episodes 4 --steps 800 --island 0 --state "$T/sh.state" --bio "$T/sh.bio" >/dev/null 2>&1
"$N" "$T/wA.bytes" "$T/wBs.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --start 16 --state "$T/sh.state" --bio "$T/sh.bio" > "$T/shB.out" 2>&1
"$N" "$T/wA.bytes" "$T/wBs.bytes" --reset --seed 12 --episodes 2 --steps 800 --island 1 --start 16 --state "$T/shn.state" --bio "$T/shn.bio" > "$T/shN.out" 2>&1
SHB=$(tl "$T/shB.out" byte-bi); SHN=$(tl "$T/shN.out" byte-bi)
SHA=$(tl "$T/shB.out" atomic-uni); SHNA=$(tl "$T/shN.out" atomic-uni)
EXCESS=$(awk -v at="$SHA" -v an="$SHNA" -v bt="$SHB" -v bn="$SHN" \
  'BEGIN{printf "%.6f", (bn-bt)-(an-at)}')
awk -v x="$EXCESS" 'BEGIN{exit !(x < 0.1)}'
gate "B7 shuffle kills contextual excess beyond frequency: $EXCESS bit/byte (<0.1)" $? 0
"$N" "$T/p3.bytes" "$T/wB.bytes" --reset --seed 11 --episodes 4 --steps 800 --island 0 --state "$T/dc.state" --bio "$T/dc.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/wB.bytes" --seed 12 --episodes 2 --steps 800 --island 1 --start 16 --state "$T/dc.state" --bio "$T/dc.bio" > "$T/dcB.out" 2>&1
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

# --- B8: the emission seat and its probation ---------------------------
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 24 --steps 600 \
    --no-mv-nav --state "$T/p8.state" --bio "$T/p8.bio" > "$T/p8.out" 2>&1
MVP=$(grep -c '^a	.*	mvp$' "$T/p8.bio")
[ "$MVP" -gt 0 ]
gate "B8 the shadow record opens probation episodes ($MVP played)" $? 0
VC=$(grep -c '^v	' "$T/p8.bio")
[ "$VC" -gt 0 ]
gate "B8 probation emits real moves ($VC move receipts)" $? 0
MVREC=$(awk '/mv played record/{print $4}' "$T/p8.out")
REFREC=$(awk '/^mv matched byte refs:/{gsub(",",""); r=$6; if($8<r)r=$8; if($10<r)r=$10; print r}' "$T/p8.out")
[ -n "$MVREC" ] && [ -n "$REFREC" ]
gate "B8 the verdict is numeric on matched bytes: mv $MVREC vs best byte $REFREC" $? 0
awk -v m="$MVREC" -v t="$REFREC" 'BEGIN{exit !(m > t)}'
SEAT=$(awk -v r="$?" 'BEGIN{print r}')
MVSEAT=$(grep -c '^a	.*	mv$' "$T/p8.bio")
if [ "$SEAT" = "0" ]; then [ "$MVSEAT" -eq 0 ]; else [ "$MVSEAT" -gt 0 ]; fi
gate "B8 the seat follows the played record (mv seats: $MVSEAT)" $? 0
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z8e.state" --bio "$T/z8e.bio" >/dev/null 2>&1
"$N" "$T/all.bytes" --reset --seed 7 --steps 200 --state "$T/z8l.state" --bio "$T/z8l.bio" --actor-lock uni >/dev/null 2>&1
cmp -s "$T/z8e.bio" "$T/z8l.bio"
gate "B8 zero intervention with four candidates and probation" $? 0
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 12 --steps 600 \
    --no-mv-nav --state "$T/p8s.state" --bio "$T/p8s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --episodes 12 --steps 600 --no-mv-nav \
    --state "$T/p8s.state" --bio "$T/p8s.bio" >/dev/null 2>&1
cmp -s "$T/p8.bio" "$T/p8s.bio" && cmp -s "$T/p8.state" "$T/p8s.state"
gate "B8 probation and played record survive restart" $? 0

# --- A8: checkpoint audit repairs --------------------------------------
# Paired external-truth court for the played move record. The two organisms
# live the same a-only biography; their unread second island is either a-only
# or b-only. The first emitted move and its prior are identical, but truth is
# not. A loss that remains equal is self-information, not judgment.
i=0; : > "$T/aa.bytes"; : > "$T/bb.bytes"
while [ $i -lt 500 ]; do
  printf 'aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa' >> "$T/aa.bytes"
  printf 'bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb' >> "$T/bb.bytes"
  i=$((i+1))
done
"$N" "$T/aa.bytes" "$T/aa.bytes" --reset --seed 9 --episodes 4 --steps 3000 --island 0 --state "$T/hit.state" --bio "$T/hit.bio" >/dev/null 2>&1
"$N" "$T/aa.bytes" "$T/bb.bytes" --reset --seed 9 --episodes 4 --steps 3000 --island 0 --state "$T/miss.state" --bio "$T/miss.bio" >/dev/null 2>&1
HL=$(wc -l < "$T/hit.bio"); ML=$(wc -l < "$T/miss.bio")
"$N" "$T/aa.bytes" "$T/aa.bytes" --actor-lock mv --no-mv-nav \
    --episodes 1 --steps 128 --island 1 \
    --state "$T/hit.state" --bio "$T/hit.bio" >"$T/hit.out" 2>&1
"$N" "$T/aa.bytes" "$T/bb.bytes" --actor-lock mv --no-mv-nav \
    --episodes 1 --steps 128 --island 1 \
    --state "$T/miss.state" --bio "$T/miss.bio" >"$T/miss.out" 2>&1
HIT=$(tail -n "+$((HL+1))" "$T/hit.bio" | awk -F'\t' '$1=="v"{print $5, $8, $9; exit}')
MISS=$(tail -n "+$((ML+1))" "$T/miss.bio" | awk -F'\t' '$1=="v"{print $5, $8, $9; exit}')
HM=$(echo "$HIT" | awk '{print $1}'); MM=$(echo "$MISS" | awk '{print $1}')
HX=$(echo "$HIT" | awk '{print $2}'); MX=$(echo "$MISS" | awk '{print $2}')
HT=$(echo "$HIT" | awk '{print $3}'); MT=$(echo "$MISS" | awk '{print $3}')
[ -n "$HM" ] && [ "$HM" = "$MM" ] && [ "$HT" != "$MT" ] && [ "$HX" != "$MX" ]
gate "A8 played judge prices external truth: same emission, loss $HX vs $MX" $? 0
grep -q '^mv control record:' "$T/hit.out" && ! grep -q '^mv played record:' "$T/hit.out"
gate "A8 forced move control never enters the persisted mandate record" $? 0

# --- A9: probation borrows the body, never the seat ---------------------
# Two-phase life: tri earns the seat on period-3, then a skewed-census
# island decays the uni-tri lead into the hysteresis band [KEEP, GAIN).
# The probation at episode 7 must not erase tri's incumbency: episode 8
# is elected on a lead that still satisfies KEEP, so the seat stays tri.
# The island court and local probation door are disabled on this world to
# isolate the incumbent law; the two jurisdictional laws have their own
# gates below.
awk 'BEGIN{s=99; for(i=0;i<25000;i++){s=(s*1103515245+12345)%2147483648; n=7+int(s/65536)%2; for(j=0;j<n;j++)printf "a"; printf "b"}}' > "$T/skew.bytes"
"$N" "$T/p3.bytes" "$T/skew.bytes" --reset --seed 5 --episodes 4 --steps 600 --island 0 --no-island-court --no-local-probation --state "$T/a9.state" --bio "$T/a9.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/skew.bytes" --seed 5 --episodes 4 --steps 600 --island 1 --no-island-court --no-local-probation --state "$T/a9.state" --bio "$T/a9.bio" >/dev/null 2>&1
S7=$(awk -F'\t' '$1=="a" && $2==7{print $3}' "$T/a9.bio")
S8=$(awk -F'\t' '$1=="a" && $2==8{print $3}' "$T/a9.bio")
[ "$S7" = "mvp" ] && [ "$S8" = "tri" ]
gate "A9 probation does not depose the sitting incumbent (ep7=$S7, ep8=$S8)" $? 0
"$N" "$T/p3.bytes" "$T/skew.bytes" --reset --seed 5 --episodes 4 --steps 600 --island 0 --no-island-court --no-local-probation --state "$T/a9b.state" --bio "$T/a9b.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/skew.bytes" --seed 5 --episodes 3 --steps 600 --island 1 --no-island-court --no-local-probation --state "$T/a9b.state" --bio "$T/a9b.bio" > "$T/a9b.out" 2>&1
AU9=$(awk '/^model atomic-uni /{print $NF}' "$T/a9b.out")
TR9=$(awk '/^model byte-tri /{print $NF}' "$T/a9b.out")
LEAD9=$(awk -v u="$AU9" -v t="$TR9" 'BEGIN{printf "%.6f", u-t}')
awk -v l="$LEAD9" 'BEGIN{exit !(l >= 0.05 && l < 0.1)}'
gate "A9 the election lead sits in the hysteresis band ($LEAD9 in [0.05,0.1))" $? 0
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 7 --steps 600 --state "$T/a9s.state" --bio "$T/a9s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --episodes 1 --steps 600 --state "$T/a9s.state" --bio "$T/a9s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 8 --steps 600 --state "$T/a9d.state" --bio "$T/a9d.bio" >/dev/null 2>&1
cmp -s "$T/a9s.bio" "$T/a9d.bio" && cmp -s "$T/a9s.state" "$T/a9d.state"
gate "A9 a life split at the probation boundary is bit-identical" $? 0

# --- B9: navigation searches only the observable wake ------------------
# The no-navigation B8 arm above is the red control: it preserves the
# corrected refusal (mv 7.057066 vs matched tri 0.288818, no ordinary mv
# seats). The new policy must close that exposure gap without changing the
# court or looking at bytes at/after the current address.
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 24 --steps 600 \
    --state "$T/p9.state" --bio "$T/p9.bio" > "$T/p9.out" 2>&1
MV9=$(awk '/mv played record/{print $4}' "$T/p9.out")
REF9=$(awk '/^mv matched byte refs:/{gsub(",",""); r=$6; if($8<r)r=$8; if($10<r)r=$10; print r}' "$T/p9.out")
awk -v m="$MV9" -v r="$REF9" 'BEGIN{exit !(r-m >= 0.1)}'
gate "B9 searched mv beats matched byte court: $MV9 vs $REF9 (gain>=0.1)" $? 0
MV9SEAT=$(awk -F'\t' '$1=="a" && $3=="mv"{n++} END{print n+0}' "$T/p9.bio")
[ "$MV9SEAT" -gt 0 ]
gate "B9 the first move mandate is earned in played life ($MV9SEAT seats)" $? 0
OLD9=$(awk '/mv played record/{print $4}' "$T/p8.out")
OLDREF9=$(awk '/^mv matched byte refs:/{gsub(",",""); r=$6; if($8<r)r=$8; if($10<r)r=$10; print r}' "$T/p8.out")
OLD9SEAT=$(awk -F'\t' '$1=="a" && $3=="mv"{n++} END{print n+0}' "$T/p8.bio")
awk -v m="$OLD9" -v r="$OLDREF9" -v s="$OLD9SEAT" \
    'BEGIN{exit !(m > r && s == 0)}'
gate "B9 no-navigation red control still loses: $OLD9 vs $OLDREF9" $? 0

# Two unread islands share the exact 16-byte observable wake at --start 16
# and diverge at the target. Independently trained states have identical
# model/RNG histories. Search must therefore choose the same anchor and emit
# the same move before the external judge is allowed to distinguish truth.
printf 'abcacbabcacbabca' > "$T/nav-hit.bytes"
printf 'abcacbabcacbabca' > "$T/nav-miss.bytes"
i=0
while [ $i -lt 300 ]; do
  printf 'cbabcacbabcacb' >> "$T/nav-hit.bytes"
  printf 'xxxxxxxxxxxxxx' >> "$T/nav-miss.bytes"
  i=$((i+1))
done
"$N" "$T/p3.bytes" "$T/nav-hit.bytes" --reset --seed 9 \
    --episodes 4 --steps 3000 --island 0 \
    --state "$T/nh.state" --bio "$T/nh.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/nav-miss.bytes" --reset --seed 9 \
    --episodes 4 --steps 3000 --island 0 \
    --state "$T/nm.state" --bio "$T/nm.bio" >/dev/null 2>&1
NHL=$(wc -l < "$T/nh.bio"); NML=$(wc -l < "$T/nm.bio")
"$N" "$T/p3.bytes" "$T/nav-hit.bytes" --actor-lock mv --start 16 \
    --episodes 1 --steps 128 --island 1 \
    --state "$T/nh.state" --bio "$T/nh.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/nav-miss.bytes" --actor-lock mv --start 16 \
    --episodes 1 --steps 128 --island 1 \
    --state "$T/nm.state" --bio "$T/nm.bio" >/dev/null 2>&1
NH=$(tail -n "+$((NHL+1))" "$T/nh.bio" | awk -F'\t' '$1=="v"{print $5, $8, $9, $10; exit}')
NM=$(tail -n "+$((NML+1))" "$T/nm.bio" | awk -F'\t' '$1=="v"{print $5, $8, $9, $10; exit}')
NHA=$(echo "$NH" | awk '{print $1}'); NMA=$(echo "$NM" | awk '{print $1}')
NHLSS=$(echo "$NH" | awk '{print $2}'); NMLSS=$(echo "$NM" | awk '{print $2}')
NHT=$(echo "$NH" | awk '{print $3}'); NMT=$(echo "$NM" | awk '{print $3}')
NHANCH=$(echo "$NH" | awk '{print $4}'); NMANCH=$(echo "$NM" | awk '{print $4}')
[ -n "$NHA" ] && [ "$NHA" = "$NMA" ] && \
    [ "$NHANCH" = "$NMANCH" ] && [ "$NHT" != "$NMT" ] && \
    [ "$NHLSS" != "$NMLSS" ]
gate "B9 identical past fixes anchor/action; only outward truth changes loss" $? 0

"$N" "$T/p3.bytes" --reset --seed 5 --episodes 12 --steps 600 \
    --state "$T/p9s.state" --bio "$T/p9s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --episodes 12 --steps 600 \
    --state "$T/p9s.state" --bio "$T/p9s.bio" >/dev/null 2>&1
cmp -s "$T/p9.bio" "$T/p9s.bio" && cmp -s "$T/p9.state" "$T/p9s.state"
gate "B9 navigation and earned move mandate survive restart" $? 0

# A deterministic three-byte-census null has units but no predictive route:
# search never reaches probation, much less authority.
awk 'BEGIN{s=12345; for(i=0;i<30000;i++){s=(s*1103515245+12345)%2147483648; printf "%c", 97+(s%3)}}' > "$T/nav-null.bytes"
"$N" "$T/nav-null.bytes" --reset --seed 5 --episodes 24 --steps 600 \
    --state "$T/nnull.state" --bio "$T/nnull.bio" > "$T/nnull.out" 2>&1
NNMVP=$(awk -F'\t' '$1=="a" && $3=="mvp"{n++} END{print n+0}' "$T/nnull.bio")
NNMV=$(awk -F'\t' '$1=="a" && $3=="mv"{n++} END{print n+0}' "$T/nnull.bio")
[ "$NNMVP" -eq 0 ] && [ "$NNMV" -eq 0 ]
gate "B9 random-order null grants search no probation or mandate" $? 0

# --- B10: the island court ----------------------------------------------
# Global models travel; records are local. A globally seated actor must
# still satisfy the island it acts on: after ACTOR_MIN_BYTES of local
# evidence, a seat whose local lead over the local newborn record falls
# under KEEP is refused for that island only. Home mandate and kin
# transfer stay untouched; a single-island life must be bit-identical.
awk 'BEGIN{s=555; for(i=0;i<30000;i++){s=(s*1103515245+12345)%2147483648; r=int(s/65536)%100; if(r<87)printf "a"; else {s=(s*1103515245+12345)%2147483648; printf "%c", 98+int(s/65536)%10}}}' > "$T/alien.bytes"
awk 'BEGIN{for(i=0;i<700;i++) printf "cacbab"}' > "$T/kin.bytes"
"$N" "$T/p3.bytes" "$T/alien.bytes" --reset --seed 5 --episodes 6 --steps 600 --island 0 --state "$T/b10.state" --bio "$T/b10.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 8 --steps 600 --island 1 --state "$T/b10.state" --bio "$T/b10.bio" > "$T/b10.out" 2>&1
RV=$(grep -c '^r	' "$T/b10.bio")
A9S=$(awk -F'\t' '$1=="a" && $2==9{print $3}' "$T/b10.bio")
[ "$RV" -gt 0 ] && [ "$A9S" = "uni" ]
gate "B10 an alien island revokes the travelling seat ($RV revocations, ep9=$A9S)" $? 0
grep '^r	' "$T/b10.bio" | head -1 | awk -F'\t' '{exit !($4 == "tri" && $5 == "uni" && $6+0 < $7+0)}'
gate "B10 the revocation is numeric: the local newborn record beats the seat" $? 0
"$N" "$T/p3.bytes" "$T/alien.bytes" --reset --seed 5 --episodes 6 --steps 600 --island 0 --no-island-court --state "$T/b10n.state" --bio "$T/b10n.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 8 --steps 600 --island 1 --no-island-court --state "$T/b10n.state" --bio "$T/b10n.bio" > "$T/b10n.out" 2>&1
RVN=$(grep -c '^r	' "$T/b10n.bio")
CB=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/b10.out")
NB=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/b10n.out")
[ "$RVN" -eq 0 ] && awk -v c="$CB" -v n="$NB" 'BEGIN{exit !(c < n)}'
gate "B10 the red arm burns: court $CB < no-court $NB on the alien stay" $? 0
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 2 --steps 600 --island 0 --state "$T/b10.state" --bio "$T/b10.bio" >/dev/null 2>&1
HR=$(awk -F'\t' '$1=="a" && $2==16{print $3}' "$T/b10.bio")
[ "$HR" = "tri" ]
gate "B10 revocation is local: the mandate still acts at home (ep16=$HR)" $? 0
"$N" "$T/p3.bytes" "$T/kin.bytes" --reset --seed 5 --episodes 6 --steps 600 --island 0 --state "$T/b10k.state" --bio "$T/b10k.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/kin.bytes" --seed 5 --episodes 8 --steps 600 --island 1 --state "$T/b10k.state" --bio "$T/b10k.bio" >/dev/null 2>&1
RVK=$(grep -c '^r	' "$T/b10k.bio")
[ "$RVK" -eq 0 ]
gate "B10 kin transfer is not punished (0 revocations on the rotated island)" $? 0
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 8 --steps 600 --state "$T/b10z.state" --bio "$T/b10z.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 8 --steps 600 --no-island-court --state "$T/b10zn.state" --bio "$T/b10zn.bio" >/dev/null 2>&1
cmp -s "$T/b10z.bio" "$T/b10zn.bio" && cmp -s "$T/b10z.state" "$T/b10zn.state"
gate "B10 zero intervention where no second island exists (bit-identical)" $? 0
"$N" "$T/p3.bytes" "$T/alien.bytes" --reset --seed 5 --episodes 6 --steps 600 --island 0 --state "$T/b10s.state" --bio "$T/b10s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 4 --steps 600 --island 1 --state "$T/b10s.state" --bio "$T/b10s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 4 --steps 600 --island 1 --state "$T/b10s.state" --bio "$T/b10s.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --reset --seed 5 --episodes 6 --steps 600 --island 0 --state "$T/b10d.state" --bio "$T/b10d.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 8 --steps 600 --island 1 --state "$T/b10d.state" --bio "$T/b10d.bio" >/dev/null 2>&1
cmp -s "$T/b10s.bio" "$T/b10d.bio" && cmp -s "$T/b10s.state" "$T/b10d.state"
gate "B10 local records and revocations survive restart (split voyage identical)" $? 0

# The local plane is redundant evidence, not an unsigned second truth. A
# finite forged score used to pass every v12 check and could reverse a court
# verdict without touching the global record or the external biography.
cp "$T/b10d.state" "$T/b10f.state"
perl -e '
  use strict; use warnings;
  my $p = shift; my $record = 96; my $islands = 2;
  open my $f, "+<:raw", $p or die $!;
  seek($f, 0, 2) or die $!;
  my $off = tell($f) - $record * $islands + 24;
  seek($f, $off, 0) or die $!;
  read($f, my $raw, 8) == 8 or die "short local score";
  my $v = unpack("d", $raw);
  seek($f, $off, 0) or die $!;
  print {$f} pack("d", $v + 1.0) or die $!;
  close $f or die $!;
' "$T/b10f.state"
"$N" "$T/p3.bytes" "$T/alien.bytes" --episodes 0 \
    --state "$T/b10f.state" --bio "$T/b10d.bio" >/dev/null 2>&1
gate "B10 forged local court score is refused against the global record" $? 1

# --- B11: the island birth floor -----------------------------------------
# A fixed uniform hand is not a travelling model. On a deterministic
# full-byte null, every travelled byte witness fails to beat eight bits during
# the blind-comity window. At the next episode the island may choose null;
# disabling only this floor restores the inherited travelling hand.
LC_ALL=C awk 'BEGIN{s=7*7919+17; for(i=0;i<5000;i++){s=(s*1103515245+12345)%2147483648; printf "%c", int(s/65536)%256}}' > "$T/null.bytes"
"$N" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 \
    --state "$T/b11.state" --bio "$T/b11.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --island 1 \
    --state "$T/b11.state" --bio "$T/b11.bio" > "$T/b11e.out" 2>&1
N11U=$(tl "$T/b11e.out" atomic-uni)
N11B=$(tl "$T/b11e.out" byte-bi)
N11T=$(tl "$T/b11e.out" byte-tri)
awk -v u="$N11U" -v b="$N11B" -v t="$N11T" \
    'BEGIN{exit !(u >= 8.0 && b >= 8.0 && t >= 8.0)}'
gate "B11 no travelled byte witness beats uniform in the null window ($N11U/$N11B/$N11T)" $? 0
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --island 1 \
    --state "$T/b11.state" --bio "$T/b11.bio" > "$T/b11f.out" 2>&1
N11A=$(awk -F'\t' '$1=="a" && $2==8{print $3}' "$T/b11.bio")
N11F=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/b11f.out")
[ "$N11A" = "null" ] && [ "$N11F" = "8.000000" ] && \
    awk -F'\t' '$1=="r" && $2==8 && $5=="null"{ok=1} END{exit !ok}' "$T/b11.bio"
gate "B11 the island invokes fixed uniform null at the earned boundary (ep8=$N11A, $N11F bits)" $? 0

"$N" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 --no-birth-floor \
    --state "$T/b11r.state" --bio "$T/b11r.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --island 1 --no-birth-floor \
    --state "$T/b11r.state" --bio "$T/b11r.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --island 1 --no-birth-floor \
    --state "$T/b11r.state" --bio "$T/b11r.bio" > "$T/b11r.out" 2>&1
N11R=$(awk -F': ' '/^bits per raw byte/{print $2}' "$T/b11r.out")
N11RA=$(awk -F'\t' '$1=="a" && $2==8{print $3}' "$T/b11r.bio")
[ "$N11RA" = "tri" ] && awk -v f="$N11F" -v r="$N11R" \
    'BEGIN{exit !(f < r && r > 8.0)}'
gate "B11 red floor-off traveller burns: null $N11F < tri $N11R" $? 0

# Comity is a byte budget, not permission for an arbitrarily large first
# episode. With zero local receipts, an episode that would cross 1000 bytes is
# null in full; the floor-off twin demonstrates the inherited overrun.
"$N" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 \
    --state "$T/b11q.state" --bio "$T/b11q.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 2000 --island 1 \
    --state "$T/b11q.state" --bio "$T/b11q.bio" > "$T/b11q.out" 2>&1
Q11A=$(awk -F'\t' '$1=="a" && $2==7{print $3}' "$T/b11q.bio")
Q11=$(grep -c '^q	' "$T/b11q.bio")
"$N" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 --no-birth-floor \
    --state "$T/b11qr.state" --bio "$T/b11qr.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 2000 --island 1 --no-birth-floor \
    --state "$T/b11qr.state" --bio "$T/b11qr.bio" >/dev/null 2>&1
Q11R=$(awk -F'\t' '$1=="a" && $2==7{print $3}' "$T/b11qr.bio")
[ "$Q11A" = "null" ] && [ "$Q11" -eq 1 ] && [ "$Q11R" = "tri" ]
gate "B11 a long first episode cannot overrun blind comity (null vs red $Q11R)" $? 0

"$N" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 \
    --state "$T/b11d.state" --bio "$T/b11d.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 2 --steps 600 --island 1 \
    --state "$T/b11d.state" --bio "$T/b11d.bio" >/dev/null 2>&1
cmp -s "$T/b11.bio" "$T/b11d.bio" && cmp -s "$T/b11.state" "$T/b11d.state"
gate "B11 null verdict and actor count survive restart (split voyage identical)" $? 0

# The fifth-body law is enforced at home too: after any real travel, a
# structureless home island cannot keep a learned-confidence hand. The
# global court already prefers the least confident witness there; the
# island floor then chooses honest uniform ignorance.
awk 'BEGIN{s=31337; for(i=0;i<30000;i++){s=(s*1103515245+12345)%2147483648; printf "%c", int(s/65536)%256}}' > "$T/uhome.bytes"
"$N" "$T/uhome.bytes" "$T/p3.bytes" --reset --no-units --seed 5 --episodes 2 --steps 600 --island 0 --state "$T/b11h.state" --bio "$T/b11h.bio" > "$T/b11h0.out" 2>&1
"$N" "$T/uhome.bytes" "$T/p3.bytes" --no-units --seed 5 --episodes 1 --steps 600 --island 1 --state "$T/b11h.state" --bio "$T/b11h.bio" >/dev/null 2>&1
"$N" "$T/uhome.bytes" "$T/p3.bytes" --no-units --seed 5 --episodes 3 --steps 600 --island 0 --state "$T/b11h.state" --bio "$T/b11h.bio" >/dev/null 2>&1
H11=$(awk -F'\t' '$1=="a" && $2>=4{printf "%s", $3}' "$T/b11h.bio")
HN8=$(awk -F'\t' 'NF>=11 && $1>=4 && $8!="8.000000"{n++} END{print n+0}' "$T/b11h.bio")
HHD=$(awk '/^island 0: /{sub(/^.*digest=/, ""); print}' "$T/b11h0.out")
HREG=$(awk -F'\t' -v d="$HHD" '$1 == "i" && $4 == d {print $3}' "$T/b11h.bio")
HRV=$(awk -F'\t' -v r="$HREG" '$1=="r" && $3==r && $5=="null"{n++} END{print n+0}' "$T/b11h.bio")
[ "$H11" = "nullnullnull" ] && [ "$HN8" -eq 0 ] && [ "$HRV" -eq 3 ]
gate "B11 a structureless home is nulled after travel (fifth-body law enforced)" $? 0

# The fixed pseudo-random tape is not universally structureless once it is
# revisited: another causal seed re-enters learned spans and tri can earn the
# full 0.1-bit local margin. This bounds the seed-5 corollary rather than
# weakening the floor; earned structure must not be nulled by its filename.
"$N" "$T/uhome.bytes" "$T/p3.bytes" --reset --no-units --seed 16 \
    --episodes 2 --steps 600 --island 0 \
    --state "$T/b11hs.state" --bio "$T/b11hs.bio" >/dev/null 2>&1
"$N" "$T/uhome.bytes" "$T/p3.bytes" --no-units --seed 16 \
    --episodes 1 --steps 600 --island 1 \
    --state "$T/b11hs.state" --bio "$T/b11hs.bio" >/dev/null 2>&1
"$N" "$T/uhome.bytes" "$T/p3.bytes" --no-units --seed 16 \
    --episodes 1 --steps 600 --island 0 \
    --state "$T/b11hs.state" --bio "$T/b11hs.bio" >/dev/null 2>&1
H16=$(awk -F'\t' '$1=="a" && $2==4{print $3}' "$T/b11hs.bio")
H16R=$(awk -F'\t' '$1=="r" && $2==4{n++} END{print n+0}' "$T/b11hs.bio")
[ "$H16" = "tri" ] && [ "$H16R" -eq 0 ]
gate "A11 the fixed random home may earn a hand (seed 16 tri, not forced null)" $? 0

# --- A11: probation has an island-local door -----------------------------
# A de Bruijn order-2 island is the red world the incoming audit could not
# find: its trigram is predictive, every bigram row is uniform, no adjacency
# reaches birth support, and its alphabet is disjoint from the home units.
# The byte hand therefore acts while the local move shadow lacks the 0.1-bit
# promise. The old global door nevertheless sent the mover at episode 15.
awk '
function db(t,p,j) {
    if (t > 2) {
        if (2 % p == 0) for (j=1; j<=p; j++) seq[++n] = a[j]
    } else {
        a[t] = a[t-p]; db(t+1,p)
        for (j=a[t-p]+1; j<k; j++) { a[t]=j; db(t+1,t) }
    }
}
BEGIN {
    k=32; db(1,1)
    for (r=0; r<40; r++) for (i=1; i<=n; i++) printf "%c", 64+seq[i]
}' > "$T/db2.bytes"
"$N" "$T/rep.bytes" "$T/db2.bytes" --reset --seed 5 \
    --episodes 7 --steps 600 --island 0 \
    --state "$T/a11.state" --bio "$T/a11.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/db2.bytes" --seed 5 \
    --episodes 7 --steps 600 --island 1 \
    --state "$T/a11.state" --bio "$T/a11.bio" >/dev/null 2>&1
cp "$T/a11.state" "$T/a11r.state"
cp "$T/a11.bio" "$T/a11r.bio"
"$N" "$T/rep.bytes" "$T/db2.bytes" --seed 5 \
    --episodes 1 --steps 600 --island 1 \
    --state "$T/a11.state" --bio "$T/a11.bio" >"$T/a11.out" 2>&1
"$N" "$T/rep.bytes" "$T/db2.bytes" --seed 5 --no-local-probation \
    --episodes 1 --steps 600 --island 1 \
    --state "$T/a11r.state" --bio "$T/a11r.bio" >/dev/null 2>&1
A11=$(awk -F'\t' '$1=="a" && $2==15{print $3}' "$T/a11.bio")
A11R=$(awk -F'\t' '$1=="a" && $2==15{print $3}' "$T/a11r.bio")
A11B=$(awk -F'\t' '$1=="b" && $2>=8{n++} END{print n+0}' "$T/a11.bio")
A11M=$(awk '/^island 1 shadow move-bi/{gsub(",", "", $5); print $5}' "$T/a11.out")
A11U=$(awk '/^island 1 shadow move-bi/{print $8}' "$T/a11.out")
[ "$A11" = "tri" ] && [ "$A11R" = "mvp" ] && [ "$A11B" -eq 0 ] &&
    awk -v u="$A11U" -v m="$A11M" 'BEGIN{exit !(u-m < 0.1)}'
gate "A11 local shadow closes a foreign probation door (tri vs red $A11R; local gain $(awk -v u="$A11U" -v m="$A11M" 'BEGIN{printf "%.6f", u-m}'))" $? 0

# --- B12: the vocabulary pays rent ---------------------------------------
# A unit unrecognised for UNIT_TTL lived bytes dies: it leaves the living
# alphabet, keeps its identity and frozen counts, and renewed pair support
# resurrects the same name. Dead weight is measurable: the undead arm
# carries a wider Laplace alphabet on a world its units never match.
"$N" "$T/rep.bytes" "$T/uhome.bytes" --reset --seed 42 --episodes 1 --steps 4000 --island 0 --state "$T/b12.state" --bio "$T/b12.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 30 --steps 600 --island 1 --state "$T/b12.state" --bio "$T/b12.bio" > "$T/b12b.out" 2>&1
B12B=$(grep -c '^b	' "$T/b12.bio")
B12D=$(grep -c '^d	' "$T/b12.bio")
B12L=$(awk '/living of .* born/{print $2}' "$T/b12b.out")
[ "$B12B" -gt 0 ] && [ "$B12D" -eq "$B12B" ] && [ "$B12L" = "0" ]
gate "B12 an unrecognised vocabulary dies ($B12B born, $B12D deaths, $B12L living)" $? 0
DEP=$(awk -F'\t' '$1=="d"{print $2}' "$T/b12.bio" | sort -un | wc -l | tr -d ' ')
[ "$DEP" -gt 1 ]
gate "B12 deaths follow each unit's own rent clock, not one bell ($DEP distinct episodes)" $? 0
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 1 --steps 600 --island 1 --state "$T/b12.state" --bio "$T/b12.bio" > "$T/b12t.out" 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --reset --seed 42 --episodes 1 --steps 4000 --island 0 --no-unit-death --state "$T/b12n.state" --bio "$T/b12n.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 30 --steps 600 --island 1 --no-unit-death --state "$T/b12n.state" --bio "$T/b12n.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 1 --steps 600 --island 1 --no-unit-death --state "$T/b12n.state" --bio "$T/b12n.bio" > "$T/b12nt.out" 2>&1
TAU=$(grep '^this-life model atomic-uni' "$T/b12t.out")
TAUN=$(grep '^this-life model atomic-uni' "$T/b12nt.out")
TUU=$(awk '/^this-life model unit-uni/{print $NF}' "$T/b12t.out")
TUUN=$(awk '/^this-life model unit-uni/{print $NF}' "$T/b12nt.out")
[ -n "$TAU" ] && [ "$TAU" = "$TAUN" ] && awk -v d="$TUU" -v n="$TUUN" 'BEGIN{exit !(d < n)}'
gate "B12 dead weight is a measured tax: unit-uni $TUU < undead arm $TUUN on the same truth" $? 0
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 1 --steps 4000 --island 0 --state "$T/b12.state" --bio "$T/b12.bio" >/dev/null 2>&1
B12U=$(grep -c '^u	' "$T/b12.bio")
B12DUP=$(awk -F'\t' '$1=="b"{print $5}' "$T/b12.bio" | sort | uniq -d | wc -l | tr -d ' ')
B12UIN=$(awk -F'\t' -v born="$B12B" '$1=="u" && $3 >= born {bad=1} END{exit bad}' "$T/b12.bio"; echo $?)
[ "$B12U" -gt 0 ] && [ "$B12DUP" = "0" ] && [ "$B12UIN" = "0" ]
gate "B12 renewed support resurrects the same name, never a twin ($B12U resurrections)" $? 0
"$N" "$T/rep.bytes" "$T/uhome.bytes" --reset --seed 42 --episodes 1 --steps 4000 --island 0 --state "$T/b12s.state" --bio "$T/b12s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 26 --steps 600 --island 1 --state "$T/b12s.state" --bio "$T/b12s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 4 --steps 600 --island 1 --state "$T/b12s.state" --bio "$T/b12s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --reset --seed 42 --episodes 1 --steps 4000 --island 0 --state "$T/b12dd.state" --bio "$T/b12dd.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 30 --steps 600 --island 1 --state "$T/b12dd.state" --bio "$T/b12dd.bio" >/dev/null 2>&1
cmp -s "$T/b12s.bio" "$T/b12dd.bio" && cmp -s "$T/b12s.state" "$T/b12dd.state"
gate "B12 deaths and rent clocks survive restart (split extinction identical)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 --state "$T/b12z.state" --bio "$T/b12z.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 --no-unit-death --state "$T/b12zn.state" --bio "$T/b12zn.bio" >/dev/null 2>&1
cmp -s "$T/b12z.bio" "$T/b12zn.bio" && cmp -s "$T/b12z.state" "$T/b12zn.state"
gate "B12 zero intervention where no rent is due (bit-identical)" $? 0
printf '\x0e\x00\x00\x00' | dd of="$T/b12z.state" bs=1 seek=8 conv=notrunc 2>/dev/null
"$N" "$T/rep.bytes" --episodes 1 --steps 4000 --state "$T/b12z.state" --bio "$T/b12z.bio" >/dev/null 2>&1
gate "B12 a version-14 state cannot enter the local probation court" $? 1

# --- B13: tombstones keep history and lose authority --------------------
# Frozen counts are resurrection memory, not a vote. With every unit dead,
# the default searched player and the old-mass control start from the exact
# same state and truth addresses; a changed receipt proves the ghosts used to
# enter the one-ply distribution. Without navigation the common denominator
# cancels and the paired receipts remain identical, locating the influence.
B13UM=$(awk '/^tombstones:/{print $2}' "$T/b12b.out")
B13PM=$(awk '/^tombstones:/{print $6}' "$T/b12b.out")
[ "$B13UM" -gt 0 ] && [ "$B13PM" -gt 0 ]
gate "B13 death excludes frozen evidence from living mass ($B13UM unigram, $B13PM transition events)" $? 0

for arm in b13 b13g b13n b13ng; do
    cp "$T/b12s.state" "$T/$arm.state"
    cp "$T/b12s.bio" "$T/$arm.bio"
done
B13BASE=$(wc -l < "$T/b12s.bio")
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --start 16 --episodes 1 --steps 600 --island 1 \
    --state "$T/b13.state" --bio "$T/b13.bio" >"$T/b13.out" 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --start 16 --keep-dead-mass --episodes 1 --steps 600 --island 1 \
    --state "$T/b13g.state" --bio "$T/b13g.bio" >"$T/b13g.out" 2>&1
tail -n "+$((B13BASE+1))" "$T/b13.bio" | grep '^v' > "$T/b13.v"
tail -n "+$((B13BASE+1))" "$T/b13g.bio" | grep '^v' > "$T/b13g.v"
cmp -s "$T/b13.v" "$T/b13g.v"
B13DIFF=$?
B13REF=$(awk '/^mv control record:/{gsub(",", ""); print $12,$14,$16}' "$T/b13.out")
B13GREF=$(awk '/^mv control record:/{gsub(",", ""); print $12,$14,$16}' "$T/b13g.out")
[ "$B13DIFF" -eq 1 ] && [ -n "$B13REF" ] && [ "$B13REF" = "$B13GREF" ]
gate "B13 dead counts cannot steer searched life (red ghost arm diverges on matched truth)" $? 0

"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --no-mv-nav --start 16 --episodes 1 --steps 600 --island 1 \
    --state "$T/b13n.state" --bio "$T/b13n.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --no-mv-nav --start 16 --keep-dead-mass --episodes 1 --steps 600 \
    --island 1 --state "$T/b13ng.state" --bio "$T/b13ng.bio" >/dev/null 2>&1
tail -n "+$((B13BASE+1))" "$T/b13n.bio" | grep '^v' > "$T/b13n.v"
tail -n "+$((B13BASE+1))" "$T/b13ng.bio" | grep '^v' > "$T/b13ng.v"
cmp -s "$T/b13n.v" "$T/b13ng.v"
gate "B13 ghost influence is isolated to searched continuation (600 no-nav receipts identical)" $? 0

# Resurrection restores the frozen evidence exactly: when all identities are
# living again, the silent and historical denominators are the same number.
cp "$T/b12.state" "$T/b13r.state"
cp "$T/b12.bio" "$T/b13r.bio"
cp "$T/b12.state" "$T/b13rg.state"
cp "$T/b12.bio" "$T/b13rg.bio"
B13RBASE=$(wc -l < "$T/b12.bio")
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --start 16 --episodes 1 --steps 600 --island 0 \
    --state "$T/b13r.state" --bio "$T/b13r.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv \
    --start 16 --keep-dead-mass --episodes 1 --steps 600 --island 0 \
    --state "$T/b13rg.state" --bio "$T/b13rg.bio" >/dev/null 2>&1
tail -n "+$((B13RBASE+1))" "$T/b13r.bio" | grep '^v' > "$T/b13r.v"
tail -n "+$((B13RBASE+1))" "$T/b13rg.bio" | grep '^v' > "$T/b13rg.v"
cmp -s "$T/b13r.v" "$T/b13rg.v"
gate "B13 resurrection restores frozen evidence to living authority exactly" $? 0

# --- B14: the island registry --------------------------------------------
# An island's identity is its content, never its seat in today's convoy.
# The life keeps an append-only registry of every island it has met:
# arrivals are biography events, absent islands keep their memory, and
# the same content is always the same identity.
"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 1 --steps 600 --island 0 --state "$T/b14o1.state" --bio "$T/b14o1.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/p3.bytes" --reset --seed 5 --episodes 1 --steps 600 --island 1 --state "$T/b14o2.state" --bio "$T/b14o2.bio" >/dev/null 2>&1
cmp -s "$T/b14o1.bio" "$T/b14o2.bio" && cmp -s "$T/b14o1.state" "$T/b14o2.state"
gate "A14 simultaneous arrivals take causal identity, not CLI order" $? 0
"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 2 --steps 600 --island 0 --state "$T/b14a.state" --bio "$T/b14a.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 2 --steps 600 --island 1 --state "$T/b14a.state" --bio "$T/b14a.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 2 --steps 600 --island 0 --state "$T/b14b.state" --bio "$T/b14b.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/p3.bytes" --seed 5 --episodes 2 --steps 600 --island 0 --state "$T/b14b.state" --bio "$T/b14b.bio" >/dev/null 2>&1
cmp -s "$T/b14a.bio" "$T/b14b.bio" && cmp -s "$T/b14a.state" "$T/b14b.state"
gate "B14 the convoy order is invisible to the life (reversed resume bit-identical)" $? 0
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 2 --steps 600 --state "$T/b14c.state" --bio "$T/b14c.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 1 --steps 600 --island 1 --state "$T/b14c.state" --bio "$T/b14c.bio" >/dev/null 2>&1
rc=$?
NARR=$(grep -c '^i	' "$T/b14c.bio")
AEP=$(awk -F'\t' '$1=="i" && $3==1{print $2}' "$T/b14c.bio")
[ "$rc" -eq 0 ] && [ "$NARR" -eq 2 ] && [ "$AEP" = "2" ]
gate "B14 a new island joins a living life as an arrival event (episode $AEP)" $? 0
cp "$T/b14a.state" "$T/b14d.ref"
"$N" "$T/p3.bytes" --seed 5 --episodes 0 --steps 600 --state "$T/b14a.state" --bio "$T/b14a.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 0 --steps 600 --state "$T/b14a.state" --bio "$T/b14a.bio" >/dev/null 2>&1
cmp -s "$T/b14a.state" "$T/b14d.ref"
gate "B14 an absent island keeps its memory (state identical after the detour)" $? 0
"$N" "$T/p3.bytes" "$T/p3.bytes" --reset --seed 5 --episodes 1 --steps 64 --island 1 --state "$T/b14e.state" --bio "$T/b14e.bio" >/dev/null 2>&1
SAME=$(grep -c '^i	' "$T/b14e.bio")
[ "$SAME" -eq 1 ]
gate "B14 the same content is one identity, wherever it sits ($SAME arrival)" $? 0
"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 1 --steps 600 --island 0 --state "$T/b14f.state" --bio "$T/b14f.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 1 --steps 600 --island 1 --state "$T/b14f.state" --bio "$T/b14f.bio" >/dev/null 2>&1
cp "$T/b14f.state" "$T/b14g.state"
cp "$T/b14f.bio" "$T/b14g.bio"
SZ14=$(wc -c < "$T/b14g.state" | tr -d ' ')
OFF14=$((SZ14-256))
B14BYTE=$(od -An -tu1 -j "$OFF14" -N 1 "$T/b14g.state" | tr -d ' ')
B14FLIP=$((B14BYTE ^ 1))
printf "\\x$(printf %02x "$B14FLIP")" | dd of="$T/b14g.state" bs=1 seek="$OFF14" conv=notrunc 2>/dev/null
"$N" "$T/rep.bytes" --seed 5 --episodes 0 --steps 600 --island 0 --state "$T/b14g.state" --bio "$T/b14g.bio" >/dev/null 2>&1
gate "A14 an absent island cannot change identity behind its arrival receipt" $? 1
SZ14=$(wc -c < "$T/b14f.state" | tr -d ' ')
dd if="$T/b14f.state" of="$T/b14f.state" bs=1 skip=$((SZ14-256)) seek=$((SZ14-128)) count=24 conv=notrunc 2>/dev/null
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 1 --steps 600 --island 0 --state "$T/b14f.state" --bio "$T/b14f.bio" >/dev/null 2>&1
gate "B14 a forged duplicate island identity is refused" $? 1
printf '\x10\x00\x00\x00' | dd of="$T/b14c.state" bs=1 seek=8 conv=notrunc 2>/dev/null
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 1 --steps 600 --state "$T/b14c.state" --bio "$T/b14c.bio" >/dev/null 2>&1
gate "B14 a version-16 state cannot enter the witnessed registry" $? 1

# --- B15: the atlas ------------------------------------------------------
# Navigation is earned from local prequential records. New shores are
# charted before mature ones are ranked; exact ties answer to stable registry
# identity, never to today's CLI order. With one available identity the organ
# is absent from the event stream and the old life is bit-identical.
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 600 \
    --state "$T/b15n.state" --bio "$T/b15n.bio" > "$T/b15n.out" 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 600 --atlas \
    --state "$T/b15a.state" --bio "$T/b15a.bio" > "$T/b15a.out" 2>&1
cmp -s "$T/b15n.state" "$T/b15a.state" && \
cmp -s "$T/b15n.bio" "$T/b15a.bio" && \
cmp -s "$T/b15n.out" "$T/b15a.out"
gate "B15 a one-island atlas is an exact no-op" $? 0

"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 1 \
    --steps 600 --atlas --state "$T/b15o1.state" --bio "$T/b15o1.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/p3.bytes" --reset --seed 5 --episodes 1 \
    --steps 600 --atlas --state "$T/b15o2.state" --bio "$T/b15o2.bio" >/dev/null 2>&1
cmp -s "$T/b15o1.state" "$T/b15o2.state" && \
cmp -s "$T/b15o1.bio" "$T/b15o2.bio"
gate "B15 a fresh atlas cannot be steered by convoy order" $? 0

"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 2 \
    --steps 600 --island 0 --state "$T/b15c.state" --bio "$T/b15c.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --seed 5 --episodes 1 --steps 600 \
    --atlas --state "$T/b15c.state" --bio "$T/b15c.bio" >/dev/null 2>&1
C15=$(awk -F'\t' '$1=="t"{print $3}' "$T/b15c.bio")
CM15=$(awk -F'\t' '$1=="t"{print $4}' "$T/b15c.bio")
CL15=$(awk -F'\t' '$1=="t"{print $5}' "$T/b15c.bio")
CR15=$(awk -F'\t' '$1=="t"{print $6}' "$T/b15c.bio")
CE15=$(awk -F'\t' '$1=="t"{print $2}' "$T/b15c.bio")
awk -F'\t' -v e="$CE15" -v r="$C15" \
    'NF>=11 && $1==e {n++; if($3!=r) bad=1} END{exit !(n==600 && !bad)}' \
    "$T/b15c.bio"
rc=$?
[ "$CM15" = "chart" ] && [ "$CL15" -eq 0 ] && [ "$CR15" -ge 1000 ] || rc=1
gate "B15 an uncharted shore outranks a familiar one ($CL15 vs $CR15 bytes) and is lived" $rc 0

"$N" "$T/p3.bytes" "$T/uhome.bytes" --reset --no-units --seed 5 \
    --episodes 2 --steps 600 --island 0 --state "$T/b15e.state" \
    --bio "$T/b15e.bio" > "$T/b15e0.out" 2>&1
"$N" "$T/p3.bytes" "$T/uhome.bytes" --no-units --seed 5 \
    --episodes 2 --steps 600 --island 1 --state "$T/b15e.state" \
    --bio "$T/b15e.bio" >/dev/null 2>&1
P15D=$(awk '/^island 0: /{sub(/^.*digest=/, ""); print}' "$T/b15e0.out")
U15D=$(awk '/^island 1: /{sub(/^.*digest=/, ""); print}' "$T/b15e0.out")
P15R=$(awk -F'\t' -v d="$P15D" '$1=="i" && $4==d{print $3}' "$T/b15e.bio")
U15R=$(awk -F'\t' -v d="$U15D" '$1=="i" && $4==d{print $3}' "$T/b15e.bio")
cp "$T/b15e.state" "$T/b15m.state"
cp "$T/b15e.bio" "$T/b15m.bio"
"$N" "$T/p3.bytes" "$T/uhome.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --atlas --state "$T/b15e.state" \
    --bio "$T/b15e.bio" >/dev/null 2>&1
E15R=$(awk -F'\t' '$1=="t"{r=$3;m=$4} END{print r}' "$T/b15e.bio")
E15M=$(awk -F'\t' '$1=="t"{m=$4} END{print m}' "$T/b15e.bio")
E15S=$(awk -F'\t' '$1=="t"{s=$5} END{print s}' "$T/b15e.bio")
E15L=$(awk -F'\t' '$1=="t"{s=$6} END{print s}' "$T/b15e.bio")
[ "$E15M" = "earned" ] && [ "$E15R" = "$P15R" ] && \
awk -v w="$E15S" -v l="$E15L" 'BEGIN{exit !(w < l)}'
gate "B15 earned navigation chooses measured structural rhyme ($E15S vs $E15L bit/byte)" $? 0
"$N" "$T/p3.bytes" "$T/uhome.bytes" --no-units --seed 5 \
    --episodes 1 --steps 600 --island 1 --state "$T/b15m.state" \
    --bio "$T/b15m.bio" >/dev/null 2>&1
ME15=$(awk -F'\t' -v r="$U15R" 'NF>=11 && $1==5 && $3==r{n++} END{print n+0}' "$T/b15m.bio")
[ "$ME15" -eq 600 ] && ! cmp -s "$T/b15e.bio" "$T/b15m.bio"
gate "B15 the manual helm remains a red arm against atlas choice" $? 0

"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 4 \
    --steps 600 --atlas --state "$T/b15r.state" --bio "$T/b15r.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 2 \
    --steps 600 --atlas --state "$T/b15s.state" --bio "$T/b15s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" "$T/p3.bytes" --seed 5 --episodes 2 \
    --steps 600 --atlas --state "$T/b15s.state" --bio "$T/b15s.bio" >/dev/null 2>&1
cmp -s "$T/b15r.state" "$T/b15s.state" && \
cmp -s "$T/b15r.bio" "$T/b15s.bio"
gate "B15 atlas choices survive restart and a reversed convoy" $? 0

# --- B16: the neural core enters in shadow --------------------------------
# The buried prototype's lineage, under the zero courts: no backprop and a
# delta-rule readout. Its first surprise-gated Hebbian dynamics lost the
# matched frozen-reservoir arm and is quarantined behind --core-hebb-v1.
# A witness with a record and no power. The grave's scars run as gates:
# exact newborn ignorance, no saturation, no degenerate embeddings, no
# self-deception below the floor, and losing plasticity stays reproducible.
"$N" "$T/p3.bytes" --reset --seed 42 --episodes 1 --steps 1 --state "$T/b16n.state" --bio "$T/b16n.bio" > "$T/b16n.out" 2>&1
grep -q '^model core bits/byte 8\.000000$' "$T/b16n.out"
gate "B16 the newborn core prices exact ignorance (8.000000 first)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 --state "$T/b16r.state" --bio "$T/b16r.bio" > "$T/b16r.out" 2>&1
CORE_R=$(awk '/^model core /{print $NF}' "$T/b16r.out")
awk -v c="$CORE_R" 'BEGIN{exit !(c < 6.0)}'
gate "B16 the core learns within one life ($CORE_R on repetition)" $? 0
SATH=$(awk '/^core health:/{gsub(",",""); print $5}' "$T/b16r.out")
DEGEN=$(awk '/^core health:/{print $(NF-2)}' "$T/b16r.out")
awk -v s="$SATH" -v d="$DEGEN" 'BEGIN{exit !(s < 0.95 && d == 0)}'
gate "B16 the grave's scars stay closed (|h| $SATH, $DEGEN degenerate)" $? 0
i=0; : > "$T/ab16.bytes"
while [ $i -lt 2000 ]; do printf 'ab' >> "$T/ab16.bytes"; i=$((i+1)); done
"$N" "$T/ab16.bytes" --reset --seed 5 --episodes 1 --steps 2000 --state "$T/b16a.state" --bio "$T/b16a.bio" > "$T/b16a.out" 2>&1
CORE_A=$(awk '/^model core /{print $NF}' "$T/b16a.out")
BI_A=$(awk '/^model byte-bi /{print $NF}' "$T/b16a.out")
awk -v c="$CORE_A" -v b="$BI_A" 'BEGIN{exit !(b - c >= 0.5)}'
gate "B16 no cone, and a first win: core $CORE_A beats byte-bi $BI_A" $? 0
awk 'BEGIN{for(i=0;i<3000;i++) printf "aabab"}' > "$T/p5.bytes"
"$N" "$T/p5.bytes" --reset --seed 5 --episodes 4 --steps 2000 --state "$T/b16p.state" --bio "$T/b16p.bio" > "$T/b16p.out" 2>&1
CORE_P=$(awk '/^model core /{print $NF}' "$T/b16p.out")
TRI_P=$(awk '/^model byte-tri /{print $NF}' "$T/b16p.out")
awk -v c="$CORE_P" -v t="$TRI_P" 'BEGIN{exit !(t - c >= 0.5)}'
gate "B16 the core sees past the trigram floor: $CORE_P vs $TRI_P on period 5" $? 0
awk 'BEGIN{for(i=0;i<3000;i++) printf "aababa"}' > "$T/p6.bytes"
"$N" "$T/p6.bytes" --reset --seed 5 --episodes 4 --steps 2000 --state "$T/b16p6.state" --bio "$T/b16p6.bio" > "$T/b16p6.out" 2>&1
CORE_P6=$(awk '/^model core /{print $NF}' "$T/b16p6.out")
TRI_P6=$(awk '/^model byte-tri /{print $NF}' "$T/b16p6.out")
awk -v c="$CORE_P6" -v t="$TRI_P6" 'BEGIN{exit !(t - c >= 0.5)}'
gate "B16 frozen dynamics carry period 6: $CORE_P6 vs $TRI_P6" $? 0
awk 'BEGIN{for(i=0;i<3000;i++) printf "aababab"}' > "$T/p7.bytes"
"$N" "$T/p7.bytes" --reset --seed 5 --episodes 4 --steps 2000 --state "$T/b16w.state" --bio "$T/b16w.bio" > "$T/b16w.out" 2>&1
CORE_W=$(awk '/^model core /{print $NF}' "$T/b16w.out")
TRI_W=$(awk '/^model byte-tri /{print $NF}' "$T/b16w.out")
awk -v c="$CORE_W" -v t="$TRI_W" 'BEGIN{exit !(t - c >= 0.4)}'
gate "B16 frozen dynamics carry period 7: $CORE_W vs $TRI_W" $? 0
"$N" "$T/p7.bytes" --reset --seed 5 --episodes 4 --steps 2000 \
    --core-hebb-v1 --state "$T/b16wh.state" --bio "$T/b16wh.bio" > "$T/b16wh.out" 2>&1
CORE_WH=$(awk '/^model core /{print $NF}' "$T/b16wh.out")
TRI_WH=$(awk '/^model byte-tri /{print $NF}' "$T/b16wh.out")
awk -v c="$CORE_WH" -v t="$TRI_WH" 'BEGIN{exit !(c - t >= 5.0)}' && \
grep -q '^core plasticity: v1 gates .* (active)$' "$T/b16wh.out"
gate "B16 quarantined Hebb-v1 reproduces its period-7 loss: $CORE_WH vs $TRI_WH" $? 0
awk 'BEGIN{for(i=0;i<3000;i++) printf "aabababa"}' > "$T/p8.bytes"
"$N" "$T/p8.bytes" --reset --seed 5 --episodes 4 --steps 2000 --state "$T/b16p8.state" --bio "$T/b16p8.bio" > "$T/b16p8.out" 2>&1
CORE_P8=$(awk '/^model core /{print $NF}' "$T/b16p8.out")
TRI_P8=$(awk '/^model byte-tri /{print $NF}' "$T/b16p8.out")
awk -v c="$CORE_P8" -v t="$TRI_P8" 'BEGIN{exit !(t - c >= 0.5)}'
gate "B16 frozen dynamics carry period 8: $CORE_P8 vs $TRI_P8" $? 0
"$N" "$T/uhome.bytes" --reset --seed 7 --episodes 1 --steps 2000 --state "$T/b16u.state" --bio "$T/b16u.bio" > "$T/b16u.out" 2>&1
CORE_U=$(awk '/^model core /{print $NF}' "$T/b16u.out")
AT_U=$(awk '/^model atomic-uni /{print $NF}' "$T/b16u.out")
awk -v c="$CORE_U" -v a="$AT_U" 'BEGIN{exit !(c >= 7.95 && c < a)}'
gate "B16 honest ignorance under the floor law: core $CORE_U, atomic $AT_U" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 1000 --state "$T/b16z.state" --bio "$T/b16z.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 2 --steps 1000 --no-core --state "$T/b16zn.state" --bio "$T/b16zn.bio" >/dev/null 2>&1
cmp -s "$T/b16z.bio" "$T/b16zn.bio"
gate "B16 the shadow casts no shadow on the game (bio identical core on/off)" $? 0
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 1000 --state "$T/b16s.state" --bio "$T/b16s.bio" >/dev/null 2>&1
"$N" "$T/rep.bytes" --episodes 1 --steps 1000 --state "$T/b16s.state" --bio "$T/b16s.bio" >/dev/null 2>&1
cmp -s "$T/b16z.bio" "$T/b16s.bio" && cmp -s "$T/b16z.state" "$T/b16s.state"
gate "B16 core weights survive restart (split life identical)" $? 0
printf '\x12\x00\x00\x00' | dd of="$T/b16s.state" bs=1 seek=8 conv=notrunc 2>/dev/null
"$N" "$T/rep.bytes" --episodes 1 --steps 1000 --state "$T/b16s.state" --bio "$T/b16s.bio" >/dev/null 2>&1
gate "B16 a version-18 state cannot enter the witnessed core v20" $? 1

i=0; : > "$T/a16.bytes"
while [ $i -lt 1000 ]; do printf 'a' >> "$T/a16.bytes"; i=$((i+1)); done
"$N" "$T/a16.bytes" --reset --no-units --seed 5 --episodes 1 --steps 100 \
    --state "$T/b16f.state" --bio "$T/b16f.bio" >/dev/null 2>&1
# This one-island, one-trigram fixture leaves 164 bytes after core_bytes.
# Change the recorded 100 bytes to 99 without violating its loose bound;
# only the neural memory witness can name the partial forgery.
SZ16=$(wc -c < "$T/b16f.state" | tr -d ' ')
OFF16=$((SZ16 - 172))
printf '\x63' | dd of="$T/b16f.state" bs=1 seek="$OFF16" conv=notrunc 2>/dev/null
"$N" "$T/a16.bytes" --no-units --seed 5 --episodes 1 --steps 100 \
    --state "$T/b16f.state" --bio "$T/b16f.bio" >"$T/b16f.out" 2>&1
rc=$?
[ "$rc" -eq 1 ] && grep -q 'core memory disagrees with its witness' "$T/b16f.out"
gate "B16 a partial neural-memory forgery is refused by name" $? 0

# --- B17: the plasticity court --------------------------------------------
# Eight byte-identical shadow cores carry the resealed gene table. Genome
# zero is the frozen incumbent itself; the instrument changes no biography,
# survives a split life exactly, witnesses all its memory, and binds the
# neural invocation law so Hebb or the jury cannot silently disappear.
"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 200 --jury \
    --state "$T/b17j.state" --bio "$T/b17j.bio" > "$T/b17j.out" 2>&1
C17=$(awk '/^this-life model core /{print $NF}' "$T/b17j.out")
J17=$(awk '/^this-life jury genome 0 /{print $6}' "$T/b17j.out")
[ "$C17" = "$J17" ] && \
    [ "$(grep -c '^this-life jury genome ' "$T/b17j.out")" -eq 8 ] && \
    awk '/^this-life jury genome /{n++; if($12 != 0) bad=1} \
         END{exit !(n == 8 && !bad)}' "$T/b17j.out"
gate "B17 eight causal twins sit, and genome zero is the frozen core ($J17)" $? 0

"$N" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 200 \
    --state "$T/b17n.state" --bio "$T/b17n.bio" >/dev/null 2>&1
cmp -s "$T/b17j.bio" "$T/b17n.bio"
gate "B17 the jury changes no ordinary biography" $? 0

"$N" "$T/p5.bytes" --reset --seed 5 --episodes 2 --steps 300 --jury \
    --state "$T/b17f.state" --bio "$T/b17f.bio" > "$T/b17f.out" 2>&1
"$N" "$T/p5.bytes" --reset --seed 5 --episodes 1 --steps 300 --jury \
    --state "$T/b17s.state" --bio "$T/b17s.bio" >/dev/null 2>&1
"$N" "$T/p5.bytes" --seed 5 --episodes 1 --steps 300 --jury \
    --state "$T/b17s.state" --bio "$T/b17s.bio" > "$T/b17s.out" 2>&1
grep '^jury genome ' "$T/b17f.out" > "$T/b17f.table"
grep '^jury genome ' "$T/b17s.out" > "$T/b17s.table"
cmp -s "$T/b17f.state" "$T/b17s.state" && \
    cmp -s "$T/b17f.bio" "$T/b17s.bio" && \
    cmp -s "$T/b17f.table" "$T/b17s.table"
gate "B17 jury memory and final table are restart-exact" $? 0

"$N" "$T/p5.bytes" --seed 5 --episodes 1 --steps 100 \
    --state "$T/b17s.state" --bio "$T/b17s.bio" > "$T/b17law.out" 2>&1
rc=$?
[ "$rc" -eq 1 ] && grep -q 'neural invocation law changed' "$T/b17law.out"
gate "B17 a resumed jury cannot silently leave the room" $? 0

"$N" "$T/p5.bytes" --reset --core-hebb-v1 --seed 5 --episodes 1 \
    --steps 100 --state "$T/b17h.state" --bio "$T/b17h.bio" >/dev/null 2>&1
"$N" "$T/p5.bytes" --seed 5 --episodes 1 --steps 100 \
    --state "$T/b17h.state" --bio "$T/b17h.bio" > "$T/b17hebb.out" 2>&1
rc=$?
[ "$rc" -eq 1 ] && grep -q 'neural invocation law changed' "$T/b17hebb.out"
gate "B17 Hebb-v1 cannot silently become frozen across resume" $? 0

cp "$T/b17j.state" "$T/b17w.state"
SZ17=$(wc -c < "$T/b17w.state" | tr -d ' ')
OFF17=$((SZ17 - 8))
B17W=$(od -An -tu1 -j "$OFF17" -N 1 "$T/b17w.state" | tr -d ' ')
if [ "$B17W" -eq 0 ]; then B17X='\x01'; else B17X='\x00'; fi
printf "$B17X" | dd of="$T/b17w.state" bs=1 seek="$OFF17" conv=notrunc 2>/dev/null
"$N" "$T/rep.bytes" --jury --seed 42 --episodes 1 --steps 100 \
    --state "$T/b17w.state" --bio "$T/b17j.bio" > "$T/b17w.out" 2>&1
rc=$?
[ "$rc" -eq 1 ] && grep -q 'jury memory disagrees with its witness' "$T/b17w.out"
gate "B17 a partial jury-memory forgery is refused by name" $? 0

# --- A18: the arena's second hand ----------------------------------------
# The C corpus-preparation hand must agree with the sealed Python hand at
# adversarial boundaries, not only on the three friendly source files.  The
# fixed shuffle vector independently pins SplitMix64 state stepping, modulo
# draw, and the descending Fisher-Yates order.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/garena_prep.c \
    -o "$T/garena-prep" 2>"$T/garena-build.log"
rc=$?; [ -s "$T/garena-build.log" ] && rc=98
gate "A18 the arena's second hand builds strict and silent" $rc 0
printf 'head\r*** START OF THE PROJECT GUTENBERG EBOOK x\r\none\r\ntwo\r*** END OF THE PROJECT GUTENBERG EBOOK x' > "$T/g-valid.raw"
printf 'one\ntwo\n' > "$T/g-valid.want"
"$T/garena-prep" normalize "$T/g-valid.raw" "$T/g-valid.body" >/dev/null 2>&1
rc=$?; cmp -s "$T/g-valid.body" "$T/g-valid.want" || rc=98
gate "A18 CR-at-EOF, CRLF, and an END without newline match the sealed hand" $rc 0
printf 'prefix *** START OF THE PROJECT GUTENBERG EBOOK x\nbody\n*** END OF THE PROJECT GUTENBERG EBOOK x\n' > "$T/g-mid.raw"
"$T/garena-prep" normalize "$T/g-mid.raw" "$T/g-mid.body" >/dev/null 2>&1
gate "A18 a START marker in the middle of a line is refused" $? 1
printf '%s\n' '*** START OF THE PROJECT GUTENBERG EBOOK x' body \
    '*** END OF THE PROJECT GUTENBERG EBOOK x' tail \
    '*** END OF THE PROJECT GUTENBERG EBOOK y' > "$T/g-double-end.raw"
"$T/garena-prep" normalize "$T/g-double-end.raw" "$T/g-double-end.body" >/dev/null 2>&1
gate "A18 a second END seal is refused instead of silently ignored" $? 1
: > "$T/g-empty.body"
"$T/garena-prep" shuffle "$T/g-empty.body" "$T/g-empty.twin" >/dev/null 2>&1
rc=$?; [ ! -s "$T/g-empty.twin" ] || rc=98
gate "A18 an empty shuffle is defined and cannot underflow" $rc 0
i=0; : > "$T/g-vector.body"; : > "$T/g-vector.want"
while [ $i -lt 16 ]; do printf "\\x$(printf %02x $i)" >> "$T/g-vector.body"; i=$((i+1)); done
for h in 09 02 00 0c 08 03 0d 01 05 04 06 0f 07 0e 0b 0a; do
    printf "\\x$h" >> "$T/g-vector.want"
done
"$T/garena-prep" shuffle "$T/g-vector.body" "$T/g-vector.twin" >/dev/null 2>&1
rc=$?; cmp -s "$T/g-vector.twin" "$T/g-vector.want" || rc=98
gate "A18 the shuffle stream, modulo draw, and swap order match the sealed vector" $rc 0

# --- B18: the mouth ------------------------------------------------------
# Speech is a read-only instrument: the mouth prices nothing, learns
# nothing, appends nothing, saves nothing, and draws from its own
# stream. The elected seat speaks; a newborn and a stranger island are
# refused. Hostile flag combinations may succeed or refuse, but none may
# acquire a write path. Only the last two prompt bytes reach a byte hand.
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 6 --steps 600 --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>&1
cp "$T/b18.state" "$T/b18.state.ref"; cp "$T/b18.bio" "$T/b18.bio.ref"
"$N" --speak 0 --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.zero" 2>/dev/null
rc=$?
[ ! -s "$T/b18.zero" ] && cmp -s "$T/b18.state" "$T/b18.state.ref" && \
    cmp -s "$T/b18.bio" "$T/b18.bio.ref" || rc=98
gate "B18 zero requested bytes is still a read-only mouth invocation" $rc 0
"$N" --speak 60 --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.words" 2>/dev/null
rc=$?
cmp -s "$T/b18.state" "$T/b18.state.ref" && cmp -s "$T/b18.bio" "$T/b18.bio.ref" && [ "$rc" -eq 0 ]
gate "B18 the mouth writes no memory (state and biography untouched)" $? 0
"$N" --speak 60 --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.words2" 2>/dev/null
cmp -s "$T/b18.words" "$T/b18.words2"
gate "B18 the same seed speaks the same words" $? 0
"$N" "$W" --reset --seed 8 --episodes 1 --steps 600 --state "$T/b18b.state" --bio "$T/b18b.bio" >/dev/null 2>&1
"$N" --speak 60 --speak-seed 7 --state "$T/b18b.state" --bio "$T/b18b.bio" > "$T/b18b.words7" 2>/dev/null
"$N" --speak 60 --speak-seed 8 --state "$T/b18b.state" --bio "$T/b18b.bio" > "$T/b18b.words8" 2>/dev/null
cmp -s "$T/b18b.words7" "$T/b18b.words8"
gate "B18 a different seed speaks differently (red: not tautological)" $? 1
printf 'ab' > "$T/b18.p1"; printf 'bc' > "$T/b18.p2"
"$N" --speak 60 --speak-seed 7 --prompt-file "$T/b18.p1" --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.wp1" 2>/dev/null
"$N" --speak 60 --speak-seed 7 --prompt-file "$T/b18.p2" --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.wp2" 2>/dev/null
cmp -s "$T/b18.wp1" "$T/b18.wp2"
gate "B18 the prompt reaches the tongue (different prompts differ)" $? 1
printf 'older-prefix-ab' > "$T/b18.ps1"; printf 'other-history-ab' > "$T/b18.ps2"
"$N" --speak 60 --speak-seed 7 --prompt-file "$T/b18.ps1" --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.ws1" 2>/dev/null
"$N" --speak 60 --speak-seed 7 --prompt-file "$T/b18.ps2" --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.ws2" 2>/dev/null
cmp -s "$T/b18.ws1" "$T/b18.ws2"
gate "B18 byte speech sees exactly the last two prompt bytes" $? 0
"$N" --speak 60 --speak-seed 9 --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b18.words9" 2>/dev/null
grep -q '^cacbab' "$T/b18.words9"
gate "B18 an unprompted mouth opens on the most-lived context" $? 0
"$N" --speak 60 --state "$T/b18v.state" --bio "$T/b18v.bio" >/dev/null 2>&1
gate "B18 a newborn has nothing to say (no lived state refused)" $? 1
"$N" "$T/p3.bytes" --reset --episodes 0 --state "$T/b18e.state" --bio "$T/b18e.bio" >/dev/null 2>&1
cp "$T/b18e.state" "$T/b18e.state.ref"; cp "$T/b18e.bio" "$T/b18e.bio.ref"
"$N" --speak 1 --state "$T/b18e.state" --bio "$T/b18e.bio" >/dev/null 2>&1
rc=$?; cmp -s "$T/b18e.state" "$T/b18e.state.ref" || rc=98
cmp -s "$T/b18e.bio" "$T/b18e.bio.ref" || rc=98
gate "B18 a persisted zero-byte life still has nothing to say" $rc 1
"$N" "$T/rep.bytes" --speak 60 --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>&1
gate "B18 the mouth cannot meet new islands" $? 1

B18H=0
for flag in --reset --atlas --no-units --no-mv-nav --no-island-court \
    --no-birth-floor --no-local-probation --no-unit-death --keep-dead-mass \
    --no-core --core-hebb-v1 --jury; do
    cp "$T/b18.state.ref" "$T/b18h.state"
    cp "$T/b18.bio.ref" "$T/b18h.bio"
    "$N" --speak 8 "$flag" --state "$T/b18h.state" \
        --bio "$T/b18h.bio" >/dev/null 2>&1 || true
    cmp -s "$T/b18h.state" "$T/b18.state.ref" || B18H=98
    cmp -s "$T/b18h.bio" "$T/b18.bio.ref" || B18H=98
done
gate "B18 reset, atlas, and every law flag leave state and biography untouched" $B18H 0

for arm in b18d b18i; do
    cp "$T/b18.state.ref" "$T/$arm.state"
    cp "$T/b18.bio.ref" "$T/$arm.bio"
done
"$N" "$T/p3.bytes" --episodes 1 --steps 600 \
    --state "$T/b18d.state" --bio "$T/b18d.bio" >/dev/null 2>&1
"$N" --speak 80 --speak-seed 17 \
    --state "$T/b18i.state" --bio "$T/b18i.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --episodes 1 --steps 600 \
    --state "$T/b18i.state" --bio "$T/b18i.bio" >/dev/null 2>&1
cmp -s "$T/b18d.state" "$T/b18i.state" && cmp -s "$T/b18d.bio" "$T/b18i.bio"
gate "B18 interleaved speech consumes none of the life's rng stream" $? 0

"$N" --speak 24 --actor-lock mv --state "$T/b18.state" \
    --bio "$T/b18.bio" >/dev/null 2>"$T/b18mv.err"
rc=$?; grep -Eq 'hand (uni|bi|tri),' "$T/b18mv.err" || rc=98
gate "B18 an mv seat falls back to a named byte witness" $rc 0
"$N" --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>&1
gate "B18 speech-only controls cannot be silently ignored by ordinary life" $? 1

# --- B19: supported generative backoff ----------------------------------
# Laplace pseudo-counts are honest prices for externally supplied unseen
# truth; emitting those pseudo-counts as if they had been lived made 113 of
# 120 period-3 bytes leave the alphabet. The default mouth now samples only
# observed continuations at its deepest available context and backs off when
# a row is empty. The old law remains an explicit reproducible red arm.
"$N" --speak 600 --speak-seed 9 --state "$T/b18.state" \
    --bio "$T/b18.bio" > "$T/b19.words" 2>/dev/null
LC_ALL=C tr -d 'abc' < "$T/b19.words" > "$T/b19.outside"
[ ! -s "$T/b19.outside" ] && grep -q '^cacbabcacbab' "$T/b19.words"
gate "B19 supported backoff stays inside the lived transition graph" $? 0
printf 'zz' > "$T/b19.unseen"
"$N" --speak 24 --speak-seed 9 --prompt-file "$T/b19.unseen" \
    --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>"$T/b19.backoff"
grep -Eq 'speak support: uni [1-9][0-9]*,' "$T/b19.backoff"
gate "B19 an unseen prompt backs off to witnessed lower-order support" $? 0
"$N" --speak 120 --speak-seed 9 --speak-laplace \
    --state "$T/b18.state" --bio "$T/b18.bio" > "$T/b19.red" 2>/dev/null
B19R=$(LC_ALL=C tr -d 'abc' < "$T/b19.red" | wc -c | tr -d ' ')
[ "$B19R" -gt 0 ]
gate "B19 the Laplace red mouth leaves the lived alphabet ($B19R/120 bytes)" $? 0
cmp -s "$T/b18.state" "$T/b18.state.ref" && cmp -s "$T/b18.bio" "$T/b18.bio.ref"
gate "B19 repaired and red mouths are equally memory-silent" $? 0

# --- B20: the island's ear ----------------------------------------------
# Every island can carry its own statistical judge, grown from its
# immutable tape and nothing else. The ear prices speech with the
# island's own Laplace ladder, takes an exact substring census against
# the fully known tape, opens no state, and writes nothing. Each shore
# judges by its own book; destroying
# the order of a true slice while keeping its census must raise the
# price (red: the ear hears structure, not alphabet).
i=0; : > "$T/x3.bytes"
while [ $i -lt 700 ]; do printf 'xyzxzy' >> "$T/x3.bytes"; i=$((i+1)); done
head -c 200 "$T/p3.bytes" > "$T/b20.slice"
"$N" "$T/p3.bytes" "$T/x3.bytes" --ear "$T/b20.slice" > "$T/b20.out" 2>/dev/null
gate "B20 the ear hears a convoy of shores" $? 0
grep -q '^ear 0: .* longest-match=200 matched16=100.0%$' "$T/b20.out"
gate "B20 a verbatim slice is caught exactly by the substring census" $? 0
e0=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b20.out")
e1=$(sed -n 's/^ear 1: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b20.out")
awk -v a="$e0" -v b="$e1" 'BEGIN{exit !(a+2.0 < b)}'
gate "B20 each shore judges by its own book (home $e0, foreign $e1 bits/byte)" $? 0
grep -q '^ear 1: .* longest-match=0 ' "$T/b20.out"
gate "B20 a foreign stream matches nothing from a disjoint shore" $? 0
"$N" "$T/p3.bytes" --ear "$T/b20.slice" > "$T/b20.d1" 2>/dev/null
"$N" "$T/p3.bytes" --ear "$T/b20.slice" > "$T/b20.d2" 2>/dev/null
cmp -s "$T/b20.d1" "$T/b20.d2"
gate "B20 the ear is deterministic" $? 0
fold -w1 < "$T/b20.slice" | LC_ALL=C sort | tr -d '\n' > "$T/b20.sorted"
"$N" "$T/p3.bytes" --ear "$T/b20.sorted" > "$T/b20.sout" 2>/dev/null
es=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b20.sout")
awk -v a="$e0" -v b="$es" 'BEGIN{exit !(a + 1.0 < b)}'
gate "B20 the ear hears order, not census (sorted twin $es over true $e0) (red)" $? 0
mkdir "$T/b20dir"
( cd "$T/b20dir" && "$N" "$T/p3.bytes" --ear "$T/b20.slice" >/dev/null 2>&1 )
[ -z "$(ls -A "$T/b20dir")" ]
gate "B20 the ear writes nothing" $? 0
"$N" --ear "$T/b20.slice" >/dev/null 2>&1
gate "B20 the ear needs a shore" $? 1
"$N" "$T/p3.bytes" --ear "$T/b20.slice" --speak 5 >/dev/null 2>&1
gate "B20 the ear and the mouth are separate invocations" $? 1
"$N" "$T/p3.bytes" --ear "$T/b20.slice" --state "$T/p3.state" >/dev/null 2>&1
gate "B20 the ear judges a shore, never a life (state refused)" $? 1
"$N" "$T/p3.bytes" --ear "$T/b20.slice" --reset >/dev/null 2>&1
gate "B20 the ear judges a shore, never a life (reset refused)" $? 1

# The public cap is inclusive, and a longer stream is refused rather than
# silently truncated.  The old feof witness rejected the exact bound.
dd if=/dev/zero of="$T/b20.cap" bs=16384 count=1 2>/dev/null
"$N" "$T/p3.bytes" --ear "$T/b20.cap" > "$T/b20.cap.out" 2>/dev/null
grep -q 'bytes=16384 ' "$T/b20.cap.out"
gate "B20 the exact 16384-byte hearing bound is admitted" $? 0
cp "$T/b20.cap" "$T/b20.over"
printf x >> "$T/b20.over"
"$N" "$T/p3.bytes" --ear "$T/b20.over" >/dev/null 2>&1
gate "B20 a 16385-byte stream is refused, never truncated" $? 1

# Exact substring includes both tape boundaries, the threshold itself, and
# binary NUL.  Fifteen bytes remain below the declared match ruler.
printf '0123456789abcdef' > "$T/b20.q16"
printf 'X0123456789abcdefY' > "$T/b20.qstream"
"$N" "$T/b20.q16" --ear "$T/b20.qstream" > "$T/b20.qout" 2>/dev/null
grep -q 'longest-match=16 matched16=88.9%' "$T/b20.qout"
gate "B20 exact matcher holds both tape boundaries at the 16-byte law" $? 0
printf '\000abcdefghijklmn\000' > "$T/b20.qnul"
"$N" "$T/b20.qnul" --ear "$T/b20.qnul" > "$T/b20.qnul.out" 2>/dev/null
grep -q 'longest-match=16 matched16=100.0%' "$T/b20.qnul.out"
gate "B20 exact matcher is byte-exact through NUL" $? 0
printf '0123456789abcde' > "$T/b20.q15"
"$N" "$T/b20.q15" --ear "$T/b20.q15" > "$T/b20.q15.out" 2>/dev/null
grep -q 'longest-match=15 matched16=0.0%' "$T/b20.q15.out"
gate "B20 fifteen matched bytes stay below the sealed match threshold" $? 0
printf '0123456789abcdefX0123456789abcdef' > "$T/b20.qunion"
"$N" "$T/b20.q16" --ear "$T/b20.qunion" > "$T/b20.qunion.out" 2>/dev/null
grep -q 'longest-match=16 matched16=97.0%' "$T/b20.qunion.out"
gate "B20 overlapping match intervals form an exact coverage union" $? 0

# An ear invocation must not inspect the default life filenames in its cwd.
# Two hardlinked ambient defaults used to abort before ear() was entered.
mkdir "$T/b20ambient"
printf x > "$T/b20ambient/netta0.state"
ln "$T/b20ambient/netta0.state" "$T/b20ambient/netta0.bio.tsv"
( cd "$T/b20ambient" && "$N" "$T/p3.bytes" --ear "$T/b20.slice" \
    > "$T/b20.ambient" 2>/dev/null )
cmp -s "$T/b20.d1" "$T/b20.ambient"
gate "B20 ambient life defaults cannot enter the ear" $? 0

# Every explicit life or mouth control is hostile in an ear invocation.
# Refusal is about invocation, even when the named value equals its default.
ear_refuses() {
    if "$N" "$T/p3.bytes" --ear "$T/b20.slice" "$@" \
        >/dev/null 2>&1; then
        return 1
    fi
    return 0
}
rc=0
for flag in --reset --atlas --no-units --no-mv-nav --no-island-court \
    --no-birth-floor --no-local-probation --no-unit-death --keep-dead-mass \
    --no-core --core-hebb-v1 --jury; do
    ear_refuses "$flag" || rc=98
done
ear_refuses --seed 1 || rc=98
ear_refuses --episodes 1 || rc=98
ear_refuses --steps 64 || rc=98
ear_refuses --island 0 || rc=98
ear_refuses --start 0 || rc=98
ear_refuses --state unused || rc=98
ear_refuses --bio unused || rc=98
ear_refuses --actor-lock uni || rc=98
ear_refuses --speak-seed 1 || rc=98
ear_refuses --prompt-file "$T/b20.slice" || rc=98
ear_refuses --speak-laplace || rc=98
gate "B20 the ear refuses the complete life-and-mouth control surface" $rc 0

# Sorting is a world-local red arm, not a universal theorem.  On a shore
# whose own order is two blocks, the block-sorted stream beats its alternating
# census twin -- the opposite of the period-3 result above.
: > "$T/b20.blocks"
i=0; while [ $i -lt 100 ]; do printf a >> "$T/b20.blocks"; i=$((i+1)); done
i=0; while [ $i -lt 100 ]; do printf b >> "$T/b20.blocks"; i=$((i+1)); done
printf 'aaaabbbb' > "$T/b20.blocks.sorted"
printf 'abababab' > "$T/b20.blocks.alt"
"$N" "$T/b20.blocks" --ear "$T/b20.blocks.sorted" \
    > "$T/b20.blocks.sout" 2>/dev/null
"$N" "$T/b20.blocks" --ear "$T/b20.blocks.alt" \
    > "$T/b20.blocks.aout" 2>/dev/null
ebs=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' \
      "$T/b20.blocks.sout")
eba=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' \
      "$T/b20.blocks.aout")
awk -v a="$ebs" -v b="$eba" 'BEGIN{exit !(a < b)}'
gate "B20 sorted speech can win on a sorted shore ($ebs < $eba) (red)" $? 0

# --- B21: the ear remembers the question -------------------------------
# Cold warmup is a named mode, not an invisible property of every hearing.
# An explicit prompt contributes exactly its last two bytes, making every
# candidate byte a trigram-priced continuation without granting authority.
: > "$T/b21.shore"
i=0; while [ $i -lt 100 ]; do printf a >> "$T/b21.shore"; i=$((i+1)); done
printf 'zzbbbb' >> "$T/b21.shore"
printf aa > "$T/b21.aa"
printf bb > "$T/b21.bb"
printf zz > "$T/b21.ctx"
printf 'discarded-prefix-zz' > "$T/b21.ctxlong"
"$N" "$T/b21.shore" --ear "$T/b21.aa" > "$T/b21.aa.cold" 2>/dev/null
"$N" "$T/b21.shore" --ear "$T/b21.bb" > "$T/b21.bb.cold" 2>/dev/null
"$N" "$T/b21.shore" --ear "$T/b21.aa" --ear-context "$T/b21.ctx" \
    > "$T/b21.aa.hot" 2>/dev/null
"$N" "$T/b21.shore" --ear "$T/b21.bb" --ear-context "$T/b21.ctx" \
    > "$T/b21.bb.hot" 2>/dev/null
ac=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b21.aa.cold")
bc=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b21.bb.cold")
ah=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b21.aa.hot")
bh=$(sed -n 's/^ear 0: .*bits\/byte=\([0-9.]*\).*/\1/p' "$T/b21.bb.hot")
awk -v ac="$ac" -v bc="$bc" -v ah="$ah" -v bh="$bh" \
    'BEGIN{exit !(ac < bc && bh < ah)}'
gate "B21 explicit context reverses the constructed cold ranking ($ac<$bc; $bh<$ah)" $? 0
grep -q ' context=7a7a ' "$T/b21.aa.hot"
gate "B21 the hearing names its two-byte context" $? 0
"$N" "$T/b21.shore" --ear "$T/b21.aa" \
    --ear-context "$T/b21.ctxlong" > "$T/b21.aa.long" 2>/dev/null
cmp -s "$T/b21.aa.hot" "$T/b21.aa.long"
gate "B21 only the last two prompt bytes enter the ear" $? 0
"$N" "$T/b21.shore" --ear-context "$T/b21.ctx" >/dev/null 2>&1
gate "B21 context without a candidate ear is refused" $? 1
printf z > "$T/b21.shortctx"
"$N" "$T/b21.shore" --ear "$T/b21.aa" \
    --ear-context "$T/b21.shortctx" >/dev/null 2>&1
gate "B21 an underspecified one-byte context is refused" $? 1

# --- B22: the question's law -------------------------------------------
# A byte hand carries two context positions. A shorter prompt would mix a
# hidden byte of the most-lived opening into the question, the exact
# hidden-cold-byte class the contextual ear refuses. A question is at
# least two bytes everywhere, or it is absent and the opening is named
# cold. The two-byte arm proves the refusal is not tautological.
printf 'a' > "$T/b22.q1"
"$N" --speak 20 --speak-seed 7 --prompt-file "$T/b22.q1" \
    --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>"$T/b22.err"
rc=$?
[ "$rc" -eq 1 ] && grep -q 'a prompt needs at least two bytes' "$T/b22.err"
gate "B22 a one-byte question is refused by name" $? 0
: > "$T/b22.q0"
"$N" --speak 20 --speak-seed 7 --prompt-file "$T/b22.q0" \
    --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>&1
gate "B22 an empty question is not a silent cold opening" $? 1
printf 'ab' > "$T/b22.q2"
"$N" --speak 20 --speak-seed 7 --prompt-file "$T/b22.q2" \
    --state "$T/b18.state" --bio "$T/b18.bio" >/dev/null 2>&1
gate "B22 a two-byte question still reaches the tongue (red)" $? 0
cmp -s "$T/b18.state" "$T/b18.state.ref" && cmp -s "$T/b18.bio" "$T/b18.bio.ref"
gate "B22 refused questions leave no fingerprints on memory" $? 0

# The mouth and contextual ear now share one question reader.  Regular files
# are stable tail snapshots; non-regular inputs are questions sealed by EOF.
# Every complete source with the same final two bytes must therefore agree.
"$N" --speak 80 --speak-seed 7 --prompt-file "$T/b22.q2" \
    --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b22.regular" 2>/dev/null
printf 'discarded-prefix-ab' > "$T/b22.long"
"$N" --speak 80 --speak-seed 7 --prompt-file "$T/b22.long" \
    --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b22.long.out" 2>/dev/null
mkfifo "$T/b22.fifo"
( printf 'fifo-prefix-ab' > "$T/b22.fifo" ) & b22wp=$!
"$N" --speak 80 --speak-seed 7 --prompt-file "$T/b22.fifo" \
    --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b22.fifo.out" 2>/dev/null
wait "$b22wp"
printf 'stdin-prefix-ab' | "$N" --speak 80 --speak-seed 7 \
    --prompt-file /dev/stdin --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b22.stdin.out" 2>/dev/null
dd if=/dev/zero of="$T/b22.huge" bs=1048576 count=8 2>/dev/null
printf 'ab' >> "$T/b22.huge"
"$N" --speak 80 --speak-seed 7 --prompt-file "$T/b22.huge" \
    --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b22.huge.out" 2>/dev/null
cmp -s "$T/b22.regular" "$T/b22.long.out" && \
    cmp -s "$T/b22.regular" "$T/b22.fifo.out" && \
    cmp -s "$T/b22.regular" "$T/b22.stdin.out" && \
    cmp -s "$T/b22.regular" "$T/b22.huge.out"
gate "B22 regular, long, FIFO, stdin, and 8MiB questions share one tail law" $? 0

mkfifo "$T/b22.shortfifo"
( printf a > "$T/b22.shortfifo" ) & b22sp=$!
"$N" --speak 8 --prompt-file "$T/b22.shortfifo" \
    --state "$T/b18.state" --bio "$T/b18.bio" \
    >/dev/null 2>"$T/b22.shortfifo.err"
rc=$?; wait "$b22sp"
[ "$rc" -eq 1 ] && \
    grep -q 'a prompt needs at least two bytes' "$T/b22.shortfifo.err"
gate "B22 EOF seals a short streaming question before refusal" $? 0

mkfifo "$T/b22.earfifo"
( printf 'ear-prefix-zz' > "$T/b22.earfifo" ) & b22ep=$!
"$N" "$T/b21.shore" --ear "$T/b21.aa" \
    --ear-context "$T/b22.earfifo" > "$T/b22.earfifo.out" 2>/dev/null
wait "$b22ep"
cmp -s "$T/b21.aa.hot" "$T/b22.earfifo.out"
gate "B22 the same streaming question law reaches the ear" $? 0

# --- B23: every shore's structural twin -------------------------------
# The explicit twin is the already-sealed Gutenberg permutation grown in
# memory for each shore.  The independent arena hand must produce the same
# bytes, digest, and ear price.
"$T/garena-prep" shuffle "$T/p3.bytes" "$T/b23.p3.twin" >/dev/null 2>&1
"$N" "$T/p3.bytes" --ear "$T/b20.slice" --ear-twin \
    > "$T/b23.inline" 2>/dev/null
"$N" "$T/b23.p3.twin" --ear "$T/b20.slice" \
    > "$T/b23.external" 2>/dev/null
itd=$(sed -n 's/.* twin-digest=\([0-9a-f]*\) .*/\1/p' "$T/b23.inline")
etd=$(sed -n 's/^ear 0: digest=\([0-9a-f]*\) .*/\1/p' "$T/b23.external")
itp=$(sed -n 's/.* twin-bits\/byte=\([0-9.]*\)$/\1/p' "$T/b23.inline")
etp=$(sed -n 's/.* bits\/byte=\([0-9.]*\) .*/\1/p' "$T/b23.external")
[ -n "$itd" ] && [ "$itd" = "$etd" ] && [ "$itp" = "$etp" ]
gate "B23 the in-memory twin agrees with the independent sealed hand" $? 0

b23true=$(sed -n 's/.* bits\/byte=\([0-9.]*\) longest.*/\1/p' "$T/b23.inline")
grep -q 'longest-match=200 matched16=100.0%' "$T/b23.inline" && \
    awk -v a="$b23true" -v b="$itp" 'BEGIN{exit !(a < b)}'
gate "B23 a literal passage is named and dearer to the structural twin ($b23true<$itp)" $? 0

: > "$T/b23.constant"
i=0; while [ $i -lt 4096 ]; do printf a >> "$T/b23.constant"; i=$((i+1)); done
printf 'aaaaaaaaaaaaaaaaaaaaaaaa' > "$T/b23.a24"
"$N" "$T/b23.constant" --ear "$T/b23.a24" --ear-twin \
    > "$T/b23.constant.out" 2>/dev/null
b23ct=$(sed -n 's/.* bits\/byte=\([0-9.]*\) longest.*/\1/p' "$T/b23.constant.out")
b23cx=$(sed -n 's/.* twin-bits\/byte=\([0-9.]*\)$/\1/p' "$T/b23.constant.out")
grep -q ' twin-changed=0/4096 ' "$T/b23.constant.out" && \
    [ "$b23ct" = "$b23cx" ]
gate "B23 a degenerate twin says it changed nothing ($b23ct=$b23cx)" $? 0

"$N" "$T/p3.bytes" --ear-twin >/dev/null 2>&1
gate "B23 a structural twin exists only inside an explicit ear" $? 1
mkdir "$T/b23dir"
( cd "$T/b23dir" && "$N" "$T/p3.bytes" --ear "$T/b20.slice" \
    --ear-twin >/dev/null 2>&1 )
[ -z "$(ls -A "$T/b23dir")" ]
gate "B23 the structural twin is grown in memory and writes nothing" $? 0

# The uniform replay red is generated from a named SplitMix64 stream.  It
# demonstrates why price, exact-match coverage, and twin price must remain a
# triple: a cheap trigram mouth can simply walk a memorised random tape.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/uniform_shore.c \
    -o "$T/uniform-shore" 2>"$T/b23.uniform.build"
rc=$?; [ -s "$T/b23.uniform.build" ] && rc=98
"$T/uniform-shore" 4096 0x534f4c554e49464f > "$T/b23.uniform"
b23vec=$(od -An -tx1 -N16 "$T/b23.uniform" | tr -d ' \n')
[ "$b23vec" = "14a06a602648d491483f08d4faee0afe" ] || rc=98
gate "B23 the uniform red shore has an exact portable generator" $rc 0
dd if="$T/b23.uniform" of="$T/b23.prompt" bs=1 skip=14 count=2 2>/dev/null
"$N" "$T/b23.uniform" --reset --seed 7 --episodes 1 --steps 302 \
    --start 16 --state "$T/b23.state" --bio "$T/b23.bio" >/dev/null 2>&1
for hand in uni bi tri; do
    "$N" --speak 300 --speak-seed 7 --actor-lock "$hand" \
        --prompt-file "$T/b23.prompt" --state "$T/b23.state" \
        --bio "$T/b23.bio" > "$T/b23.$hand" 2>"$T/b23.$hand.err"
    "$N" "$T/b23.uniform" --ear "$T/b23.$hand" \
        --ear-context "$T/b23.prompt" --ear-twin \
        > "$T/b23.$hand.ear" 2>/dev/null
done
b23u=$(sed -n 's/.* bits\/byte=\([0-9.]*\) longest.*/\1/p' "$T/b23.uni.ear")
b23b=$(sed -n 's/.* bits\/byte=\([0-9.]*\) longest.*/\1/p' "$T/b23.bi.ear")
b23t=$(sed -n 's/.* bits\/byte=\([0-9.]*\) longest.*/\1/p' "$T/b23.tri.ear")
b23tx=$(sed -n 's/.* twin-bits\/byte=\([0-9.]*\)$/\1/p' "$T/b23.tri.ear")
awk -v u="$b23u" -v b="$b23b" -v t="$b23t" \
    'BEGIN{exit !(t < b && b < u)}' && \
    grep -q 'longest-match=300 matched16=100.0%' "$T/b23.tri.ear" && \
    grep -q 'speak support: uni 0, bi 0, tri 300' "$T/b23.tri.err"
gate "B23 low trigram price can be a 300-byte literal replay ($b23t<$b23b<$b23u)" $? 0
awk -v t="$b23t" -v x="$b23tx" 'BEGIN{exit !(t < x)}'
gate "B23 the preregistered twin coordinate exposes that replay ($b23t<$b23tx)" $? 0

# --- B24: the pattern court ---------------------------------------------
# The court reads the sealed triple and returns one verdict from a
# four-word lattice frozen from named calibration worlds before any
# candidate was read: abstain when the twin changed nothing, replay at
# matched-16 >= 50, order at structure gap >= 0.5 bits/byte, stranger
# otherwise. Verdicts are measurement patterns, never causal
# accusations: the independently written cycle below replays a shore
# no hand ever copied. The court holds no office and writes nothing.
"$N" "$T/p3.bytes" --court "$T/b20.slice" > "$T/b24.a" 2>/dev/null
grep -q 'verdict=replay receipt=' "$T/b24.a"
gate "B24 a literal slice is judged replay" $? 0
i=0; : > "$T/b24.ind"
while [ $i -lt 20 ]; do printf 'cacbab' >> "$T/b24.ind"; i=$((i+1)); done
"$N" "$T/p3.bytes" --court "$T/b24.ind" > "$T/b24.b" 2>/dev/null
grep -q 'verdict=replay receipt=' "$T/b24.b"
gate "B24 an independently written cycle is judged replay (pattern, not cause)" $? 0
i=0; : > "$T/b24.ord"
while [ $i -lt 25 ]; do printf 'abcacbabcacz' >> "$T/b24.ord"; i=$((i+1)); done
"$N" "$T/p3.bytes" --court "$T/b24.ord" > "$T/b24.g" 2>/dev/null
grep -q 'verdict=order receipt=' "$T/b24.g"
gate "B24 short lived runs with a foreign heartbeat are judged order" $? 0
i=0; : > "$T/b24.zzz"
while [ $i -lt 300 ]; do printf 'z' >> "$T/b24.zzz"; i=$((i+1)); done
"$N" "$T/p3.bytes" --court "$T/b24.zzz" > "$T/b24.d" 2>/dev/null
grep -q 'P=8.013738 Q=8.013738 .*gap-micro=0 .*verdict=abstain receipt=' "$T/b24.d"
gate "B25 a foreign stream invisible to the structural null now abstains" $? 0
i=0; : > "$T/b24.const"
while [ $i -lt 4096 ]; do printf 'a' >> "$T/b24.const"; i=$((i+1)); done
head -c 300 "$T/b24.const" > "$T/b24.cs"
"$N" "$T/b24.const" --court "$T/b24.cs" > "$T/b24.e" 2>/dev/null
grep -q 'matched16=100.0000% .*verdict=abstain receipt=' "$T/b24.e"
gate "B24 a shore whose twin cannot move abstains over a perfect match" $? 0
head -c 168 "$T/p3.bytes" > "$T/b24.hi"
i=0; while [ $i -lt 60 ]; do printf 'x' >> "$T/b24.hi"; i=$((i+1)); done
head -c 84 "$T/p3.bytes" > "$T/b24.lo"
i=0; while [ $i -lt 144 ]; do printf 'x' >> "$T/b24.lo"; i=$((i+1)); done
"$N" "$T/p3.bytes" --court "$T/b24.hi" > "$T/b24.h" 2>/dev/null
"$N" "$T/p3.bytes" --court "$T/b24.lo" > "$T/b24.i" 2>/dev/null
grep -q 'verdict=replay receipt=' "$T/b24.h" && grep -q 'verdict=order receipt=' "$T/b24.i"
gate "B24 the match threshold bites: a straddle pair flips the verdict (red)" $? 0
"$N" "$T/p3.bytes" --court "$T/b24.zzz" --ear-context "$T/b18.p1" \
    > "$T/b24.ctx" 2>/dev/null
grep -q 'context=6162 ' "$T/b24.ctx"
gate "B24 an explicit context enters the court and is named" $? 0
"$N" "$T/p3.bytes" --court "$T/b24.zzz" --ear "$T/b24.zzz" >/dev/null 2>&1
gate "B24 the court and the ear are separate invocations" $? 1
"$N" "$T/p3.bytes" --court "$T/b24.zzz" --ear-twin >/dev/null 2>&1
gate "B24 the court grows its own twin and refuses the flag" $? 1
"$N" "$T/p3.bytes" --court "$T/b24.zzz" --speak 5 >/dev/null 2>&1
gate "B24 the court and the mouth are separate invocations" $? 1
"$N" "$T/p3.bytes" --court "$T/b24.zzz" --seed 9 >/dev/null 2>&1
gate "B24 the court refuses every life control" $? 1
"$N" --court "$T/b24.zzz" >/dev/null 2>&1
gate "B24 the court needs a shore" $? 1
mkdir "$T/b24dir"
( cd "$T/b24dir" && "$N" "$T/p3.bytes" --court "$T/b20.slice" \
    >/dev/null 2>&1 )
[ -z "$(ls -A "$T/b24dir")" ]
gate "B24 the court writes nothing" $? 0

# --- B25: the court earns a public warrant -----------------------------
# Every deciding operand is printed. Exact integer match coverage fixes the
# rounded-threshold contradiction; a structural null must move the measured
# candidate coordinate at the declared one-microbit/byte resolution before
# the court may render a non-abstaining verdict.
head -c 1000 "$T/p3.bytes" > "$T/b25.tie"
i=0; while [ $i -lt 1000 ]; do printf 'x' >> "$T/b25.tie"; i=$((i+1)); done
cp "$T/b25.tie" "$T/b25.below"
printf 'x' >> "$T/b25.below"
"$N" "$T/p3.bytes" --court "$T/b25.tie" > "$T/b25.tie.out" 2>/dev/null
"$N" "$T/p3.bytes" --court "$T/b25.below" > "$T/b25.below.out" 2>/dev/null
grep -q 'matched16=50.0000% matched-bytes=1000/2000 .*verdict=replay receipt=' \
    "$T/b25.tie.out" && \
grep -q 'matched16=49.9750% matched-bytes=1000/2001 .*verdict=order receipt=' \
    "$T/b25.below.out"
gate "B25 the public exact fraction reconstructs both sides of the 50% law" $? 0

: > "$T/b25.near"
i=0; while [ $i -lt 2048 ]; do printf 'a' >> "$T/b25.near"; i=$((i+1)); done
printf 'b' >> "$T/b25.near"
i=0; while [ $i -lt 2047 ]; do printf 'a' >> "$T/b25.near"; i=$((i+1)); done
head -c 300 "$T/b24.const" > "$T/b25.aaa"
"$N" "$T/b25.near" --court "$T/b25.aaa" > "$T/b25.near.out" 2>/dev/null
grep -q 'P=0.087545 Q=0.087545 .*G=0.000000 gap-micro=0 changed=2/4096 verdict=abstain receipt=' \
    "$T/b25.near.out"
gate "B25 moved bytes without a moved measurement do not confer jurisdiction" $? 0

head -c 15 "$T/p3.bytes" > "$T/b25.fragment"
: > "$T/b25.quilt"
i=0; while [ $i -lt 100 ]; do
    cat "$T/b25.fragment" >> "$T/b25.quilt"
    printf 'x' >> "$T/b25.quilt"
    i=$((i+1))
done
"$N" "$T/p3.bytes" --court "$T/b25.quilt" > "$T/b25.quilt.out" 2>/dev/null
grep -q 'matched16=0.0000% matched-bytes=0/1600 .*verdict=order receipt=' \
    "$T/b25.quilt.out"
gate "B25 a quilt of sub-threshold literals remains structural order" $? 0

: > "$T/b25.census"
i=0; while [ $i -lt 100 ]; do printf 'a' >> "$T/b25.census"; i=$((i+1)); done
i=0; while [ $i -lt 100 ]; do printf 'b' >> "$T/b25.census"; i=$((i+1)); done
i=0; while [ $i -lt 100 ]; do printf 'c' >> "$T/b25.census"; i=$((i+1)); done
"$N" "$T/p3.bytes" --court "$T/b25.census" > "$T/b25.census.out" 2>/dev/null
grep -q 'verdict=stranger receipt=' "$T/b25.census.out"
gate "B25 the trigram court still names its equal-census blind spot" $? 0

printf 'ab' > "$T/b25.ab"
printf 'cb' > "$T/b25.cb"
"$N" "$T/p3.bytes" --court "$T/b25.ab" > "$T/b25.cold" 2>/dev/null
"$N" "$T/p3.bytes" --court "$T/b25.ab" --ear-context "$T/b25.cb" \
    > "$T/b25.hot" 2>/dev/null
grep -q 'context=cold .*verdict=stranger receipt=' "$T/b25.cold" && \
grep -q 'context=6362 .*verdict=order receipt=' "$T/b25.hot"
gate "B25 context may change a verdict only while naming its jurisdiction" $? 0

# --- B26: the warrant's receipt -----------------------------------------
# A verdict is citable only through a receipt an external hand can
# recompute, and every warrant names the law it was judged under. The
# external hand re-derives every verdict from printed operands alone
# and recomputes every receipt against the named law-digest; a changed
# operand, verdict, receipt, or silently amended law text refuses the
# transcript by line. The receipt authenticates the tuple; it grants
# nothing.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/warrant_check.c \
    -lm -o "$T/wcheck" 2>"$T/wcheck-build.log"
rc=$?; [ -s "$T/wcheck-build.log" ] && rc=98
gate "B26 the warrant's external hand builds strict and silent" $rc 0
{ "$N" "$T/p3.bytes" --court "$T/b20.slice"
  "$N" "$T/p3.bytes" --court "$T/b24.ord"
  "$N" "$T/p3.bytes" --court "$T/b24.zzz"
  "$N" "$T/b24.const" --court "$T/b24.cs"
  "$N" "$T/p3.bytes" --court "$T/b24.lo"
} > "$T/b26.transcript" 2>/dev/null
"$T/wcheck" < "$T/b26.transcript" > "$T/b26.accept"
rc=$?; grep -q '^warrant accepted: 5 verdicts under law ' "$T/b26.accept" || rc=98
gate "B26 five hearings are accepted by the external hand" $rc 0
grep -q '^court law: pattern-court law v2: ' "$T/b26.transcript"
gate "B26 every sitting opens by naming its law and law-digest" $? 0
"$N" "$T/p3.bytes" --court "$T/b20.slice" 2>/dev/null | grep '^court law:' \
    > "$T/b26.law1"
"$N" "$T/p3.bytes" --court "$T/b24.zzz" 2>/dev/null | grep '^court law:' \
    > "$T/b26.law2"
cmp -s "$T/b26.law1" "$T/b26.law2"
gate "B26 the law-digest is one law for every hearing" $? 0
sed 's/gap-micro=0 /gap-micro=1 /' "$T/b26.transcript" > "$T/b26.t1"
"$T/wcheck" < "$T/b26.t1" > "$T/b26.r1"
rc=$?
[ "$rc" -eq 1 ] && grep -q '^warrant refused: line ' "$T/b26.r1"
gate "B26 a tampered operand is refused by name (red)" $? 0
sed 's/verdict=order/verdict=replay/' "$T/b26.transcript" > "$T/b26.t2"
"$T/wcheck" < "$T/b26.t2" >/dev/null 2>&1
gate "B26 a tampered verdict is refused (red)" $? 1
sed 's/receipt=\(............\)..../receipt=\1beef/' "$T/b26.transcript" \
    > "$T/b26.t3"
"$T/wcheck" < "$T/b26.t3" >/dev/null 2>&1
gate "B26 a tampered receipt is refused (red)" $? 1
sed 's/order if gap-micro>=500000/order if gap-micro>=500001/' \
    "$T/b26.transcript" > "$T/b26.t4"
"$T/wcheck" < "$T/b26.t4" > "$T/b26.r4"
rc=$?
[ "$rc" -eq 1 ] && grep -q 'law is not canonical v2' "$T/b26.r4"
gate "B26 a silently amended law is refused by its own digest (red)" $? 0
grep -v '^court law:' "$T/b26.transcript" | "$T/wcheck" >/dev/null 2>&1
gate "B26 a verdict without a law is refused (red)" $? 1

# --- B27: the warrant's sitting docket ---------------------------------
# Individually valid leaves do not prove a complete book. A sitting now
# names the exact candidate, context, and shore count, then closes with a
# digest over the ordered header and every full warrant line. The external
# hand is a strict framing and canonical-grammar parser. The fixture hand is
# deliberately public: it can rebuild FNV witnesses, proving that malformed
# re-sealed records are refused by grammar rather than by secrecy.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/warrant_fixture.c \
    -o "$T/wfixture" 2>"$T/wfixture-build.log"
rc=$?; [ -s "$T/wfixture-build.log" ] && rc=98
gate "B27 the public red constructor builds strict and silent" $rc 0

"$N" "$T/p3.bytes" "$T/b24.const" --court "$T/b20.slice" \
    > "$T/b27.full" 2>/dev/null
"$T/wcheck" < "$T/b27.full" > "$T/b27.accept"
rc=$?
grep -q '^warrant accepted: 2 verdicts under law .* in 1 complete sittings$' \
    "$T/b27.accept" || rc=98
gate "B27 one two-shore sitting is accepted only with its close" $rc 0

"$N" "$T/p3.bytes" --court "$T/b20.slice" > "$T/b27.one" 2>/dev/null
"$N" "$T/p3.bytes" "$T/b24.const" --court "$T/b24.zzz" \
    > "$T/b27.other" 2>/dev/null
b27a=$(sed -n '3p' "$T/b27.one" | sed -E 's/.*candidate-digest=([^ ]+).*/\1/')
b27b=$(sed -n '3p' "$T/b27.full" | sed -E 's/.*candidate-digest=([^ ]+).*/\1/')
b27c=$(sed -n '3p' "$T/b27.other" | sed -E 's/.*candidate-digest=([^ ]+).*/\1/')
b27x=$("$T/wfixture" bytes < "$T/b20.slice")
[ "$b27a" = "$b27b" ] && [ "$b27a" = "$b27x" ] && [ "$b27a" != "$b27c" ]
gate "B27 candidate identity is external, convoy-stable, and byte-sensitive" $? 0

sed '5d' "$T/b27.full" > "$T/b27.drop"
"$T/wcheck" < "$T/b27.drop" >/dev/null 2>&1
gate "B27 dropping a valid leaf breaks the sitting" $? 1
{ sed -n '1,4p' "$T/b27.full"; sed -n '4,6p' "$T/b27.full"; } \
    > "$T/b27.duplicate"
"$T/wcheck" < "$T/b27.duplicate" >/dev/null 2>&1
gate "B27 duplicating a valid leaf breaks the sitting" $? 1
{ sed -n '1,3p' "$T/b27.full"; sed -n '5p' "$T/b27.full";
  sed -n '4p' "$T/b27.full"; sed -n '6p' "$T/b27.full"; } \
    > "$T/b27.reorder"
"$T/wcheck" < "$T/b27.reorder" >/dev/null 2>&1
gate "B27 reordering valid leaves breaks the sitting" $? 1
{ sed -n '1,4p' "$T/b27.full"; sed -n '5p' "$T/b27.other";
  sed -n '6p' "$T/b27.full"; } > "$T/b27.splice"
"$T/wcheck" < "$T/b27.splice" >/dev/null 2>&1
gate "B27 cross-candidate splicing breaks the sitting" $? 1

cat "$T/b27.full" "$T/b27.other" > "$T/b27.concat"
"$T/wcheck" < "$T/b27.concat" > "$T/b27.concat.out"
rc=$?
grep -q '4 verdicts .* in 2 complete sittings$' "$T/b27.concat.out" || rc=98
gate "B27 concatenated complete sittings remain a legal book" $rc 0
{ sed -n '1,2p' "$T/b27.full"; sed -n '2,6p' "$T/b27.full"; } \
    > "$T/b27.double-law"
"$T/wcheck" < "$T/b27.double-law" >/dev/null 2>&1
gate "B27 a repeated law cannot restart an unfinished sitting" $? 1
sed 's/$/\r/' "$T/b27.full" > "$T/b27.crlf"
"$T/wcheck" < "$T/b27.crlf" >/dev/null 2>&1
gate "B27 CRLF transport preserves a complete sitting" $? 0

printf '%s\n' 'pattern-court law v999: a digest is not semantics' \
    | "$T/wfixture" hash > "$T/b27.alien.digest"
b27ad=$(cat "$T/b27.alien.digest")
{ printf 'NETTA ZERO\n';
  printf 'court law: pattern-court law v999: a digest is not semantics law-digest=%s\n' \
      "$b27ad";
} > "$T/b27.alien"
"$T/wcheck" < "$T/b27.alien" > "$T/b27.alien.out"
rc=$?
[ "$rc" -eq 1 ] && grep -q 'law is not canonical v2' "$T/b27.alien.out"
gate "B27 a self-consistent foreign law cannot borrow v2 semantics" $? 0

b27law=$(sed -n '2p' "$T/b27.one" | sed -E 's/.*law-digest=//')
sed -n '4p' "$T/b27.one" | sed -E 's/ receipt=[0-9a-f]+$//' \
    > "$T/b27.base"
seal_bad() {
    bad=$1
    "$T/wfixture" receipt "$b27law" < "$T/$bad.base" > "$T/$bad.line" || return 2
    { sed -n '3p' "$T/b27.one"; cat "$T/$bad.line"; } \
        | "$T/wfixture" docket > "$T/$bad.docket" || return 2
    bd=$(cat "$T/$bad.docket")
    { sed -n '1,3p' "$T/b27.one"; cat "$T/$bad.line";
      printf 'court close: verdicts=1 docket=%s\n' "$bd"; } > "$T/$bad"
}

sed 's/gap-micro=[^ ]*/gap-micro=garbage/;s/verdict=replay/verdict=abstain/' \
    "$T/b27.base" > "$T/b27.badnum.base"
seal_bad b27.badnum
"$T/wcheck" < "$T/b27.badnum" >/dev/null 2>&1
gate "B27 a re-sealed malformed integer is refused by grammar" $? 1

sed 's/^court 0:/court 9999999999999999999999999999999:/' \
    "$T/b27.base" > "$T/b27.overflow.base"
seal_bad b27.overflow
"$T/wcheck" < "$T/b27.overflow" >/dev/null 2>&1
gate "B27 a re-sealed overflowing integer is refused without conversion UB" $? 1

sed -E 's/ P=[^ ]+ Q=[^ ]+ matched16=[^ ]+//;s/ G=[^ ]+//' \
    "$T/b27.base" > "$T/b27.missing.base"
seal_bad b27.missing
"$T/wcheck" < "$T/b27.missing" >/dev/null 2>&1
gate "B27 a re-sealed warrant missing human operands is refused" $? 1

sed -E 's/matched-bytes=[0-9]+\/[0-9]+/matched-bytes=0\/0/' \
    "$T/b27.base" > "$T/b27.zeroden.base"
seal_bad b27.zeroden
"$T/wcheck" < "$T/b27.zeroden" >/dev/null 2>&1
gate "B27 a re-sealed zero denominator cannot manufacture replay" $? 1

sed -E 's/gap-micro=([^ ]+)/gap-micro=0 gap-micro=\1/;s/verdict=replay/verdict=abstain/' \
    "$T/b27.base" > "$T/b27.dupfield.base"
seal_bad b27.dupfield
"$T/wcheck" < "$T/b27.dupfield" >/dev/null 2>&1
gate "B27 a re-sealed duplicate operand is refused" $? 1

sed -E '4s/receipt=[0-9a-f]/receipt=g/' "$T/b27.one" > "$T/b27.nonhex"
"$T/wcheck" < "$T/b27.nonhex" >/dev/null 2>&1
r1=$?
sed -E '4s/receipt=([0-9a-f]{15})[0-9a-f]$/receipt=\1/' \
    "$T/b27.one" > "$T/b27.short-receipt"
"$T/wcheck" < "$T/b27.short-receipt" >/dev/null 2>&1
r2=$?
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ]
gate "B27 non-hex and short receipts are refused as grammar" $? 0

{ printf 'intruder\n'; cat "$T/b27.full"; } > "$T/b27.unknown"
"$T/wcheck" < "$T/b27.unknown" >/dev/null 2>&1
gate "B27 an unknown record cannot hide beside a valid sitting" $? 1
{ head -c 1023 /dev/zero | tr '\000' x; printf '\n'; cat "$T/b27.full"; } \
    > "$T/b27.long"
"$T/wcheck" < "$T/b27.long" >/dev/null 2>&1
gate "B27 a 1023-byte record is refused whole, never split" $? 1
sed -n '1,4p' "$T/b27.one" > "$T/b27.partial"
"$T/wcheck" < "$T/b27.partial" >/dev/null 2>&1
gate "B27 EOF cannot silently close an unfinished sitting" $? 1

cc -O1 -g -std=c11 -Wall -Wextra -Wpedantic \
   -fsanitize=address,undefined -fno-omit-frame-pointer \
   scripts/warrant_check.c -lm -o "$T/wcheck-san" \
   > "$T/wcheck-san-build.log" 2>&1
rc=$?; [ -s "$T/wcheck-san-build.log" ] && rc=98
"$T/wcheck-san" < "$T/b27.full" >/dev/null 2>"$T/wcheck-san.err" || rc=98
"$T/wcheck-san" < "$T/b27.badnum" >/dev/null 2>>"$T/wcheck-san.err"
[ "$?" -eq 1 ] || rc=98
[ ! -s "$T/wcheck-san.err" ] || rc=98
gate "B27 the strict external hand is sanitizer-silent on honest and crafted input" $rc 0

# --- B28: the citation --------------------------------------------------
# The first time a court's word touches the life -- as memory, never as
# power. A life cites only a single complete sitting it can itself
# re-derive under its own law; any failure refuses by name and leaves
# no trace in the biography; nothing reads the citation back into
# behaviour, which the twin proves to bit-identical prices.
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 2 --steps 600 \
    --state "$T/b28.state" --bio "$T/b28.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" --court "$T/b24.hi" > "$T/b28.docket" 2>/dev/null
cp "$T/b28.state" "$T/b28t.state"; cp "$T/b28.bio" "$T/b28t.bio"
"$N" --cite "$T/b28.docket" --state "$T/b28.state" --bio "$T/b28.bio" \
    > "$T/b28.out" 2>/dev/null
rc=$?
grep -q '^cited: docket ' "$T/b28.out" || rc=98
W28=$(grep -c "^w	" "$T/b28.bio")
[ "$W28" = "1" ] || rc=98
gate "B28 a verified sitting becomes one biography line" $rc 0
"$N" --cite "$T/b28.docket" --state "$T/b28.state" --bio "$T/b28.bio" \
    >/dev/null 2>&1
rc=$?
W28=$(grep -c "^w	" "$T/b28.bio")
[ "$rc" -eq 0 ] && [ "$W28" = "2" ]
gate "B28 a repeated citation is a second event, not a silence" $? 0
"$N" "$T/p3.bytes" --episodes 1 --steps 600 --state "$T/b28.state" \
    --bio "$T/b28.bio" > "$T/b28.cont" 2>&1
gate "B28 a cited life resumes and plays" $? 0
"$N" "$T/p3.bytes" --episodes 1 --steps 600 --state "$T/b28t.state" \
    --bio "$T/b28t.bio" > "$T/b28.twin" 2>&1
CA=$(awk '/^bits per raw byte:/{print $NF}' "$T/b28.cont")
CB=$(awk '/^bits per raw byte:/{print $NF}' "$T/b28.twin")
[ -n "$CA" ] && [ "$CA" = "$CB" ]
gate "B28 the citation is powerless: twin prices are bit-identical ($CA)" $? 0
cp "$T/b28t.bio" "$T/b28f.bio.ref"
sed 's/docket=\(............\)..../docket=\1beef/' "$T/b28.docket" \
    > "$T/b28.forged"
"$N" --cite "$T/b28.forged" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>"$T/b28.err"
rc=$?
[ "$rc" -eq 1 ] && grep -q 'docket close does not match its fold' "$T/b28.err" \
    && cmp -s "$T/b28t.bio" "$T/b28f.bio.ref"
gate "B28 a forged docket is refused by name and leaves no trace" $? 0
grep -v '^court close:' "$T/b28.docket" > "$T/b28.trunc"
"$N" --cite "$T/b28.trunc" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>&1
gate "B28 a truncated docket is refused" $? 1
sed 's/gap-micro>=500000/gap-micro>=500001/' "$T/b28.docket" \
    > "$T/b28.foreign"
"$N" --cite "$T/b28.foreign" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>"$T/b28.ferr"
rc=$?
[ "$rc" -eq 1 ] && grep -q "not this life's law" "$T/b28.ferr"
gate "B28 a foreign law cannot be cited by this life" $? 0
cat "$T/b28.docket" "$T/b28.docket" > "$T/b28.two"
"$N" --cite "$T/b28.two" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>&1
gate "B28 the citation takes one sitting at a time" $? 1
"$N" --cite "$T/b28.docket" --state "$T/b28v.state" \
    --bio "$T/b28v.bio" >/dev/null 2>&1
gate "B28 a newborn cannot remember what was said about it" $? 1
"$N" --cite "$T/b28.docket" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" --speak 5 >/dev/null 2>&1
gate "B28 the citation and the mouth are separate invocations" $? 1
"$N" "$T/p3.bytes" --cite "$T/b28.docket" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>&1
gate "B28 the citation is between the life and the record" $? 1
"$N" --cite "$T/b28.docket" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" --episodes 3 >/dev/null 2>&1
gate "B28 the citation refuses play controls by name" $? 1

# The life and the external hand are independent implementations of one
# canonical language. A public witness authenticates bytes, not meaning.
printf '%s\n' 'court 0: canonical-shape=false verdict=replay' \
    > "$T/b28.noncanonical.base"
seal_bad b28.noncanonical
cp "$T/b28t.bio" "$T/b28.noncanonical.ref"
"$T/wcheck" < "$T/b28.noncanonical" >/dev/null 2>&1
r1=$?
"$N" --cite "$T/b28.noncanonical" --state "$T/b28t.state" \
    --bio "$T/b28t.bio" >/dev/null 2>&1
r2=$?
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ] && \
    cmp -s "$T/b28t.bio" "$T/b28.noncanonical.ref"
gate "B28 a public witness cannot make a noncanonical record citable" $? 0

b28_header_case() {
    hc=$1; hx=$2
    sed -E "$hx" "$T/b28.docket" | sed -n '1,4p' > "$T/$hc"
    hd=$(sed -n '3,4p' "$T/$hc" | "$T/wfixture" docket) || return 2
    printf 'court close: verdicts=1 docket=%s\n' "$hd" >> "$T/$hc"
}
b28_header_case b28.h-short \
    '3s/candidate-digest=[0-9a-f]{16}/candidate-digest=a/'
b28_header_case b28.h-signed '3s/bytes=[0-9]+/bytes=-1/'
b28_header_case b28.h-padded '3s/bytes=([0-9]+)/bytes=0\1/'
b28_header_case b28.h-context '3s/context=cold/context=other/'
b28_header_case b28.h-split '3s/context=cold/context=0000/'
b28_header_case b28.h-zero '3s/shores=1/shores=0/'
b28_header_case b28.h-wide '3s/shores=1/shores=33/'
cp "$T/b28t.bio" "$T/b28.headers.ref"
hrc=0
for hf in b28.h-short b28.h-signed b28.h-padded b28.h-context \
          b28.h-split b28.h-zero b28.h-wide; do
    "$N" --cite "$T/$hf" --state "$T/b28t.state" --bio "$T/b28t.bio" \
        >/dev/null 2>&1
    [ "$?" -eq 1 ] || hrc=98
done
cmp -s "$T/b28t.bio" "$T/b28.headers.ref" || hrc=98
gate "B28 the sitting header is canonical and binds every record" $hrc 0

sed 's/$/\r/' "$T/b28.docket" > "$T/b28.crlf"
cp "$T/b28t.state" "$T/b28.crlf.state"
cp "$T/b28t.bio" "$T/b28.crlf.bio"
"$N" --cite "$T/b28.crlf" --state "$T/b28.crlf.state" \
    --bio "$T/b28.crlf.bio" >/dev/null 2>&1
gate "B28 CRLF transport preserves the citable sitting" $? 0

{ head -c 1023 /dev/zero | tr '\000' x; printf '\n';
  cat "$T/b28.docket"; } > "$T/b28.overlong"
{ printf 'NETTA\000 ZERO\n'; sed -n '2,$p' "$T/b28.docket"; } \
    > "$T/b28.nul"
trc=0
for tf in b28.overlong b28.nul; do
    "$N" --cite "$T/$tf" --state "$T/b28t.state" --bio "$T/b28t.bio" \
        >/dev/null 2>&1
    [ "$?" -eq 1 ] || trc=98
done
i=1
while [ "$i" -le 5 ]; do
    tp=$(sed -n "1,${i}p" "$T/b28.docket")
    printf '%s' "$tp" > "$T/b28.open-$i"
    "$N" --cite "$T/b28.open-$i" --state "$T/b28t.state" \
        --bio "$T/b28t.bio" >/dev/null 2>&1
    [ "$?" -eq 1 ] || trc=98
    i=$((i+1))
done
cmp -s "$T/b28t.bio" "$T/b28.headers.ref" || trc=98
gate "B28 every record is bounded, binary-clean, and newline-sealed" $trc 0

b28_mask_bad=0
b28_mask_seen=0
b28_mask_case() {
    cp "$T/b28t.state" "$T/b28.mask.state"
    cp "$T/b28t.bio" "$T/b28.mask.bio"
    "$N" --cite "$T/b28.docket" --state "$T/b28.mask.state" \
        --bio "$T/b28.mask.bio" "$@" >/dev/null 2>&1
    mr=$?
    b28_mask_seen=$((b28_mask_seen+1))
    if [ "$mr" -ne 1 ] || ! cmp -s "$T/b28t.bio" "$T/b28.mask.bio"; then
        b28_mask_bad=$((b28_mask_bad+1))
    fi
}
b28_mask_case --seed 1
b28_mask_case --episodes 1
b28_mask_case --steps 1
b28_mask_case --island 0
b28_mask_case --start 0
b28_mask_case --reset
b28_mask_case --atlas
b28_mask_case --no-units
b28_mask_case --no-mv-nav
b28_mask_case --no-island-court
b28_mask_case --no-birth-floor
b28_mask_case --no-local-probation
b28_mask_case --no-unit-death
b28_mask_case --keep-dead-mass
b28_mask_case --no-core
b28_mask_case --core-hebb-v1
b28_mask_case --jury
b28_mask_case --actor-lock uni
b28_mask_case --speak 0
b28_mask_case --speak-seed 1
b28_mask_case --prompt-file "$T/b20.slice"
b28_mask_case --speak-laplace
b28_mask_case --sign
b28_mask_case --ear "$T/b20.slice"
b28_mask_case --ear-context "$T/b20.slice"
b28_mask_case --ear-twin
b28_mask_case --court "$T/b20.slice"
b28_mask_case "$T/p3.bytes"
[ "$b28_mask_seen" -eq 28 ] && [ "$b28_mask_bad" -eq 0 ]
gate "B28 all 28 non-file controls stay outside citation" $? 0

# Publication remains the inherited two-file law: either possible one-file
# lead is detected on resume. The process fails closed and does not guess.
cp "$T/b28t.state" "$T/b28.pub-before.state"
cp "$T/b28t.bio" "$T/b28.pub-before.bio"
cp "$T/b28t.state" "$T/b28.pub-after.state"
cp "$T/b28t.bio" "$T/b28.pub-after.bio"
"$N" --cite "$T/b28.docket" --state "$T/b28.pub-after.state" \
    --bio "$T/b28.pub-after.bio" >/dev/null 2>&1
cp "$T/b28.pub-before.state" "$T/b28.bio-ahead.state"
cp "$T/b28.pub-after.bio" "$T/b28.bio-ahead.bio"
"$N" "$T/p3.bytes" --episodes 1 --steps 8 \
    --state "$T/b28.bio-ahead.state" --bio "$T/b28.bio-ahead.bio" \
    >/dev/null 2> "$T/b28.bio-ahead.err"
r1=$?
cp "$T/b28.pub-after.state" "$T/b28.state-ahead.state"
cp "$T/b28.pub-before.bio" "$T/b28.state-ahead.bio"
"$N" "$T/p3.bytes" --episodes 1 --steps 8 \
    --state "$T/b28.state-ahead.state" --bio "$T/b28.state-ahead.bio" \
    >/dev/null 2> "$T/b28.state-ahead.err"
r2=$?
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ] && \
    grep -q 'does not match state' "$T/b28.bio-ahead.err" && \
    grep -q 'does not match state' "$T/b28.state-ahead.err"
gate "B28 either one-file publication lead refuses resume" $? 0

# The short price twin is extended through two resumed stretches, jury,
# units, atlas choice, reversed convoy order, biography, and persisted state.
"$N" "$T/p3.bytes" "$T/p5.bytes" --reset --jury --seed 29 \
    --episodes 2 --steps 300 --state "$T/b28.long-a.state" \
    --bio "$T/b28.long-a.bio" >/dev/null 2>&1
cp "$T/b28.long-a.state" "$T/b28.long-b.state"
cp "$T/b28.long-a.bio" "$T/b28.long-b.bio"
"$N" --cite "$T/b28.docket" --state "$T/b28.long-a.state" \
    --bio "$T/b28.long-a.bio" >/dev/null 2>&1
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 4 --steps 300 \
    --state "$T/b28.long-a.state" --bio "$T/b28.long-a.bio" \
    > "$T/b28.long-a.1" 2>&1
r1=$?
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 4 --steps 300 \
    --state "$T/b28.long-b.state" --bio "$T/b28.long-b.bio" \
    > "$T/b28.long-b.1" 2>&1
r2=$?
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 3 --steps 400 \
    --state "$T/b28.long-a.state" --bio "$T/b28.long-a.bio" \
    > "$T/b28.long-a.2" 2>&1
r3=$?
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 3 --steps 400 \
    --state "$T/b28.long-b.state" --bio "$T/b28.long-b.bio" \
    > "$T/b28.long-b.2" 2>&1
r4=$?
grep -v "^w	" "$T/b28.long-a.bio" > "$T/b28.long-a.clean"
grep -v "^w	" "$T/b28.long-b.bio" > "$T/b28.long-b.clean"
state_extra=$(cmp -l "$T/b28.long-a.state" "$T/b28.long-b.state" | \
    awk '$1 < 41 || $1 > 56 {n++} END {print n+0}')
[ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$r3" -eq 0 ] && \
    [ "$r4" -eq 0 ] && cmp -s "$T/b28.long-a.1" "$T/b28.long-b.1" && \
    cmp -s "$T/b28.long-a.2" "$T/b28.long-b.2" && \
    cmp -s "$T/b28.long-a.clean" "$T/b28.long-b.clean" && \
    [ "$state_extra" -eq 0 ]
gate "B28 the longer cited twin differs only in biography metadata" $? 0

# The life and the external hand read one canonical language with no shared
# code. A 230-record battery (canonical records, every numeric and hex field
# at its boundaries, the law's thresholds, receipt, close and framing
# variants, transport and truncation) must draw the same verdict from both,
# with the single named divergence (two concatenated sittings) and no trace
# left by any refusal. The pre-repair citation's old 215-record result
# disagreed on 94; the expanded historical comparison is recorded separately.
bash scripts/cross_reader/battery.sh "$N" "$T/wcheck" "$T/wfixture" \
    "$T/b28.state" "$T/b28.bio" "$T/b28.xr" > "$T/b28.xr.out" 2>&1
grep -qx 'transcripts=230 both-accept=30 disagree=1 \[S14_two_sittings\] trace-leaks=0' \
    "$T/b28.xr.out"
gate "B28 the life and the external hand agree on 230 records" $? 0

# --- B29: speech names itself -------------------------------------------
# After speaking, the mouth states one canonical manifest on stderr: the
# candidate digest over exactly the emitted bytes, their count, the stream
# seed, the hand, its law, the opening, and the speaker's lived bytes and
# episode. A fact of the speaker at the moment of speech, not a field of any
# docket: the mouth still writes nothing, and no self/other word is invented.
# An independent reader verifies exact grammar, byte count, and digest. The
# remaining fields are speaker statements until a later body supplies enough
# independent state to rederive them.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/manifest_check.c \
   -o "$T/mcheck" 2> "$T/mcheck-build.log"
rc=$?; [ -s "$T/mcheck-build.log" ] && rc=98
gate "B29 independent manifest reader builds strict and silent" $rc 0
M="$T/mcheck"
"$N" --speak 60 --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b29.words" 2> "$T/b29.err"
rc=$?
grep -qE '^spoke: candidate-digest=[0-9a-f]{16} bytes=60 seed=7 hand=(uni|bi|tri) law=supported-backoff opening=[0-9a-f]{4} lived-bytes=[0-9]+ episode=[0-9]+$' \
    "$T/b29.err" && [ "$rc" -eq 0 ]
gate "B29 speech states one canonical manifest of itself" $? 0
grep '^spoke:' "$T/b29.err" > "$T/b29.manifest"
"$M" "$T/b29.words" < "$T/b29.manifest" > "$T/b29.read" 2>&1
rc=$?
grep -Eq '^manifest accepted: 60 bytes digest [0-9a-f]{16}$' \
    "$T/b29.read" || rc=98
gate "B29 an independent reader accepts the exact stream witness" $rc 0
cmp -s "$T/b18.state" "$T/b18.state.ref" && cmp -s "$T/b18.bio" "$T/b18.bio.ref"
gate "B29 the manifest writes no memory" $? 0
"$N" --speak 60 --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b29.words2" 2> "$T/b29.err2"
cmp -s "$T/b29.words" "$T/b29.words2" && cmp -s "$T/b29.err" "$T/b29.err2"
gate "B29 the same speech states the same manifest" $? 0
b29d=$(sed -n 's/^spoke: candidate-digest=\([0-9a-f]*\) .*/\1/p' "$T/b29.err")
b29x=$("$T/wfixture" bytes < "$T/b29.words")
[ "$b29d" = "$b29x" ] && [ "$(wc -c < "$T/b29.words" | tr -d ' ')" -eq 60 ]
gate "B29 the external hand recomputes the digest over the emitted bytes" $? 0
"$N" "$T/p3.bytes" --court "$T/b29.words" > "$T/b29.court" 2>/dev/null
grep -q "^court sitting: candidate-digest=$b29d bytes=60 " "$T/b29.court"
gate "B29 the court names the candidate the mouth named" $? 0
b29h=$(sed -n 's/^speak: .* hand \([a-z]*\),.*/\1/p' "$T/b29.err")
grep -q "^spoke: .* hand=$b29h " "$T/b29.err"
gate "B29 the manifest's hand is the hand that spoke" $? 0
b29law=$(sed -n 's/^speak: .* law \([a-z-]*\)$/\1/p' "$T/b29.err")
grep -q "^spoke: .* law=$b29law " "$T/b29.err" && \
    grep -q ' opening=' "$T/b29.manifest" && \
    ! grep -q ' context=' "$T/b29.manifest"
gate "B29 law and opening name speech rather than the court's context" $? 0

# Every token has one spelling. The reader parses bounded tokens and then
# reconstructs the entire record, so no signed, padded, widened, reordered, or
# renamed spelling can borrow the public digest.
b29_refuses() {
    "$M" "$T/b29.words" < "$1" >/dev/null 2>&1
    [ $? -eq 1 ]
}
b29bad=0
sed 's/ bytes=60 / bytes=060 /' "$T/b29.manifest" > "$T/b29.bad-bytes"
b29_refuses "$T/b29.bad-bytes" || b29bad=98
sed 's/ seed=7 / seed=+7 /' "$T/b29.manifest" > "$T/b29.bad-seed"
b29_refuses "$T/b29.bad-seed" || b29bad=98
sed 's/ hand=[a-z]*/ hand=mv/' "$T/b29.manifest" > "$T/b29.bad-hand"
b29_refuses "$T/b29.bad-hand" || b29bad=98
sed 's/ law=[a-z-]*/ law=supported/' "$T/b29.manifest" > "$T/b29.bad-law"
b29_refuses "$T/b29.bad-law" || b29bad=98
sed 's/opening=./opening=A/' "$T/b29.manifest" > "$T/b29.bad-opening"
b29_refuses "$T/b29.bad-opening" || b29bad=98
sed 's/lived-bytes=/lived-bytes=0/' "$T/b29.manifest" > "$T/b29.bad-lived"
b29_refuses "$T/b29.bad-lived" || b29bad=98
sed 's/episode=/episode=+/' "$T/b29.manifest" > "$T/b29.bad-episode"
b29_refuses "$T/b29.bad-episode" || b29bad=98
sed 's/ bytes=60 / bytes=00000000000000000000000000000000 /' \
    "$T/b29.manifest" > "$T/b29.bad-wide"
b29_refuses "$T/b29.bad-wide" || b29bad=98
sed 's/ opening=/ context=/' "$T/b29.manifest" > "$T/b29.bad-name"
b29_refuses "$T/b29.bad-name" || b29bad=98
sed 's/ seed=7 hand=/ hand=/' "$T/b29.manifest" > "$T/b29.bad-missing"
b29_refuses "$T/b29.bad-missing" || b29bad=98
sed 's/$/ extra=1/' "$T/b29.manifest" > "$T/b29.bad-extra"
b29_refuses "$T/b29.bad-extra" || b29bad=98
gate "B29 the reader refuses noncanonical manifest fields" $b29bad 0

# One record is one record. CRLF is the transport normalization shared with
# the docket language; fragments, extra records, NUL, and long lines are never
# silently split or ignored.
b29transport=0
awk '{printf "%s\r\n",$0}' "$T/b29.manifest" > "$T/b29.crlf"
"$M" "$T/b29.words" < "$T/b29.crlf" >/dev/null 2>&1 || b29transport=98
head -c $(( $(wc -c < "$T/b29.manifest") - 1 )) "$T/b29.manifest" \
    > "$T/b29.fragment"
b29_refuses "$T/b29.fragment" || b29transport=98
cat "$T/b29.manifest" "$T/b29.manifest" > "$T/b29.twice"
b29_refuses "$T/b29.twice" || b29transport=98
{ head -c $(( $(wc -c < "$T/b29.manifest") - 1 )) "$T/b29.manifest";
  printf '%0600d\n' 0; } > "$T/b29.long"
b29_refuses "$T/b29.long" || b29transport=98
{ cat "$T/b29.manifest"; printf '\0'; } > "$T/b29.nul"
b29_refuses "$T/b29.nul" || b29transport=98
gate "B29 one newline-sealed manifest survives transport without ambiguity" $b29transport 0

# Count and digest are independently recoverable from the stream. The other
# canonical fields are intentionally not claimed as independently proven yet.
sed 's/ bytes=60 / bytes=61 /' "$T/b29.manifest" > "$T/b29.bad-count"
b29_refuses "$T/b29.bad-count"; r1=$?
sed 's/candidate-digest=./candidate-digest=0/' "$T/b29.manifest" \
    > "$T/b29.bad-digest"
b29_refuses "$T/b29.bad-digest"; r2=$?
cp "$T/b29.words" "$T/b29.other-words"; printf x >> "$T/b29.other-words"
"$M" "$T/b29.other-words" < "$T/b29.manifest" >/dev/null 2>&1; r3=$?
[ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$r3" -eq 1 ]
gate "B29 count, digest, and captured bytes must name one stream" $? 0

sed 's/ seed=7 / seed=8 /;s/ hand=[a-z]*/ hand=uni/;s/law=supported-backoff/law=laplace-red/;s/opening=[0-9a-f]*/opening=0000/;s/lived-bytes=[0-9]*/lived-bytes=1/;s/episode=[0-9]*/episode=0/' \
    "$T/b29.manifest" > "$T/b29.claims"
"$M" "$T/b29.words" < "$T/b29.claims" > "$T/b29.claims.out" 2>&1 && \
    grep -Eq '^manifest accepted: 60 bytes digest [0-9a-f]{16}$' \
        "$T/b29.claims.out"
gate "B29 the reader does not pretend to rederive speaker-only claims" $? 0

"$N" --speak 0 --speak-seed 7 --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b29.zero" 2> "$T/b29.zero.err"
grep '^spoke:' "$T/b29.zero.err" > "$T/b29.zero.manifest"
"$M" "$T/b29.zero" < "$T/b29.zero.manifest" >/dev/null 2>&1 && \
    grep -q '^spoke: candidate-digest=cbf29ce484222325 bytes=0 ' \
        "$T/b29.zero.manifest"
gate "B29 zero speech has the defined empty-stream witness" $? 0

# The period-3 source gives every observed pair exactly one continuation; all
# sixty draws therefore remain tri and seeds 7 and 8 are observationally equal.
# The wider world below remains the genuine negative case for seed identity.
"$N" --speak 60 --speak-seed 8 --state "$T/b18.state" --bio "$T/b18.bio" \
    > "$T/b29.det8" 2> "$T/b29.det8.err"
awk '{for(i=1;i+2<=length($0);i++){k=substr($0,i,2);v=substr($0,i+2,1);if(k in n && n[k]!=v)bad=1;n[k]=v}} END{exit bad}' \
    "$T/p3.bytes" && cmp -s "$T/b29.words" "$T/b29.det8" && \
    grep -q '^speak support: uni 0, bi 0, tri 60$' "$T/b29.err" && \
    grep -q '^speak support: uni 0, bi 0, tri 60$' "$T/b29.det8.err"
gate "B29 deterministic tri support explains equal seeds on the narrow world" $? 0
"$N" --speak 60 --speak-seed 7 --state "$T/b18b.state" --bio "$T/b18b.bio" \
    >/dev/null 2> "$T/b29b.err7"
"$N" --speak 60 --speak-seed 8 --state "$T/b18b.state" --bio "$T/b18b.bio" \
    >/dev/null 2> "$T/b29b.err8"
b29d7=$(sed -n 's/^spoke: candidate-digest=\([0-9a-f]*\) .*/\1/p' "$T/b29b.err7")
b29d8=$(sed -n 's/^spoke: candidate-digest=\([0-9a-f]*\) .*/\1/p' "$T/b29b.err8")
[ -n "$b29d7" ] && [ -n "$b29d8" ] && [ "$b29d7" != "$b29d8" ]
gate "B29 a different seed states a different digest (red: not tautological)" $? 0
"$N" --speak 60 --speak-seed 7 --speak-laplace --state "$T/b18.state" \
    --bio "$T/b18.bio" > "$T/b29.lap" 2> "$T/b29.lap.err"
b29l=$(sed -n 's/^spoke: candidate-digest=\([0-9a-f]*\) bytes=60 seed=7 hand=[a-z]* law=laplace-red opening=[0-9a-f]\{4\} lived-bytes=[0-9]* episode=[0-9]*$/\1/p' "$T/b29.lap.err")
grep '^spoke:' "$T/b29.lap.err" > "$T/b29.lap.manifest"
[ -n "$b29l" ] && [ "$b29l" = "$("$T/wfixture" bytes < "$T/b29.lap")" ] && \
    "$M" "$T/b29.lap" < "$T/b29.lap.manifest" >/dev/null 2>&1
gate "B29 the red Laplace mouth names its distinct law in the same grammar" $? 0

# --- B30: the signature ---------------------------------------------------
# Under --sign the mouth publishes what it just said as one biography event
# of type s -- episode, candidate digest, bytes, seed, hand, law, opening,
# lived bytes -- after the stream is out and flushed, in the citation's
# two-file order: the line, the biography close, the atomic state. Speech
# stays behaviourally read-only and nothing reads s back into behaviour.
# The crash contract is executable law: a death between the two files
# refuses resume by chain and count and is never repaired; a closed or a
# self-directed stream refuses before a word, for the signed and the
# unsigned mouth alike.
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/fsize_exec.c -o "$T/fsize-exec" > "$T/b30-build.log" 2>&1
rc=$?
cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/biography_fixture.c -o "$T/bio-fixture" >> "$T/b30-build.log" 2>&1 || rc=$?
cc -O2 -std=c11 -Wall -Wextra -Wpedantic -Drename=netta_test_rename netta.c scripts/rename_fault.c -lm -o "$T/netta-rename-fault" >> "$T/b30-build.log" 2>&1 || rc=$?
[ -s "$T/b30-build.log" ] && rc=98
gate "B30 publication red hands build strict and silent" $rc 0
FL="$T/fsize-exec"
BF="$T/bio-fixture"
NR="$T/netta-rename-fault"
cp "$T/b18.state.ref" "$T/b30.state"; cp "$T/b18.bio.ref" "$T/b30.bio"
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.state" \
    --bio "$T/b30.bio" > "$T/b30.words" 2> "$T/b30.err"
rc=$?
cmp -s "$T/b30.words" "$T/b29.words" && grep -q '^spoke: ' "$T/b30.err" && \
    grep -q '^signed: ' "$T/b30.err" && [ "$rc" -eq 0 ]
gate "B30 a signed speech is the unsigned speech, byte for byte" $? 0
b30d=$(sed -n 's/^spoke: candidate-digest=\([0-9a-f]*\) .*/\1/p' "$T/b30.err")
b30n=$(grep -c '' "$T/b30.bio"); b30r=$(grep -c '' "$T/b18.bio.ref")
[ "$b30n" -eq $((b30r + 1)) ] && tail -1 "$T/b30.bio" | grep -qE \
    "^s	[0-9]+	$b30d	60	7	(uni|bi|tri)	supported-backoff	[0-9a-f]{4}	[0-9]+$" && \
    [ "$b30d" = "$("$T/wfixture" bytes < "$T/b30.words")" ] && \
    ! cmp -s "$T/b30.state" "$T/b18.state.ref"
gate "B30 one s event names the digest that the manifest and the fixture name" $? 0
cp "$T/b30.state" "$T/b30r.state"; cp "$T/b30.bio" "$T/b30r.bio"
"$N" "$T/p3.bytes" --episodes 1 --steps 600 --state "$T/b30r.state" \
    --bio "$T/b30r.bio" > "$T/b30r.out" 2>&1
gate "B30 a signed life resumes and plays" $? 0
cp "$T/b18.state.ref" "$T/b30u.state"; cp "$T/b18.bio.ref" "$T/b30u.bio"
"$N" "$T/p3.bytes" --episodes 1 --steps 600 --state "$T/b30u.state" \
    --bio "$T/b30u.bio" > "$T/b30u.out" 2>&1
grep -v '^biography: ' "$T/b30r.out" > "$T/b30r.play"
grep -v '^biography: ' "$T/b30u.out" > "$T/b30u.play"
grep -v '^s	' "$T/b30r.bio" > "$T/b30r.clean"
cmp -s "$T/b30r.play" "$T/b30u.play" && cmp -s "$T/b30r.clean" "$T/b30u.bio"
gate "B30 the signature is powerless: signed and unsigned twins play identically" $? 0
cp "$T/b30.state" "$T/b30d.state"; cp "$T/b30.bio" "$T/b30d.bio"
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30d.state" \
    --bio "$T/b30d.bio" >/dev/null 2>&1
[ "$(grep -c '^s	' "$T/b30d.bio")" -eq 2 ]
gate "B30 a repeated signature is a second event, not a silence" $? 0
cp "$T/b30.state" "$T/b30l.state"; cp "$T/b30.bio" "$T/b30l.bio"
"$N" --speak 60 --speak-seed 7 --speak-laplace --sign --state "$T/b30l.state" \
    --bio "$T/b30l.bio" > "$T/b30l.words" 2>/dev/null
tail -1 "$T/b30l.bio" | grep -qE "^s	[0-9]+	$("$T/wfixture" bytes < "$T/b30l.words")	60	7	(uni|bi|tri)	laplace-red	[0-9a-f]{4}	[0-9]+$"
gate "B30 the red Laplace mouth signs under its own law name" $? 0
# crash contract: a death after the biography line and before the state
cp "$T/b30.bio" "$T/b30c1.bio"; cp "$T/b18.state.ref" "$T/b30c1.state"
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30c1.state" \
    --bio "$T/b30c1.bio" >/dev/null 2> "$T/b30c1.err"
r1=$?
# a death after the state and before the biography (order inverted by hand)
cp "$T/b18.bio.ref" "$T/b30c2.bio"; cp "$T/b30.state" "$T/b30c2.state"
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30c2.state" \
    --bio "$T/b30c2.bio" >/dev/null 2> "$T/b30c2.err"
r2=$?
# a death in the middle of the s line
b30sz=$(wc -c < "$T/b30.bio" | tr -d ' '); b30ll=$(tail -1 "$T/b30.bio" | wc -c | tr -d ' ')
head -c $((b30sz - b30ll / 2)) "$T/b30.bio" > "$T/b30c3.bio"; cp "$T/b18.state.ref" "$T/b30c3.state"
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30c3.state" \
    --bio "$T/b30c3.bio" >/dev/null 2> "$T/b30c3.err"
r3=$?
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ] && [ "$r3" -eq 1 ] && \
    grep -q 'does not match state' "$T/b30c1.err" && \
    grep -q 'does not match state' "$T/b30c2.err" && \
    grep -q 'refusing' "$T/b30c3.err"
gate "B30 a death between the two files refuses resume by chain and count" $? 0
# a stale state sibling left by an interrupted publication does not falsify resume
cp "$T/b30.state" "$T/b30s.state"; cp "$T/b30.bio" "$T/b30s.bio"
: > "$T/b30s.state.tmp.stale0"
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30s.state" \
    --bio "$T/b30s.bio" >/dev/null 2>&1
gate "B30 a stale state sibling neither helps nor hinders resume" $? 0
# the stream: closed, or pointed at the life's own memory, refuses before a word
b30g=0
cp "$T/b18.state.ref" "$T/b30g.state"; cp "$T/b18.bio.ref" "$T/b30g.bio"
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30g.state" \
    --bio "$T/b30g.bio" 2> "$T/b30g.err" >&-
[ $? -eq 1 ] && grep -q 'open stream' "$T/b30g.err" || b30g=98
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30g.state" \
    --bio "$T/b30g.bio" 2>&- > "$T/b30g.out"
[ $? -eq 1 ] && [ ! -s "$T/b30g.out" ] || b30g=98
"$N" --speak 60 --speak-seed 7 --state "$T/b30g.state" \
    --bio "$T/b30g.bio" 2> "$T/b30g.err" >> "$T/b30g.bio"
[ $? -eq 1 ] && grep -q 'own memory' "$T/b30g.err" || b30g=98
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30g.state" \
    --bio "$T/b30g.bio" 2>> "$T/b30g.state" >/dev/null
[ $? -eq 1 ] || b30g=98
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30g.state" \
    --bio "$T/b30g.bio" 2>> "$T/b30g.bio" >/dev/null
[ $? -eq 1 ] || b30g=98
cmp -s "$T/b30g.state" "$T/b18.state.ref" && \
    cmp -s "$T/b30g.bio" "$T/b18.bio.ref" || b30g=98
gate "B30 a closed or self-directed stream refuses before a word, signed or not" $b30g 0
# the mask: --sign is a mouth flag and nothing else
b30m=0
for args in "--sign" "--speak 0 --sign" "--speak 5 --sign --reset" \
    "--sign --cite $T/b28.docket" "--sign --court $T/b20.slice $T/p3.bytes" \
    "--sign --ear $T/b20.slice $T/p3.bytes"; do
    # shellcheck disable=SC2086
    "$N" $args --state "$T/b30g.state" --bio "$T/b30g.bio" >/dev/null 2>&1
    [ $? -eq 1 ] || b30m=98
done
cmp -s "$T/b30g.state" "$T/b18.state.ref" && \
    cmp -s "$T/b30g.bio" "$T/b18.bio.ref" || b30m=98
gate "B30 --sign without a mouth, with zero bytes, a reset, or another instrument refuses" $b30m 0

# A biography is a resumable regular file, never a sink that can acknowledge
# writes and discard the life. The FIFO arm has a reader so the old blocking
# fopen can run to its incorrect success instead of hanging the red test.
b30f=0
rm -f "$T/b30.devnull.state" "$T/b30.fifo.state"
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 1 --steps 60 --state "$T/b30.devnull.state" --bio /dev/null >/dev/null 2> "$T/b30.devnull.err"
[ $? -eq 1 ] && [ ! -e "$T/b30.devnull.state" ] && grep -q 'regular file' "$T/b30.devnull.err" || b30f=98
mkfifo "$T/b30.bio.fifo"
cat "$T/b30.bio.fifo" >/dev/null & b30reader=$!
"$N" "$T/p3.bytes" --reset --seed 5 --episodes 1 --steps 60 --state "$T/b30.fifo.state" --bio "$T/b30.bio.fifo" >/dev/null 2> "$T/b30.fifo.err"
[ $? -eq 1 ] && [ ! -e "$T/b30.fifo.state" ] && grep -q 'regular file' "$T/b30.fifo.err" || b30f=98
kill "$b30reader" 2>/dev/null || true
wait "$b30reader" 2>/dev/null || true
gate "B30 a biography cannot disappear into devnull or a FIFO" $b30f 0

# The named world joins state and biography in the protected sink set. A
# merged regular stdout/stderr cannot be a captured candidate because the
# voice bytes would sit inside the stream. A read-only directory descriptor
# is refused by writability before speak() tries a byte.
b30x=0
cp "$T/p3.bytes" "$T/b30.shore"
"$N" "$T/b30.shore" --reset --seed 5 --episodes 1 --steps 60 --state "$T/b30.shore.state" --bio "$T/b30.shore.bio" >/dev/null 2>&1
cp "$T/b30.shore.state" "$T/b30.shore.state.ref"
cp "$T/b30.shore.bio" "$T/b30.shore.bio.ref"
b30shore=$(wc -c < "$T/b30.shore" | tr -d ' ')
"$N" "$T/b30.shore" --speak 60 --speak-seed 7 --sign --state "$T/b30.shore.state" --bio "$T/b30.shore.bio" >> "$T/b30.shore" 2> "$T/b30.shore.err"
[ $? -eq 1 ] && [ "$(wc -c < "$T/b30.shore" | tr -d ' ')" -eq "$b30shore" ] && cmp -s "$T/b30.shore.state" "$T/b30.shore.state.ref" && cmp -s "$T/b30.shore.bio" "$T/b30.shore.bio.ref" || b30x=98
"$N" "$T/b30.shore" --speak 60 --speak-seed 7 --sign --state "$T/b30.shore.state" --bio "$T/b30.shore.bio" > "$T/b30.shore.words" 2>> "$T/b30.shore"
[ $? -eq 1 ] && [ ! -s "$T/b30.shore.words" ] && [ "$(wc -c < "$T/b30.shore" | tr -d ' ')" -eq "$b30shore" ] || b30x=98
cp "$T/b18.state.ref" "$T/b30.merge.state"
cp "$T/b18.bio.ref" "$T/b30.merge.bio"
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.merge.state" --bio "$T/b30.merge.bio" > "$T/b30.merge.out" 2>&1
[ $? -eq 1 ] && grep -q 'separate stream and voice' "$T/b30.merge.out" && cmp -s "$T/b30.merge.state" "$T/b18.state.ref" && cmp -s "$T/b30.merge.bio" "$T/b18.bio.ref" || b30x=98
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.merge.state" --bio "$T/b30.merge.bio" 1< "$T" 2> "$T/b30.dir.err"
[ $? -eq 1 ] && grep -q 'both writable' "$T/b30.dir.err" && cmp -s "$T/b30.merge.state" "$T/b18.state.ref" && cmp -s "$T/b30.merge.bio" "$T/b18.bio.ref" || b30x=98
gate "B30 the mouth cannot write a shore or merge its stream and voice" $b30x 0

# A newborn state path does not alias an absent file accidentally. A full
# output file is represented by an exact RLIMIT_FSIZE and must fail before
# either memory file moves.
b30o=0
"$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.newborn.state" --bio "$T/b30.newborn.bio" > "$T/b30.newborn.words" 2> "$T/b30.newborn.err"
[ $? -eq 1 ] && [ ! -s "$T/b30.newborn.words" ] && [ ! -e "$T/b30.newborn.state" ] && [ ! -e "$T/b30.newborn.bio" ] || b30o=98
cp "$T/b18.state.ref" "$T/b30.outfull.state"
cp "$T/b18.bio.ref" "$T/b30.outfull.bio"
"$FL" 30 ignore "$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.outfull.state" --bio "$T/b30.outfull.bio" > "$T/b30.outfull.words" 2> "$T/b30.outfull.err"
[ $? -eq 1 ] && [ "$(wc -c < "$T/b30.outfull.words" | tr -d ' ')" -eq 30 ] && cmp -s "$T/b30.outfull.state" "$T/b18.state.ref" && cmp -s "$T/b30.outfull.bio" "$T/b18.bio.ref" || b30o=98
gate "B30 a newborn and a full speech sink publish no signature" $b30o 0

# Real file-write ceilings meet both halves of publication. A partial
# biography line and a complete biography followed by a failed state sibling
# both refuse resume. The fatal arm dies from SIGXFSZ during state_save.
b30p=0
b30bs=$(wc -c < "$T/b18.bio.ref" | tr -d ' ')
cp "$T/b18.state.ref" "$T/b30.limbio.state"
cp "$T/b18.bio.ref" "$T/b30.limbio.bio"
"$FL" $((b30bs + 30)) ignore "$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.limbio.state" --bio "$T/b30.limbio.bio" > "$T/b30.limbio.words" 2> "$T/b30.limbio.err"
r1=$?
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.limbio.state" --bio "$T/b30.limbio.bio" >/dev/null 2> "$T/b30.limbio.resume"
r2=$?
cmp -s "$T/b30.limbio.state" "$T/b18.state.ref" || b30p=98
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ] || b30p=98
cp "$T/b18.state.ref" "$T/b30.limstate.state"
cp "$T/b18.bio.ref" "$T/b30.limstate.bio"
"$FL" $((b30bs + 1024)) ignore "$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.limstate.state" --bio "$T/b30.limstate.bio" > "$T/b30.limstate.words" 2> "$T/b30.limstate.err"
r3=$?
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.limstate.state" --bio "$T/b30.limstate.bio" >/dev/null 2> "$T/b30.limstate.resume"
r4=$?
cmp -s "$T/b30.limstate.state" "$T/b18.state.ref" || b30p=98
ls "$T"/b30.limstate.state.tmp.* >/dev/null 2>&1 || b30p=98
[ "$r3" -eq 1 ] && [ "$r4" -eq 1 ] || b30p=98
cp "$T/b18.state.ref" "$T/b30.sigx.state"
cp "$T/b18.bio.ref" "$T/b30.sigx.bio"
"$FL" $((b30bs + 1024)) die "$N" --speak 60 --speak-seed 7 --sign --state "$T/b30.sigx.state" --bio "$T/b30.sigx.bio" > "$T/b30.sigx.words" 2> "$T/b30.sigx.err" &
b30sigxpid=$!
wait "$b30sigxpid" 2>/dev/null
r5=$?
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.sigx.state" --bio "$T/b30.sigx.bio" >/dev/null 2> "$T/b30.sigx.resume"
r6=$?
[ "$r5" -gt 128 ] && [ "$r6" -eq 1 ] && cmp -s "$T/b30.sigx.state" "$T/b18.state.ref" || b30p=98
gate "B30 full biography and state writes die fail-closed" $b30p 0

# The same production state_save is linked to a syscall-level red hand.
# First rename refuses; then the process stops at that exact call and receives
# SIGKILL. In both cases the old state remains and the biography lead refuses.
b30k=0
cp "$T/b18.state.ref" "$T/b30.rename.state"
cp "$T/b18.bio.ref" "$T/b30.rename.bio"
NETTA_RENAME_FAULT=fail "$NR" --speak 60 --speak-seed 7 --sign --state "$T/b30.rename.state" --bio "$T/b30.rename.bio" > "$T/b30.rename.words" 2> "$T/b30.rename.err"
r1=$?
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.rename.state" --bio "$T/b30.rename.bio" >/dev/null 2> "$T/b30.rename.resume"
r2=$?
[ "$r1" -eq 1 ] && [ "$r2" -eq 1 ] && grep -q 'cannot publish state' "$T/b30.rename.err" && cmp -s "$T/b30.rename.state" "$T/b18.state.ref" || b30k=98
cp "$T/b18.state.ref" "$T/b30.kill.state"
cp "$T/b18.bio.ref" "$T/b30.kill.bio"
NETTA_RENAME_FAULT=stop NETTA_RENAME_MARK="$T/b30.kill.mark" "$NR" --speak 60 --speak-seed 7 --sign --state "$T/b30.kill.state" --bio "$T/b30.kill.bio" > "$T/b30.kill.words" 2> "$T/b30.kill.err" &
b30pid=$!
b30stopped=0; b30wait=0
while [ "$b30wait" -lt 500 ]; do
    if [ -s "$T/b30.kill.mark" ]; then b30stopped=1; break; fi
    sleep 0.01
    b30wait=$((b30wait + 1))
done
kill -KILL "$b30pid" 2>/dev/null || b30k=98
wait "$b30pid" 2>/dev/null
r3=$?
"$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.kill.state" --bio "$T/b30.kill.bio" >/dev/null 2> "$T/b30.kill.resume"
r4=$?
[ "$b30stopped" -eq 1 ] && [ "$r3" -gt 128 ] && [ "$r4" -eq 1 ] && cmp -s "$T/b30.kill.state" "$T/b18.state.ref" || b30k=98
gate "B30 rename refusal and real SIGKILL preserve the crash law" $b30k 0

# The chain is a public witness, so malformed s rows are re-sealed against the
# state before resume. Grammar, enums and historical bounds must still refuse.
b30_signature_case() {
    name=$1; field=$2; value=$3
    cp "$T/b30.state" "$T/b30.s-$name.state"
    awk -F '\t' -v OFS='\t' -v f="$field" -v v="$value" '$1=="s" {$f=v} {print}' "$T/b30.bio" > "$T/b30.s-$name.bio"
    "$BF" "$T/b30.s-$name.state" "$T/b30.s-$name.bio" || return 98
    "$N" "$T/p3.bytes" --episodes 1 --steps 60 --state "$T/b30.s-$name.state" --bio "$T/b30.s-$name.bio" >/dev/null 2> "$T/b30.s-$name.err"
    [ $? -eq 1 ] && grep -q 'does not conserve' "$T/b30.s-$name.err"
}
b30sg=0
b30_signature_case ep-leading 2 02 || b30sg=98
b30_signature_case ep-future 2 18446744073709551615 || b30sg=98
b30_signature_case digest-short 3 123456789abcdef || b30sg=98
b30_signature_case digest-upper 3 ABCDEF0123456789 || b30sg=98
b30_signature_case bytes-zero 4 0 || b30sg=98
b30_signature_case bytes-leading 4 060 || b30sg=98
b30_signature_case seed-sign 5 +7 || b30sg=98
b30_signature_case hand-mv 6 mv || b30sg=98
b30_signature_case law-foreign 7 forged || b30sg=98
b30_signature_case opening-short 8 abc || b30sg=98
b30_signature_case opening-upper 8 ABCD || b30sg=98
b30_signature_case lived-zero 9 0 || b30sg=98
b30_signature_case lived-future 9 18446744073709551615 || b30sg=98
b30_signature_case extra-field 10 extra || b30sg=98
gate "B30 fourteen re-sealed malformed s events are not signatures" $b30sg 0

# Three signatures are interleaved with jury, units, atlas, reversed convoy
# order and two final resumes. The unsigned twin receives the same speech.
# Only s rows and the state's count/chain words may differ.
"$N" "$T/p3.bytes" "$T/p5.bytes" --reset --jury --seed 29 --episodes 2 --steps 300 --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" >/dev/null 2>&1
cp "$T/b30.long-a.state" "$T/b30.long-b.state"
cp "$T/b30.long-a.bio" "$T/b30.long-b.bio"
"$N" --jury --speak 73 --speak-seed 7 --sign --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.s1" 2>/dev/null
"$N" --jury --speak 73 --speak-seed 7 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.s1" 2>/dev/null
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 4 --steps 300 --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.1" 2>&1
r1=$?
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 4 --steps 300 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.1" 2>&1
r2=$?
"$N" --jury --speak 91 --speak-seed 8 --speak-laplace --sign --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.s2" 2>/dev/null
"$N" --jury --speak 91 --speak-seed 8 --speak-laplace --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.s2" 2>/dev/null
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 3 --steps 400 --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.2" 2>&1
r3=$?
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 3 --steps 400 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.2" 2>&1
r4=$?
"$N" --jury --speak 127 --speak-seed 9 --sign --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.s3" 2>/dev/null
"$N" --jury --speak 127 --speak-seed 9 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.s3" 2>/dev/null
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 2 --steps 500 --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.3" 2>&1
r5=$?
"$N" "$T/p3.bytes" "$T/p5.bytes" --jury --atlas --episodes 2 --steps 500 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.3" 2>&1
r6=$?
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 2 --steps 500 --state "$T/b30.long-a.state" --bio "$T/b30.long-a.bio" > "$T/b30.long-a.4" 2>&1
r7=$?
"$N" "$T/p5.bytes" "$T/p3.bytes" --jury --atlas --episodes 2 --steps 500 --state "$T/b30.long-b.state" --bio "$T/b30.long-b.bio" > "$T/b30.long-b.4" 2>&1
r8=$?
for x in 1 2 3 4; do
    grep -v '^biography: ' "$T/b30.long-a.$x" > "$T/b30.long-a.$x.clean"
    grep -v '^biography: ' "$T/b30.long-b.$x" > "$T/b30.long-b.$x.clean"
done
grep -v "^s	" "$T/b30.long-a.bio" > "$T/b30.long-a.clean"
b30extra=$(cmp -l "$T/b30.long-a.state" "$T/b30.long-b.state" | awk '$1 < 41 || $1 > 56 {n++} END {print n+0}')
b30long=0
[ "$r1" -eq 0 ] && [ "$r2" -eq 0 ] && [ "$r3" -eq 0 ] && [ "$r4" -eq 0 ] && [ "$r5" -eq 0 ] && [ "$r6" -eq 0 ] && [ "$r7" -eq 0 ] && [ "$r8" -eq 0 ] || b30long=98
for x in s1 s2 s3 1.clean 2.clean 3.clean 4.clean; do
    cmp -s "$T/b30.long-a.$x" "$T/b30.long-b.$x" || b30long=98
done
cmp -s "$T/b30.long-a.clean" "$T/b30.long-b.bio" || b30long=98
[ "$(grep -c "^s	" "$T/b30.long-a.bio")" -eq 3 ] || b30long=98
[ "$b30extra" -eq 0 ] || b30long=98
gate "B30 three signatures remain powerless through the long twin" $b30long 0

# --- S: sanitizers are executable law, not a remembered side run --------
cc -O1 -g -std=c11 -Wall -Wextra -Wpedantic \
   -fsanitize=address,undefined -fno-omit-frame-pointer \
   netta.c -lm -o "$T/netta-san" >"$T/san-build.log" 2>&1
rc=$?
cc -O1 -g -std=c11 -Wall -Wextra -Wpedantic \
   -fsanitize=address,undefined -fno-omit-frame-pointer \
   scripts/manifest_check.c -o "$T/mcheck-san" \
   >>"$T/san-build.log" 2>&1 || rc=$?
[ -s "$T/san-build.log" ] && rc=98
gate "S sanitizer builds are strict and silent" $rc 0
S="$T/netta-san"
MS="$T/mcheck-san"
"$MS" "$T/b29.words" < "$T/b29.manifest" >/dev/null 2> "$T/ms29.err"
rc=$?
"$MS" "$T/b29.words" < "$T/b29.long" >/dev/null 2>> "$T/ms29.err"
[ $? -eq 1 ] || rc=98
grep -Eq 'AddressSanitizer|runtime error:' "$T/ms29.err" && rc=98
gate "S ASan/UBSan silent through the independent manifest reader" $rc 0
"$S" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 \
    --state "$T/sr.state" --bio "$T/sr.bio" >/dev/null 2>"$T/sr.err"
rc=$?
"$S" "$T/all.bytes" --reset --seed 7 --episodes 1 --steps 200 \
    --state "$T/sb.state" --bio "$T/sb.bio" >/dev/null 2>"$T/sb.err" || rc=$?
[ -s "$T/sr.err" ] && rc=98
[ -s "$T/sb.err" ] && rc=98
gate "S ASan/UBSan silent on repeated and full-binary worlds" $rc 0
"$S" "$T/p3.bytes" --reset --seed 5 --episodes 24 --steps 600 \
    --state "$T/sm.state" --bio "$T/sm.bio" >/dev/null 2>"$T/sm.err"
rc=$?; [ -s "$T/sm.err" ] && rc=98
gate "S ASan/UBSan silent through move navigation and restart state v20" $rc 0
"$S" "$T/p3.bytes" "$T/alien.bytes" --reset --seed 5 --episodes 6 --steps 600 \
    --island 0 --state "$T/si.state" --bio "$T/si.bio" >/dev/null 2>"$T/si.err"
rc=$?
"$S" "$T/p3.bytes" "$T/alien.bytes" --seed 5 --episodes 8 --steps 600 \
    --island 1 --state "$T/si.state" --bio "$T/si.bio" >/dev/null 2>>"$T/si.err" || rc=$?
[ -s "$T/si.err" ] && rc=98
gate "S ASan/UBSan silent through island travel and local revocation" $rc 0
"$S" "$T/rep.bytes" "$T/uhome.bytes" --reset --seed 42 --episodes 1 --steps 4000 \
    --island 0 --state "$T/sd.state" --bio "$T/sd.bio" >/dev/null 2>"$T/sd.err"
rc=$?
"$S" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 30 --steps 600 \
    --island 1 --state "$T/sd.state" --bio "$T/sd.bio" >/dev/null 2>>"$T/sd.err" || rc=$?
"$S" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --actor-lock mv --start 16 \
    --episodes 1 --steps 128 --island 1 --state "$T/sd.state" \
    --bio "$T/sd.bio" >/dev/null 2>>"$T/sd.err" || rc=$?
"$S" "$T/rep.bytes" "$T/uhome.bytes" --seed 42 --episodes 1 --steps 4000 \
    --island 0 --state "$T/sd.state" --bio "$T/sd.bio" >/dev/null 2>>"$T/sd.err" || rc=$?
[ -s "$T/sd.err" ] && rc=98
gate "S ASan/UBSan silent through extinction, searched tombstones, and resurrection" $rc 0
"$S" "$T/p3.bytes" "$T/null.bytes" --reset --no-units --seed 5 \
    --episodes 6 --steps 600 --island 0 \
    --state "$T/sn.state" --bio "$T/sn.bio" >/dev/null 2>"$T/sn.err"
rc=$?
"$S" "$T/p3.bytes" "$T/null.bytes" --no-units --seed 5 \
    --episodes 1 --steps 2000 --island 1 \
    --state "$T/sn.state" --bio "$T/sn.bio" >/dev/null 2>>"$T/sn.err" || rc=$?
[ -s "$T/sn.err" ] && rc=98
gate "S ASan/UBSan silent through byte-bounded comity and null action" $rc 0
"$S" "$T/p3.bytes" "$T/rep.bytes" --reset --seed 5 --episodes 2 --steps 600 \
    --atlas --state "$T/sr14.state" --bio "$T/sr14.bio" >/dev/null 2>"$T/sr14.err"
rc=$?
"$S" "$T/rep.bytes" "$T/p3.bytes" --seed 5 --episodes 2 --steps 600 \
    --atlas --state "$T/sr14.state" --bio "$T/sr14.bio" >/dev/null 2>>"$T/sr14.err" || rc=$?
[ -s "$T/sr14.err" ] && rc=98
gate "S ASan/UBSan silent through the atlas and a reversed convoy" $rc 0
"$S" "$T/p5.bytes" --reset --seed 5 --episodes 4 --steps 2000 \
    --state "$T/sc16.state" --bio "$T/sc16.bio" >/dev/null 2>"$T/sc16.err"
rc=$?
"$S" "$T/rep.bytes" --reset --seed 42 --episodes 1 --steps 4000 \
    --state "$T/sc16b.state" --bio "$T/sc16b.bio" >/dev/null 2>>"$T/sc16.err" || rc=$?
[ -s "$T/sc16.err" ] && rc=98
gate "S ASan/UBSan silent through the learning core" $rc 0
"$S" "$T/p3.bytes" --reset --seed 5 --episodes 6 --steps 600 \
    --state "$T/ss18.state" --bio "$T/ss18.bio" >/dev/null 2>"$T/ss18.err"
rc=$?
"$S" --speak 200 --speak-seed 7 --prompt-file "$T/b18.p1" \
    --state "$T/ss18.state" --bio "$T/ss18.bio" >/dev/null 2>>"$T/ss18.err" || rc=$?
"$S" --speak 200 --speak-seed 7 --speak-laplace \
    --state "$T/ss18.state" --bio "$T/ss18.bio" >/dev/null 2>>"$T/ss18.err" || rc=$?
"$S" --speak 200 --speak-seed 7 --sign \
    --state "$T/ss18.state" --bio "$T/ss18.bio" >/dev/null 2>>"$T/ss18.err" || rc=$?
"$S" "$T/p3.bytes" --episodes 1 --steps 60 \
    --state "$T/ss18.state" --bio "$T/ss18.bio" >/dev/null 2>>"$T/ss18.err" || rc=$?
grep -v '^speak:' "$T/ss18.err" | grep -v '^speak support:' | grep -v '^spoke:' | \
    grep -v '^signed:' | \
    grep -v '^NETTA ZERO' > "$T/ss18.err2" || true
[ -s "$T/ss18.err2" ] && rc=98
gate "S ASan/UBSan silent through the speaking mouth" $rc 0
"$S" "$T/p3.bytes" "$T/x3.bytes" --ear "$T/b20.slice" \
    > /dev/null 2>"$T/ss20.err"
rc=$?; [ -s "$T/ss20.err" ] && rc=98
gate "S ASan/UBSan silent through the ear" $rc 0
"$S" "$T/b21.shore" --ear "$T/b21.aa" \
    --ear-context "$T/b21.ctx" > /dev/null 2>"$T/ss21.err"
rc=$?; [ -s "$T/ss21.err" ] && rc=98
gate "S ASan/UBSan silent through the context-bearing ear" $rc 0
"$S" "$T/p3.bytes" --ear "$T/b20.slice" --ear-twin \
    > /dev/null 2>"$T/ss23.err"
rc=$?; [ -s "$T/ss23.err" ] && rc=98
gate "S ASan/UBSan silent through the structural-twin ear" $rc 0
"$S" "$T/p3.bytes" --court "$T/b20.slice" > /dev/null 2>"$T/ss24.err"
rc=$?; [ -s "$T/ss24.err" ] && rc=98
gate "S ASan/UBSan silent through the pattern court" $rc 0
cp "$T/b28.state" "$T/ss28.state"; cp "$T/b28.bio" "$T/ss28.bio"
"$S" --cite "$T/b28.docket" --state "$T/ss28.state" \
    --bio "$T/ss28.bio" > /dev/null 2>"$T/ss28.err"
rc=$?; [ -s "$T/ss28.err" ] && rc=98
gate "S ASan/UBSan silent through the citation" $rc 0
"$S" "$T/p5.bytes" --reset --jury --seed 5 --episodes 1 --steps 300 \
    --state "$T/sc17.state" --bio "$T/sc17.bio" >/dev/null 2>"$T/sc17.err"
rc=$?
"$S" "$T/p5.bytes" --jury --seed 5 --episodes 1 --steps 300 \
    --state "$T/sc17.state" --bio "$T/sc17.bio" >/dev/null 2>>"$T/sc17.err" || rc=$?
[ -s "$T/sc17.err" ] && rc=98
gate "S ASan/UBSan silent through the plasticity jury and restart" $rc 0

echo "----"
if [ $FAIL -eq 0 ]; then echo "ALL GATES PASS"; exit 0; fi
echo "$FAIL GATE(S) FAILED"; exit 1
