/* transfer4.c -- NETTA transfer court 4 development builder under
   TRANSFER4_DRAFT.md. Not frozen and not a verdict. Builder hand only.
   Recognition: exact intersection of mutual-best B and mutual-best raw
   frequency F, with first-occurrence tie-breaking. Cargo: source
   probabilities measured only at learned-unit starts. Authority belongs
   to an exact RelationKey after 32 prospective bits; remaps never inherit.
   Arms: cold / exact-relation traveller / 19 structural nulls / oracle.
   C11, stdlib only. The builder grades nothing. */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MERGES 2048
#define MIN_PAIR 4
#define BASE_UNITS 256
#define MAX_UNITS (BASE_UNITS + MERGES)
#define PACK 21
#define PACK_MASK ((1u << PACK) - 1u)
#define EPSILON 0.1
#define CHUNK 1024
#define RUN_BYTES 131072
#define HALF_BOUNDARY 65536
#define AUX_BYTES 16384
#define AUX_SPLIT 8192
#define EARN_BITS 32.0
#define REVOKE_BITS 16.0
#define L_ENTER 0.05
#define L_LO 0.01
#define L_HI 0.5
#define ETA 0.05
#define CIPHER_SEED 0xB170C5ull
#define GHOST_SEED 0x5EED02ull
#define HALFTAIL_SEED 0x5EED03ull
#define AUXTAIL_SEED 0x5EED04ull
#define MAPLAW_SOURCE_SEED 0xB06726C4ull
#define MAPLAW_DEST_SEED 0x62FD57F1ull
#define NULL_K 19
#define REL_CAP 65536
#define REL_HASH (1u << 17)

static const uint64_t NULL_SEEDS[NULL_K] = {
    0x243f6a8885a308d3ull, 0x13198a2e03707344ull,
    0xa4093822299f31d0ull, 0x082efa98ec4e6c89ull,
    0x452821e638d01377ull, 0xbe5466cf34e90c6cull,
    0xc0ac29b7c97c50ddull, 0x3f84d5b5b5470917ull,
    0x9216d5d98979fb1bull, 0xd1310ba698dfb5acull,
    0x2ffd72dbd01adfb7ull, 0xb8e1afed6a267e96ull,
    0xba7c9045f12c7f99ull, 0x24a19947b3916cf7ull,
    0x0801f2e2858efc16ull, 0x636920d871574e69ull,
    0xa458fea3f4933d7eull, 0x0d95748f728eb658ull,
    0x718bcd5882154aeeull
};

static void die(const char *m) { fprintf(stderr, "transfer: %s\n", m); exit(1); }

static void check_null_seeds(void) {
    for (int i = 0; i < NULL_K; i++)
        for (int j = 0; j < i; j++)
            if (NULL_SEEDS[i] == NULL_SEEDS[j])
                die("structural null seeds are not distinct");
}

static uint64_t rng_state;
static uint64_t rng_step(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return *state = x;
}
static uint64_t rng_next(void) {
    return rng_step(&rng_state);
}
static void fy_perm(uint8_t *p, uint64_t seed) {
    for (int i = 0; i < 256; i++) p[i] = (uint8_t)i;
    rng_state = seed;
    for (int i = 255; i >= 1; i--) {
        int j = (int)(rng_next() % (uint64_t)(i + 1));
        uint8_t t = p[i]; p[i] = p[j]; p[j] = t;
    }
}

/* Source raw counts are used only by frequency recognizer F. */
static uint64_t s1cnt[256], s1tot;

static int cmp_u32(const void *x, const void *y) {
    uint32_t a = *(const uint32_t *)x, b = *(const uint32_t *)y;
    return (a > b) - (a < b);
}
static int cmp_u64(const void *x, const void *y) {
    uint64_t a = *(const uint64_t *)x, b = *(const uint64_t *)y;
    return (a > b) - (a < b);
}

static void cargo_build(const uint8_t *b, size_t n) {
    memset(s1cnt, 0, sizeof(s1cnt));
    for (size_t i = 0; i < n; i++) s1cnt[b[i]]++;
    s1tot = n;
}

static void range32(const uint32_t *k, size_t n, uint32_t pre, unsigned shift, size_t *lo, size_t *hi) {
    uint32_t lok = pre << shift, hik = (pre + 1) << shift;
    size_t a = 0, b2 = n;
    while (a < b2) { size_t m = a + (b2 - a) / 2; if (k[m] < lok) a = m + 1; else b2 = m; }
    *lo = a; size_t c = a, d = n;
    while (c < d) { size_t m = c + (d - c) / 2; if (k[m] < hik) c = m + 1; else d = m; }
    *hi = c;
}
static void range64k(const uint64_t *k, size_t n, uint64_t pre, unsigned shift, size_t *lo, size_t *hi) {
    uint64_t lok = pre << shift, hik = (pre + 1) << shift;
    size_t a = 0, b2 = n;
    while (a < b2) { size_t m = a + (b2 - a) / 2; if (k[m] < lok) a = m + 1; else b2 = m; }
    *lo = a; size_t c = a, d = n;
    while (c < d) { size_t m = c + (d - c) / 2; if (k[m] < hik) c = m + 1; else d = m; }
    *hi = c;
}

/* ───────── local model: units, R5 tie-break, R2 floor ───────── */
typedef struct {
    uint32_t merge_l[MERGES], merge_r[MERGES];
    uint32_t nmerges, nunits;
    uint8_t *pool; size_t pool_len, pool_cap;
    size_t exp_off[MAX_UNITS];
    uint32_t exp_len[MAX_UNITS];
    uint32_t *stream; size_t nstream;
    uint64_t *tri, *bi; size_t ntri, nbi;
    uint32_t n1[MAX_UNITS];
    uint64_t n1_total;
    double Z;
} Model;

static void mexp_append(Model *M, uint32_t id, const uint8_t *b, uint32_t len) {
    if (M->pool_len + len > M->pool_cap) {
        M->pool_cap = M->pool_cap ? M->pool_cap * 2 : (1u << 20);
        M->pool = realloc(M->pool, M->pool_cap);
        if (!M->pool) die("oom pool");
    }
    M->exp_off[id] = M->pool_len;
    M->exp_len[id] = len;
    memcpy(M->pool + M->pool_len, b, len);
    M->pool_len += len;
}

#define PH_BITS 19
#define PH_SIZE (1u << PH_BITS)
static uint64_t ph_key[PH_SIZE];
static uint32_t ph_cnt[PH_SIZE];
static size_t ph_first[PH_SIZE];
static uint32_t ph_used[PH_SIZE];
static uint32_t ph_used_n;

static int merge_round(Model *M) {
    ph_used_n = 0;
    uint32_t *t = M->stream;
    size_t n = M->nstream;
    for (size_t i = 0; i + 1 < n; i++) {
        uint64_t key = ((uint64_t)t[i] << 32) | t[i + 1];
        uint32_t h = (uint32_t)((key * 0x9E3779B97F4A7C15ull) >> (64 - PH_BITS));
        for (;;) {
            if (ph_cnt[h] == 0) {
                ph_key[h] = key; ph_cnt[h] = 1; ph_first[h] = i;
                ph_used[ph_used_n++] = h;
                break;
            }
            if (ph_key[h] == key) { ph_cnt[h]++; break; }
            h = (h + 1) & (PH_SIZE - 1);
        }
    }
    uint32_t bc = 0; size_t bf = (size_t)-1; uint64_t bk = UINT64_MAX;
    for (uint32_t u = 0; u < ph_used_n; u++) {
        uint32_t h = ph_used[u];
        if (ph_cnt[h] > bc ||
            (ph_cnt[h] == bc && (ph_first[h] < bf ||
             (ph_first[h] == bf && ph_key[h] < bk)))) {
            bc = ph_cnt[h]; bf = ph_first[h]; bk = ph_key[h];
        }
    }
    for (uint32_t u = 0; u < ph_used_n; u++) ph_cnt[ph_used[u]] = 0;
    if (bc < MIN_PAIR) return 0;
    uint32_t a = (uint32_t)(bk >> 32), b = (uint32_t)bk;
    uint32_t id = M->nunits++;
    M->merge_l[M->nmerges] = a; M->merge_r[M->nmerges] = b; M->nmerges++;
    {
        uint32_t la = M->exp_len[a], lb = M->exp_len[b];
        uint8_t *tmp = malloc((size_t)la + lb);
        if (!tmp) die("oom");
        memcpy(tmp, M->pool + M->exp_off[a], la);
        memcpy(tmp + la, M->pool + M->exp_off[b], lb);
        mexp_append(M, id, tmp, la + lb);
        free(tmp);
    }
    size_t w = 0;
    for (size_t i = 0; i < n; ) {
        if (i + 1 < n && t[i] == a && t[i + 1] == b) { t[w++] = id; i += 2; }
        else t[w++] = t[i++];
    }
    M->nstream = w;
    return 1;
}

static void model_build(Model *M, const uint8_t *bytes, size_t n) {
    free(M->stream); free(M->tri); free(M->bi); free(M->pool);
    memset(M, 0, sizeof(*M));
    M->stream = malloc((n ? n : 1) * sizeof(uint32_t));
    if (!M->stream) die("oom");
    M->nstream = n;
    for (size_t i = 0; i < n; i++) M->stream[i] = bytes[i];
    M->nunits = BASE_UNITS;
    for (uint32_t i = 0; i < BASE_UNITS; i++) { uint8_t bb = (uint8_t)i; mexp_append(M, i, &bb, 1); }
    if (n) while (M->nmerges < MERGES && merge_round(M)) {}
    size_t s = M->nstream;
    M->ntri = s >= 3 ? s - 2 : 0;
    M->nbi = s >= 2 ? s - 1 : 0;
    M->tri = malloc((M->ntri ? M->ntri : 1) * sizeof(uint64_t));
    M->bi = malloc((M->nbi ? M->nbi : 1) * sizeof(uint64_t));
    if (!M->tri || !M->bi) die("oom");
    for (size_t i = 0; i < M->ntri; i++)
        M->tri[i] = ((uint64_t)M->stream[i] << (2 * PACK)) |
                    ((uint64_t)M->stream[i + 1] << PACK) | M->stream[i + 2];
    for (size_t i = 0; i < M->nbi; i++)
        M->bi[i] = ((uint64_t)M->stream[i] << PACK) | M->stream[i + 1];
    qsort(M->tri, M->ntri, sizeof(uint64_t), cmp_u64);
    qsort(M->bi, M->nbi, sizeof(uint64_t), cmp_u64);
    memset(M->n1, 0, sizeof(M->n1));
    for (size_t i = 0; i < s; i++) M->n1[M->stream[i]]++;
    M->n1_total = s;
    M->Z = 0;
    for (uint32_t u = 0; u < M->nunits; u++) M->Z += pow(256.0, -(double)M->exp_len[u]);
}

static uint32_t *segment(const Model *M, const uint8_t *b, size_t n, size_t *out_n) {
    uint32_t *t = malloc((n ? n : 1) * sizeof(uint32_t));
    if (!t) die("oom");
    size_t tn = n;
    for (size_t i = 0; i < n; i++) t[i] = b[i];
    for (uint32_t m = 0; m < M->nmerges; m++) {
        uint32_t a = M->merge_l[m], bb = M->merge_r[m], id = BASE_UNITS + m;
        size_t w = 0;
        for (size_t i = 0; i < tn; ) {
            if (i + 1 < tn && t[i] == a && t[i + 1] == bb) { t[w++] = id; i += 2; }
            else t[w++] = t[i++];
        }
        tn = w;
    }
    *out_n = tn;
    return t;
}

static size_t token_coverage(const Model *M, const uint32_t *tokens, size_t n) {
    size_t bytes = 0;
    for (size_t i = 0; i < n; i++) {
        if (M->exp_len[tokens[i]] > SIZE_MAX - bytes)
            die("token coverage overflow");
        bytes += M->exp_len[tokens[i]];
    }
    return bytes;
}

typedef struct { uint32_t tok; uint32_t cnt; } CC;
#define MAX_CAND 65536
static CC ccbuf[MAX_CAND];
static size_t collect(const uint64_t *k, size_t lo, size_t hi) {
    size_t n = 0, i = lo;
    while (i < hi && n < MAX_CAND) {
        uint32_t tok = (uint32_t)(k[i] & PACK_MASK);
        size_t j = i;
        while (j < hi && (uint32_t)(k[j] & PACK_MASK) == tok) j++;
        ccbuf[n].tok = tok; ccbuf[n].cnt = (uint32_t)(j - i); n++;
        i = j;
    }
    return n;
}
static void krange(const uint64_t *k, size_t n, uint64_t pre, unsigned sh, size_t *lo, size_t *hi) {
    range64k(k, n, pre, sh, lo, hi);
}

static double floorU(const Model *M, uint32_t u) {
    return pow(256.0, -(double)M->exp_len[u]) / M->Z;
}

static double price_local(const Model *M, uint32_t c1, uint32_t c2, uint32_t truth) {
    double p1;
    if (M->n1_total > 0)
        p1 = (1.0 - EPSILON) * ((double)M->n1[truth] / (double)M->n1_total)
             + EPSILON * floorU(M, truth);
    else
        p1 = floorU(M, truth);
    if (c2 == UINT32_MAX) return p1;
    double p2 = p1;
    {
        size_t lo, hi;
        krange(M->bi, M->nbi, c2, PACK, &lo, &hi);
        if (hi > lo) {
            size_t nc = collect(M->bi, lo, hi);
            double sum = 0, wt = 0;
            for (size_t k = 0; k < nc; k++) { sum += ccbuf[k].cnt; if (ccbuf[k].tok == truth) wt = ccbuf[k].cnt; }
            p2 = (1.0 - EPSILON) * (wt / sum) + EPSILON * p1;
        }
    }
    if (c1 == UINT32_MAX) return p2;
    double p = p2;
    {
        uint64_t ctx = ((uint64_t)c1 << PACK) | c2;
        size_t lo, hi;
        krange(M->tri, M->ntri, ctx, PACK, &lo, &hi);
        if (hi > lo) {
            size_t nc = collect(M->tri, lo, hi);
            double sum = 0, wt = 0;
            for (size_t k = 0; k < nc; k++) { sum += ccbuf[k].cnt; if (ccbuf[k].tok == truth) wt = ccbuf[k].cnt; }
            p = (1.0 - EPSILON) * (wt / sum) + EPSILON * p2;
        }
    }
    return p;
}

/* Source cargo for the event judged by a relation specialist: the first
   byte at an actual learned-unit boundary, conditioned on 0..3 preceding
   raw bytes. */
static uint64_t us1cnt[256], us1tot;
static uint32_t *us2k, *us3k;
static uint64_t *us4k;
static size_t nus2, nus3, nus4;
static uint64_t source_first[256];

static void unit_start_build(const uint8_t *bytes, size_t n, const Model *S) {
    memset(us1cnt, 0, sizeof(us1cnt));
    us1tot = 0;
    us2k = malloc((S->nstream ? S->nstream : 1) * sizeof(uint32_t));
    us3k = malloc((S->nstream ? S->nstream : 1) * sizeof(uint32_t));
    us4k = malloc((S->nstream ? S->nstream : 1) * sizeof(uint64_t));
    if (!us2k || !us3k || !us4k) die("oom unit-start cargo");
    nus2 = nus3 = nus4 = 0;
    size_t bp = 0;
    for (size_t i = 0; i < S->nstream; i++) {
        uint32_t u = S->stream[i];
        if (bp >= n) die("unit-start offset overflow");
        uint8_t truth = bytes[bp];
        const uint8_t *exp = S->pool + S->exp_off[u];
        if (exp[0] != truth) die("unit-start segmentation mismatch");
        us1cnt[truth]++;
        us1tot++;
        if (bp >= 1)
            us2k[nus2++] = ((uint32_t)bytes[bp - 1] << 8) | truth;
        if (bp >= 2)
            us3k[nus3++] = ((uint32_t)bytes[bp - 2] << 16) |
                           ((uint32_t)bytes[bp - 1] << 8) | truth;
        if (bp >= 3)
            us4k[nus4++] = ((uint64_t)bytes[bp - 3] << 24) |
                           ((uint64_t)bytes[bp - 2] << 16) |
                           ((uint64_t)bytes[bp - 1] << 8) | truth;
        bp += S->exp_len[u];
    }
    if (bp != n) die("unit-start stream does not cover source");
    qsort(us2k, nus2, sizeof(uint32_t), cmp_u32);
    qsort(us3k, nus3, sizeof(uint32_t), cmp_u32);
    qsort(us4k, nus4, sizeof(uint64_t), cmp_u64);
}

static void unit_start_ps_vec(const uint8_t *ctx, int cl, double *out) {
    double v1[256];
    for (int i = 0; i < 256; i++)
        v1[i] = us1tot
                ? (1.0 - EPSILON) * ((double)us1cnt[i] / (double)us1tot) +
                  EPSILON / 256.0
                : 1.0 / 256.0;
    if (cl < 1) { memcpy(out, v1, sizeof(v1)); return; }
    double v2[256];
    {
        size_t lo, hi;
        range32(us2k, nus2, ctx[cl - 1], 8, &lo, &hi);
        if (hi > lo) {
            double total = (double)(hi - lo);
            for (int i = 0; i < 256; i++) v2[i] = EPSILON * v1[i];
            size_t at = lo;
            while (at < hi) {
                uint32_t d = us2k[at] & 0xFF;
                size_t end = at;
                while (end < hi && (us2k[end] & 0xFF) == d) end++;
                v2[d] += (1.0 - EPSILON) * ((double)(end - at) / total);
                at = end;
            }
        } else memcpy(v2, v1, sizeof(v2));
    }
    if (cl < 2) { memcpy(out, v2, sizeof(v2)); return; }
    double v3[256];
    {
        uint32_t pre = ((uint32_t)ctx[cl - 2] << 8) | ctx[cl - 1];
        size_t lo, hi;
        range32(us3k, nus3, pre, 8, &lo, &hi);
        if (hi > lo) {
            double total = (double)(hi - lo);
            for (int i = 0; i < 256; i++) v3[i] = EPSILON * v2[i];
            size_t at = lo;
            while (at < hi) {
                uint32_t d = us3k[at] & 0xFF;
                size_t end = at;
                while (end < hi && (us3k[end] & 0xFF) == d) end++;
                v3[d] += (1.0 - EPSILON) * ((double)(end - at) / total);
                at = end;
            }
        } else memcpy(v3, v2, sizeof(v3));
    }
    if (cl < 3) { memcpy(out, v3, sizeof(v3)); return; }
    {
        uint64_t pre = ((uint64_t)ctx[cl - 3] << 16) |
                       ((uint64_t)ctx[cl - 2] << 8) | ctx[cl - 1];
        size_t lo, hi;
        range64k(us4k, nus4, pre, 8, &lo, &hi);
        if (hi > lo) {
            double total = (double)(hi - lo);
            for (int i = 0; i < 256; i++) out[i] = EPSILON * v3[i];
            size_t at = lo;
            while (at < hi) {
                uint32_t d = (uint32_t)(us4k[at] & 0xFF);
                size_t end = at;
                while (end < hi && (uint32_t)(us4k[end] & 0xFF) == d) end++;
                out[d] += (1.0 - EPSILON) * ((double)(end - at) / total);
                at = end;
            }
        } else memcpy(out, v3, sizeof(v3));
    }
}

typedef struct {
    uint32_t unit[MAX_UNITS];
    uint32_t off[257];
} FirstUnits;

static void first_units_build(const Model *M, FirstUnits *F) {
    uint32_t count[256] = {0}, cursor[256];
    for (uint32_t u = 0; u < M->nunits; u++) {
        const uint8_t *b = M->pool + M->exp_off[u];
        count[b[0]]++;
    }
    F->off[0] = 0;
    for (int d = 0; d < 256; d++) F->off[d + 1] = F->off[d] + count[d];
    memcpy(cursor, F->off, sizeof(cursor));
    for (uint32_t u = 0; u < M->nunits; u++) {
        const uint8_t *b = M->pool + M->exp_off[u];
        F->unit[cursor[b[0]]++] = u;
    }
}

/* ───────── recognizer B ───────── */
static double srcA2[256][256], srcA2L[256][256];
static double RA[256][256], LAp[256][256];

static void trans_kt(const uint8_t *b, size_t n, double R[256][256], double L[256][256]) {
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
            B[s][d] = -(js_bits(RA[s], RD[d]) + js_bits(LAp[s], LD[d]));
}

static void frequency_score(const uint64_t dc[256], size_t n,
                            double F[256][256]) {
    for (int s = 0; s < 256; s++) {
        double ps = ((double)s1cnt[s] + 0.5) / ((double)s1tot + 128.0);
        for (int d = 0; d < 256; d++) {
            double pd = ((double)dc[d] + 0.5) / ((double)n + 128.0);
            F[s][d] = -fabs(log2(ps) - log2(pd));
        }
    }
}

static void mutual_map(double S[256][256], const int *alive_s,
                       const int *alive_d, const uint64_t *rank_s,
                       const uint64_t *rank_d, int16_t fwd[256],
                       int16_t inv[256], int *matched) {
    int row_best[256], col_best[256];
    for (int s = 0; s < 256; s++) {
        row_best[s] = -1;
        if (!alive_s[s]) continue;
        double best = -INFINITY;
        for (int d = 0; d < 256; d++) {
            if (!alive_d[d]) continue;
            if (S[s][d] > best ||
                (S[s][d] == best &&
                 (row_best[s] < 0 || rank_d[d] < rank_d[row_best[s]]))) {
                best = S[s][d];
                row_best[s] = d;
            }
        }
    }
    for (int d = 0; d < 256; d++) {
        col_best[d] = -1;
        if (!alive_d[d]) continue;
        double best = -INFINITY;
        for (int s = 0; s < 256; s++) {
            if (!alive_s[s]) continue;
            if (S[s][d] > best ||
                (S[s][d] == best &&
                 (col_best[d] < 0 || rank_s[s] < rank_s[col_best[d]]))) {
                best = S[s][d];
                col_best[d] = s;
            }
        }
    }
    for (int i = 0; i < 256; i++) fwd[i] = inv[i] = -1;
    *matched = 0;
    for (int s = 0; s < 256; s++) {
        int d = row_best[s];
        if (d >= 0 && col_best[d] == s) {
            fwd[s] = (int16_t)d;
            inv[d] = (int16_t)s;
            (*matched)++;
        }
    }
}

static void admission_map(double B[256][256], double F[256][256],
                          const int *alive_s, const int *alive_d,
                          const uint64_t *rank_s, const uint64_t *rank_d,
                          int16_t fwd[256], int16_t inv[256], int *matched,
                          int16_t bonly_fwd[256], int *bonly_matched) {
    int16_t bi[256], ff[256], fi[256];
    int fn;
    mutual_map(B, alive_s, alive_d, rank_s, rank_d,
               bonly_fwd, bi, bonly_matched);
    mutual_map(F, alive_s, alive_d, rank_s, rank_d, ff, fi, &fn);
    (void)bi;
    (void)fi;
    (void)fn;
    for (int i = 0; i < 256; i++) fwd[i] = inv[i] = -1;
    *matched = 0;
    for (int s = 0; s < 256; s++) {
        if (bonly_fwd[s] >= 0 && bonly_fwd[s] == ff[s]) {
            fwd[s] = bonly_fwd[s];
            inv[fwd[s]] = (int16_t)s;
            (*matched)++;
        }
    }
}

static void check_admission_equivariance(double B[256][256],
                                         double F[256][256],
                                         const int *alive_s,
                                         const int *alive_d,
                                         const uint64_t *rank_s,
                                         const uint64_t *rank_d,
                                         const int16_t real_fwd[256]) {
    static double Bp[256][256], Fp[256][256];
    uint8_t ps[256], pd[256];
    int16_t pf[256], pi[256], pb[256];
    int asp[256] = {0}, adp[256] = {0}, pn, pbn;
    uint64_t rsp[256], rdp[256];
    fy_perm(ps, MAPLAW_SOURCE_SEED);
    fy_perm(pd, MAPLAW_DEST_SEED);
    for (int s = 0; s < 256; s++) {
        asp[ps[s]] = alive_s[s];
        rsp[ps[s]] = rank_s[s];
    }
    for (int d = 0; d < 256; d++) {
        adp[pd[d]] = alive_d[d];
        rdp[pd[d]] = rank_d[d];
    }
    for (int s = 0; s < 256; s++)
        for (int d = 0; d < 256; d++) {
            Bp[ps[s]][pd[d]] = B[s][d];
            Fp[ps[s]][pd[d]] = F[s][d];
        }
    admission_map(Bp, Fp, asp, adp, rsp, rdp, pf, pi, &pn, pb, &pbn);
    (void)pi;
    (void)pn;
    (void)pbn;
    for (int s = 0; s < 256; s++) {
        int16_t got = pf[ps[s]];
        int16_t want = real_fwd[s] < 0 ? -1 : pd[real_fwd[s]];
        if (got != want) die("admission map is not permutation-equivariant");
    }
}

static uint64_t uniform_bounded(uint64_t *state, uint64_t bound) {
    uint64_t threshold = (uint64_t)(-bound) % bound;
    uint64_t x;
    do x = rng_step(state); while (x < threshold);
    return x % bound;
}

static int profile_scramble(const int *alive_d, const uint64_t *rank_d,
                            uint64_t seed, uint8_t rho[256]) {
    uint8_t order[256], perm[256];
    int n = 0;
    for (int d = 0; d < 256; d++)
        if (alive_d[d]) {
            int at = n;
            while (at > 0 && rank_d[order[at - 1]] > rank_d[d]) {
                order[at] = order[at - 1];
                at--;
            }
            order[at] = (uint8_t)d;
            n++;
        }
    for (int i = 0; i < n; i++) perm[i] = (uint8_t)i;
    uint64_t state = seed;
    for (int i = 1; i < n; i++) {
        int j = (int)uniform_bounded(&state, (uint64_t)(i + 1));
        uint8_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }
    memset(rho, 0xFF, 256);
    int seen[256] = {0};
    for (int i = 0; i < n; i++) {
        uint8_t profile_d = order[perm[i]];
        rho[order[i]] = profile_d;
        if (!alive_d[profile_d] || seen[profile_d])
            die("structural null is not a bijection on alive profiles");
        seen[profile_d] = 1;
    }
    return n;
}

static void structural_null_admission(double B[256][256],
                                      double F[256][256],
                                      const int *alive_s, const int *alive_d,
                                      const uint64_t *rank_s,
                                      const uint64_t *rank_d, uint64_t seed,
                                      uint8_t rho[256], int16_t fwd[256],
                                      int16_t inv[256], int *matched) {
    static double Bnull[256][256];
    int n = profile_scramble(alive_d, rank_d, seed, rho);
    for (int s = 0; s < 256; s++)
        for (int d = 0; d < 256; d++)
            Bnull[s][d] = alive_d[d] ? B[s][rho[d]] : B[s][d];
    int16_t bonly[256];
    int bonly_n;
    admission_map(Bnull, F, alive_s, alive_d, rank_s, rank_d,
                  fwd, inv, matched, bonly, &bonly_n);
    (void)bonly_n;
    if (n == 0 && *matched != 0)
        die("empty structural null produced an admitted pair");
}

static void oracle_map(const uint8_t *omap, const int16_t learned_fwd[256],
                       const int *alive_d, int16_t fwd[256],
                       int16_t inv[256], int *matched) {
    for (int i = 0; i < 256; i++) fwd[i] = inv[i] = -1;
    *matched = 0;
    for (int s = 0; s < 256; s++)
        if (learned_fwd[s] >= 0 && alive_d[omap[s]]) {
            fwd[s] = omap[s];
            inv[omap[s]] = (int16_t)s;
            (*matched)++;
        }
}

/* ───────── worlds ───────── */
static uint8_t cipher_pi[256];
static uint8_t *build_cipher(const uint8_t *src, size_t n) {
    uint8_t *w = malloc(n);
    if (!w) die("oom");
    for (size_t i = 0; i < n; i++) w[i] = cipher_pi[src[i]];
    return w;
}
static uint8_t *build_ghost_bytes(const uint8_t *base, size_t basen, size_t outn, uint64_t seed) {
    uint64_t cnt[256] = {0}, cum[256], tot = 0;
    for (size_t i = 0; i < basen; i++) cnt[base[i]]++;
    for (int i = 0; i < 256; i++) { tot += cnt[i]; cum[i] = tot; }
    uint8_t *w = malloc(outn);
    if (!w) die("oom");
    rng_state = seed;
    for (size_t i = 0; i < outn; i++) {
        uint64_t r = rng_next() % tot;
        int lo = 0, hi = 255;
        while (lo < hi) { int m = (lo + hi) / 2; if (cum[m] <= r) lo = m + 1; else hi = m; }
        w[i] = (uint8_t)lo;
    }
    return w;
}
static uint8_t ff_words[16][64]; static uint32_t ff_wlen[16];
static int is_ws(uint8_t c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }
static uint64_t bhash(const uint8_t *s, uint32_t len) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t i = 0; i < len; i++) h = (h ^ s[i]) * 0x100000001b3ull;
    return h;
}
static uint8_t *build_ff(const uint8_t *src, size_t n, size_t *out_n) {
    enum { WH = 1 << 16 };
    static struct { uint8_t s[64]; uint32_t len; uint64_t cnt; size_t first; int used; } wh[WH];
    memset(wh, 0, sizeof(wh));
    size_t i = 0;
    while (i < n) {
        if (is_ws(src[i])) { i++; continue; }
        size_t j = i;
        while (j < n && !is_ws(src[j])) j++;
        uint32_t len = (uint32_t)(j - i);
        if (len < 64) {
            uint32_t slot = (uint32_t)(bhash(src + i, len) >> (64 - 16));
            for (;;) {
                if (!wh[slot].used) {
                    memcpy(wh[slot].s, src + i, len);
                    wh[slot].len = len; wh[slot].cnt = 1; wh[slot].first = i; wh[slot].used = 1;
                    break;
                }
                if (wh[slot].len == len && !memcmp(wh[slot].s, src + i, len)) { wh[slot].cnt++; break; }
                slot = (slot + 1) & (WH - 1);
            }
        }
        i = j;
    }
    int picked[16];
    for (int k = 0; k < 16; k++) {
        int best = -1;
        for (int s = 0; s < WH; s++) {
            if (!wh[s].used) continue;
            int dup = 0;
            for (int q2 = 0; q2 < k; q2++) if (picked[q2] == s) dup = 1;
            if (dup) continue;
            if (best < 0 || wh[s].cnt > wh[best].cnt ||
                (wh[s].cnt == wh[best].cnt && wh[s].first < wh[best].first)) best = s;
        }
        if (best < 0) die("false-friend world has fewer than 16 words");
        picked[k] = best;
        memcpy(ff_words[k], wh[best].s, wh[best].len);
        ff_wlen[k] = wh[best].len;
    }
    size_t cap = 0;
    i = 0;
    while (i < n) {
        size_t add;
        if (is_ws(src[i])) {
            add = 1;
            i++;
        } else {
            size_t j = i;
            while (j < n && !is_ws(src[j])) j++;
            uint32_t len = (uint32_t)(j - i);
            int hit = -1;
            for (int k = 0; k < 16; k++)
                if (ff_wlen[k] == len && !memcmp(ff_words[k], src + i, len)) {
                    hit = k;
                    break;
                }
            add = hit < 0 ? len : ff_wlen[(hit % 2 == 0) ? hit + 1 : hit - 1];
            i = j;
        }
        if (add > SIZE_MAX - cap) die("false-friend output length overflow");
        cap += add;
    }
    uint8_t *w = malloc(cap ? cap : 1);
    if (!w) die("oom");
    size_t o = 0;
    i = 0;
    while (i < n) {
        if (is_ws(src[i])) { w[o++] = src[i++]; continue; }
        size_t j = i;
        while (j < n && !is_ws(src[j])) j++;
        uint32_t len = (uint32_t)(j - i);
        int hit = -1;
        for (int k = 0; k < 16; k++)
            if (ff_wlen[k] == len && !memcmp(ff_words[k], src + i, len)) { hit = k; break; }
        if (hit >= 0) {
            int partner = (hit % 2 == 0) ? hit + 1 : hit - 1;
            memcpy(w + o, ff_words[partner], ff_wlen[partner]);
            o += ff_wlen[partner];
        } else {
            memcpy(w + o, src + i, len);
            o += len;
        }
        i = j;
    }
    *out_n = o;
    return w;
}

/* -------- exact relations and court -------- */
static char outdir[1024];
static FILE *open_out(const char *name) {
    char path[1200];
    snprintf(path, sizeof(path), "%s/%s", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot open artifact");
    return f;
}

static void write_chunk_seam_fixture(void) {
    static const uint8_t prefix[] = "ababababababababa";
    static const uint8_t extended[] = "ababababababababab";
    static const uint8_t chunk[] = "b";
    Model M;
    memset(&M, 0, sizeof(M));
    model_build(&M, (const uint8_t *)"", 0);
    M.merge_l[0] = (uint32_t)'a';
    M.merge_r[0] = (uint32_t)'b';
    M.nmerges = 1;
    M.nunits = BASE_UNITS + 1;
    {
        const uint8_t ab[2] = {'a', 'b'};
        mexp_append(&M, BASE_UNITS, ab, 2);
    }

    size_t np, ne, nc;
    uint32_t *old = segment(&M, prefix, sizeof(prefix) - 1, &np);
    uint32_t *all = segment(&M, extended, sizeof(extended) - 1, &ne);
    uint32_t *fresh = segment(&M, chunk, sizeof(chunk) - 1, &nc);
    size_t old_bytes = token_coverage(&M, old, np);
    size_t unsafe_bytes = token_coverage(&M, all, np);
    size_t fresh_bytes = token_coverage(&M, fresh, nc);
    if (old_bytes != sizeof(prefix) - 1 || unsafe_bytes != sizeof(extended) - 1 ||
        unsafe_bytes == sizeof(prefix) - 1 || fresh_bytes != sizeof(chunk) - 1 ||
        nc != 1 || M.exp_len[fresh[0]] != 1 ||
        M.pool[M.exp_off[fresh[0]]] != chunk[0])
        die("chunk seam fixture failed");

    FILE *f = open_out("chunk_seam_fixture.tsv");
    fprintf(f, "status\tprefix_bytes\tprefix_tokens\tprefix_coverage\t"
            "unsafe_first_prefix_tokens_coverage\tchunk_bytes\t"
            "seam_clamped_chunk_tokens\tseam_clamped_chunk_coverage\n");
    fprintf(f, "PASS\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\t%zu\n",
            sizeof(prefix) - 1, np, old_bytes, unsafe_bytes,
            sizeof(chunk) - 1, nc, fresh_bytes);
    fclose(f);
    free(old); free(all); free(fresh);
    free(M.stream); free(M.tri); free(M.bi); free(M.pool);
}

static void write_null_scramble_fixture(void) {
    static const uint8_t arrival[8] = {
        11, 29, 47, 83, 131, 173, 211, 251
    };
    static const uint8_t expected[NULL_K][8] = {
        {211, 131,  11, 173, 251,  83,  29,  47},
        { 83,  11,  29, 211,  47, 131, 173, 251},
        {211,  29, 173, 251,  11,  47, 131,  83},
        { 29,  47, 173, 251,  83, 211,  11, 131},
        {131, 173,  11,  83,  47,  29, 211, 251},
        {131,  47, 173,  83, 211,  11, 251,  29},
        {173,  11, 131, 251,  83,  29,  47, 211},
        {211, 173, 251,  83,  11,  29,  47, 131},
        { 11, 131,  47, 173, 251, 211,  29,  83},
        { 11,  47, 131, 251, 211,  83,  29, 173},
        {211, 131, 173,  83, 251,  29,  47,  11},
        { 11, 251,  47, 131, 211,  83, 173,  29},
        { 29, 131,  83,  47, 211,  11, 251, 173},
        { 29,  11, 211, 251, 173,  83,  47, 131},
        {131,  83,  11, 173,  29, 251, 211,  47},
        { 11,  29,  83,  47, 251, 173, 131, 211},
        {211,  47, 173, 131, 251,  11,  83,  29},
        {131,  11, 173, 211,  47, 251,  29,  83},
        { 47, 173,  83, 211, 251,  29,  11, 131}
    };
    int alive[256] = {0};
    uint64_t rank[256];
    uint8_t rho[256];
    for (int d = 0; d < 256; d++) rank[d] = UINT64_MAX;
    for (int i = 0; i < 8; i++) {
        alive[arrival[i]] = 1;
        rank[arrival[i]] = (uint64_t)i;
    }

    FILE *f = open_out("null_scramble_fixture.tsv");
    fprintf(f, "arm\tseed\tarrival_index\tdestination\tprofile\texpected\texact\n");
    for (int k = 0; k < NULL_K; k++) {
        if (profile_scramble(alive, rank, NULL_SEEDS[k], rho) != 8)
            die("null scramble fixture has wrong alive count");
        for (int i = 0; i < 8; i++) {
            uint8_t got = rho[arrival[i]];
            int exact = got == expected[k][i];
            fprintf(f, "null%d\t%016llx\t%d\t%u\t%u\t%u\t%d\n",
                    k, (unsigned long long)NULL_SEEDS[k], i,
                    arrival[i], got, expected[k][i], exact);
            if (!exact) die("null scramble fixture mismatch");
        }
    }
    fclose(f);
}

enum {
    ARM_COLD = 0,
    ARM_REL = 1,
    ARM_NULL0 = 2,
    ARM_ORACLE = ARM_NULL0 + NULL_K,
    NARMS
};
static const char *ARM_NAMES[NARMS] = {
    "cold", "relation",
    "null0", "null1", "null2", "null3",
    "null4", "null5", "null6", "null7",
    "null8", "null9", "null10", "null11",
    "null12", "null13", "null14", "null15",
    "null16", "null17", "null18",
    "oracle"
};

typedef struct {
    uint8_t target_s, target_d;
    uint32_t target_epoch;
    uint8_t cl;
    uint8_t ctx_s[3], ctx_d[3];
    uint32_t ctx_epoch[3];
} RelationKey;

typedef struct {
    RelationKey key;
    uint64_t seen, positive, negative;
    double ledger, peak, L;
    int state, ever_earned;
} Relation;

typedef struct {
    Relation *rel;
    int32_t *slot;
    uint32_t n;
} RelationBook;

typedef struct {
    int16_t fwd[256], inv[256];
    int pair_prev[256];
    uint32_t pair_epoch[256];
    int matched;
    RelationBook book;
    uint64_t earns, revokes, winner_uses, suppressions;
} Arm;

typedef struct {
    uint64_t earns, revokes, winner_uses, suppressions;
} ChunkStats;

static int relation_equal(const RelationKey *a, const RelationKey *b);

static int same_double(double a, double b) {
    return memcmp(&a, &b, sizeof(a)) == 0;
}

static void assert_chunk_identical(const ChunkStats *a, const ChunkStats *b) {
    if (a->earns != b->earns || a->revokes != b->revokes ||
        a->winner_uses != b->winner_uses ||
        a->suppressions != b->suppressions)
        die("neutral B changed downstream chunk statistics");
}

static void assert_arm_identical(const Arm *a, const Arm *b) {
    if (memcmp(a->fwd, b->fwd, sizeof(a->fwd)) != 0 ||
        memcmp(a->inv, b->inv, sizeof(a->inv)) != 0 ||
        memcmp(a->pair_prev, b->pair_prev, sizeof(a->pair_prev)) != 0 ||
        memcmp(a->pair_epoch, b->pair_epoch, sizeof(a->pair_epoch)) != 0 ||
        a->matched != b->matched || a->book.n != b->book.n ||
        a->earns != b->earns || a->revokes != b->revokes ||
        a->winner_uses != b->winner_uses ||
        a->suppressions != b->suppressions)
        die("neutral B changed downstream arm state");
    if (memcmp(a->book.slot, b->book.slot,
               REL_HASH * sizeof(*a->book.slot)) != 0)
        die("neutral B changed downstream relation index");
    for (uint32_t i = 0; i < a->book.n; i++) {
        const Relation *ra = &a->book.rel[i];
        const Relation *rb = &b->book.rel[i];
        if (!relation_equal(&ra->key, &rb->key) ||
            ra->seen != rb->seen || ra->positive != rb->positive ||
            ra->negative != rb->negative || ra->state != rb->state ||
            ra->ever_earned != rb->ever_earned ||
            !same_double(ra->ledger, rb->ledger) ||
            !same_double(ra->peak, rb->peak) ||
            !same_double(ra->L, rb->L))
            die("neutral B changed downstream relation state");
    }
}

static uint64_t relation_hash(const RelationKey *k) {
    uint64_t h = 1469598103934665603ull;
#define HBYTE(x) do { h ^= (uint8_t)(x); h *= 1099511628211ull; } while (0)
    HBYTE(k->target_s); HBYTE(k->target_d);
    for (int sh = 0; sh < 32; sh += 8) HBYTE(k->target_epoch >> sh);
    HBYTE(k->cl);
    for (int i = 0; i < 3; i++) {
        HBYTE(k->ctx_s[i]); HBYTE(k->ctx_d[i]);
        for (int sh = 0; sh < 32; sh += 8) HBYTE(k->ctx_epoch[i] >> sh);
    }
#undef HBYTE
    return h;
}

static int relation_equal(const RelationKey *a, const RelationKey *b) {
    if (a->target_s != b->target_s || a->target_d != b->target_d ||
        a->target_epoch != b->target_epoch || a->cl != b->cl) return 0;
    for (int i = 0; i < 3; i++)
        if (a->ctx_s[i] != b->ctx_s[i] || a->ctx_d[i] != b->ctx_d[i] ||
            a->ctx_epoch[i] != b->ctx_epoch[i]) return 0;
    return 1;
}

static int relation_key_cmp(const RelationKey *a, const RelationKey *b) {
#define CMP_FIELD(x) do { if (a->x != b->x) return a->x < b->x ? -1 : 1; } while (0)
    CMP_FIELD(target_s); CMP_FIELD(target_d); CMP_FIELD(target_epoch);
    CMP_FIELD(cl);
    for (int i = 0; i < 3; i++) {
        if (a->ctx_s[i] != b->ctx_s[i]) return a->ctx_s[i] < b->ctx_s[i] ? -1 : 1;
        if (a->ctx_d[i] != b->ctx_d[i]) return a->ctx_d[i] < b->ctx_d[i] ? -1 : 1;
        if (a->ctx_epoch[i] != b->ctx_epoch[i])
            return a->ctx_epoch[i] < b->ctx_epoch[i] ? -1 : 1;
    }
#undef CMP_FIELD
    return 0;
}

static void book_init(RelationBook *B) {
    B->rel = calloc(REL_CAP, sizeof(Relation));
    B->slot = malloc(REL_HASH * sizeof(int32_t));
    if (!B->rel || !B->slot) die("oom relation book");
    for (uint32_t i = 0; i < REL_HASH; i++) B->slot[i] = -1;
    B->n = 0;
}

static void book_free(RelationBook *B) {
    free(B->rel);
    free(B->slot);
    memset(B, 0, sizeof(*B));
}

static Relation *relation_get(RelationBook *B, const RelationKey *key) {
    uint32_t h = (uint32_t)relation_hash(key) & (REL_HASH - 1);
    for (;;) {
        int32_t at = B->slot[h];
        if (at < 0) {
            if (B->n >= REL_CAP) die("relation table full");
            at = (int32_t)B->n++;
            B->slot[h] = at;
            B->rel[at].key = *key;
            return &B->rel[at];
        }
        if (relation_equal(&B->rel[at].key, key)) return &B->rel[at];
        h = (h + 1) & (REL_HASH - 1);
    }
}

static void arm_init(Arm *A) {
    memset(A, 0, sizeof(*A));
    for (int s = 0; s < 256; s++)
        A->fwd[s] = A->inv[s] = A->pair_prev[s] = -1;
    book_init(&A->book);
}

static void arm_set_map(Arm *A, const int16_t fwd[256],
                        const int16_t inv[256], int matched) {
    for (int s = 0; s < 256; s++) {
        int d = fwd[s];
        if (d != A->pair_prev[s]) {
            A->pair_prev[s] = d;
            A->pair_epoch[s]++;
        }
    }
    memcpy(A->fwd, fwd, sizeof(A->fwd));
    memcpy(A->inv, inv, sizeof(A->inv));
    A->matched = matched;
}

static void make_relation_key(const Arm *A, const uint8_t *W, size_t bp,
                              uint8_t s, uint8_t d, RelationKey *key) {
    memset(key, 0, sizeof(*key));
    key->target_s = s;
    key->target_d = d;
    key->target_epoch = A->pair_epoch[s];
    uint8_t ns[3], nd[3];
    uint32_t ne[3];
    int cl = 0;
    for (int k = 1; k <= 3 && bp >= (size_t)k; k++) {
        uint8_t db = W[bp - (size_t)k];
        int16_t ss = A->inv[db];
        if (ss < 0) break;
        ns[cl] = (uint8_t)ss;
        nd[cl] = db;
        ne[cl] = A->pair_epoch[ss];
        cl++;
    }
    key->cl = (uint8_t)cl;
    for (int i = 0; i < cl; i++) {
        key->ctx_s[i] = ns[cl - 1 - i];
        key->ctx_d[i] = nd[cl - 1 - i];
        key->ctx_epoch[i] = ne[cl - 1 - i];
    }
}

static double specialist_delta(const Model *M, const FirstUnits *FU,
                               uint32_t c1, uint32_t c2, uint32_t truth,
                               const RelationKey *key) {
    double pE = 0;
    uint8_t d = key->target_d;
    for (uint32_t i = FU->off[d]; i < FU->off[d + 1]; i++)
        pE += price_local(M, c1, c2, FU->unit[i]);
    if (!(pE > 0.0 && pE < 1.0)) die("specialist local event outside (0,1)");
    double pv[256];
    unit_start_ps_vec(key->ctx_s, key->cl, pv);
    double r = pv[key->target_s];
    if (!(r > 0.0 && r < 1.0)) die("specialist source event outside (0,1)");
    const uint8_t *tb = M->pool + M->exp_off[truth];
    return tb[0] == d ? log2(r / pE) : log2((1.0 - r) / (1.0 - pE));
}

static void write_key(FILE *f, const RelationKey *k) {
    fprintf(f, "%u\t%u\t%u\t%u", k->target_s, k->target_d,
            k->target_epoch, k->cl);
    for (int i = 0; i < 3; i++)
        fprintf(f, "\t%u\t%u\t%u", k->ctx_s[i], k->ctx_d[i],
                k->ctx_epoch[i]);
}

static void apply_shadow_receipt(Relation *r, double delta) {
    r->seen++;
    r->ledger += delta;
    if (delta > 0) r->positive++;
    else if (delta < 0) r->negative++;
    if (r->ledger > r->peak) r->peak = r->ledger;
}

static double relation_price_and_update(Arm *A, int arm_index,
                                        const Model *M, const FirstUnits *FU,
                                        uint32_t c1, uint32_t c2,
                                        uint32_t truth, double pl,
                                        const uint8_t *W, size_t bp,
                                        size_t unit_pos, ChunkStats *stats,
                                        FILE *winner_out, FILE *event_out) {
    Relation *active[256], *winner = NULL;
    double delta[256];
    uint8_t source[256];
    int n = 0, winner_at = -1;
    for (int s = 0; s < 256; s++) {
        if (A->fwd[s] < 0) continue;
        RelationKey key;
        make_relation_key(A, W, bp, (uint8_t)s, (uint8_t)A->fwd[s], &key);
        Relation *r = relation_get(&A->book, &key);
        active[n] = r;
        source[n] = (uint8_t)s;
        if (key.cl >= 1 && r->state &&
            (!winner || r->ledger > winner->ledger ||
             (r->ledger == winner->ledger &&
              relation_key_cmp(&r->key, &winner->key) < 0))) {
            winner = r;
            winner_at = n;
        }
        n++;
    }
    for (int i = 0; i < n; i++)
        delta[i] = specialist_delta(M, FU, c1, c2, truth, &active[i]->key);

    double pfin = pl;
    if (winner) {
        double qtruth = pl * exp2(delta[winner_at]);
        pfin = (1.0 - winner->L) * pl + winner->L * qtruth;
        if (!(pfin > 0.0 && pfin <= 1.0 + 1e-12))
            die("live relation price outside probability range");
        stats->winner_uses++;
        A->winner_uses++;
        fprintf(winner_out, "%zu\t%zu\t%s\t%.17g\t%.17g\t",
                bp, unit_pos, ARM_NAMES[arm_index], winner->ledger, winner->L);
        write_key(winner_out, &winner->key);
        fputc('\n', winner_out);
        double wrate = winner->seen ? winner->ledger / (double)winner->seen : 0;
        for (int i = 0; i < n; i++) {
            Relation *r = active[i];
            if (r == winner || !r->state || r->key.cl < 1 || !r->seen) continue;
            double rate = r->ledger / (double)r->seen;
            if (r->seen < winner->seen && rate > wrate) {
                stats->suppressions++;
                A->suppressions++;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        Relation *r = active[i];
        apply_shadow_receipt(r, delta[i]);
        if (r->key.cl == 0) {
            if (r->state || r->ever_earned || r->L != 0.0)
                die("context-zero relation acquired authority");
            continue;
        }
        if (r->state) {
            double next = r->L * exp(ETA * delta[i]);
            r->L = next < L_LO ? L_LO : next > L_HI ? L_HI : next;
        }
        if (!r->state && r->ledger >= EARN_BITS) {
            r->state = 1;
            r->ever_earned = 1;
            r->L = L_ENTER;
            stats->earns++;
            A->earns++;
            fprintf(event_out, "%zu\t%zu\t%s\tearn\t%.17g\t",
                    bp, unit_pos, ARM_NAMES[arm_index], r->ledger);
            write_key(event_out, &r->key);
            fputc('\n', event_out);
        } else if (r->state && r->ledger < REVOKE_BITS) {
            r->state = 0;
            r->L = 0;
            stats->revokes++;
            A->revokes++;
            fprintf(event_out, "%zu\t%zu\t%s\trevoke\t%.17g\t",
                    bp, unit_pos, ARM_NAMES[arm_index], r->ledger);
            write_key(event_out, &r->key);
            fputc('\n', event_out);
        }
        (void)source[i];
    }
    return pfin;
}

static void surface_shadow_update(RelationBook books[2], const Model *M,
                                  const FirstUnits *FU, uint32_t c1,
                                  uint32_t c2, uint32_t truth,
                                  const uint8_t *W, size_t bp) {
    static const uint8_t target[2] = {(uint8_t)'a', (uint8_t)'e'};
    for (int b = 0; b < 2; b++) {
        RelationKey key;
        memset(&key, 0, sizeof(key));
        key.target_s = target[b];
        key.target_d = target[b];
        key.target_epoch = 1;
        size_t cl = bp < 3 ? bp : 3;
        key.cl = (uint8_t)cl;
        for (size_t i = 0; i < cl; i++) {
            uint8_t x = W[bp - cl + i];
            key.ctx_s[i] = x;
            key.ctx_d[i] = x;
            key.ctx_epoch[i] = 1;
        }
        double delta = specialist_delta(M, FU, c1, c2, truth, &key);
        Relation *r = relation_get(&books[b], &key);
        apply_shadow_receipt(r, delta);
        if (r->state || r->ever_earned || r->L != 0.0)
            die("forced surface shadow acquired authority");
    }
}

static void write_surface_shadows(const char *wname, RelationBook books[2]) {
    char fn[160];
    snprintf(fn, sizeof(fn), "surface_shadows_%s.tsv", wname);
    FILE *f = open_out(fn);
    fprintf(f, "surface_pair\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\t"
            "c1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\t"
            "c3_s\tc3_d\tc3_epoch\tseen\tpositive\tnegative\t"
            "ledger_bits\tpeak_bits\tstate\tL\tever_earned\n");
    static const char *name[2] = {"a_to_a", "e_to_e"};
    for (int b = 0; b < 2; b++)
        for (uint32_t i = 0; i < books[b].n; i++) {
            Relation *r = &books[b].rel[i];
            fprintf(f, "%s\t", name[b]);
            write_key(f, &r->key);
            fprintf(f, "\t%llu\t%llu\t%llu\t%.17g\t%.17g\t%d\t%.17g\t%d\n",
                    (unsigned long long)r->seen,
                    (unsigned long long)r->positive,
                    (unsigned long long)r->negative, r->ledger, r->peak,
                    r->state, r->L, r->ever_earned);
        }
    fclose(f);
}

static double downstream_fixture_step(Arm arms[NARMS], const Model *M,
                                      const FirstUnits *FU, uint32_t truth,
                                      double pl, const uint8_t *W, size_t bp,
                                      size_t step, ChunkStats *live_stats,
                                      FILE *winner_out, FILE *event_out) {
    double pfin[NARMS] = {0};
    ChunkStats stats[NARMS];
    memset(stats, 0, sizeof(stats));
    for (int a = ARM_REL; a < ARM_ORACLE; a++)
        pfin[a] = relation_price_and_update(&arms[a], a, M, FU,
                                            UINT32_MAX, UINT32_MAX,
                                            truth, pl, W, bp, step, &stats[a],
                                            winner_out, event_out);
    for (int k = 0; k < NULL_K; k++) {
        int a = ARM_NULL0 + k;
        if (!same_double(pfin[ARM_REL], pfin[a]))
            die("downstream fixture price mismatch");
        assert_chunk_identical(&stats[ARM_REL], &stats[a]);
        assert_arm_identical(&arms[ARM_REL], &arms[a]);
    }
    *live_stats = stats[ARM_REL];
    return pfin[ARM_REL];
}

static void write_downstream_fixture(void) {
    uint8_t source = 0;
    double source_p = -1.0;
    for (int s = 0; s < 256; s++) {
        if (!us1cnt[s]) continue;
        uint8_t ctx[3] = {(uint8_t)s, (uint8_t)s, (uint8_t)s};
        double pv[256];
        unit_start_ps_vec(ctx, 3, pv);
        if (pv[s] > source_p ||
            (same_double(pv[s], source_p) &&
             source_first[s] < source_first[source])) {
            source_p = pv[s];
            source = (uint8_t)s;
        }
    }
    if (!(source_p > 0.0 && source_p < 1.0))
        die("downstream fixture cannot select source event");

    const uint8_t destination = 255;
    const uint8_t local_other = 0;
    uint8_t local_train[4096];
    memset(local_train, local_other, sizeof(local_train));
    Model M;
    memset(&M, 0, sizeof(M));
    model_build(&M, local_train, sizeof(local_train));
    FirstUnits FU;
    first_units_build(&M, &FU);

    Arm arms[NARMS];
    memset(arms, 0, sizeof(arms));
    for (int a = ARM_REL; a < ARM_ORACLE; a++) arm_init(&arms[a]);
    int16_t fwd[256], inv[256];
    for (int i = 0; i < 256; i++) fwd[i] = inv[i] = -1;
    fwd[source] = destination;
    inv[destination] = source;
    for (int a = ARM_REL; a < ARM_ORACLE; a++)
        arm_set_map(&arms[a], fwd, inv, 1);
    for (int k = 0; k < NULL_K; k++)
        assert_arm_identical(&arms[ARM_REL], &arms[ARM_NULL0 + k]);

    uint8_t context[4] = {
        destination, destination, destination, destination
    };
    RelationKey initial_key;
    make_relation_key(&arms[ARM_REL], context, 3, source, destination,
                      &initial_key);
    if (initial_key.cl != 3 || initial_key.target_epoch != 1)
        die("downstream fixture initial key is not contextual epoch one");

    double pl_win = price_local(&M, UINT32_MAX, UINT32_MAX, destination);
    double pl_loss = price_local(&M, UINT32_MAX, UINT32_MAX, local_other);
    double delta_win = specialist_delta(&M, &FU, UINT32_MAX, UINT32_MAX,
                                        destination, &initial_key);
    double delta_loss = specialist_delta(&M, &FU, UINT32_MAX, UINT32_MAX,
                                         local_other, &initial_key);
    if (!(delta_win > 0.0 && delta_loss < 0.0))
        die("downstream fixture lacks both receipt signs");

    FILE *winner_out = open_out("downstream_fixture_winners.tsv");
    FILE *event_out = open_out("downstream_fixture_events.tsv");
    FILE *trace = open_out("downstream_fixture.tsv");
    FILE *summary = open_out("downstream_fixture_summary.tsv");
    fprintf(winner_out, "byte_offset\tunit_position\tarm\tledger_before\tL_before\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\n");
    fprintf(event_out, "byte_offset\tunit_position\tarm\tevent\tledger_after\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\n");
    fprintf(trace, "step\tphase\ttruth\tp_local\tp_final\tledger\tstate\tL\tseen\tpositive\tnegative\tearns\trevokes\twinner_uses\n");

    size_t step = 0;
    size_t earn_step = 0, revoke_step = 0;
    int saw_live_price = 0;
    while (!arms[ARM_REL].book.n || !arms[ARM_REL].book.rel[0].state) {
        if (step >= 10000) die("downstream fixture did not earn");
        ChunkStats cs;
        double pfin = downstream_fixture_step(arms, &M, &FU, destination,
                                              pl_win, context, 3, step, &cs,
                                              winner_out, event_out);
        Relation *r = &arms[ARM_REL].book.rel[0];
        fprintf(trace, "%zu\tearn\t%u\t%.17g\t%.17g\t%.17g\t%d\t%.17g\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\n",
                step, destination, pl_win, pfin, r->ledger, r->state, r->L,
                (unsigned long long)r->seen,
                (unsigned long long)r->positive,
                (unsigned long long)r->negative,
                (unsigned long long)arms[ARM_REL].earns,
                (unsigned long long)arms[ARM_REL].revokes,
                (unsigned long long)arms[ARM_REL].winner_uses);
        step++;
    }
    earn_step = step - 1;
    if (!(arms[ARM_REL].book.rel[0].L > 0.0))
        die("downstream fixture earned without live weight");

    {
        ChunkStats cs;
        double pfin = downstream_fixture_step(arms, &M, &FU, destination,
                                              pl_win, context, 3, step, &cs,
                                              winner_out, event_out);
        Relation *r = &arms[ARM_REL].book.rel[0];
        if (same_double(pfin, pl_win) || cs.winner_uses == 0)
            die("downstream fixture did not exercise live pricing");
        saw_live_price = 1;
        fprintf(trace, "%zu\tlive\t%u\t%.17g\t%.17g\t%.17g\t%d\t%.17g\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\n",
                step, destination, pl_win, pfin, r->ledger, r->state, r->L,
                (unsigned long long)r->seen,
                (unsigned long long)r->positive,
                (unsigned long long)r->negative,
                (unsigned long long)arms[ARM_REL].earns,
                (unsigned long long)arms[ARM_REL].revokes,
                (unsigned long long)arms[ARM_REL].winner_uses);
        step++;
    }
    while (arms[ARM_REL].book.rel[0].state) {
        if (step >= 20000) die("downstream fixture did not revoke");
        ChunkStats cs;
        double pfin = downstream_fixture_step(arms, &M, &FU, local_other,
                                              pl_loss, context, 3, step, &cs,
                                              winner_out, event_out);
        Relation *r = &arms[ARM_REL].book.rel[0];
        fprintf(trace, "%zu\trevoke\t%u\t%.17g\t%.17g\t%.17g\t%d\t%.17g\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\n",
                step, local_other, pl_loss, pfin, r->ledger, r->state, r->L,
                (unsigned long long)r->seen,
                (unsigned long long)r->positive,
                (unsigned long long)r->negative,
                (unsigned long long)arms[ARM_REL].earns,
                (unsigned long long)arms[ARM_REL].revokes,
                (unsigned long long)arms[ARM_REL].winner_uses);
        step++;
    }
    revoke_step = step - 1;

    Relation retired = arms[ARM_REL].book.rel[0];
    int16_t empty[256];
    for (int i = 0; i < 256; i++) empty[i] = -1;
    for (int a = ARM_REL; a < ARM_ORACLE; a++) {
        arm_set_map(&arms[a], empty, empty, 0);
        arm_set_map(&arms[a], fwd, inv, 1);
    }
    for (int k = 0; k < NULL_K; k++)
        assert_arm_identical(&arms[ARM_REL], &arms[ARM_NULL0 + k]);
    RelationKey fresh_key;
    make_relation_key(&arms[ARM_REL], context, 3, source, destination,
                      &fresh_key);
    if (fresh_key.target_epoch == retired.key.target_epoch ||
        relation_equal(&fresh_key, &retired.key))
        die("downstream fixture remap inherited relation key");
    {
        ChunkStats cs;
        (void)downstream_fixture_step(arms, &M, &FU, destination, pl_win,
                                      context, 3, step, &cs,
                                      winner_out, event_out);
    }
    if (arms[ARM_REL].book.n < 2 ||
        relation_equal(&arms[ARM_REL].book.rel[0].key,
                       &arms[ARM_REL].book.rel[1].key) ||
        arms[ARM_REL].book.rel[1].seen != 1)
        die("downstream fixture did not create a fresh epoch relation");
    step++;

    uint64_t earns_before_cl0 = arms[ARM_REL].earns;
    uint64_t revokes_before_cl0 = arms[ARM_REL].revokes;
    Relation *cl0 = NULL;
    do {
        if (step >= 30000) die("context-zero fixture did not cross EARN ledger");
        ChunkStats cs;
        double pfin = downstream_fixture_step(arms, &M, &FU, destination,
                                              pl_win, context, 0, step, &cs,
                                              winner_out, event_out);
        if (!same_double(pfin, pl_win) || cs.earns || cs.revokes ||
            cs.winner_uses)
            die("context-zero fixture changed authority or voice");
        if (arms[ARM_REL].book.n < 3)
            die("context-zero fixture did not create telemetry relation");
        cl0 = &arms[ARM_REL].book.rel[2];
        if (cl0->key.cl != 0 || cl0->state || cl0->ever_earned || cl0->L != 0.0)
            die("context-zero fixture acquired authority");
        step++;
    } while (cl0->ledger < EARN_BITS);
    if (arms[ARM_REL].earns != earns_before_cl0 ||
        arms[ARM_REL].revokes != revokes_before_cl0)
        die("context-zero fixture polluted authority events");
    if (!arms[ARM_REL].book.rel[0].positive ||
        !arms[ARM_REL].book.rel[0].negative ||
        !arms[ARM_REL].earns || !arms[ARM_REL].revokes ||
        !arms[ARM_REL].winner_uses || !saw_live_price)
        die("downstream fixture did not cover the frozen state machine");

    Relation *fresh = &arms[ARM_REL].book.rel[1];
    fprintf(summary,
            "status\trole\tsource\tdestination\tlocal_other\tsource_p\t"
            "p_local_win\tp_local_loss\tdelta_win\tdelta_loss\t"
            "earn_step\trevoke_step\tretired_epoch\tfresh_epoch\t"
            "retired_seen\tretired_positive\tretired_negative\t"
            "retired_ledger\tretired_state\tretired_L\tfresh_seen\t"
            "fresh_ledger\tfresh_state\tfresh_L\tcl0_seen\tcl0_ledger\t"
            "cl0_state\tcl0_L\tcl0_ever_earned\texact_arms\n");
    fprintf(summary,
            "PASS\tfixture-not-court\t%u\t%u\t%u\t%.17g\t%.17g\t%.17g\t"
            "%.17g\t%.17g\t%zu\t%zu\t%u\t%u\t%llu\t%llu\t%llu\t"
            "%.17g\t%d\t%.17g\t%llu\t%.17g\t%d\t%.17g\t%llu\t%.17g\t"
            "%d\t%.17g\t%d\t%d\n",
            source, destination, local_other, source_p, pl_win, pl_loss,
            delta_win, delta_loss, earn_step, revoke_step,
            retired.key.target_epoch, fresh->key.target_epoch,
            (unsigned long long)retired.seen,
            (unsigned long long)retired.positive,
            (unsigned long long)retired.negative,
            retired.ledger, retired.state, retired.L,
            (unsigned long long)fresh->seen, fresh->ledger, fresh->state,
            fresh->L, (unsigned long long)cl0->seen, cl0->ledger, cl0->state,
            cl0->L, cl0->ever_earned, 1 + NULL_K);
    fclose(summary);
    fclose(trace);
    fclose(winner_out);
    fclose(event_out);
    for (int a = ARM_REL; a < ARM_ORACLE; a++) book_free(&arms[a].book);
    free(M.stream); free(M.tri); free(M.bi); free(M.pool);
}

static uint32_t contextual_earned(const RelationBook *B) {
    uint32_t n = 0;
    for (uint32_t i = 0; i < B->n; i++)
        if (B->rel[i].key.cl >= 1 && B->rel[i].ever_earned) n++;
    return n;
}

static int map_hits(const int16_t fwd[256], const uint8_t *omap) {
    if (!omap) return -1;
    int n = 0;
    for (int s = 0; s < 256; s++)
        if (fwd[s] >= 0 && fwd[s] == omap[s]) n++;
    return n;
}

static void write_relations(const char *wname, Arm arms[NARMS], int has_oracle) {
    char fn[160];
    snprintf(fn, sizeof(fn), "relations_%s.tsv", wname);
    FILE *f = open_out(fn);
    fprintf(f, "arm\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\tseen\tpositive\tnegative\tledger_bits\tpeak_bits\tstate\tL\tever_earned\n");
    for (int a = 1; a < NARMS; a++) {
        if (a == ARM_ORACLE && !has_oracle) continue;
        RelationBook *B = &arms[a].book;
        for (uint32_t i = 0; i < B->n; i++) {
            Relation *r = &B->rel[i];
            fprintf(f, "%s\t", ARM_NAMES[a]);
            write_key(f, &r->key);
            fprintf(f, "\t%llu\t%llu\t%llu\t%.17g\t%.17g\t%d\t%.17g\t%d\n",
                    (unsigned long long)r->seen,
                    (unsigned long long)r->positive,
                    (unsigned long long)r->negative, r->ledger, r->peak,
                    r->state, r->L, r->ever_earned);
        }
    }
    fclose(f);
}

static void run_court(const char *wname, const uint8_t *W, size_t Wn_full,
                      const uint8_t *omap, int check_surface, int neutral_b,
                      double G[NARMS][5], size_t run_bytes,
                      int *surface_admitted_out) {
    int has_oracle = omap != NULL;
    size_t Wn = Wn_full < run_bytes ? Wn_full : run_bytes;
    char fn[160];
    snprintf(fn, sizeof(fn), "evidence_%s.tsv", wname);
    FILE *ef = open_out(fn);
    snprintf(fn, sizeof(fn), "maps_%s.tsv", wname);
    FILE *mf = open_out(fn);
    snprintf(fn, sizeof(fn), "map_summary_%s.tsv", wname);
    FILE *msf = open_out(fn);
    snprintf(fn, sizeof(fn), "profile_scrambles_%s.tsv", wname);
    FILE *psf = open_out(fn);
    snprintf(fn, sizeof(fn), "winners_%s.tsv", wname);
    FILE *wf = open_out(fn);
    snprintf(fn, sizeof(fn), "relation_events_%s.tsv", wname);
    FILE *rf = open_out(fn);
    fprintf(ef, "chunk\tlived_before\tbytes\tarm\tpositions\tbits\tpairs\trelations\tcontextual_earned\tcontextual_earn_events\tcontextual_revoke_events\twinner_uses\tseniority_suppressions\tsurface_admitted\n");
    fprintf(mf, "chunk\tlived\tmap\tsource\tdest\tepoch\tcorrect\n");
    fprintf(msf, "chunk\tlived\tbonly_pairs\tbonly_hits\tadmitted_pairs\tadmitted_hits\tsurface_admitted\n");
    fprintf(psf, "chunk\tlived\tarm\tdestination\tprofile_destination\tfixed\n");
    fprintf(wf, "byte_offset\tunit_position\tarm\tledger_before\tL_before\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\n");
    fprintf(rf, "byte_offset\tunit_position\tarm\tevent\tledger_after\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\n");

    Model M;
    memset(&M, 0, sizeof(M));
    Arm arms[NARMS];
    memset(arms, 0, sizeof(arms));
    for (int a = 1; a < NARMS; a++) arm_init(&arms[a]);
    RelationBook surface_books[2];
    memset(surface_books, 0, sizeof(surface_books));
    if (check_surface)
        for (int b = 0; b < 2; b++) book_init(&surface_books[b]);
    FirstUnits FU;
    static const size_t horizons[5] = {1024, 4096, 16384, 65536, RUN_BYTES};
    for (int a = 0; a < NARMS; a++)
        for (int h = 0; h < 5; h++) G[a][h] = 0;
    int alive_src[256];
    for (int s = 0; s < 256; s++) alive_src[s] = s1cnt[s] > 0;
    uint8_t previous_rho[NULL_K][256];
    int previous_alive_dst[256] = {0};
    memset(previous_rho, 0xFF, sizeof(previous_rho));
    model_build(&M, W, 0);
    first_units_build(&M, &FU);
    int surface_admitted = 0;

    size_t lived = 0, chunk_no = 0, priced_units = 0;
    while (lived < Wn) {
        size_t clen = Wn - lived < CHUNK ? Wn - lived : CHUNK;
        size_t np, npc;
        uint32_t *old = segment(&M, W, lived, &np);
        uint32_t *fresh = segment(&M, W + lived, clen, &npc);
        if (token_coverage(&M, fresh, npc) != clen)
            die("seam-clamped chunk does not cover its raw bytes");
        double bits[NARMS] = {0};
        ChunkStats cs[NARMS];
        memset(cs, 0, sizeof(cs));
        size_t byte_off = lived;
        for (size_t pos = 0; pos < npc; pos++) {
            uint32_t truth = fresh[pos];
            uint32_t c2 = pos >= 1 ? fresh[pos - 1] :
                          np >= 1 ? old[np - 1] : UINT32_MAX;
            uint32_t c1 = pos >= 2 ? fresh[pos - 2] :
                          pos == 1 && np >= 1 ? old[np - 1] :
                          pos == 0 && np >= 2 ? old[np - 2] : UINT32_MAX;
            uint32_t span = M.exp_len[truth];
            if (span > lived + clen - byte_off ||
                memcmp(M.pool + M.exp_off[truth], W + byte_off, span) != 0)
                die("seam-clamped token expansion disagrees with raw chunk");
            double pl = price_local(&M, c1, c2, truth);
            bits[ARM_COLD] += -log2(pl);
            if (check_surface)
                surface_shadow_update(surface_books, &M, &FU, c1, c2, truth,
                                      W, byte_off);
            for (int a = 1; a < NARMS; a++) {
                if (a == ARM_ORACLE && !has_oracle) continue;
                double pfin = relation_price_and_update(&arms[a], a, &M, &FU,
                                                        c1, c2, truth, pl, W,
                                                        byte_off,
                                                        priced_units + pos,
                                                        &cs[a],
                                                        wf, rf);
                bits[a] += -log2(pfin);
            }
            byte_off += span;
        }
        if (byte_off != lived + clen)
            die("chunk pricing did not consume exactly clen raw bytes");
        size_t positions = npc;
        for (int a = 0; a < NARMS; a++) {
            if (a == ARM_ORACLE && !has_oracle) continue;
            uint32_t nr = a == ARM_COLD ? 0 : arms[a].book.n;
            uint32_t ce = a == ARM_COLD ? 0 : contextual_earned(&arms[a].book);
            int pairs = a == ARM_COLD ? 0 : arms[a].matched;
            fprintf(ef, "%zu\t%zu\t%zu\t%s\t%zu\t%.17g\t%d\t%u\t%u\t%llu\t%llu\t%llu\t%llu\t%d\n",
                    chunk_no, lived, clen, ARM_NAMES[a], positions, bits[a],
                    pairs, nr, ce,
                    (unsigned long long)cs[a].earns,
                    (unsigned long long)cs[a].revokes,
                    (unsigned long long)cs[a].winner_uses,
                    (unsigned long long)cs[a].suppressions,
                    surface_admitted);
            for (int h = 0; h < 5; h++)
                if (lived + clen <= horizons[h])
                    G[a][h] += bits[ARM_COLD] - bits[a];
        }
        if (neutral_b)
            for (int k = 0; k < NULL_K; k++) {
                int a = ARM_NULL0 + k;
                if (!same_double(bits[ARM_REL], bits[a]))
                    die("neutral B changed downstream price");
                assert_chunk_identical(&cs[ARM_REL], &cs[a]);
                assert_arm_identical(&arms[ARM_REL], &arms[a]);
            }
        free(old);
        free(fresh);
        priced_units += npc;
        lived += clen;
        chunk_no++;

        model_build(&M, W, lived);
        first_units_build(&M, &FU);
        int alive_dst[256];
        uint64_t dc[256] = {0}, dest_first[256];
        for (int d = 0; d < 256; d++) dest_first[d] = UINT64_MAX;
        for (size_t i = 0; i < lived; i++) {
            uint8_t d = W[i];
            dc[d]++;
            if (dest_first[d] == UINT64_MAX) dest_first[d] = (uint64_t)i;
        }
        for (int d = 0; d < 256; d++) alive_dst[d] = dc[d] > 0;
        static double R[256][256], L[256][256], B[256][256], F[256][256];
        trans_kt(W, lived, R, L);
        build_B(B, R, L);
        if (neutral_b) memset(B, 0, sizeof(B));
        frequency_score(dc, lived, F);
        int16_t learned_fwd[256], learned_inv[256], bonly_fwd[256];
        int16_t null_fwd[NULL_K][256], null_inv[NULL_K][256];
        uint8_t null_rho[NULL_K][256];
        int16_t oracle_fwd[256], oracle_inv[256];
        int learned_n, bonly_n, null_n[NULL_K], oracle_n = 0;
        admission_map(B, F, alive_src, alive_dst, source_first, dest_first,
                      learned_fwd, learned_inv, &learned_n,
                      bonly_fwd, &bonly_n);
        check_admission_equivariance(B, F, alive_src, alive_dst, source_first,
                                     dest_first, learned_fwd);
        for (int k = 0; k < NULL_K; k++)
            structural_null_admission(B, F, alive_src, alive_dst,
                                      source_first, dest_first, NULL_SEEDS[k],
                                      null_rho[k], null_fwd[k], null_inv[k],
                                      &null_n[k]);
        if (neutral_b)
            for (int k = 0; k < NULL_K; k++)
                if (null_n[k] != learned_n ||
                    memcmp(null_fwd[k], learned_fwd,
                           sizeof(learned_fwd)) != 0 ||
                    memcmp(null_inv[k], learned_inv,
                           sizeof(learned_inv)) != 0)
                    die("neutral B changed admission map");
        int newly_alive = 0;
        for (int d = 0; d < 256; d++)
            if (alive_dst[d] && !previous_alive_dst[d]) newly_alive++;
        for (int k = 0; k < NULL_K; k++) {
            int changed_old = 0;
            for (int d = 0; d < 256; d++)
                if (previous_alive_dst[d] &&
                    previous_rho[k][d] != null_rho[k][d])
                    changed_old++;
            if (changed_old > newly_alive)
                die("structural null churn exceeds alphabet growth");
        }
        if (has_oracle)
            oracle_map(omap, learned_fwd, alive_dst,
                       oracle_fwd, oracle_inv, &oracle_n);
        if (check_surface && alive_src[(uint8_t)'a'] && alive_src[(uint8_t)'e'] &&
            alive_dst[(uint8_t)'a'] && alive_dst[(uint8_t)'e'] &&
            (learned_fwd[(uint8_t)'a'] == (uint8_t)'a' ||
             learned_fwd[(uint8_t)'e'] == (uint8_t)'e'))
            surface_admitted = 1;

        arm_set_map(&arms[ARM_REL], learned_fwd, learned_inv, learned_n);
        for (int k = 0; k < NULL_K; k++)
            arm_set_map(&arms[ARM_NULL0 + k], null_fwd[k], null_inv[k],
                        null_n[k]);
        if (neutral_b)
            for (int k = 0; k < NULL_K; k++)
                assert_arm_identical(&arms[ARM_REL],
                                     &arms[ARM_NULL0 + k]);
        if (has_oracle)
            arm_set_map(&arms[ARM_ORACLE], oracle_fwd, oracle_inv, oracle_n);

        int bh = map_hits(bonly_fwd, omap);
        int lh = map_hits(learned_fwd, omap);
        fprintf(msf, "%zu\t%zu\t%d\t%d\t%d\t%d\t%d\n",
                chunk_no, lived, bonly_n, bh, learned_n, lh,
                surface_admitted);
        for (int k = 0; k < NULL_K; k++)
            for (int d = 0; d < 256; d++)
                if (alive_dst[d])
                    fprintf(psf, "%zu\t%zu\t%s\t%d\t%u\t%d\n",
                            chunk_no, lived, ARM_NAMES[ARM_NULL0 + k], d,
                            null_rho[k][d], null_rho[k][d] == d);
        memcpy(previous_rho, null_rho, sizeof(previous_rho));
        memcpy(previous_alive_dst, alive_dst, sizeof(previous_alive_dst));
        for (int s = 0; s < 256; s++) {
            if (bonly_fwd[s] >= 0)
                fprintf(mf, "%zu\t%zu\tbonly\t%d\t%d\t0\t%d\n",
                        chunk_no, lived, s, bonly_fwd[s],
                        omap ? bonly_fwd[s] == omap[s] : -1);
            for (int a = 1; a < NARMS; a++) {
                if (a == ARM_ORACLE && !has_oracle) continue;
                if (arms[a].fwd[s] >= 0)
                    fprintf(mf, "%zu\t%zu\t%s\t%d\t%d\t%u\t%d\n",
                            chunk_no, lived, ARM_NAMES[a], s, arms[a].fwd[s],
                            arms[a].pair_epoch[s],
                            omap ? arms[a].fwd[s] == omap[s] : -1);
            }
        }
    }

    if (neutral_b)
        for (int k = 0; k < NULL_K; k++)
            for (int h = 0; h < 5; h++)
                if (!same_double(G[ARM_REL][h],
                                 G[ARM_NULL0 + k][h]))
                    die("neutral B changed G horizon");

    write_relations(wname, arms, has_oracle);
    if (check_surface) write_surface_shadows(wname, surface_books);
    fclose(ef); fclose(mf); fclose(msf); fclose(psf); fclose(wf); fclose(rf);
    free(M.stream); free(M.tri); free(M.bi); free(M.pool);
    for (int a = 1; a < NARMS; a++) book_free(&arms[a].book);
    if (check_surface)
        for (int b = 0; b < 2; b++) book_free(&surface_books[b]);
    if (surface_admitted_out) *surface_admitted_out = surface_admitted;
    fprintf(stderr,
            "court4 %s: %zu chunks | pairs rel/or %d/%d | null0..18",
            wname, chunk_no, arms[ARM_REL].matched,
            has_oracle ? arms[ARM_ORACLE].matched : 0);
    for (int k = 0; k < NULL_K; k++)
        fprintf(stderr, " %d", arms[ARM_NULL0 + k].matched);
    fprintf(stderr, " | surface %d\n", surface_admitted);
}

int main(int argc, char **argv) {
    check_null_seeds();
    const char *apath = NULL, *dpath = NULL;
    outdir[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(outdir, sizeof(outdir), "%s", argv[++i]);
        else if (!apath) apath = argv[i];
        else dpath = argv[i];
    }
    if (!apath || !dpath || !outdir[0]) die("usage: transfer <sourceA> <baseD> --out <dir>");

    write_null_scramble_fixture();
    write_chunk_seam_fixture();

    FILE *f = fopen(apath, "rb");
    if (!f) die("open A");
    fseek(f, 0, SEEK_END); size_t an = (size_t)ftell(f); fseek(f, 0, SEEK_SET);
    uint8_t *A = malloc(an);
    if (!A || fread(A, 1, an, f) != an) die("read A");
    fclose(f);
    size_t atrain = (an * 9) / 10;
    cargo_build(A, atrain);
    for (int s = 0; s < 256; s++) source_first[s] = UINT64_MAX;
    for (size_t i = 0; i < atrain; i++)
        if (source_first[A[i]] == UINT64_MAX) source_first[A[i]] = (uint64_t)i;
    Model source_units;
    memset(&source_units, 0, sizeof(source_units));
    model_build(&source_units, A, atrain);
    unit_start_build(A, atrain, &source_units);
    fprintf(stderr, "source unit starts: %llu | learned units %u\n",
            (unsigned long long)us1tot, source_units.nunits);
    free(source_units.stream); free(source_units.tri); free(source_units.bi);
    free(source_units.pool);
    write_downstream_fixture();
    trans_kt(A, atrain, srcA2, srcA2L);
    for (int s = 0; s < 256; s++) {
        memcpy(RA[s], srcA2[s], sizeof(RA[s]));
        memcpy(LAp[s], srcA2L[s], sizeof(LAp[s]));
        qsort(RA[s], 256, sizeof(double), cmp_desc);
        qsort(LAp[s], 256, sizeof(double), cmp_desc);
    }

    f = fopen(dpath, "rb");
    if (!f) die("open D");
    if (fseek(f, 0, SEEK_END) != 0) die("seek D");
    long dend = ftell(f);
    if (dend < 0) die("size D");
    size_t dn = (size_t)dend;
    if (dn < RUN_BYTES) die("D shorter than frozen RUN_BYTES");
    if (fseek(f, 0, SEEK_SET) != 0) die("rewind D");
    uint8_t *D = malloc(dn);
    if (!D || fread(D, 1, dn, f) != dn) die("read D");
    fclose(f);

    fy_perm(cipher_pi, CIPHER_SEED);

    uint8_t *Wiso = build_cipher(D, dn);
    uint8_t *Wgh = build_ghost_bytes(D, dn, dn, GHOST_SEED);
    size_t ffn;
    uint8_t *Wff = build_ff(D, dn, &ffn);
    if (ffn < RUN_BYTES) die("false-friend world shorter than frozen RUN_BYTES");
    uint8_t *Wswap = malloc(dn ? dn : 1);
    if (!Wswap) die("oom swap-ea world");
    for (size_t i = 0; i < dn; i++)
        Wswap[i] = D[i] == (uint8_t)'e' ? (uint8_t)'a' :
                   D[i] == (uint8_t)'a' ? (uint8_t)'e' : D[i];
    uint8_t *Whalf = malloc(RUN_BYTES);
    if (!Whalf) die("oom");
    memcpy(Whalf, Wiso, HALF_BOUNDARY);
    {
        uint8_t *tail = build_ghost_bytes(D, dn, RUN_BYTES - HALF_BOUNDARY, HALFTAIL_SEED);
        memcpy(Whalf + HALF_BOUNDARY, tail, RUN_BYTES - HALF_BOUNDARY);
        free(tail);
    }
    uint8_t *aux1 = malloc(AUX_BYTES), *aux2 = malloc(AUX_BYTES);
    if (!aux1 || !aux2) die("oom");
    memcpy(aux1, Wiso, AUX_BYTES);
    memcpy(aux2, Wiso, AUX_SPLIT);
    {
        uint8_t *tail = build_ghost_bytes(D, dn, AUX_BYTES - AUX_SPLIT, AUXTAIL_SEED);
        memcpy(aux2 + AUX_SPLIT, tail, AUX_BYTES - AUX_SPLIT);
        free(tail);
    }
    {
        FILE *o = open_out("W_iso.bin"); fwrite(Wiso, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_plain.bin"); fwrite(D, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_ghost.bin"); fwrite(Wgh, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_half.bin"); fwrite(Whalf, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_ff.bin"); fwrite(Wff, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_swap_ea.bin"); fwrite(Wswap, 1, RUN_BYTES, o); fclose(o);
        o = open_out("W_aux1.bin"); fwrite(aux1, 1, AUX_BYTES, o); fclose(o);
        o = open_out("W_aux2.bin"); fwrite(aux2, 1, AUX_BYTES, o); fclose(o);
        o = open_out("oracle_cipher.tsv");
        for (int i = 0; i < 256; i++) fprintf(o, "%d\t%d\n", i, cipher_pi[i]);
        fclose(o);
    }
    /* ghost raw invariants */
    {
        double cD[256] = {0}, cG[256] = {0};
        for (size_t i = 0; i < dn; i++) cD[D[i]]++;
        for (size_t i = 0; i < dn; i++) cG[Wgh[i]]++;
        double l1 = 0;
        for (int i = 0; i < 256; i++) l1 += fabs(cD[i] / (double)dn - cG[i] / (double)dn);
        static double jj[256][256];
        memset(jj, 0, sizeof(jj));
        for (size_t i = 0; i + 1 < dn; i++) jj[Wgh[i]][Wgh[i + 1]]++;
        double mi = 0, tot = (double)(dn - 1);
        for (int a2 = 0; a2 < 256; a2++)
            for (int b2 = 0; b2 < 256; b2++) {
                if (jj[a2][b2] == 0) continue;
                double pab = jj[a2][b2] / tot;
                double pa = cG[a2] / (double)dn, pb = cG[b2] / (double)dn;
                mi += pab * log2(pab / (pa * pb));
            }
        FILE *o = open_out("ghost_invariants.tsv");
        fprintf(o, "raw_unigram_L1\t%.6f\nraw_bigram_MI\t%.6f\n", l1, mi);
        fclose(o);
        fprintf(stderr, "ghost raw: L1 %.6f | MI %.6f\n", l1, mi);
    }
    /* A1 fixture + A3 exactness probe */
    {
        Model M; memset(&M, 0, sizeof(M));
        M.stream = NULL; M.tri = NULL; M.bi = NULL; M.pool = NULL;
        model_build(&M, (const uint8_t *)"", 0);
        double p = price_local(&M, UINT32_MAX, UINT32_MAX, 'x');
        FILE *o = open_out("fixtures.tsv");
        fprintf(o, "F1_empty_floor\t%.17g\texpected\t%.17g\texact\t%d\n",
                p, 1.0 / 256.0, p == 1.0 / 256.0);
        fclose(o);
        fprintf(stderr, "A3 empty floor: P=%.17g exact==1/256: %d\n", p, p == 1.0 / 256.0);
        free(M.stream); free(M.tri); free(M.bi); free(M.pool);
    }

    uint8_t identity_map[256];
    for (int i = 0; i < 256; i++) identity_map[i] = (uint8_t)i;
    uint8_t swap_map[256];
    memcpy(swap_map, identity_map, sizeof(swap_map));
    swap_map[(uint8_t)'a'] = (uint8_t)'e';
    swap_map[(uint8_t)'e'] = (uint8_t)'a';
    double Gi[NARMS][5], Gp[NARMS][5], Gg[NARMS][5], Gh[NARMS][5];
    double Gf[NARMS][5], Gs[NARMS][5], Ga1[NARMS][5], Ga2[NARMS][5];
    double Gn[NARMS][5];
    int swap_surface = 0;
    fprintf(stderr, "past: A-train %zu B\n", atrain);
    run_court("iso", Wiso, dn, cipher_pi, 0, 0, Gi, RUN_BYTES, NULL);
    run_court("plain", D, dn, identity_map, 0, 0, Gp, RUN_BYTES, NULL);
    run_court("ghost", Wgh, dn, NULL, 0, 0, Gg, RUN_BYTES, NULL);
    run_court("half", Whalf, RUN_BYTES, cipher_pi, 0, 0, Gh, RUN_BYTES, NULL);
    run_court("ff", Wff, ffn, identity_map, 0, 0, Gf, RUN_BYTES, NULL);
    run_court("swap_ea", Wswap, dn, swap_map, 1, 0, Gs, RUN_BYTES,
              &swap_surface);
    run_court("aux1", aux1, AUX_BYTES, cipher_pi, 0, 0,
              Ga1, AUX_BYTES, NULL);
    run_court("aux2", aux2, AUX_BYTES, cipher_pi, 0, 0,
              Ga2, AUX_BYTES, NULL);
    run_court("neutralB", Wiso, dn, cipher_pi, 0, 1,
              Gn, RUN_BYTES, NULL);
    {
        FILE *o = open_out("neutral_B_law.tsv");
        fprintf(o, "world\tbytes\tarms_compared\texact\n");
        fprintf(o, "iso\t%d\t%d\t1\n", RUN_BYTES, NULL_K);
        fclose(o);
    }

    fprintf(stderr, "builder sanity G_N bits (NOT results) [1K,4K,16K,64K,full]:\n");
    const char *wn[6] = {"iso", "plain", "ghost", "half", "ff", "swap_ea"};
    double (*GG[6])[5] = {Gi, Gp, Gg, Gh, Gf, Gs};
    for (int w = 0; w < 6; w++)
        for (int a = 1; a < NARMS; a++) {
            if (a == ARM_ORACLE && w == 2) continue;
            fprintf(stderr, "  %s %s: %.1f %.1f %.1f %.1f %.1f\n",
                    wn[w], ARM_NAMES[a], GG[w][a][0], GG[w][a][1],
                    GG[w][a][2], GG[w][a][3], GG[w][a][4]);
        }
    fprintf(stderr, "transfer4 development artifacts in %s | swap surface %d\n",
            outdir, swap_surface);
    free(A); free(D); free(Wiso); free(Wgh); free(Wff); free(Wswap);
    free(Whalf); free(aux1); free(aux2);
    free(us2k); free(us3k); free(us4k);
    return 0;
}
