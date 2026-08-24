/* netta.c -- NETTA Body 0 builder, under PROTOCOL.md (frozen).
   Builder only: eats a world, splits 90/10, grows units on train,
   builds n-gram tables and the Hebbian field, prices the held-out
   test under five arms, speaks, and emits raw evidence artifacts.
   It grades nothing: every result number belongs to the independent
   verifier. C11, stdlib only. Deterministic.

   Arms: a = raw byte trigram; b = unit trigram no field;
         c = unit trigram + field; e = unit trigram + permuted field;
         d = word trigram sanity baseline.

   Frozen constants live in PROTOCOL.md; duplicated here as macros. */

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MERGES 2048
#define MIN_PAIR 4
#define BASE_UNITS 256
#define MAX_UNITS (BASE_UNITS + MERGES)
#define PACK 21 /* id bits in packed n-gram keys, all arms */
#define PACK_MASK ((1u << PACK) - 1u)
#define HEBB_BITS 21
#define HEBB_SIZE (1u << HEBB_BITS)
#define HEBB_WINDOW 8
#define FIELD_K 4
#define EPSILON 0.1
#define TEMP 0.8
#define TOP_K 15
#define REP_WINDOW 12
#define REP_PENALTY 0.5
#define SPEAK_BYTES 700
#define SPEAK_HARD 256
#define PERM_SEED 0xC0FFEEull
static const uint64_t SPEAK_SEEDS[5] = {7, 19, 42, 101, 271};
static double BETA = 0.3; /* --beta overrides for the equivalence probe */

static void die(const char *m) { fprintf(stderr, "netta: %s\n", m); exit(1); }

/* ── rng (frozen xorshift64) ── */
static uint64_t rng_state;
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return rng_state = x;
}
static size_t rng_below(size_t n) { return (size_t)(rng_next() % (uint64_t)n); }
static double rng_double(void) { return (double)(rng_next() >> 11) / (double)(1ull << 53); }

/* ── world ── */
static uint8_t *world;
static size_t world_n, train_n, test_n;
static const uint8_t *train_bytes, *test_bytes;

static void read_world(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open world");
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (n < 100) die("world too small");
    world_n = (size_t)n;
    world = malloc(world_n);
    if (!world || fread(world, 1, world_n, f) != world_n) die("cannot read world");
    fclose(f);
    train_n = (world_n * 9) / 10;
    test_n = world_n - train_n;
    train_bytes = world;
    test_bytes = world + train_n;
}

/* ── units: frozen BPE ── */
static uint32_t merge_left[MERGES], merge_right[MERGES];
static uint32_t nmerges;
static uint8_t *exp_pool;
static size_t exp_pool_cap, exp_pool_len;
static size_t exp_off[MAX_UNITS];
static uint32_t exp_len[MAX_UNITS];
static uint32_t nunits;

static void exp_append(uint32_t id, const uint8_t *b, uint32_t len) {
    if (exp_pool_len + len > exp_pool_cap) {
        exp_pool_cap = exp_pool_cap ? exp_pool_cap * 2 : (1u << 20);
        exp_pool = realloc(exp_pool, exp_pool_cap);
        if (!exp_pool) die("oom pool");
    }
    exp_off[id] = exp_pool_len;
    exp_len[id] = len;
    memcpy(exp_pool + exp_pool_len, b, len);
    exp_pool_len += len;
}

#define PH_BITS 19
#define PH_SIZE (1u << PH_BITS)
static uint64_t ph_key[PH_SIZE];
static uint32_t ph_cnt[PH_SIZE];
static uint32_t ph_used[PH_SIZE];
static uint32_t ph_used_n;

static int merge_round(uint32_t *t, size_t *pn) {
    size_t n = *pn;
    ph_used_n = 0;
    for (size_t i = 0; i + 1 < n; i++) {
        uint64_t key = ((uint64_t)t[i] << 32) | t[i + 1];
        uint32_t h = (uint32_t)((key * 0x9E3779B97F4A7C15ull) >> (64 - PH_BITS));
        for (;;) {
            if (ph_cnt[h] == 0) { ph_key[h] = key; ph_cnt[h] = 1; ph_used[ph_used_n++] = h; break; }
            if (ph_key[h] == key) { ph_cnt[h]++; break; }
            h = (h + 1) & (PH_SIZE - 1);
        }
    }
    uint64_t best_key = UINT64_MAX;
    uint32_t best_cnt = 0;
    for (uint32_t u = 0; u < ph_used_n; u++) {
        uint32_t h = ph_used[u];
        if (ph_cnt[h] > best_cnt || (ph_cnt[h] == best_cnt && ph_key[h] < best_key)) {
            best_cnt = ph_cnt[h];
            best_key = ph_key[h];
        }
    }
    for (uint32_t u = 0; u < ph_used_n; u++) ph_cnt[ph_used[u]] = 0;
    if (best_cnt < MIN_PAIR) return 0;

    uint32_t a = (uint32_t)(best_key >> 32), b = (uint32_t)best_key;
    uint32_t id = nunits++;
    merge_left[nmerges] = a;
    merge_right[nmerges] = b;
    nmerges++;
    {
        uint32_t la = exp_len[a], lb = exp_len[b];
        uint8_t *tmp = malloc((size_t)la + lb);
        if (!tmp) die("oom");
        memcpy(tmp, exp_pool + exp_off[a], la);
        memcpy(tmp + la, exp_pool + exp_off[b], lb);
        exp_append(id, tmp, la + lb);
        free(tmp);
    }
    size_t w = 0;
    for (size_t i = 0; i < n; ) {
        if (i + 1 < n && t[i] == a && t[i + 1] == b) { t[w++] = id; i += 2; }
        else t[w++] = t[i++];
    }
    *pn = w;
    return 1;
}

/* replay merges in creation order over an arbitrary byte stream */
static uint32_t *segment(const uint8_t *bytes, size_t n, size_t *out_n) {
    uint32_t *t = malloc(n * sizeof(uint32_t));
    if (!t) die("oom");
    size_t tn = n;
    for (size_t i = 0; i < n; i++) t[i] = bytes[i];
    for (uint32_t m = 0; m < nmerges; m++) {
        uint32_t a = merge_left[m], b = merge_right[m], id = BASE_UNITS + m;
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

/* ── field ── */
static uint64_t hebb_key[HEBB_SIZE];
static double hebb_val[HEBB_SIZE];

static uint32_t hebb_find(uint32_t a, uint32_t b) {
    uint32_t ma = a < b ? a : b, mb = a < b ? b : a;
    uint64_t key = ((uint64_t)ma << PACK) | mb;
    uint32_t h = (uint32_t)((key * 0x9E3779B97F4A7C15ull) >> (64 - HEBB_BITS));
    for (;;) {
        if (hebb_val[h] == 0.0) { hebb_key[h] = key; return h; }
        if (hebb_key[h] == key) return h;
        h = (h + 1) & (HEBB_SIZE - 1);
    }
}
static void build_field(const uint32_t *t, size_t n) {
    for (size_t i = 0; i < n; i++) {
        size_t hi = i + HEBB_WINDOW + 1;
        if (hi > n) hi = n;
        for (size_t j = i + 1; j < hi; j++)
            hebb_val[hebb_find(t[i], t[j])] += 1.0 / (1.0 + (double)(j - i));
    }
    double mx = 0;
    for (size_t i = 0; i < HEBB_SIZE; i++) if (hebb_val[i] > mx) mx = hebb_val[i];
    if (mx > 0) for (size_t i = 0; i < HEBB_SIZE; i++) hebb_val[i] /= mx;
}
static double H(uint32_t a, uint32_t b) {
    uint32_t ma = a < b ? a : b, mb = a < b ? b : a;
    uint64_t key = ((uint64_t)ma << PACK) | mb;
    uint32_t h = (uint32_t)((key * 0x9E3779B97F4A7C15ull) >> (64 - HEBB_BITS));
    for (;;) {
        if (hebb_val[h] == 0.0) return 0.0;
        if (hebb_key[h] == key) return hebb_val[h];
        h = (h + 1) & (HEBB_SIZE - 1);
    }
}

/* frozen Fisher-Yates permutation of [0,V) */
static uint32_t *perm;
static void build_perm(uint32_t V) {
    perm = malloc(V * sizeof(uint32_t));
    if (!perm) die("oom");
    for (uint32_t i = 0; i < V; i++) perm[i] = i;
    rng_state = PERM_SEED;
    for (uint32_t i = V - 1; i >= 1; i--) {
        uint32_t j = (uint32_t)(rng_next() % (uint64_t)(i + 1));
        uint32_t tmp = perm[i]; perm[i] = perm[j]; perm[j] = tmp;
    }
}

/* fieldmode: 0 none, 1 field, 2 permuted field */
static double field_factor(uint32_t u, const uint32_t *past, size_t npast, int fieldmode) {
    if (fieldmode == 0) return 1.0;
    double f = 1.0;
    for (size_t j = 0; j < npast; j++) {
        double s = (fieldmode == 1) ? H(u, past[j]) : H(perm[u], perm[past[j]]);
        f *= 1.0 + BETA * s;
    }
    return f;
}

/* ── one pricing engine, five arms ── */
typedef struct {
    const char *name;
    uint32_t *train, *test;
    size_t ntrain, ntest;
    uint32_t V;
    int fieldmode;
    uint64_t *tri, *bi;      /* sorted packed train keys */
    size_t ntri, nbi;
    uint32_t *n1;            /* size V */
    uint64_t n1_total;
    uint32_t *alive; size_t nalive;
    uint32_t oov_id;         /* arm d only; UINT32_MAX otherwise */
    uint32_t *tok_bytelen;   /* byte length per test token (for offsets) */
} Arm;

static int cmp_u64(const void *x, const void *y) {
    uint64_t a = *(const uint64_t *)x, b = *(const uint64_t *)y;
    return (a > b) - (a < b);
}

static void arm_tables(Arm *A) {
    size_t n = A->ntrain;
    A->ntri = n >= 3 ? n - 2 : 0;
    A->nbi = n >= 2 ? n - 1 : 0;
    A->tri = malloc(A->ntri * sizeof(uint64_t));
    A->bi = malloc(A->nbi * sizeof(uint64_t));
    A->n1 = calloc(A->V, sizeof(uint32_t));
    if ((A->ntri && !A->tri) || (A->nbi && !A->bi) || !A->n1) die("oom tables");
    for (size_t i = 0; i < A->ntri; i++)
        A->tri[i] = ((uint64_t)A->train[i] << (2 * PACK)) |
                    ((uint64_t)A->train[i + 1] << PACK) | A->train[i + 2];
    for (size_t i = 0; i < A->nbi; i++)
        A->bi[i] = ((uint64_t)A->train[i] << PACK) | A->train[i + 1];
    qsort(A->tri, A->ntri, sizeof(uint64_t), cmp_u64);
    qsort(A->bi, A->nbi, sizeof(uint64_t), cmp_u64);
    for (size_t i = 0; i < n; i++) A->n1[A->train[i]]++;
    A->n1_total = n;
    A->alive = malloc(A->V * sizeof(uint32_t));
    A->nalive = 0;
    for (uint32_t u = 0; u < A->V; u++) if (A->n1[u]) A->alive[A->nalive++] = u;
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

/* collect unique continuations (lowest PACK bits) with counts from a sorted range */
static size_t collect(const uint64_t *keys, size_t lo, size_t hi) {
    size_t n = 0, i = lo;
    while (i < hi && n < MAX_CAND) {
        uint32_t tok = (uint32_t)(keys[i] & PACK_MASK);
        size_t j = i;
        while (j < hi && (uint32_t)(keys[j] & PACK_MASK) == tok) j++;
        ccbuf[n].tok = tok;
        ccbuf[n].cnt = (uint32_t)(j - i);
        n++;
        i = j;
    }
    return n;
}

/* P(truth | context) under the frozen law; returns level used */
static int price(const Arm *A, size_t pos, double *out_p) {
    uint32_t truth = A->test[pos];
    uint32_t past[FIELD_K];
    size_t npast = 0;
    for (size_t j = pos; j > 0 && npast < FIELD_K; )
        past[npast++] = A->test[--j];

    /* P1 */
    double sum1, w1t;
    if (A->fieldmode == 0) {
        sum1 = (double)A->n1_total;
        w1t = (truth < A->V) ? (double)A->n1[truth] : 0.0;
    } else {
        sum1 = 0;
        for (size_t k = 0; k < A->nalive; k++) {
            uint32_t u = A->alive[k];
            sum1 += (double)A->n1[u] * field_factor(u, past, npast, A->fieldmode);
        }
        w1t = (truth < A->V && A->n1[truth])
                  ? (double)A->n1[truth] * field_factor(truth, past, npast, A->fieldmode)
                  : 0.0;
    }
    double p1 = (1.0 - EPSILON) * (sum1 > 0 ? w1t / sum1 : 0.0) + EPSILON / (double)A->V;
    if (pos == 0) { *out_p = p1; return 1; }

    /* P2 */
    uint32_t c2 = A->test[pos - 1];
    double p2 = p1;
    int lvl = 1;
    if (c2 < A->V) {
        size_t lo, hi;
        key_range(A->bi, A->nbi, c2, PACK, &lo, &hi);
        if (hi > lo) {
            size_t nc = collect(A->bi, lo, hi);
            double sum2 = 0, w2t = 0;
            for (size_t k = 0; k < nc; k++) {
                double w = (double)ccbuf[k].cnt * field_factor(ccbuf[k].tok, past, npast, A->fieldmode);
                sum2 += w;
                if (ccbuf[k].tok == truth) w2t = w;
            }
            p2 = (1.0 - EPSILON) * (sum2 > 0 ? w2t / sum2 : 0.0) + EPSILON * p1;
            lvl = 2;
        }
    }
    if (pos == 1) { *out_p = p2; return lvl; }

    /* P3 */
    uint32_t c1 = A->test[pos - 2];
    double p = p2;
    if (c1 < A->V && c2 < A->V) {
        uint64_t ctx = ((uint64_t)c1 << PACK) | c2;
        size_t lo, hi;
        key_range(A->tri, A->ntri, ctx, PACK, &lo, &hi);
        if (hi > lo) {
            size_t nc = collect(A->tri, lo, hi);
            double sum3 = 0, w3t = 0;
            for (size_t k = 0; k < nc; k++) {
                double w = (double)ccbuf[k].cnt * field_factor(ccbuf[k].tok, past, npast, A->fieldmode);
                sum3 += w;
                if (ccbuf[k].tok == truth) w3t = w;
            }
            p = (1.0 - EPSILON) * (sum3 > 0 ? w3t / sum3 : 0.0) + EPSILON * p2;
            lvl = 3;
        }
    }
    *out_p = p;
    return lvl;
}

/* ── evidence ── */
static char outdir[1024];
static FILE *open_out(const char *name) {
    char path[1200];
    snprintf(path, sizeof(path), "%s/%s", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot open artifact for writing");
    return f;
}

static double run_evidence(const Arm *A) {
    char fname[128];
    snprintf(fname, sizeof(fname), "evidence_%s.tsv", A->name);
    FILE *f = open_out(fname);
    double bits = 0;
    size_t off = 0;
    for (size_t pos = 0; pos < A->ntest; pos++) {
        uint32_t truth = A->test[pos];
        uint32_t blen = A->tok_bytelen[pos];
        double p;
        int lvl;
        if (A->oov_id != UINT32_MAX && truth == A->oov_id) {
            p = pow(256.0, -(double)blen); /* frozen word-escape */
            lvl = 0;
        } else {
            lvl = price(A, pos, &p);
        }
        bits += -log2(p);
        fprintf(f, "%zu\t%zu\t%u\t%u\t%d\t%.17g\n", pos, off, blen, truth, lvl, p);
        off += blen;
    }
    fclose(f);
    return bits / (double)test_n;
}

/* ── speech (unit arms only) ── */
static int starts_upper(uint32_t id) {
    uint8_t c = exp_pool[exp_off[id]];
    return c >= 'A' && c <= 'Z';
}
static int ends_sentence(uint32_t id) {
    uint8_t c = exp_pool[exp_off[id] + exp_len[id] - 1];
    return c == '.' || c == '!' || c == '?' || c == '\n';
}

static void speak(const Arm *A, uint64_t seed, const char *tag) {
    char fname[128];
    snprintf(fname, sizeof(fname), "speech_%s_%llu.bin", tag, (unsigned long long)seed);
    FILE *f = open_out(fname);
    rng_state = seed ^ 0x9E3779B97F4A7C15ull;
    if (!rng_state) rng_state = 1;

    size_t nstarts = 0;
    size_t *starts = malloc(A->ntrain * sizeof(size_t));
    if (!starts) die("oom");
    for (size_t i = 0; i + 1 < A->ntrain; i++)
        if (starts_upper(A->train[i]) && (i == 0 || ends_sentence(A->train[i - 1])))
            starts[nstarts++] = i;
    if (nstarts == 0) die("no sentence starts");
    size_t sp = starts[rng_below(nstarts)];
    free(starts);

    uint32_t em[8192];
    size_t nem = 0, ebytes = 0;
    em[nem++] = A->train[sp];
    em[nem++] = A->train[sp + 1];
    fwrite(exp_pool + exp_off[em[0]], 1, exp_len[em[0]], f);
    fwrite(exp_pool + exp_off[em[1]], 1, exp_len[em[1]], f);
    ebytes = exp_len[em[0]] + exp_len[em[1]];

    size_t want = SPEAK_BYTES, hard = SPEAK_BYTES + SPEAK_HARD;
    while (ebytes < want && nem + 1 < 8192) {
        uint32_t u1 = em[nem - 2], u2 = em[nem - 1];
        uint32_t past[FIELD_K];
        size_t npast = 0;
        for (size_t j = nem; j > 0 && npast < FIELD_K; ) past[npast++] = em[--j];

        size_t lo, hi, nc = 0;
        uint64_t ctx = ((uint64_t)u1 << PACK) | u2;
        key_range(A->tri, A->ntri, ctx, PACK, &lo, &hi);
        if (hi > lo) nc = collect(A->tri, lo, hi);
        if (nc == 0) {
            key_range(A->bi, A->nbi, u2, PACK, &lo, &hi);
            if (hi > lo) nc = collect(A->bi, lo, hi);
        }
        if (nc == 0) {
            for (size_t k = 0; k < A->nalive && k < MAX_CAND; k++) {
                ccbuf[k].tok = A->alive[k];
                ccbuf[k].cnt = A->n1[A->alive[k]];
            }
            nc = A->nalive < MAX_CAND ? A->nalive : MAX_CAND;
        }

        typedef struct { uint32_t tok; double s; } SC;
        static SC sc[MAX_CAND];
        for (size_t k = 0; k < nc; k++) {
            double w = (double)ccbuf[k].cnt * field_factor(ccbuf[k].tok, past, npast, A->fieldmode);
            int freq = 0;
            size_t rst = nem > REP_WINDOW ? nem - REP_WINDOW : 0;
            for (size_t j = rst; j < nem; j++) if (em[j] == ccbuf[k].tok) freq++;
            sc[k].tok = ccbuf[k].tok;
            sc[k].s = log(w + 1e-300) - log(1.0 + REP_PENALTY * (double)freq);
        }
        size_t limit = nc < TOP_K ? nc : TOP_K;
        for (size_t i = 0; i < limit; i++) {
            size_t best = i;
            for (size_t j = i + 1; j < nc; j++) if (sc[j].s > sc[best].s) best = j;
            if (best != i) { SC t = sc[i]; sc[i] = sc[best]; sc[best] = t; }
        }
        double ls[TOP_K], mx = -1e300, tot = 0;
        for (size_t i = 0; i < limit; i++) { ls[i] = sc[i].s / TEMP; if (ls[i] > mx) mx = ls[i]; }
        for (size_t i = 0; i < limit; i++) { ls[i] = exp(ls[i] - mx); tot += ls[i]; }
        double r = rng_double() * tot, cum = 0;
        uint32_t chosen = sc[0].tok;
        for (size_t i = 0; i < limit; i++) { cum += ls[i]; if (cum > r) { chosen = sc[i].tok; break; } }

        em[nem++] = chosen;
        fwrite(exp_pool + exp_off[chosen], 1, exp_len[chosen], f);
        ebytes += exp_len[chosen];
        if (ebytes >= want && !ends_sentence(chosen) && ebytes < hard) want = ebytes + 1;
    }
    fclose(f);
}

/* ── word tokenizer (arm d) ── */
static int is_wordbyte(uint8_t c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '\'';
}
#define WH_BITS 18
#define WH_SIZE (1u << WH_BITS)
static struct { char *s; uint32_t len, id; } wh[WH_SIZE];
static uint32_t nwords;

static uint32_t word_lookup(const uint8_t *s, uint32_t len, int create) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t i = 0; i < len; i++) h = (h ^ s[i]) * 0x100000001b3ull;
    uint32_t slot = (uint32_t)(h >> (64 - WH_BITS));
    for (;;) {
        if (!wh[slot].s) {
            if (!create) return UINT32_MAX;
            wh[slot].s = malloc(len);
            memcpy(wh[slot].s, s, len);
            wh[slot].len = len;
            wh[slot].id = BASE_UNITS + nwords++;
            return wh[slot].id;
        }
        if (wh[slot].len == len && !memcmp(wh[slot].s, s, len)) return wh[slot].id;
        slot = (slot + 1) & (WH_SIZE - 1);
    }
}
/* tokens: single non-word bytes keep id=byte; words get 256+k; OOV -> oov_id */
static uint32_t *word_tokenize(const uint8_t *b, size_t n, int create, uint32_t oov_id,
                               size_t *out_n, uint32_t **out_lens) {
    uint32_t *t = malloc(n * sizeof(uint32_t));
    uint32_t *lens = malloc(n * sizeof(uint32_t));
    if (!t || !lens) die("oom");
    size_t tn = 0, i = 0;
    while (i < n) {
        if (is_wordbyte(b[i])) {
            size_t j = i;
            while (j < n && is_wordbyte(b[j])) j++;
            uint32_t id = word_lookup(b + i, (uint32_t)(j - i), create);
            t[tn] = (id == UINT32_MAX) ? oov_id : id;
            lens[tn] = (uint32_t)(j - i);
            tn++;
            i = j;
        } else {
            t[tn] = b[i];
            lens[tn] = 1;
            tn++;
            i++;
        }
    }
    *out_n = tn;
    *out_lens = lens;
    return t;
}

/* ── artifact writers ── */
static void write_merges(void) {
    FILE *f = open_out("merges.tsv");
    for (uint32_t m = 0; m < nmerges; m++)
        fprintf(f, "%u\t%u\t%u\n", m, merge_left[m], merge_right[m]);
    fclose(f);
}
static void write_train_tokens(const uint32_t *t, size_t n) {
    FILE *f = open_out("train_tokens.u32");
    for (size_t i = 0; i < n; i++) {
        uint8_t le[4] = { (uint8_t)t[i], (uint8_t)(t[i] >> 8), (uint8_t)(t[i] >> 16), (uint8_t)(t[i] >> 24) };
        fwrite(le, 1, 4, f);
    }
    fclose(f);
}

int main(int argc, char **argv) {
    const char *path = NULL;
    outdir[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(outdir, sizeof(outdir), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--beta") && i + 1 < argc) BETA = atof(argv[++i]);
        else path = argv[i];
    }
    if (!path || !outdir[0]) die("usage: netta <world> --out <dir> [--beta X]");

    read_world(path);

    /* units on train */
    uint32_t *utrain = malloc(train_n * sizeof(uint32_t));
    if (!utrain) die("oom");
    size_t un = train_n;
    for (size_t i = 0; i < train_n; i++) utrain[i] = train_bytes[i];
    nunits = BASE_UNITS;
    for (uint32_t i = 0; i < BASE_UNITS; i++) { uint8_t b = (uint8_t)i; exp_append(i, &b, 1); }
    while (nmerges < MERGES && merge_round(utrain, &un)) {}
    write_merges();
    write_train_tokens(utrain, un);

    size_t utest_n;
    uint32_t *utest = segment(test_bytes, test_n, &utest_n);
    build_field(utrain, un);
    build_perm(nunits);

    uint32_t *ulens = malloc(utest_n * sizeof(uint32_t));
    if (!ulens) die("oom");
    for (size_t i = 0; i < utest_n; i++) ulens[i] = exp_len[utest[i]];

    /* byte streams for arm a */
    uint32_t *btrain = malloc(train_n * sizeof(uint32_t));
    uint32_t *btest = malloc(test_n * sizeof(uint32_t));
    uint32_t *blens = malloc(test_n * sizeof(uint32_t));
    if (!btrain || !btest || !blens) die("oom");
    for (size_t i = 0; i < train_n; i++) btrain[i] = train_bytes[i];
    for (size_t i = 0; i < test_n; i++) { btest[i] = test_bytes[i]; blens[i] = 1; }

    /* word streams for arm d */
    size_t wtrain_n, wtest_n;
    uint32_t *wtrain_lens, *wtest_lens;
    uint32_t *wtrain = word_tokenize(train_bytes, train_n, 1, 0, &wtrain_n, &wtrain_lens);
    uint32_t wV = BASE_UNITS + nwords;
    uint32_t oov = wV; /* reserved id outside inventory */
    uint32_t *wtest = word_tokenize(test_bytes, test_n, 0, oov, &wtest_n, &wtest_lens);

    Arm arm_a = {"a", btrain, btest, train_n, test_n, 256, 0, 0,0,0,0, 0,0,0,0, UINT32_MAX, blens};
    Arm arm_b = {"b", utrain, utest, un, utest_n, nunits, 0, 0,0,0,0, 0,0,0,0, UINT32_MAX, ulens};
    Arm arm_c = {"c", utrain, utest, un, utest_n, nunits, 1, 0,0,0,0, 0,0,0,0, UINT32_MAX, ulens};
    Arm arm_e = {"e", utrain, utest, un, utest_n, nunits, 2, 0,0,0,0, 0,0,0,0, UINT32_MAX, ulens};
    Arm arm_d = {"d", wtrain, wtest, wtrain_n, wtest_n, wV, 0, 0,0,0,0, 0,0,0,0, oov, wtest_lens};

    arm_tables(&arm_a);
    arm_tables(&arm_b);
    arm_c.tri = arm_b.tri; arm_c.bi = arm_b.bi; arm_c.ntri = arm_b.ntri; arm_c.nbi = arm_b.nbi;
    arm_c.n1 = arm_b.n1; arm_c.n1_total = arm_b.n1_total; arm_c.alive = arm_b.alive; arm_c.nalive = arm_b.nalive;
    arm_e.tri = arm_b.tri; arm_e.bi = arm_b.bi; arm_e.ntri = arm_b.ntri; arm_e.nbi = arm_b.nbi;
    arm_e.n1 = arm_b.n1; arm_e.n1_total = arm_b.n1_total; arm_e.alive = arm_b.alive; arm_e.nalive = arm_b.nalive;
    arm_tables(&arm_d);

    fprintf(stderr, "netta: world %zu B | train %zu | test %zu | merges %u | units %u "
                    "(stream %zu, avg %.2f B/unit) | words %u\n",
            world_n, train_n, test_n, nmerges, nunits, un,
            (double)train_n / (double)un, nwords);

    double bb_a = run_evidence(&arm_a);
    double bb_b = run_evidence(&arm_b);
    double bb_c = run_evidence(&arm_c);
    double bb_e = run_evidence(&arm_e);
    double bb_d = run_evidence(&arm_d);
    fprintf(stderr, "builder sanity (NOT results): bits/byte a=%.4f b=%.4f c=%.4f e=%.4f d=%.4f\n",
            bb_a, bb_b, bb_c, bb_e, bb_d);

    for (int s = 0; s < 5; s++) {
        speak(&arm_c, SPEAK_SEEDS[s], "c");
        speak(&arm_b, SPEAK_SEEDS[s], "b");
    }

    fprintf(stderr, "netta: artifacts written to %s\n", outdir);
    return 0;
}
