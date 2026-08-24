/* mapdiag.c v4 -- B-only conversion diagnostic. NOT a court, NOT
   transfer evidence, no thresholds, no winner declared here.
   B is the frozen recognition law:
     B[s][d] = -(JS(sorted right profiles) + JS(sorted left profiles)).
   Question under test: how does B become a map, and who spoiled the
   court-2 recognizer -- Sinkhorn or the one-to-one requirement.
   Four conversions, diagnostic peers (2^B is NOT pre-crowned; exp is
   a link-function choice and JS is not a log-likelihood):
     c1 argmax per matchable row (hard)
     c2 greedy one-to-one partial matching over live bytes (hard)
     c3 Sinkhorn(exp2(B)) as a pure converter, no R, no loop (soft)
     c4 row-softmax M_B(d|s) = 2^B / sum over live d' (soft)
   For soft maps: true-mass mean/median over matchable, with the
   chance line 1/K_h printed beside. For hard maps: hits over
   matchable rows, with the chance expectation rows/K_h beside.
   Structural null (frozen derangement, fixed once) runs beside the
   real pipe; null is judged against chance expectation, never
   against literal zero. Machine laws kept for B:
     L2 synthetic permuted destination: B self-max on every row
     L4 rho-reassignment of the destination side only permutes B.
   C11, stdlib only. */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK 1024
#define SINKHORN_IT 20
#define CIPHER_SEED 0xB170C5ull
#define NULL_SEED 0x0DDBA11ull
#define RHO_SEED 0xDEC0DEull
static const int REFINE_AT[] = {1, 2, 4, 8, 16, 32, 64, 128};
#define NH 8

static void die(const char *m) { fprintf(stderr, "mapdiag: %s\n", m); exit(1); }

static uint64_t rng_state;
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return rng_state = x;
}
static void fy_perm(uint8_t *p, uint64_t seed) {
    for (int i = 0; i < 256; i++) p[i] = (uint8_t)i;
    rng_state = seed;
    for (int i = 255; i >= 1; i--) {
        int j = (int)(rng_next() % (uint64_t)(i + 1));
        uint8_t t = p[i]; p[i] = p[j]; p[j] = t;
    }
}

static uint8_t *read_file(const char *p, size_t *n) {
    FILE *f = fopen(p, "rb");
    if (!f) die("open");
    fseek(f, 0, SEEK_END);
    *n = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *b = malloc(*n);
    if (!b || fread(b, 1, *n, f) != *n) die("read");
    fclose(f);
    return b;
}

static void trans_build(const uint8_t *b, size_t n, double R[256][256], double L[256][256]) {
    static uint64_t c2[256][256];
    memset(c2, 0, sizeof(c2));
    for (size_t i = 0; i + 1 < n; i++) c2[b[i]][b[i + 1]]++;
    for (int s = 0; s < 256; s++) {
        double rowN = 0, colN = 0;
        for (int d = 0; d < 256; d++) rowN += (double)c2[s][d];
        for (int d = 0; d < 256; d++) R[s][d] = ((double)c2[s][d] + 0.5) / (rowN + 128.0);
        for (int d = 0; d < 256; d++) colN += (double)c2[d][s];
        for (int d = 0; d < 256; d++) L[s][d] = ((double)c2[d][s] + 0.5) / (colN + 128.0);
    }
}

static int cmp_desc(const void *x, const void *y) {
    double a = *(const double *)x, b = *(const double *)y;
    return (a < b) - (a > b);
}
static double js_bits(const double *p, const double *q) {
    double js = 0;
    for (int i = 0; i < 256; i++) {
        double m = 0.5 * (p[i] + q[i]);
        if (p[i] > 0) js += 0.5 * p[i] * log2(p[i] / m);
        if (q[i] > 0) js += 0.5 * q[i] * log2(q[i] / m);
    }
    return js;
}

static uint64_t s1cnt[256], s1tot;
static double srcA2[256][256], srcA2L[256][256];
static double RA[256][256], LA[256][256];

static void build_B(double B[256][256], double B2[256][256], double B2L[256][256]) {
    static double RD[256][256], LD[256][256];
    for (int d = 0; d < 256; d++) {
        memcpy(RD[d], B2[d], sizeof(RD[d]));
        memcpy(LD[d], B2L[d], sizeof(LD[d]));
        qsort(RD[d], 256, sizeof(double), cmp_desc);
        qsort(LD[d], 256, sizeof(double), cmp_desc);
    }
    for (int s = 0; s < 256; s++)
        for (int d = 0; d < 256; d++)
            B[s][d] = -(js_bits(RA[s], RD[d]) + js_bits(LA[s], LD[d]));
}

static void sinkhorn(double M[256][256], const int *rmask, const int *cmask) {
    for (int it = 0; it < SINKHORN_IT; it++) {
        for (int s = 0; s < 256; s++) {
            if (!rmask[s]) continue;
            double r = 0;
            for (int d = 0; d < 256; d++) if (cmask[d]) r += M[s][d];
            if (r > 0) for (int d = 0; d < 256; d++) if (cmask[d]) M[s][d] /= r;
        }
        for (int d = 0; d < 256; d++) {
            if (!cmask[d]) continue;
            double r = 0;
            for (int s = 0; s < 256; s++) if (rmask[s]) r += M[s][d];
            if (r > 0) for (int s = 0; s < 256; s++) if (rmask[s]) M[s][d] /= r;
        }
    }
}

static int hard_hits_argmax(double B[256][256], const uint8_t *pi,
                            const int *matchable, const int *alive_dst) {
    int hits = 0;
    for (int s = 0; s < 256; s++) {
        if (!matchable[s]) continue;
        int bd = -1;
        double bv = -1e300;
        for (int d = 0; d < 256; d++) {
            if (!alive_dst[d]) continue;
            if (B[s][d] > bv) { bv = B[s][d]; bd = d; }
        }
        if (bd == pi[s]) hits++;
    }
    return hits;
}

static int hard_hits_greedy11(double B[256][256], const uint8_t *pi,
                              const int *matchable, const int *alive_src, const int *alive_dst) {
    int used_s[256] = {0}, used_d[256] = {0}, hits = 0;
    for (;;) {
        int bs = -1, bd = -1;
        double bv = -1e300;
        for (int s = 0; s < 256; s++) {
            if (!alive_src[s] || used_s[s]) continue;
            for (int d = 0; d < 256; d++) {
                if (!alive_dst[d] || used_d[d]) continue;
                if (B[s][d] > bv) { bv = B[s][d]; bs = s; bd = d; }
            }
        }
        if (bs < 0) break;
        used_s[bs] = 1; used_d[bd] = 1;
        if (matchable[bs] && bd == pi[bs]) hits++;
    }
    return hits;
}

static void soft_stats(double M[256][256], const uint8_t *pi,
                       const int *matchable, const int *alive_dst,
                       int *out_hits, double *out_mean, double *out_med) {
    int hits = 0, n = 0;
    double masses[256];
    for (int s = 0; s < 256; s++) {
        if (!matchable[s]) continue;
        double rsum = 0, bv = -1e300;
        int bd = -1;
        for (int d = 0; d < 256; d++) {
            if (!alive_dst[d]) continue;
            rsum += M[s][d];
            if (M[s][d] > bv) { bv = M[s][d]; bd = d; }
        }
        if (bd == pi[s]) hits++;
        masses[n++] = rsum > 0 ? M[s][pi[s]] / rsum : 0;
    }
    double mean = 0;
    for (int i = 0; i < n; i++) mean += masses[i];
    mean = n ? mean / n : 0;
    for (int i = 0; i < n; i++)
        for (int j = i + 1; j < n; j++)
            if (masses[j] < masses[i]) { double t = masses[i]; masses[i] = masses[j]; masses[j] = t; }
    *out_hits = hits;
    *out_mean = mean;
    *out_med = n ? (n % 2 ? masses[n / 2] : (masses[n / 2 - 1] + masses[n / 2]) / 2.0) : 0;
}

static void run_conversions(const char *tag, double B[256][256], const uint8_t *pi,
                            const int *matchable, int nmatch,
                            const int *alive_src, const int *alive_dst, int K,
                            FILE *out, int horizon, size_t lived) {
    double chance_rows = K > 0 ? (double)nmatch / (double)K : 0;
    double chance_mass = K > 0 ? 1.0 / (double)K : 0;
    int h1 = hard_hits_argmax(B, pi, matchable, alive_dst);
    int h2 = hard_hits_greedy11(B, pi, matchable, alive_src, alive_dst);
    static double M3[256][256], M4[256][256];
    for (int s = 0; s < 256; s++)
        for (int d = 0; d < 256; d++) {
            double v = alive_src[s] && alive_dst[d] ? exp2(B[s][d]) : 0;
            M3[s][d] = v;
            M4[s][d] = v;
        }
    sinkhorn(M3, alive_src, alive_dst);
    int h3, h4;
    double m3mean, m3med, m4mean, m4med;
    soft_stats(M3, pi, matchable, alive_dst, &h3, &m3mean, &m3med);
    soft_stats(M4, pi, matchable, alive_dst, &h4, &m4mean, &m4med);
    fprintf(stderr,
        "  %s: c1 argmax %d | c2 one2one %d | c3 sinkhorn %d (mass %.4f/%.4f) | c4 softmax %d (mass %.4f/%.4f) || chance @1 %.2f mass %.4f (rows %d, K %d)\n",
        tag, h1, h2, h3, m3mean, m3med, h4, m4mean, m4med, chance_rows, chance_mass, nmatch, K);
    fprintf(out, "%d\t%zu\t%s\tc1_argmax\t%d\t-\t-\t%.4f\t%d\t%d\n", horizon, lived, tag, h1, chance_rows, nmatch, K);
    fprintf(out, "%d\t%zu\t%s\tc2_one2one\t%d\t-\t-\t%.4f\t%d\t%d\n", horizon, lived, tag, h2, chance_rows, nmatch, K);
    fprintf(out, "%d\t%zu\t%s\tc3_sinkhorn\t%d\t%.6f\t%.6f\t%.6f\t%d\t%d\n", horizon, lived, tag, h3, m3mean, m3med, chance_mass, nmatch, K);
    fprintf(out, "%d\t%zu\t%s\tc4_softmax\t%d\t%.6f\t%.6f\t%.6f\t%d\t%d\n", horizon, lived, tag, h4, m4mean, m4med, chance_mass, nmatch, K);
}

int main(int argc, char **argv) {
    if (argc < 3) die("usage: mapdiag <sourceA> <baseD>");
    size_t an, dn;
    uint8_t *A = read_file(argv[1], &an);
    uint8_t *D = read_file(argv[2], &dn);
    size_t atrain = (an * 9) / 10;

    memset(s1cnt, 0, sizeof(s1cnt));
    for (size_t i = 0; i < atrain; i++) s1cnt[A[i]]++;
    s1tot = atrain;
    trans_build(A, atrain, srcA2, srcA2L);
    for (int s = 0; s < 256; s++) {
        memcpy(RA[s], srcA2[s], sizeof(RA[s]));
        memcpy(LA[s], srcA2L[s], sizeof(LA[s]));
        qsort(RA[s], 256, sizeof(double), cmp_desc);
        qsort(LA[s], 256, sizeof(double), cmp_desc);
    }
    int alive_src[256], n_alive_src = 0;
    for (int s = 0; s < 256; s++) { alive_src[s] = s1cnt[s] > 0; n_alive_src += alive_src[s]; }

    uint8_t pi[256], pinv[256];
    fy_perm(pi, CIPHER_SEED);
    for (int s = 0; s < 256; s++) pinv[pi[s]] = (uint8_t)s;
    uint8_t *W = malloc(dn);
    if (!W) die("oom");
    for (size_t i = 0; i < dn; i++) W[i] = pi[D[i]];

    uint8_t nullp[256];
    fy_perm(nullp, NULL_SEED);
    for (int i = 0; i < 256; i++)
        if (nullp[i] == i) {
            int j = (i + 1) & 255;
            uint8_t t = nullp[i]; nullp[i] = nullp[j]; nullp[j] = t;
        }

    fprintf(stderr, "== machine laws (B) ==\n");
    {
        static double B2s[256][256], B2Ls[256][256], Bs[256][256];
        for (int d = 0; d < 256; d++)
            for (int d2 = 0; d2 < 256; d2++) {
                B2s[d][d2] = srcA2[pinv[d]][pinv[d2]];
                B2Ls[d][d2] = srcA2L[pinv[d]][pinv[d2]];
            }
        build_B(Bs, B2s, B2Ls);
        int ok = 0;
        double worst = 0;
        for (int s = 0; s < 256; s++) {
            double self = Bs[s][pi[s]];
            if (fabs(self) > worst) worst = fabs(self);
            int is_max = 1;
            for (int d = 0; d < 256; d++)
                if (d != pi[s] && Bs[s][d] > self) { is_max = 0; break; }
            ok += is_max;
        }
        fprintf(stderr, "L2 synthetic: worst |B self| %.3g, row-max %d/256 %s\n",
                worst, ok, (worst < 1e-12 && ok == 256) ? "PASS" : "CHECK");
    }

    FILE *out = fopen("transfer2/mapdiag.tsv", "wb");
    if (!out) die("open out");
    fprintf(out, "horizon\tlived\tpipe\tconversion\thits\tmass_mean\tmass_med\tchance\tnmatch\tK\n");

    for (int hi = 0; hi < NH; hi++) {
        size_t lived = (size_t)REFINE_AT[hi] * CHUNK;
        if (lived > dn) break;
        uint64_t dcnt[256] = {0};
        for (size_t i = 0; i < lived; i++) dcnt[W[i]]++;
        int alive_dst[256], K = 0;
        for (int d = 0; d < 256; d++) { alive_dst[d] = dcnt[d] > 0; K += alive_dst[d]; }
        int matchable[256], nmatch = 0;
        for (int s = 0; s < 256; s++) {
            matchable[s] = alive_src[s] && alive_dst[pi[s]];
            nmatch += matchable[s];
        }
        static double B2[256][256], B2L[256][256], Breal[256][256], Bnull[256][256];
        trans_build(W, lived, B2, B2L);
        build_B(Breal, B2, B2L);
        static double B2n[256][256], B2Ln[256][256];
        for (int d = 0; d < 256; d++) {
            memcpy(B2n[d], B2[nullp[d]], sizeof(B2n[d]));
            memcpy(B2Ln[d], B2L[nullp[d]], sizeof(B2Ln[d]));
        }
        build_B(Bnull, B2n, B2Ln);
        if (hi == 0) {
            uint8_t rho[256];
            fy_perm(rho, RHO_SEED);
            static double B2r[256][256], B2Lr[256][256], Br[256][256];
            for (int d = 0; d < 256; d++) {
                memcpy(B2r[d], B2[rho[d]], sizeof(B2r[d]));
                memcpy(B2Lr[d], B2L[rho[d]], sizeof(B2Lr[d]));
            }
            build_B(Br, B2r, B2Lr);
            double mx = 0;
            for (int s = 0; s < 256; s++)
                for (int d = 0; d < 256; d++) {
                    double diff = fabs(Br[s][d] - Breal[s][rho[d]]);
                    if (diff > mx) mx = diff;
                }
            fprintf(stderr, "L4 rho-equivariance of B: max_abs_diff %.3g %s\n",
                    mx, mx <= 1e-12 ? "PASS" : "CHECK");
        }
        fprintf(stderr, "h%d lived %zu | alive src %d dst %d matchable %d\n",
                hi, lived, n_alive_src, K, nmatch);
        run_conversions("real", Breal, pi, matchable, nmatch, alive_src, alive_dst, K, out, hi, lived);
        run_conversions("null", Bnull, pi, matchable, nmatch, alive_src, alive_dst, K, out, hi, lived);
    }
    fclose(out);
    free(A); free(D); free(W);
    return 0;
}
