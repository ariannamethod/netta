/* Second hand on G1: the same exhaustive census as g1_property.py, in C, with
   the exact reference built from __int128 integers instead of Python Fractions.
   Different language, different arithmetic, same question -- so a bug in the
   harness's classification cannot pass unnoticed by agreeing with itself.

   Every w is a double; w = m * 2^(e-53) with m a 53-bit integer, so comparing
   w_a*(2+f_b) against w_b*(2+f_a) in reals is comparing m_a*k_b*2^(e_a) against
   m_b*k_a*2^(e_b) in integers.  m < 2^53 and k <= 14 keep the products under
   2^57; the exponent spread over this grid is under 8 bits.  No rounding. */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REP_WINDOW 12
#define REP_PENALTY 0.5
#define CNT_GRID 64
#define MAX_FACTORS 64

static double fac[MAX_FACTORS];
static size_t nfac = 0;

static double *gw, *gs;
static int *gk;
static __int128 *gm;
static int *ge;
static size_t ng = 0;

static void die(const char *m) { fprintf(stderr, "g1_confirm: %s\n", m); exit(2); }

static void add_factor(double f) {
    for (size_t i = 0; i < nfac; i++) if (fac[i] == f) return;
    if (nfac == MAX_FACTORS) die("too many factors");
    fac[nfac++] = f;
}

static void read_factors(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) die("cannot open relations book");
    static char line[8192];
    if (!fgets(line, sizeof line, f)) die("empty relations book");
    add_factor(1.0);
    while (fgets(line, sizeof line, f)) {
        char *p = line;
        int col = 1;
        while (col < 21 && p) { p = strchr(p, '\t'); if (p) { p++; col++; } }
        if (!p) continue;
        add_factor(1.0 + strtod(p, NULL));
    }
    fclose(f);
}

/* exact sign of w_a*(2+f_b) - w_b*(2+f_a), in integers */
static int truth_cmp(size_t a, size_t b) {
    __int128 A = gm[a] * (__int128)gk[b];
    __int128 B = gm[b] * (__int128)gk[a];
    int ea = ge[a], eb = ge[b];
    if (ea > eb) {
        if (ea - eb > 60) die("exponent spread beyond the safe shift");
        A <<= (ea - eb);
    } else if (eb > ea) {
        if (eb - ea > 60) die("exponent spread beyond the safe shift");
        B <<= (eb - ea);
    }
    return (A > B) - (A < B);
}

static int cmp_q(const void *pa, const void *pb) {
    size_t a = *(const size_t *)pa, b = *(const size_t *)pb;
    /* order by w/k ascending: w_a*k_b vs w_b*k_a is the same comparison */
    int c = truth_cmp(a, b);
    if (c) return c;
    return (a > b) - (a < b);
}

int main(int argc, char **argv) {
    const char *rel = argc > 1 ? argv[1] : "speech_court/court4_relations.tsv";
    read_factors(rel);
    printf("factors (%zu):", nfac);
    for (size_t i = 0; i < nfac; i++) printf(" %.17g", fac[i]);
    printf("\n");

    ng = nfac * CNT_GRID * (REP_WINDOW + 1);
    gw = malloc(ng * sizeof *gw); gs = malloc(ng * sizeof *gs);
    gk = malloc(ng * sizeof *gk); gm = malloc(ng * sizeof *gm);
    ge = malloc(ng * sizeof *ge);
    if (!gw || !gs || !gk || !gm || !ge) die("oom");

    size_t n = 0;
    for (size_t fi = 0; fi < nfac; fi++)
        for (int cnt = 1; cnt <= CNT_GRID; cnt++) {
            double w = (double)cnt * fac[fi];
            for (int freq = 0; freq <= REP_WINDOW; freq++) {
                int e;
                double mant = frexp(w, &e);
                double scaled = mant * 9007199254740992.0; /* 2^53 */
                __int128 m = (__int128)scaled;
                if ((double)m != scaled) die("mantissa is not an integer at 2^53");
                gw[n] = w;
                gk[n] = 2 + freq;
                gm[n] = m;
                ge[n] = e;
                gs[n] = log(w + 1e-300) - log(1.0 + REP_PENALTY * (double)freq);
                n++;
            }
        }
    if (n != ng) die("grid size mismatch");
    printf("grid states: %zu (cnt 1..%d x freq 0..%d x %zu factors)\n",
           ng, CNT_GRID, REP_WINDOW, nfac);

    static long long tab[3][3][3];
    unsigned long long pairs = 0;
    for (size_t i = 0; i < ng; i++)
        for (size_t j = 0; j < ng; j++) {
            if (i == j) continue;
            double pa = gw[i] * (double)gk[j];
            double pb = gw[j] * (double)gk[i];
            int ex = (pa > pb) - (pa < pb);
            int lg = (gs[i] > gs[j]) - (gs[i] < gs[j]);
            int tr = truth_cmp(i, j);
            tab[lg + 1][ex + 1][tr + 1]++;
            pairs++;
        }
    printf("GRID exhaustive: %llu ordered pairs\n", pairs);
    printf("  log exact truth        count\n");
    static const char *nm[3] = {"-1", " 0", "+1"};
    for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) for (int c = 0; c < 3; c++)
        if (tab[a][b][c])
            printf("  %s   %s    %s   %14lld\n", nm[a], nm[b], nm[c], tab[a][b][c]);

    long long E = 0, R = 0, X = 0, coll = 0, inv = 0, ph = 0;
    long long lsplit = 0, linv = 0, lmerge = 0, ldis = 0, edis = 0;
    long long rightE = 0, wrongE = 0;
    for (int a = 0; a < 3; a++) for (int b = 0; b < 3; b++) for (int c = 0; c < 3; c++) {
        long long v = tab[a][b][c];
        if (!v) continue;
        int l = a - 1, e = b - 1, t = c - 1;
        if (e == 0 && l != 0) E += v;
        if (e != 0 && l == 0) R += v;
        if (e != 0 && l != 0 && e != l) X += v;
        if (t != 0 && e == 0) coll += v;
        if (t != 0 && e != 0 && e != t) inv += v;
        if (t == 0 && e != 0) ph += v;
        if (t == 0 && l != 0) lsplit += v;
        if (t != 0 && l != 0 && l != t) linv += v;
        if (t != 0 && l == 0) lmerge += v;
        if (l != e) ldis += v;
        if (e != t) edis += v;
        /* out-of-class divergences: does exact or log match the truth there? */
        if (l != e && e != 0) { if (e == t) rightE += v; else wrongE += v; }
    }
    printf("  E_exact_ties_log_splits              %lld\n", E);
    printf("  R_log_ties_exact_splits              %lld\n", R);
    printf("  X_opposite_strict                    %lld\n", X);
    printf("  collapse_exact_ties_truth_strict     %lld\n", coll);
    printf("  inversion_exact_vs_truth             %lld\n", inv);
    printf("  phantom_exact_strict_truth_ties      %lld\n", ph);
    printf("  log_splits_true_tie                  %lld\n", lsplit);
    printf("  log_inverts_true_order               %lld\n", linv);
    printf("  log_merges_true_order                %lld\n", lmerge);
    printf("  log_disagrees_with_exact             %lld\n", ldis);
    printf("  exact_disagrees_with_truth           %lld\n", edis);
    printf("  out_of_class_exact_matches_truth     %lld\n", rightE);
    printf("  out_of_class_exact_contradicts_truth %lld\n", wrongE);

    size_t *ord = malloc(ng * sizeof *ord);
    if (!ord) die("oom");
    for (size_t i = 0; i < ng; i++) ord[i] = i;
    qsort(ord, ng, sizeof *ord, cmp_q);
    size_t ncls = 0, nmulti = 0, split_exact = 0, split_log = 0;
    size_t i0 = 0;
    while (i0 < ng) {
        size_t i1 = i0 + 1;
        while (i1 < ng && truth_cmp(ord[i0], ord[i1]) == 0) i1++;
        ncls++;
        size_t sz = i1 - i0;
        if (sz >= 2) {
            nmulti++;
            int se = 0, sl = 0;
            for (size_t x = i0; x < i1 && !(se && sl); x++)
                for (size_t y = x + 1; y < i1; y++) {
                    size_t a = ord[x], b = ord[y];
                    double pa = gw[a] * (double)gk[b], pb = gw[b] * (double)gk[a];
                    if (pa != pb) se = 1;
                    if (gs[a] != gs[b]) sl = 1;
                    if (se && sl) break;
                }
            split_exact += (size_t)se;
            split_log += (size_t)sl;
        }
        i0 = i1;
    }
    printf("CLASSES: %zu equality classes, %zu with >=2 members; "
           "split by exact law = %zu, split by log law = %zu\n",
           ncls, nmulti, split_exact, split_log);
    free(ord);
    return 0;
}
