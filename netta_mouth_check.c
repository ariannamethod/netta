/* netta_mouth_check.c -- independent reader for NETTA Body 1.

   This file shares no implementation with netta_mouth.c.  It rebuilds the
   BPE inventory with a separately written pair counter, reconstructs every
   spoken byte from the mouth's token trace, scans the lived stream afresh at
   every choice to enforce highest-support quad/tri/bi/uni backoff, prices the
   observed tokens, and repeats Body 0's published >=32-byte anti-copy census.

   usage: netta_mouth_check <world> --dir <run-dir> --report <file>
            [--merges N=4096] [--min-pair N=4] [--order 4|3=4]
            [--seeds a,b,c,...=7,19,42,101,271]
            [--citizens-mode none|live|shuffled=none]

   C11, stdlib only. */

#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE 256u
#define MAX_SEEDS 32u
#define PAIR_BITS 20u
#define PAIR_SIZE (1u << PAIR_BITS)
#define PAIR_MASK (PAIR_SIZE - 1u)
#define COPY_BITS 20u
#define COPY_SIZE (1u << COPY_BITS)
#define COPY_MASK (COPY_SIZE - 1u)

static void die(const char *m) { fprintf(stderr, "netta_mouth_check: %s\n", m); exit(1); }

static void *xmalloc(size_t n) {
    void *p = malloc(n ? n : 1u);
    if (!p) die("out of memory");
    return p;
}

static void *xcalloc(size_t n, size_t z) {
    void *p = calloc(n ? n : 1u, z);
    if (!p) die("out of memory");
    return p;
}

static void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n ? n : 1u);
    if (!q) die("out of memory");
    return q;
}

static uint8_t *read_file(const char *path, size_t *n_out) {
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open input file");
    if (fseek(f, 0, SEEK_END)) die("cannot seek input file");
    long z = ftell(f);
    if (z < 0 || fseek(f, 0, SEEK_SET)) die("cannot size input file");
    size_t n = (size_t)z;
    uint8_t *p = xmalloc(n);
    if (n && fread(p, 1, n, f) != n) die("cannot read input file");
    if (fclose(f)) die("cannot close input file");
    *n_out = n;
    return p;
}

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

static size_t parse_size_arg(const char *s, const char *what) {
    uint64_t v = parse_u64(s, what);
    if (v > SIZE_MAX) die(what);
    return (size_t)v;
}

static double parse_real(const char *s, const char *what) {
    char *end = NULL;
    if (!s || !*s) die(what);
    errno = 0;
    double v = strtod(s, &end);
    if (errno || !end || *end || !isfinite(v)) die(what);
    return v;
}

static size_t parse_seeds(const char *s, uint64_t out[MAX_SEEDS]) {
    size_t n = 0;
    const char *p = s;
    if (!p || !*p) die("empty seed list");
    while (*p) {
        const char *a = p;
        while (*p && *p != ',') p++;
        size_t z = (size_t)(p - a);
        if (!z || z >= 64u) die("bad seed list");
        char one[64]; memcpy(one, a, z); one[z] = 0;
        if (n == MAX_SEEDS) die("too many seeds");
        uint64_t v = parse_u64(one, "bad seed");
        for (size_t i = 0; i < n; i++) if (out[i] == v) die("duplicate seed");
        out[n++] = v;
        if (*p == ',') { p++; if (!*p) die("bad seed list"); }
    }
    return n;
}

/* Independent BPE reconstruction.  Unlike the mouth's packed n-gram arrays,
   this reader counts pairs in its own open-addressed table and later scans
   the reconstructed stream directly for every support query. */
typedef struct {
    size_t off;
    uint32_t len;
} Unit;

typedef struct {
    uint32_t *stream;
    size_t nstream;
    Unit *unit;
    uint32_t nunits;
    uint8_t *pool;
    size_t pool_n, pool_cap;
    uint32_t *unigram;
    size_t nalive;
} WorldModel;

static uint64_t pair_key[PAIR_SIZE];
static uint32_t pair_count[PAIR_SIZE];
static uint32_t pair_used[PAIR_SIZE];
static uint32_t pair_used_n;

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30; x *= 0xbf58476d1ce4e5b9ull;
    x ^= x >> 27; x *= 0x94d049bb133111ebull;
    return x ^ (x >> 31);
}

static void append_expansion(WorldModel *m, uint32_t id, const uint8_t *p, uint32_t n) {
    if ((size_t)n > SIZE_MAX - m->pool_n) die("expansion pool overflow");
    if (m->pool_n + n > m->pool_cap) {
        if (!m->pool_cap) m->pool_cap = 1u << 20;
        while (m->pool_n + n > m->pool_cap) {
            if (m->pool_cap > SIZE_MAX / 2u) die("expansion pool overflow");
            m->pool_cap *= 2u;
        }
        m->pool = xrealloc(m->pool, m->pool_cap);
    }
    m->unit[id].off = m->pool_n;
    m->unit[id].len = n;
    memcpy(m->pool + m->pool_n, p, n);
    m->pool_n += n;
}

static int independent_merge_round(WorldModel *m, uint32_t min_pair) {
    pair_used_n = 0;
    if (m->nstream >= PAIR_SIZE) die("world exceeds independent pair-table boundary");
    for (size_t i = 0; i + 1 < m->nstream; i++) {
        uint64_t key = ((uint64_t)m->stream[i] << 32) | m->stream[i + 1];
        uint32_t h = (uint32_t)mix64(key) & PAIR_MASK;
        for (;;) {
            if (!pair_count[h]) {
                pair_key[h] = key; pair_count[h] = 1;
                pair_used[pair_used_n++] = h;
                break;
            }
            if (pair_key[h] == key) { pair_count[h]++; break; }
            h = (h + 1u) & PAIR_MASK;
        }
    }
    uint64_t best_key = UINT64_MAX;
    uint32_t best_count = 0;
    for (uint32_t i = 0; i < pair_used_n; i++) {
        uint32_t h = pair_used[i];
        if (pair_count[h] > best_count ||
            (pair_count[h] == best_count && pair_key[h] < best_key)) {
            best_count = pair_count[h]; best_key = pair_key[h];
        }
    }
    for (uint32_t i = 0; i < pair_used_n; i++) pair_count[pair_used[i]] = 0;
    if (best_count < min_pair) return 0;

    uint32_t left = (uint32_t)(best_key >> 32), right = (uint32_t)best_key;
    uint32_t id = m->nunits++;
    uint32_t ln = m->unit[left].len, rn = m->unit[right].len;
    if ((uint64_t)ln + rn > UINT32_MAX) die("unit expansion too large");
    uint8_t *joined = xmalloc((size_t)ln + rn);
    memcpy(joined, m->pool + m->unit[left].off, ln);
    memcpy(joined + ln, m->pool + m->unit[right].off, rn);
    append_expansion(m, id, joined, ln + rn);
    free(joined);

    size_t w = 0;
    for (size_t i = 0; i < m->nstream;) {
        if (i + 1 < m->nstream && m->stream[i] == left && m->stream[i + 1] == right) {
            m->stream[w++] = id; i += 2;
        } else {
            m->stream[w++] = m->stream[i++];
        }
    }
    m->nstream = w;
    return 1;
}

static WorldModel build_world(const uint8_t *world, size_t n, uint32_t merges, uint32_t min_pair) {
    WorldModel m; memset(&m, 0, sizeof m);
    if (n < 100u) die("world too small");
    if ((uint64_t)BASE + merges > 65536u) die("merge budget exceeds unit id space");
    m.unit = xcalloc((size_t)BASE + merges, sizeof *m.unit);
    m.stream = xmalloc(n * sizeof *m.stream);
    m.nstream = n; m.nunits = BASE;
    for (uint32_t i = 0; i < BASE; i++) {
        uint8_t b = (uint8_t)i;
        append_expansion(&m, i, &b, 1);
    }
    for (size_t i = 0; i < n; i++) m.stream[i] = world[i];
    uint32_t made = 0;
    while (made < merges && independent_merge_round(&m, min_pair)) made++;
    if (m.nunits != BASE + made) die("internal merge count drift");
    m.unigram = xcalloc(m.nunits, sizeof *m.unigram);
    for (size_t i = 0; i < m.nstream; i++) m.unigram[m.stream[i]]++;
    for (uint32_t i = 0; i < m.nunits; i++) if (m.unigram[i]) m.nalive++;
    return m;
}

typedef struct {
    int level;
    size_t types;
    uint64_t occurrences;
} Support;

static Support support_for(const WorldModel *m, const uint32_t *past, size_t npast,
                           int order, uint32_t *counts) {
    Support s; memset(&s, 0, sizeof s);
    int highest = order >= 4 && npast >= 3 ? 4 : npast >= 2 ? 3 : npast >= 1 ? 2 : 1;
    for (int level = highest; level >= 1; level--) {
        memset(counts, 0, (size_t)m->nunits * sizeof *counts);
        uint64_t occ = 0;
        if (level == 1) {
            for (uint32_t u = 0; u < m->nunits; u++) {
                counts[u] = m->unigram[u]; occ += counts[u];
            }
        } else {
            size_t need = (size_t)(level - 1);
            for (size_t i = need; i < m->nstream; i++) {
                int match = 1;
                for (size_t j = 0; j < need; j++)
                    if (m->stream[i - need + j] != past[npast - need + j]) { match = 0; break; }
                if (match) { counts[m->stream[i]]++; occ++; }
            }
        }
        if (occ) {
            size_t types = 0;
            for (uint32_t u = 0; u < m->nunits; u++) if (counts[u]) types++;
            s.level = level; s.types = types; s.occurrences = occ;
            return s;
        }
    }
    die("no lived support");
    return s;
}

typedef struct {
    uint32_t token;
    int has_start;
    size_t start;
    int backoff;
    size_t support_types;
    uint64_t support_occurrences;
    uint32_t chosen_occurrences;
    uint32_t expansion_bytes;
    unsigned advice_row;
    double advice_factor;
} TraceRow;

typedef struct {
    TraceRow *row;
    size_t n, cap;
} Trace;

static int split_tabs(char *line, char **f, int cap) {
    int n = 0;
    if (!line || !*line) return 0;
    f[n++] = line;
    for (char *p = line; *p; p++) if (*p == '\t') {
        *p = 0;
        if (n == cap) die("trace row has too many fields");
        f[n++] = p + 1;
    }
    return n;
}

static Trace read_trace(const char *path) {
    static const char header[] =
        "index\ttoken_id\tstart_position\tbackoff\tsupport_types\t"
        "support_occurrences\tchosen_occurrences\texpansion_bytes\t"
        "advice_book_row\tadvice_factor";
    FILE *f = fopen(path, "rb");
    if (!f) die("cannot open token trace");
    Trace t; memset(&t, 0, sizeof t);
    char line[1024]; size_t line_no = 0;
    while (fgets(line, sizeof line, f)) {
        line_no++;
        size_t z = strlen(line);
        if (!z || line[z - 1] != '\n') die("unterminated or oversized trace row");
        line[--z] = 0;
        if (z && line[z - 1] == '\r') die("trace is not canonical LF text");
        if (line_no == 1) { if (strcmp(line, header)) die("trace header drifted"); continue; }
        char *v[10];
        if (split_tabs(line, v, 10) != 10) die("trace row width drifted");
        if (parse_size_arg(v[0], "bad trace index") != t.n) die("trace index drifted");
        if (t.n == t.cap) { t.cap = t.cap ? t.cap * 2u : 256u; t.row = xrealloc(t.row, t.cap * sizeof *t.row); }
        TraceRow *r = &t.row[t.n++]; memset(r, 0, sizeof *r);
        r->token = parse_u32(v[1], "bad trace token");
        if (strcmp(v[2], "-")) { r->has_start = 1; r->start = parse_size_arg(v[2], "bad trace start"); }
        r->backoff = (int)parse_u32(v[3], "bad trace backoff");
        r->support_types = parse_size_arg(v[4], "bad trace support types");
        r->support_occurrences = parse_u64(v[5], "bad trace support occurrences");
        r->chosen_occurrences = parse_u32(v[6], "bad trace chosen occurrences");
        r->expansion_bytes = parse_u32(v[7], "bad trace expansion bytes");
        r->advice_row = parse_u32(v[8], "bad trace advice row");
        r->advice_factor = parse_real(v[9], "bad trace advice factor");
        if (r->advice_factor < 1.0 || r->advice_factor > 1.5) die("trace advice factor outside law");
        if ((!r->advice_row && r->advice_factor != 1.0) || (r->advice_row && r->advice_factor == 1.0))
            die("trace advice row/factor disagree");
    }
    if (ferror(f) || fclose(f)) die("cannot finish token trace");
    if (t.n < 3u) die("token trace too short");
    return t;
}

typedef struct { uint32_t *pos; uint32_t n, cap; } PosList;

/* Independently transcribed from the five state=1 rows of the authenticated
   Court-4 capsule.  Distinct ledgers make row 7 the only eligible winner in
   the shared destination-space context.  Shuffling rotates target bundles,
   so row 7 carries row 18's target byte ('o') in the null. */
static void expected_advice(int mode, uint8_t prev, uint8_t next,
                            unsigned *row, double *factor) {
    *row = 0; *factor = 1.0;
    if (mode == 0 || prev != 32u) return;
    uint8_t target = mode == 1 ? 97u : 111u;
    if (next == target) { *row = 7u; *factor = 1.5; }
}

static uint32_t pack4(const uint8_t *p) {
    return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) |
           ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t hash32(uint32_t x) {
    x ^= x >> 16; x *= 0x7feb352du; x ^= x >> 15;
    x *= 0x846ca68bu; return x ^ (x >> 16);
}

static PosList *build_copy_index(const uint8_t *world, size_t n) {
    if (n > UINT32_MAX) die("world too large for copy index");
    PosList *idx = xcalloc(COPY_SIZE, sizeof *idx);
    for (size_t i = 0; i + 4u <= n; i++) {
        PosList *b = &idx[hash32(pack4(world + i)) & COPY_MASK];
        if (b->n == b->cap) {
            b->cap = b->cap ? b->cap * 2u : 4u;
            b->pos = xrealloc(b->pos, (size_t)b->cap * sizeof *b->pos);
        }
        b->pos[b->n++] = (uint32_t)i;
    }
    return idx;
}

static uint32_t longest_at(const uint8_t *speech, size_t sn, size_t at,
                           const uint8_t *world, size_t wn, PosList *idx) {
    size_t remain = sn - at;
    uint32_t best = 0;
    if (remain < 4u || wn < 4u) {
        for (size_t p = 0; p < wn; p++) {
            size_t z = 0, lim = remain < wn - p ? remain : wn - p;
            while (z < lim && speech[at + z] == world[p + z]) z++;
            if (z > best) best = (uint32_t)z;
        }
        return best;
    }
    PosList *b = &idx[hash32(pack4(speech + at)) & COPY_MASK];
    for (uint32_t i = 0; i < b->n; i++) {
        size_t p = b->pos[i];
        if (pack4(speech + at) != pack4(world + p)) continue;
        size_t z = 0, lim = remain < wn - p ? remain : wn - p;
        while (z < lim && speech[at + z] == world[p + z]) z++;
        if (z > best) best = (uint32_t)z;
    }
    return best;
}

typedef struct { uint32_t longest; size_t covered; double fraction; } CopyResult;

static CopyResult copy_census(const uint8_t *speech, size_t sn,
                              const uint8_t *world, size_t wn, PosList *idx) {
    CopyResult r; memset(&r, 0, sizeof r);
    if (!sn) return r;
    uint8_t *covered = xcalloc(sn, 1);
    for (size_t i = 0; i < sn; i++) {
        uint32_t z = longest_at(speech, sn, i, world, wn, idx);
        if (z > r.longest) r.longest = z;
        if (z >= 32u) for (size_t j = i; j < i + z && j < sn; j++) covered[j] = 1;
    }
    for (size_t i = 0; i < sn; i++) r.covered += covered[i] != 0;
    r.fraction = (double)r.covered / (double)sn;
    free(covered);
    return r;
}

static void make_path(char out[1200], const char *dir, const char *kind, uint64_t seed, const char *ext) {
    int n = snprintf(out, 1200, "%s/%s_%llu.%s", dir, kind, (unsigned long long)seed, ext);
    if (n < 0 || n >= 1200) die("artifact path too long");
}

typedef struct { int ear_ok, copy_ok; } StreamVerdict;

static StreamVerdict read_one(FILE *report, const WorldModel *m, const uint8_t *world,
                              size_t world_n, PosList *copy_index, const char *dir,
                              uint64_t seed, int order, int citizen_mode) {
    char path[1200];
    make_path(path, dir, "trace", seed, "tsv");
    Trace t = read_trace(path);
    make_path(path, dir, "speech", seed, "bin");
    size_t speech_n = 0; uint8_t *speech = read_file(path, &speech_n);
    if (!speech_n) die("empty speech stream");

    size_t rebuilt_n = 0;
    for (size_t i = 0; i < t.n; i++) {
        if (t.row[i].token >= m->nunits || !m->unigram[t.row[i].token]) die("trace names non-lived token");
        if (t.row[i].expansion_bytes != m->unit[t.row[i].token].len) die("trace expansion length mismatch");
        if (rebuilt_n > SIZE_MAX - m->unit[t.row[i].token].len) die("rebuilt speech overflow");
        rebuilt_n += m->unit[t.row[i].token].len;
    }
    if (rebuilt_n != speech_n) die("trace/speech byte length mismatch");
    uint8_t *rebuilt = xmalloc(rebuilt_n); size_t w = 0;
    for (size_t i = 0; i < t.n; i++) {
        Unit u = m->unit[t.row[i].token];
        memcpy(rebuilt + w, m->pool + u.off, u.len); w += u.len;
    }
    if (memcmp(rebuilt, speech, speech_n)) die("trace does not reconstruct speech bytes");
    free(rebuilt);

    if (!t.row[0].has_start || !t.row[1].has_start || !t.row[2].has_start ||
        t.row[1].start != t.row[0].start + 1u || t.row[2].start != t.row[0].start + 2u ||
        t.row[2].start >= m->nstream) die("initial lived run trace mismatch");
    for (size_t i = 0; i < 3u; i++) {
        if (m->stream[t.row[i].start] != t.row[i].token || t.row[i].backoff != 0 ||
            t.row[i].support_types || t.row[i].support_occurrences || t.row[i].chosen_occurrences)
            die("initial lived run is not exact");
    }

    uint32_t *tokens = xmalloc(t.n * sizeof *tokens);
    uint32_t *counts = xcalloc(m->nunits, sizeof *counts);
    double model_bits = 0.0, ignorance_bits = 0.0;
    for (size_t i = 0; i < t.n; i++) {
        tokens[i] = t.row[i].token;
        Support s = support_for(m, tokens, i, order, counts);
        uint32_t chosen = counts[tokens[i]];
        if (!chosen) die("emitted token is outside lived support");
        if (i >= 3u && (t.row[i].has_start || t.row[i].backoff != s.level ||
            t.row[i].support_types != s.types ||
            t.row[i].support_occurrences != s.occurrences ||
            t.row[i].chosen_occurrences != chosen))
            die("mouth trace disagrees with independent support scan");
        if (i < 3u) {
            if (t.row[i].advice_row || t.row[i].advice_factor != 1.0)
                die("initial run carries unlawful advice");
        } else {
            Unit before = m->unit[tokens[i - 1u]], current = m->unit[tokens[i]];
            uint8_t prev = m->pool[before.off + before.len - 1u];
            uint8_t next = m->pool[current.off];
            unsigned expected_row = 0; double expected_factor = 1.0;
            expected_advice(citizen_mode, prev, next, &expected_row, &expected_factor);
            if (t.row[i].advice_row != expected_row || t.row[i].advice_factor != expected_factor)
                die("mouth trace disagrees with independent single-winner replay");
        }
        model_bits += -log2((double)chosen / (double)s.occurrences);
        ignorance_bits += log2((double)m->nalive);
    }
    free(counts); free(tokens);

    CopyResult c = copy_census(speech, speech_n, world, world_n, copy_index);
    double model_bpb = model_bits / (double)speech_n;
    double ignorance_bpb = ignorance_bits / (double)speech_n;
    StreamVerdict verdict = {model_bpb < ignorance_bpb, c.fraction < 0.50};
    int pass = verdict.ear_ok && verdict.copy_ok;
    fprintf(report, "\nBEGIN RAW SPEECH seed=%llu bytes=%zu\n",
            (unsigned long long)seed, speech_n);
    if (fwrite(speech, 1, speech_n, report) != speech_n) die("cannot write report");
    if (!speech_n || speech[speech_n - 1] != '\n') fputc('\n', report);
    fprintf(report, "END RAW SPEECH seed=%llu\n", (unsigned long long)seed);
    fprintf(report, "seed=%llu\ttokens=%zu\tmodel_bits_per_byte=%.9f\t"
                    "ignorance_bits_per_byte=%.9f\tlongest_match_bytes=%u\t"
                    "coverage_ge32=%.9f\tear=%s\tanti_copy=%s\t"
                    "support=PASS\tstream=%s\n",
            (unsigned long long)seed, t.n, model_bpb, ignorance_bpb,
            c.longest, c.fraction, verdict.ear_ok ? "PASS" : "FAIL",
            verdict.copy_ok ? "PASS" : "FAIL", pass ? "PASS" : "FAIL");
    fprintf(stderr, "seed %llu: tokens=%zu model=%.6f ignorance=%.6f longest=%u coverage>=32=%.6f %s\n",
            (unsigned long long)seed, t.n, model_bpb, ignorance_bpb,
            c.longest, c.fraction, pass ? "PASS" : "FAIL");
    free(speech); free(t.row);
    return verdict;
}

int main(int argc, char **argv) {
    const char *world_path = NULL, *dir = NULL, *report_path = NULL;
    uint32_t merges = 4096u, min_pair = 4u;
    int order = 4, citizen_mode = 0;
    uint64_t seeds[MAX_SEEDS] = {7,19,42,101,271}; size_t nseeds = 5;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--dir") && i + 1 < argc) dir = argv[++i];
        else if (!strcmp(argv[i], "--report") && i + 1 < argc) report_path = argv[++i];
        else if (!strcmp(argv[i], "--merges") && i + 1 < argc) merges = parse_u32(argv[++i], "bad --merges");
        else if (!strcmp(argv[i], "--min-pair") && i + 1 < argc) min_pair = parse_u32(argv[++i], "bad --min-pair");
        else if (!strcmp(argv[i], "--order") && i + 1 < argc) {
            uint32_t v = parse_u32(argv[++i], "bad --order");
            if (v > INT_MAX) die("bad --order"); order = (int)v;
        }
        else if (!strcmp(argv[i], "--seeds") && i + 1 < argc) nseeds = parse_seeds(argv[++i], seeds);
        else if (!strcmp(argv[i], "--citizens-mode") && i + 1 < argc) {
            const char *v = argv[++i];
            if (!strcmp(v, "none")) citizen_mode = 0;
            else if (!strcmp(v, "live")) citizen_mode = 1;
            else if (!strcmp(v, "shuffled")) citizen_mode = 2;
            else die("bad --citizens-mode");
        }
        else if (argv[i][0] == '-') die("unknown or incomplete option");
        else if (world_path) die("more than one world path");
        else world_path = argv[i];
    }
    if (!world_path || !dir || !report_path) die("usage: netta_mouth_check <world> --dir <run-dir> --report <file> [dials]");
    if (order != 3 && order != 4) die("--order must be 3 or 4");
    if (min_pair < 2u) die("--min-pair must be at least 2");

    size_t world_n = 0; uint8_t *world = read_file(world_path, &world_n);
    WorldModel m = build_world(world, world_n, merges, min_pair);
    PosList *copy_index = build_copy_index(world, world_n);
    FILE *report = fopen(report_path, "wb");
    if (!report) die("cannot open report");
    static const char *mode_name[3] = {"none", "live", "shuffled"};
    fprintf(report, "NETTA BODY 1 — INDEPENDENT EAR\n"
                    "world_bytes=%zu\tlived_units=%zu\tmerges=%u\tinventory=%u\talive=%zu\torder=%d\tcitizens_mode=%s\n",
            world_n, m.nstream, m.nunits - BASE, m.nunits, m.nalive, order,
            mode_name[citizen_mode]);
    int all_ear = 1, all_copy = 1;
    for (size_t i = 0; i < nseeds; i++) {
        StreamVerdict v = read_one(report, &m, world, world_n, copy_index, dir,
                                   seeds[i], order, citizen_mode);
        if (!v.ear_ok) all_ear = 0;
        if (!v.copy_ok) all_copy = 0;
    }
    const char *final_line = all_ear && all_copy ?
        "SPEECH PASS: the mouth speaks below ignorance and above copying" :
        !all_ear && !all_copy ?
        "SPEECH FAIL: ignorance and frozen anti-copy gates failed" :
        !all_ear ?
        "SPEECH FAIL: one or more streams did not beat honest ignorance" :
        "SPEECH FAIL: one or more streams reached frozen anti-copy coverage 0.50";
    fprintf(report, "\n%s\n", final_line);
    if (fclose(report)) die("cannot close report");
    fprintf(stderr, "%s\n", final_line);
    return all_ear && all_copy ? 0 : 2;
}
