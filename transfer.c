/* transfer.c -- NETTA transfer court builder, under TRANSFER_PROTOCOL.md
   (frozen, SHA-256 230030d7...). Builder hand only: constructs the
   destination worlds with known truth, grows the traveller's past on
   A-train, runs the prequential adaptation court for all arms, and
   emits raw evidence. It grades nothing; the independent verifier owns
   every result. C11, stdlib only. Deterministic.

   Arms: 0 cold, 1 cache, 2 align, 3 both, 4 shuffled, 5 oracle(control).
   The dead field of Body 0 is not here and nothing resurrects it:
   transfer memory = unit streams + renaming-invariant descriptors. */

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
#define WARMUP 16384
#define TOP_M 256
#define Z_GATE 2.0
#define EARN_BITS 32.0
#define REVOKE_BITS 16.0
#define L_ENTER 0.05
#define L_LO 0.01
#define L_HI 0.5
#define ETA 0.05
#define NDESC 12
#define CIPHER_SEED 0xC1F3E5ull
#define GHOST_SEED 0x5EED01ull
#define SHUF_SEED 0x54AFF1Eull
#define NARMS 6
static const char *ARM_NAMES[NARMS] = {"cold","cache","align","both","shuf","oracle"};

static void die(const char *m) { fprintf(stderr, "transfer: %s\n", m); exit(1); }

static uint64_t rng_state;
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return rng_state = x;
}

/* ───────── generic BPE model (frozen v2 tie-break: first appearance) ───────── */
typedef struct {
    uint32_t merge_l[MERGES], merge_r[MERGES];
    uint32_t nmerges, nunits;
    uint8_t *pool; size_t pool_len, pool_cap;
    size_t exp_off[MAX_UNITS];
    uint32_t exp_len[MAX_UNITS];
    uint32_t *stream; size_t nstream;      /* lived stream tokens */
    uint64_t *tri, *bi; size_t ntri, nbi;  /* sorted packed keys */
    uint32_t n1[MAX_UNITS];
    uint64_t n1_total;
    uint32_t alive[MAX_UNITS]; size_t nalive;
} Model;

static void model_exp_append(Model *M, uint32_t id, const uint8_t *b, uint32_t len) {
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

static int model_merge_round(Model *M) {
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
    uint32_t best_cnt = 0;
    size_t best_first = (size_t)-1;
    uint64_t best_key = 0;
    for (uint32_t u = 0; u < ph_used_n; u++) {
        uint32_t h = ph_used[u];
        if (ph_cnt[h] > best_cnt ||
            (ph_cnt[h] == best_cnt && ph_first[h] < best_first)) {
            best_cnt = ph_cnt[h]; best_first = ph_first[h]; best_key = ph_key[h];
        }
    }
    for (uint32_t u = 0; u < ph_used_n; u++) ph_cnt[ph_used[u]] = 0;
    if (best_cnt < MIN_PAIR) return 0;

    uint32_t a = (uint32_t)(best_key >> 32), b = (uint32_t)best_key;
    uint32_t id = M->nunits++;
    M->merge_l[M->nmerges] = a; M->merge_r[M->nmerges] = b; M->nmerges++;
    {
        uint32_t la = M->exp_len[a], lb = M->exp_len[b];
        uint8_t *tmp = malloc((size_t)la + lb);
        if (!tmp) die("oom");
        memcpy(tmp, M->pool + M->exp_off[a], la);
        memcpy(tmp + la, M->pool + M->exp_off[b], lb);
        model_exp_append(M, id, tmp, la + lb);
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

static int cmp_u64(const void *x, const void *y) {
    uint64_t a = *(const uint64_t *)x, b = *(const uint64_t *)y;
    return (a > b) - (a < b);
}

static void model_build(Model *M, const uint8_t *bytes, size_t n) {
    free(M->stream); free(M->tri); free(M->bi); free(M->pool);
    memset(M, 0, sizeof(*M));
    M->stream = malloc((n ? n : 1) * sizeof(uint32_t));
    if (!M->stream) die("oom");
    M->nstream = n;
    for (size_t i = 0; i < n; i++) M->stream[i] = bytes[i];
    M->nunits = BASE_UNITS;
    for (uint32_t i = 0; i < BASE_UNITS; i++) { uint8_t bb = (uint8_t)i; model_exp_append(M, i, &bb, 1); }
    if (n) while (M->nmerges < MERGES && model_merge_round(M)) {}
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
    M->nalive = 0;
    for (uint32_t u = 0; u < M->nunits; u++) if (M->n1[u]) M->alive[M->nalive++] = u;
}

/* segment arbitrary bytes with a model's merges (replay, greedy L2R) */
static uint32_t *model_segment(const Model *M, const uint8_t *bytes, size_t n, size_t *out_n) {
    uint32_t *t = malloc((n ? n : 1) * sizeof(uint32_t));
    if (!t) die("oom");
    size_t tn = n;
    for (size_t i = 0; i < n; i++) t[i] = bytes[i];
    for (uint32_t m = 0; m < M->nmerges; m++) {
        uint32_t a = M->merge_l[m], b = M->merge_r[m], id = BASE_UNITS + m;
        size_t w = 0;
        for (size_t i = 0; i < tn; ) {
            if (i + 1 < tn && t[i] == a && t[i + 1] == b) { t[w++] = id; i += 2; }
            else t[w++] = t[i++];
        }
        tn = w;
    }
    *out_n = tn;
    return t;
}

static void key_range(const uint64_t *keys, size_t n, uint64_t prefix, unsigned shift,
                      size_t *lo_out, size_t *hi_out) {
    uint64_t lo_key = prefix << shift, hi_key = (prefix + 1) << shift;
    size_t lo = 0, hi = n;
    while (lo < hi) { size_t m = lo + (hi - lo) / 2; if (keys[m] < lo_key) lo = m + 1; else hi = m; }
    *lo_out = lo;
    size_t lo2 = lo, hi2 = n;
    while (lo2 < hi2) { size_t m = lo2 + (hi2 - lo2) / 2; if (keys[m] < hi_key) lo2 = m + 1; else hi2 = m; }
    *hi_out = lo2;
}

typedef struct { uint32_t tok; uint32_t cnt; } CC;
#define MAX_CAND 65536
static CC ccbuf[MAX_CAND];
static size_t collect(const uint64_t *keys, size_t lo, size_t hi) {
    size_t n = 0, i = lo;
    while (i < hi && n < MAX_CAND) {
        uint32_t tok = (uint32_t)(keys[i] & PACK_MASK);
        size_t j = i;
        while (j < hi && (uint32_t)(keys[j] & PACK_MASK) == tok) j++;
        ccbuf[n].tok = tok; ccbuf[n].cnt = (uint32_t)(j - i); n++;
        i = j;
    }
    return n;
}

/* P_local under the Body 0 chain, no field. ctx c1,c2 may be UINT32_MAX (absent). */
static double price_local(const Model *M, uint32_t c1, uint32_t c2, uint32_t truth) {
    double p1;
    if (M->n1_total > 0)
        p1 = (1.0 - EPSILON) * ((double)M->n1[truth] / (double)M->n1_total)
             + EPSILON / (double)M->nunits;
    else
        p1 = EPSILON / (double)M->nunits + 0.0; /* empty world: escape floor only */
    if (c2 == UINT32_MAX) return p1;
    double p2 = p1;
    {
        size_t lo, hi;
        key_range(M->bi, M->nbi, c2, PACK, &lo, &hi);
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
        key_range(M->tri, M->ntri, ctx, PACK, &lo, &hi);
        if (hi > lo) {
            size_t nc = collect(M->tri, lo, hi);
            double sum = 0, wt = 0;
            for (size_t k = 0; k < nc; k++) { sum += ccbuf[k].cnt; if (ccbuf[k].tok == truth) wt = ccbuf[k].cnt; }
            p = (1.0 - EPSILON) * (wt / sum) + EPSILON * p2;
        }
    }
    return p;
}

/* candidate set = deepest supported level (tri else bi else alive) */
static size_t candidates(const Model *M, uint32_t c1, uint32_t c2, CC *out) {
    if (c1 != UINT32_MAX && c2 != UINT32_MAX) {
        uint64_t ctx = ((uint64_t)c1 << PACK) | c2;
        size_t lo, hi;
        key_range(M->tri, M->ntri, ctx, PACK, &lo, &hi);
        if (hi > lo) { size_t n = collect(M->tri, lo, hi); memcpy(out, ccbuf, n * sizeof(CC)); return n; }
    }
    if (c2 != UINT32_MAX) {
        size_t lo, hi;
        key_range(M->bi, M->nbi, c2, PACK, &lo, &hi);
        if (hi > lo) { size_t n = collect(M->bi, lo, hi); memcpy(out, ccbuf, n * sizeof(CC)); return n; }
    }
    size_t n = M->nalive < MAX_CAND ? M->nalive : MAX_CAND;
    for (size_t k = 0; k < n; k++) { out[k].tok = M->alive[k]; out[k].cnt = M->n1[M->alive[k]]; }
    return n;
}

/* ───────── descriptors: 12 renaming-invariant scalars, z-scored per world ───────── */
typedef struct {
    double raw[MAX_UNITS][NDESC];
    double z[MAX_UNITS][NDESC];
    int have[MAX_UNITS];
} Desc;

static void desc_build(Desc *D, const Model *M) {
    memset(D->have, 0, sizeof(D->have));
    const uint32_t *t = M->stream;
    size_t n = M->nstream;
    for (size_t a = 0; a < M->nalive; a++) {
        uint32_t u = M->alive[a];
        double *d = D->raw[u];
        memset(d, 0, NDESC * sizeof(double));
        d[0] = log2((double)M->n1[u]);
        d[1] = (double)M->exp_len[u];
        /* right stats from bi range */
        size_t lo, hi;
        key_range(M->bi, M->nbi, u, PACK, &lo, &hi);
        size_t nc = collect(M->bi, lo, hi);
        double tot = 0, ent = 0, top1 = 0, t4[4] = {0,0,0,0};
        for (size_t k = 0; k < nc; k++) tot += ccbuf[k].cnt;
        for (size_t k = 0; k < nc; k++) {
            double p = ccbuf[k].cnt / tot;
            ent += -p * log2(p);
            if (p > t4[0]) { t4[3]=t4[2]; t4[2]=t4[1]; t4[1]=t4[0]; t4[0]=p; }
            else if (p > t4[1]) { t4[3]=t4[2]; t4[2]=t4[1]; t4[1]=p; }
            else if (p > t4[2]) { t4[3]=t4[2]; t4[2]=p; }
            else if (p > t4[3]) t4[3]=p;
        }
        top1 = t4[0];
        d[2] = nc ? ent : 0;
        d[4] = log2((double)(nc ? nc : 1));
        d[6] = top1;
        d[7] = t4[0]+t4[1]+t4[2]+t4[3];
        D->have[u] = 1;
    }
    /* left stats: one pass building left counts per unit via reversed bi */
    {
        static uint32_t lcnt_deg[MAX_UNITS];
        static double lent[MAX_UNITS];
        memset(lcnt_deg, 0, sizeof(lcnt_deg));
        memset(lent, 0, sizeof(lent));
        /* build left-sorted keys: (right<<PACK)|left */
        uint64_t *lb = malloc((M->nbi ? M->nbi : 1) * sizeof(uint64_t));
        if (!lb) die("oom");
        for (size_t i = 0; i + 1 < n + 0 && i < M->nbi; i++) ;
        size_t m = 0;
        for (size_t i = 0; i + 1 < n; i++)
            lb[m++] = ((uint64_t)t[i + 1] << PACK) | t[i];
        qsort(lb, m, sizeof(uint64_t), cmp_u64);
        for (size_t a = 0; a < M->nalive; a++) {
            uint32_t u = M->alive[a];
            size_t lo, hi;
            key_range(lb, m, u, PACK, &lo, &hi);
            size_t nc = collect(lb, lo, hi);
            double tot = 0, ent = 0;
            for (size_t k = 0; k < nc; k++) tot += ccbuf[k].cnt;
            for (size_t k = 0; k < nc; k++) { double p = ccbuf[k].cnt / tot; ent += -p * log2(p); }
            D->raw[u][3] = nc ? ent : 0;
            D->raw[u][5] = log2((double)(nc ? nc : 1));
        }
        free(lb);
    }
    /* partner concentration at exact distances 1,2,4 (Simpson index) */
    for (int di = 0; di < 3; di++) {
        int d = (di == 0) ? 1 : (di == 1) ? 2 : 4;
        uint64_t *keys = malloc((n ? n : 1) * sizeof(uint64_t));
        if (!keys) die("oom");
        size_t m = 0;
        for (size_t i = 0; i + (size_t)d < n; i++)
            keys[m++] = ((uint64_t)t[i] << PACK) | t[i + d];
        qsort(keys, m, sizeof(uint64_t), cmp_u64);
        for (size_t a = 0; a < M->nalive; a++) {
            uint32_t u = M->alive[a];
            size_t lo, hi;
            key_range(keys, m, u, PACK, &lo, &hi);
            size_t nc = collect(keys, lo, hi);
            double tot = 0, simp = 0;
            for (size_t k = 0; k < nc; k++) tot += ccbuf[k].cnt;
            if (tot > 0) for (size_t k = 0; k < nc; k++) { double p = ccbuf[k].cnt / tot; simp += p * p; }
            D->raw[u][8 + di] = simp;
        }
        free(keys);
    }
    /* one refinement step: prob-weighted mean of right neighbours' right-entropy */
    for (size_t a = 0; a < M->nalive; a++) {
        uint32_t u = M->alive[a];
        size_t lo, hi;
        key_range(M->bi, M->nbi, u, PACK, &lo, &hi);
        size_t nc = collect(M->bi, lo, hi);
        double tot = 0, acc = 0;
        for (size_t k = 0; k < nc; k++) tot += ccbuf[k].cnt;
        for (size_t k = 0; k < nc; k++)
            acc += (ccbuf[k].cnt / tot) * D->raw[ccbuf[k].tok][2];
        D->raw[u][11] = nc ? acc : 0;
    }
    /* z-score per dimension over alive units */
    for (int dd = 0; dd < NDESC; dd++) {
        double mean = 0, var = 0;
        for (size_t a = 0; a < M->nalive; a++) mean += D->raw[M->alive[a]][dd];
        mean /= (double)M->nalive;
        for (size_t a = 0; a < M->nalive; a++) {
            double v = D->raw[M->alive[a]][dd] - mean;
            var += v * v;
        }
        double sd = sqrt(var / (double)M->nalive);
        for (size_t a = 0; a < M->nalive; a++) {
            uint32_t u = M->alive[a];
            D->z[u][dd] = sd > 1e-9 ? (D->raw[u][dd] - mean) / sd : 0.0;
        }
    }
}

static double zdist(const double *x, const double *y) {
    double s = 0;
    for (int i = 0; i < NDESC; i++) { double v = x[i] - y[i]; s += v * v; }
    return sqrt(s);
}

/* ───────── the traveller's past (source A) ───────── */
static Model A;           /* built on A-train */
static Desc DA;
static uint32_t A_top[TOP_M]; size_t A_ntop;
static uint32_t A_top4[MAX_UNITS][4]; uint32_t A_top4n[MAX_UNITS];
static uint32_t A_shuf[MAX_UNITS];   /* row permutation for the shuffled arm */

/* expansion hash for oracle lookup */
#define EH_BITS 13
#define EH_SIZE (1u << EH_BITS)
static struct { uint32_t id; int used; } eh[EH_SIZE];
static uint64_t exp_hash(const uint8_t *s, uint32_t len) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t i = 0; i < len; i++) h = (h ^ s[i]) * 0x100000001b3ull;
    return h;
}
static void eh_build(void) {
    memset(eh, 0, sizeof(eh));
    for (uint32_t u = 0; u < A.nunits; u++) {
        if (!A.n1[u]) continue;
        uint32_t slot = (uint32_t)(exp_hash(A.pool + A.exp_off[u], A.exp_len[u]) >> (64 - EH_BITS));
        for (;;) {
            if (!eh[slot].used) { eh[slot].id = u; eh[slot].used = 1; break; }
            uint32_t v = eh[slot].id;
            if (A.exp_len[v] == A.exp_len[u] &&
                !memcmp(A.pool + A.exp_off[v], A.pool + A.exp_off[u], A.exp_len[u])) break;
            slot = (slot + 1) & (EH_SIZE - 1);
        }
    }
}
static uint32_t eh_find(const uint8_t *s, uint32_t len) {
    uint32_t slot = (uint32_t)(exp_hash(s, len) >> (64 - EH_BITS));
    for (;;) {
        if (!eh[slot].used) return UINT32_MAX;
        uint32_t v = eh[slot].id;
        if (A.exp_len[v] == len && !memcmp(A.pool + A.exp_off[v], s, len)) return v;
        slot = (slot + 1) & (EH_SIZE - 1);
    }
}

/* A-side lookups with optional shuffled indirection */
static uint32_t aside(uint32_t u, int shuf) { return shuf ? A_shuf[u] : u; }
/* continuation count a->c in A (bi range) */
static double A_cont(uint32_t a, uint32_t c) {
    size_t lo, hi;
    key_range(A.bi, A.nbi, a, PACK, &lo, &hi);
    size_t nc = collect(A.bi, lo, hi);
    for (size_t k = 0; k < nc; k++) if (ccbuf[k].tok == c) return (double)ccbuf[k].cnt;
    return 0;
}

/* ───────── worlds ───────── */
static uint8_t *worldA; static size_t worldA_n, trainA_n;
static uint8_t cipher_pi[256], cipher_inv[256];

static uint8_t *build_iso(void) {
    for (int i = 0; i < 256; i++) cipher_pi[i] = (uint8_t)i;
    rng_state = CIPHER_SEED;
    for (int i = 255; i >= 1; i--) {
        int j = (int)(rng_next() % (uint64_t)(i + 1));
        uint8_t tmp = cipher_pi[i]; cipher_pi[i] = cipher_pi[j]; cipher_pi[j] = tmp;
    }
    for (int i = 0; i < 256; i++) cipher_inv[cipher_pi[i]] = (uint8_t)i;
    uint8_t *w = malloc(worldA_n);
    if (!w) die("oom");
    for (size_t i = 0; i < worldA_n; i++) w[i] = cipher_pi[worldA[i]];
    return w;
}
static uint8_t *build_ghost(void) {
    uint64_t cnt[256] = {0};
    for (size_t i = 0; i < trainA_n; i++) cnt[worldA[i]]++;
    uint64_t cum[256], tot = 0;
    for (int i = 0; i < 256; i++) { tot += cnt[i]; cum[i] = tot; }
    uint8_t *w = malloc(worldA_n);
    if (!w) die("oom");
    rng_state = GHOST_SEED;
    for (size_t i = 0; i < worldA_n; i++) {
        uint64_t r = rng_next() % tot;
        int lo = 0, hi = 255;
        while (lo < hi) { int m = (lo + hi) / 2; if (cum[m] <= r) lo = m + 1; else hi = m; }
        w[i] = (uint8_t)lo;
    }
    return w;
}
/* 16 most frequent whitespace-delimited words of A-train, swapped by rank pairs */
static uint8_t ff_words[16][64]; static uint32_t ff_wlen[16];
static int is_ws(uint8_t c) { return c == ' ' || c == '\n' || c == '\r' || c == '\t'; }
static uint8_t *build_ff(size_t *out_n) {
    /* count words (len<=63) */
    enum { WH = 1 << 16 };
    static struct { uint8_t s[64]; uint32_t len; uint64_t cnt; size_t first; int used; } wh[WH];
    memset(wh, 0, sizeof(wh));
    size_t i = 0;
    while (i < trainA_n) {
        if (is_ws(worldA[i])) { i++; continue; }
        size_t j = i;
        while (j < trainA_n && !is_ws(worldA[j])) j++;
        uint32_t len = (uint32_t)(j - i);
        if (len < 64) {
            uint32_t slot = (uint32_t)(exp_hash(worldA + i, len) >> (64 - 16));
            for (;;) {
                if (!wh[slot].used) {
                    memcpy(wh[slot].s, worldA + i, len);
                    wh[slot].len = len; wh[slot].cnt = 1; wh[slot].first = i; wh[slot].used = 1;
                    break;
                }
                if (wh[slot].len == len && !memcmp(wh[slot].s, worldA + i, len)) { wh[slot].cnt++; break; }
                slot = (slot + 1) & (WH - 1);
            }
        }
        i = j;
    }
    /* top 16 by count, tie -> earlier first occurrence */
    int picked[16];
    for (int k = 0; k < 16; k++) {
        int best = -1;
        for (int s = 0; s < WH; s++) {
            if (!wh[s].used || wh[s].cnt == 0) continue;
            int dup = 0;
            for (int q = 0; q < k; q++) if (picked[q] == s) dup = 1;
            if (dup) continue;
            if (best < 0 || wh[s].cnt > wh[best].cnt ||
                (wh[s].cnt == wh[best].cnt && wh[s].first < wh[best].first)) best = s;
        }
        picked[k] = best;
        memcpy(ff_words[k], wh[best].s, wh[best].len);
        ff_wlen[k] = wh[best].len;
    }
    /* rewrite whole A with rank-pair swaps 0<->1, 2<->3, ... */
    uint8_t *w = malloc(worldA_n * 2);
    if (!w) die("oom");
    size_t o = 0;
    i = 0;
    while (i < worldA_n) {
        if (is_ws(worldA[i])) { w[o++] = worldA[i++]; continue; }
        size_t j = i;
        while (j < worldA_n && !is_ws(worldA[j])) j++;
        uint32_t len = (uint32_t)(j - i);
        int hit = -1;
        for (int k = 0; k < 16; k++)
            if (ff_wlen[k] == len && !memcmp(ff_words[k], worldA + i, len)) { hit = k; break; }
        if (hit >= 0) {
            int partner = (hit % 2 == 0) ? hit + 1 : hit - 1;
            memcpy(w + o, ff_words[partner], ff_wlen[partner]);
            o += ff_wlen[partner];
        } else {
            memcpy(w + o, worldA + i, len);
            o += len;
        }
        i = j;
    }
    *out_n = o;
    return w;
}

/* ───────── the court ───────── */
static char outdir[1024];
static FILE *open_out(const char *name) {
    char path[1200];
    snprintf(path, sizeof(path), "%s/%s", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot open artifact");
    return f;
}

typedef struct {
    int state;        /* 0 shadow, 1 live */
    double L, ledger;
} ArmState;

/* per-world alignment: map B-unit -> A-unit (UINT32_MAX = none) */
static uint32_t align_map[MAX_UNITS];
static uint32_t oracle_map[MAX_UNITS];

static void compute_alignment(const Model *B, const Desc *DB, int shuf, FILE *af, size_t chunk_no) {
    for (uint32_t u = 0; u < MAX_UNITS; u++) align_map[u] = UINT32_MAX;
    /* top-M of B by freq (tie -> smaller id) */
    uint32_t btop[TOP_M]; size_t nb = 0;
    for (size_t k = 0; k < (size_t)TOP_M; k++) {
        uint32_t best = UINT32_MAX;
        for (size_t a = 0; a < B->nalive; a++) {
            uint32_t u = B->alive[a];
            int dup = 0;
            for (size_t q = 0; q < nb; q++) if (btop[q] == u) dup = 1;
            if (dup) continue;
            if (best == UINT32_MAX || B->n1[u] > B->n1[best]) best = u;
        }
        if (best == UINT32_MAX) break;
        btop[nb++] = best;
    }
    static double dist[TOP_M][TOP_M];
    static int rowdone[TOP_M], coldone[TOP_M];
    memset(rowdone, 0, sizeof(rowdone));
    memset(coldone, 0, sizeof(coldone));
    for (size_t x = 0; x < A_ntop; x++)
        for (size_t y = 0; y < nb; y++)
            dist[x][y] = zdist(DA.z[aside(A_top[x], shuf)], DB->z[btop[y]]);
    for (size_t it = 0; it < nb && it < A_ntop; it++) {
        double best = 1e300; int bx = -1, by = -1;
        for (size_t x = 0; x < A_ntop; x++) {
            if (rowdone[x]) continue;
            for (size_t y = 0; y < nb; y++) {
                if (coldone[y]) continue;
                if (dist[x][y] < best) { best = dist[x][y]; bx = (int)x; by = (int)y; }
            }
        }
        if (bx < 0 || best > Z_GATE) break;
        rowdone[bx] = 1; coldone[by] = 1;
        align_map[btop[by]] = A_top[bx]; /* note: shuffle applies at READ time via aside() */
        if (af) fprintf(af, "%zu\t%u\t%u\t%.6f\n", chunk_no, btop[by], A_top[bx], best);
    }
}

/* nearest A unit to a B descriptor (memoized per epoch) */
static uint32_t cache_match[MAX_UNITS];
static void cache_match_reset(void) { memset(cache_match, 0xFF, sizeof(cache_match)); }
static uint32_t nearest_A(const Desc *DB, uint32_t bu, int shuf) {
    if (cache_match[bu] != UINT32_MAX) return cache_match[bu];
    double best = 1e300; uint32_t ba = 0;
    for (size_t a = 0; a < A.nalive; a++) {
        uint32_t u = A.alive[a];
        double d = zdist(DA.z[aside(u, shuf)], DB->z[bu]);
        if (d < best) { best = d; ba = u; }
    }
    cache_match[bu] = ba;
    return ba;
}

/* priors over a candidate set; returns 1 if defined */
static int prior_cache(const Desc *DB, uint32_t c2, const CC *cand, size_t nc,
                       double *out, int shuf, const int *bhave) {
    if (c2 == UINT32_MAX || !bhave[c2]) return 0;
    uint32_t a = nearest_A(DB, c2, shuf);
    uint32_t am = aside(a, shuf);
    if (!A_top4n[am]) return 0;
    double mx = -1e300;
    static double r[MAX_CAND];
    for (size_t k = 0; k < nc; k++) {
        if (!bhave[cand[k].tok]) { r[k] = -1e300; continue; }
        double m = 1e300;
        for (uint32_t j = 0; j < A_top4n[am]; j++) {
            double d = zdist(DB->z[cand[k].tok], DA.z[aside(A_top4[am][j], shuf)]);
            if (d < m) m = d;
        }
        r[k] = -m;
        if (r[k] > mx) mx = r[k];
    }
    if (mx <= -1e300) return 0;
    double tot = 0;
    for (size_t k = 0; k < nc; k++) { out[k] = r[k] <= -1e300 ? 0 : exp(r[k] - mx); tot += out[k]; }
    for (size_t k = 0; k < nc; k++) out[k] /= tot;
    return 1;
}
static int prior_align(uint32_t c2, const CC *cand, size_t nc, double *out,
                       int shuf, int oracle) {
    const uint32_t *map = oracle ? oracle_map : align_map;
    if (c2 == UINT32_MAX || map[c2] == UINT32_MAX) return 0;
    uint32_t ac = aside(map[c2], oracle ? 0 : shuf);
    double tot = 0;
    for (size_t k = 0; k < nc; k++) {
        uint32_t am = map[cand[k].tok];
        out[k] = (am == UINT32_MAX) ? 0 : A_cont(ac, aside(am, oracle ? 0 : shuf));
        tot += out[k];
    }
    if (tot <= 0) return 0;
    for (size_t k = 0; k < nc; k++) out[k] /= tot;
    return 1;
}

static void run_court(const char *wname, const uint8_t *W, size_t Wn, int has_oracle,
                      int oracle_kind /*1=iso,2=ff*/, double G[NARMS][4]) {
    char fname[128];
    snprintf(fname, sizeof(fname), "transfer_evidence_%s.tsv", wname);
    FILE *ef = open_out(fname);
    snprintf(fname, sizeof(fname), "alignments_%s.tsv", wname);
    FILE *af = open_out(fname);

    Model B; memset(&B, 0, sizeof(B));
    B.stream = NULL; B.tri = NULL; B.bi = NULL; B.pool = NULL;
    static Desc DB;
    ArmState st[NARMS];
    for (int a = 0; a < NARMS; a++) { st[a].state = 0; st[a].L = 0; st[a].ledger = 0; }
    const size_t horizons[4] = {1024, 4096, 16384, 65536};
    for (int a = 0; a < NARMS; a++) for (int h = 0; h < 4; h++) G[a][h] = 0;

    model_build(&B, W, 0); /* empty start */
    desc_build(&DB, &B);
    cache_match_reset();
    int aligned_ready = 0;

    size_t lived = 0, chunk_no = 0;
    static uint32_t ptoks_last2[2];
    while (lived < Wn) {
        size_t clen = Wn - lived < CHUNK ? Wn - lived : CHUNK;
        /* tokenize prefix and prefix+chunk under CURRENT merges */
        size_t np, npc;
        uint32_t *tp = model_segment(&B, W, lived, &np);
        uint32_t *tpc = model_segment(&B, W, lived + clen, &npc);
        (void)ptoks_last2;
        double bits[NARMS] = {0};
        size_t undef[NARMS] = {0};
        size_t positions = npc - np;
        /* oracle map for current B inventory */
        if (has_oracle) {
            for (uint32_t u = 0; u < B.nunits; u++) {
                oracle_map[u] = UINT32_MAX;
                if (!B.n1[u]) continue;
                uint8_t tmp[512];
                uint32_t len = B.exp_len[u];
                if (len > 512) continue;
                if (oracle_kind == 1) {
                    for (uint32_t i2 = 0; i2 < len; i2++) tmp[i2] = cipher_inv[B.pool[B.exp_off[u] + i2]];
                } else {
                    memcpy(tmp, B.pool + B.exp_off[u], len);
                    for (int k = 0; k < 16; k++)
                        if (ff_wlen[k] == len && !memcmp(ff_words[k], tmp, len)) {
                            int partner = (k % 2 == 0) ? k + 1 : k - 1;
                            len = ff_wlen[partner];
                            memcpy(tmp, ff_words[partner], len);
                            break;
                        }
                }
                oracle_map[u] = eh_find(tmp, len);
            }
        }
        for (size_t pos = np; pos < npc; pos++) {
            uint32_t truth = tpc[pos];
            uint32_t c2 = pos >= 1 ? tpc[pos - 1] : UINT32_MAX;
            uint32_t c1 = pos >= 2 ? tpc[pos - 2] : UINT32_MAX;
            double pl = price_local(&B, c1, c2, truth);
            double ll = -log2(pl);
            static CC cand[MAX_CAND];
            size_t nc = candidates(&B, c1, c2, cand);
            static double pc_[MAX_CAND], pa_[MAX_CAND], pr[MAX_CAND];
            int has_c = prior_cache(&DB, c2, cand, nc, pc_, 0, DB.have);
            int has_a = aligned_ready ? prior_align(c2, cand, nc, pa_, 0, 0) : 0;
            int has_cS = prior_cache(&DB, c2, cand, nc, pr, 1, DB.have); /* shuffled cache into pr */
            static double psh[MAX_CAND];
            if (has_cS) memcpy(psh, pr, nc * sizeof(double));
            int has_aS = aligned_ready ? prior_align(c2, cand, nc, pr, 1, 0) : 0;
            static double pshA[MAX_CAND];
            if (has_aS) memcpy(pshA, pr, nc * sizeof(double));
            int has_o = has_oracle && aligned_ready ? prior_align(c2, cand, nc, pr, 0, 1) : 0;
            static double por[MAX_CAND];
            if (has_o) memcpy(por, pr, nc * sizeof(double));

            for (int arm = 0; arm < NARMS; arm++) {
                if (arm == 5 && !has_oracle) continue;
                double lp = ll; /* prior price defaults neutral */
                int defined = 0;
                double praw[MAX_CAND];
                if (arm == 1 && has_c) { memcpy(praw, pc_, nc * sizeof(double)); defined = 1; }
                else if (arm == 2 && has_a) { memcpy(praw, pa_, nc * sizeof(double)); defined = 1; }
                else if (arm == 3 && (has_c || has_a)) {
                    for (size_t k = 0; k < nc; k++)
                        praw[k] = has_c && has_a ? 0.5 * pc_[k] + 0.5 * pa_[k]
                                  : has_c ? pc_[k] : pa_[k];
                    defined = 1;
                }
                else if (arm == 4 && (has_cS || has_aS)) {
                    for (size_t k = 0; k < nc; k++)
                        praw[k] = has_cS && has_aS ? 0.5 * psh[k] + 0.5 * pshA[k]
                                  : has_cS ? psh[k] : pshA[k];
                    defined = 1;
                }
                else if (arm == 5 && has_o) { memcpy(praw, por, nc * sizeof(double)); defined = 1; }

                double pfin = pl;
                if (arm > 0) {
                    ArmState *S = &st[arm];
                    if (defined) {
                        double ptruth_raw = 0;
                        for (size_t k = 0; k < nc; k++) if (cand[k].tok == truth) { ptruth_raw = praw[k]; break; }
                        double pp = 0.9 * ptruth_raw + 0.1 * pl; /* frozen escape */
                        lp = -log2(pp);
                        S->ledger += ll - lp;
                        if (S->state == 1) {
                            pfin = (1.0 - S->L) * pl + S->L * pp;
                            double lnew = S->L * exp(ETA * (ll - lp));
                            S->L = lnew < L_LO ? L_LO : lnew > L_HI ? L_HI : lnew;
                        }
                        /* earn / revoke */
                        if (S->state == 0 && S->ledger >= EARN_BITS) { S->state = 1; S->L = L_ENTER; }
                        else if (S->state == 1 && S->ledger < REVOKE_BITS) { S->state = 0; S->L = 0; }
                    } else {
                        undef[arm]++;
                        /* neutral: ledger unchanged, pfin = pl (even when live) */
                    }
                }
                bits[arm] += -log2(pfin);
            }
        }
        /* evidence rows */
        for (int arm = 0; arm < NARMS; arm++) {
            if (arm == 5 && !has_oracle) continue;
            fprintf(ef, "%zu\t%zu\t%zu\t%s\t%zu\t%.17g\t%.6f\t%.6f\t%d\t%zu\n",
                    chunk_no, lived, clen, ARM_NAMES[arm], positions, bits[arm],
                    st[arm].L, st[arm].ledger, st[arm].state, undef[arm]);
            for (int h = 0; h < 4; h++)
                if (lived + clen <= horizons[h]) G[arm][h] += bits[0] - bits[arm];
        }
        free(tp); free(tpc);
        lived += clen;
        chunk_no++;
        /* absorb: rebuild model, descriptors, matches, alignment */
        model_build(&B, W, lived);
        desc_build(&DB, &B);
        cache_match_reset();
        if (lived >= WARMUP) {
            compute_alignment(&B, &DB, 0, af, chunk_no);
            aligned_ready = 1;
        }
    }
    /* consumed-representation stats for the ghost note: unit unigram entropy + bigram MI */
    {
        double H1 = 0, MI = 0;
        if (B.nstream >= 2) {
            for (size_t a = 0; a < B.nalive; a++) {
                double p = (double)B.n1[B.alive[a]] / (double)B.n1_total;
                H1 += -p * log2(p);
            }
            size_t i = 0;
            while (i < B.nbi) {
                uint64_t k = B.bi[i];
                size_t j = i;
                while (j < B.nbi && B.bi[j] == k) j++;
                double pab = (double)(j - i) / (double)B.nbi;
                uint32_t ua = (uint32_t)(k >> PACK), ub = (uint32_t)(k & PACK_MASK);
                double pa = (double)B.n1[ua] / (double)B.n1_total;
                double pb = (double)B.n1[ub] / (double)B.n1_total;
                MI += pab * log2(pab / (pa * pb));
                i = j;
            }
        }
        fprintf(stderr, "court %s done: %zu chunks | final units %u stream %zu | unit H1 %.4f | unit bigram MI %.4f\n",
                wname, chunk_no, B.nunits, B.nstream, H1, MI);
    }
    fclose(ef); fclose(af);
}

int main(int argc, char **argv) {
    const char *apath = NULL;
    outdir[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(outdir, sizeof(outdir), "%s", argv[++i]);
        else apath = argv[i];
    }
    if (!apath || !outdir[0]) die("usage: transfer <worldA> --out <dir>");

    /* read A */
    FILE *f = fopen(apath, "rb");
    if (!f) die("cannot open A");
    fseek(f, 0, SEEK_END);
    worldA_n = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);
    worldA = malloc(worldA_n);
    if (!worldA || fread(worldA, 1, worldA_n, f) != worldA_n) die("read A");
    fclose(f);
    trainA_n = (worldA_n * 9) / 10;

    /* worlds */
    uint8_t *Wiso = build_iso();
    uint8_t *Wgh = build_ghost();
    size_t Wff_n;
    uint8_t *Wff = build_ff(&Wff_n);
    {
        FILE *o = open_out("W_iso.bin"); fwrite(Wiso, 1, worldA_n, o); fclose(o);
        o = open_out("W_ghost.bin"); fwrite(Wgh, 1, worldA_n, o); fclose(o);
        o = open_out("W_ff.bin"); fwrite(Wff, 1, Wff_n, o); fclose(o);
        o = open_out("oracle_cipher.tsv");
        for (int i = 0; i < 256; i++) fprintf(o, "%d\t%d\n", i, cipher_pi[i]);
        fclose(o);
        o = open_out("oracle_ff.tsv");
        for (int k = 0; k < 16; k += 2)
            fprintf(o, "%.*s\t%.*s\n", (int)ff_wlen[k], ff_words[k], (int)ff_wlen[k+1], ff_words[k+1]);
        fclose(o);
    }
    /* ghost raw invariants */
    {
        double cA[256] = {0}, cG[256] = {0};
        for (size_t i = 0; i < trainA_n; i++) cA[worldA[i]]++;
        for (size_t i = 0; i < worldA_n; i++) cG[Wgh[i]]++;
        double l1 = 0;
        for (int i = 0; i < 256; i++) l1 += fabs(cA[i] / (double)trainA_n - cG[i] / (double)worldA_n);
        static double jj[256][256];
        memset(jj, 0, sizeof(jj));
        for (size_t i = 0; i + 1 < worldA_n; i++) jj[Wgh[i]][Wgh[i + 1]]++;
        double mi = 0, tot = (double)(worldA_n - 1);
        for (int a = 0; a < 256; a++)
            for (int b = 0; b < 256; b++) {
                if (jj[a][b] == 0) continue;
                double pab = jj[a][b] / tot;
                double pa = cG[a] / (double)worldA_n, pb = cG[b] / (double)worldA_n;
                mi += pab * log2(pab / (pa * pb));
            }
        FILE *o = open_out("ghost_invariants.tsv");
        fprintf(o, "raw_unigram_L1\t%.6f\nraw_bigram_MI\t%.6f\n", l1, mi);
        fclose(o);
        fprintf(stderr, "ghost raw invariants: unigram L1 %.6f | bigram MI %.6f bits\n", l1, mi);
    }

    /* the traveller's past */
    memset(&A, 0, sizeof(A));
    model_build(&A, worldA, trainA_n);
    desc_build(&DA, &A);
    eh_build();
    /* top-M of A, top-4 continuations */
    A_ntop = 0;
    for (size_t k = 0; k < (size_t)TOP_M; k++) {
        uint32_t best = UINT32_MAX;
        for (size_t a = 0; a < A.nalive; a++) {
            uint32_t u = A.alive[a];
            int dup = 0;
            for (size_t q = 0; q < A_ntop; q++) if (A_top[q] == u) dup = 1;
            if (dup) continue;
            if (best == UINT32_MAX || A.n1[u] > A.n1[best]) best = u;
        }
        if (best == UINT32_MAX) break;
        A_top[A_ntop++] = best;
    }
    for (size_t a = 0; a < A.nalive; a++) {
        uint32_t u = A.alive[a];
        size_t lo, hi;
        key_range(A.bi, A.nbi, u, PACK, &lo, &hi);
        size_t nc = collect(A.bi, lo, hi);
        /* top-4 by count, tie -> smaller id */
        uint32_t ids[4] = {0,0,0,0}; uint32_t cnts[4] = {0,0,0,0}; uint32_t nn = 0;
        for (size_t k = 0; k < nc; k++) {
            uint32_t c = ccbuf[k].cnt, id2 = ccbuf[k].tok;
            for (int s = 0; s < 4; s++) {
                if (c > cnts[s] || (c == cnts[s] && s < (int)nn && id2 < ids[s])) {
                    for (int q = 3; q > s; q--) { cnts[q] = cnts[q-1]; ids[q] = ids[q-1]; }
                    cnts[s] = c; ids[s] = id2;
                    if (nn < 4) nn++;
                    break;
                }
            }
        }
        A_top4n[u] = nn;
        for (uint32_t s = 0; s < nn; s++) A_top4[u][s] = ids[s];
    }
    /* shuffled indirection over A inventory (alive units; identity elsewhere) */
    for (uint32_t u = 0; u < MAX_UNITS; u++) A_shuf[u] = u;
    {
        rng_state = SHUF_SEED;
        for (size_t i = A.nalive - 1; i >= 1; i--) {
            size_t j = (size_t)(rng_next() % (uint64_t)(i + 1));
            uint32_t tmp = A_shuf[A.alive[i]];
            A_shuf[A.alive[i]] = A_shuf[A.alive[j]];
            A_shuf[A.alive[j]] = tmp;
        }
    }
    fprintf(stderr, "past: A-train %zu B | merges %u | units %u | stream %zu | top %zu\n",
            trainA_n, A.nmerges, A.nunits, A.nstream, A_ntop);

    /* isomorphism check: units grown on cipher(A-train) must be pi(units of A-train) */
    {
        uint8_t *ct = malloc(trainA_n);
        if (!ct) die("oom");
        for (size_t i = 0; i < trainA_n; i++) ct[i] = cipher_pi[worldA[i]];
        Model C; memset(&C, 0, sizeof(C));
        model_build(&C, ct, trainA_n);
        uint32_t match = 0;
        uint32_t total = A.nmerges < C.nmerges ? A.nmerges : C.nmerges;
        for (uint32_t m = 0; m < total; m++) {
            uint32_t ua = BASE_UNITS + m, uc = BASE_UNITS + m;
            if (A.exp_len[ua] != C.exp_len[uc]) continue;
            int ok = 1;
            for (uint32_t i = 0; i < A.exp_len[ua]; i++)
                if (cipher_pi[A.pool[A.exp_off[ua] + i]] != C.pool[C.exp_off[uc] + i]) { ok = 0; break; }
            match += ok;
        }
        fprintf(stderr, "isomorphism: %u/%u merges correspond under pi (A merges %u, cipher merges %u)\n",
                match, total, A.nmerges, C.nmerges);
        FILE *o = open_out("isomorphism.tsv");
        fprintf(o, "matched\t%u\ntotal\t%u\nA_merges\t%u\ncipher_merges\t%u\n",
                match, total, A.nmerges, C.nmerges);
        fclose(o);
        free(C.stream); free(C.tri); free(C.bi); free(C.pool); free(ct);
    }

    /* courts */
    double Giso[NARMS][4], Ggh[NARMS][4], Gff[NARMS][4];
    run_court("iso", Wiso, worldA_n, 1, 1, Giso);
    run_court("ghost", Wgh, worldA_n, 0, 0, Ggh);
    run_court("ff", Wff, Wff_n, 1, 2, Gff);

    fprintf(stderr, "builder sanity G_N in bits (NOT results): [arm][1K,4K,16K,64K]\n");
    const char *wn[3] = {"iso","ghost","ff"};
    double (*GG[3])[4] = {Giso, Ggh, Gff};
    for (int w = 0; w < 3; w++)
        for (int a = 1; a < NARMS; a++) {
            if (a == 5 && w == 1) continue;
            fprintf(stderr, "  %s %s: %.1f %.1f %.1f %.1f\n", wn[w], ARM_NAMES[a],
                    GG[w][a][0], GG[w][a][1], GG[w][a][2], GG[w][a][3]);
        }
    fprintf(stderr, "transfer: artifacts in %s\n", outdir);
    return 0;
}
