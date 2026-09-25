/* netta_mouth.c -- NETTA Body 1: the mouth, under MOUTH_PROTOCOL.md.

   The organism speaks without the mycelium.  Life on a world grows
   earned units (Body 0's frozen merge law); speech is sampled ONLY over
   lived continuations, quad -> tri -> bi -> lived unigrams.  There is
   no smoothing hand in the voice and no field: Body 0's verdict deleted
   the field organ, and the mouth does not resurrect it.

   Court-4 citizens (live earned relations from the sealed book) advise
   sampling only through the destination-byte projection declared in the
   contract.  Court 4's single-winner law is preserved: greatest ledger,
   full RelationKey tie-break; citizens are never multiplied.  Controls:
   --citizens-mode none (plain mouth) and shuffled (deterministic rotation
   of target identities) run beside the advised mouth.

   Iteration is the law: --merges, --order, --temp, --topk, --bytes are
   open dials; the same flags and seed speak the same bytes.

   usage: netta_mouth <world> --out <dir>
            [--merges N=4096] [--min-pair N=4] [--order 4|3=4]
            [--bytes N=700] [--seeds a,b,c,...=7,19,42,101,271]
            [--temp X=0.8] [--topk N=15] [--corridor K=3]
            [--citizens FILE --citizens-mode live|none|shuffled=none]

   C11, stdlib only.  Deterministic.                                   */

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_UNITS 256
#define PACK 16 /* unit id bits in packed n-gram keys; V must stay below 65536 */
#define PACK_MASK ((1u << PACK) - 1u)
#define REP_WINDOW 12
#define REP_PENALTY 0.5
#define SPEAK_HARD 256
#define MAX_SEEDS 32
#define MAX_CITIZENS 64
#define MAX_EM 65536
#define MAX_SPEAK_BYTES 65000
#define CITIZENS_BYTES 36101u

static const uint8_t CITIZENS_SHA256[32] = {
    0xd3,0xe5,0xe5,0x14,0xea,0x4c,0xcc,0xc4,
    0xa0,0x44,0xad,0x22,0x4d,0x19,0x17,0x0e,
    0x63,0x7b,0xeb,0x79,0xa8,0x10,0xfb,0x08,
    0x75,0x81,0x6f,0x79,0x82,0x11,0xb4,0x38
};

static void die(const char *m) { fprintf(stderr, "netta_mouth: %s\n", m); exit(1); }

/* Small local SHA-256: the mouth authenticates the sealed citizens book
   without invoking a shell or accepting a path as identity. */
typedef struct {
    uint32_t h[8];
    uint64_t bits;
    uint8_t block[64];
    size_t used;
} Sha256;

static uint32_t rotr32(uint32_t x, unsigned n) { return (x >> n) | (x << (32u - n)); }

static void sha256_block(Sha256 *s, const uint8_t b[64]) {
    static const uint32_t k[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t w[64];
    for (unsigned i = 0; i < 16; i++)
        w[i] = ((uint32_t)b[4*i] << 24) | ((uint32_t)b[4*i+1] << 16) |
               ((uint32_t)b[4*i+2] << 8) | (uint32_t)b[4*i+3];
    for (unsigned i = 16; i < 64; i++) {
        uint32_t x = w[i-15], y = w[i-2];
        uint32_t a = rotr32(x,7) ^ rotr32(x,18) ^ (x >> 3);
        uint32_t z = rotr32(y,17) ^ rotr32(y,19) ^ (y >> 10);
        w[i] = w[i-16] + a + w[i-7] + z;
    }
    uint32_t a=s->h[0], c=s->h[2], d=s->h[3], e=s->h[4];
    uint32_t f=s->h[5], g=s->h[6], h=s->h[7], bb=s->h[1];
    for (unsigned i = 0; i < 64; i++) {
        uint32_t s1=rotr32(e,6)^rotr32(e,11)^rotr32(e,25);
        uint32_t ch=(e&f)^((~e)&g), t1=h+s1+ch+k[i]+w[i];
        uint32_t s0=rotr32(a,2)^rotr32(a,13)^rotr32(a,22);
        uint32_t maj=(a&bb)^(a&c)^(bb&c), t2=s0+maj;
        h=g; g=f; f=e; e=d+t1; d=c; c=bb; bb=a; a=t1+t2;
    }
    s->h[0]+=a; s->h[1]+=bb; s->h[2]+=c; s->h[3]+=d;
    s->h[4]+=e; s->h[5]+=f; s->h[6]+=g; s->h[7]+=h;
}

static void sha256_init(Sha256 *s) {
    static const uint32_t iv[8] = {0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
                                    0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u};
    memcpy(s->h, iv, sizeof iv); s->bits = 0; s->used = 0;
}

static void sha256_update(Sha256 *s, const uint8_t *p, size_t n) {
    s->bits += (uint64_t)n * 8u;
    while (n) {
        size_t take = 64u - s->used;
        if (take > n) take = n;
        memcpy(s->block + s->used, p, take);
        s->used += take; p += take; n -= take;
        if (s->used == 64u) { sha256_block(s, s->block); s->used = 0; }
    }
}

static void sha256_final(Sha256 *s, uint8_t out[32]) {
    uint64_t bits = s->bits;
    s->block[s->used++] = 0x80;
    if (s->used > 56u) {
        while (s->used < 64u) s->block[s->used++] = 0;
        sha256_block(s, s->block); s->used = 0;
    }
    while (s->used < 56u) s->block[s->used++] = 0;
    for (unsigned i = 0; i < 8; i++) s->block[63u-i] = (uint8_t)(bits >> (8u*i));
    sha256_block(s, s->block);
    for (unsigned i = 0; i < 8; i++) {
        out[4*i]=(uint8_t)(s->h[i]>>24); out[4*i+1]=(uint8_t)(s->h[i]>>16);
        out[4*i+2]=(uint8_t)(s->h[i]>>8); out[4*i+3]=(uint8_t)s->h[i];
    }
}

static void sha256_stream(FILE *f, uint8_t out[32], size_t *bytes) {
    Sha256 s; uint8_t buf[16384]; size_t total = 0, n;
    sha256_init(&s);
    for (;;) {
        n = fread(buf, 1, sizeof buf, f);
        if (n) { sha256_update(&s, buf, n); total += n; }
        if (n != sizeof buf) {
            if (ferror(f)) die("cannot hash file");
            break;
        }
    }
    sha256_final(&s, out);
    if (bytes) *bytes = total;
}

/* ── dials ── */
static uint32_t MERGES = 4096;
static uint32_t MIN_PAIR = 4;
static int ORDER = 4;
static size_t SPEAK_BYTES = 700;
static double TEMP = 0.8;
static size_t TOP_K = 15;
static uint32_t CORRIDOR = 3; /* Amendment 2: 0 disables the corridor law */

/* ── rng (frozen xorshift64, Body 0's law) ── */
static uint64_t rng_state;
static uint64_t rng_next(void) {
    uint64_t x = rng_state;
    x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    return rng_state = x;
}
static size_t rng_below(size_t n) { return (size_t)(rng_next() % (uint64_t)n); }
static double rng_double(void) { return (double)(rng_next() >> 11) / (double)(1ull << 53); }

/* ── world: the mouth lives all of it ── */
static uint8_t *world;
static size_t world_n;

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
}

/* ── units: Body 0's frozen BPE law, budget is a dial ── */
static uint32_t *merge_left, *merge_right;
static uint32_t nmerges, max_units;
static uint8_t *exp_pool;
static size_t exp_pool_cap, exp_pool_len;
static size_t *exp_off;
static uint32_t *exp_lenv;
static uint32_t nunits;

static void exp_append(uint32_t id, const uint8_t *b, uint32_t len) {
    if ((size_t)len > SIZE_MAX - exp_pool_len) die("unit expansion pool overflow");
    if (exp_pool_len + len > exp_pool_cap) {
        if (!exp_pool_cap) exp_pool_cap = 1u << 20;
        while (exp_pool_len + len > exp_pool_cap) {
            if (exp_pool_cap > SIZE_MAX / 2) die("unit expansion pool overflow");
            exp_pool_cap *= 2;
        }
        exp_pool = realloc(exp_pool, exp_pool_cap);
        if (!exp_pool) die("oom pool");
    }
    exp_off[id] = exp_pool_len;
    exp_lenv[id] = len;
    memcpy(exp_pool + exp_pool_len, b, len);
    exp_pool_len += len;
}

/* The merge law is unchanged, but its evidence is kept instead of recounted.
   Positions never move: a replacement lives at its left byte position and the
   right position leaves the linked stream.  Pair occurrence vectors are lazy;
   stale positions are rejected when their pair next reaches the frontier. */
typedef struct {
    uint64_t key;
    uint32_t count;
    size_t *pos;
    size_t npos, cap;
    int used;
} PairState;

typedef struct {
    uint64_t key;
    uint32_t count;
} PairHeapEntry;

static PairState *pair_tab;
static size_t pair_cap, pair_used;
static PairHeapEntry *pair_heap;
static size_t pair_heap_n, pair_heap_cap;
static uint32_t *link_sym;
static size_t *link_prev, *link_next;
static size_t link_n;

static uint64_t pair_hash(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

static int heap_better(PairHeapEntry a, PairHeapEntry b) {
    return a.count > b.count || (a.count == b.count && a.key < b.key);
}

static void heap_push(uint64_t key, uint32_t count) {
    if (!count) return;
    if (pair_heap_n == pair_heap_cap) {
        size_t nc = pair_heap_cap ? pair_heap_cap * 2u : 4096u;
        if (nc < pair_heap_cap || nc > SIZE_MAX / sizeof *pair_heap) die("pair heap overflow");
        pair_heap = realloc(pair_heap, nc * sizeof *pair_heap);
        if (!pair_heap) die("oom pair heap");
        pair_heap_cap = nc;
    }
    size_t i = pair_heap_n++;
    PairHeapEntry e = {key, count};
    while (i) {
        size_t p = (i - 1u) / 2u;
        if (!heap_better(e, pair_heap[p])) break;
        pair_heap[i] = pair_heap[p];
        i = p;
    }
    pair_heap[i] = e;
}

static PairHeapEntry heap_pop(void) {
    if (!pair_heap_n) die("empty pair heap");
    PairHeapEntry top = pair_heap[0];
    PairHeapEntry tail = pair_heap[--pair_heap_n];
    if (pair_heap_n) {
        size_t i = 0;
        for (;;) {
            size_t left = i * 2u + 1u;
            if (left >= pair_heap_n) break;
            size_t right = left + 1u;
            size_t child = right < pair_heap_n && heap_better(pair_heap[right], pair_heap[left])
                         ? right : left;
            if (!heap_better(pair_heap[child], tail)) break;
            pair_heap[i] = pair_heap[child];
            i = child;
        }
        pair_heap[i] = tail;
    }
    return top;
}

static size_t pair_slot_in(PairState *tab, size_t cap, uint64_t key) {
    size_t h = (size_t)pair_hash(key) & (cap - 1u);
    while (tab[h].used && tab[h].key != key) h = (h + 1u) & (cap - 1u);
    return h;
}

static void pair_rehash(size_t cap) {
    PairState *old = pair_tab;
    size_t old_cap = pair_cap;
    pair_tab = calloc(cap, sizeof *pair_tab);
    if (!pair_tab) die("oom pair table");
    pair_cap = cap;
    for (size_t i = 0; i < old_cap; i++) if (old[i].used) {
        size_t h = pair_slot_in(pair_tab, pair_cap, old[i].key);
        pair_tab[h] = old[i];
    }
    free(old);
}

static PairState *pair_get(uint64_t key, int create) {
    if (!pair_cap) pair_rehash(1u << 17);
    if (create && (pair_used + 1u) * 10u >= pair_cap * 7u) pair_rehash(pair_cap * 2u);
    size_t h = pair_slot_in(pair_tab, pair_cap, key);
    if (!pair_tab[h].used) {
        if (!create) return NULL;
        pair_tab[h].used = 1;
        pair_tab[h].key = key;
        pair_used++;
    }
    return &pair_tab[h];
}

static void pair_position(PairState *p, size_t pos) {
    if (p->npos == p->cap) {
        size_t nc = p->cap ? p->cap * 2u : 4u;
        if (nc < p->cap || nc > SIZE_MAX / sizeof *p->pos) die("pair positions overflow");
        p->pos = realloc(p->pos, nc * sizeof *p->pos);
        if (!p->pos) die("oom pair positions");
        p->cap = nc;
    }
    p->pos[p->npos++] = pos;
}

static void pair_seed(uint64_t key, size_t pos) {
    PairState *p = pair_get(key, 1);
    if (p->count == UINT32_MAX) die("pair count overflow");
    p->count++;
    pair_position(p, pos);
}

static void pair_add(uint64_t key, size_t pos) {
    PairState *p = pair_get(key, 1);
    if (p->count == UINT32_MAX) die("pair count overflow");
    p->count++;
    pair_position(p, pos);
    heap_push(key, p->count);
}

static void pair_remove(uint64_t key) {
    PairState *p = pair_get(key, 0);
    if (!p || !p->count) die("pair frontier underflow");
    p->count--;
}

static int cmp_size(const void *va, const void *vb) {
    size_t a = *(const size_t *)va, b = *(const size_t *)vb;
    return (a > b) - (a < b);
}

static PairState *pair_best(uint64_t *key, uint32_t *count) {
    while (pair_heap_n) {
        PairHeapEntry e = heap_pop();
        PairState *p = pair_get(e.key, 0);
        if (!p) die("pair heap lost its key");
        if (p->count == e.count) {
            *key = e.key;
            *count = e.count;
            return p;
        }
        heap_push(e.key, p->count);
    }
    return NULL;
}

static void pair_frontier_free(void) {
    for (size_t i = 0; i < pair_cap; i++) if (pair_tab[i].used) free(pair_tab[i].pos);
    free(pair_tab); pair_tab = NULL; pair_cap = pair_used = 0;
    free(pair_heap); pair_heap = NULL; pair_heap_n = pair_heap_cap = 0;
    free(link_prev); link_prev = NULL;
    free(link_next); link_next = NULL;
    link_sym = NULL; link_n = 0;
}

static void grow_units(uint32_t *t, size_t *pn) {
    const size_t none = SIZE_MAX;
    link_sym = t;
    link_n = *pn;
    link_prev = malloc(link_n * sizeof *link_prev);
    link_next = malloc(link_n * sizeof *link_next);
    if (!link_prev || !link_next) die("oom unit links");
    for (size_t i = 0; i < link_n; i++) {
        link_prev[i] = i ? i - 1u : none;
        link_next[i] = i + 1u < link_n ? i + 1u : none;
    }
    for (size_t i = 0; i + 1u < link_n; i++)
        pair_seed(((uint64_t)link_sym[i] << 32) | link_sym[i + 1u], i);
    for (size_t i = 0; i < pair_cap; i++) if (pair_tab[i].used && pair_tab[i].count)
        heap_push(pair_tab[i].key, pair_tab[i].count);

    size_t live = link_n;
    while (nmerges < MERGES) {
        uint64_t best_key = 0;
        uint32_t best_count = 0;
        PairState *best = pair_best(&best_key, &best_count);
        if (!best || best_count < MIN_PAIR) break;
        uint32_t a = (uint32_t)(best_key >> 32), b = (uint32_t)best_key;

        qsort(best->pos, best->npos, sizeof *best->pos, cmp_size);
        size_t valid = 0, previous = none;
        for (size_t k = 0; k < best->npos; k++) {
            size_t i = best->pos[k];
            if (i == previous) continue;
            previous = i;
            size_t j = link_next[i];
            if (link_sym[i] == a && j != none && link_sym[j] == b)
                best->pos[valid++] = i;
        }
        best->npos = valid;
        if (valid != best_count) die("pair frontier count drift");
        size_t *positions = best->pos;

        uint32_t id = nunits++;
        merge_left[nmerges] = a;
        merge_right[nmerges] = b;
        nmerges++;
        {
            uint32_t la = exp_lenv[a], lb = exp_lenv[b];
            uint8_t *tmp = malloc((size_t)la + lb);
            if (!tmp) die("oom");
            memcpy(tmp, exp_pool + exp_off[a], la);
            memcpy(tmp + la, exp_pool + exp_off[b], lb);
            exp_append(id, tmp, la + lb);
            free(tmp);
        }

        for (size_t k = 0; k < valid; k++) {
            size_t i = positions[k];
            size_t j = link_next[i];
            if (link_sym[i] != a || j == none || link_sym[j] != b) continue;
            size_t left = link_prev[i], right = link_next[j];
            uint32_t sl = left != none ? link_sym[left] : 0;
            uint32_t sr = right != none ? link_sym[right] : 0;

            pair_remove(best_key);
            if (left != none) pair_remove(((uint64_t)sl << 32) | a);
            if (right != none) pair_remove(((uint64_t)b << 32) | sr);

            link_sym[i] = id;
            link_sym[j] = UINT32_MAX;
            link_next[i] = right;
            if (right != none) link_prev[right] = i;
            link_prev[j] = link_next[j] = none;
            live--;

            if (left != none) pair_add(((uint64_t)sl << 32) | id, left);
            if (right != none) pair_add(((uint64_t)id << 32) | sr, i);
        }
    }

    size_t w = 0;
    for (size_t i = 0; i != none; i = link_next[i]) t[w++] = link_sym[i];
    if (w != live) die("linked stream length drift");
    *pn = w;
    pair_frontier_free();
}

/* ── lived tables: uni, bi, tri, quad over the lived stream ── */
static uint64_t *tbl_bi, *tbl_tri, *tbl_quad;
static size_t n_bi, n_tri, n_quad;
static uint32_t *n1;
static uint32_t *alive; static size_t nalive;

static int cmp_u64(const void *x, const void *y) {
    uint64_t a = *(const uint64_t *)x, b = *(const uint64_t *)y;
    return (a > b) - (a < b);
}

static void build_tables(const uint32_t *t, size_t n, uint32_t V) {
    n_bi = n >= 2 ? n - 1 : 0;
    n_tri = n >= 3 ? n - 2 : 0;
    n_quad = (ORDER >= 4 && n >= 4) ? n - 3 : 0;
    tbl_bi = malloc(n_bi * sizeof(uint64_t));
    tbl_tri = malloc(n_tri * sizeof(uint64_t));
    tbl_quad = n_quad ? malloc(n_quad * sizeof(uint64_t)) : NULL;
    n1 = calloc(V, sizeof(uint32_t));
    if ((n_bi && !tbl_bi) || (n_tri && !tbl_tri) || (n_quad && !tbl_quad) || !n1) die("oom tables");
    for (size_t i = 0; i < n_bi; i++)
        tbl_bi[i] = ((uint64_t)t[i] << PACK) | t[i + 1];
    for (size_t i = 0; i < n_tri; i++)
        tbl_tri[i] = ((uint64_t)t[i] << (2 * PACK)) | ((uint64_t)t[i + 1] << PACK) | t[i + 2];
    for (size_t i = 0; i < n_quad; i++)
        tbl_quad[i] = ((uint64_t)t[i] << (3 * PACK)) | ((uint64_t)t[i + 1] << (2 * PACK)) |
                      ((uint64_t)t[i + 2] << PACK) | t[i + 3];
    qsort(tbl_bi, n_bi, sizeof(uint64_t), cmp_u64);
    qsort(tbl_tri, n_tri, sizeof(uint64_t), cmp_u64);
    if (n_quad) qsort(tbl_quad, n_quad, sizeof(uint64_t), cmp_u64);
    for (size_t i = 0; i < n; i++) n1[t[i]]++;
    alive = malloc(V * sizeof(uint32_t));
    if (!alive) die("oom");
    nalive = 0;
    for (uint32_t u = 0; u < V; u++) if (n1[u]) alive[nalive++] = u;
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
        ccbuf[n].tok = tok;
        ccbuf[n].cnt = (uint32_t)(j - i);
        n++;
        i = j;
    }
    return n;
}

/* ── Court-4 citizens: authenticated book, projected context, one winner ── */
typedef struct {
    uint8_t target_s, target_d;
    uint32_t target_epoch;
    uint8_t cl;
    uint8_t ctx_s[3], ctx_d[3];
    uint32_t ctx_epoch[3];
    uint64_t seen, positive, negative;
    double ledger, peak, L;
    int state, ever_earned;
    unsigned book_row;
} Citizen;
static Citizen cit[MAX_CITIZENS];
static size_t ncit;
static int cit_mode; /* 0 none, 1 live, 2 shuffled */

static uint64_t parse_u64(const char *s, const char *what) {
    char *end = NULL;
    if (!s || !*s || *s == '-') die(what);
    errno = 0;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno || !end || *end) die(what);
    return (uint64_t)v;
}

static uint32_t parse_u32(const char *s, const char *what) {
    uint64_t v = parse_u64(s, what);
    if (v > UINT32_MAX) die(what);
    return (uint32_t)v;
}

static uint8_t parse_u8(const char *s, const char *what) {
    uint32_t v = parse_u32(s, what);
    if (v > UINT8_MAX) die(what);
    return (uint8_t)v;
}

static double parse_real(const char *s, const char *what) {
    char *end = NULL;
    errno = 0;
    double v = strtod(s, &end);
    if (!s || !*s || errno || !end || *end || !isfinite(v)) die(what);
    return v;
}

static size_t parse_size_arg(const char *s, const char *what) {
    uint64_t v = parse_u64(s, what);
    if (v > SIZE_MAX) die(what);
    return (size_t)v;
}

static size_t parse_seed_list(const char *s, uint64_t out[MAX_SEEDS]) {
    size_t n = 0;
    const char *p = s;
    if (!p || !*p) die("empty seed list");
    while (*p) {
        const char *start = p;
        while (*p && *p != ',') p++;
        size_t len = (size_t)(p - start);
        if (!len || len >= 64) die("bad seed list");
        char one[64];
        memcpy(one, start, len); one[len] = 0;
        if (n == MAX_SEEDS) die("too many seeds");
        uint64_t v = parse_u64(one, "bad seed");
        for (size_t i = 0; i < n; i++) if (out[i] == v) die("duplicate seed");
        out[n++] = v;
        if (*p == ',') { p++; if (!*p) die("bad seed list"); }
    }
    return n;
}

static int split_tabs(char *line, char **fld, int cap) {
    int n = 0;
    if (!line || !*line) return 0;
    fld[n++] = line;
    for (char *p = line; *p; p++) {
        if (*p == '\t') {
            *p = 0;
            if (n == cap) die("citizens row has too many fields");
            fld[n++] = p + 1;
        }
    }
    return n;
}

static int citizen_key_cmp(const Citizen *a, const Citizen *b) {
#define CMP(x) do { if (a->x != b->x) return a->x < b->x ? -1 : 1; } while (0)
    CMP(target_s); CMP(target_d); CMP(target_epoch); CMP(cl);
    for (int i = 0; i < 3; i++) {
        if (a->ctx_s[i] != b->ctx_s[i]) return a->ctx_s[i] < b->ctx_s[i] ? -1 : 1;
        if (a->ctx_d[i] != b->ctx_d[i]) return a->ctx_d[i] < b->ctx_d[i] ? -1 : 1;
        if (a->ctx_epoch[i] != b->ctx_epoch[i]) return a->ctx_epoch[i] < b->ctx_epoch[i] ? -1 : 1;
    }
#undef CMP
    return 0;
}

static void load_citizens(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open citizens book");
    uint8_t digest[32]; size_t bytes = 0;
    sha256_stream(f, digest, &bytes);
    if (bytes != CITIZENS_BYTES || memcmp(digest, CITIZENS_SHA256, sizeof digest))
        die("citizens book identity mismatch");
    if (fseek(f, 0, SEEK_SET)) die("cannot rewind citizens book");

    static const char header[] =
        "arm\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\t"
        "c2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\tseen\tpositive\tnegative\t"
        "ledger_bits\tpeak_bits\tstate\tL\tever_earned";
    char line[4096];
    unsigned row = 0;
    while (fgets(line, sizeof line, f)) {
        row++;
        size_t len = strlen(line);
        if (!len || line[len - 1] != '\n') die("citizens book has unterminated or oversized row");
        line[--len] = 0;
        if (len && line[len - 1] == '\r') die("citizens book is not canonical LF text");
        if (row == 1) {
            if (strcmp(line, header)) die("citizens book header drifted");
            continue;
        }
        char *fld[22];
        int nf = split_tabs(line, fld, 22);
        if (nf != 22) die("citizens book row width drifted");
        if (strcmp(fld[0], "relation")) continue;   /* oracle and nulls are controls, never advisers */
        if (strcmp(fld[19], "1")) continue;          /* state=1 only: live earned balance */
        if (ncit == MAX_CITIZENS) die("too many citizens");
        Citizen *c = &cit[ncit];
        memset(c, 0, sizeof *c);
        c->target_s = parse_u8(fld[1], "bad citizen target_s");
        c->target_d = parse_u8(fld[2], "bad citizen target_d");
        c->target_epoch = parse_u32(fld[3], "bad citizen target_epoch");
        c->cl = parse_u8(fld[4], "bad citizen context_len");
        c->ctx_s[0] = parse_u8(fld[5], "bad citizen c1_s");
        c->ctx_d[0] = parse_u8(fld[6], "bad citizen c1_d");
        c->ctx_epoch[0] = parse_u32(fld[7], "bad citizen c1_epoch");
        c->ctx_s[1] = parse_u8(fld[8], "bad citizen c2_s");
        c->ctx_d[1] = parse_u8(fld[9], "bad citizen c2_d");
        c->ctx_epoch[1] = parse_u32(fld[10], "bad citizen c2_epoch");
        c->ctx_s[2] = parse_u8(fld[11], "bad citizen c3_s");
        c->ctx_d[2] = parse_u8(fld[12], "bad citizen c3_d");
        c->ctx_epoch[2] = parse_u32(fld[13], "bad citizen c3_epoch");
        c->seen = parse_u64(fld[14], "bad citizen seen");
        c->positive = parse_u64(fld[15], "bad citizen positive");
        c->negative = parse_u64(fld[16], "bad citizen negative");
        c->ledger = parse_real(fld[17], "bad citizen ledger");
        c->peak = parse_real(fld[18], "bad citizen peak");
        c->state = (int)parse_u32(fld[19], "bad citizen state");
        c->L = parse_real(fld[20], "bad citizen L");
        c->ever_earned = (int)parse_u32(fld[21], "bad citizen ever_earned");
        c->book_row = row;
        if (c->cl != 1 || c->ctx_epoch[0] == 0 || c->target_epoch == 0 ||
            c->state != 1 || c->ever_earned != 1 || c->seen == 0 ||
            c->positive > c->seen || c->negative != c->seen - c->positive ||
            c->L <= 0.0 || c->L > 0.5)
            die("citizen outside the sealed law");
        ncit++;
    }
    if (ferror(f)) die("cannot read citizens book");
    fclose(f);
    if (ncit != 5) die("citizens book live population drifted");
    if (cit_mode == 2) { /* rotate the target identity bundle, preserving evidence/context */
        uint8_t s0 = cit[0].target_s, d0 = cit[0].target_d;
        uint32_t e0 = cit[0].target_epoch;
        for (size_t i = 0; i + 1 < ncit; i++) {
            cit[i].target_s = cit[i + 1].target_s;
            cit[i].target_d = cit[i + 1].target_d;
            cit[i].target_epoch = cit[i + 1].target_epoch;
        }
        cit[ncit - 1].target_s = s0;
        cit[ncit - 1].target_d = d0;
        cit[ncit - 1].target_epoch = e0;
    }
}

/* Court 4 chooses one eligible citizen before pricing a truth.  Here the
   full context has an explicitly declared destination-byte projection. */
static const Citizen *citizen_winner(uint8_t prev_byte) {
    const Citizen *winner = NULL;
    if (cit_mode == 0 || ncit == 0) return NULL;
    for (size_t i = 0; i < ncit; i++) {
        const Citizen *c = &cit[i];
        if (c->cl != 1 || c->ctx_d[0] != prev_byte) continue;
        if (!winner || c->ledger > winner->ledger ||
            (c->ledger == winner->ledger && citizen_key_cmp(c, winner) < 0))
            winner = c;
    }
    return winner;
}

static double advice(const Citizen *winner, uint32_t cand, unsigned *book_row) {
    if (!winner) { *book_row = 0; return 1.0; }
    uint8_t first = exp_pool[exp_off[cand]];
    if (winner->target_d != first) { *book_row = 0; return 1.0; }
    *book_row = winner->book_row;
    return 1.0 + winner->L;
}

/* ── speech: lived continuations only, quad -> tri -> bi -> lived uni ── */
static int starts_upper(uint32_t id) {
    uint8_t c = exp_pool[exp_off[id]];
    return c >= 'A' && c <= 'Z';
}
static int ends_sentence(uint32_t id) {
    uint8_t c = exp_pool[exp_off[id] + exp_lenv[id] - 1];
    return c == '.' || c == '!' || c == '?' || c == '\n';
}

static char outdir[1024];
static FILE *open_out(const char *name) {
    char path[1200];
    int n = snprintf(path, sizeof(path), "%s/%s", outdir, name);
    if (n < 0 || (size_t)n >= sizeof path) die("output path too long");
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot open artifact for writing");
    return f;
}

static void write_bytes(FILE *f, const void *p, size_t n) {
    if (n && fwrite(p, 1, n, f) != n) die("cannot write artifact");
}

static void close_out(FILE *f) {
    if (fclose(f)) die("cannot close artifact");
}

/* lived support at one explicit level; fills ccbuf, returns type count */
static size_t support_collect(const uint32_t *em, size_t nem, int level) {
    size_t lo, hi;
    if (level == 4) {
        if (ORDER < 4 || !n_quad || nem < 3) return 0;
        uint64_t ctx = ((uint64_t)em[nem - 3] << (2 * PACK)) |
                       ((uint64_t)em[nem - 2] << PACK) | em[nem - 1];
        key_range(tbl_quad, n_quad, ctx, PACK, &lo, &hi);
        return hi > lo ? collect(tbl_quad, lo, hi) : 0;
    }
    if (level == 3) {
        if (nem < 2) return 0;
        uint64_t ctx = ((uint64_t)em[nem - 2] << PACK) | em[nem - 1];
        key_range(tbl_tri, n_tri, ctx, PACK, &lo, &hi);
        return hi > lo ? collect(tbl_tri, lo, hi) : 0;
    }
    if (level == 2) {
        key_range(tbl_bi, n_bi, em[nem - 1], PACK, &lo, &hi);
        return hi > lo ? collect(tbl_bi, lo, hi) : 0;
    }
    size_t k;
    for (k = 0; k < nalive && k < MAX_CAND; k++) {
        ccbuf[k].tok = alive[k];
        ccbuf[k].cnt = n1[alive[k]];
    }
    return k;
}

/* Amendment 3: close the one token that would keep walking the higher
   singleton corridor.  Candidate types are unique inside ccbuf. */
static size_t support_close_token(size_t n, uint32_t token, int *closed) {
    size_t w = 0;
    *closed = 0;
    for (size_t i = 0; i < n; i++) {
        if (ccbuf[i].tok == token) {
            *closed = 1;
            continue;
        }
        ccbuf[w++] = ccbuf[i];
    }
    return w;
}

static void speak(const uint32_t *t, size_t n, uint64_t seed) {
    char fname[128], tname[128];
    int fn = snprintf(fname, sizeof(fname), "speech_%llu.bin", (unsigned long long)seed);
    int tn = snprintf(tname, sizeof(tname), "trace_%llu.tsv", (unsigned long long)seed);
    if (fn < 0 || (size_t)fn >= sizeof fname || tn < 0 || (size_t)tn >= sizeof tname)
        die("seed filename too long");
    FILE *f = open_out(fname), *tf = open_out(tname);
    fprintf(tf, "index\ttoken_id\tstart_position\tbackoff\tsupport_types\t"
                "support_occurrences\tchosen_occurrences\texpansion_bytes\t"
                "advice_book_row\tadvice_factor\tcorridor\tcorridor_veto\n");
    rng_state = seed ^ 0x9E3779B97F4A7C15ull;
    if (!rng_state) rng_state = 1;

    size_t nstarts = 0;
    size_t *starts = malloc(n * sizeof(size_t));
    if (!starts) die("oom");
    for (size_t i = 0; i + 3 < n; i++)
        if (starts_upper(t[i]) && (i == 0 || ends_sentence(t[i - 1])))
            starts[nstarts++] = i;
    if (nstarts == 0) die("no sentence starts");
    size_t sp = starts[rng_below(nstarts)];
    free(starts);

    static uint32_t em[MAX_EM];
    size_t nem = 0, ebytes = 0;
    for (int k = 0; k < 3; k++) {
        em[nem++] = t[sp + (size_t)k];
        write_bytes(f, exp_pool + exp_off[em[nem - 1]], exp_lenv[em[nem - 1]]);
        ebytes += exp_lenv[em[nem - 1]];
        fprintf(tf, "%zu\t%u\t%zu\t0\t0\t0\t0\t%u\t0\t1\t0\t-\n",
                nem - 1, em[nem - 1], sp + (size_t)k, exp_lenv[em[nem - 1]]);
    }

    uint32_t corr = 0; /* Amendment 2: consecutive single-type steps */
    size_t want = SPEAK_BYTES, hard = SPEAK_BYTES + SPEAK_HARD;
    while (ebytes < want && nem + 1 < MAX_EM) {
        size_t nc = 0;
        int backoff = 0;
        for (int lvl = 4; lvl >= 1 && nc == 0; lvl--) {
            nc = support_collect(em, nem, lvl);
            if (nc) backoff = lvl;
        }
        if (nc == 0) die("empty lived support");

        /* the corridor law and its real-exit invariant (A2 + A3) */
        uint32_t corridor_before = corr;
        uint32_t corridor_veto = UINT32_MAX;
        if (nc >= 2) {
            corr = 0;
        } else if (CORRIDOR != 0) {
            if (corr >= CORRIDOR) {
                int highest_level = backoff;
                size_t highest_types = nc;
                uint32_t corridor_token = ccbuf[0].tok;
                int found = 0;
                for (int lvl = backoff - 1; lvl >= 1; lvl--) {
                    size_t alt = support_collect(em, nem, lvl);
                    if (alt >= 2) {
                        int closed = 0;
                        size_t admitted = support_close_token(alt, corridor_token, &closed);
                        if (!closed) die("lower support lost the corridor continuation");
                        if (admitted) {
                            nc = admitted;
                            backoff = lvl;
                            corridor_veto = corridor_token;
                            found = 1;
                            break;
                        }
                    }
                }
                if (found) {
                    corr = 0;
                } else {
                    nc = support_collect(em, nem, highest_level);
                    if (nc != highest_types || nc != 1 || ccbuf[0].tok != corridor_token)
                        die("highest corridor support changed while descending");
                    backoff = highest_level;
                    corr++;
                }
            } else {
                corr++;
            }
        }

        uint32_t last = em[nem - 1];
        uint8_t prev_byte = exp_pool[exp_off[last] + exp_lenv[last] - 1];
        const Citizen *winner = citizen_winner(prev_byte);

        typedef struct { uint32_t tok, cnt; unsigned advice_row; double factor, s; } SC;
        static SC sc[MAX_CAND];
        for (size_t k = 0; k < nc; k++) {
            unsigned advice_row = 0;
            double factor = advice(winner, ccbuf[k].tok, &advice_row);
            double w = (double)ccbuf[k].cnt * factor;
            int freq = 0;
            size_t rst = nem > REP_WINDOW ? nem - REP_WINDOW : 0;
            for (size_t j = rst; j < nem; j++) if (em[j] == ccbuf[k].tok) freq++;
            sc[k].tok = ccbuf[k].tok;
            sc[k].cnt = ccbuf[k].cnt;
            sc[k].advice_row = advice_row;
            sc[k].factor = factor;
            sc[k].s = log(w + 1e-300) - log(1.0 + REP_PENALTY * (double)freq);
        }
        size_t limit = nc < TOP_K ? nc : TOP_K;
        for (size_t i = 0; i < limit; i++) {
            size_t best = i;
            for (size_t j = i + 1; j < nc; j++)
                if (sc[j].s > sc[best].s ||
                    (sc[j].s == sc[best].s && sc[j].tok < sc[best].tok)) best = j;
            if (best != i) { SC tmp = sc[i]; sc[i] = sc[best]; sc[best] = tmp; }
        }
        double ls[256], mx = -1e300, tot = 0;
        if (limit > 256) limit = 256;
        for (size_t i = 0; i < limit; i++) { ls[i] = sc[i].s / TEMP; if (ls[i] > mx) mx = ls[i]; }
        for (size_t i = 0; i < limit; i++) { ls[i] = exp(ls[i] - mx); tot += ls[i]; }
        double r = rng_double() * tot, cum = 0;
        size_t chosen_at = 0;
        for (size_t i = 0; i < limit; i++) { cum += ls[i]; if (cum > r) { chosen_at = i; break; } }
        uint32_t chosen = sc[chosen_at].tok;

        uint64_t support_occ = 0;
        for (size_t i = 0; i < nc; i++) support_occ += ccbuf[i].cnt;

        em[nem++] = chosen;
        write_bytes(f, exp_pool + exp_off[chosen], exp_lenv[chosen]);
        ebytes += exp_lenv[chosen];
        fprintf(tf, "%zu\t%u\t-\t%d\t%zu\t%llu\t%u\t%u\t%u\t%.17g\t%u\t",
                nem - 1, chosen, backoff, nc, (unsigned long long)support_occ,
                sc[chosen_at].cnt, exp_lenv[chosen], sc[chosen_at].advice_row,
                sc[chosen_at].factor, corridor_before);
        if (corridor_veto == UINT32_MAX) fputs("-\n", tf);
        else fprintf(tf, "%u\n", corridor_veto);
        if (ebytes >= want && !ends_sentence(chosen) && ebytes < hard) want = ebytes + 1;
    }
    close_out(f);
    close_out(tf);
}

/* ── main ── */
int main(int argc, char **argv) {
    const char *path = NULL, *cit_path = NULL;
    uint64_t seeds[MAX_SEEDS] = {7, 19, 42, 101, 271};
    size_t nseeds = 5;
    outdir[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) {
            int n = snprintf(outdir, sizeof(outdir), "%s", argv[++i]);
            if (n < 0 || (size_t)n >= sizeof outdir) die("output directory path too long");
        }
        else if (!strcmp(argv[i], "--merges") && i + 1 < argc) MERGES = parse_u32(argv[++i], "bad --merges");
        else if (!strcmp(argv[i], "--min-pair") && i + 1 < argc) MIN_PAIR = parse_u32(argv[++i], "bad --min-pair");
        else if (!strcmp(argv[i], "--order") && i + 1 < argc) {
            uint32_t v = parse_u32(argv[++i], "bad --order");
            if (v > INT_MAX) die("bad --order");
            ORDER = (int)v;
        }
        else if (!strcmp(argv[i], "--bytes") && i + 1 < argc) SPEAK_BYTES = parse_size_arg(argv[++i], "bad --bytes");
        else if (!strcmp(argv[i], "--temp") && i + 1 < argc) TEMP = parse_real(argv[++i], "bad --temp");
        else if (!strcmp(argv[i], "--topk") && i + 1 < argc) TOP_K = parse_size_arg(argv[++i], "bad --topk");
        else if (!strcmp(argv[i], "--corridor") && i + 1 < argc) CORRIDOR = parse_u32(argv[++i], "bad --corridor");
        else if (!strcmp(argv[i], "--citizens") && i + 1 < argc) cit_path = argv[++i];
        else if (!strcmp(argv[i], "--citizens-mode") && i + 1 < argc) {
            const char *m = argv[++i];
            if (!strcmp(m, "none")) cit_mode = 0;
            else if (!strcmp(m, "live")) cit_mode = 1;
            else if (!strcmp(m, "shuffled")) cit_mode = 2;
            else die("unknown citizens mode");
        }
        else if (!strcmp(argv[i], "--seeds") && i + 1 < argc) {
            nseeds = parse_seed_list(argv[++i], seeds);
        }
        else if (argv[i][0] == '-') die("unknown or incomplete option");
        else if (path) die("more than one world path");
        else path = argv[i];
    }
    if (!path || !outdir[0]) die("usage: netta_mouth <world> --out <dir> [dials]");
    if (ORDER != 3 && ORDER != 4) die("--order must be 3 or 4");
    if ((uint64_t)BASE_UNITS + MERGES > (1ull << PACK)) die("merge budget exceeds packed id space");
    if (MIN_PAIR < 2) die("--min-pair must be at least 2");
    if (SPEAK_BYTES == 0 || SPEAK_BYTES > MAX_SPEAK_BYTES) die("--bytes outside 1..65000");
    if (!(TEMP > 0.0) || !isfinite(TEMP)) die("--temp must be finite and positive");
    if (TOP_K == 0 || TOP_K > 256) die("--topk outside 1..256");
    if (CORRIDOR > 4096) die("--corridor outside 0..4096");
    if (cit_mode != 0 && !cit_path) die("citizens mode without a book");
    if (cit_path && cit_mode != 0) load_citizens(cit_path);

    read_world(path);

    merge_left = malloc((MERGES ? MERGES : 1u) * sizeof(uint32_t));
    merge_right = malloc((MERGES ? MERGES : 1u) * sizeof(uint32_t));
    exp_off = malloc((BASE_UNITS + (size_t)MERGES) * sizeof(size_t));
    exp_lenv = malloc((BASE_UNITS + (size_t)MERGES) * sizeof(uint32_t));
    if (!merge_left || !merge_right || !exp_off || !exp_lenv) die("oom");
    max_units = BASE_UNITS + MERGES;

    uint32_t *stream = malloc(world_n * sizeof(uint32_t));
    if (!stream) die("oom");
    size_t sn = world_n;
    for (size_t i = 0; i < world_n; i++) stream[i] = world[i];
    nunits = BASE_UNITS;
    for (uint32_t i = 0; i < BASE_UNITS; i++) { uint8_t b = (uint8_t)i; exp_append(i, &b, 1); }
    grow_units(stream, &sn);
    if (nunits > max_units) die("unit budget overflow");

    build_tables(stream, sn, nunits);

    fprintf(stderr, "netta_mouth: world %zu B | lived stream %zu units | merges %u | "
                    "V %u (avg %.2f B/unit) | order %d | citizens %zu (mode %d)\n",
            world_n, sn, nmerges, nunits, (double)world_n / (double)sn, ORDER, ncit, cit_mode);

    for (size_t s = 0; s < nseeds; s++) speak(stream, sn, seeds[s]);
    fprintf(stderr, "netta_mouth: %zu speech streams and token traces written to %s\n", nseeds, outdir);
    return 0;
}
