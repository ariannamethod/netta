/*
 * netta_check.c - independent verifier for NETTA Body 0.
 *
 * Written from PROTOCOL.md alone. Never reads netta.c. Shares no code
 * or assumptions with the builder. Re-derives train segmentation from
 * merges.tsv, recounts every table from train bytes only, recomputes
 * every P(truth) under the frozen probability chain, checks Sigma=1
 * at every position, totals bits/byte per arm, and computes anti-copy
 * on the speech artifacts.
 *
 * Usage: netta_check <world.txt> <artifacts_dir> <results_out.tsv>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <math.h>

#define EPS 0.1
#define BETA 0.3
#define WINDOW 8
#define SENTINEL 0xFFFFFFFFu

/* ---------------- utility: file IO ---------------- */

static void die(const char *msg) {
    fprintf(stderr, "FATAL: %s\n", msg);
    exit(1);
}

static uint8_t *read_file(const char *path, size_t *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(1); }
    if (fseek(f, 0, SEEK_END) != 0) die("fseek failed");
    long sz = ftell(f);
    if (sz < 0) die("ftell failed");
    if (fseek(f, 0, SEEK_SET) != 0) die("fseek rewind failed");
    uint8_t *buf = malloc((size_t)sz > 0 ? (size_t)sz : 1);
    if (!buf) die("malloc failed");
    if (sz > 0) {
        size_t got = fread(buf, 1, (size_t)sz, f);
        if (got != (size_t)sz) { fprintf(stderr, "short read %s\n", path); exit(1); }
    }
    fclose(f);
    *out_len = (size_t)sz;
    return buf;
}

static uint32_t *load_u32le(const char *path, size_t *out_n) {
    size_t bytes;
    uint8_t *raw = read_file(path, &bytes);
    if (bytes % 4 != 0) die("u32 file not a multiple of 4 bytes");
    size_t n = bytes / 4;
    uint32_t *out = malloc((n > 0 ? n : 1) * sizeof(uint32_t));
    if (!out) die("malloc failed");
    for (size_t i = 0; i < n; i++) {
        out[i] = (uint32_t)raw[4*i] | ((uint32_t)raw[4*i+1] << 8) |
                 ((uint32_t)raw[4*i+2] << 16) | ((uint32_t)raw[4*i+3] << 24);
    }
    free(raw);
    *out_n = n;
    return out;
}

/* ---------------- BPE merges ---------------- */

typedef struct { uint32_t left, right; } Merge;

static Merge *load_merges(const char *path, size_t *out_n) {
    FILE *f = fopen(path, "r");
    if (!f) die("cannot open merges.tsv");
    size_t cap = 4096, n = 0;
    Merge *m = malloc(cap * sizeof(Merge));
    if (!m) die("malloc failed");
    size_t ordinal, left, right;
    while (fscanf(f, "%zu %zu %zu", &ordinal, &left, &right) == 3) {
        if (ordinal != n) die("merges.tsv ordinal out of creation order");
        if (n == cap) { cap *= 2; m = realloc(m, cap * sizeof(Merge)); if (!m) die("realloc failed"); }
        m[n].left = (uint32_t)left;
        m[n].right = (uint32_t)right;
        n++;
    }
    fclose(f);
    *out_n = n;
    return m;
}

/* ---------------- token stream with byte-span tracking ---------------- */

typedef struct {
    uint32_t *id;
    uint32_t *off;
    uint32_t *len;
    size_t n;
} TokStream;

/* replay merges left-to-right, greedy, non-overlapping, creation order */
static TokStream bpe_replay(const uint8_t *bytes, size_t nbytes, const Merge *merges, size_t nmerges) {
    size_t cap = nbytes > 0 ? nbytes : 1;
    uint32_t *id  = malloc(cap * sizeof(uint32_t));
    uint32_t *off = malloc(cap * sizeof(uint32_t));
    uint32_t *len = malloc(cap * sizeof(uint32_t));
    uint32_t *nid  = malloc(cap * sizeof(uint32_t));
    uint32_t *noff = malloc(cap * sizeof(uint32_t));
    uint32_t *nlen = malloc(cap * sizeof(uint32_t));
    if (!id || !off || !len || !nid || !noff || !nlen) die("malloc failed in bpe_replay");
    for (size_t i = 0; i < nbytes; i++) { id[i] = bytes[i]; off[i] = (uint32_t)i; len[i] = 1; }
    size_t cur_n = nbytes;
    for (size_t m = 0; m < nmerges; m++) {
        uint32_t newid = 256u + (uint32_t)m;
        uint32_t L = merges[m].left, R = merges[m].right;
        size_t out = 0, i = 0;
        while (i < cur_n) {
            if (i + 1 < cur_n && id[i] == L && id[i+1] == R) {
                nid[out] = newid;
                noff[out] = off[i];
                nlen[out] = len[i] + len[i+1];
                out++;
                i += 2;
            } else {
                nid[out] = id[i];
                noff[out] = off[i];
                nlen[out] = len[i];
                out++;
                i += 1;
            }
        }
        uint32_t *t;
        t = id; id = nid; nid = t;
        t = off; off = noff; noff = t;
        t = len; len = nlen; nlen = t;
        cur_n = out;
    }
    free(nid); free(noff); free(nlen);
    TokStream ts = { id, off, len, cur_n };
    return ts;
}

/* ---------------- word law ---------------- */

static int is_word_byte(uint8_t b) {
    return (b >= 'A' && b <= 'Z') || (b >= 'a' && b <= 'z') || b == '\'';
}

#define WD_TABLE_BITS 20u
#define WD_TABLE_SIZE (1u << WD_TABLE_BITS)
#define WD_TABLE_MASK (WD_TABLE_SIZE - 1u)

typedef struct {
    const uint8_t **wptr;
    uint32_t *wlen;
    size_t n, cap;
    int64_t *table;
} WordDict;

static uint64_t fnv1a(const uint8_t *p, size_t n) {
    uint64_t h = 14695981039346656037ULL;
    for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}

static WordDict *make_word_dict(void) {
    WordDict *d = malloc(sizeof(WordDict));
    if (!d) die("malloc failed");
    d->n = 0; d->cap = 4096;
    d->wptr = malloc(d->cap * sizeof(uint8_t *));
    d->wlen = malloc(d->cap * sizeof(uint32_t));
    d->table = malloc((size_t)WD_TABLE_SIZE * sizeof(int64_t));
    if (!d->wptr || !d->wlen || !d->table) die("malloc failed");
    for (size_t i = 0; i < WD_TABLE_SIZE; i++) d->table[i] = -1;
    return d;
}

/* returns dict index. create=1: insert on miss. create=0: -1 means OOV. */
static int64_t word_dict_lookup(WordDict *d, const uint8_t *bytes, uint32_t wlen, int create) {
    uint64_t h = fnv1a(bytes, wlen);
    uint32_t slot = (uint32_t)(h & WD_TABLE_MASK);
    for (;;) {
        int64_t idx = d->table[slot];
        if (idx == -1) {
            if (!create) return -1;
            if (d->n == d->cap) {
                d->cap *= 2;
                d->wptr = realloc(d->wptr, d->cap * sizeof(uint8_t *));
                d->wlen = realloc(d->wlen, d->cap * sizeof(uint32_t));
                if (!d->wptr || !d->wlen) die("realloc failed");
            }
            d->wptr[d->n] = bytes;
            d->wlen[d->n] = wlen;
            d->table[slot] = (int64_t)d->n;
            return (int64_t)(d->n++);
        }
        if (d->wlen[idx] == wlen && memcmp(d->wptr[idx], bytes, wlen) == 0) return idx;
        slot = (slot + 1u) & WD_TABLE_MASK;
    }
}

static TokStream word_scan(const uint8_t *bytes, size_t n, WordDict *d, int create) {
    size_t cap = n > 0 ? n : 1;
    uint32_t *id = malloc(cap * sizeof(uint32_t));
    uint32_t *off = malloc(cap * sizeof(uint32_t));
    uint32_t *len = malloc(cap * sizeof(uint32_t));
    if (!id || !off || !len) die("malloc failed in word_scan");
    size_t out = 0, i = 0;
    while (i < n) {
        if (is_word_byte(bytes[i])) {
            size_t j = i;
            while (j < n && is_word_byte(bytes[j])) j++;
            uint32_t wl = (uint32_t)(j - i);
            int64_t idx = word_dict_lookup(d, bytes + i, wl, create);
            id[out] = (idx < 0) ? SENTINEL : (uint32_t)(256 + idx);
            off[out] = (uint32_t)i;
            len[out] = wl;
            out++;
            i = j;
        } else {
            id[out] = bytes[i];
            off[out] = (uint32_t)i;
            len[out] = 1;
            out++;
            i++;
        }
    }
    TokStream ts = { id, off, len, out };
    return ts;
}

/* ---------------- n-gram tables (train-only) ---------------- */

typedef struct {
    uint64_t *N1;
    uint32_t *bg_prev, *bg_cur; uint64_t *bg_cnt; size_t bg_k; uint32_t *bg_offset;
    uint32_t *tg_prev2, *tg_prev1, *tg_cur; uint64_t *tg_cnt; size_t tg_k;
    uint32_t V;
} Tables;

static int cmp_pair(const void *a, const void *b) {
    const uint32_t *pa = a, *pb = b;
    if (pa[0] != pb[0]) return pa[0] < pb[0] ? -1 : 1;
    if (pa[1] != pb[1]) return pa[1] < pb[1] ? -1 : 1;
    return 0;
}
static int cmp_triple(const void *a, const void *b) {
    const uint32_t *pa = a, *pb = b;
    for (int i = 0; i < 3; i++) if (pa[i] != pb[i]) return pa[i] < pb[i] ? -1 : 1;
    return 0;
}

static Tables build_tables(const uint32_t *stream, size_t n, uint32_t V) {
    Tables T; memset(&T, 0, sizeof(T));
    T.V = V;
    T.N1 = calloc(V, sizeof(uint64_t));
    if (!T.N1) die("malloc failed");
    for (size_t i = 0; i < n; i++) {
        if (stream[i] >= V) die("token id out of range building N1");
        T.N1[stream[i]]++;
    }

    size_t nb = (n >= 1) ? n - 1 : 0;
    uint32_t (*bpairs)[2] = malloc((nb > 0 ? nb : 1) * sizeof(*bpairs));
    if (!bpairs) die("malloc failed");
    for (size_t i = 0; i < nb; i++) { bpairs[i][0] = stream[i]; bpairs[i][1] = stream[i+1]; }
    qsort(bpairs, nb, sizeof(*bpairs), cmp_pair);
    T.bg_prev = malloc((nb > 0 ? nb : 1) * sizeof(uint32_t));
    T.bg_cur  = malloc((nb > 0 ? nb : 1) * sizeof(uint32_t));
    T.bg_cnt  = malloc((nb > 0 ? nb : 1) * sizeof(uint64_t));
    if (!T.bg_prev || !T.bg_cur || !T.bg_cnt) die("malloc failed");
    size_t k = 0;
    for (size_t i = 0; i < nb; ) {
        size_t j = i;
        while (j < nb && bpairs[j][0] == bpairs[i][0] && bpairs[j][1] == bpairs[i][1]) j++;
        T.bg_prev[k] = bpairs[i][0]; T.bg_cur[k] = bpairs[i][1]; T.bg_cnt[k] = (uint64_t)(j - i);
        k++; i = j;
    }
    T.bg_k = k;
    free(bpairs);
    T.bg_offset = malloc(((size_t)V + 1) * sizeof(uint32_t));
    if (!T.bg_offset) die("malloc failed");
    {
        size_t idx = 0;
        for (uint32_t x = 0; x <= V; x++) {
            while (idx < k && T.bg_prev[idx] < x) idx++;
            T.bg_offset[x] = (uint32_t)idx;
        }
    }

    size_t nt = (n >= 2) ? n - 2 : 0;
    uint32_t (*tt)[3] = malloc((nt > 0 ? nt : 1) * sizeof(*tt));
    if (!tt) die("malloc failed");
    for (size_t i = 0; i < nt; i++) { tt[i][0] = stream[i]; tt[i][1] = stream[i+1]; tt[i][2] = stream[i+2]; }
    qsort(tt, nt, sizeof(*tt), cmp_triple);
    T.tg_prev2 = malloc((nt > 0 ? nt : 1) * sizeof(uint32_t));
    T.tg_prev1 = malloc((nt > 0 ? nt : 1) * sizeof(uint32_t));
    T.tg_cur   = malloc((nt > 0 ? nt : 1) * sizeof(uint32_t));
    T.tg_cnt   = malloc((nt > 0 ? nt : 1) * sizeof(uint64_t));
    if (!T.tg_prev2 || !T.tg_prev1 || !T.tg_cur || !T.tg_cnt) die("malloc failed");
    k = 0;
    for (size_t i = 0; i < nt; ) {
        size_t j = i;
        while (j < nt && tt[j][0] == tt[i][0] && tt[j][1] == tt[i][1] && tt[j][2] == tt[i][2]) j++;
        T.tg_prev2[k] = tt[i][0]; T.tg_prev1[k] = tt[i][1]; T.tg_cur[k] = tt[i][2]; T.tg_cnt[k] = (uint64_t)(j - i);
        k++; i = j;
    }
    T.tg_k = k;
    free(tt);
    return T;
}

static void trigram_range(const Tables *T, uint32_t p2, uint32_t p1, size_t *lo_out, size_t *hi_out) {
    size_t lo = 0, hi = T->tg_k;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t mp2 = T->tg_prev2[mid], mp1 = T->tg_prev1[mid];
        if (mp2 < p2 || (mp2 == p2 && mp1 < p1)) lo = mid + 1; else hi = mid;
    }
    size_t start = lo;
    hi = T->tg_k;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        uint32_t mp2 = T->tg_prev2[mid], mp1 = T->tg_prev1[mid];
        if (mp2 < p2 || (mp2 == p2 && mp1 <= p1)) lo = mid + 1; else hi = mid;
    }
    *lo_out = start; *hi_out = lo;
}

/* ---------------- field (symmetric, frozen) ---------------- */

typedef struct { double *H; uint32_t V; } Field;

static Field build_field(const uint32_t *stream, size_t n, uint32_t V) {
    Field F; F.V = V;
    size_t total = (size_t)V * (size_t)V;
    F.H = calloc(total > 0 ? total : 1, sizeof(double));
    if (!F.H) die("malloc failed building field");
    for (size_t i = 0; i < n; i++) {
        for (size_t j = i + 1; j <= i + WINDOW && j < n; j++) {
            uint32_t a = stream[i], b = stream[j];
            double val = 1.0 / (1.0 + (double)(j - i));
            if (a == b) {
                F.H[(size_t)a * V + b] += val;
            } else {
                F.H[(size_t)a * V + b] += val;
                F.H[(size_t)b * V + a] += val;
            }
        }
    }
    double mx = 0.0;
    for (size_t idx = 0; idx < total; idx++) if (F.H[idx] > mx) mx = F.H[idx];
    if (mx > 0.0) for (size_t idx = 0; idx < total; idx++) F.H[idx] /= mx;
    return F;
}

/* ---------------- permutation (arm e) ---------------- */

static uint32_t *build_permutation(uint32_t V) {
    uint32_t *pi = malloc(V * sizeof(uint32_t));
    if (!pi) die("malloc failed");
    for (uint32_t i = 0; i < V; i++) pi[i] = i;
    uint64_t x = 0xC0FFEEULL;
    for (uint32_t i = V - 1; i >= 1; i--) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
        uint32_t j = (uint32_t)(x % (uint64_t)(i + 1u));
        uint32_t t = pi[i]; pi[i] = pi[j]; pi[j] = t;
        if (i == 1) break; /* guard: unsigned i would wrap after this iteration otherwise */
    }
    return pi;
}

/* ---------------- probability chain ---------------- */

typedef struct {
    double *P1, *P2, *P3;
    double *Fbuf;
} Workspace;

static Workspace make_workspace(uint32_t V) {
    Workspace W;
    W.P1 = malloc(V * sizeof(double));
    W.P2 = malloc(V * sizeof(double));
    W.P3 = malloc(V * sizeof(double));
    W.Fbuf = malloc(V * sizeof(double));
    if (!W.P1 || !W.P2 || !W.P3 || !W.Fbuf) die("malloc failed (workspace)");
    return W;
}

static void compute_F_all(const Field *Fld, const uint32_t *perm, uint32_t V,
                           const uint32_t *past, int npast, double *out) {
    if (!Fld || npast == 0) {
        for (uint32_t u = 0; u < V; u++) out[u] = 1.0;
        return;
    }
    for (uint32_t u = 0; u < V; u++) {
        double prod = 1.0;
        uint32_t pu = perm ? perm[u] : u;
        for (int j = 0; j < npast; j++) {
            uint32_t p = past[j];
            uint32_t pp = perm ? perm[p] : p;
            double h = Fld->H[(size_t)pu * Fld->V + pp];
            prod *= (1.0 + BETA * h);
        }
        out[u] = prod;
    }
}

typedef struct {
    double P;
    int level;
    double sumcheck; /* NAN if not applicable */
} PriceResult;

static PriceResult price_position(const Tables *T, const Field *Fld, const uint32_t *perm,
                                   Workspace *W, uint32_t truth,
                                   int prev1_valid, uint32_t prev1,
                                   int prev2_valid, uint32_t prev2,
                                   int level_cap,
                                   const uint32_t *past, int npast) {
    uint32_t V = T->V;
    PriceResult R; R.level = 0; R.P = 0.0; R.sumcheck = NAN;

    compute_F_all(Fld, perm, V, past, npast, W->Fbuf);

    double sigma1 = 0.0;
    for (uint32_t u = 0; u < V; u++) {
        double w1 = (double)T->N1[u] * W->Fbuf[u];
        W->P1[u] = w1;
        sigma1 += w1;
    }
    for (uint32_t u = 0; u < V; u++)
        W->P1[u] = (1.0 - EPS) * (W->P1[u] / sigma1) + EPS * (1.0 / (double)V);

    if (level_cap == 1) {
        R.level = 1;
        R.P = W->P1[truth];
        double s = 0.0; for (uint32_t u = 0; u < V; u++) s += W->P1[u];
        R.sumcheck = s;
        return R;
    }

    int c2_nonempty = 0;
    size_t b_lo = 0, b_hi = 0;
    if (prev1_valid && prev1 < V) {
        b_lo = T->bg_offset[prev1];
        b_hi = T->bg_offset[prev1 + 1];
        c2_nonempty = (b_hi > b_lo);
    }
    if (!c2_nonempty) {
        memcpy(W->P2, W->P1, (size_t)V * sizeof(double));
    } else {
        double sigma2 = 0.0;
        for (uint32_t u = 0; u < V; u++) W->P2[u] = EPS * W->P1[u];
        for (size_t idx = b_lo; idx < b_hi; idx++) {
            uint32_t cu = T->bg_cur[idx];
            sigma2 += (double)T->bg_cnt[idx] * W->Fbuf[cu];
        }
        for (size_t idx = b_lo; idx < b_hi; idx++) {
            uint32_t cu = T->bg_cur[idx];
            double w2 = (double)T->bg_cnt[idx] * W->Fbuf[cu];
            W->P2[cu] += (1.0 - EPS) * (w2 / sigma2);
        }
    }

    if (level_cap == 2) {
        R.level = c2_nonempty ? 2 : 1;
        R.P = W->P2[truth];
        double s = 0.0; for (uint32_t u = 0; u < V; u++) s += W->P2[u];
        R.sumcheck = s;
        return R;
    }

    int c3_nonempty = 0;
    size_t t_lo = 0, t_hi = 0;
    if (prev1_valid && prev2_valid && prev1 < V && prev2 < V) {
        trigram_range(T, prev2, prev1, &t_lo, &t_hi);
        c3_nonempty = (t_hi > t_lo);
    }
    if (!c3_nonempty) {
        memcpy(W->P3, W->P2, (size_t)V * sizeof(double));
    } else {
        double sigma3 = 0.0;
        for (uint32_t u = 0; u < V; u++) W->P3[u] = EPS * W->P2[u];
        for (size_t idx = t_lo; idx < t_hi; idx++) {
            uint32_t cu = T->tg_cur[idx];
            sigma3 += (double)T->tg_cnt[idx] * W->Fbuf[cu];
        }
        for (size_t idx = t_lo; idx < t_hi; idx++) {
            uint32_t cu = T->tg_cur[idx];
            double w3 = (double)T->tg_cnt[idx] * W->Fbuf[cu];
            W->P3[cu] += (1.0 - EPS) * (w3 / sigma3);
        }
    }
    R.level = c3_nonempty ? 3 : (c2_nonempty ? 2 : 1);
    R.P = W->P3[truth];
    { double s = 0.0; for (uint32_t u = 0; u < V; u++) s += W->P3[u]; R.sumcheck = s; }
    return R;
}

/* ---------------- per-arm evidence walk ---------------- */

typedef struct {
    const char *name;
    double total_bits;
    size_t positions;
    size_t p_mismatch;
    size_t sumcheck_fail;
    size_t id_mismatch;
    size_t offset_mismatch;
    size_t builder_extra_lines; /* 1 if builder file had more lines than my stream */
    long first_bad_pos;
} ArmReport;

static ArmReport run_arm(const char *arm_name, const TokStream *test_ts, const Tables *T,
                          const Field *Fld, const uint32_t *perm, Workspace *W,
                          const char *evidence_path, int check_ids, int is_word_arm) {
    ArmReport rep; memset(&rep, 0, sizeof(rep));
    rep.name = arm_name; rep.first_bad_pos = -1;

    FILE *ef = fopen(evidence_path, "r");
    if (!ef) die("cannot open evidence file");

    uint32_t past[4] = {0,0,0,0};
    int npast = 0;
    uint32_t prev1 = 0, prev2 = 0;
    int prev1_valid = 0, prev2_valid = 0;

    for (size_t pos = 0; pos < test_ts->n; pos++) {
        uint32_t truth = test_ts->id[pos];
        uint32_t off = test_ts->off[pos];
        uint32_t tlen = test_ts->len[pos];

        double P; int level; double sumcheck = NAN;

        if (is_word_arm && truth == SENTINEL) {
            P = exp2(-8.0 * (double)tlen);
            level = 0;
        } else {
            int level_cap = (pos == 0) ? 1 : (pos == 1) ? 2 : 3;
            PriceResult R = price_position(T, Fld, perm, W, truth,
                                            prev1_valid, prev1, prev2_valid, prev2,
                                            level_cap, past, npast);
            P = R.P; level = R.level; sumcheck = R.sumcheck;
            if (!isnan(sumcheck) && fabs(sumcheck - 1.0) > 1e-6) {
                rep.sumcheck_fail++;
                if (rep.first_bad_pos < 0) rep.first_bad_pos = (long)pos;
            }
        }

        rep.total_bits += -log2(P);

        size_t b_ord; uint32_t b_off, b_len, b_truth; int b_level; double b_P;
        int got = fscanf(ef, "%zu %u %u %u %d %lf", &b_ord, &b_off, &b_len, &b_truth, &b_level, &b_P);
        if (got != 6) {
            fprintf(stderr, "%s: evidence file has fewer lines than my token stream (stopped at pos %zu)\n", arm_name, pos);
            rep.positions = pos;
            fclose(ef);
            return rep;
        }
        if (b_ord != pos) fprintf(stderr, "%s: ordinal mismatch at %zu (builder says %zu)\n", arm_name, pos, b_ord);
        if (b_off != off || b_len != tlen) {
            rep.offset_mismatch++;
            if (rep.first_bad_pos < 0) rep.first_bad_pos = (long)pos;
        }
        if (check_ids && !(is_word_arm && truth == SENTINEL) && b_truth != truth) {
            rep.id_mismatch++;
            if (rep.first_bad_pos < 0) rep.first_bad_pos = (long)pos;
        }
        (void)b_level;
        (void)level;
        double reltol = (fabs(P) > 0) ? fabs(P - b_P) / fabs(P) : fabs(P - b_P);
        if (reltol > 1e-9) {
            rep.p_mismatch++;
            if (rep.first_bad_pos < 0) rep.first_bad_pos = (long)pos;
        }

        rep.positions++;

        for (int k = 3; k > 0; k--) past[k] = past[k-1];
        past[0] = truth;
        if (npast < 4) npast++;
        prev2 = prev1; prev2_valid = prev1_valid;
        prev1 = truth; prev1_valid = 1;
    }

    {
        size_t b_ord; uint32_t b_off, b_len, b_truth; int b_level; double b_P;
        if (fscanf(ef, "%zu %u %u %u %d %lf", &b_ord, &b_off, &b_len, &b_truth, &b_level, &b_P) == 6) {
            rep.builder_extra_lines = 1;
            fprintf(stderr, "%s: builder evidence file has MORE lines than my token stream (extra at ordinal %zu)\n", arm_name, b_ord);
        }
    }

    fclose(ef);
    return rep;
}

/* ---------------- anti-copy ---------------- */

#define PFX_BITS 20u
#define PFX_SIZE (1u << PFX_BITS)
#define PFX_MASK (PFX_SIZE - 1u)

typedef struct { uint32_t *pos; uint32_t n, cap; } PosList;
typedef struct { PosList *buckets; } PrefixIndex;

static uint32_t pack4(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}
static uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16;
    return x;
}

static PrefixIndex build_prefix_index(const uint8_t *train, size_t train_len) {
    PrefixIndex idx;
    idx.buckets = calloc(PFX_SIZE, sizeof(PosList));
    if (!idx.buckets) die("malloc failed (prefix index)");
    if (train_len >= 4) {
        for (size_t i = 0; i + 4 <= train_len; i++) {
            uint32_t key = pack4(train + i);
            uint32_t h = hash32(key) & PFX_MASK;
            PosList *b = &idx.buckets[h];
            if (b->n == b->cap) {
                b->cap = b->cap ? b->cap * 2 : 4;
                b->pos = realloc(b->pos, b->cap * sizeof(uint32_t));
                if (!b->pos) die("realloc failed (prefix index)");
            }
            b->pos[b->n++] = (uint32_t)i;
        }
    }
    return idx;
}

static uint32_t best_match_at(const uint8_t *speech, size_t speech_len, size_t i,
                               const uint8_t *train, size_t train_len, const PrefixIndex *idx) {
    uint32_t best = 0;
    size_t remain = speech_len - i;
    if (remain >= 4 && train_len >= 4) {
        uint32_t key = pack4(speech + i);
        uint32_t h = hash32(key) & PFX_MASK;
        PosList *b = &idx->buckets[h];
        for (uint32_t bi = 0; bi < b->n; bi++) {
            uint32_t tp = b->pos[bi];
            size_t maxlen = remain;
            size_t avail = train_len - tp;
            if (avail < maxlen) maxlen = avail;
            size_t L = 0;
            while (L < maxlen && speech[i + L] == train[tp + L]) L++;
            if (L > best) best = (uint32_t)L;
        }
    } else {
        for (size_t tp = 0; tp < train_len; tp++) {
            size_t maxlen = remain;
            size_t avail = train_len - tp;
            if (avail < maxlen) maxlen = avail;
            size_t L = 0;
            while (L < maxlen && speech[i + L] == train[tp + L]) L++;
            if (L > best) best = (uint32_t)L;
        }
    }
    return best;
}

static void anti_copy(const char *label, const uint8_t *speech, size_t slen,
                       const uint8_t *train, size_t tlen, const PrefixIndex *idx, FILE *report) {
    if (slen == 0) { fprintf(report, "%s\tEMPTY\n", label); return; }
    uint32_t *bestlen = malloc(slen * sizeof(uint32_t));
    if (!bestlen) die("malloc failed (anti_copy)");
    uint32_t overall_max = 0;
    for (size_t i = 0; i < slen; i++) {
        bestlen[i] = best_match_at(speech, slen, i, train, tlen, idx);
        if (bestlen[i] > overall_max) overall_max = bestlen[i];
    }
    uint8_t *covered = calloc(slen, 1);
    if (!covered) die("malloc failed (anti_copy covered)");
    for (size_t i = 0; i < slen; i++) {
        if (bestlen[i] >= 32) {
            size_t end = i + bestlen[i];
            if (end > slen) end = slen;
            for (size_t k = i; k < end; k++) covered[k] = 1;
        }
    }
    size_t covered_n = 0;
    for (size_t i = 0; i < slen; i++) covered_n += covered[i];
    double frac = (double)covered_n / (double)slen;
    fprintf(report, "%s\tlongest_match_bytes=%u\tcoverage_ge32=%.6f\tspeech_bytes=%zu\n",
            label, overall_max, frac, slen);
    free(bestlen); free(covered);
}

/* ---------------- main ---------------- */

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <world.txt> <artifacts_dir> <out_results.tsv>\n", argv[0]);
        return 1;
    }
    const char *world_path = argv[1];
    const char *art_dir = argv[2];
    const char *out_path = argv[3];

    size_t wlen;
    uint8_t *world = read_file(world_path, &wlen);
    size_t train_len = (wlen * 9) / 10;
    size_t test_len = wlen - train_len;
    const uint8_t *train = world;
    const uint8_t *test = world + train_len;

    char path[4096];

    snprintf(path, sizeof(path), "%s/merges.tsv", art_dir);
    size_t nmerges;
    Merge *merges = load_merges(path, &nmerges);
    uint32_t V_unit = 256u + (uint32_t)nmerges;

    TokStream train_unit = bpe_replay(train, train_len, merges, nmerges);
    snprintf(path, sizeof(path), "%s/train_tokens.u32", art_dir);
    size_t builder_ntok;
    uint32_t *builder_tokens = load_u32le(path, &builder_ntok);

    size_t mismatch_count = 0;
    long first_mismatch = -1;
    if (builder_ntok != train_unit.n) {
        fprintf(stderr, "REFUSE world %s: train_tokens.u32 length mismatch: mine=%zu builder=%zu\n",
                world_path, train_unit.n, builder_ntok);
        mismatch_count = 1;
    } else {
        for (size_t i = 0; i < builder_ntok; i++) {
            if (builder_tokens[i] != train_unit.id[i]) {
                if (first_mismatch < 0) first_mismatch = (long)i;
                mismatch_count++;
            }
        }
    }
    if (mismatch_count > 0) {
        fprintf(stderr, "REFUSE world %s: train_tokens.u32 mismatch, count=%zu first_at=%ld\n",
                world_path, mismatch_count, first_mismatch);
        if (first_mismatch >= 0) {
            fprintf(stderr, "  mine=%u builder=%u\n",
                    train_unit.id[first_mismatch], builder_tokens[first_mismatch]);
        }
        fprintf(stderr, "Cannot trust unit-arm computations for this world. Aborting.\n");
        return 2;
    }
    fprintf(stderr, "OK world %s: train_tokens.u32 reproduced byte-identically (%zu tokens)\n",
            world_path, train_unit.n);

    TokStream test_unit = bpe_replay(test, test_len, merges, nmerges);

    TokStream train_byte; train_byte.n = train_len;
    train_byte.id = malloc((train_len > 0 ? train_len : 1) * sizeof(uint32_t));
    train_byte.off = NULL; train_byte.len = NULL;
    if (!train_byte.id) die("malloc failed");
    for (size_t i = 0; i < train_len; i++) train_byte.id[i] = train[i];

    TokStream test_byte; test_byte.n = test_len;
    test_byte.id  = malloc((test_len > 0 ? test_len : 1) * sizeof(uint32_t));
    test_byte.off = malloc((test_len > 0 ? test_len : 1) * sizeof(uint32_t));
    test_byte.len = malloc((test_len > 0 ? test_len : 1) * sizeof(uint32_t));
    if (!test_byte.id || !test_byte.off || !test_byte.len) die("malloc failed");
    for (size_t i = 0; i < test_len; i++) {
        test_byte.id[i] = test[i]; test_byte.off[i] = (uint32_t)i; test_byte.len[i] = 1;
    }

    Field Fld = build_field(train_unit.id, train_unit.n, V_unit);
    uint32_t *perm = build_permutation(V_unit);

    Tables T_byte = build_tables(train_byte.id, train_byte.n, 256);
    Tables T_unit = build_tables(train_unit.id, train_unit.n, V_unit);

    WordDict *wd = make_word_dict();
    TokStream train_word = word_scan(train, train_len, wd, 1);
    uint32_t V_word = 256u + (uint32_t)wd->n;
    Tables T_word = build_tables(train_word.id, train_word.n, V_word);
    TokStream test_word = word_scan(test, test_len, wd, 0);

    Workspace W_byte = make_workspace(256);
    Workspace W_unit = make_workspace(V_unit);
    Workspace W_word = make_workspace(V_word);

    ArmReport reports[5];
    char epath[4096];

    snprintf(epath, sizeof(epath), "%s/evidence_a.tsv", art_dir);
    reports[0] = run_arm("a", &test_byte, &T_byte, NULL, NULL, &W_byte, epath, 1, 0);

    snprintf(epath, sizeof(epath), "%s/evidence_b.tsv", art_dir);
    reports[1] = run_arm("b", &test_unit, &T_unit, NULL, NULL, &W_unit, epath, 1, 0);

    snprintf(epath, sizeof(epath), "%s/evidence_c.tsv", art_dir);
    reports[2] = run_arm("c", &test_unit, &T_unit, &Fld, NULL, &W_unit, epath, 1, 0);

    snprintf(epath, sizeof(epath), "%s/evidence_e.tsv", art_dir);
    reports[3] = run_arm("e", &test_unit, &T_unit, &Fld, perm, &W_unit, epath, 1, 0);

    snprintf(epath, sizeof(epath), "%s/evidence_d.tsv", art_dir);
    reports[4] = run_arm("d", &test_word, &T_word, NULL, NULL, &W_word, epath, 0, 1);

    FILE *out = fopen(out_path, "w");
    if (!out) die("cannot open results output");
    fprintf(out, "arm\tpositions\ttest_bytes\ttotal_bits\tbits_per_byte\n");
    for (int i = 0; i < 5; i++) {
        double bpb = reports[i].total_bits / (double)test_len;
        fprintf(out, "%s\t%zu\t%zu\t%.10f\t%.10f\n",
                reports[i].name, reports[i].positions, test_len, reports[i].total_bits, bpb);
    }
    fclose(out);

    fprintf(stderr, "\n=== world %s summary (V_unit=%u V_word=%u test_bytes=%zu) ===\n",
            world_path, V_unit, V_word, test_len);
    for (int i = 0; i < 5; i++) {
        ArmReport *r = &reports[i];
        fprintf(stderr,
            "arm %s: positions=%zu bits=%.6f bits/byte=%.6f | P-mismatch(>1e-9 rel)=%zu id-mismatch=%zu "
            "offset-mismatch=%zu sumcheck-fail=%zu builder-extra-lines=%zu first-bad-pos=%ld\n",
            r->name, r->positions, r->total_bits, r->total_bits / (double)test_len,
            r->p_mismatch, r->id_mismatch, r->offset_mismatch, r->sumcheck_fail,
            r->builder_extra_lines, r->first_bad_pos);
    }

    PrefixIndex pidx = build_prefix_index(train, train_len);
    fprintf(stderr, "\n=== anti-copy (world %s) ===\n", world_path);
    int seeds[5] = {7, 19, 42, 101, 271};
    const char *speecharm[2] = {"b", "c"};
    for (int a = 0; a < 2; a++) {
        for (int s = 0; s < 5; s++) {
            char sp[4096];
            snprintf(sp, sizeof(sp), "%s/speech_%s_%d.bin", art_dir, speecharm[a], seeds[s]);
            FILE *sf = fopen(sp, "rb");
            if (!sf) { fprintf(stderr, "speech file missing: %s\n", sp); continue; }
            fclose(sf);
            size_t slen;
            uint8_t *sbuf = read_file(sp, &slen);
            char label[256];
            snprintf(label, sizeof(label), "speech_%s_%d", speecharm[a], seeds[s]);
            anti_copy(label, sbuf, slen, train, train_len, &pidx, stderr);
            free(sbuf);
        }
    }

    return 0;
}
