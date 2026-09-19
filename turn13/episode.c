/* JOINT-PREFIX: unchanged causal frontend/cold learner, seven source arms.
   All quote construction is independent of stdin and the current truth.
   The dictionary addresses doubt: only stored branch records vote, and only
   the single record whose prefix content is the longest matched suffix. */
#include "../byte_recurrence/frontend.h"
#include "../portable_recurrence/recurrence.h"

#include <float.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_RULES 32
#define HORIZON 32
#define MAX_ARCHIVE_BYTES 528
#define MAX_RECORDS ((MAX_ARCHIVE_BYTES - 16) / 16)
#define MAX_FLAT_RECORDS ((MAX_ARCHIVE_BYTES - 16) / 16)
#define NGRAMMARS 5
#define MATCH_ARMS 6
#define NARMS 7

typedef struct { uint16_t left, right; } Rule;
typedef struct { uint8_t length, event[HORIZON]; } Expansion;
typedef struct { uint8_t rule_id, prefix_len; uint16_t counts[7]; } Branch;
typedef struct {
    size_t count, records, bytes;
    Rule *rule;
    Expansion *forward;
    Branch *branch;
} Grammar;
typedef struct { uint8_t length, context[31]; uint16_t counts[7]; } FlatRecord;
typedef struct { size_t count, bytes; FlatRecord *record; } Flat;
typedef struct { uint8_t event[HORIZON]; size_t length; } History;
typedef struct {
    int length, nmatches, matches[1];
    uint64_t votes[7];
} Match;
typedef struct {
    double shadow, odds, gain, minimum, peak, drawdown;
    int active;
    size_t activation;
} Outer;
typedef struct {
    BFQuote bf;
    PRQuote local;
    int canonical_group[256];
    uint8_t rank[256], heads[BF_DEPTH];
    int k;
    double mass[PR_GROUPS], cold[256];
} Quote;

static const char *arm_names[NARMS] = {"episode", "isolated", "frequency", "reverse", "permuted", "flat", "row"};
static const PRArchive empty_archive = {{0}};

static void fail(const char *message) {
    fprintf(stderr, "episode: %s\n", message);
    exit(1);
}

static double la(double a, double b) {
    if (a == -INFINITY) return b;
    if (b == -INFINITY) return a;
    double high = a > b ? a : b, low = a > b ? b : a;
    return high + log1p(exp2(low - high)) / log(2.0);
}

static unsigned char *read_bytes(const char *path, size_t limit, size_t *length) {
    FILE *f = fopen(path, "rb");
    if (!f) fail("open archive/alphabet");
    unsigned char *bytes = malloc(limit + 1);
    if (!bytes) fail("allocate input bytes");
    *length = fread(bytes, 1, limit + 1, f);
    int bad = ferror(f);
    if (fclose(f)) bad = 1;
    if (bad || *length > limit) fail("invalid input size/read");
    return bytes;
}

static uint16_t le16(const unsigned char *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static uint32_t le32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t le64(const unsigned char *p) {
    uint64_t value = 0;
    for (int i = 7; i >= 0; i--) value = (value << 8) | p[i];
    return value;
}

static void append_child(Expansion *dst, unsigned child, const Expansion *past) {
    if (child < 7) {
        if (dst->length == HORIZON) fail("episode expansion over horizon");
        dst->event[dst->length++] = (uint8_t)child;
    } else {
        const Expansion *part = &past[child - 7];
        if ((unsigned)dst->length + part->length > HORIZON)
            fail("episode expansion over horizon");
        memcpy(dst->event + dst->length, part->event, part->length);
        dst->length = (uint8_t)(dst->length + part->length);
    }
}

/* NETEI001: magic 8, rule_count u32, record_count u32, then 4-byte rules
   (left u16, right u16; no unused support) and 16-byte branch records
   (rule_id u8, prefix_len u8, seven exact u16 continuation counts). The
   prefix content of a record is the first prefix_len events of its rule's
   expansion: the tree is the only addressing structure ever paid for. */
static Grammar load_grammar(const char *path) {
    Grammar g = {0};
    unsigned char *bytes = read_bytes(path, MAX_ARCHIVE_BYTES, &g.bytes);
    if (g.bytes < 16 || memcmp(bytes, "NETEI001", 8))
        fail("invalid NETEI001 header");
    g.count = le32(bytes + 8);
    g.records = le32(bytes + 12);
    if (g.count > MAX_RULES || g.records > MAX_RECORDS ||
        g.bytes != 16 + 4 * g.count + 16 * g.records)
        fail("invalid branch archive count/size");
    if (g.count) {
        g.rule = calloc(g.count, sizeof(*g.rule));
        g.forward = calloc(g.count, sizeof(*g.forward));
        if (!g.rule || !g.forward) fail("allocate episode cache");
    }
    if (g.records) {
        g.branch = calloc(g.records, sizeof(*g.branch));
        if (!g.branch) fail("allocate branch cache");
    }
    for (size_t i = 0; i < g.count; i++) {
        const unsigned char *p = bytes + 16 + 4 * i;
        g.rule[i] = (Rule){le16(p), le16(p + 2)};
        if (g.rule[i].left >= 7 + i || g.rule[i].right >= 7 + i)
            fail("invalid episode topology");
        append_child(&g.forward[i], g.rule[i].left, g.forward);
        append_child(&g.forward[i], g.rule[i].right, g.forward);
    }
    for (size_t i = 0; i < g.records; i++) {
        const unsigned char *p = bytes + 16 + 4 * g.count + 16 * i;
        Branch *b = &g.branch[i];
        b->rule_id = p[0];
        b->prefix_len = p[1];
        for (int j = 0; j < 7; j++) b->counts[j] = le16(p + 2 + 2 * j);
        if (b->rule_id >= g.count || !b->prefix_len ||
            b->prefix_len >= g.forward[b->rule_id].length)
            fail("invalid branch record addressing");
        for (size_t j = 0; j < i; j++)
            if (g.branch[j].prefix_len == b->prefix_len &&
                !memcmp(g.forward[g.branch[j].rule_id].event,
                        g.forward[b->rule_id].event, b->prefix_len))
                fail("duplicate branch prefix content");
    }
    free(bytes);
    return g;
}

static Flat load_flat(const char *path, size_t budget) {
    Flat flat = {0};
    unsigned char *bytes = read_bytes(path, budget, &flat.bytes);
    if (flat.bytes < 16 || memcmp(bytes, "NETFI001", 8) || le32(bytes + 12))
        fail("invalid NETFI001 header");
    flat.count = le32(bytes + 8);
    if (flat.count > MAX_FLAT_RECORDS) fail("invalid flat count");
    if (flat.count) {
        flat.record = calloc(flat.count, sizeof(*flat.record));
        if (!flat.record) fail("allocate flat records");
    }
    size_t cursor = 16;
    for (size_t i = 0; i < flat.count; i++) {
        if (cursor >= flat.bytes) fail("truncated flat record");
        unsigned length = bytes[cursor++];
        if (!length || length > 31 || flat.bytes - cursor < length + 14)
            fail("invalid flat context length");
        FlatRecord *r = &flat.record[i];
        r->length = (uint8_t)length;
        memcpy(r->context, bytes + cursor, length);
        cursor += length;
        for (unsigned j = 0; j < length; j++)
            if (r->context[j] > 6) fail("invalid flat terminal");
        uint64_t total = 0;
        for (int j = 0; j < 7; j++) {
            r->counts[j] = le16(bytes + cursor);
            cursor += 2;
            total += r->counts[j];
        }
        if (total < 16) fail("insufficient flat source support");
        for (size_t j = 0; j < i; j++)
            if (flat.record[j].length == length &&
                !memcmp(flat.record[j].context, r->context, length))
                fail("duplicate flat context");
    }
    if (cursor != flat.bytes) fail("extra flat bytes");
    free(bytes);
    return flat;
}

static PRArchive load_row(const char *path) {
    PRArchive row;
    size_t length;
    unsigned char *bytes = read_bytes(path, PR_ARCHIVE_BYTES, &length);
    if (length != PR_ARCHIVE_BYTES || memcmp(bytes, "NETHD256", 8) ||
        le32(bytes + 8) != PR_DEPTH || le32(bytes + 12) != PR_CELLS)
        fail("invalid HEAD256 row archive");
    for (int i = 0; i < PR_CELLS; i++) row.counts[i] = le64(bytes + 16 + 8 * i);
    free(bytes);
    return row;
}

static double validate(const double logp[256]) {
    double total = 0;
    for (int b = 0; b < 256; b++) {
        if (!isfinite(logp[b]) || logp[b] > 1e-9) fail("invalid full log probability");
        double probability = exp2(logp[b]);
        if (!(probability > 0) || !isfinite(probability)) fail("nonpositive full probability");
        total += probability;
    }
    double error = fabs(total - 1.0);
    if (error > 1e-8) fail("full-vector normalization");
    return error;
}

static int rank_heads(const BFQuote *bf, uint8_t heads[BF_DEPTH], uint8_t rank[256]) {
    memset(rank, 0, 256);
    int k = 0;
    if (bf->context_len != BF_DEPTH) return 0;
    for (int i = BF_DEPTH - 1; i >= 0; i--) {
        uint8_t head = bf->unit_bytes[i][0];
        if (!rank[head]) {
            heads[k++] = head;
            rank[head] = (uint8_t)k;
        }
    }
    return k;
}

static void quote_cold(BFState *front, const PRLife *life, Quote *q) {
    memset(q, 0, sizeof(*q));
    if (bf_quote(front, &q->bf) || q->bf.t != life->events) fail("frontend quote/event");
    q->k = rank_heads(&q->bf, q->heads, q->rank);
    const char *pattern = q->k ? q->bf.pattern : NULL;
    if (q->k) {
        if (q->k != q->bf.repeat_classes) fail("rank/canonical class mismatch");
        double named = -INFINITY;
        for (int g = 0; g < q->k; g++) {
            q->mass[g] = q->bf.logp_base[q->bf.heads[g]];
            named = la(named, q->mass[g]);
        }
        q->mass[q->k] = log2(-expm1(named * log(2.0)));
    }
    if (pr_quote(life, &empty_archive, pattern, q->mass, 32, &q->local))
        fail("local PR quote");
    for (int b = 0; b < 256; b++) {
        int g = -1;
        if (q->k) {
            g = 0;
            while (g < q->k && q->bf.heads[g] != b) g++;
        }
        q->canonical_group[b] = g;
        q->cold[b] = q->bf.logp_base[b] + q->local.cold_scale_log2[g < 0 ? 0 : g];
    }
    (void)validate(q->bf.logp_base);
    (void)validate(q->cold);
}

static void observe_cold(BFState *front, PRLife *life, const Quote *q, uint8_t truth) {
    PRStep step;
    if (pr_observe(life, &q->local, q->canonical_group[truth],
                   q->bf.logp_base[truth], DBL_MAX, &step)) fail("local PR observe");
    if (step.cold_log2 != q->cold[truth] || step.candidate_log2 != q->cold[truth] ||
        step.live_log2 != q->cold[truth] || life->active) fail("cold learner changed authority");
    if (bf_observe(front, truth)) fail("frontend observe");
}

/* Greatest L whose stored prefix content equals the last L completed events.
   Deduplication by content makes that record unique. Nothing else votes. */
static Match branch_match(const Grammar *g, const History *h) {
    Match out = {0};
    for (size_t i = 0; i < g->records; i++) {
        const Branch *b = &g->branch[i];
        size_t length = b->prefix_len;
        if (length > h->length || (int)length <= out.length) continue;
        const uint8_t *content = g->forward[b->rule_id].event;
        if (memcmp(content, h->event + h->length - length, length)) continue;
        memset(&out, 0, sizeof(out));
        out.length = (int)length;
        out.nmatches = 1;
        out.matches[0] = (int)i;
        for (int j = 0; j < 7; j++) out.votes[j] = b->counts[j];
    }
    return out;
}

static Match flat_match(const Flat *flat, const History *h) {
    Match out = {0};
    for (size_t i = 0; i < flat->count; i++) {
        const FlatRecord *r = &flat->record[i];
        if (r->length <= h->length && r->length > out.length &&
            !memcmp(r->context, h->event + h->length - r->length, r->length)) {
            memset(&out, 0, sizeof(out));
            out.length = r->length;
            out.nmatches = 1;
            out.matches[0] = (int)i;
            for (int j = 0; j < 7; j++) out.votes[j] = r->counts[j];
        }
    }
    return out;
}

static void candidate(const Quote *q, const Match *match, double out[256]) {
    memcpy(out, q->cold, sizeof(q->cold));
    uint64_t total_votes = 0;
    for (int r = 1; r <= q->k; r++) total_votes += match->votes[r];
    if (!match->length || !total_votes || !q->k) return;
    double repeat_mass = -INFINITY;
    for (int r = 0; r < q->k; r++) repeat_mass = la(repeat_mass, q->cold[q->heads[r]]);
    for (int r = 1; r <= q->k; r++)
        out[q->heads[r - 1]] = repeat_mass +
            log2(((double)match->votes[r] + 0.5) / ((double)total_votes + 0.5 * q->k));
}

static int observe_outer(Outer *s, size_t t, double cold, double cand, double live) {
    double delta = cand - cold;
    s->gain += live - cold;
    s->shadow += delta;
    int activated = 0;
    if (s->active) {
        double z = s->odds + delta;
        s->odds = log1p(-PR_HMM_HAZARD) / log(2.0) - la(-z, log2(PR_HMM_HAZARD));
    } else if (s->shadow >= 32.0) {
        s->active = activated = 1;
        s->odds = 0;
        s->activation = t + 1;
    }
    if (!isfinite(s->odds) || !isfinite(s->shadow) || !isfinite(s->gain))
        fail("nonfinite outer state");
    if (s->gain < s->minimum) s->minimum = s->gain;
    if (s->gain > s->peak) s->peak = s->gain;
    if (s->peak - s->gain > s->drawdown) s->drawdown = s->peak - s->gain;
    if (s->minimum < -1.0 - 1e-7 || s->drawdown > 16.0 + 1e-7)
        fail("outer one-bit/sixteen-bit loss bound");
    return activated;
}

static void print_heads(const Quote *q, int prices) {
    if (!q->k) { putchar('-'); return; }
    for (int r = 0; r < q->k; r++) {
        if (r) putchar(',');
        if (prices) printf("%.17g", q->cold[q->heads[r]]);
        else printf("%u", (unsigned)q->heads[r]);
    }
}

static void print_match(const Match *match) {
    printf("\t%d\t", match->length);
    if (!match->nmatches) putchar('-');
    for (int j = 0; j < match->nmatches; j++)
        printf("%s%d", j ? "," : "", match->matches[j]);
    putchar('\t');
    for (int j = 0; j < 7; j++) printf("%s%" PRIu64, j ? "," : "", match->votes[j]);
}

static void stream_end(void) {
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
}

/* xorshift64*: nonzero uint64 state; shifts 12,25,27, then multiply by
   2685821657736338717 modulo 2^64. Draw alphabet[output & 63]; reject current
   repeat heads and redraw. Only NEW consumes RNG, with no modulo-64 bias. */
static uint64_t random64(uint64_t *state) {
    uint64_t x = *state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *state = x;
    return x * UINT64_C(2685821657736338717);
}

static void emit(const char *alphabet_path, const char *seed_text) {
    size_t n;
    unsigned char *alphabet = read_bytes(alphabet_path, 64, &n);
    if (n != 64) fail("alphabet must contain exactly 64 raw bytes");
    for (size_t i = 0; i < 64; i++) for (size_t j = 0; j < i; j++)
        if (alphabet[i] == alphabet[j]) fail("duplicate alphabet byte");
    uint64_t seed = 0;
    if (!*seed_text) fail("empty RNG seed");
    for (const char *p = seed_text; *p; p++) {
        if (*p < '0' || *p > '9' || seed > (UINT64_MAX - (unsigned)(*p - '0')) / 10)
            fail("seed must be a decimal uint64");
        seed = seed * 10 + (unsigned)(*p - '0');
    }
    if (!seed) fail("zero RNG seed");
    BFState *front = bf_create();
    if (!front) fail("allocate emitter frontend");
    size_t count = 0;
    for (;;) {
        BFQuote bf;
        uint8_t heads[BF_DEPTH], ranks[256];
        if (bf_quote(front, &bf)) fail("emitter quote");
        int k = rank_heads(&bf, heads, ranks);
        int command = fgetc(stdin);
        if (command == EOF) break;
        if (command > 3) fail("command outside 0..3");
        uint8_t byte;
        if (command > 0 && command <= k) byte = heads[command - 1];
        else do { byte = alphabet[random64(&seed) & 63]; } while (ranks[byte]);
        if (putchar(byte) == EOF || bf_observe(front, byte)) fail("emitter output/observe");
        count++;
    }
    stream_end();
    fprintf(stderr, "episode emit: bytes=%zu rng_final=%" PRIu64 "\n", count, seed);
    bf_destroy(front);
    free(alphabet);
}

static void trace(int compact) {
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate trace frontend");
    fputs("t\tpattern\tk\theads\ttruth\trank\tlogcold_truth", stdout);
    if (!compact) for (int b = 0; b < 256; b++) printf("\tlogcold%d", b);
    putchar('\n');
    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        printf("%zu\t%s\t%d\t", q.bf.t, q.bf.pattern, q.k);
        print_heads(&q, 0);
        printf("\t%u\t%u\t%.17g", (unsigned)truth, (unsigned)q.rank[truth], q.cold[truth]);
        if (!compact) for (int b = 0; b < 256; b++) printf("\t%.17g", q.cold[b]);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
    }
    stream_end();
    fprintf(stderr, "episode trace: bytes=%" PRIu64 " compact=%d\n", life.events, compact);
    bf_destroy(front);
}

static void predict(const char *episode_path, const char *isolated_path,
                    const char *frequency_path, const char *reverse_path,
                    const char *permuted_path, const char *flat_path,
                    const char *row_path) {
    Grammar grammar[NGRAMMARS] = {load_grammar(episode_path),
                                  load_grammar(isolated_path),
                                  load_grammar(frequency_path),
                                  load_grammar(reverse_path),
                                  load_grammar(permuted_path)};
    /* Every small arm has its own 528-byte cap, even if episode uses less. */
    Flat flat = load_flat(flat_path, MAX_ARCHIVE_BYTES);
    PRArchive row_archive = load_row(row_path);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate predictor frontend");
    Outer outer[NARMS] = {{0}};
    History history = {{0}, 0};
    fputs("t\tpattern\tk\theads\ttruth\trank\thistory", stdout);
    for (int a = 0; a < MATCH_ARMS; a++)
        printf("\t%s_matchedL\t%s_matches\t%s_votes", arm_names[a], arm_names[a], arm_names[a]);
    fputs("\tlogcold\tcold_heads", stdout);
    for (int a = 0; a < NARMS; a++) {
        const char *name = arm_names[a];
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before\t%s_active_before\t%s_activated_after\t%s_gain_after",
               name, name, name, name, name, name, name);
    }
    fputs("\tmax_norm_error\tnew_exact\n", stdout);
    double overall_norm = 0;
    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match matches[MATCH_ARMS] = {branch_match(&grammar[0], &history),
                                     branch_match(&grammar[1], &history),
                                     branch_match(&grammar[2], &history),
                                     branch_match(&grammar[3], &history),
                                     branch_match(&grammar[4], &history),
                                     flat_match(&flat, &history)};
        double candidates[NARMS][256], live[NARMS][256];
        for (int a = 0; a < MATCH_ARMS; a++) candidate(&q, &matches[a], candidates[a]);
        PRQuote row_quote;
        if (pr_quote(&life, &row_archive, q.k ? q.bf.pattern : NULL, q.mass, 32, &row_quote))
            fail("row source quote");
        for (int b = 0; b < 256; b++) {
            int g = q.canonical_group[b] < 0 ? 0 : q.canonical_group[b];
            if (row_quote.cold_scale_log2[g] != q.local.cold_scale_log2[g]) fail("row cold differs");
            candidates[NARMS - 1][b] = q.bf.logp_base[b] + row_quote.candidate_scale_log2[g];
        }
        double norm = fmax(validate(q.bf.logp_base), validate(q.cold));
        for (int a = 0; a < NARMS; a++) {
            for (int b = 0; b < 256; b++) {
                /* Exact equality is an algebraic HMM identity, including NEW. */
                live[a][b] = !outer[a].active || candidates[a][b] == q.cold[b]
                    ? q.cold[b] : la(q.cold[b], outer[a].odds + candidates[a][b]) - la(0, outer[a].odds);
                if (!q.rank[b] && (candidates[a][b] != q.cold[b] || live[a][b] != q.cold[b]))
                    fail("protected NEW changed");
            }
            norm = fmax(norm, validate(candidates[a]));
            norm = fmax(norm, validate(live[a]));
        }
        /* All 16 vectors, match decisions and NEW checks precede input. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        if (norm > overall_norm) overall_norm = norm;
        printf("%zu\t%s\t%d\t", q.bf.t, q.bf.pattern, q.k);
        print_heads(&q, 0);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        for (int a = 0; a < MATCH_ARMS; a++) print_match(&matches[a]);
        printf("\t%.17g\t", q.cold[truth]);
        print_heads(&q, 1);
        for (int a = 0; a < NARMS; a++) {
            double shadow = outer[a].shadow, odds = outer[a].odds;
            int active = outer[a].active;
            int activated = observe_outer(&outer[a], q.bf.t, q.cold[truth], candidates[a][truth], live[a][truth]);
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[a][truth], live[a][truth], shadow, odds, active, activated, outer[a].gain);
        }
        printf("\t%.17g\t1\n", norm);
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) {
            memmove(history.event, history.event + 1, HORIZON - 1);
            history.length--;
        }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    size_t expansion_cache = 0, rule_cache = 0, branch_cache = 0;
    for (int a = 0; a < NGRAMMARS; a++) {
        expansion_cache += grammar[a].count * sizeof(Expansion);
        rule_cache += grammar[a].count * sizeof(Rule);
        branch_cache += grammar[a].records * sizeof(Branch);
    }
    fprintf(stderr, "episode predict: bytes=%" PRIu64 " episode_archive_bytes=%zu isolated_archive_bytes=%zu frequency_archive_bytes=%zu "
                    "reverse_archive_bytes=%zu permuted_archive_bytes=%zu flat_archive_bytes=%zu row_archive_bytes=%d "
                    "rules=%zu branch_records=%zu flat_records=%zu grammar_expansion_cache_bytes=%zu "
                    "grammar_rule_cache_bytes=%zu branch_cache_bytes=%zu flat_cache_bytes=%zu "
                    "history_event_bytes=%zu history_struct_bytes=%zu outer_state_bytes=%zu "
                    "cold_state_bytes=%zu quote_struct_bytes=%zu candidate_live_vector_bytes=%zu max_norm_error=%.17g\n",
            life.events, grammar[0].bytes, grammar[1].bytes, grammar[2].bytes, grammar[3].bytes, grammar[4].bytes, flat.bytes, PR_ARCHIVE_BYTES,
            grammar[0].count, grammar[0].records, flat.count, expansion_cache, rule_cache, branch_cache,
            flat.count * sizeof(FlatRecord), sizeof(history.event), sizeof(history), sizeof(outer),
            sizeof(life), sizeof(Quote), (size_t)2 * NARMS * 256 * sizeof(double), overall_norm);
    for (int a = 0; a < NARMS; a++)
        fprintf(stderr, "%s gain=%.17g shadow=%.17g odds=%.17g active=%d activation=%zu minimum=%.17g drawdown=%.17g\n",
                arm_names[a], outer[a].gain, outer[a].shadow, outer[a].odds, outer[a].active,
                outer[a].activation, outer[a].minimum, outer[a].drawdown);
    bf_destroy(front);
    for (int a = 0; a < NGRAMMARS; a++) {
        free(grammar[a].rule);
        free(grammar[a].forward);
        free(grammar[a].branch);
    }
    free(flat.record);
}

int main(int argc, char **argv) {
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 4 && !strcmp(argv[1], "emit")) emit(argv[2], argv[3]);
    else if ((argc == 2 || (argc == 3 && !strcmp(argv[2], "compact"))) && !strcmp(argv[1], "trace")) trace(argc == 3);
    else if (argc == 9 && !strcmp(argv[1], "predict")) predict(argv[2], argv[3], argv[4], argv[5], argv[6], argv[7], argv[8]);
    else {
        fputs("usage: episode emit ALPHABET_FILE NONZERO_SEED < COMMANDS\n"
              "       episode trace [compact] < RAW\n"
              "       episode predict EPISODE.bin ISOLATED.bin FREQUENCY.bin REVERSE.bin PERMUTED.bin FLAT.bin ROW.bin < RAW\n", stderr);
        return 2;
    }
    return 0;
}
