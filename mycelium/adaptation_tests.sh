#!/bin/sh
# Body A1 gates.  The first hand writes; the second shares no code and reads.
set -u

ROOT=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
CAP="$ROOT/mycelium/court4_biography"
T=$(mktemp -d /tmp/mycelium-a1-tests.XXXXXX) || exit 1
trap 'rm -rf "$T"' EXIT HUP INT TERM
W="$T/adaptation_witness"
R="$T/adaptation_check"
N=0

pass() { N=$((N+1)); printf 'PASS %02d  %s\n' "$N" "$1"; }
fail() { printf 'FAIL      %s\n' "$1" >&2; exit 1; }
expect_fail() {
    name=$1; needle=$2; shift 2
    if "$@" >"$T/out" 2>"$T/err"; then
        fail "$name (accepted)"
    elif grep -F "$needle" "$T/out" "$T/err" >/dev/null; then
        pass "$name"
    else
        fail "$name (wrong refusal: $(tr '\n' ' ' <"$T/err"))"
    fi
}

${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/adaptation_witness.cpp" -o "$W" || exit 1
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    "$ROOT/mycelium/adaptation_check.c" -o "$R" || exit 1
pass "both A1 hands build strict"

mkdir "$T/room"
for f in .mycelium.ledger .mycelium.proposals .mycelium.school \
         .mycelium.parliament .mycelium.notes .mycelium.circulation; do
    printf 'older-body-sentinel:%s\n' "$f" >"$T/room/$f"
done
before=$(shasum -a 256 "$T/room/.mycelium.ledger" \
    "$T/room/.mycelium.proposals" "$T/room/.mycelium.school" \
    "$T/room/.mycelium.parliament" "$T/room/.mycelium.notes" \
    "$T/room/.mycelium.circulation" | shasum -a 256)
(cd "$T/room" && "$W" "$CAP" >writer.out) || fail "writer rejected sealed capsule"
after=$(shasum -a 256 "$T/room"/.mycelium.ledger \
    "$T/room"/.mycelium.proposals "$T/room"/.mycelium.school \
    "$T/room"/.mycelium.parliament "$T/room"/.mycelium.notes \
    "$T/room"/.mycelium.circulation | shasum -a 256)
[ "$before" = "$after" ] || fail "A1 changed an older body"
pass "writer witnesses the sealed public capsule and changes no older body"

(cd "$T/room" && "$R" >reader.out) || fail "independent reader rejected witness"
grep -F '162 candidates / 5 live / 5 ever-earned, 230 controls' \
    "$T/room/reader.out" >/dev/null || fail "reader census"
pass "independent reader derives the exact citizen/control census"

ledger_before=$(shasum -a 256 "$T/room/.mycelium.court4-witness")
blob_before=$(shasum -a 256 "$T/room/.mycelium.court4-witness.d"/*)
(cd "$T/room" && "$W" "$CAP" >again.out) || fail "idempotent witness refused"
[ "$ledger_before" = "$(shasum -a 256 "$T/room/.mycelium.court4-witness")" ] || \
    fail "duplicate witness changed ledger"
[ "$blob_before" = "$(shasum -a 256 "$T/room/.mycelium.court4-witness.d"/*)" ] || \
    fail "duplicate witness changed blob"
grep -F 'already witnessed Court 4' "$T/room/again.out" >/dev/null || \
    fail "duplicate witness wording"
pass "the same Court cannot be imported twice"

mkdir "$T/room2"
(cd "$T/room2" && "$W" "$CAP" >/dev/null) || fail "clean-room writer"
cmp "$T/room/.mycelium.court4-witness" \
    "$T/room2/.mycelium.court4-witness" >/dev/null || fail "ledger reproduction"
cmp "$T/room/.mycelium.court4-witness.d"/* \
    "$T/room2/.mycelium.court4-witness.d"/* >/dev/null || fail "blob reproduction"
pass "clean rooms reproduce ledger and biography byte for byte"

# The spent ff biography contains no REVOKE.  Exercise the frozen transition
# grammar in a compile-time-pinned miniature capsule; production has no flag
# that can weaken its exact Court-4 identities.
mkdir "$T/revoke-cap" "$T/revoke-room"
python3 - "$T/revoke-cap" "$T/revoke-pins.h" <<'PY'
import hashlib, pathlib, sys
d=pathlib.Path(sys.argv[1]); pins=pathlib.Path(sys.argv[2])
eh=("byte_offset\tunit_position\tarm\tevent\tledger_after\ttarget_s\t"
    "target_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\t"
    "c2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\n")
key="97\t97\t1\t1\t32\t32\t1\t0\t0\t0\t0\t0\t0"
events=(eh+"1\t1\trelation\tearn\t33\t"+key+"\n"+
           "2\t2\trelation\trevoke\t10\t"+key+"\n").encode()
rh=("arm\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\t"
    "c1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\tseen\t"
    "positive\tnegative\tledger_bits\tpeak_bits\tstate\tL\tever_earned\n")
relations=(rh+"relation\t"+key+"\t2\t1\t1\t10\t33\t0\t0\t1\n").encode()
es=hashlib.sha256(events).hexdigest(); rs=hashlib.sha256(relations).hexdigest()
manifest=("domain\tarianna-method.netta.mycelium.court4-biography/v1\n"
"source_commit\te02c645\n"
"verdict\tCONFIRMATORY PASS: microscopic relation replicated in selected class\n"
"stage\troots2\tCOURT4_CONFIRMATORY_ROOTS2.tsv\t2123\t9b21de267935713d4d4b84d7281aca6968e1071b5878c558b99b58668b3bc3bc\n"
"stage\tbase_commit2\tBASE_COMMIT2.tsv\t186\tacf118b62a9505e5c4f3066239b97710ea5dce8d15cf3faf9d950abd49b06f31\n"
"stage\tfreeze2\tCOURT4_BASE_COMMIT_FREEZE2.tsv\t274\t5de9b2f356353e3a22500dc3196f5f35f2f6d4bb32eb6d36f6d71c235e1287da\n"
"stage\tselection2\tSELECTION2.tsv\t901\t69c3ee3d0e8e4d0b68f3f975edbba1245c95cb0b19599227a7f526996264dacc\n"
"stage\tbuilder_receipt\tCOURT4_DRAW2_BUILDER_OUTPUT_RECEIPT.tsv\t4379\t5281e130d35ef677fad7893fd96bee626a51e3fae9ad4aff5b70ea19299a5678\n"
"stage\tc8_verdict\tCOURT4_DRAW2_C8_VERDICT_RECORD.tsv\t5398\tb528c3e2efe1a2dd38b892e18f67e2db98338b61e617c1b581f119686fc76b1e\n"
f"artifact\tevents\trelation_events.tsv\t{len(events)}\t{es}\n"
f"artifact\trelations\trelations.tsv\t{len(relations)}\t{rs}\n"
"summary\tevents\t2\nsummary\tearn\t1\nsummary\trevoke\t1\n"
"summary\trelation_candidates\t1\nsummary\trelation_live\t0\n"
"summary\trelation_ever_earned\t1\nsummary\tcontrol_rows\t0\n").encode()
(d/'relation_events.tsv').write_bytes(events)
(d/'relations.tsv').write_bytes(relations)
(d/'MANIFEST.tsv').write_bytes(manifest)
pins.write_text(f'#define A1_EVENTS_BYTES {len(events)}\n#define A1_EVENTS_SHA "{es}"\n'
                f'#define A1_RELATIONS_BYTES {len(relations)}\n#define A1_RELATIONS_SHA "{rs}"\n'
                f'#define MANIFEST_SHA "{hashlib.sha256(manifest).hexdigest()}"\n')
PY
${CXX:-c++} -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror \
    -include "$T/revoke-pins.h" "$ROOT/mycelium/adaptation_witness.cpp" \
    -o "$T/revoke-writer" || exit 1
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Wpedantic -Werror \
    -include "$T/revoke-pins.h" "$ROOT/mycelium/adaptation_check.c" \
    -o "$T/revoke-reader" || exit 1
(cd "$T/revoke-room" && "$T/revoke-writer" "$T/revoke-cap" >/dev/null && \
    "$T/revoke-reader" >reader.out) || fail "EARN/REVOKE transition probe"
grep -F '2 events (1 EARN, 1 REVOKE), 1 candidates / 0 live / 1 ever-earned' \
    "$T/revoke-room/reader.out" >/dev/null || fail "REVOKE census"
pass "both hands replay EARN then REVOKE without granting live state"

cp -R "$CAP" "$T/cap-event-flip"
python3 - "$T/cap-event-flip/relation_events.tsv" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
b = bytearray(p.read_bytes())
b[b.index(b"33.997345")]=ord("4")
p.write_bytes(b)
PY
expect_fail "writer refuses a flipped Court event book" \
    "artifact identity mismatch" "$W" "$T/cap-event-flip" \
    "$T/event-flip-ledger"
[ ! -e "$T/event-flip-ledger" ] || fail "failed import wrote a ledger"

cp -R "$CAP" "$T/cap-summary"
python3 - "$T/cap-summary/MANIFEST.tsv" <<'PY'
import pathlib, sys
p = pathlib.Path(sys.argv[1])
p.write_text(p.read_text().replace("summary\trelation_live\t5\n",
                                  "summary\trelation_live\t6\n"))
PY
expect_fail "writer recomputes rather than trusts the manifest census" \
    "manifest summary drifted" "$W" "$T/cap-summary" "$T/summary-ledger"

mkdir "$T/cap-link"
cp "$CAP/MANIFEST.tsv" "$CAP/relations.tsv" "$T/cap-link/"
ln -s "$CAP/relation_events.tsv" "$T/cap-link/relation_events.tsv"
expect_fail "writer refuses a symlinked artifact" "not a regular file" \
    "$W" "$T/cap-link" "$T/link-ledger"

cp -R "$T/room" "$T/reader-chain"
python3 - "$T/reader-chain/.mycelium.court4-witness" <<'PY'
import pathlib, sys
p=pathlib.Path(sys.argv[1]); b=bytearray(p.read_bytes()); b[-2]^=1; p.write_bytes(b)
PY
expect_fail "reader refuses a witness-chain flip" "chain broken" \
    "$R" "$T/reader-chain/.mycelium.court4-witness"

cp -R "$T/room" "$T/reader-trunc"
python3 - "$T/reader-trunc/.mycelium.court4-witness" <<'PY'
import pathlib, sys
p=pathlib.Path(sys.argv[1]); p.write_bytes(p.read_bytes()[:-1])
PY
expect_fail "reader refuses an unsealed witness tail" "cleanly sealed" \
    "$R" "$T/reader-trunc/.mycelium.court4-witness"

cp -R "$T/room" "$T/reader-census"
python3 - "$T/reader-census/.mycelium.court4-witness" <<'PY'
import pathlib, sys
SEED=0xcbf29ce484222325; PRIME=0x100000001b3
def fold(b,h):
    for x in b: h=((h^x)*PRIME)&0xffffffffffffffff
    return h
p=pathlib.Path(sys.argv[1]); rows=p.read_bytes().splitlines(); h=SEED; out=[]
for i,row in enumerate(rows):
    payload=row[:-17]
    if i==1:
        f=payload.split(b'\t'); f[10]=b'163'; payload=b'\t'.join(f)
    h=fold(payload,h); out.append(payload+b'\t'+f'{h:016x}'.encode())
p.write_bytes(b'\n'.join(out)+b'\n')
PY
expect_fail "reader rejects a canonically resealed false census" \
    "census disagrees" "$R" "$T/reader-census/.mycelium.court4-witness"

cp -R "$T/room" "$T/reader-semantic"
python3 - "$T/reader-semantic/.mycelium.court4-witness" <<'PY'
import hashlib, pathlib, sys
SEED=0xcbf29ce484222325; PRIME=0x100000001b3
def fold(b,h):
    for x in b: h=((h^x)*PRIME)&0xffffffffffffffff
    return h
ledger=pathlib.Path(sys.argv[1]); old=ledger.read_bytes().splitlines()
wf=old[1][:-17].split(b'\t'); blobdir=pathlib.Path(str(ledger)+'.d')
blob=(blobdir/wf[6].decode()).read_bytes()
rows=blob.split(b'\n',4); head=b'\n'.join(rows[:4])+b'\n'; body=rows[4]
sizes=[int(rows[i].split(b'\t')[1]) for i in range(1,4)]
m=body[:sizes[0]]; e=body[sizes[0]:sizes[0]+sizes[1]]; r=body[sizes[0]+sizes[1]:]
rr=r.splitlines()
for i in range(1,len(rr)):
    f=rr[i].split(b'\t')
    if f[0]==b'relation' and f[19]==b'1': f[19]=b'0'; rr[i]=b'\t'.join(f); break
r=b'\n'.join(rr)+b'\n'; rh=hashlib.sha256(r).hexdigest().encode()
hdr=[rows[0],rows[1],rows[2],b'relations\t'+str(len(r)).encode()+b'\t'+rh]
newblob=b'\n'.join(hdr)+b'\n'+m+e+r; bh=hashlib.sha256(newblob).hexdigest().encode()
(blobdir/bh.decode()).write_bytes(newblob)
payloads=[old[0][:-17],old[1][:-17]]; f=payloads[1].split(b'\t'); f[5]=rh; f[6]=bh; payloads[1]=b'\t'.join(f)
h=SEED; out=[]
for p in payloads: h=fold(p,h); out.append(p+b'\t'+f'{h:016x}'.encode())
ledger.write_bytes(b'\n'.join(out)+b'\n')
PY
expect_fail "reader rejects a fully resealed alternative relation book" \
    "witness Court or manifest identity drifted" \
    "$R" "$T/reader-semantic/.mycelium.court4-witness"

printf '%s\n' '----' "ALL $N ADAPTATION A1 GATES PASS"
