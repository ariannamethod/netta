#!/bin/sh
# Invariant probes for Netta.
#
# Each probe states an invariant, produces a machine verdict, and reports
# PASS or FAIL. Nothing here reads a number from prose: every figure comes
# from a run performed in this script, and the reference values are the ones
# an audit can restate independently.
#
#   ./probes.sh            run every probe
#   ./probes.sh 3 5        run only probes 3 and 5
#
# Exit code is the number of failing probes.

set -u

ROOT=$(cd "$(dirname "$0")" && pwd)
WORK=${NETTA_PROBE_DIR:-$(mktemp -d)}
BIN=$WORK/netta
CC=${CC:-cc}
FAILED=0
RAN=0
WANT="$*"

say()  { printf '%s\n' "$*"; }
pass() { RAN=$((RAN+1)); say "PASS  $1"; }
fail() { RAN=$((RAN+1)); FAILED=$((FAILED+1)); say "FAIL  $1"; [ $# -gt 1 ] && say "      $2"; }
want() { [ -z "$WANT" ] && return 0; for w in $WANT; do [ "$w" = "$1" ] && return 0; done; return 1; }

# A fresh directory per condition: probes must never inherit another's state.
world() {
    d=$WORK/$1
    rm -rf "$d"
    mkdir -p "$d"
    cp "$ROOT/netta.txt" "$d/"
    printf '%s' "$d"
}

coherence() { awk -F'\t' 'NR>1{n++;s+=$15} END{if(n)printf "%.4f", s/n}' "$1/netta.probe.tsv"; }
column()    { awk -F'\t' -v c="$2" 'NR>1{n++;s+=$c} END{if(n)printf "%.4f", s/n}' "$1/netta.probe.tsv"; }

say "netta probes"
say "source $(cd "$ROOT" && git rev-parse --short HEAD 2>/dev/null || echo '(not a repo)')"
say "workdir $WORK"
say ""

# --- build ------------------------------------------------------------------
if want 1; then
    out=$($CC -O2 -std=c11 -Wall -Wextra -Wpedantic -o "$BIN" "$ROOT/netta.c" -lm 2>&1)
    if [ -n "$out" ]; then
        fail "1 build is clean under -Wall -Wextra -Wpedantic" "$(printf '%s' "$out" | head -3)"
    else
        pass "1 build is clean under -Wall -Wextra -Wpedantic"
    fi
else
    $CC -O2 -std=c11 -o "$BIN" "$ROOT/netta.c" -lm 2>/dev/null
fi
[ -x "$BIN" ] || { say "cannot build; stopping"; exit 99; }

# --- invariant 4: no seam physics; a world too short to play is refused ------
if want 2; then
    d=$(world tiny)
    mkdir -p "$d/nettatexts"
    printf 'ref: refs/heads/main\n' > "$d/nettatexts/tiny.md"
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 1 --island 1 ) \
        > "$d/out" 2>&1
    rc=$?
    if [ $rc -eq 0 ] && grep -q '#' "$d/out" 2>/dev/null && grep -q 'island 1' "$d/out"; then
        fail "2 a three-token file is refused as a world" \
             "registered and played; context ran past its end"
    elif [ $rc -eq 0 ] && grep -q 'island 1' "$d/out"; then
        fail "2 a three-token file is refused as a world" "registered as island 1"
    else
        pass "2 a three-token file is refused as a world"
    fi
fi

# --- invariant 5: failure is evidence ---------------------------------------
if want 3; then
    d=$(world ledger)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 4 ) >/dev/null 2>&1
    before=$(wc -l < "$d/netta.history.tsv")
    ( cd "$d" && "$BIN" netta.txt --random-glyph --seed 42 --steps 0 ) >/dev/null 2>&1
    rc=$?
    after=$(wc -l < "$d/netta.history.tsv")
    if [ "$before" = "$after" ] && [ $rc -ne 0 ]; then
        pass "3 an unreadable snapshot fails instead of erasing the ledger"
    else
        fail "3 an unreadable snapshot fails instead of erasing the ledger" \
             "rc=$rc, ledger $before -> $after rows"
    fi
fi

# --- invariant 3: identity before memory ------------------------------------
if want 4; then
    d=$(world negative)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 0 --island -1 ) \
        >/dev/null 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then
        pass "4 a negative island index is refused"
    else
        fail "4 a negative island index is refused" "rc=0"
    fi
fi

# --- invariant 1: world locality --------------------------------------------
# A zero-episode organism saved on A and resumed on B must be indistinguishable
# from one born on B: no episode has been played, so nothing may differ.
if want 5; then
    fresh=$(world loc_fresh); mkdir -p "$fresh/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$fresh/nettatexts/"
    ( cd "$fresh" && "$BIN" netta.txt --reset --seed 42 --steps 0 --island 1 --probe 128 ) \
        >/dev/null 2>&1

    ab=$(world loc_ab); mkdir -p "$ab/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$ab/nettatexts/"
    ( cd "$ab" && "$BIN" netta.txt --reset --seed 42 --steps 0 --island 0 ) >/dev/null 2>&1
    ( cd "$ab" && "$BIN" netta.txt --seed 42 --steps 0 --island 1 --probe 128 ) \
        >/dev/null 2>&1

    f=$(column "$fresh" 4); a=$(column "$ab" 4)
    fb=$(column "$fresh" 6); ab_=$(column "$ab" 6)
    if [ "$f" = "$a" ] && [ "$fb" = "$ab_" ]; then
        pass "5 arriving in a world with no episodes played changes nothing ($f / $fb)"
    else
        fail "5 arriving in a world with no episodes played changes nothing" \
             "token accuracy $f -> $a, bigrams $fb -> $ab_"
    fi
fi

# --- invariant 2: biography continuity, restart is not a fork ---------------
if want 6; then
    a=$(world restart_a); b=$(world restart_b)
    ( cd "$a" && "$BIN" netta.txt --reset --seed 42 --steps 160 ) >/dev/null 2>&1
    ( cd "$b" && "$BIN" netta.txt --reset --seed 42 --steps 80 ) >/dev/null 2>&1
    ( cd "$b" && "$BIN" netta.txt --seed 42 --steps 80 ) >/dev/null 2>&1
    sa=$(shasum -a 256 < "$a/netta.state" | cut -d' ' -f1)
    sb=$(shasum -a 256 < "$b/netta.state" | cut -d' ' -f1)
    la=$(shasum -a 256 < "$a/netta.history.tsv" | cut -d' ' -f1)
    lb=$(shasum -a 256 < "$b/netta.history.tsv" | cut -d' ' -f1)
    if [ "$sa" = "$sb" ] && [ "$la" = "$lb" ]; then
        pass "6 160 episodes equal 80 + restart + 80, byte for byte"
    else
        fail "6 160 episodes equal 80 + restart + 80, byte for byte" \
             "state $(printf '%.8s' "$sa")/$(printf '%.8s' "$sb") ledger $(printf '%.8s' "$la")/$(printf '%.8s' "$lb")"
    fi
fi

# --- invariant 3: a world that changed under her is not her world ----------
if want 7; then
    d=$(world manifest); mkdir -p "$d/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$d/nettatexts/"
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 2 --island 1 ) >/dev/null 2>&1
    printf '\nan appended sentence that was not there before\n' >> "$d/nettatexts/MODELCARD.md"
    ( cd "$d" && "$BIN" netta.txt --seed 42 --steps 1 --island 1 ) > "$d/out" 2>&1
    rc=$?
    if [ $rc -ne 0 ]; then
        pass "7 resume into a world whose text changed is refused"
    else
        fail "7 resume into a world whose text changed is refused" \
             "rc=0, resumed as if nothing happened"
    fi
fi

# --- invariant 1 again: island life is not one shared curriculum ------------
if want 8; then
    alone=$(world life_alone); mkdir -p "$alone/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$alone/nettatexts/"
    ( cd "$alone" && "$BIN" netta.txt --reset --seed 42 --steps 60 --island 0 ) >/dev/null 2>&1
    ( cd "$alone" && "$BIN" netta.txt --seed 42 --steps 1 --island 0 --probe 128 ) >/dev/null 2>&1

    via=$(world life_via); mkdir -p "$via/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$via/nettatexts/"
    ( cd "$via" && "$BIN" netta.txt --reset --seed 42 --steps 60 --island 0 ) >/dev/null 2>&1
    ( cd "$via" && "$BIN" netta.txt --seed 42 --steps 60 --island 1 ) >/dev/null 2>&1
    ( cd "$via" && "$BIN" netta.txt --seed 42 --steps 1 --island 0 --probe 128 ) >/dev/null 2>&1

    ca=$(awk -F'\t' 'NR>1{n++;s+=$29} END{if(n)printf "%.5f", s/n}' "$alone/netta.history.tsv")
    cv=$(awk -F'\t' 'NR>1{n++;s+=$29} END{if(n)printf "%.5f", s/n}' "$via/netta.history.tsv")
    if [ -n "$ca" ] && [ "$ca" = "$cv" ]; then
        pass "8 time spent in another world leaves this world's curriculum alone"
    else
        fail "8 time spent in another world leaves this world's curriculum alone" \
             "mean curriculum progress $ca vs $cv"
    fi
fi

# --- invariant 6: authority is earned, not aged -----------------------------
if want 9; then
    d=$(world authority)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 30 ) >/dev/null 2>&1
    if [ -f "$d/netta.receipts.tsv" ]; then
        pass "9 a decision can be inspected before it teaches anything"
    else
        fail "9 a decision can be inspected before it teaches anything" \
             "no receipt written; --receipts absent"
    fi
fi

# --- sanitizers -------------------------------------------------------------
if want 10; then
    san=$WORK/netta_san
    $CC -O1 -g -std=c11 -fsanitize=address,undefined -o "$san" "$ROOT/netta.c" -lm 2>/dev/null
    if [ ! -x "$san" ]; then
        fail "10 four episodes and a probe run clean under ASan/UBSan" "sanitizer build failed"
    else
        d=$(world sanitizer); mkdir -p "$d/nettatexts"
        cp "$ROOT/docs/MODELCARD.md" "$d/nettatexts/"
        ( cd "$d" && "$san" netta.txt --reset --seed 42 --steps 4 --probe 8 --island 1 ) \
            >/dev/null 2>"$d/err"
        hits=$(grep -ciE 'AddressSanitizer|runtime error' "$d/err")
        if [ "$hits" = "0" ]; then
            pass "10 four episodes and a probe run clean under ASan/UBSan"
        else
            fail "10 four episodes and a probe run clean under ASan/UBSan" \
                 "$(grep -m1 -E 'AddressSanitizer|runtime error' "$d/err")"
        fi
    fi
fi

# --- a newborn is the same newborn on every hand ---------------------------
# Anchored by a second auditor on a separately built binary. If a change to
# her learning moves this, it moved something it had no business touching.
if want 11; then
    d=$(world newborn)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 424242 --steps 0 --probe 128 ) \
        > "$d/exam" 2>&1
    tri=$(awk '/corpus trigrams:/{print $3}' "$d/exam")
    coh=$(awk '/coherence outcome:/{print $3}' "$d/exam")
    if [ "$tri" = "0.5996" ] && [ "$coh" = "0.5777" ]; then
        pass "11 a newborn scores 0.5996 trigrams and 0.5777 coherence"
    else
        fail "11 a newborn scores 0.5996 trigrams and 0.5777 coherence" \
             "got $tri and $coh"
    fi
fi

say ""
say "$((RAN-FAILED))/$RAN passed"
[ -n "${NETTA_PROBE_DIR:-}" ] || say "receipts under $WORK"
exit $FAILED
