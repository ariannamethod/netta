/* transfer_check.c — independent verifier for the NETTA transfer court.
 * Law: TRANSFER_PROTOCOL.md (SHA-256 230030d7...1dd72), consulted alone,
 * plus PROTOCOL.md of Body 0 for the probability chain it references.
 * Written without reading transfer.c, netta.c or netta_check.c.
 *
 * Stage 1: worlds, BPE growth (permutation-equivariant tie-break),
 * cipher isomorphism proof, ghost raw invariants, and the prequential
 * LOCAL pricing loop (cold arm) for all three worlds. Stage 2 (in a
 * later edit of this same file) adds descriptors, cache/align/oracle/
 * shuffled priors, the shadow/earn/revoke ledger, G_N and verdicts.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* ---------------- SHA-256 (public domain, compact) ---------------- */

typedef struct {
    uint32_t state[8];
    uint64_t bitlen;
    unsigned char buf[64];
    size_t buflen;
} Sha256;

static const uint32_t sha256_k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};

static uint32_t sha256_rotr(uint32_t x, int n) { return (x >> n) | (x << (32 - n)); }

static void sha256_transform(Sha256 *ctx, const unsigned char *data) {
    uint32_t m[64], a, b, c, d, e, f, g, h, t1, t2;
    int i;
    for (i = 0; i < 16; i++)
        m[i] = ((uint32_t)data[i*4] << 24) | ((uint32_t)data[i*4+1] << 16) |
               ((uint32_t)data[i*4+2] << 8) | ((uint32_t)data[i*4+3]);
    for (i = 16; i < 64; i++) {
        uint32_t s0 = sha256_rotr(m[i-15], 7) ^ sha256_rotr(m[i-15], 18) ^ (m[i-15] >> 3);
        uint32_t s1 = sha256_rotr(m[i-2], 17) ^ sha256_rotr(m[i-2], 19) ^ (m[i-2] >> 10);
        m[i] = m[i-16] + s0 + m[i-7] + s1;
    }
    a = ctx->state[0]; b = ctx->state[1]; c = ctx->state[2]; d = ctx->state[3];
    e = ctx->state[4]; f = ctx->state[5]; g = ctx->state[6]; h = ctx->state[7];
    for (i = 0; i < 64; i++) {
        uint32_t S1 = sha256_rotr(e,6) ^ sha256_rotr(e,11) ^ sha256_rotr(e,25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        t1 = h + S1 + ch + sha256_k[i] + m[i];
        uint32_t S0 = sha256_rotr(a,2) ^ sha256_rotr(a,13) ^ sha256_rotr(a,22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        t2 = S0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    ctx->state[0] += a; ctx->state[1] += b; ctx->state[2] += c; ctx->state[3] += d;
    ctx->state[4] += e; ctx->state[5] += f; ctx->state[6] += g; ctx->state[7] += h;
}

static void sha256_init(Sha256 *ctx) {
    ctx->state[0]=0x6a09e667; ctx->state[1]=0xbb67ae85; ctx->state[2]=0x3c6ef372; ctx->state[3]=0xa54ff53a;
    ctx->state[4]=0x510e527f; ctx->state[5]=0x9b05688c; ctx->state[6]=0x1f83d9ab; ctx->state[7]=0x5be0cd19;
    ctx->bitlen = 0; ctx->buflen = 0;
}

static void sha256_update(Sha256 *ctx, const unsigned char *data, size_t len) {
    size_t i;
    for (i = 0; i < len; i++) {
        ctx->buf[ctx->buflen++] = data[i];
        if (ctx->buflen == 64) {
            sha256_transform(ctx, ctx->buf);
            ctx->bitlen += 512;
            ctx->buflen = 0;
        }
    }
}

static void sha256_final(Sha256 *ctx, unsigned char out[32]) {
    size_t i = ctx->buflen;
    ctx->bitlen += (uint64_t)ctx->buflen * 8;
    ctx->buf[i++] = 0x80;
    if (i > 56) {
        while (i < 64) ctx->buf[i++] = 0;
        sha256_transform(ctx, ctx->buf);
        i = 0;
    }
    while (i < 56) ctx->buf[i++] = 0;
    for (int k = 7; k >= 0; k--) ctx->buf[i++] = (unsigned char)(ctx->bitlen >> (k*8));
    sha256_transform(ctx, ctx->buf);
    for (i = 0; i < 8; i++) {
        out[i*4]   = (unsigned char)(ctx->state[i] >> 24);
        out[i*4+1] = (unsigned char)(ctx->state[i] >> 16);
        out[i*4+2] = (unsigned char)(ctx->state[i] >> 8);
        out[i*4+3] = (unsigned char)(ctx->state[i]);
    }
}

static void sha256_hex(const unsigned char *data, long len, char out_hex[65]) {
    Sha256 ctx; unsigned char digest[32];
    sha256_init(&ctx);
    sha256_update(&ctx, data, (size_t)len);
    sha256_final(&ctx, digest);
    for (int i = 0; i < 32; i++) sprintf(out_hex + i*2, "%02x", digest[i]);
    out_hex[64] = 0;
}

/* ---------------- xorshift64 RNG (frozen law) ---------------- */

static uint64_t rng_next(uint64_t *state) {
    uint64_t x = *state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    *state = x;
    return x;
}

/* ---------------- file IO ---------------- */

static unsigned char *read_file(const char *path, long *out_len) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(1); }
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc((size_t)n);
    if (fread(buf, 1, (size_t)n, f) != (size_t)n) { fprintf(stderr, "short read %s\n", path); exit(1); }
    fclose(f);
    *out_len = n;
    return buf;
}

/* ---------------- World construction ---------------- */

static void build_cipher_perm(unsigned char perm[256], uint64_t seed) {
    uint64_t st = seed;
    for (int i = 0; i < 256; i++) perm[i] = (unsigned char)i;
    for (int i = 255; i >= 1; i--) {
        uint64_t r = rng_next(&st);
        int j = (int)(r % (uint64_t)(i + 1));
        unsigned char t = perm[i]; perm[i] = perm[j]; perm[j] = t;
    }
}

static unsigned char *build_w_iso(const unsigned char *A, long n, const unsigned char perm[256]) {
    unsigned char *out = malloc((size_t)n);
    for (long i = 0; i < n; i++) out[i] = perm[A[i]];
    return out;
}

static unsigned char *build_w_ghost(const unsigned char *A_train, long train_len, long out_n, uint64_t seed) {
    long counts[256] = {0};
    for (long i = 0; i < train_len; i++) counts[A_train[i]]++;
    long cum[257]; cum[0] = 0;
    for (int b = 0; b < 256; b++) cum[b+1] = cum[b] + counts[b];
    long total = train_len;
    unsigned char *out = malloc((size_t)out_n);
    uint64_t st = seed;
    for (long i = 0; i < out_n; i++) {
        uint64_t r = rng_next(&st);
        long v = (long)(r % (uint64_t)total);
        int lo = 0, hi = 255, b = 0;
        /* binary search: cum[b] <= v < cum[b+1] */
        while (lo <= hi) {
            int mid = (lo + hi) / 2;
            if (v < cum[mid+1]) { b = mid; hi = mid - 1; }
            else lo = mid + 1;
        }
        out[i] = (unsigned char)b;
    }
    return out;
}

typedef struct { unsigned char word[32]; int len; long count; long first_pos; } WordFreq;

static int is_ws(unsigned char c) {
    return c==' '||c=='\t'||c=='\n'||c=='\r'||c=='\v'||c=='\f';
}

/* Returns malloc'd output buffer, sets out_n. pairs[16][2][32] hold the
 * swapped words (rank order 0..15) and their lengths in pair_len[16][2],
 * for reporting; swap_map is filled with the from/to byte-strings used. */
static unsigned char *build_w_ff(const unsigned char *A, long n, long train_len,
                                  long *out_n,
                                  unsigned char rank_word[16][32], int rank_len[16]) {
    /* tokenize train region by whitespace, count frequency + first occurrence word-index */
    WordFreq *freqs = NULL; long nfreq = 0, cap = 0;
    long widx = 0;
    long i = 0;
    while (i < train_len) {
        if (is_ws(A[i])) { i++; continue; }
        long j = i;
        while (j < train_len && !is_ws(A[j])) j++;
        int wlen = (int)(j - i);
        if (wlen > 31) wlen = 31; /* clamp; top words are short in practice */
        /* find in freqs */
        long found = -1;
        for (long k = 0; k < nfreq; k++) {
            if (freqs[k].len == wlen && memcmp(freqs[k].word, A + i, (size_t)wlen) == 0) { found = k; break; }
        }
        if (found >= 0) {
            freqs[found].count++;
        } else {
            if (nfreq == cap) {
                cap = cap ? cap * 2 : 1024;
                freqs = realloc(freqs, (size_t)cap * sizeof(WordFreq));
            }
            memcpy(freqs[nfreq].word, A + i, (size_t)wlen);
            freqs[nfreq].len = wlen;
            freqs[nfreq].count = 1;
            freqs[nfreq].first_pos = widx;
            nfreq++;
        }
        widx++;
        i = j;
    }
    /* sort by count desc, tie-break first_pos asc (simple insertion since top16 needed, but do full sort for determinism) */
    for (long a = 1; a < nfreq; a++) {
        WordFreq key = freqs[a];
        long b = a - 1;
        while (b >= 0 && (freqs[b].count < key.count ||
               (freqs[b].count == key.count && freqs[b].first_pos > key.first_pos))) {
            freqs[b+1] = freqs[b];
            b--;
        }
        freqs[b+1] = key;
    }
    int top = nfreq < 16 ? (int)nfreq : 16;
    if (top < 16) { fprintf(stderr, "FATAL: fewer than 16 distinct words in A-train\n"); exit(1); }
    for (int k = 0; k < 16; k++) {
        rank_len[k] = freqs[k].len;
        memcpy(rank_word[k], freqs[k].word, (size_t)freqs[k].len);
    }
    free(freqs);

    /* build swap: pairs (0,1)(2,3)... */
    /* apply to full A: tokenize by whitespace, replace matching words */
    unsigned char *out = malloc((size_t)n * 4 + 16); /* generous upper bound */
    long oi = 0;
    i = 0;
    while (i < n) {
        if (is_ws(A[i])) { out[oi++] = A[i]; i++; continue; }
        long j = i;
        while (j < n && !is_ws(A[j])) j++;
        int wlen = (int)(j - i);
        int matched = -1;
        if (wlen <= 31) {
            for (int k = 0; k < 16; k++) {
                if (rank_len[k] == wlen && memcmp(rank_word[k], A + i, (size_t)wlen) == 0) { matched = k; break; }
            }
        }
        if (matched >= 0) {
            int partner = matched ^ 1; /* 0<->1, 2<->3, ... */
            memcpy(out + oi, rank_word[partner], (size_t)rank_len[partner]);
            oi += rank_len[partner];
        } else {
            memcpy(out + oi, A + i, (size_t)wlen);
            oi += wlen;
        }
        i = j;
    }
    *out_n = oi;
    return out;
}

/* ---------------- BPE growth (permutation-equivariant tie-break) ---------------- */

typedef struct { int32_t l, r; } Merge;

typedef struct {
    int32_t *tok;
    long n;
} TokStream;

/* grow_bpe: builds up to max_merges merges from byte/unit stream `init`,
 * length n0. Tie-break: max count; ties broken by earliest first
 * position of pair appearance. Stops at max_merges or max count < min_pair.
 * Returns final token stream (caller frees .tok) and fills *merges_out
 * (caller frees) with *n_merges_out entries. */
static TokStream grow_bpe(const int32_t *init, long n0, int max_merges, int min_pair,
                           Merge **merges_out, int *n_merges_out) {
    int32_t *tok = malloc((size_t)(n0 ? n0 : 1) * sizeof(int32_t));
    memcpy(tok, init, (size_t)n0 * sizeof(int32_t));
    int32_t *scratch = malloc((size_t)(n0 ? n0 : 1) * sizeof(int32_t));
    long n = n0;
    Merge *merges = malloc((size_t)max_merges * sizeof(Merge));
    int nm = 0;
    int32_t next_id = 256;

    /* generation-stamped hashmap for pair -> (count, first_pos): avoids
     * clearing the whole table every merge iteration (O(1) "clear" via
     * bumping cur_gen instead of O(cap) writes). */
    long cap = 1;
    while (cap < (n0 * 2 + 16)) cap <<= 1;
    int64_t *keys = malloc(cap * sizeof(int64_t));
    long *counts = malloc(cap * sizeof(long));
    long *firstpos = malloc(cap * sizeof(long));
    uint32_t *gen = calloc((size_t)cap, sizeof(uint32_t));
    uint32_t cur_gen = 0;

    for (int m = 0; m < max_merges; m++) {
        if (n < 2) break;
        cur_gen++;
        long best_count = -1, best_first = -1;
        int32_t best_l = 0, best_r = 0;
        int have_best = 0;
        for (long i = 0; i < n - 1; i++) {
            int64_t key = ((int64_t)tok[i] << 32) ^ (int64_t)(uint32_t)tok[i+1];
            uint64_t h = (uint64_t)key * 0x9E3779B97F4A7C15ULL;
            long slot = (long)(h & (uint64_t)(cap - 1));
            while (gen[slot] == cur_gen && keys[slot] != key) slot = (slot + 1) & (cap - 1);
            if (gen[slot] != cur_gen) { gen[slot] = cur_gen; keys[slot] = key; counts[slot] = 0; firstpos[slot] = i; }
            counts[slot]++;
            long c = counts[slot], f = firstpos[slot];
            if (c > best_count || (c == best_count && f < best_first)) {
                best_count = c; best_first = f;
                best_l = tok[i]; best_r = tok[i+1];
                have_best = 1;
            }
        }
        if (!have_best || best_count < min_pair) break;
        /* apply merge left-to-right, non-overlapping, into the scratch
         * buffer, then swap (avoids malloc/free every iteration) */
        long ni = 0;
        long i = 0;
        while (i < n) {
            if (i < n - 1 && tok[i] == best_l && tok[i+1] == best_r) {
                scratch[ni++] = next_id;
                i += 2;
            } else {
                scratch[ni++] = tok[i];
                i += 1;
            }
        }
        { int32_t *tmp = tok; tok = scratch; scratch = tmp; }
        n = ni;
        merges[nm].l = best_l; merges[nm].r = best_r;
        nm++;
        next_id++;
    }
    free(scratch);
    free(keys); free(counts); free(firstpos); free(gen);
    *merges_out = merges;
    *n_merges_out = nm;
    TokStream result; result.tok = tok; result.n = n;
    return result;
}

/* segment: replay merges in creation order over a fresh byte/unit stream */
static TokStream segment_with_merges(const int32_t *init, long n0, const Merge *merges, int n_merges) {
    int32_t *tok = malloc((size_t)n0 * sizeof(int32_t));
    memcpy(tok, init, (size_t)n0 * sizeof(int32_t));
    long n = n0;
    int32_t next_id = 256;
    for (int m = 0; m < n_merges; m++) {
        int32_t l = merges[m].l, r = merges[m].r;
        long ni = 0;
        long i = 0;
        int any = 0;
        for (long k = 0; k < n - 1; k++) if (tok[k]==l && tok[k+1]==r) { any = 1; break; }
        if (!any) { next_id++; continue; }
        int32_t *nt = malloc((size_t)n * sizeof(int32_t));
        while (i < n) {
            if (i < n - 1 && tok[i] == l && tok[i+1] == r) {
                nt[ni++] = next_id;
                i += 2;
            } else {
                nt[ni++] = tok[i];
                i += 1;
            }
        }
        free(tok);
        tok = nt;
        n = ni;
        next_id++;
    }
    TokStream result; result.tok = tok; result.n = n;
    return result;
}

static int32_t *bytes_to_units(const unsigned char *b, long n) {
    int32_t *u = malloc((size_t)n * sizeof(int32_t));
    for (long i = 0; i < n; i++) u[i] = b[i];
    return u;
}

/* ---------------- count tables + backoff probability ---------------- */

typedef struct {
    int32_t a, b;
    long c;
} Pair2;
typedef struct {
    int32_t a, b, c;
    long n;
} Pair3;

static int cmp_pair2(const void *x, const void *y) {
    const Pair2 *p = x, *q = y;
    if (p->a != q->a) return (p->a < q->a) ? -1 : 1;
    return (p->b < q->b) ? -1 : (p->b > q->b) ? 1 : 0;
}
static int cmp_pair3(const void *x, const void *y) {
    const Pair3 *p = x, *q = y;
    if (p->a != q->a) return (p->a < q->a) ? -1 : 1;
    if (p->b != q->b) return (p->b < q->b) ? -1 : 1;
    return (p->c < q->c) ? -1 : (p->c > q->c) ? 1 : 0;
}

typedef struct {
    long *n1;         /* size V, unigram counts */
    long V;
    Pair2 *fwd;        /* sorted by (a,b), RLE counts, distance-1 forward pairs */
    long n_fwd;
    Pair3 *tri;        /* sorted by (a,b,c), RLE counts */
    long n_tri;
} CountTable;

static long *build_n1(const int32_t *tok, long n, long V) {
    long *n1 = calloc((size_t)V, sizeof(long));
    for (long i = 0; i < n; i++) n1[tok[i]]++;
    return n1;
}

/* build RLE'd, sorted pair table from raw (a,b) list */
static Pair2 *rle_pairs2(int32_t *raw_a, int32_t *raw_b, long m, long *out_n) {
    Pair2 *p = malloc((size_t)m * sizeof(Pair2));
    for (long i = 0; i < m; i++) { p[i].a = raw_a[i]; p[i].b = raw_b[i]; p[i].c = 1; }
    qsort(p, (size_t)m, sizeof(Pair2), cmp_pair2);
    long w = 0;
    for (long i = 0; i < m; i++) {
        if (w > 0 && p[w-1].a == p[i].a && p[w-1].b == p[i].b) p[w-1].c++;
        else p[w++] = p[i];
    }
    *out_n = w;
    return p;
}

static Pair3 *rle_pairs3(int32_t *ra, int32_t *rb, int32_t *rc, long m, long *out_n) {
    Pair3 *p = malloc((size_t)m * sizeof(Pair3));
    for (long i = 0; i < m; i++) { p[i].a = ra[i]; p[i].b = rb[i]; p[i].c = rc[i]; p[i].n = 1; }
    qsort(p, (size_t)m, sizeof(Pair3), cmp_pair3);
    long w = 0;
    for (long i = 0; i < m; i++) {
        if (w > 0 && p[w-1].a == p[i].a && p[w-1].b == p[i].b && p[w-1].c == p[i].c) p[w-1].n++;
        else p[w++] = p[i];
    }
    *out_n = w;
    return p;
}

static CountTable build_counts(const int32_t *tok, long n, long V) {
    CountTable ct;
    ct.V = V;
    ct.n1 = build_n1(tok, n, V);
    long m2 = n > 1 ? n - 1 : 0;
    int32_t *a2 = malloc((size_t)(m2?m2:1) * sizeof(int32_t));
    int32_t *b2 = malloc((size_t)(m2?m2:1) * sizeof(int32_t));
    for (long i = 0; i < m2; i++) { a2[i] = tok[i]; b2[i] = tok[i+1]; }
    ct.fwd = rle_pairs2(a2, b2, m2, &ct.n_fwd);
    free(a2); free(b2);

    long m3 = n > 2 ? n - 2 : 0;
    int32_t *a3 = malloc((size_t)(m3?m3:1) * sizeof(int32_t));
    int32_t *b3 = malloc((size_t)(m3?m3:1) * sizeof(int32_t));
    int32_t *c3 = malloc((size_t)(m3?m3:1) * sizeof(int32_t));
    for (long i = 0; i < m3; i++) { a3[i] = tok[i]; b3[i] = tok[i+1]; c3[i] = tok[i+2]; }
    ct.tri = rle_pairs3(a3, b3, c3, m3, &ct.n_tri);
    free(a3); free(b3); free(c3);
    return ct;
}

static void free_counts(CountTable *ct) {
    free(ct->n1); free(ct->fwd); free(ct->tri);
}

/* binary search: first index in fwd with a==key (fwd sorted by (a,b)) */
static long fwd_lower_bound(const Pair2 *fwd, long n_fwd, int32_t a) {
    long lo = 0, hi = n_fwd;
    while (lo < hi) { long mid = (lo+hi)/2; if (fwd[mid].a < a) lo = mid+1; else hi = mid; }
    return lo;
}
static long tri_lower_bound(const Pair3 *tri, long n_tri, int32_t a, int32_t b) {
    long lo = 0, hi = n_tri;
    while (lo < hi) {
        long mid = (lo+hi)/2;
        Pair3 *p = (Pair3*)&tri[mid];
        int less = (p->a < a) || (p->a == a && p->b < b);
        if (less) lo = mid+1; else hi = mid;
    }
    return lo;
}

/* context sum + specific weight for level-2 (bigram) */
static void ctx2_lookup(const CountTable *ct, int32_t c1, int32_t u, long *sum_out, long *w_out) {
    long i = fwd_lower_bound(ct->fwd, ct->n_fwd, c1);
    long sum = 0, w = 0;
    while (i < ct->n_fwd && ct->fwd[i].a == c1) {
        sum += ct->fwd[i].c;
        if (ct->fwd[i].b == u) w = ct->fwd[i].c;
        i++;
    }
    *sum_out = sum; *w_out = w;
}
static void ctx3_lookup(const CountTable *ct, int32_t c2, int32_t c1, int32_t u, long *sum_out, long *w_out) {
    long i = tri_lower_bound(ct->tri, ct->n_tri, c2, c1);
    long sum = 0, w = 0;
    while (i < ct->n_tri && ct->tri[i].a == c2 && ct->tri[i].b == c1) {
        sum += ct->tri[i].n;
        if (ct->tri[i].c == u) w = ct->tri[i].n;
        i++;
    }
    *sum_out = sum; *w_out = w;
}

#define EPS 0.1

static double P1_of(const CountTable *ct, long n1_total, int32_t u) {
    long w = (u >= 0 && u < ct->V) ? ct->n1[u] : 0;
    double top = (n1_total > 0) ? (1.0 - EPS) * (double)w / (double)n1_total : 0.0;
    return top + EPS * (1.0 / (double)ct->V);
}
static double P2_of(const CountTable *ct, long n1_total, int32_t c1, int32_t u) {
    long sum, w;
    ctx2_lookup(ct, c1, u, &sum, &w);
    if (sum == 0) return P1_of(ct, n1_total, u);
    return (1.0 - EPS) * (double)w / (double)sum + EPS * P1_of(ct, n1_total, u);
}
static double P3_of(const CountTable *ct, long n1_total, int32_t c2, int32_t c1, int32_t u) {
    long sum, w;
    ctx3_lookup(ct, c2, c1, u, &sum, &w);
    if (sum == 0) return P2_of(ct, n1_total, c1, u);
    return (1.0 - EPS) * (double)w / (double)sum + EPS * P2_of(ct, n1_total, c1, u);
}

/* boundary-aware price: pos_in_world is the absolute position (0-based)
 * in the FULL lived destination stream (not just this chunk); ctx2/ctx1
 * are the previous two units of that same lived stream (or -1 if not
 * yet available). */
static double price_unit(const CountTable *ct, long n1_total, long pos_in_world,
                          int32_t c2, int32_t c1, int32_t u) {
    if (pos_in_world == 0) return P1_of(ct, n1_total, u);
    if (pos_in_world == 1) return P2_of(ct, n1_total, c1, u);
    return P3_of(ct, n1_total, c2, c1, u);
}

/* ---------------- unit byte-length + descriptors (12 scalars) ---------------- */

static long *build_unit_len(const Merge *merges, int n_merges) {
    long V = 256 + n_merges;
    long *ulen = malloc((size_t)V * sizeof(long));
    for (int i = 0; i < 256; i++) ulen[i] = 1;
    for (int i = 0; i < n_merges; i++) ulen[256+i] = ulen[merges[i].l] + ulen[merges[i].r];
    return ulen;
}

static Pair2 *build_dist_pairs(const int32_t *tok, long n, int d, long *out_n) {
    long m = (n > d) ? n - d : 0;
    int32_t *a = malloc((size_t)(m?m:1) * sizeof(int32_t));
    int32_t *b = malloc((size_t)(m?m:1) * sizeof(int32_t));
    for (long i = 0; i < m; i++) { a[i] = tok[i]; b[i] = tok[i+d]; }
    Pair2 *p = rle_pairs2(a, b, m, out_n);
    free(a); free(b);
    return p;
}

static Pair2 *build_reverse_pairs(const Pair2 *fwd, long n_fwd) {
    Pair2 *rev = malloc((size_t)(n_fwd?n_fwd:1) * sizeof(Pair2));
    for (long i = 0; i < n_fwd; i++) { rev[i].a = fwd[i].b; rev[i].b = fwd[i].a; rev[i].c = fwd[i].c; }
    qsort(rev, (size_t)n_fwd, sizeof(Pair2), cmp_pair2);
    return rev;
}

#define NDESC 12

typedef struct {
    long V;
    unsigned char *alive; /* 0/1, size V */
    double *raw;          /* V*NDESC */
    double *z;            /* V*NDESC, z-scored over alive units only */
} Descriptors;

/* group-wise pass over a Pair2 array sorted by 'a': calls fn(u, group_start, group_len) */
static void for_each_group(const Pair2 *p, long n, void (*fn)(int32_t u, long start, long len, void *ctx), void *ctx) {
    long i = 0;
    while (i < n) {
        long j = i;
        while (j < n && p[j].a == p[i].a) j++;
        fn(p[i].a, i, j - i, ctx);
        i = j;
    }
}

typedef struct {
    const Pair2 *fwd1;
    double *raw; /* V*NDESC */
    long V;
} EntropyCtx;

static void entropy_pass(int32_t u, long start, long len, void *ctx_) {
    EntropyCtx *ctx = ctx_;
    long total = 0;
    for (long k = 0; k < len; k++) total += ctx->fwd1[start+k].c;
    if (total == 0 || u < 0 || u >= ctx->V) return;
    double H = 0.0, top1 = 0.0, sorted4[4] = {0,0,0,0};
    for (long k = 0; k < len; k++) {
        double p = (double)ctx->fwd1[start+k].c / (double)total;
        H += -p * log2(p);
        if (p > top1) top1 = p;
        for (int s = 0; s < 4; s++) {
            if (p > sorted4[s]) {
                for (int t = 3; t > s; t--) sorted4[t] = sorted4[t-1];
                sorted4[s] = p;
                break;
            }
        }
    }
    double top4 = sorted4[0]+sorted4[1]+sorted4[2]+sorted4[3];
    ctx->raw[u*NDESC + 2] = H;      /* right transition entropy */
    ctx->raw[u*NDESC + 4] = log2((double)len); /* log2 right degree */
    ctx->raw[u*NDESC + 6] = top1;
    ctx->raw[u*NDESC + 7] = top4;
}

typedef struct {
    const Pair2 *rev1;
    double *raw;
    long V;
} LeftCtx;

static void left_entropy_pass(int32_t u, long start, long len, void *ctx_) {
    LeftCtx *ctx = ctx_;
    long total = 0;
    for (long k = 0; k < len; k++) total += ctx->rev1[start+k].c;
    if (total == 0 || u < 0 || u >= ctx->V) return;
    double H = 0.0;
    for (long k = 0; k < len; k++) {
        double p = (double)ctx->rev1[start+k].c / (double)total;
        H += -p * log2(p);
    }
    ctx->raw[u*NDESC + 3] = H;                 /* left transition entropy */
    ctx->raw[u*NDESC + 5] = log2((double)len); /* log2 left degree */
}

typedef struct {
    const Pair2 *dtab;
    double *raw;
    int dimidx; /* 8,9,10 for d=1,2,4 */
    long V;
} ConcCtx;

static void concentration_pass(int32_t u, long start, long len, void *ctx_) {
    ConcCtx *ctx = ctx_;
    long total = 0;
    for (long k = 0; k < len; k++) total += ctx->dtab[start+k].c;
    if (total == 0 || u < 0 || u >= ctx->V) return;
    double simpson = 0.0;
    for (long k = 0; k < len; k++) {
        double frac = (double)ctx->dtab[start+k].c / (double)total;
        simpson += frac * frac;
    }
    ctx->raw[u*NDESC + ctx->dimidx] = simpson;
}

/* refinement (dim 11): transition-probability-weighted mean of right
 * neighbors' right-entropies. Needs H_right (dim index 2) already filled. */
typedef struct {
    const Pair2 *fwd1;
    double *raw;
    long V;
} RefineCtx;

static void refine_pass(int32_t u, long start, long len, void *ctx_) {
    RefineCtx *ctx = ctx_;
    long total = 0;
    for (long k = 0; k < len; k++) total += ctx->fwd1[start+k].c;
    if (total == 0 || u < 0 || u >= ctx->V) return;
    double acc = 0.0;
    for (long k = 0; k < len; k++) {
        int32_t v = ctx->fwd1[start+k].b;
        double p = (double)ctx->fwd1[start+k].c / (double)total;
        double Hv = (v >= 0 && v < ctx->V) ? ctx->raw[v*NDESC + 2] : 0.0;
        acc += p * Hv;
    }
    ctx->raw[u*NDESC + 11] = acc;
}

static Descriptors compute_descriptors(const int32_t *tok, long n, long V, const long *n1, const long *ulen) {
    Descriptors d;
    d.V = V;
    d.alive = calloc((size_t)V, 1);
    d.raw = calloc((size_t)V * NDESC, sizeof(double));
    d.z = calloc((size_t)V * NDESC, sizeof(double));

    long n_fwd1; Pair2 *fwd1 = build_dist_pairs(tok, n, 1, &n_fwd1);
    long n_fwd2; Pair2 *fwd2 = build_dist_pairs(tok, n, 2, &n_fwd2);
    long n_fwd4; Pair2 *fwd4 = build_dist_pairs(tok, n, 4, &n_fwd4);
    Pair2 *rev1 = build_reverse_pairs(fwd1, n_fwd1);

    for (long u = 0; u < V; u++) {
        d.alive[u] = (n1[u] > 0);
        if (n1[u] > 0) d.raw[u*NDESC + 0] = log2((double)n1[u]);
        d.raw[u*NDESC + 1] = (double)ulen[u];
    }

    EntropyCtx ec = { fwd1, d.raw, V };
    for_each_group(fwd1, n_fwd1, entropy_pass, &ec);
    LeftCtx lc = { rev1, d.raw, V };
    for_each_group(rev1, n_fwd1, left_entropy_pass, &lc);
    ConcCtx cc1 = { fwd1, d.raw, 8, V };
    for_each_group(fwd1, n_fwd1, concentration_pass, &cc1);
    ConcCtx cc2 = { fwd2, d.raw, 9, V };
    for_each_group(fwd2, n_fwd2, concentration_pass, &cc2);
    ConcCtx cc4 = { fwd4, d.raw, 10, V };
    for_each_group(fwd4, n_fwd4, concentration_pass, &cc4);
    RefineCtx rc = { fwd1, d.raw, V };
    for_each_group(fwd1, n_fwd1, refine_pass, &rc);

    free(fwd1); free(fwd2); free(fwd4); free(rev1);

    /* z-score per dimension over alive units */
    for (int k = 0; k < NDESC; k++) {
        double sum = 0.0; long cnt = 0;
        for (long u = 0; u < V; u++) if (d.alive[u]) { sum += d.raw[u*NDESC+k]; cnt++; }
        double mean = cnt ? sum / (double)cnt : 0.0;
        double var = 0.0;
        for (long u = 0; u < V; u++) if (d.alive[u]) { double diff = d.raw[u*NDESC+k]-mean; var += diff*diff; }
        double std = cnt ? sqrt(var / (double)cnt) : 0.0;
        for (long u = 0; u < V; u++) {
            if (!d.alive[u]) continue;
            d.z[u*NDESC+k] = (std > 1e-12) ? (d.raw[u*NDESC+k]-mean)/std : 0.0;
        }
    }
    return d;
}

static void free_descriptors(Descriptors *d) {
    free(d->alive); free(d->raw); free(d->z);
}

static double zdist(const Descriptors *d, int32_t u, const Descriptors *d2, int32_t v) {
    double s = 0.0;
    for (int k = 0; k < NDESC; k++) {
        double diff = d->z[u*NDESC+k] - d2->z[v*NDESC+k];
        s += diff*diff;
    }
    return sqrt(s);
}

/* ---------------- unit byte-strings (for the oracle's true map) ---------------- */

typedef struct { unsigned char *data; int len; } ByteStr;

static ByteStr *build_bytestrings(const Merge *merges, int n_merges) {
    long V = 256 + n_merges;
    ByteStr *bs = malloc((size_t)V * sizeof(ByteStr));
    for (int i = 0; i < 256; i++) { bs[i].data = malloc(1); bs[i].data[0] = (unsigned char)i; bs[i].len = 1; }
    for (int i = 0; i < n_merges; i++) {
        int32_t l = merges[i].l, r = merges[i].r;
        int len = bs[l].len + bs[r].len;
        unsigned char *d = malloc((size_t)len);
        memcpy(d, bs[l].data, (size_t)bs[l].len);
        memcpy(d + bs[l].len, bs[r].data, (size_t)bs[r].len);
        bs[256+i].data = d; bs[256+i].len = len;
    }
    return bs;
}
static void free_bytestrings(ByteStr *bs, long V) {
    for (long i = 0; i < V; i++) free(bs[i].data);
    free(bs);
}

/* ---------------- oracle true-map (byte permutation / word swap) ---------------- */

typedef enum { XFORM_NONE, XFORM_CIPHER, XFORM_FF } XformKind;
typedef struct {
    XformKind kind;
    unsigned char cipher_perm[256];
    unsigned char rank_word[16][32];
    int rank_len[16];
} OracleXform;

static void xform_apply(const OracleXform *x, const unsigned char *in, int inlen, unsigned char *out, int *outlen) {
    if (x->kind == XFORM_CIPHER) {
        for (int i = 0; i < inlen; i++) out[i] = x->cipher_perm[in[i]];
        *outlen = inlen;
    } else if (x->kind == XFORM_FF) {
        int matched = -1;
        if (inlen <= 31) {
            for (int k = 0; k < 16; k++)
                if (x->rank_len[k] == inlen && memcmp(x->rank_word[k], in, (size_t)inlen) == 0) { matched = k; break; }
        }
        if (matched >= 0) {
            int partner = matched ^ 1;
            memcpy(out, x->rank_word[partner], (size_t)x->rank_len[partner]);
            *outlen = x->rank_len[partner];
        } else {
            memcpy(out, in, (size_t)inlen);
            *outlen = inlen;
        }
    } else {
        memcpy(out, in, (size_t)inlen);
        *outlen = inlen;
    }
}

typedef struct { const unsigned char *data; int len; int32_t id; } BSEntry;
static int cmp_bsentry(const void *x, const void *y) {
    const BSEntry *p = x, *q = y;
    int minlen = p->len < q->len ? p->len : q->len;
    int c = minlen ? memcmp(p->data, q->data, (size_t)minlen) : 0;
    if (c != 0) return c;
    return p->len - q->len;
}

typedef struct { int32_t *map; long V_A; } OracleMap;

static OracleMap build_oracle_map(const ByteStr *A_bs, long V_A, const ByteStr *dest_bs, long V_dest, const OracleXform *xform) {
    OracleMap om; om.V_A = V_A; om.map = malloc((size_t)V_A * sizeof(int32_t));
    for (long i = 0; i < V_A; i++) om.map[i] = -1;
    if (xform->kind == XFORM_NONE) return om;
    BSEntry *entries = malloc((size_t)V_dest * sizeof(BSEntry));
    for (long i = 0; i < V_dest; i++) { entries[i].data = dest_bs[i].data; entries[i].len = dest_bs[i].len; entries[i].id = (int32_t)i; }
    qsort(entries, (size_t)V_dest, sizeof(BSEntry), cmp_bsentry);
    unsigned char buf[8192];
    for (long i = 0; i < V_A; i++) {
        if (A_bs[i].len > 8000) continue;
        int outlen;
        xform_apply(xform, A_bs[i].data, A_bs[i].len, buf, &outlen);
        long lo = 0, hi = V_dest - 1, found = -1;
        while (lo <= hi) {
            long mid = (lo + hi) / 2;
            BSEntry *e = &entries[mid];
            int minlen = e->len < outlen ? e->len : outlen;
            int c = minlen ? memcmp(e->data, buf, (size_t)minlen) : 0;
            if (c == 0) c = e->len - outlen;
            if (c == 0) { found = mid; break; }
            else if (c < 0) lo = mid + 1; else hi = mid - 1;
        }
        if (found >= 0) om.map[i] = entries[found].id;
    }
    free(entries);
    return om;
}
static void free_oracle_map(OracleMap *m) { free(m->map); }

/* ---------------- greedy top-256 alignment map ---------------- */

typedef struct { int32_t *dest_to_A; int32_t *A_to_dest; long V_dest, V_A; } AlignMap;

typedef struct { int32_t id; long freq; } FreqItem;
static int cmp_freq_desc(const void *x, const void *y) {
    const FreqItem *p = x, *q = y;
    if (p->freq != q->freq) return p->freq > q->freq ? -1 : 1;
    return p->id < q->id ? -1 : 1;
}
typedef struct { double dist; int32_t di, ai; } CandPair;
static int cmp_candpair(const void *x, const void *y) {
    const CandPair *p = x, *q = y;
    if (p->dist != q->dist) return p->dist < q->dist ? -1 : 1;
    if (p->ai != q->ai) return p->ai < q->ai ? -1 : 1;
    return p->di < q->di ? -1 : (p->di > q->di ? 1 : 0);
}

static AlignMap build_align_map(const Descriptors *dd, const long *dest_n1,
                                 const Descriptors *ad, const long *a_n1,
                                 const int32_t *shuf) {
    AlignMap am; am.V_dest = dd->V; am.V_A = ad->V;
    am.dest_to_A = malloc((size_t)dd->V * sizeof(int32_t));
    am.A_to_dest = malloc((size_t)ad->V * sizeof(int32_t));
    for (long i = 0; i < dd->V; i++) am.dest_to_A[i] = -1;
    for (long i = 0; i < ad->V; i++) am.A_to_dest[i] = -1;

    FreqItem *df = malloc((size_t)dd->V * sizeof(FreqItem)); long ndf = 0;
    for (long i = 0; i < dd->V; i++) if (dd->alive[i]) { df[ndf].id = (int32_t)i; df[ndf].freq = dest_n1[i]; ndf++; }
    qsort(df, (size_t)ndf, sizeof(FreqItem), cmp_freq_desc);
    long topD = ndf < 256 ? ndf : 256;

    FreqItem *af = malloc((size_t)ad->V * sizeof(FreqItem)); long naf = 0;
    for (long i = 0; i < ad->V; i++) if (ad->alive[i]) { af[naf].id = (int32_t)i; af[naf].freq = a_n1[i]; naf++; }
    qsort(af, (size_t)naf, sizeof(FreqItem), cmp_freq_desc);
    long topA = naf < 256 ? naf : 256;

    long cap_pairs = topD * topA;
    if (cap_pairs < 1) cap_pairs = 1;
    CandPair *pairs = malloc((size_t)cap_pairs * sizeof(CandPair));
    long pi = 0;
    for (long i = 0; i < topA; i++) {
        int32_t a = af[i].id;
        int32_t eff_a = shuf ? shuf[a] : a;
        if (!ad->alive[eff_a]) continue;
        for (long j = 0; j < topD; j++) {
            int32_t di = df[j].id;
            pairs[pi].dist = zdist(dd, di, ad, eff_a);
            pairs[pi].di = di; pairs[pi].ai = a;
            pi++;
        }
    }
    qsort(pairs, (size_t)pi, sizeof(CandPair), cmp_candpair);
    unsigned char *matchedD = calloc((size_t)dd->V, 1), *matchedA = calloc((size_t)ad->V, 1);
    for (long k = 0; k < pi; k++) {
        if (pairs[k].dist > 2.0) break;
        if (matchedD[pairs[k].di] || matchedA[pairs[k].ai]) continue;
        matchedD[pairs[k].di] = 1; matchedA[pairs[k].ai] = 1;
        am.dest_to_A[pairs[k].di] = pairs[k].ai;
        am.A_to_dest[pairs[k].ai] = pairs[k].di;
    }
    free(matchedD); free(matchedA); free(pairs); free(df); free(af);
    return am;
}
static void free_align_map(AlignMap *m) { free(m->dest_to_A); free(m->A_to_dest); }

/* raw (pre-escape) prior via an alignment map (used identically for
 * learned align and for oracle, which only swaps in the true map).
 * "only aligned continuations count": among a_star's own right-neighbour
 * distribution in A (denominator = A's own full mass, not renormalized),
 * only neighbours that are themselves aligned to some destination unit
 * contribute; if none are aligned the position is undefined. */
static double align_style_prior_raw(const int32_t *dest_to_A, const int32_t *A_to_dest, long V_dest, long V_A,
                                     const Pair2 *a_fwd1, long a_n_fwd1,
                                     const int32_t *shuf, int32_t c1, int32_t truth) {
    if (c1 < 0 || c1 >= V_dest) return -1.0;
    int32_t a_star = dest_to_A[c1];
    if (a_star < 0) return -1.0;
    int32_t eff_star = shuf ? shuf[a_star] : a_star;
    long i = fwd_lower_bound(a_fwd1, a_n_fwd1, eff_star);
    long total_all = 0, aligned_count = 0;
    double truth_weight = -1.0;
    while (i < a_n_fwd1 && a_fwd1[i].a == eff_star) {
        int32_t v = a_fwd1[i].b;
        total_all += a_fwd1[i].c;
        if (v >= 0 && v < V_A) {
            int32_t dest_v = A_to_dest[v];
            if (dest_v >= 0) {
                aligned_count++;
                if (dest_v == truth) truth_weight = (double)a_fwd1[i].c;
            }
        }
        i++;
    }
    if (total_all == 0 || aligned_count == 0) return -1.0;
    return truth_weight >= 0 ? truth_weight / (double)total_all : 0.0;
}

/* ---------------- cache prior (context-keyed, cached per chunk) ---------------- */

static void get_top4_right(const Pair2 *fwd1, long n_fwd1, int32_t unit, int32_t out_ids[4], int *n_out) {
    long i = fwd_lower_bound(fwd1, n_fwd1, unit);
    long start = i, total = 0;
    while (i < n_fwd1 && fwd1[i].a == unit) { total += fwd1[i].c; i++; }
    double sorted_p[4] = {-1,-1,-1,-1};
    int32_t sorted_id[4] = {-1,-1,-1,-1};
    if (total > 0) {
        for (long k = start; k < i; k++) {
            double p = (double)fwd1[k].c / (double)total;
            for (int s = 0; s < 4; s++) {
                if (p > sorted_p[s]) {
                    for (int t = 3; t > s; t--) { sorted_p[t]=sorted_p[t-1]; sorted_id[t]=sorted_id[t-1]; }
                    sorted_p[s] = p; sorted_id[s] = fwd1[k].b;
                    break;
                }
            }
        }
    }
    int n = 0;
    for (int s = 0; s < 4; s++) if (sorted_id[s] >= 0) out_ids[n++] = sorted_id[s];
    *n_out = n;
}

typedef struct {
    int32_t a_star;
    int32_t top4[4]; int n_top4;
    double max_r, sum_exp;
} CacheCtxInfo;

static double min_zdist_to_top4(const Descriptors *dd, int32_t u, const Descriptors *ad,
                                 const int32_t *top4, int n4, const int32_t *shuf) {
    double mind = 1e300;
    for (int k = 0; k < n4; k++) {
        int32_t eff_nb = shuf ? shuf[top4[k]] : top4[k];
        if (!ad->alive[eff_nb]) continue;
        double dtmp = zdist(dd, u, ad, eff_nb);
        if (dtmp < mind) mind = dtmp;
    }
    return mind;
}

static CacheCtxInfo compute_cache_ctx(int32_t c1, const Descriptors *dd, const Descriptors *ad,
                                       const Pair2 *a_fwd1, long a_n_fwd1, const int32_t *shuf) {
    CacheCtxInfo info; memset(&info, 0, sizeof(info)); info.a_star = -1;
    if (c1 < 0 || c1 >= dd->V || !dd->alive[c1]) return info;
    double best = 1e300; int32_t best_a = -1;
    for (long a = 0; a < ad->V; a++) {
        if (!ad->alive[a]) continue;
        int32_t eff_a = shuf ? shuf[a] : (int32_t)a;
        if (!ad->alive[eff_a]) continue;
        double dist = zdist(dd, c1, ad, eff_a);
        if (dist < best) { best = dist; best_a = (int32_t)a; }
    }
    if (best_a < 0) return info;
    info.a_star = best_a;
    int32_t eff_star = shuf ? shuf[best_a] : best_a;
    get_top4_right(a_fwd1, a_n_fwd1, eff_star, info.top4, &info.n_top4);
    if (info.n_top4 == 0) { info.a_star = -1; return info; }

    double max_r = -1e300;
    for (long u = 0; u < dd->V; u++) {
        if (!dd->alive[u]) continue;
        double mind = min_zdist_to_top4(dd, (int32_t)u, ad, info.top4, info.n_top4, shuf);
        double r = -mind;
        if (r > max_r) max_r = r;
    }
    double sum_exp = 0.0;
    for (long u = 0; u < dd->V; u++) {
        if (!dd->alive[u]) continue;
        double mind = min_zdist_to_top4(dd, (int32_t)u, ad, info.top4, info.n_top4, shuf);
        double r = -mind;
        sum_exp += exp(r - max_r);
    }
    info.max_r = max_r; info.sum_exp = sum_exp;
    return info;
}

static double cache_prior_raw(const CacheCtxInfo *info, const Descriptors *dd, const Descriptors *ad,
                               const int32_t *shuf, int32_t truth) {
    if (info->a_star < 0 || info->sum_exp <= 0.0) return -1.0;
    if (truth < 0 || truth >= dd->V || !dd->alive[truth]) return -1.0;
    double mind = min_zdist_to_top4(dd, truth, ad, info->top4, info->n_top4, shuf);
    double r = -mind;
    return exp(r - info->max_r) / info->sum_exp;
}

/* ---------------- shadow / earn / revoke ledger ---------------- */

typedef struct { double L; double ledger; int earned; } ArmLedger;

/* price_with_ledger takes an ALREADY-ESCAPED prior probability
 * (P_prior_used = 0.9*raw + 0.1*P_local), or a negative value meaning
 * "undefined at this position" (unaligned context / empty carried set /
 * no destination-side descriptor for the context unit). */
static double price_with_ledger(ArmLedger *st, double P_local, double P_prior_used, long *undefined_ctr) {
    double l_local = -log2(P_local);
    if (P_prior_used < 0.0) {
        if (undefined_ctr) (*undefined_ctr)++;
        return l_local;
    }
    double l_prior = -log2(P_prior_used);
    double L = st->earned ? st->L : 0.0;
    double P_final = (1.0 - L) * P_local + L * P_prior_used;
    double bits = -log2(P_final);
    st->ledger += (l_local - l_prior);
    if (!st->earned) {
        if (st->ledger >= 32.0) { st->earned = 1; st->L = 0.05; }
    } else {
        if (st->ledger < 16.0) { st->earned = 0; st->L = 0.0; }
        else {
            st->L *= exp(0.05 * (l_local - l_prior));
            if (st->L < 0.01) st->L = 0.01;
            if (st->L > 0.5) st->L = 0.5;
        }
    }
    return bits;
}

static double escape_mix(double raw, double P_local) {
    return raw >= 0.0 ? (0.9 * raw + 0.1 * P_local) : -1.0;
}

/* ---------------- arms ---------------- */

typedef enum { ARM_COLD=0, ARM_CACHE, ARM_ALIGN, ARM_BOTH, ARM_SHUF, ARM_ORACLE, N_ARMS } ArmId;
static const char *ARM_NAME[N_ARMS] = {"cold","cache","align","both","shuf","oracle"};

typedef struct {
    long chunk, byte_offset, byte_len, positions;
    double bits;
    double L_end, ledger_end;
    int state_end;
    long undefined_positions;
} ArmChunkResult;

/* ---------------- A-side past (built once) ---------------- */

typedef struct {
    long train_len;
    TokStream tok;
    Merge *merges; int n_merges;
    long V;
    CountTable ct;
    long *ulen;
    Descriptors desc;
    ByteStr *bs;
} APast;

static APast build_a_past(const unsigned char *A, long train_len) {
    APast p; memset(&p, 0, sizeof(p));
    p.train_len = train_len;
    int32_t *units = bytes_to_units(A, train_len);
    p.tok = grow_bpe(units, train_len, 2048, 4, &p.merges, &p.n_merges);
    free(units);
    p.V = 256 + p.n_merges;
    p.ct = build_counts(p.tok.tok, p.tok.n, p.V);
    p.ulen = build_unit_len(p.merges, p.n_merges);
    p.desc = compute_descriptors(p.tok.tok, p.tok.n, p.V, p.ct.n1, p.ulen);
    p.bs = build_bytestrings(p.merges, p.n_merges);
    return p;
}

/* grow BPE on cipher(A-train) independently and prove, in lockstep, that
 * merges correspond to A's own merges under the known permutation. */
static void prove_isomorphism(const unsigned char *A, long train_len, const unsigned char perm[256],
                               const Merge *merges_A, int n_merges_A, long *out_matched, long *out_total,
                               int *out_cipher_merges) {
    unsigned char *cipherA = malloc((size_t)train_len);
    for (long i = 0; i < train_len; i++) cipherA[i] = perm[A[i]];
    int32_t *units = bytes_to_units(cipherA, train_len);
    Merge *merges_C; int n_merges_C;
    TokStream t = grow_bpe(units, train_len, 2048, 4, &merges_C, &n_merges_C);
    free(units); free(cipherA); free(t.tok);

    long V_max = 256 + (n_merges_A > n_merges_C ? n_merges_A : n_merges_C);
    int32_t *corr = malloc((size_t)V_max * sizeof(int32_t));
    for (int b = 0; b < 256; b++) corr[b] = perm[b];
    for (long i = 256; i < V_max; i++) corr[i] = -1;
    int total = n_merges_A < n_merges_C ? n_merges_A : n_merges_C;
    int matched = 0;
    for (int i = 0; i < total; i++) {
        if (corr[merges_A[i].l] == merges_C[i].l && corr[merges_A[i].r] == merges_C[i].r) {
            corr[256+i] = 256+i;
            matched++;
        } else break;
    }
    free(corr); free(merges_C);
    *out_matched = matched; *out_total = n_merges_A; *out_cipher_merges = n_merges_C;
}

static int32_t *build_shuffle_map(const Descriptors *ad, uint64_t seed) {
    long V = ad->V;
    int32_t *ids = malloc((size_t)(V?V:1) * sizeof(int32_t));
    long m = 0;
    for (long i = 0; i < V; i++) if (ad->alive[i]) ids[m++] = (int32_t)i;
    long *order = malloc((size_t)(m?m:1) * sizeof(long));
    for (long i = 0; i < m; i++) order[i] = i;
    uint64_t st = seed;
    for (long i = m - 1; i >= 1; i--) {
        uint64_t r = rng_next(&st);
        long j = (long)(r % (uint64_t)(i + 1));
        long tmp = order[i]; order[i] = order[j]; order[j] = tmp;
    }
    int32_t *sigma = malloc((size_t)(V?V:1) * sizeof(int32_t));
    for (long i = 0; i < V; i++) sigma[i] = (int32_t)i;
    for (long k = 0; k < m; k++) sigma[ids[k]] = ids[order[k]];
    free(ids); free(order);
    return sigma;
}

/* ---------------- full per-world, per-arm prequential court ---------------- */

typedef struct {
    long n_chunks;
    ArmChunkResult *res[N_ARMS];
} CourtResult;

#define WARMUP_BYTES 16384

static CourtResult run_full_court(const unsigned char *dest, long dn, const APast *past,
                                   const OracleXform *xform, const int32_t *shuf_map, int report,
                                   const char *world_name) {
    long chunk_size = 1024;
    long n_chunks = (dn + chunk_size - 1) / chunk_size;
    CourtResult cr; cr.n_chunks = n_chunks;
    for (int a = 0; a < N_ARMS; a++) cr.res[a] = malloc((size_t)n_chunks * sizeof(ArmChunkResult));

    Merge *cur_merges = NULL; int n_cur_merges = 0;
    TokStream prefix_tok; prefix_tok.tok = NULL; prefix_tok.n = 0;
    long V = 256;
    CountTable ct; memset(&ct, 0, sizeof(ct)); ct.V = V; ct.n1 = calloc((size_t)V, sizeof(long));
    long n1_total = 0;
    long world_token_index = 0;

    Descriptors dd; memset(&dd, 0, sizeof(dd)); dd.V = V;
    dd.alive = calloc((size_t)V, 1); dd.raw = calloc((size_t)V*NDESC, sizeof(double)); dd.z = calloc((size_t)V*NDESC, sizeof(double));
    ByteStr *dest_bs = build_bytestrings(NULL, 0); /* 256 raw bytes, no merges yet */
    long *dest_ulen = build_unit_len(NULL, 0);

    ArmLedger ledger[N_ARMS];
    for (int a = 0; a < N_ARMS; a++) { ledger[a].L = 0.0; ledger[a].ledger = 0.0; ledger[a].earned = 0; }

    AlignMap amap; memset(&amap, 0, sizeof(amap));
    AlignMap amap_shuf; memset(&amap_shuf, 0, sizeof(amap_shuf));
    int have_align = 0;

    for (long ch = 0; ch < n_chunks; ch++) {
        long off = ch * chunk_size;
        long blen = (off + chunk_size <= dn) ? chunk_size : (dn - off);
        int past_warmup = (off >= WARMUP_BYTES);

        int32_t *chunk_units = bytes_to_units(dest + off, blen);
        TokStream ctok = segment_with_merges(chunk_units, blen, cur_merges, n_cur_merges);
        free(chunk_units);

        int32_t c2ctx = -1, c1ctx = -1;
        if (prefix_tok.n >= 2) { c2ctx = prefix_tok.tok[prefix_tok.n-2]; c1ctx = prefix_tok.tok[prefix_tok.n-1]; }
        else if (prefix_tok.n == 1) c1ctx = prefix_tok.tok[0];

        /* per-chunk oracle map + realignment (only once past warmup) */
        OracleMap omap; memset(&omap, 0, sizeof(omap));
        int32_t *oracle_dest_to_A = NULL; /* inverse of omap.map (A-id -> dest-id) */
        if (past_warmup) {
            if (xform->kind != XFORM_NONE) {
                omap = build_oracle_map(past->bs, past->V, dest_bs, V, xform);
                oracle_dest_to_A = malloc((size_t)(V>0?V:1) * sizeof(int32_t));
                for (long i = 0; i < V; i++) oracle_dest_to_A[i] = -1;
                for (long i = 0; i < past->V; i++) {
                    int32_t dv = omap.map[i];
                    if (dv >= 0 && dv < V) oracle_dest_to_A[dv] = (int32_t)i;
                }
            }
            if (have_align) { free_align_map(&amap); free_align_map(&amap_shuf); }
            amap = build_align_map(&dd, ct.n1, &past->desc, past->ct.n1, NULL);
            amap_shuf = build_align_map(&dd, ct.n1, &past->desc, past->ct.n1, shuf_map);
            have_align = 1;
        }

        CacheCtxInfo *cctx = calloc((size_t)(V>0?V:1), sizeof(CacheCtxInfo));
        unsigned char *cctx_done = calloc((size_t)(V>0?V:1), 1);
        CacheCtxInfo *cctx_s = calloc((size_t)(V>0?V:1), sizeof(CacheCtxInfo));
        unsigned char *cctx_s_done = calloc((size_t)(V>0?V:1), 1);

        double bits[N_ARMS]; for (int a = 0; a < N_ARMS; a++) bits[a] = 0.0;
        long undef[N_ARMS]; for (int a = 0; a < N_ARMS; a++) undef[a] = 0;

        for (long k = 0; k < ctok.n; k++) {
            int32_t cc2, cc1;
            if (k == 0) { cc2 = c2ctx; cc1 = c1ctx; }
            else if (k == 1) { cc2 = c1ctx; cc1 = ctok.tok[0]; }
            else { cc2 = ctok.tok[k-2]; cc1 = ctok.tok[k-1]; }
            int32_t truth = ctok.tok[k];

            double Plocal = price_unit(&ct, n1_total, world_token_index, cc2, cc1, truth);
            bits[ARM_COLD] += -log2(Plocal);

            int c1_ok = (cc1 >= 0 && cc1 < dd.V && dd.alive[cc1]);

            if (c1_ok && !cctx_done[cc1]) {
                cctx[cc1] = compute_cache_ctx(cc1, &dd, &past->desc, past->ct.fwd, past->ct.n_fwd, NULL);
                cctx_done[cc1] = 1;
            }
            if (c1_ok && !cctx_s_done[cc1]) {
                cctx_s[cc1] = compute_cache_ctx(cc1, &dd, &past->desc, past->ct.fwd, past->ct.n_fwd, shuf_map);
                cctx_s_done[cc1] = 1;
            }
            double cache_raw = c1_ok ? cache_prior_raw(&cctx[cc1], &dd, &past->desc, NULL, truth) : -1.0;
            double cache_raw_s = c1_ok ? cache_prior_raw(&cctx_s[cc1], &dd, &past->desc, shuf_map, truth) : -1.0;

            double align_raw = -1.0, oracle_raw = -1.0, align_raw_s = -1.0;
            if (past_warmup) {
                align_raw = align_style_prior_raw(amap.dest_to_A, amap.A_to_dest, amap.V_dest, amap.V_A,
                                                   past->ct.fwd, past->ct.n_fwd, NULL, cc1, truth);
                align_raw_s = align_style_prior_raw(amap_shuf.dest_to_A, amap_shuf.A_to_dest, amap_shuf.V_dest, amap_shuf.V_A,
                                                     past->ct.fwd, past->ct.n_fwd, shuf_map, cc1, truth);
                if (xform->kind != XFORM_NONE)
                    oracle_raw = align_style_prior_raw(oracle_dest_to_A, omap.map, V, past->V,
                                                        past->ct.fwd, past->ct.n_fwd, NULL, cc1, truth);
            }

            bits[ARM_CACHE] += price_with_ledger(&ledger[ARM_CACHE], Plocal, escape_mix(cache_raw, Plocal), &undef[ARM_CACHE]);
            bits[ARM_ALIGN] += price_with_ledger(&ledger[ARM_ALIGN], Plocal, escape_mix(align_raw, Plocal), &undef[ARM_ALIGN]);

            if (!past_warmup) {
                bits[ARM_BOTH] += price_with_ledger(&ledger[ARM_BOTH], Plocal, escape_mix(cache_raw, Plocal), &undef[ARM_BOTH]);
                bits[ARM_SHUF] += price_with_ledger(&ledger[ARM_SHUF], Plocal, escape_mix(cache_raw_s, Plocal), &undef[ARM_SHUF]);
            } else {
                double c_comp = escape_mix(cache_raw, Plocal); if (c_comp < 0) c_comp = Plocal;
                double a_comp = escape_mix(align_raw, Plocal); if (a_comp < 0) a_comp = Plocal;
                double both_used = (cache_raw < 0.0 && align_raw < 0.0) ? -1.0 : (0.5*c_comp + 0.5*a_comp);
                bits[ARM_BOTH] += price_with_ledger(&ledger[ARM_BOTH], Plocal, both_used, &undef[ARM_BOTH]);

                double cs_comp = escape_mix(cache_raw_s, Plocal); if (cs_comp < 0) cs_comp = Plocal;
                double as_comp = escape_mix(align_raw_s, Plocal); if (as_comp < 0) as_comp = Plocal;
                double shuf_used = (cache_raw_s < 0.0 && align_raw_s < 0.0) ? -1.0 : (0.5*cs_comp + 0.5*as_comp);
                bits[ARM_SHUF] += price_with_ledger(&ledger[ARM_SHUF], Plocal, shuf_used, &undef[ARM_SHUF]);
            }

            bits[ARM_ORACLE] += price_with_ledger(&ledger[ARM_ORACLE], Plocal, escape_mix(oracle_raw, Plocal), &undef[ARM_ORACLE]);

            world_token_index++;
        }

        for (int a = 0; a < N_ARMS; a++) {
            ArmChunkResult *out = &cr.res[a][ch];
            out->chunk = ch; out->byte_offset = off; out->byte_len = blen;
            out->positions = ctok.n; out->bits = bits[a];
            out->L_end = ledger[a].earned ? ledger[a].L : 0.0;
            out->ledger_end = ledger[a].ledger;
            out->state_end = ledger[a].earned;
            out->undefined_positions = undef[a];
        }
        if (report && (ch < 3 || ch == n_chunks-1 || ch % 50 == 0))
            fprintf(stderr, "  [%s] chunk %ld off=%ld positions=%ld cold=%.3f cache=%.3f align=%.3f\n",
                    world_name, ch, off, ctok.n, bits[ARM_COLD], bits[ARM_CACHE], bits[ARM_ALIGN]);

        free(ctok.tok);
        free(cctx); free(cctx_done); free(cctx_s); free(cctx_s_done);
        free_oracle_map(&omap);
        free(oracle_dest_to_A);

        /* absorb + rebuild for next chunk */
        long prefix_len = off + blen;
        free(cur_merges);
        if (prefix_tok.tok) free(prefix_tok.tok);
        free_counts(&ct);
        free_descriptors(&dd);
        free_bytestrings(dest_bs, V);
        free(dest_ulen);
        int32_t *prefix_units = bytes_to_units(dest, prefix_len);
        prefix_tok = grow_bpe(prefix_units, prefix_len, 2048, 4, &cur_merges, &n_cur_merges);
        free(prefix_units);
        V = 256 + n_cur_merges;
        ct = build_counts(prefix_tok.tok, prefix_tok.n, V);
        n1_total = 0;
        for (long i = 0; i < V; i++) n1_total += ct.n1[i];
        dest_ulen = build_unit_len(cur_merges, n_cur_merges);
        dd = compute_descriptors(prefix_tok.tok, prefix_tok.n, V, ct.n1, dest_ulen);
        dest_bs = build_bytestrings(cur_merges, n_cur_merges);
    }
    free(cur_merges);
    if (prefix_tok.tok) free(prefix_tok.tok);
    free_counts(&ct);
    free_descriptors(&dd);
    free_bytestrings(dest_bs, V);
    free(dest_ulen);
    if (have_align) { free_align_map(&amap); free_align_map(&amap_shuf); }
    return cr;
}

/* ---------------- main (stage 1+2 driver) ---------------- */

int main(void) {
    long A_n;
    unsigned char *A = read_file("/Users/ataeff/arianna/netta/netta.txt", &A_n);
    {
        char hex[65];
        sha256_hex(A, A_n, hex);
        const char *expect_A = "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb";
        printf("A.sha256=%s len=%ld match=%d\n", hex, A_n,
               (strcmp(hex, expect_A) == 0 && A_n == 447545));
    }
    long train_len = (long)(0.9 * (double)A_n);
    printf("A-train length: %ld\n", train_len);

    /* ---- worlds ---- */
    unsigned char perm[256];
    build_cipher_perm(perm, 0xC1F3E5ULL);
    unsigned char *W_iso = build_w_iso(A, A_n, perm);
    unsigned char *W_ghost = build_w_ghost(A, train_len, A_n, 0x5EED01ULL);
    unsigned char rank_word[16][32]; int rank_len[16];
    long W_ff_n;
    unsigned char *W_ff = build_w_ff(A, A_n, train_len, &W_ff_n, rank_word, rank_len);

    char hex_iso[65], hex_ghost[65], hex_ff[65];
    sha256_hex(W_iso, A_n, hex_iso);
    sha256_hex(W_ghost, A_n, hex_ghost);
    sha256_hex(W_ff, W_ff_n, hex_ff);

    const char *expect_iso = "6f6816cfe77f952d7cba723d1068f6d637de7156a3a80a366a5002fcd22306c6";
    const char *expect_ghost = "b422ad11aa911c650c72ca99e66356a20eeb79fdf2c07f09e316f3829f4e8af1";
    const char *expect_ff = "9caae8c103896bea71b80276406239aa2f251b90a256f546827e02e4d88fbc47";
    /* NOTE: expected strings above are 65 chars incl NUL slot check below */
    printf("W_iso   sha256=%s  match=%d  (manifest %.64s)\n", hex_iso, strncmp(hex_iso, expect_iso, 64)==0, expect_iso);
    printf("W_ghost sha256=%s  match=%d\n", hex_ghost, strncmp(hex_ghost, expect_ghost, 64)==0);
    printf("W_ff    sha256=%s  match=%d  len=%ld (expect 445973)\n", hex_ff, strncmp(hex_ff, expect_ff, 64)==0, W_ff_n);

    /* ---- A-side past (built once from A-train) ---- */
    fprintf(stderr, "building A-side past (BPE on A-train, %ld bytes)...\n", train_len);
    APast past = build_a_past(A, train_len);
    fprintf(stderr, "past: A-train %ld B | merges %d | units %ld | stream %ld\n",
            train_len, past.n_merges, past.V, past.tok.n);

    /* ---- cipher isomorphism proof ---- */
    long iso_matched, iso_total; int iso_cipher_merges;
    prove_isomorphism(A, train_len, perm, past.merges, past.n_merges, &iso_matched, &iso_total, &iso_cipher_merges);
    fprintf(stderr, "isomorphism: matched=%ld total=%ld A_merges=%d cipher_merges=%d\n",
            iso_matched, iso_total, past.n_merges, iso_cipher_merges);

    /* ---- ghost raw invariants (verifier-computed, independent of ghost_invariants.tsv) ---- */
    double ghost_L1, ghost_MI;
    {
        long a_counts[256] = {0}, g_counts[256] = {0};
        for (long i = 0; i < train_len; i++) a_counts[A[i]]++;
        for (long i = 0; i < A_n; i++) g_counts[W_ghost[i]]++;
        double l1 = 0.0;
        for (int b = 0; b < 256; b++) {
            double pa = (double)a_counts[b] / (double)train_len;
            double pg = (double)g_counts[b] / (double)A_n;
            l1 += fabs(pa - pg);
        }
        ghost_L1 = l1;
        /* raw bigram MI on W_ghost itself */
        long *bi = calloc(256*256, sizeof(long));
        for (long i = 0; i < A_n - 1; i++) bi[W_ghost[i]*256 + W_ghost[i+1]]++;
        long total_bi = A_n - 1;
        double mi = 0.0;
        for (int x = 0; x < 256; x++) {
            double px = (double)g_counts[x] / (double)A_n;
            if (px <= 0) continue;
            for (int y = 0; y < 256; y++) {
                long c = bi[x*256+y];
                if (c == 0) continue;
                double pxy = (double)c / (double)total_bi;
                double py = (double)g_counts[y] / (double)A_n;
                if (py <= 0) continue;
                mi += pxy * log2(pxy / (px*py));
            }
        }
        ghost_MI = mi;
        free(bi);
        fprintf(stderr, "ghost raw invariants (verifier): unigram L1=%.6f (bound <=0.01) | bigram MI=%.6f bits (bound <=0.01)\n", ghost_L1, ghost_MI);
    }

    /* ---- ghost consumed-level stats (open point: no frozen bound) ---- */
    double ghost_unit_H1 = 0.0, ghost_unit_MI = 0.0; long ghost_final_units = 0, ghost_final_stream = 0;
    {
        int32_t *units = bytes_to_units(W_ghost, A_n);
        Merge *gm; int n_gm;
        TokStream gt = grow_bpe(units, A_n, 2048, 4, &gm, &n_gm);
        free(units);
        long V = 256 + n_gm;
        long *n1 = build_n1(gt.tok, gt.n, V);
        long total = gt.n;
        double H1 = 0.0;
        for (long u = 0; u < V; u++) if (n1[u] > 0) { double p = (double)n1[u]/(double)total; H1 += -p*log2(p); }
        long n_fwd1; Pair2 *fwd1 = build_dist_pairs(gt.tok, gt.n, 1, &n_fwd1);
        double mi = 0.0;
        long m2 = gt.n - 1;
        for (long i = 0; i < n_fwd1; i++) {
            double pxy = (double)fwd1[i].c / (double)m2;
            double px = (double)n1[fwd1[i].a] / (double)total;
            double py = (double)n1[fwd1[i].b] / (double)total;
            mi += pxy * log2(pxy / (px*py));
        }
        ghost_unit_H1 = H1; ghost_unit_MI = mi;
        ghost_final_units = V; ghost_final_stream = gt.n;
        fprintf(stderr, "ghost consumed-level (verifier, OPEN POINT no frozen bound): unit H1=%.4f bits | unit bigram MI=%.4f bits | units=%ld stream=%ld\n",
                ghost_unit_H1, ghost_unit_MI, ghost_final_units, ghost_final_stream);
        free(n1); free(fwd1); free(gm); free(gt.tok);
    }

    /* ---- shuffle map (A alive units, seed 0x54AFF1E) ---- */
    int32_t *shuf_map = build_shuffle_map(&past.desc, 0x54AFF1EULL);

    /* ---- oracle transforms ---- */
    OracleXform xform_iso; memset(&xform_iso, 0, sizeof(xform_iso));
    xform_iso.kind = XFORM_CIPHER;
    memcpy(xform_iso.cipher_perm, perm, 256);

    OracleXform xform_ghost; memset(&xform_ghost, 0, sizeof(xform_ghost));
    xform_ghost.kind = XFORM_NONE;

    OracleXform xform_ff; memset(&xform_ff, 0, sizeof(xform_ff));
    xform_ff.kind = XFORM_FF;
    memcpy(xform_ff.rank_word, rank_word, sizeof(rank_word));
    memcpy(xform_ff.rank_len, rank_len, sizeof(rank_len));

    /* ---- run the full court per world ---- */
    fprintf(stderr, "running full court: W-iso (%ld bytes)...\n", A_n);
    CourtResult cr_iso = run_full_court(W_iso, A_n, &past, &xform_iso, shuf_map, 1, "iso");
    fprintf(stderr, "running full court: W-ghost (%ld bytes)...\n", A_n);
    CourtResult cr_ghost = run_full_court(W_ghost, A_n, &past, &xform_ghost, shuf_map, 1, "ghost");
    fprintf(stderr, "running full court: W-ff (%ld bytes)...\n", W_ff_n);
    CourtResult cr_ff = run_full_court(W_ff, W_ff_n, &past, &xform_ff, shuf_map, 1, "ff");

    /* ---- full per-chunk-per-arm dump (debug aid, NOT one of the required outputs) ---- */
    {
        CourtResult *crs[3] = { &cr_iso, &cr_ghost, &cr_ff };
        const char *names[3] = { "iso", "ghost", "ff" };
        for (int w = 0; w < 3; w++) {
            char path[512];
            snprintf(path, sizeof(path), "/private/tmp/claude-501/-Users-ataeff/c2fa4c8e-e745-48a4-aaab-ee818ef74980/scratchpad/my_evidence_%s.tsv", names[w]);
            FILE *f = fopen(path, "w");
            for (int a = 0; a < N_ARMS; a++) {
                for (long ch = 0; ch < crs[w]->n_chunks; ch++) {
                    ArmChunkResult *r = &crs[w]->res[a][ch];
                    fprintf(f, "%ld\t%ld\t%ld\t%s\t%ld\t%.17g\t%.6f\t%.6f\t%d\t%ld\n",
                            r->chunk, r->byte_offset, r->byte_len, ARM_NAME[a], r->positions, r->bits,
                            r->L_end, r->ledger_end, r->state_end, r->undefined_positions);
                }
            }
            fclose(f);
        }
    }

    /* ---- G_N computation ---- */
    long horizons[4] = {1024, 4096, 16384, 65536};
    CourtResult *crs[3] = { &cr_iso, &cr_ghost, &cr_ff };
    const char *wnames[3] = { "iso", "ghost", "ff" };
    double GN[3][N_ARMS][4]; /* [world][arm][horizon] */
    for (int w = 0; w < 3; w++) {
        for (int a = 0; a < N_ARMS; a++) {
            for (int h = 0; h < 4; h++) {
                long n_ch_needed = horizons[h] / 1024;
                double g = 0.0;
                for (long ch = 0; ch < n_ch_needed && ch < crs[w]->n_chunks; ch++)
                    g += crs[w]->res[ARM_COLD][ch].bits - crs[w]->res[a][ch].bits;
                GN[w][a][h] = g;
            }
        }
    }

    /* ---- write results_transfer.tsv ---- */
    {
        FILE *f = fopen("/Users/ataeff/arianna/netta/transfer0/results_transfer.tsv", "w");
        fprintf(f, "world\tarm\thorizon\tG_N\tfinal_state\tfinal_ledger\tfinal_L\n");
        for (int w = 0; w < 3; w++) {
            for (int a = 0; a < N_ARMS; a++) {
                long last_ch = crs[w]->n_chunks - 1;
                ArmChunkResult *last = &crs[w]->res[a][last_ch];
                for (int h = 0; h < 4; h++) {
                    fprintf(f, "%s\t%s\t%ld\t%.6f\t%d\t%.6f\t%.6f\n",
                            wnames[w], ARM_NAME[a], horizons[h], GN[w][a][h],
                            last->state_end, last->ledger_end, last->L_end);
                }
            }
        }
        fclose(f);
    }

    /* ---- shadow gate self-check (state=0 rows must equal cold bits exactly) ---- */
    long shadow_violations = 0;
    for (int w = 0; w < 3; w++) {
        for (int a = 1; a < N_ARMS; a++) {
            for (long ch = 0; ch < crs[w]->n_chunks; ch++) {
                ArmChunkResult *r = &crs[w]->res[a][ch];
                if (r->state_end == 0) {
                    /* state at CHUNK END is 0; if it was ALSO 0 at chunk START (i.e. never
                       earned during this chunk) bits must equal cold's bits exactly. We only
                       have chunk-end state here, so this checks the common case (arms that
                       stayed unearned the whole chunk); a chunk that earns-then-revokes within
                       itself would show state_end=0 but bits != cold, which is NOT a violation
                       of the frozen law (mid-chunk earn/revoke is legal) -- flagged separately. */
                    double cold_bits = crs[w]->res[ARM_COLD][ch].bits;
                    if (fabs(r->bits - cold_bits) > 1e-6 * (fabs(cold_bits) + 1.0)) {
                        shadow_violations++;
                    }
                }
            }
        }
    }
    fprintf(stderr, "shadow-gate raw scan: %ld chunk/arm rows with state_end=0 but bits != cold bits "
                     "(may include legitimate mid-chunk earn-then-revoke; see report)\n", shadow_violations);

    /* ---- PASS/FAIL verdict ---- */
    {
        FILE *f = fopen("/Users/ataeff/arianna/netta/transfer0/verdict_transfer.md", "w");
        fprintf(f, "# NETTA Transfer Court — Independent Verdict\n\n");
        fprintf(f, "Computed entirely from this program's own repriced numbers, per TRANSFER_PROTOCOL.md.\n\n");
        fprintf(f, "## G_N table (bits)\n\n");
        fprintf(f, "| world | arm | G_1024 | G_4096 | G_16384 | G_65536 |\n|---|---|---|---|---|---|\n");
        for (int w = 0; w < 3; w++)
            for (int a = 0; a < N_ARMS; a++)
                fprintf(f, "| %s | %s | %.2f | %.2f | %.2f | %.2f |\n", wnames[w], ARM_NAME[a],
                        GN[w][a][0], GN[w][a][1], GN[w][a][2], GN[w][a][3]);

        double MARGIN = 0.01 * 16384.0;
        fprintf(f, "\nDeciding horizon N=16384, MARGIN=%.2f bits.\n\n", MARGIN);

        fprintf(f, "## Anti-smoothing difference D = G_16384(arm,iso) - G_16384(arm,ghost)\n\n");
        for (int a = 1; a < N_ARMS; a++) {
            double D = GN[0][a][2] - GN[1][a][2];
            fprintf(f, "- %s: D = %.2f bits\n", ARM_NAME[a], D);
        }

        fprintf(f, "\n## Gates\n\n");
        int ff_fail[N_ARMS] = {0}, ghost_fail[N_ARMS] = {0}, interferes_any[N_ARMS] = {0};
        for (int a = 1; a < N_ARMS; a++) {
            if (GN[2][a][2] <= -MARGIN) { ff_fail[a] = 1; fprintf(f, "- %s: FALSE-FRIEND GATE FAILED on W-ff (\"surface authority leaked\"), G_16384=%.2f\n", ARM_NAME[a], GN[2][a][2]); }
            if (GN[1][a][2] <= -MARGIN) { ghost_fail[a] = 1; fprintf(f, "- %s: NON-IMPOSITION GATE FAILED on W-ghost (\"past imposed on a strange world\"), G_16384=%.2f\n", ARM_NAME[a], GN[1][a][2]); }
            for (int w = 0; w < 3; w++) if (GN[w][a][2] <= -MARGIN) interferes_any[a] = 1;
        }

        fprintf(f, "\n## Verdicts (competition arms: cache, align, both)\n\n");
        int any_pass = 0;
        for (int a = ARM_CACHE; a <= ARM_BOTH; a++) {
            double g_iso = GN[0][a][2];
            double g_shuf = GN[0][ARM_SHUF][2];
            double D = GN[0][a][2] - GN[1][a][2];
            int beats_margin = (g_iso >= MARGIN);
            int beats_shuf = (g_iso - g_shuf >= MARGIN);
            int d_ok = (D >= MARGIN);
            int ff_ok = !ff_fail[a];
            int pass = beats_margin && beats_shuf && d_ok && ff_ok;
            fprintf(f, "- **%s**: G_16384(iso)=%.2f (need >=%.2f: %s) | beats shuffled by %.2f (need >=%.2f: %s) | D=%.2f (need >=%.2f: %s) | false-friend gate: %s => %s\n",
                    ARM_NAME[a], g_iso, MARGIN, beats_margin?"yes":"no",
                    g_iso-g_shuf, MARGIN, beats_shuf?"yes":"no",
                    D, MARGIN, d_ok?"yes":"no",
                    ff_ok?"passes":"FAILS",
                    pass ? "TRANSFER EARNED (PREDICTIVELY)" : "not earned");
            if (pass) any_pass = 1;
        }

        fprintf(f, "\n## Overall\n\n");
        if (any_pass) {
            fprintf(f, "At least one competition arm meets \"Transfer earned (predictively)\" at N=16384. Ceiling: predictive status only.\n");
        } else {
            double g_oracle_iso = GN[0][ARM_ORACLE][2];
            fprintf(f, "\"Transfer not detected\": no competition arm meets the PASS wording at N=16384.\n");
            if (g_oracle_iso < MARGIN) {
                fprintf(f, "\nAdditionally, oracle itself fails G_16384>=MARGIN on W-iso (oracle G_16384=%.2f < %.2f): "
                            "\"transfer not detected: carried memory insufficient\" — the matcher is exonerated, the memory itself is the failure.\n",
                        g_oracle_iso, MARGIN);
            }
        }

        fprintf(f, "\n## Interference (any arm, any world, G_16384 <= -MARGIN)\n\n");
        int any_interferes = 0;
        for (int a = 1; a < N_ARMS; a++) if (interferes_any[a]) {
            any_interferes = 1;
            for (int w = 0; w < 3; w++) if (GN[w][a][2] <= -MARGIN)
                fprintf(f, "- %s on %s: G_16384=%.2f <= -%.2f -> \"past experience interferes\"\n", ARM_NAME[a], wnames[w], GN[w][a][2], MARGIN);
        }
        if (!any_interferes) fprintf(f, "None.\n");

        fprintf(f, "\n## Ghost invariants (verifier-computed)\n\n");
        fprintf(f, "- Raw: unigram L1=%.6f (bound <=0.01, %s) | bigram MI=%.6f bits (bound <=0.01, %s)\n",
                ghost_L1, ghost_L1<=0.01?"holds":"VIOLATED", ghost_MI, ghost_MI<=0.01?"holds":"VIOLATED");
        fprintf(f, "- Consumed (post-unit) level, OPEN POINT — the protocol froze raw-level thresholds only, no bound exists at this level: "
                    "unit unigram entropy=%.4f bits, unit bigram MI=%.4f bits (units=%ld, stream=%ld tokens)\n",
                ghost_unit_H1, ghost_unit_MI, ghost_final_units, ghost_final_stream);

        fprintf(f, "\n## Isomorphism (W-iso construction)\n\n");
        fprintf(f, "- A-train merges: %d | cipher(A-train) merges: %d | matched in lockstep: %ld / %ld\n",
                past.n_merges, iso_cipher_merges, iso_matched, iso_total);

        fprintf(f, "\n## World reconstruction\n\n");
        fprintf(f, "- W-iso sha256=%s match=%d\n", hex_iso, strncmp(hex_iso, expect_iso, 64)==0);
        fprintf(f, "- W-ghost sha256=%s match=%d\n", hex_ghost, strncmp(hex_ghost, expect_ghost, 64)==0);
        fprintf(f, "- W-ff sha256=%s match=%d len=%ld\n", hex_ff, strncmp(hex_ff, expect_ff, 64)==0, W_ff_n);

        fclose(f);
    }

    fprintf(stderr, "done. results_transfer.tsv and verdict_transfer.md written.\n");

    free(A); free(W_iso); free(W_ghost); free(W_ff);
    return 0;
}
