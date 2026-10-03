#!/usr/bin/env python3
"""G1 property harness for SITTING 2 (speech_court/SITTING2_PREREG.md).

The frozen question: does ranking the mouth's candidates by the exact cross
product instead of by a difference of logarithms change any decision outside a
mathematically equal class?

Three comparators per ordered candidate pair (a, b), all fed the same doubles
the organisms feed them:

  truth  exact rational comparison of w_a/(2+f_a) against w_b/(2+f_b), done in
         Python integers from the exact binary value of each double.  No
         rounding anywhere: this is the mathematical answer, the reference.
  exact  what netta_mouth.c and netta.py rank_cmp compute now --
         w_a*(2+f_b) vs w_b*(2+f_a) in IEEE-754 double.
  log    what they computed before -- log(w+1e-300) - log(1+freq/2), compared.

Sign convention: +1 means a ranks above b; 0 means the comparator ties them and
the lower unit id decides.  The tie-break itself is identical under both laws,
so only the primary comparison is measured here.

The output is the full 3x3x3 cross-tabulation of (log, exact, truth).  Every
gate question is answered from that table rather than from a single headline
number.  --law log reruns the class-split detector with the OLD comparator as
the law under test: that is the G5a red polarity, and it must find splits.
"""

import argparse
import math
import random
import sys
from collections import defaultdict
from fractions import Fraction

import numpy as np

REP_PENALTY = 0.5
REP_WINDOW = 12
CNT_GRID = 64
RANDOM_CNT_MAX = 65536
EXAMPLES = 6


def read_factors(path):
    """{1.0} union {1.0 + L} over the distinct L in the sealed citizens book."""
    seen = {1.0}
    with open(path) as f:
        next(f)
        for line in f:
            col = line.rstrip("\n").split("\t")
            if len(col) >= 21 and col[20] != "":
                seen.add(1.0 + float(col[20]))
    return sorted(seen)


def old_score(w, freq):
    """Bit-for-bit the pre-change expression (netta_mouth.c:902, netta.py)."""
    return math.log(w + 1e-300) - math.log(1.0 + REP_PENALTY * freq)


def exact_q(w, k):
    """The exact rational w/k.  Fraction(float) is exact, not a decimal guess."""
    num, den = float(w).as_integer_ratio()
    return Fraction(num, den * k)


def truth_ranks(ws, ks):
    """Dense rank by the exact rational w/(2+freq); equal value -> equal rank.

    Ranking on the Fraction itself, not on the float quotient: two states can
    share a rounded quotient while their true values differ, and two with equal
    true values must land in one class however the float lands.
    """
    qs = [exact_q(w, k) for w, k in zip(ws, ks)]
    order = sorted(range(len(qs)), key=lambda i: qs[i])
    rank = [0] * len(qs)
    groups = []
    r = -1
    prev = None
    for i in order:
        if prev is None or qs[i] != qs[prev]:
            r += 1
            groups.append([])
        rank[i] = r
        groups[r].append(i)
        prev = i
    return rank, groups


def build_grid(factors):
    ws, freqs = [], []
    for factor in factors:
        for cnt in range(1, CNT_GRID + 1):
            w = float(cnt) * factor
            for freq in range(0, REP_WINDOW + 1):
                ws.append(w)
                freqs.append(freq)
    return ws, freqs


def sweep(ws, freqs, rank, chunk_rows=512):
    """Full ordered-pair cross-tabulation of (log, exact, truth) signs."""
    n = len(ws)
    w = np.asarray(ws, dtype=np.float64)
    k = np.asarray([2 + f for f in freqs], dtype=np.float64)
    s = np.asarray([old_score(ws[i], freqs[i]) for i in range(n)], dtype=np.float64)
    rk = np.asarray(rank, dtype=np.int64)

    table = np.zeros((3, 3, 3), dtype=np.int64)
    found = defaultdict(list)
    for lo in range(0, n, chunk_rows):
        hi = min(lo + chunk_rows, n)
        wa = w[lo:hi, None]
        ka = k[lo:hi, None]
        pa = wa * k[None, :]
        pb = w[None, :] * ka
        ex = (pa > pb).astype(np.int8) - (pa < pb).astype(np.int8)
        sa = s[lo:hi, None]
        lg = (sa > s[None, :]).astype(np.int8) - (sa < s[None, :]).astype(np.int8)
        ra = rk[lo:hi, None]
        tr = (ra > rk[None, :]).astype(np.int8) - (ra < rk[None, :]).astype(np.int8)

        diag = np.zeros(ex.shape, dtype=bool)
        idx = np.arange(lo, hi)
        diag[np.arange(hi - lo), idx] = True
        keep = ~diag

        li = (lg + 1).astype(np.int64)
        ei = (ex + 1).astype(np.int64)
        ti = (tr + 1).astype(np.int64)
        flat = (li * 9 + ei * 3 + ti)[keep]
        counts = np.bincount(flat, minlength=27)
        table += counts.reshape(3, 3, 3)

        for name, mask in (
            ("E_exact_ties_log_splits", (ex == 0) & (lg != 0)),
            ("R_log_ties_exact_splits", (ex != 0) & (lg == 0)),
            ("X_opposite_strict", (ex != 0) & (lg != 0) & (ex != lg)),
            ("collapse_exact_ties_truth_strict", (tr != 0) & (ex == 0)),
            ("inversion_exact_vs_truth", (tr != 0) & (ex != 0) & (ex != tr)),
            ("phantom_exact_strict_truth_ties", (tr == 0) & (ex != 0)),
            ("log_splits_true_tie", (tr == 0) & (lg != 0)),
            ("log_inverts_true_order", (tr != 0) & (lg != 0) & (lg != tr)),
            ("log_merges_true_order", (tr != 0) & (lg == 0)),
        ):
            if len(found[name]) >= EXAMPLES:
                continue
            rr, cc = np.nonzero(mask & keep)
            for t in range(min(len(rr), EXAMPLES - len(found[name]))):
                i, j = lo + int(rr[t]), int(cc[t])
                found[name].append((i, j))
    return table, found


def class_report(ws, freqs, groups, law):
    """Does the law under test split a mathematically equal class?"""
    s = [old_score(ws[i], freqs[i]) for i in range(len(ws))]
    multi = [g for g in groups if len(g) >= 2]
    split, split_examples = 0, []
    for g in multi:
        broke = None
        for x in range(len(g)):
            for y in range(x + 1, len(g)):
                i, j = g[x], g[y]
                if law == "exact":
                    pa = ws[i] * (2 + freqs[j])
                    pb = ws[j] * (2 + freqs[i])
                    tied = pa == pb
                else:
                    tied = s[i] == s[j]
                if not tied:
                    broke = (i, j)
                    break
            if broke:
                break
        if broke:
            split += 1
            if len(split_examples) < EXAMPLES:
                split_examples.append(broke)
    return len(groups), len(multi), split, split_examples


def describe(ws, freqs, i):
    return "w=%.17g freq=%d" % (ws[i], freqs[i])


def random_pairs(factors, n_pairs, seed):
    """n_pairs independent random pairs with cnt up to RANDOM_CNT_MAX."""
    rng = random.Random(seed)
    wcache, lcache = {}, {}
    logf = [math.log(1.0 + REP_PENALTY * f) for f in range(REP_WINDOW + 1)]
    table = np.zeros((3, 3, 3), dtype=np.int64)
    found = defaultdict(list)

    def state():
        cnt = rng.randint(1, RANDOM_CNT_MAX)
        factor = factors[rng.randrange(len(factors))]
        freq = rng.randrange(REP_WINDOW + 1)
        key = (cnt, factor)
        w = wcache.get(key)
        if w is None:
            w = float(cnt) * factor
            wcache[key] = w
            lcache[key] = math.log(w + 1e-300)
        return w, freq, lcache[key], w.as_integer_ratio()

    for _ in range(n_pairs):
        wa, fa, lwa, (na, da) = state()
        wb, fb, lwb, (nb, db) = state()
        ka, kb = 2 + fa, 2 + fb
        sa = lwa - logf[fa]
        sb = lwb - logf[fb]
        lg = (sa > sb) - (sa < sb)
        pa = wa * kb
        pb = wb * ka
        ex = (pa > pb) - (pa < pb)
        left = na * db * kb
        right = nb * da * ka
        tr = (left > right) - (left < right)
        table[lg + 1, ex + 1, tr + 1] += 1
        if lg != ex or ex != tr:
            for name, hit in (
                ("E_exact_ties_log_splits", ex == 0 and lg != 0),
                ("R_log_ties_exact_splits", ex != 0 and lg == 0),
                ("X_opposite_strict", ex != 0 and lg != 0 and ex != lg),
                ("collapse_exact_ties_truth_strict", tr != 0 and ex == 0),
                ("inversion_exact_vs_truth", tr != 0 and ex != 0 and ex != tr),
                ("phantom_exact_strict_truth_ties", tr == 0 and ex != 0),
                ("log_splits_true_tie", tr == 0 and lg != 0),
                ("log_inverts_true_order", tr != 0 and lg != 0 and lg != tr),
                ("log_merges_true_order", tr != 0 and lg == 0),
            ):
                if hit and len(found[name]) < EXAMPLES:
                    found[name].append(((wa, fa), (wb, fb)))
    return table, found


def dump_table(table, total_label):
    name = {0: "-1", 1: " 0", 2: "+1"}
    print("%s: %d ordered pairs" % (total_label, int(table.sum())))
    print("  log exact truth        count")
    for a in range(3):
        for b in range(3):
            for c in range(3):
                v = int(table[a, b, c])
                if v:
                    print("  %s   %s    %s   %14d" % (name[a], name[b], name[c], v))


def derived(table):
    out = {}
    idx = range(3)
    def total(pred):
        return int(sum(table[a, b, c] for a in idx for b in idx for c in idx
                       if pred(a - 1, b - 1, c - 1)))
    out["E_exact_ties_log_splits"] = total(lambda l, e, t: e == 0 and l != 0)
    out["R_log_ties_exact_splits"] = total(lambda l, e, t: e != 0 and l == 0)
    out["X_opposite_strict"] = total(lambda l, e, t: e != 0 and l != 0 and e != l)
    out["collapse_exact_ties_truth_strict"] = total(lambda l, e, t: t != 0 and e == 0)
    out["inversion_exact_vs_truth"] = total(lambda l, e, t: t != 0 and e != 0 and e != t)
    out["phantom_exact_strict_truth_ties"] = total(lambda l, e, t: t == 0 and e != 0)
    out["log_splits_true_tie"] = total(lambda l, e, t: t == 0 and l != 0)
    out["log_inverts_true_order"] = total(lambda l, e, t: t != 0 and l != 0 and l != t)
    out["log_merges_true_order"] = total(lambda l, e, t: t != 0 and l == 0)
    out["log_disagrees_with_exact"] = total(lambda l, e, t: l != e)
    out["exact_disagrees_with_truth"] = total(lambda l, e, t: e != t)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--relations", default="speech_court/court4_relations.tsv")
    ap.add_argument("--law", choices=("exact", "log"), default="exact",
                    help="law under test for the class-split detector; "
                         "'log' is the G5a red polarity")
    ap.add_argument("--random-pairs", type=int, default=2_000_000)
    ap.add_argument("--random-seed", type=int, default=20261003)
    args = ap.parse_args()

    factors = read_factors(args.relations)
    print("factors (%d): %s" % (len(factors), ", ".join("%.17g" % f for f in factors)))

    ws, freqs = build_grid(factors)
    ks = [2 + f for f in freqs]
    rank, groups = truth_ranks(ws, ks)
    print("grid states: %d (cnt 1..%d x freq 0..%d x %d factors)"
          % (len(ws), CNT_GRID, REP_WINDOW, len(factors)))

    table, found = sweep(ws, freqs, rank)
    dump_table(table, "GRID exhaustive")
    d = derived(table)
    for key in sorted(d):
        print("  %-36s %d" % (key, d[key]))
    for key in sorted(found):
        if found[key]:
            print("  example %s:" % key)
            for i, j in found[key]:
                print("    a[%s]  b[%s]" % (describe(ws, freqs, i), describe(ws, freqs, j)))

    ncls, nmulti, nsplit, examples = class_report(ws, freqs, groups, args.law)
    print("CLASSES law=%s: %d equality classes, %d with >=2 members, %d split"
          % (args.law, ncls, nmulti, nsplit))
    for i, j in examples:
        print("  split: a[%s] s=%.17g  b[%s] s=%.17g"
              % (describe(ws, freqs, i), old_score(ws[i], freqs[i]),
                 describe(ws, freqs, j), old_score(ws[j], freqs[j])))

    rtable, rfound = random_pairs(factors, args.random_pairs, args.random_seed)
    dump_table(rtable, "RANDOM cnt 1..%d" % RANDOM_CNT_MAX)
    rd = derived(rtable)
    for key in sorted(rd):
        print("  %-36s %d" % (key, rd[key]))
    for key in sorted(rfound):
        if rfound[key]:
            print("  example %s:" % key)
            for a, b in rfound[key]:
                print("    a[w=%.17g freq=%d]  b[w=%.17g freq=%d]"
                      % (a[0], a[1], b[0], b[1]))

    bad = (d["X_opposite_strict"] + d["inversion_exact_vs_truth"]
           + d["phantom_exact_strict_truth_ties"]
           + rd["X_opposite_strict"] + rd["inversion_exact_vs_truth"]
           + rd["phantom_exact_strict_truth_ties"])
    exact_law_splits = nsplit if args.law == "exact" else None
    print("VERDICT law=%s: order-inverting or phantom divergences = %d" % (args.law, bad))
    if args.law == "exact":
        print("VERDICT law=exact: equality classes split by the law under test = %d"
              % nsplit)
        ok = bad == 0 and exact_law_splits == 0
    else:
        print("VERDICT law=log (red polarity): the detector must find splits; found %d"
              % nsplit)
        ok = nsplit >= 1
    print("G1_RESULT=%s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
