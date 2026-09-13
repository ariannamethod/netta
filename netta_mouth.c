/* netta_mouth.c -- NETTA Body 1: the mouth, under MOUTH_PROTOCOL.md.

   The organism speaks without the mycelium.  Life on a world grows
   earned units (Body 0's frozen merge law); speech is sampled ONLY over
   lived continuations, quad -> tri -> bi -> lived unigrams.  There is
   no smoothing hand in the voice and no field: Body 0's verdict deleted
   the field organ, and the mouth does not resurrect it.

   Court-4 citizens (live earned relations from the sealed book) advise
   sampling inside their exact context and nowhere else, with the L they
   earned.  Controls: --citizens-mode none (plain mouth) and shuffled
   (deterministic rotation of targets) run beside the advised mouth.

   Iteration is the law: --merges, --order, --temp, --topk, --bytes are
   open dials; the same flags and seed speak the same bytes.

   usage: netta_mouth <world> --out <dir>
            [--merges N=4096] [--min-pair N=4] [--order 4|3=4]
            [--bytes N=700] [--seeds a,b,c,...=7,19,42,101,271]
            [--temp X=0.8] [--topk N=15]
            [--citizens FILE --citizens-mode live|none|shuffled=none]

   C11, stdlib only.  Deterministic.                                   */

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

static void die(const char *m) { fprintf(stderr, "netta_mouth: %s\n", m); exit(1); }

/* ── dials ── */
static uint32_t MERGES = 4096;
static uint32_t MIN_PAIR = 4;
static int ORDER = 4;
static size_t SPEAK_BYTES = 700;
static double TEMP = 0.8;
static size_t TOP_K = 15;

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
    if (exp_pool_len + len > exp_pool_cap) {
        exp_pool_cap = exp_pool_cap ? exp_pool_cap * 2 : (1u << 20);
        exp_pool = realloc(exp_pool, exp_pool_cap);
        if (!exp_pool) die("oom pool");
    }
    exp_off[id] = exp_pool_len;
    exp_lenv[id] = len;
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
        uint32_t la = exp_lenv[a], lb = exp_lenv[b];
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

/* ── Court-4 citizens: live earned relations advise in their context ── */
typedef struct { uint8_t ctx, target; double L; } Citizen;
static Citizen cit[MAX_CITIZENS];
static size_t ncit;
static int cit_mode; /* 0 none, 1 live, 2 shuffled */

static void load_citizens(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open citizens book");
    char line[4096];
    int row = 0;
    while (fgets(line, sizeof line, f)) {
        row++;
        if (row == 1) {
            if (strncmp(line, "arm\t", 4)) die("citizens book header drifted");
            continue;
        }
        char *save = line, *fld[24];
        int nf = 0;
        for (char *p = strsep(&save, "\t\n"); p && nf < 24; p = strsep(&save, "\t\n"))
            fld[nf++] = p;
        if (nf < 22) continue;
        if (strcmp(fld[0], "relation")) continue;   /* oracle and nulls are controls, never advisers */
        if (strcmp(fld[19], "1")) continue;          /* state=1 only: live earned balance */
        long clen = strtol(fld[4], NULL, 10);
        if (clen != 1) die("citizen with unexpected context length");
        long tgt = strtol(fld[2], NULL, 10);         /* target_d: destination-side byte */
        long ctx = strtol(fld[6], NULL, 10);         /* c1_d: destination-side context byte */
        double L = strtod(fld[20], NULL);
        if (tgt < 0 || tgt > 255 || ctx < 0 || ctx > 255 || L <= 0.0 || L > 0.5)
            die("citizen outside the sealed law");
        if (ncit == MAX_CITIZENS) die("too many citizens");
        cit[ncit].ctx = (uint8_t)ctx;
        cit[ncit].target = (uint8_t)tgt;
        cit[ncit].L = L;
        ncit++;
    }
    fclose(f);
    if (!ncit) die("citizens book holds no live relation");
    if (cit_mode == 2) { /* deterministic rotation of targets: the null hand */
        uint8_t t0 = cit[0].target;
        for (size_t i = 0; i + 1 < ncit; i++) cit[i].target = cit[i + 1].target;
        cit[ncit - 1].target = t0;
    }
}

/* the advice law: inside the citizen's exact context only, weight = earned L */
static double advice(uint8_t prev_byte, uint32_t cand) {
    if (cit_mode == 0 || ncit == 0) return 1.0;
    uint8_t first = exp_pool[exp_off[cand]];
    double f = 1.0;
    for (size_t i = 0; i < ncit; i++)
        if (cit[i].ctx == prev_byte && cit[i].target == first)
            f *= 1.0 + cit[i].L;
    return f;
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
    snprintf(path, sizeof(path), "%s/%s", outdir, name);
    FILE *f = fopen(path, "wb");
    if (!f) die("cannot open artifact for writing");
    return f;
}

static void speak(const uint32_t *t, size_t n, uint64_t seed) {
    char fname[128];
    snprintf(fname, sizeof(fname), "speech_%llu.bin", (unsigned long long)seed);
    FILE *f = open_out(fname);
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
        fwrite(exp_pool + exp_off[em[nem - 1]], 1, exp_lenv[em[nem - 1]], f);
        ebytes += exp_lenv[em[nem - 1]];
    }

    size_t want = SPEAK_BYTES, hard = SPEAK_BYTES + SPEAK_HARD;
    while (ebytes < want && nem + 1 < MAX_EM) {
        size_t nc = 0, lo, hi;
        if (ORDER >= 4 && n_quad && nem >= 3) {
            uint64_t ctx = ((uint64_t)em[nem - 3] << (2 * PACK)) |
                           ((uint64_t)em[nem - 2] << PACK) | em[nem - 1];
            key_range(tbl_quad, n_quad, ctx, PACK, &lo, &hi);
            if (hi > lo) nc = collect(tbl_quad, lo, hi);
        }
        if (nc == 0 && nem >= 2) {
            uint64_t ctx = ((uint64_t)em[nem - 2] << PACK) | em[nem - 1];
            key_range(tbl_tri, n_tri, ctx, PACK, &lo, &hi);
            if (hi > lo) nc = collect(tbl_tri, lo, hi);
        }
        if (nc == 0) {
            key_range(tbl_bi, n_bi, em[nem - 1], PACK, &lo, &hi);
            if (hi > lo) nc = collect(tbl_bi, lo, hi);
        }
        if (nc == 0) {
            for (size_t k = 0; k < nalive && k < MAX_CAND; k++) {
                ccbuf[k].tok = alive[k];
                ccbuf[k].cnt = n1[alive[k]];
            }
            nc = nalive < MAX_CAND ? nalive : MAX_CAND;
        }

        uint32_t last = em[nem - 1];
        uint8_t prev_byte = exp_pool[exp_off[last] + exp_lenv[last] - 1];

        typedef struct { uint32_t tok; double s; } SC;
        static SC sc[MAX_CAND];
        for (size_t k = 0; k < nc; k++) {
            double w = (double)ccbuf[k].cnt * advice(prev_byte, ccbuf[k].tok);
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
            if (best != i) { SC tmp = sc[i]; sc[i] = sc[best]; sc[best] = tmp; }
        }
        double ls[256], mx = -1e300, tot = 0;
        if (limit > 256) limit = 256;
        for (size_t i = 0; i < limit; i++) { ls[i] = sc[i].s / TEMP; if (ls[i] > mx) mx = ls[i]; }
        for (size_t i = 0; i < limit; i++) { ls[i] = exp(ls[i] - mx); tot += ls[i]; }
        double r = rng_double() * tot, cum = 0;
        uint32_t chosen = sc[0].tok;
        for (size_t i = 0; i < limit; i++) { cum += ls[i]; if (cum > r) { chosen = sc[i].tok; break; } }

        em[nem++] = chosen;
        fwrite(exp_pool + exp_off[chosen], 1, exp_lenv[chosen], f);
        ebytes += exp_lenv[chosen];
        if (ebytes >= want && !ends_sentence(chosen) && ebytes < hard) want = ebytes + 1;
    }
    fclose(f);
}

/* ── main ── */
int main(int argc, char **argv) {
    const char *path = NULL, *cit_path = NULL;
    uint64_t seeds[MAX_SEEDS] = {7, 19, 42, 101, 271};
    size_t nseeds = 5;
    outdir[0] = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--out") && i + 1 < argc) snprintf(outdir, sizeof(outdir), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--merges") && i + 1 < argc) MERGES = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--min-pair") && i + 1 < argc) MIN_PAIR = (uint32_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--order") && i + 1 < argc) ORDER = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--bytes") && i + 1 < argc) SPEAK_BYTES = (size_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--temp") && i + 1 < argc) TEMP = atof(argv[++i]);
        else if (!strcmp(argv[i], "--topk") && i + 1 < argc) TOP_K = (size_t)strtoul(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--citizens") && i + 1 < argc) cit_path = argv[++i];
        else if (!strcmp(argv[i], "--citizens-mode") && i + 1 < argc) {
            const char *m = argv[++i];
            if (!strcmp(m, "none")) cit_mode = 0;
            else if (!strcmp(m, "live")) cit_mode = 1;
            else if (!strcmp(m, "shuffled")) cit_mode = 2;
            else die("unknown citizens mode");
        }
        else if (!strcmp(argv[i], "--seeds") && i + 1 < argc) {
            nseeds = 0;
            char *dup = argv[++i];
            for (char *p = strtok(dup, ","); p && nseeds < MAX_SEEDS; p = strtok(NULL, ","))
                seeds[nseeds++] = strtoull(p, NULL, 10);
            if (!nseeds) die("empty seed list");
        }
        else path = argv[i];
    }
    if (!path || !outdir[0]) die("usage: netta_mouth <world> --out <dir> [dials]");
    if (ORDER != 3 && ORDER != 4) die("--order must be 3 or 4");
    if (BASE_UNITS + MERGES > (1u << PACK)) die("merge budget exceeds packed id space");
    if (cit_mode != 0 && !cit_path) die("citizens mode without a book");
    if (cit_path && cit_mode != 0) load_citizens(cit_path);

    read_world(path);

    merge_left = malloc(MERGES * sizeof(uint32_t));
    merge_right = malloc(MERGES * sizeof(uint32_t));
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
    while (nmerges < MERGES && merge_round(stream, &sn)) {}
    if (nunits > max_units) die("unit budget overflow");

    build_tables(stream, sn, nunits);

    fprintf(stderr, "netta_mouth: world %zu B | lived stream %zu units | merges %u | "
                    "V %u (avg %.2f B/unit) | order %d | citizens %zu (mode %d)\n",
            world_n, sn, nmerges, nunits, (double)world_n / (double)sn, ORDER, ncit, cit_mode);

    for (size_t s = 0; s < nseeds; s++) speak(stream, sn, seeds[s]);
    fprintf(stderr, "netta_mouth: %zu speech streams written to %s\n", nseeds, outdir);
    return 0;
}
