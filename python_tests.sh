#!/bin/sh
# NETTA ZERO C/Python equivalence gates. Run from the repository root;
# optionally pass the Python executable to test (default: $PYTHON or python3).
set -u

PY_INPUT=${1:-${PYTHON:-python3}}
PY=$(command -v "$PY_INPUT") || {
    printf 'python_tests: cannot find %s\n' "$PY_INPUT" >&2
    exit 99
}
T=$(mktemp -d) || exit 99
trap 'rm -rf "$T"' EXIT
FAIL=0

say() { printf '%s %s\n' "$1" "$2"; }
gate() {
    if [ "$2" -eq "$3" ]; then
        say PASS "$1"
    else
        say FAIL "$1 (rc=$2 want $3)"
        FAIL=$((FAIL + 1))
    fi
}

cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm \
    -o "$T/netta-c" 2>"$T/build.err"
rc=$?
[ -s "$T/build.err" ] && rc=98
gate "PY0 canonical C build is strict and silent" "$rc" 0
C="$T/netta-c"

PYTHONPYCACHEPREFIX="$T/pycache" "$PY" -m py_compile netta.py \
    2>"$T/pycompile.err"
rc=$?
[ -s "$T/pycompile.err" ] && rc=98
gate "PY0 netta.py compiles under $($PY --version 2>&1)" "$rc" 0

i=0
: >"$T/p3.bytes"
while [ "$i" -lt 700 ]; do
    printf 'abcacb' >>"$T/p3.bytes"
    i=$((i + 1))
done

equal_run() {
    tag=$1
    shift
    "$C" "$T/p3.bytes" --state "$T/$tag.c.state" \
        --bio "$T/$tag.c.bio" "$@" \
        >"$T/$tag.c.out" 2>"$T/$tag.c.err"
    c_rc=$?
    "$PY" netta.py "$T/p3.bytes" --state "$T/$tag.py.state" \
        --bio "$T/$tag.py.bio" "$@" \
        >"$T/$tag.py.out" 2>"$T/$tag.py.err"
    p_rc=$?
    rc=0
    [ "$c_rc" -eq 0 ] && [ "$p_rc" -eq 0 ] || rc=98
    for surface in state bio out err; do
        cmp -s "$T/$tag.c.$surface" "$T/$tag.py.$surface" || rc=98
    done
    gate "$tag: state, biography, stdout and stderr equal" "$rc" 0
}

equal_failure() {
    tag=$1
    shift
    "$C" "$@" >"$T/$tag.c.out" 2>"$T/$tag.c.err"
    c_rc=$?
    "$PY" netta.py "$@" >"$T/$tag.py.out" 2>"$T/$tag.py.err"
    p_rc=$?
    rc=0
    [ "$c_rc" -eq 1 ] && [ "$p_rc" -eq 1 ] || rc=98
    cmp -s "$T/$tag.c.out" "$T/$tag.py.out" || rc=98
    cmp -s "$T/$tag.c.err" "$T/$tag.py.err" || rc=98
    gate "$tag: exit, stdout and stderr equal" "$rc" 0
}

equal_run "PY1 plain life" --reset --seed 5 --episodes 2 --steps 300
equal_run "PY2 active Hebb" --reset --seed 17 --episodes 1 --steps 100 \
    --core-hebb-v1
equal_run "PY3 jury" --reset --seed 17 --episodes 1 --steps 40 --jury
equal_run "PY4 strtoull edge" --reset --seed ' -1' --episodes 0

cp "$T/PY1 plain life.c.state" "$T/resume.c.state"
cp "$T/PY1 plain life.c.state" "$T/resume.py.state"
cp "$T/PY1 plain life.c.bio" "$T/resume.c.bio"
cp "$T/PY1 plain life.c.bio" "$T/resume.py.bio"
"$C" "$T/p3.bytes" --state "$T/resume.c.state" \
    --bio "$T/resume.c.bio" --seed 5 --episodes 1 --steps 60 \
    >"$T/resume.c.out" 2>"$T/resume.c.err"
c_rc=$?
"$PY" netta.py "$T/p3.bytes" --state "$T/resume.py.state" \
    --bio "$T/resume.py.bio" --seed 5 --episodes 1 --steps 60 \
    >"$T/resume.py.out" 2>"$T/resume.py.err"
p_rc=$?
rc=0
[ "$c_rc" -eq 0 ] && [ "$p_rc" -eq 0 ] || rc=98
for surface in state bio out err; do
    cmp -s "$T/resume.c.$surface" "$T/resume.py.$surface" || rc=98
done
gate "PY5 C state resumes identically in Python" "$rc" 0

equal_failure "PY6 underscore integer refused" "$T/p3.bytes" --reset \
    --island 1_0 --episodes 0 --state "$T/bad.state" --bio "$T/bad.bio"

printf 'NETTAZR0' >"$T/version-short.state"
equal_failure "PY7 truncated version verdict" "$T/p3.bytes" \
    --state "$T/version-short.state" --bio "$T/missing.bio" --episodes 0
printf 'NETTAZR0\024\000\000\000' >"$T/law-short.state"
equal_failure "PY8 truncated law verdict" "$T/p3.bytes" \
    --state "$T/law-short.state" --bio "$T/missing.bio" --episodes 0

mkdir "$T/state-directory"
equal_failure "PY9 state directory verdict" "$T/p3.bytes" \
    --state "$T/state-directory" --bio "$T/missing.bio" --episodes 0
mkdir "$T/island-directory"
equal_failure "PY10 island directory verdict" "$T/island-directory" --reset \
    --state "$T/id.state" --bio "$T/id.bio" --episodes 0

cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/fsize_exec.c \
    -o "$T/fsize-exec" 2>"$T/fsize-build.err"
rc=$?
[ -s "$T/fsize-build.err" ] && rc=98
gate "PY11 file-size instrument builds strict and silent" "$rc" 0
"$T/fsize-exec" 200000 ignore "$C" "$T/p3.bytes" --reset --seed 5 \
    --episodes 1 --steps 60 --state "$T/fsize.c.state" \
    --bio "$T/fsize.c.bio" >"$T/fsize.c.out" 2>"$T/fsize.c.err"
c_rc=$?
"$T/fsize-exec" 200000 ignore "$PY" "$PWD/netta.py" "$T/p3.bytes" \
    --reset --seed 5 --episodes 1 --steps 60 \
    --state "$T/fsize.py.state" --bio "$T/fsize.py.bio" \
    >"$T/fsize.py.out" 2>"$T/fsize.py.err"
p_rc=$?
rc=0
[ "$c_rc" -eq 1 ] && [ "$p_rc" -eq 1 ] || rc=98
cmp -s "$T/fsize.c.out" "$T/fsize.py.out" || rc=98
cmp -s "$T/fsize.c.err" "$T/fsize.py.err" || rc=98
[ "$(cat "$T/fsize.py.err")" = "netta: state write failed" ] || rc=98
gate "PY12 state publication fails by contract without traceback" "$rc" 0

ln -s "$T/PY1 plain life.c.bio" "$T/bio-link"
cp "$T/PY1 plain life.c.state" "$T/link.c.state"
cp "$T/PY1 plain life.c.state" "$T/link.py.state"
"$C" "$T/p3.bytes" --state "$T/link.c.state" --bio "$T/bio-link" \
    --episodes 0 >"$T/link.c.out" 2>"$T/link.c.err"
c_rc=$?
"$PY" netta.py "$T/p3.bytes" --state "$T/link.py.state" \
    --bio "$T/bio-link" --episodes 0 \
    >"$T/link.py.out" 2>"$T/link.py.err"
p_rc=$?
rc=0
[ "$c_rc" -eq 0 ] && [ "$p_rc" -eq 0 ] || rc=98
cmp -s "$T/link.c.state" "$T/link.py.state" || rc=98
cmp -s "$T/link.c.out" "$T/link.py.out" || rc=98
cmp -s "$T/link.c.err" "$T/link.py.err" || rc=98
gate "PY13 regular biography symlink is read through one descriptor" "$rc" 0

mkfifo "$T/bio-fifo"
cp "$T/PY1 plain life.c.state" "$T/fifo.state"
equal_failure "PY14 biography FIFO refuses without blocking" "$T/p3.bytes" \
    --state "$T/fifo.state" --bio "$T/bio-fifo" --episodes 0

cc -O2 -std=c11 -Wall -Wextra -Wpedantic scripts/biography_fixture.c \
    -o "$T/biography-fixture" 2>"$T/fixture-build.err"
rc=$?
[ -s "$T/fixture-build.err" ] && rc=98
gate "PY15 public biography fixture builds strict and silent" "$rc" 0
corpus_rc=0
corpus_total=0
for corpus in scripts/biography_corpus/*_malformed.rows; do
    while IFS= read -r row || [ -n "$row" ]; do
        corpus_total=$((corpus_total + 1))
        cp "$T/PY1 plain life.c.state" "$T/corpus.input.state"
        cp "$T/PY1 plain life.c.bio" "$T/corpus.input.bio"
        printf '%s\n' "$row" >>"$T/corpus.input.bio"
        "$T/biography-fixture" "$T/corpus.input.state" \
            "$T/corpus.input.bio" || corpus_rc=98

        cp "$T/corpus.input.state" "$T/corpus.state"
        cp "$T/corpus.input.bio" "$T/corpus.bio"
        "$C" "$T/p3.bytes" --state "$T/corpus.state" \
            --bio "$T/corpus.bio" --episodes 0 \
            >"$T/corpus.c.out" 2>"$T/corpus.c.err"
        c_rc=$?

        cp "$T/corpus.input.state" "$T/corpus.state"
        cp "$T/corpus.input.bio" "$T/corpus.bio"
        "$PY" netta.py "$T/p3.bytes" --state "$T/corpus.state" \
            --bio "$T/corpus.bio" --episodes 0 \
            >"$T/corpus.py.out" 2>"$T/corpus.py.err"
        p_rc=$?

        [ "$c_rc" -eq 1 ] && [ "$p_rc" -eq 1 ] || corpus_rc=98
        cmp -s "$T/corpus.c.out" "$T/corpus.py.out" || corpus_rc=98
        cmp -s "$T/corpus.c.err" "$T/corpus.py.err" || corpus_rc=98
    done <"$corpus"
done
[ "$corpus_total" -eq 137 ] || corpus_rc=98
gate "PY16 all $corpus_total malformed rows receive one verdict" \
    "$corpus_rc" 0

echo "----"
if [ "$FAIL" -eq 0 ]; then
    echo "ALL PYTHON GATES PASS"
    exit 0
fi
echo "$FAIL PYTHON GATE(S) FAILED"
exit 1
