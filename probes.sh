#!/bin/sh
# Invariant probes for Netta.
#
# Each probe states an invariant, produces a machine verdict, and reports
# PASS or FAIL. Nothing here reads a number from prose: every figure comes
# from a run performed in this script, and the reference values are the ones
# an audit can restate independently.
#
#   ./probes.sh            run every probe except 13 (15/15 when healthy)
#   ./probes.sh 3 5        run only probes 3 and 5
#   ./probes.sh 13         geometry health -- RED until geometry repair (S3),
#                          excluded from the default run on purpose, run
#                          explicitly
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
# exam_field FILE combined|source bpt|in_pool|escape|ft -- one field off the
# "exam[reading] bpt=.. in_pool=.. escape=.. ft=.." line run_exam prints.
exam_field() {
    grep "exam\[$2\]" "$1" | sed -n "s/.*[[:space:]]$3=\([0-9.]*\).*/\1/p"
}

say "netta probes"
say "source $(cd "$ROOT" && git rev-parse --short HEAD 2>/dev/null || echo '(not a repo)')"
say "workdir $WORK"
say ""

# A harness that judges fail-closed behaviour must itself be fail-closed. A
# missing fixture used to leave the comparisons empty, and empty compared
# equal to empty: probes reported PASS while measuring nothing at all.
for fixture in "$ROOT/netta.txt" "$ROOT/docs/MODELCARD.md" "$ROOT/docs/HELDOUT.md"; do
    [ -f "$fixture" ] || { say "missing fixture: $fixture"; exit 99; }
done

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
    if [ -z "$f" ] || [ -z "$a" ] || [ -z "$fb" ] || [ -z "$ab_" ]; then
        fail "5 arriving in a world with no episodes played changes nothing" \
             "no measurement taken: probe ledger missing or empty"
    elif [ "$f" = "$a" ] && [ "$fb" = "$ab_" ]; then
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

# --- invariant 1 again: island life is not one shared curriculum, and her
# entry into a second world is not timed by a stranger's clock -------------
# Strengthened per Sol's S5c audit (P0-1): the original check only read the
# printed progress column, which curriculum_update writes unconditionally --
# it can be nonzero while curriculum_priority_for still refuses the same
# region as zero, because load_state ran after curriculum_init and restored
# an older, smaller curriculum_count over the fresh island's slice. Two more
# assertions close that false-negative: the entry priority itself, and a
# causal-scheduling proof that the island's own exposure clock, not her whole
# lifetime, decides which region a first visit lands in.
if want 8; then
    alone=$(world life_alone); mkdir -p "$alone/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$alone/nettatexts/"
    ( cd "$alone" && "$BIN" netta.txt --reset --seed 42 --steps 60 --island 0 ) >/dev/null 2>&1
    ( cd "$alone" && "$BIN" netta.txt --seed 42 --steps 1 --island 0 --probe 128 ) >/dev/null 2>&1

    ( cd "$alone" && "$BIN" netta.txt --seed 42 --steps 60 --island 1 ) >/dev/null 2>&1

    # Comparing progress between two organisms cannot answer this: one that
    # also lived elsewhere is simply older, and region choice depends on the
    # episode count. What is observable is the symptom the defect left: the
    # position of an episode was local to its world while the region it
    # updated was indexed from the whole corpus, so on any world but the
    # first the update fell outside the table and the curriculum silently
    # learned nothing at all.
    prog=$(awk -F'\t' 'NR>62{n++; s+=$30} END{if(n)printf "%.5f", s/n}' \
        "$alone/netta.history.tsv")

    # Entry priority (P0-1a): row 63 is the transfer arm's first island-1
    # episode (60 rows for island 0, one more resumed island-0 row for the
    # --probe 128 step, then island 1 begins). Under the pre-fix bug this
    # region reads exactly 0.00000 forever, because it sits at or past the
    # curriculum_count the snapshot restored over the fresh island's slice.
    entry_region=$(awk -F'\t' 'NR==63{print $28}' "$alone/netta.history.tsv")
    entry_priority=$(awk -F'\t' 'NR==63{print $29}' "$alone/netta.history.tsv")

    # Causal scheduling + paired entry (P0-1b): a newborn on the SAME
    # island, same seed, played from scratch. If region selection were still
    # keyed by global episode_count (60 already lived on island 0) instead
    # of this island's own exposure clock, the transfer arm and the newborn
    # would survey different neighbourhoods of island 1 on their first
    # episode; paired, they must land on the same region.
    newborn=$(world life_newborn_b); mkdir -p "$newborn/nettatexts"
    cp "$ROOT/docs/MODELCARD.md" "$newborn/nettatexts/"
    ( cd "$newborn" && "$BIN" netta.txt --reset --seed 42 --steps 1 --island 1 ) \
        >/dev/null 2>&1
    newborn_region=$(awk -F'\t' 'NR==2{print $28}' "$newborn/netta.history.tsv")

    label="8 a second world's curriculum learns at all, enters at a live priority, and is timed by its own clock"
    if [ -z "$prog" ] || [ -z "$entry_region" ] || [ -z "$entry_priority" ] || \
       [ -z "$newborn_region" ]; then
        fail "$label" "no measurement taken: a ledger is missing or too short"
    elif [ "$prog" = "0.00000" ]; then
        fail "$label" "progress stayed at $prog across 60 episodes"
    elif [ "$entry_priority" = "0.00000" ]; then
        fail "$label" \
             "entry priority read 0.00000 at region $entry_region -- the P0-1 curriculum-count seam"
    elif [ "$entry_region" != "$newborn_region" ]; then
        fail "$label" \
             "transfer arm entered region $entry_region, a paired newborn entered $newborn_region -- unpaired B exposure"
    else
        pass "$label (progress=$prog entry_priority=$entry_priority region=$entry_region, paired with newborn)"
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
# If a change to her learning moves this, it moved something it had no
# business touching.
#
# The first anchor, 0.5996/0.5777, pinned a defect: successor tables ordered
# by biography rather than by the text. Removing that order moved the
# newborn to 0.6318/0.5828 -- confirmed independently at 6b525b0 by a second
# auditor on a separately built binary -- and closing the same defect in the
# experience scan moved it again, and making the oracle deterministic moved
# it a third time, to the values below -- taken by the auditor on f2a09ce
# and transcribed here. The order was not only nondeterministic; it was
# noise she was born carrying.
#
# A third re-pin, F5b: the word-geometry pass moved from float to double
# after 4421 of 8153 words (54%) turned out to be silent zero embeddings,
# a float32 accumulator overflow masked as a normal-looking vector until
# checked directly. The newborn's coherence moved 0.5738 -> 0.7218, taken
# by two hands independently on separately built binaries.
#
# Sol's audit (P0-1) named this pin what it actually is: a reproducibility
# invariant for the program, not a quality anchor. Almost all of F5b's
# apparent gain came from cosine channels declaring nearly every candidate
# equivalent to truth once the geometry collapsed to a cone -- discrimination
# fell (first-token 0.3594 -> 0.2520 at 512 positions) while this composite
# rose. Split into three probes: this one stays a pin on the program, probe
# 12 pins the external-quality coordinates the composite was hiding, and
# probe 13 measures whether the geometry backing all of it is healthy.
#
# S3 re-pin (four-figure age): three geometries replace the compound pass --
# a newborn's combined = island (residual is zero at birth), rebuilt by the
# frozen-seed sweep instead of one compounding pass. The pin moves from
# 0.6328/0.7218 to 0.6309/0.6203, taken on two independently built binaries
# by Sonnet and confirmed independently by Fable before acceptance.
#
# Checked, not moved, at S5c core shadow authority (the fifth re-pin epoch,
# see probe 14): a newborn plays 0 episodes, and island_core_gate starts at
# 0.0 exactly like the old age-based clamp(episode_count/2500, 0, 0.65) did
# at episode_count=0 -- the two mechanisms agree at birth by construction,
# so this pin and probe 12's do not move even though the mechanism under
# them changed.
if want 11; then
    d=$(world newborn)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 424242 --steps 0 --probe 128 ) \
        > "$d/exam" 2>&1
    tri=$(awk '/corpus trigrams:/{print $3}' "$d/exam")
    coh=$(awk '/coherence outcome:/{print $3}' "$d/exam")
    if [ -z "$tri" ] || [ -z "$coh" ]; then
        fail "11 newborn reproducibility pin (not a quality claim): 0.6309 trigrams, 0.6203 coherence" \
             "no measurement taken: exam produced nothing"
    elif [ "$tri" = "0.6309" ] && [ "$coh" = "0.6203" ]; then
        pass "11 newborn reproducibility pin (not a quality claim): 0.6309 trigrams, 0.6203 coherence"
    else
        fail "11 newborn reproducibility pin (not a quality claim): 0.6309 trigrams, 0.6203 coherence" \
             "got $tri and $coh"
    fi
fi

# --- external quality coordinates, the numbers probe 11's composite hid ----
# Taken at 512 positions rather than 128 -- the wider sample the audit used
# to show the composite's gain was discrimination loss wearing a rising
# number. Pinned as reproducibility, exactly like probe 11: these are not
# claimed to be good, only to be the honest, reproducible external read.
#
# S3 re-pin: 0.2520/0.0515/0.9848/0.6589 -> 0.2910/0.0623/0.9931/0.6448 --
# the exact coordinates of Sol's own falsifier prototype, now produced by
# the frozen-seed geometry this pin measures rather than by a counterfactual
# patch. Three of four move up (discrimination recovered from the cone);
# trigram moves down 0.0141, the expected, disclosed trade of a
# non-compounding pass, not a regression.
if want 12; then
    d=$(world quality512)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 424242 --steps 0 --probe 512 ) \
        > "$d/exam" 2>&1
    ft=$(awk '/^  first-token accuracy:/{print $3}' "$d/exam")
    ta=$(awk '/^  token accuracy:/{print $3}' "$d/exam")
    bg=$(awk '/^  corpus bigrams:/{print $3}' "$d/exam")
    tg=$(awk '/^  corpus trigrams:/{print $3}' "$d/exam")
    label="12 newborn external quality pin: first-token 0.2910, token 0.0623, bigram 0.9931, trigram 0.6448"
    if [ -z "$ft" ] || [ -z "$ta" ] || [ -z "$bg" ] || [ -z "$tg" ]; then
        fail "$label" "no measurement taken: exam produced nothing"
    elif [ "$ft" = "0.2910" ] && [ "$ta" = "0.0623" ] && \
         [ "$bg" = "0.9931" ] && [ "$tg" = "0.6448" ]; then
        pass "$label"
    else
        fail "$label" "got first-token=$ft token=$ta bigram=$bg trigram=$tg"
    fi
fi

# --- anchor80: a canonical lived organism, not just a newborn -------------
# Probes 11/12 pin the newborn; nothing until now pinned a life. Seed 42
# (same convention as probe 6), 80 episodes, --reset. Two independent binary
# builds on this tree produced identical sha256 for both netta.state and
# netta.history.tsv -- geometry, residual, floor and core together, the
# whole organism after the first half of probe 6's own restart split.
#
# v45 re-pin: S5c P0-1 adds a persisted per-island exposure clock
# (island_episode_count) to the state format and bumps STATE_VERSION
# 44 -> 45. netta.history.tsv -- the byte-for-byte record of every decision
# this life made -- is unchanged (still 110e2aadef11...): the fix does not
# move the single-island path. Only netta.state moves, mechanically, because
# any new persisted field forces a version bump. Reproduced on two
# independent builds by Claude, confirmed by a third independent build by
# Sol: v45: per-island exposure clocks, ledger unchanged -- пятый пере-пин,
# формат, не поведение.
#
# v46 re-pin: S5c core shadow authority. This one is behavioral, not just
# format, exactly as the audit anticipated ("anchor80 will fall; this is
# the expected re-pin of the epoch"). The core's live gate used to be
# clamp(episode_count / 2500, 0, 0.65) -- nonzero and rising through this
# very life, reaching ~0.032 by episode 80. It is now 0.0 until this
# island's earned-authority ledger shows CORE_SHADOW_WIN_K=24 net wins
# over a window of CORE_SHADOW_WINDOW=400 disagreement receipts, which 80
# episodes on a freshly-initialized core cannot reach (measured: after
# 1200 episodes on this same seed, window_wins_minus_losses=-384, still
# unearned -- see --core-status). Both netta.state and netta.history.tsv
# move this time, because the decisions themselves moved, not only the
# format. Reproduced identically on two independent local builds.
if want 14; then
    d=$(world anchor80)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 42 --steps 80 ) >/dev/null 2>&1
    sh=$(shasum -a 256 "$d/netta.state" 2>/dev/null | cut -d' ' -f1)
    lh=$(shasum -a 256 "$d/netta.history.tsv" 2>/dev/null | cut -d' ' -f1)
    label="14 anchor80: a canonical 80-episode life stays bit-for-bit reproducible"
    if [ -z "$sh" ] || [ -z "$lh" ]; then
        fail "$label" "no measurement taken: run produced no state/ledger"
    elif [ "$sh" = "32b54fc60b1a326494d1163f4496e1f211d5ee6ca0846a490e6fd3ac11b925cb" ] && \
         [ "$lh" = "17a9fb61c303264bca013a85eaa59f8f5777ad4525c60b75d319a1d5ef9f41a3" ]; then
        pass "$label"
    else
        fail "$label" "state=$(printf '%.12s' "$sh") history=$(printf '%.12s' "$lh")"
    fi
fi

# --- S4 held-out exam: proper-scoring reproducibility pin -------------------
# docs/HELDOUT.md is the first ~3000 bytes of Alice's Adventures in
# Wonderland (public domain, Project Gutenberg) -- a text this organism has
# never read. Its content-hash is checked against every loaded island's own
# token_hash at exam start and refused if it matches; it does not, because
# it was never one of her islands. exam-seed 424242, newborn on netta.txt
# (--steps 0, same convention as probes 11-13). Combined and source read
# identical here on purpose: a newborn's combined geometry equals her
# island geometry exactly (residual is zero at birth, S3) -- the pin exists
# so a future divergence at birth is caught, not assumed away.
#
# Re-pin (S5c examiner v2, Sol's audit P0-3): --exam-n dropped, so this now
# runs the census default -- all 611 admissible HELDOUT.md positions exactly
# once, not 256 draws with replacement (the old sample held only 210 unique
# positions at seed 424242; census needs no such disclosure). The screening
# formula itself is untouched; only the position set moved from a biased
# with-replacement draw to the whole fixture.
#
# Checked, not moved, at S5c core shadow authority: same reasoning as
# probes 11/12 -- a newborn plays 0 episodes, and island_core_gate starts
# at 0.0 exactly where the old age-based clamp also read 0 at birth.
if want 15; then
    d=$(world exam_newborn)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 424242 --steps 0 \
        --exam "$ROOT/docs/HELDOUT.md" --exam-seed 424242 ) \
        > "$d/exam" 2>&1
    cbpt=$(exam_field "$d/exam" combined bpt)
    cip=$(exam_field "$d/exam" combined in_pool)
    cesc=$(exam_field "$d/exam" combined escape)
    cft=$(exam_field "$d/exam" combined ft)
    sbpt=$(exam_field "$d/exam" source bpt)
    sip=$(exam_field "$d/exam" source in_pool)
    sesc=$(exam_field "$d/exam" source escape)
    sft=$(exam_field "$d/exam" source ft)
    label="15 held-out exam census pin (v2): bpt=13.0923 in_pool=0.1997 escape=0.1522 ft=0.0442 (611 positions, combined == source at birth)"
    if [ -z "$cbpt" ] || [ -z "$sbpt" ]; then
        fail "$label" "no measurement taken: exam produced nothing"
    elif [ "$cbpt" = "13.0923" ] && [ "$cip" = "0.1997" ] && \
         [ "$cesc" = "0.1522" ] && [ "$cft" = "0.0442" ] && \
         [ "$sbpt" = "13.0923" ] && [ "$sip" = "0.1997" ] && \
         [ "$sesc" = "0.1522" ] && [ "$sft" = "0.0442" ]; then
        pass "$label"
    else
        fail "$label" \
             "combined bpt=$cbpt in_pool=$cip escape=$cesc ft=$cft / source bpt=$sbpt in_pool=$sip escape=$sesc ft=$sft"
    fi
fi

# --- S4 held-out exam: observer invariance ----------------------------------
# Two invariants Sol's audit named for any read-only observer (P0-5's
# "observer consumes no RNG and changes neither state, history nor
# decisions", restated for S4 as "state before the exam equals state after,
# bit for bit"): a life played with --exam produces the same netta.state as
# the identical life played without it, and within one run, the state saved
# immediately before run_exam() and immediately after are byte-identical.
if want 16; then
    a=$(world exam_inv_noexam)
    ( cd "$a" && "$BIN" netta.txt --reset --seed 42 --steps 80 ) >/dev/null 2>&1
    sa=$(shasum -a 256 "$a/netta.state" 2>/dev/null | cut -d' ' -f1)

    b=$(world exam_inv_withexam)
    ( cd "$b" && "$BIN" netta.txt --reset --seed 42 --steps 80 \
        --exam "$ROOT/docs/HELDOUT.md" --exam-seed 424242 ) \
        >/dev/null 2>&1
    sb=$(shasum -a 256 "$b/netta.state" 2>/dev/null | cut -d' ' -f1)
    spre=$(shasum -a 256 "$b/netta.state.exam_pre" 2>/dev/null | cut -d' ' -f1)
    spost=$(shasum -a 256 "$b/netta.state.exam_post" 2>/dev/null | cut -d' ' -f1)

    label="16 observer invariance: with/without exam and pre/post-exam state are bit-identical"
    if [ -z "$sa" ] || [ -z "$sb" ] || [ -z "$spre" ] || [ -z "$spost" ]; then
        fail "$label" "no measurement taken: a state file is missing"
    elif [ "$sa" = "$sb" ] && [ "$sb" = "$spre" ] && [ "$spre" = "$spost" ]; then
        pass "$label ($(printf '%.12s' "$sa"))"
    else
        fail "$label" \
             "no-exam=$(printf '%.12s' "$sa") with-exam=$(printf '%.12s' "$sb") pre=$(printf '%.12s' "$spre") post=$(printf '%.12s' "$spost")"
    fi
fi

# --- geometry health -- RED until geometry repair (S3), run explicitly -----
# Not part of the default run: this probe states the invariant a *healthy*
# source geometry needs, and the present compound geometry fails every
# threshold on purpose. Wiring it into the default 0-failure gate would
# either hide the cone behind a passing suite or force a false PASS message;
# neither is honest. Run it on demand with `./probes.sh 13` and read the
# numbers -- they are the falsifier for whichever repair replaces the
# compound pass in S3.
if [ -n "$WANT" ] && want 13; then
    d=$(world geometry)
    ( cd "$d" && "$BIN" netta.txt --reset --seed 424242 --steps 0 \
        --geometry-probe 20000 ) > "$d/exam" 2>&1
    bgm=$(awk '/background cosine median:/{print $4}' "$d/exam")
    q10=$(awk '/background cosine q10:/{print $4}' "$d/exam")
    cf=$(awk '/background clamp fraction:/{print $4}' "$d/exam")
    cn=$(awk '/centroid norm:/{print $3}' "$d/exam")
    hs=$(awk '/hub share:/{print $3}' "$d/exam")
    ar=$(awk '/actual rank median:/{print $4}' "$d/exam")
    rr=$(awk '/random rank median:/{print $4}' "$d/exam")
    label="13 geometry health (healthy: bg median<0.35, centroid norm<0.3, hub share<0.1, actual rank<random rank)"
    detail="bg_median=$bgm bg_q10=$q10 clamp_fraction=$cf centroid=$cn hub=$hs actual_rank=$ar random_rank=$rr"
    if [ -z "$bgm" ] || [ -z "$cn" ] || [ -z "$hs" ] || \
       [ -z "$ar" ] || [ -z "$rr" ]; then
        fail "$label" "no measurement taken: exam produced nothing"
    elif awk -v v="$bgm" 'BEGIN{exit !(v<0.35)}' && \
         awk -v v="$cn" 'BEGIN{exit !(v<0.3)}' && \
         awk -v v="$hs" 'BEGIN{exit !(v<0.1)}' && \
         [ "$ar" -lt "$rr" ]; then
        pass "$label ($detail)"
    else
        fail "$label" "$detail"
    fi
fi

say ""
say "$((RAN-FAILED))/$RAN passed"
[ -n "${NETTA_PROBE_DIR:-}" ] || say "receipts under $WORK"
exit $FAILED
