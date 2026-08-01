/*
 * netta.c — NETTA's Experiential Text Training Architecture
 *
 * A sovereign recursive language learner.
 * No backpropagation. No external ML library.
 *
 * Build:
 *   cc -O2 -std=c11 -Wall -Wextra -o netta netta.c -lm
 *
 * Run:
 *   ./netta netta.txt
 *
 * Optional:
 *   ./netta netta.txt --steps 5000
 *   ./netta netta.txt --prompt "Bianca"
 *   ./netta netta.txt --reset
 *
 * Files:
 *   netta.state        persistent learned state
 *   netta.history.tsv  immutable episode ledger
 *
 * The source corpus, PostGPT-like oracle line, and Netta attempt are
 * kept separate. Negative experience is remembered but never promoted
 * into source truth.
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_VOCAB       8192
#define MAX_TOKEN_LEN   48
#define MAX_CORPUS      300000
#define MAX_EDGES       500000
#define MAX_TRIGRAMS    350000
#define TRI_BUCKETS     262144
#define EMBED_DIM       24
#define HIDDEN_DIM      32
#define STATE_DIM       24
#define SCORE_DIM       8
#define CONTEXT         16
#define ROLLOUT         8
#define CANDIDATES      32
#define PHRASE_TABLE    131072
#define BASIN_MEMORY    256
#define TOP_SUCCESSORS  12
#define STATE_MAGIC     0x4E455454u /* NETT */
#define STATE_VERSION   13u

typedef struct {
    char text[MAX_TOKEN_LEN];
    uint32_t count;
    float emb[EMBED_DIM];        /* undirected semantic field */
    float left_emb[EMBED_DIM];   /* contexts that tend to precede token */
    float right_emb[EMBED_DIM];  /* contexts that tend to follow token */
} Token;

typedef struct {
    uint32_t from;
    uint32_t to;
    uint32_t source_count;

    /* Mark-to-market experiential value, never a permanent prize. */
    float quote;          /* current relative value of this transition */
    float debt;           /* unresolved prophecy error */
    float volatility;     /* how unstable the transition's value is */
    float momentum;       /* recent direction of revaluation */
    float support;        /* slow positive evidence */
    float opposition;     /* slow negative evidence */

    uint32_t positive_uses;
    uint32_t negative_uses;
    uint64_t last_mark_episode;
    int32_t next_from;
} Edge;

typedef struct {
    uint32_t a;
    uint32_t b;
    uint32_t c;
    uint32_t count;
    int32_t next_bucket;
} TrigramEdge;

typedef struct {
    float syntax_local;
    float source_grounding;
    float oracle_parity;
    float semantic_continuity;
    float intent_fidelity;
    float novelty;
    float rollout_stability;
    float anti_repetition;
} ScoreVector;

typedef struct {
    int prev;
    int chosen;
    int oracle;
    float predicted[SCORE_DIM];
    ScoreVector immediate;
} TrajectoryStep;

typedef struct {
    float wxh[HIDDEN_DIM][EMBED_DIM * 4 + STATE_DIM];
    float whh[HIDDEN_DIM][HIDDEN_DIM];
    float who[SCORE_DIM][HIDDEN_DIM];
    float hidden[HIDDEN_DIM];
    float state[STATE_DIM];
} Core;

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t vocab_size;
    uint32_t edge_count;
    uint64_t episodes;
    Core core;
    int32_t basin_count;
    int32_t basin_cursor;
} StateHeader;

static Token vocab[MAX_VOCAB];
static int vocab_size = 0;
static int corpus[MAX_CORPUS];
static int corpus_n = 0;

static Edge edges[MAX_EDGES];
static int edge_count = 0;
static int first_edge[MAX_VOCAB];

static TrigramEdge trigrams[MAX_TRIGRAMS];
static int trigram_count_total = 0;
static int first_trigram[TRI_BUCKETS];

static int top_successors[MAX_VOCAB][TOP_SUCCESSORS];
static uint32_t top_successor_counts[MAX_VOCAB][TOP_SUCCESSORS];
static uint8_t top_successor_n[MAX_VOCAB];

static Core core;
static uint64_t episode_count = 0;
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static uint64_t recursive_depth_total = 0;
static uint64_t recursive_call_total = 0;

/* Anti-cheat memory: generated phrase trajectories and recent semantic basins. */
static uint32_t phrase_counts[PHRASE_TABLE];
static float basin_memory[BASIN_MEMORY][EMBED_DIM];
static uint32_t basin_uses[BASIN_MEMORY];
static int basin_count = 0;
static int basin_cursor = 0;

static uint64_t rng_u64(void) {
    uint64_t x = rng_state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng_state = x;
    return x * 2685821657736338717ull;
}

static float randf(void) {
    return (float)((rng_u64() >> 40) & 0xFFFFFFu) / 16777216.0f;
}

static float randn(float scale) {
    float u1 = randf() + 1e-7f;
    float u2 = randf();
    return scale * sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
}

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

static float vec_dot(const float *a, const float *b, int n) {
    float s = 0.0f;
    for (int i = 0; i < n; ++i) s += a[i] * b[i];
    return s;
}

static float vec_norm(const float *a, int n) {
    return sqrtf(vec_dot(a, a, n) + 1e-8f);
}

static float cosine(const float *a, const float *b, int n) {
    return vec_dot(a, b, n) / (vec_norm(a, n) * vec_norm(b, n));
}

static void normalize(float *a, int n) {
    float z = vec_norm(a, n);
    for (int i = 0; i < n; ++i) a[i] /= z;
}

static unsigned hash_word(const char *s) {
    unsigned h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 16777619u;
    }
    return h;
}

/* Open-addressed vocabulary index. */
static int vocab_slots[MAX_VOCAB * 2];

static int token_id(const char *word, int create) {
    unsigned mask = (MAX_VOCAB * 2) - 1;
    unsigned h = hash_word(word) & mask;

    for (unsigned probe = 0; probe <= mask; ++probe) {
        unsigned idx = (h + probe) & mask;
        int slot = vocab_slots[idx];

        if (slot == 0) {
            if (!create || vocab_size >= MAX_VOCAB) return -1;
            int id = vocab_size++;
            vocab_slots[idx] = id + 1;
            snprintf(vocab[id].text, MAX_TOKEN_LEN, "%s", word);
            vocab[id].count = 0;
            for (int d = 0; d < EMBED_DIM; ++d) {
                float seed = randn(0.08f);
                vocab[id].emb[d] = seed;
                vocab[id].left_emb[d] = seed + randn(0.015f);
                vocab[id].right_emb[d] = seed + randn(0.015f);
            }
            normalize(vocab[id].emb, EMBED_DIM);
            normalize(vocab[id].left_emb, EMBED_DIM);
            normalize(vocab[id].right_emb, EMBED_DIM);
            return id;
        }

        int id = slot - 1;
        if (strncmp(vocab[id].text, word, MAX_TOKEN_LEN) == 0)
            return id;
    }
    return -1;
}


static int token_is_content_id(int id) {
    if (id < 0 || id >= vocab_size) return 0;
    const char *t = vocab[id].text;
    if (t[0] == '\0' || strcmp(t, "\n") == 0) return 0;

    int has_alnum = 0;
    for (int i = 0; t[i]; ++i) {
        unsigned char c = (unsigned char)t[i];
        if (c >= 128 || isalnum(c)) {
            has_alnum = 1;
            break;
        }
    }
    return has_alnum;
}

static int is_punct_token(int c) {
    return c == '.' || c == ',' || c == ';' || c == ':' ||
           c == '!' || c == '?' || c == '(' || c == ')' ||
           c == '"' || c == '\'' || c == '-' || c == '\n';
}

static void emit_token(char *buf, int *len) {
    if (*len <= 0 || corpus_n >= MAX_CORPUS) return;
    buf[*len] = '\0';

    /* ASCII lowercase; UTF-8 bytes remain stable rather than destroyed. */
    for (int i = 0; i < *len; ++i) {
        unsigned char c = (unsigned char)buf[i];
        if (c < 128) buf[i] = (char)tolower(c);
    }

    int id = token_id(buf, 1);
    if (id >= 0) {
        corpus[corpus_n++] = id;
        vocab[id].count++;
    }
    *len = 0;
}

static int load_corpus(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "netta: cannot open %s: %s\n", path, strerror(errno));
        return 0;
    }

    memset(vocab_slots, 0, sizeof(vocab_slots));
    memset(first_edge, 0xFF, sizeof(first_edge));
    memset(first_trigram, 0xFF, sizeof(first_trigram));
    memset(top_successors, 0xFF, sizeof(top_successors));
    memset(top_successor_counts, 0, sizeof(top_successor_counts));
    memset(top_successor_n, 0, sizeof(top_successor_n));

    char buf[MAX_TOKEN_LEN];
    int len = 0;
    int c;

    while ((c = fgetc(f)) != EOF && corpus_n < MAX_CORPUS) {
        if (isspace((unsigned char)c)) {
            emit_token(buf, &len);
        } else if (is_punct_token(c)) {
            emit_token(buf, &len);
            char p[2] = {(char)c, '\0'};
            int id = token_id(p, 1);
            if (id >= 0) {
                corpus[corpus_n++] = id;
                vocab[id].count++;
            }
        } else if (len < MAX_TOKEN_LEN - 1) {
            buf[len++] = (char)c;
        }
    }
    emit_token(buf, &len);
    fclose(f);

    return corpus_n > CONTEXT + ROLLOUT;
}

static int find_edge(int from, int to) {
    for (int e = first_edge[from]; e >= 0; e = edges[e].next_from)
        if ((int)edges[e].to == to) return e;
    return -1;
}

static int get_edge(int from, int to, int create) {
    int e = find_edge(from, to);
    if (e >= 0 || !create || edge_count >= MAX_EDGES) return e;

    e = edge_count++;
    edges[e].from = (uint32_t)from;
    edges[e].to = (uint32_t)to;
    edges[e].source_count = 0;
    edges[e].quote = 0.0f;
    edges[e].debt = 1.0f;
    edges[e].volatility = 0.0f;
    edges[e].momentum = 0.0f;
    edges[e].support = 0.0f;
    edges[e].opposition = 0.0f;
    edges[e].positive_uses = 0;
    edges[e].negative_uses = 0;
    edges[e].last_mark_episode = 0;
    edges[e].next_from = first_edge[from];
    first_edge[from] = e;
    return e;
}


static unsigned trigram_bucket(int a, int b) {
    uint32_t h = (uint32_t)a * 2654435761u;
    h ^= (uint32_t)b * 2246822519u;
    h ^= h >> 16;
    return h & (TRI_BUCKETS - 1);
}

static int get_trigram(int a, int b, int c, int create) {
    unsigned bucket = trigram_bucket(a, b);
    for (int t = first_trigram[bucket]; t >= 0;
         t = trigrams[t].next_bucket) {
        if ((int)trigrams[t].a == a &&
            (int)trigrams[t].b == b &&
            (int)trigrams[t].c == c)
            return t;
    }

    if (!create || trigram_count_total >= MAX_TRIGRAMS)
        return -1;

    int t = trigram_count_total++;
    trigrams[t].a = (uint32_t)a;
    trigrams[t].b = (uint32_t)b;
    trigrams[t].c = (uint32_t)c;
    trigrams[t].count = 0;
    trigrams[t].next_bucket = first_trigram[bucket];
    first_trigram[bucket] = t;
    return t;
}

static uint32_t source_trigram_count(int a, int b, int c) {
    int t = get_trigram(a, b, c, 0);
    return t >= 0 ? trigrams[t].count : 0;
}


static void build_top_successors(void) {
    for (int from = 0; from < vocab_size; ++from) {
        for (int e = first_edge[from]; e >= 0; e = edges[e].next_from) {
            int tok = (int)edges[e].to;
            uint32_t count = edges[e].source_count;
            int n = top_successor_n[from];
            int slot = n;

            if (slot < TOP_SUCCESSORS) {
                top_successor_n[from]++;
            } else if (count <= top_successor_counts[from][TOP_SUCCESSORS - 1]) {
                continue;
            } else {
                slot = TOP_SUCCESSORS - 1;
            }

            while (slot > 0 &&
                   count > top_successor_counts[from][slot - 1]) {
                if (slot < TOP_SUCCESSORS) {
                    top_successor_counts[from][slot] =
                        top_successor_counts[from][slot - 1];
                    top_successors[from][slot] =
                        top_successors[from][slot - 1];
                }
                slot--;
            }

            top_successor_counts[from][slot] = count;
            top_successors[from][slot] = tok;
        }
    }
}

static void build_source_graph(void) {
    for (int i = 0; i + 1 < corpus_n; ++i) {
        int e = get_edge(corpus[i], corpus[i + 1], 1);
        if (e >= 0) edges[e].source_count++;
    }

    for (int i = 0; i + 2 < corpus_n; ++i) {
        int t = get_trigram(corpus[i], corpus[i + 1], corpus[i + 2], 1);
        if (t >= 0) trigrams[t].count++;
    }

    build_top_successors();

    /* Corpus-derived embeddings: local Hebbian co-occurrence. */
    int window = 5;
    for (int i = 0; i < corpus_n; ++i) {
        int a = corpus[i];
        int hi = i + window + 1;
        if (hi > corpus_n) hi = corpus_n;
        for (int j = i + 1; j < hi; ++j) {
            int b = corpus[j];
            float rate = 0.015f / (float)(j - i);
            for (int d = 0; d < EMBED_DIM; ++d) {
                float av = vocab[a].emb[d];
                float bv = vocab[b].emb[d];

                /* Undirected topic geometry. */
                vocab[a].emb[d] += rate * bv;
                vocab[b].emb[d] += rate * av;

                /* Directed role geometry: a -> b. */
                vocab[a].right_emb[d] += rate * vocab[b].emb[d];
                vocab[b].left_emb[d]  += rate * vocab[a].emb[d];
            }
        }
    }
    for (int i = 0; i < vocab_size; ++i) {
        normalize(vocab[i].emb, EMBED_DIM);
        normalize(vocab[i].left_emb, EMBED_DIM);
        normalize(vocab[i].right_emb, EMBED_DIM);
    }
}

static void core_init(void) {
    memset(&core, 0, sizeof(core));

    int input_dim = EMBED_DIM * 4 + STATE_DIM;
    for (int h = 0; h < HIDDEN_DIM; ++h) {
        for (int i = 0; i < input_dim; ++i)
            core.wxh[h][i] = randn(0.05f);
        for (int j = 0; j < HIDDEN_DIM; ++j)
            core.whh[h][j] = randn(0.03f);
    }

    for (int o = 0; o < SCORE_DIM; ++o)
        for (int h = 0; h < HIDDEN_DIM; ++h)
            core.who[o][h] = randn(0.04f);
}

static void context_embedding(const int *ctx, int n, float *out) {
    memset(out, 0, EMBED_DIM * sizeof(float));
    if (n <= 0) return;

    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        float w = 1.0f + (float)i / (float)n;
        total += w;
        for (int d = 0; d < EMBED_DIM; ++d)
            out[d] += w * vocab[ctx[i]].emb[d];
    }
    for (int d = 0; d < EMBED_DIM; ++d) out[d] /= total;
    normalize(out, EMBED_DIM);
}

static void intent_from_context(const int *ctx, int n, float *out) {
    float present[EMBED_DIM];
    float direction[EMBED_DIM] = {0};
    context_embedding(ctx, n, present);

    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        float w = 0.5f + (float)(i + 1) / (float)n;
        total += w;
        for (int d = 0; d < EMBED_DIM; ++d)
            direction[d] += w * vocab[ctx[i]].right_emb[d];
    }
    if (total > 0.0f)
        for (int d = 0; d < EMBED_DIM; ++d)
            direction[d] /= total;
    normalize(direction, EMBED_DIM);

    /*
     * Intent is not the current topic centroid. It is the direction in which
     * the current discourse statistically wants to unfold.
     */
    for (int d = 0; d < EMBED_DIM; ++d)
        out[d] = 0.40f * present[d] + 0.60f * direction[d];
    normalize(out, EMBED_DIM);
}


static void core_predict(const float *ctx_emb, const float *intent,
                         int candidate, int oracle,
                         float out[SCORE_DIM], int commit_state) {
    float input[EMBED_DIM * 4 + STATE_DIM];
    int k = 0;

    for (int d = 0; d < EMBED_DIM; ++d) input[k++] = ctx_emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) input[k++] = vocab[candidate].emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) input[k++] = vocab[oracle].emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) input[k++] = intent[d];
    for (int d = 0; d < STATE_DIM; ++d) input[k++] = core.state[d];

    float next_hidden[HIDDEN_DIM];
    for (int h = 0; h < HIDDEN_DIM; ++h) {
        float s = vec_dot(core.wxh[h], input, k);
        s += vec_dot(core.whh[h], core.hidden, HIDDEN_DIM);
        next_hidden[h] = tanhf(s);
    }

    for (int o = 0; o < SCORE_DIM; ++o)
        out[o] = tanhf(vec_dot(core.who[o], next_hidden, HIDDEN_DIM));

    if (commit_state) {
        memcpy(core.hidden, next_hidden, sizeof(next_hidden));
        for (int d = 0; d < STATE_DIM; ++d) {
            float source = next_hidden[d % HIDDEN_DIM];
            core.state[d] = 0.94f * core.state[d] + 0.06f * source;
        }
    }
}

static int recursive_depth_for(int prev, int candidate,
                               float intent_debt,
                               int *min_depth_out) {
    int e = find_edge(prev, candidate);
    float debt = e >= 0 ? edges[e].debt : 1.0f;
    float volatility = e >= 0 ? edges[e].volatility : 0.35f;
    float uncertainty =
        debt + 0.85f * intent_debt + 2.5f * volatility;

    int min_depth =
        2 + (int)floorf(clampf(uncertainty, 0.0f, 2.0f));
    int max_depth =
        min_depth + 2 +
        (int)floorf(clampf(debt + 0.6f * intent_debt,
                           0.0f, 2.0f));
    if (min_depth < 2) min_depth = 2;
    if (min_depth > 4) min_depth = 4;
    if (max_depth < min_depth) max_depth = min_depth;
    if (max_depth > 7) max_depth = 7;

    *min_depth_out = min_depth;
    return max_depth;
}

static int core_predict_recursive(const float *ctx_emb, const float *intent,
                                  float intent_debt,
                                  int prev, int candidate, int oracle,
                                  const float prior[SCORE_DIM],
                                  float out[SCORE_DIM], int commit_state) {
    float saved_hidden[HIDDEN_DIM];
    float saved_state[STATE_DIM];
    memcpy(saved_hidden, core.hidden, sizeof(saved_hidden));
    memcpy(saved_state, core.state, sizeof(saved_state));

    int min_depth = 2;
    int max_depth =
        recursive_depth_for(prev, candidate, intent_debt, &min_depth);
    float last[SCORE_DIM] = {0};
    int used = 0;

    int e = find_edge(prev, candidate);
    float debt = e >= 0 ? edges[e].debt : 1.0f;
    float debt_scale = clampf(debt / 2.0f, 0.0f, 1.0f);
    float consensus_threshold = 0.17f - 0.07f * debt_scale;

    for (int depth = 0; depth < max_depth; ++depth) {
        core_predict(ctx_emb, intent, candidate, oracle, out, 1);
        used++;

        if (depth > 0) {
            float change = 0.0f;
            float consensus = 0.0f;
            for (int o = 0; o < SCORE_DIM; ++o) {
                change += fabsf(out[o] - last[o]);
                float prophecy01 = 0.5f * (out[o] + 1.0f);
                consensus += fabsf(prophecy01 - prior[o]);
            }
            change /= (float)SCORE_DIM;
            consensus /= (float)SCORE_DIM;

            /*
             * Internal self-agreement is insufficient. The shared block may
             * settle only when it also reaches tolerable agreement with the
             * metaweight/intent prior.
             */
            if (used >= min_depth &&
                change < 0.0075f &&
                consensus < consensus_threshold)
                break;
        }
        memcpy(last, out, sizeof(last));
    }

    recursive_depth_total += (uint64_t)used;
    recursive_call_total++;

    if (!commit_state) {
        memcpy(core.hidden, saved_hidden, sizeof(saved_hidden));
        memcpy(core.state, saved_state, sizeof(saved_state));
    }
    return used;
}

static int oracle_next(int prev) {
    uint64_t total = 0;
    for (int e = first_edge[prev]; e >= 0; e = edges[e].next_from)
        total += edges[e].source_count;

    if (total == 0)
        return (int)(rng_u64() % (uint64_t)vocab_size);

    uint64_t r = rng_u64() % total;
    uint64_t acc = 0;
    for (int e = first_edge[prev]; e >= 0; e = edges[e].next_from) {
        acc += edges[e].source_count;
        if (r < acc) return (int)edges[e].to;
    }
    return prev;
}


static int oracle_next_context(const int *ctx, int ctx_n) {
    if (ctx_n >= 2) {
        int a = ctx[ctx_n - 2];
        int b = ctx[ctx_n - 1];
        unsigned bucket = trigram_bucket(a, b);
        uint64_t total = 0;

        int tri_scanned = 0;
        for (int t = first_trigram[bucket]; t >= 0 && tri_scanned < 20;
             t = trigrams[t].next_bucket) {
            tri_scanned++;
            if ((int)trigrams[t].a == a && (int)trigrams[t].b == b)
                total += trigrams[t].count;
        }

        if (total > 0) {
            uint64_t r = rng_u64() % total;
            uint64_t acc = 0;
            for (int t = first_trigram[bucket]; t >= 0;
                 t = trigrams[t].next_bucket) {
                if ((int)trigrams[t].a != a || (int)trigrams[t].b != b)
                    continue;
                acc += trigrams[t].count;
                if (r < acc)
                    return (int)trigrams[t].c;
            }
        }
    }

    return oracle_next(ctx[ctx_n - 1]);
}

static void intent_update(float *intent, float *debt, int chosen) {
    float fulfillment =
        fmaxf(0.0f, cosine(vocab[chosen].emb, intent, EMBED_DIM));

    *debt = clampf(0.92f * (*debt) +
                   0.08f * (1.0f - fulfillment), 0.05f, 2.0f);

    for (int d = 0; d < EMBED_DIM; ++d) {
        intent[d] =
            0.76f * intent[d] -
            0.16f * fulfillment * vocab[chosen].emb[d] +
            0.24f * vocab[chosen].right_emb[d];
    }
    normalize(intent, EMBED_DIM);
}


static float directional_compatibility(int from, int to) {
    float forward = 0.5f * (1.0f + cosine(vocab[from].right_emb,
                                         vocab[to].emb, EMBED_DIM));
    float arrival = 0.5f * (1.0f + cosine(vocab[to].left_emb,
                                         vocab[from].emb, EMBED_DIM));
    return 0.5f * (forward + arrival);
}

static float edge_market_value(const Edge *e) {
    if (!e) return 0.0f;
    float risk = 1.0f + e->debt + 0.75f * e->volatility;
    return (e->quote + 0.25f * e->momentum) / risk;
}

static float learned_relation_score(int from, int to) {
    int e = find_edge(from, to);
    if (e < 0) return 0.0f;

    float slow = (edges[e].support - edges[e].opposition) /
                 (1.0f + edges[e].support + edges[e].opposition);
    float market = edge_market_value(&edges[e]);

    /* Old success cannot remain valuable without current confirmation. */
    uint64_t age = episode_count - edges[e].last_mark_episode;
    float freshness = 1.0f / sqrtf(1.0f + (float)age / 250.0f);
    float fatigue = 1.0f /
        sqrtf(1.0f + (float)edges[e].positive_uses / 8.0f);

    return clampf((0.55f * market + 0.45f * slow) *
                  freshness * fatigue, -1.0f, 1.0f);
}

static int repeated_recently(const int *seq, int n, int token) {
    int start = n - 6;
    if (start < 0) start = 0;
    int count = 0;
    for (int i = start; i < n; ++i)
        if (seq[i] == token) count++;
    return count;
}

static float cycle_freshness(const int *ctx, int ctx_n, int candidate);
static float semantic_cycle_freshness(const int *ctx, int ctx_n,
                                      int candidate);
static float counterfactual_rollout_score(const int *ctx, int ctx_n,
                                          int candidate,
                                          const float *intent,
                                          float current_intent_debt);

static ScoreVector observe_score(const int *ctx, int ctx_n, int candidate,
                                 int oracle, int truth,
                                 const float *intent,
                                 float intent_debt) {
    ScoreVector s;
    memset(&s, 0, sizeof(s));

    int prev = ctx[ctx_n - 1];
    int e = find_edge(prev, candidate);
    uint32_t src = e >= 0 ? edges[e].source_count : 0;
    uint32_t tri = ctx_n >= 2 ?
        source_trigram_count(ctx[ctx_n - 2], prev, candidate) : 0;

    float local_evidence = (float)src + 2.5f * (float)tri;
    s.syntax_local = 1.0f - expf(-local_evidence * 0.25f);
    s.source_grounding = candidate == truth ? 1.0f :
        0.5f * (1.0f + cosine(vocab[candidate].emb, vocab[truth].emb, EMBED_DIM));
    s.oracle_parity = candidate == oracle ? 1.0f :
        0.5f * (1.0f + cosine(vocab[candidate].emb, vocab[oracle].emb, EMBED_DIM));

    float cemb[EMBED_DIM];
    context_embedding(ctx, ctx_n, cemb);
    float semantic_field =
        0.5f * (1.0f + cosine(cemb, vocab[candidate].emb, EMBED_DIM));
    float role_order = directional_compatibility(prev, candidate);
    s.semantic_continuity = 0.58f * semantic_field + 0.42f * role_order;

    float next_discourse[EMBED_DIM];
    for (int d = 0; d < EMBED_DIM; ++d)
        next_discourse[d] = 0.72f * cemb[d] + 0.28f * vocab[candidate].emb[d];
    normalize(next_discourse, EMBED_DIM);

    float before_intent = cosine(cemb, intent, EMBED_DIM);
    float after_intent = cosine(next_discourse, intent, EMBED_DIM);
    float progress = after_intent - before_intent;
    float progress_score = clampf(0.5f + 3.5f * progress, 0.0f, 1.0f);
    float absolute_score = 0.5f * (1.0f + after_intent);
    s.intent_fidelity = 0.78f * progress_score + 0.22f * absolute_score;

    float freq = (float)vocab[candidate].count / (float)(corpus_n + 1);
    s.novelty = clampf(1.0f - 20.0f * freq, 0.0f, 1.0f);

    int rollout_ctx[CONTEXT + 1];
    int rollout_n = ctx_n < CONTEXT ? ctx_n : CONTEXT;
    memcpy(rollout_ctx, &ctx[ctx_n - rollout_n],
           rollout_n * sizeof(int));
    rollout_ctx[rollout_n++] = candidate;
    int next_oracle = oracle_next_context(rollout_ctx, rollout_n);
    float immediate_future =
        0.5f * (1.0f + cosine(vocab[next_oracle].emb,
                              vocab[truth].emb, EMBED_DIM));
    float imagined_future =
        counterfactual_rollout_score(ctx, ctx_n, candidate, intent, intent_debt);
    s.rollout_stability =
        0.45f * immediate_future + 0.55f * imagined_future;

    int rep = repeated_recently(ctx, ctx_n, candidate);
    float cycle = cycle_freshness(ctx, ctx_n, candidate);
    float semantic_cycle =
        semantic_cycle_freshness(ctx, ctx_n, candidate);
    s.anti_repetition =
        (1.0f / (1.0f + (float)rep)) *
        cycle * semantic_cycle;
    s.novelty *=
        (0.65f + 0.35f * cycle) *
        (0.55f + 0.45f * semantic_cycle);

    if (!token_is_content_id(candidate)) {
        s.source_grounding *= 0.25f;
        s.oracle_parity *= 0.25f;
        s.semantic_continuity *= 0.10f;
        s.intent_fidelity *= 0.10f;
        s.novelty *= 0.05f;
        s.rollout_stability *= 0.25f;
        s.anti_repetition *= 0.10f;
    }

    return s;
}

static void score_to_array(ScoreVector s, float out[SCORE_DIM]) {
    out[0] = s.syntax_local;
    out[1] = s.source_grounding;
    out[2] = s.oracle_parity;
    out[3] = s.semantic_continuity;
    out[4] = s.intent_fidelity;
    out[5] = s.novelty;
    out[6] = s.rollout_stability;
    out[7] = s.anti_repetition;
}

/*
 * Pareto preference with coherence-first tie breaking.
 * No permanent scalar "truth" is stored.
 */



static float cycle_freshness(const int *ctx, int ctx_n, int candidate) {
    /*
     * Detect local ABAB / phrase-loop closure without banning ordinary reuse.
     * The penalty is relational: it asks whether the new suffix already
     * occurred earlier in the active context.
     */
    float freshness = 1.0f;

    for (int len = 2; len <= 8; ++len) {
        if (ctx_n + 1 < len) continue;

        int suffix[8];
        for (int k = 0; k < len - 1; ++k)
            suffix[k] = ctx[ctx_n - (len - 1) + k];
        suffix[len - 1] = candidate;

        for (int start = 0; start + len <= ctx_n; ++start) {
            int same = 1;
            for (int k = 0; k < len; ++k) {
                if (ctx[start + k] != suffix[k]) {
                    same = 0;
                    break;
                }
            }
            if (same) {
                float p;
                if (len == 2) p = 0.12f;
                else if (len == 3) p = 0.22f;
                else if (len == 4) p = 0.34f;
                else if (len <= 6) p = 0.46f;
                else p = 0.58f;
                if (p < freshness) freshness = p;
            }
        }
    }
    return freshness;
}


static float semantic_cycle_freshness(const int *ctx, int ctx_n,
                                      int candidate) {
    /*
     * Exact n-gram checks miss paraphrastic loops. Compare the predicted
     * four-token suffix with earlier four-token windows in the active
     * context. A return to nearly the same semantic state is a soft
     * fixed point and receives less freshness.
     */
    if (ctx_n < 7) return 1.0f;

    float suffix[EMBED_DIM] = {0};
    int suffix_start = ctx_n - 3;
    if (suffix_start < 0) suffix_start = 0;
    int suffix_count = 0;

    for (int i = suffix_start; i < ctx_n; ++i) {
        for (int d = 0; d < EMBED_DIM; ++d)
            suffix[d] += vocab[ctx[i]].emb[d];
        suffix_count++;
    }
    for (int d = 0; d < EMBED_DIM; ++d)
        suffix[d] += vocab[candidate].emb[d];
    suffix_count++;

    for (int d = 0; d < EMBED_DIM; ++d)
        suffix[d] /= (float)suffix_count;
    normalize(suffix, EMBED_DIM);

    float max_sim = -1.0f;
    for (int start = 0; start + 4 <= ctx_n - 3; ++start) {
        float window[EMBED_DIM] = {0};
        for (int i = start; i < start + 4; ++i)
            for (int d = 0; d < EMBED_DIM; ++d)
                window[d] += 0.25f * vocab[ctx[i]].emb[d];
        normalize(window, EMBED_DIM);

        float sim = cosine(suffix, window, EMBED_DIM);
        if (sim > max_sim) max_sim = sim;
    }

    if (max_sim < 0.86f) return 1.0f;
    float pressure = clampf((max_sim - 0.86f) / 0.14f, 0.0f, 1.0f);
    return 1.0f - 0.82f * pressure;
}

static uint32_t phrase_hash_tokens(const int *seq, int n, int candidate) {
    uint32_t h = 2166136261u;
    int start = n - 8;
    if (start < 0) start = 0;
    for (int i = start; i < n; ++i) {
        h ^= (uint32_t)(seq[i] + 0x9e3779b9u);
        h *= 16777619u;
    }
    h ^= (uint32_t)(candidate + 0x85ebca6bu);
    h *= 16777619u;
    return h & (PHRASE_TABLE - 1);
}

static uint32_t phrase_use_count(const int *ctx, int ctx_n, int candidate) {
    uint32_t h = phrase_hash_tokens(ctx, ctx_n, candidate);
    return phrase_counts[h];
}

static float phrase_freshness(const int *ctx, int ctx_n, int candidate) {
    uint32_t c = phrase_use_count(ctx, ctx_n, candidate);
    return 1.0f / (1.0f + (float)c);
}

static void phrase_remember(const int *ctx, int ctx_n, int candidate) {
    uint32_t h = phrase_hash_tokens(ctx, ctx_n, candidate);
    if (phrase_counts[h] < UINT32_MAX) phrase_counts[h]++;
}

static float semantic_basin_freshness(const float *candidate_emb) {
    if (basin_count == 0) return 1.0f;

    int checks = basin_count < 64 ? basin_count : 64;
    float max_sim = -1.0f;

    for (int k = 0; k < checks; ++k) {
        int i;
        if (basin_count < BASIN_MEMORY) {
            i = basin_count - 1 - k;
        } else {
            i = basin_cursor - 1 - k;
            while (i < 0) i += BASIN_MEMORY;
        }

        float sim = cosine(candidate_emb, basin_memory[i], EMBED_DIM);
        if (sim > max_sim) max_sim = sim;
    }

    float penalty = clampf((max_sim - 0.72f) / 0.28f, 0.0f, 1.0f);
    return 1.0f - 0.75f * penalty;
}

static void basin_remember(const float *episode_emb) {
    int slot;
    if (basin_count < BASIN_MEMORY) {
        slot = basin_count++;
    } else {
        slot = basin_cursor;
        basin_cursor = (basin_cursor + 1) % BASIN_MEMORY;
    }
    memcpy(basin_memory[slot], episode_emb, EMBED_DIM * sizeof(float));
    normalize(basin_memory[slot], EMBED_DIM);
    basin_uses[slot]++;
}


static float intent_progress_for(const int *ctx, int ctx_n, int candidate,
                                 const float *intent) {
    float cemb[EMBED_DIM];
    float next_discourse[EMBED_DIM];
    context_embedding(ctx, ctx_n, cemb);

    for (int d = 0; d < EMBED_DIM; ++d)
        next_discourse[d] =
            0.72f * cemb[d] + 0.28f * vocab[candidate].emb[d];
    normalize(next_discourse, EMBED_DIM);

    float before = cosine(cemb, intent, EMBED_DIM);
    float after = cosine(next_discourse, intent, EMBED_DIM);
    float progress = after - before;
    float progress_score = clampf(0.5f + 3.5f * progress, 0.0f, 1.0f);
    float absolute_score = 0.5f * (1.0f + after);
    return 0.78f * progress_score + 0.22f * absolute_score;
}

static float simulated_token_value(const int *ctx, int ctx_n, int candidate,
                                   const float *intent) {
    int prev = ctx[ctx_n - 1];
    uint32_t bigram = 0;
    int e = find_edge(prev, candidate);
    if (e >= 0) bigram = edges[e].source_count;

    uint32_t tri = ctx_n >= 2 ?
        source_trigram_count(ctx[ctx_n - 2], prev, candidate) : 0;

    float evidence =
        1.0f - expf(-((float)bigram + 2.5f * (float)tri) * 0.25f);
    float direction = directional_compatibility(prev, candidate);
    float intent_score = intent_progress_for(ctx, ctx_n, candidate, intent);
    float cycle = cycle_freshness(ctx, ctx_n, candidate);
    float semantic_cycle =
        semantic_cycle_freshness(ctx, ctx_n, candidate);
    float market = 0.5f + 0.5f * learned_relation_score(prev, candidate);

    return 0.29f * evidence +
           0.18f * direction +
           0.24f * intent_score +
           0.10f * cycle +
           0.09f * semantic_cycle +
           0.10f * market;
}

static int simulated_policy_next(const int *ctx, int ctx_n,
                                 const float *intent) {
    int prev = ctx[ctx_n - 1];
    int best = -1;
    float best_value = -1e30f;

    if (ctx_n >= 2) {
        int a = ctx[ctx_n - 2];
        unsigned bucket = trigram_bucket(a, prev);
        for (int t = first_trigram[bucket]; t >= 0;
             t = trigrams[t].next_bucket) {
            if ((int)trigrams[t].a != a ||
                (int)trigrams[t].b != prev)
                continue;
            int tok = (int)trigrams[t].c;
            float value = simulated_token_value(ctx, ctx_n, tok, intent);
            if (value > best_value) {
                best_value = value;
                best = tok;
            }
        }
    }

    int top_n = top_successor_n[prev];
    for (int i = 0; i < top_n; ++i) {
        int tok = top_successors[prev][i];
        if (tok < 0) continue;
        float value = simulated_token_value(ctx, ctx_n, tok, intent);
        if (value > best_value) {
            best_value = value;
            best = tok;
        }
    }

    if (best < 0)
        best = oracle_next_context(ctx, ctx_n);
    return best;
}

static float counterfactual_rollout_score(const int *ctx, int ctx_n,
                                          int candidate,
                                          const float *intent,
                                          float current_intent_debt) {
    int sim_ctx[CONTEXT + 5];
    int keep = ctx_n < CONTEXT ? ctx_n : CONTEXT;
    memcpy(sim_ctx, &ctx[ctx_n - keep], keep * sizeof(int));
    int sim_n = keep;

    float sim_intent[EMBED_DIM];
    memcpy(sim_intent, intent, sizeof(sim_intent));
    float sim_debt = current_intent_debt;

    float total = simulated_token_value(sim_ctx, sim_n, candidate, sim_intent);
    float worst_cycle = cycle_freshness(sim_ctx, sim_n, candidate);
    sim_ctx[sim_n++] = candidate;
    intent_update(sim_intent, &sim_debt, candidate);

    const int horizon = 3;
    for (int step = 0; step < horizon; ++step) {
        int current_n = sim_n < CONTEXT ? sim_n : CONTEXT;
        int *current = &sim_ctx[sim_n - current_n];
        int next = simulated_policy_next(current, current_n, sim_intent);

        float value = simulated_token_value(
            current, current_n, next, sim_intent);
        float cyc = cycle_freshness(current, current_n, next);
        if (cyc < worst_cycle) worst_cycle = cyc;

        total += value;
        sim_ctx[sim_n++] = next;
        intent_update(sim_intent, &sim_debt, next);
    }

    float mean = total / (float)(horizon + 1);
    return mean * (0.55f + 0.45f * worst_cycle) /
           (1.0f + 0.15f * sim_debt);
}

static void prior_score(const int *ctx, int ctx_n, int candidate, int oracle,
                        const float *intent, float out[SCORE_DIM]) {
    int prev = ctx[ctx_n - 1];
    int e = find_edge(prev, candidate);
    uint32_t src = e >= 0 ? edges[e].source_count : 0;
    uint32_t tri = ctx_n >= 2 ?
        source_trigram_count(ctx[ctx_n - 2], prev, candidate) : 0;

    float cemb[EMBED_DIM];
    context_embedding(ctx, ctx_n, cemb);

    int content = token_is_content_id(candidate);
    float local_evidence = (float)src + 2.5f * (float)tri;
    out[0] = 1.0f - expf(-local_evidence * 0.25f);
    out[1] = 0.5f + 0.5f * learned_relation_score(prev, candidate);
    out[2] = candidate == oracle ? 1.0f :
             0.5f * (1.0f + cosine(vocab[candidate].emb,
                                   vocab[oracle].emb, EMBED_DIM));
    float semantic_field =
        0.5f * (1.0f + cosine(cemb, vocab[candidate].emb, EMBED_DIM));
    float role_order = directional_compatibility(prev, candidate);
    out[3] = 0.58f * semantic_field + 0.42f * role_order;

    out[4] = intent_progress_for(ctx, ctx_n, candidate, intent);

    if (!content) {
        out[1] *= 0.20f;
        out[3] *= 0.15f;
        out[4] *= 0.10f;
    }

    float freq = (float)vocab[candidate].count / (float)(corpus_n + 1);
    float lexical_novelty = clampf(1.0f - 20.0f * freq, 0.0f, 1.0f);
    float phrase_novelty = phrase_freshness(ctx, ctx_n, candidate);
    float basin_novelty = semantic_basin_freshness(vocab[candidate].emb);
    float cycle = cycle_freshness(ctx, ctx_n, candidate);
    float semantic_cycle =
        semantic_cycle_freshness(ctx, ctx_n, candidate);
    out[5] = lexical_novelty * phrase_novelty * basin_novelty *
             (0.65f + 0.35f * cycle) *
             (0.55f + 0.45f * semantic_cycle);
    if (!content) out[5] *= 0.05f;

    int rollout_ctx[CONTEXT + 1];
    int rollout_n = ctx_n < CONTEXT ? ctx_n : CONTEXT;
    memcpy(rollout_ctx, &ctx[ctx_n - rollout_n],
           rollout_n * sizeof(int));
    rollout_ctx[rollout_n++] = candidate;
    int next = oracle_next_context(rollout_ctx, rollout_n);
    float mirror_future =
        0.5f * (1.0f + cosine(vocab[next].emb,
                              vocab[oracle].emb, EMBED_DIM));
    out[6] = mirror_future * (0.5f + 0.5f * basin_novelty);

    int rep = repeated_recently(ctx, ctx_n, candidate);
    out[7] = (1.0f / (1.0f + (float)rep)) *
             phrase_novelty * cycle * semantic_cycle;
    if (!content) out[7] *= 0.10f;
}

static int vector_prefer(const float a[SCORE_DIM], const float b[SCORE_DIM]) {
    int a_better = 0, b_better = 0;
    for (int d = 0; d < SCORE_DIM; ++d) {
        if (a[d] > b[d] + 0.025f) a_better = 1;
        if (b[d] > a[d] + 0.025f) b_better = 1;
    }
    if (a_better && !b_better) return 1;
    if (b_better && !a_better) return 0;

    /* Lexicographic coherence priorities, not one stored scalar loss. */
    static const int order[SCORE_DIM] = {0, 3, 4, 1, 2, 7, 6, 5};
    for (int k = 0; k < SCORE_DIM; ++k) {
        int d = order[k];
        if (a[d] > b[d] + 0.015f) return 1;
        if (b[d] > a[d] + 0.015f) return 0;
    }
    return 0;
}


static float screening_utility(const float score[SCORE_DIM]) {
    return 0.16f * score[0] +
           0.10f * score[1] +
           0.11f * score[2] +
           0.17f * score[3] +
           0.16f * score[4] +
           0.08f * score[5] +
           0.14f * score[6] +
           0.08f * score[7];
}

static int choose_candidate(const int *ctx, int ctx_n, int oracle, int truth,
                            const float *intent, float intent_debt,
                            ScoreVector *observed_out,
                            float predicted_out[SCORE_DIM]) {
    int candidates[CANDIDATES];
    int n = 0;
    int prev = ctx[ctx_n - 1];

    if (ctx_n >= 2) {
        int a = ctx[ctx_n - 2];
        unsigned bucket = trigram_bucket(a, prev);
        for (int t = first_trigram[bucket];
             t >= 0 && n < CANDIDATES - 10;
             t = trigrams[t].next_bucket) {
            if ((int)trigrams[t].a == a &&
                (int)trigrams[t].b == prev)
                candidates[n++] = (int)trigrams[t].c;
        }
    }

    int punct_added = 0;
    int top_n = top_successor_n[prev];
    for (int i = 0; i < top_n && n < CANDIDATES - 6; ++i) {
        int tok = top_successors[prev][i];
        if (tok < 0) continue;
        if (!token_is_content_id(tok)) {
            if (punct_added >= 3 || !token_is_content_id(prev)) continue;
            punct_added++;
        }
        candidates[n++] = tok;
    }

    if (n < CANDIDATES &&
        (token_is_content_id(oracle) || token_is_content_id(prev)))
        candidates[n++] = oracle;

    int best_exp[3] = {-1, -1, -1};
    float best_val[3] = {-1e9f, -1e9f, -1e9f};
    int experience_scanned = 0;
    for (int e = first_edge[prev];
         e >= 0 && experience_scanned < 128;
         e = edges[e].next_from) {
        experience_scanned++;
        float v = learned_relation_score(prev, (int)edges[e].to);
        for (int k = 0; k < 3; ++k) {
            if (v > best_val[k]) {
                for (int q = 2; q > k; --q) {
                    best_val[q] = best_val[q - 1];
                    best_exp[q] = best_exp[q - 1];
                }
                best_val[k] = v;
                best_exp[k] = (int)edges[e].to;
                break;
            }
        }
    }
    for (int k = 0; k < 3 && n < CANDIDATES - 2; ++k)
        if (best_exp[k] >= 0) candidates[n++] = best_exp[k];

    while (n < CANDIDATES) {
        int tok = (int)(rng_u64() % (uint64_t)vocab_size);
        if (token_is_content_id(tok))
            candidates[n++] = tok;
    }

    /* Deduplicate the screening pool. */
    int unique_n = 0;
    for (int i = 0; i < n; ++i) {
        int duplicate = 0;
        for (int j = 0; j < unique_n; ++j)
            if (candidates[j] == candidates[i]) {
                duplicate = 1;
                break;
            }
        if (!duplicate)
            candidates[unique_n++] = candidates[i];
    }
    n = unique_n;

    uint32_t min_phrase_use = UINT32_MAX;
    for (int i = 0; i < n; ++i) {
        if (!token_is_content_id(candidates[i])) continue;
        uint32_t uses = phrase_use_count(ctx, ctx_n, candidates[i]);
        if (uses < min_phrase_use) min_phrase_use = uses;
    }
    if (min_phrase_use == UINT32_MAX) min_phrase_use = 0;

    float ctx_emb[EMBED_DIM];
    context_embedding(ctx, ctx_n, ctx_emb);
    float mlp_gate =
        clampf((float)episode_count / 2500.0f, 0.0f, 0.65f);

    enum { FINALISTS = 4 };
    int final_tok[FINALISTS];
    float final_utility[FINALISTS];
    float final_mlp[FINALISTS][SCORE_DIM];
    float final_prior[FINALISTS][SCORE_DIM];
    float final_mix[FINALISTS][SCORE_DIM];
    int final_n = 0;

    for (int i = 0; i < n; ++i) {
        int cand = candidates[i];
        uint32_t phrase_uses = phrase_use_count(ctx, ctx_n, cand);

        if (token_is_content_id(cand) &&
            phrase_uses > min_phrase_use + 1 &&
            phrase_uses >= 3)
            continue;

        float prior[SCORE_DIM], mlp[SCORE_DIM], mix[SCORE_DIM];
        prior_score(ctx, ctx_n, cand, oracle, intent, prior);
        core_predict_recursive(ctx_emb, intent, intent_debt,
                               prev, cand, oracle, prior, mlp, 0);

        for (int d = 0; d < SCORE_DIM; ++d) {
            float m = 0.5f * (mlp[d] + 1.0f);
            mix[d] = (1.0f - mlp_gate) * prior[d] + mlp_gate * m;
        }

        float utility =
            screening_utility(mix) +
            0.14f * clampf(intent_debt, 0.0f, 1.5f) * mix[4];
        if (!token_is_content_id(cand) && prior[0] < 0.82f)
            utility -= 0.20f;

        int slot = final_n;
        if (slot < FINALISTS) {
            final_n++;
        } else if (utility <= final_utility[FINALISTS - 1]) {
            continue;
        } else {
            slot = FINALISTS - 1;
        }

        while (slot > 0 && utility > final_utility[slot - 1]) {
            if (slot < FINALISTS) {
                final_tok[slot] = final_tok[slot - 1];
                final_utility[slot] = final_utility[slot - 1];
                memcpy(final_mlp[slot], final_mlp[slot - 1],
                       sizeof(final_mlp[slot]));
                memcpy(final_prior[slot], final_prior[slot - 1],
                       sizeof(final_prior[slot]));
                memcpy(final_mix[slot], final_mix[slot - 1],
                       sizeof(final_mix[slot]));
            }
            slot--;
        }

        final_tok[slot] = cand;
        final_utility[slot] = utility;
        memcpy(final_mlp[slot], mlp, sizeof(mlp));
        memcpy(final_prior[slot], prior, sizeof(prior));
        memcpy(final_mix[slot], mix, sizeof(mix));
    }

    if (final_n == 0) {
        int fallback = oracle;
        float prior[SCORE_DIM], mlp[SCORE_DIM];
        prior_score(ctx, ctx_n, fallback, oracle, intent, prior);
        core_predict_recursive(ctx_emb, intent, intent_debt,
                               prev, fallback, oracle, prior, mlp, 0);
        *observed_out =
            observe_score(ctx, ctx_n, fallback, oracle, truth,
                          intent, intent_debt);
        memcpy(predicted_out, mlp, sizeof(mlp));
        return fallback;
    }

    /*
     * Only finalists receive expensive imagination. This is selective
     * test-time compute rather than exhaustive fantasy.
     */
    int best_idx = 0;
    float best_deep[SCORE_DIM];
    memcpy(best_deep, final_mix[0], sizeof(best_deep));
    float imagined =
        counterfactual_rollout_score(ctx, ctx_n, final_tok[0], intent, intent_debt);
    best_deep[6] = 0.58f * best_deep[6] + 0.42f * imagined;

    for (int i = 1; i < final_n; ++i) {
        float deep[SCORE_DIM];
        memcpy(deep, final_mix[i], sizeof(deep));
        float future =
            counterfactual_rollout_score(ctx, ctx_n, final_tok[i], intent, intent_debt);
        deep[6] = 0.58f * deep[6] + 0.42f * future;

        if (vector_prefer(deep, best_deep) ||
            (!vector_prefer(best_deep, deep) &&
             screening_utility(deep) > screening_utility(best_deep))) {
            best_idx = i;
            memcpy(best_deep, deep, sizeof(best_deep));
        }
    }

    int best = final_tok[best_idx];

    /* Prefer a viable content finalist over weak punctuation. */
    if (!token_is_content_id(best)) {
        for (int i = 0; i < final_n; ++i) {
            if (token_is_content_id(final_tok[i]) &&
                final_mix[i][0] > 0.30f) {
                best_idx = i;
                best = final_tok[i];
                break;
            }
        }
    }

    *observed_out =
        observe_score(ctx, ctx_n, best, oracle, truth,
                      intent, intent_debt);
    memcpy(predicted_out, final_mlp[best_idx], SCORE_DIM * sizeof(float));
    return best;
}

static void learn_local(const int *ctx, int ctx_n, int chosen, int oracle,
                        const float *intent, float intent_debt,
                        ScoreVector observed, const float predicted[SCORE_DIM]) {
    float target[SCORE_DIM];
    score_to_array(observed, target);

    float ctx_emb[EMBED_DIM];
    context_embedding(ctx, ctx_n, ctx_emb);

    /* Recreate hidden activation and locally update only the readout.
       This is reward-modulated Hebbian learning, not backprop. */
    float dummy[SCORE_DIM];
    float local_prior[SCORE_DIM];
    int prev_for_depth = ctx[ctx_n - 1];
    prior_score(ctx, ctx_n, chosen, oracle, intent, local_prior);
    core_predict_recursive(ctx_emb, intent, intent_debt,
                           prev_for_depth, chosen, oracle,
                           local_prior, dummy, 1);

    float rate = 0.025f;
    float global_error = 0.0f;
    float global_surprise = 0.0f;
    for (int o = 0; o < SCORE_DIM; ++o) {
        float error = target[o] - (0.5f * (predicted[o] + 1.0f));
        global_error += error;
        global_surprise += fabsf(error);
        for (int h = 0; h < HIDDEN_DIM; ++h)
            core.who[o][h] += rate * error * core.hidden[h];
    }
    global_error /= (float)SCORE_DIM;
    global_surprise /= (float)SCORE_DIM;

    /*
     * The recurrent dynamics themselves learn locally. This is not
     * backpropagation: the current prophecy error modulates Hebbian
     * co-activation between the visible input and the active hidden state.
     */
    float local_input[EMBED_DIM * 4 + STATE_DIM];
    int input_n = 0;
    for (int d = 0; d < EMBED_DIM; ++d) local_input[input_n++] = ctx_emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) local_input[input_n++] = vocab[chosen].emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) local_input[input_n++] = vocab[oracle].emb[d];
    for (int d = 0; d < EMBED_DIM; ++d) local_input[input_n++] = intent[d];
    for (int d = 0; d < STATE_DIM; ++d) local_input[input_n++] = core.state[d];

    float surprise_gate =
        clampf((global_surprise - 0.04f) / 0.24f, 0.0f, 1.0f);
    float modulation =
        clampf(global_error, -0.20f, 0.20f) * surprise_gate;
    float input_rate = 0.00035f;
    float recurrent_rate = 0.00008f;

    for (int h = 0; h < HIDDEN_DIM; ++h) {
        for (int i = 0; i < input_n; ++i) {
            core.wxh[h][i] =
                0.999999f * core.wxh[h][i] +
                input_rate * modulation * core.hidden[h] * local_input[i];
        }
        for (int j = 0; j < HIDDEN_DIM; ++j) {
            core.whh[h][j] =
                0.9999995f * core.whh[h][j] +
                recurrent_rate * modulation * core.hidden[h] * core.hidden[j];
        }
    }

    /*
     * Prophecy / destiny market.
     *
     * predicted[] is prophecy before the move.
     * target[] is destiny after the move.
     * The transition is re-priced by their changing difference.
     * No success becomes a permanent constant reward.
     */
    int prev = ctx[ctx_n - 1];
    int e = get_edge(prev, chosen, 1);
    if (e >= 0) {
        float debt_now = 0.0f;
        float signed_surprise = 0.0f;

        for (int o = 0; o < SCORE_DIM; ++o) {
            float prophecy = 0.5f * (predicted[o] + 1.0f);
            float delta = target[o] - prophecy;
            debt_now += fabsf(delta);
            signed_surprise += delta;
        }
        debt_now /= (float)SCORE_DIM;
        signed_surprise /= (float)SCORE_DIM;

        float previous_quote = edges[e].quote;

        /* Fast quote, slow debt, and volatility of revaluation. */
        edges[e].momentum =
            0.82f * edges[e].momentum + 0.18f * signed_surprise;
        edges[e].quote =
            0.90f * edges[e].quote + 0.10f * signed_surprise;
        edges[e].debt =
            0.94f * edges[e].debt + 0.06f * debt_now;
        edges[e].volatility =
            0.92f * edges[e].volatility +
            0.08f * fabsf(edges[e].quote - previous_quote);
        edges[e].last_mark_episode = episode_count;

        /*
         * Slow evidence also floats: it decays continuously and receives
         * only the current marked value, not a fixed medal.
         */
        edges[e].support *= 0.9985f;
        edges[e].opposition *= 0.9985f;

        float marked = signed_surprise - 0.70f * debt_now;
        if (marked >= 0.0f) {
            edges[e].support += marked;
            edges[e].positive_uses++;
        } else {
            edges[e].opposition += -marked;
            edges[e].negative_uses++;
        }

        /* Excessive exploitation itself raises debt and volatility. */
        if (edges[e].positive_uses > 16) {
            float crowding = (float)(edges[e].positive_uses - 16) * 0.002f;
            edges[e].debt += crowding;
            edges[e].volatility += 0.5f * crowding;
        }
    }

    /* Experience embedding: successful relations attract; failed ones
       receive a context-specific repulsive trace. */
    float valence =
        (observed.syntax_local +
         observed.source_grounding +
         observed.semantic_continuity) / 3.0f;
    float direction = valence >= 0.5f ? 1.0f : -0.35f;
    float erate = 0.008f * direction;

    for (int d = 0; d < EMBED_DIM; ++d) {
        vocab[chosen].emb[d] += erate * ctx_emb[d];

        /* The chosen token learns what tends to precede it. */
        vocab[chosen].left_emb[d] += erate * vocab[prev].emb[d];

        /* The previous token learns what tends to follow it. */
        vocab[prev].right_emb[d] += erate * vocab[chosen].emb[d];
    }
    normalize(vocab[chosen].emb, EMBED_DIM);
    normalize(vocab[chosen].left_emb, EMBED_DIM);
    normalize(vocab[prev].right_emb, EMBED_DIM);
    phrase_remember(ctx, ctx_n, chosen);
}

static void token_print(FILE *f, int id, int *at_line_start) {
    const char *t = vocab[id].text;

    if (strcmp(t, "\n") == 0) {
        fputc('\n', f);
        *at_line_start = 1;
        return;
    }

    int punct = strlen(t) == 1 && strchr(".,;:!?)]", t[0]);
    int opening = strlen(t) == 1 && strchr("([", t[0]);

    if (!*at_line_start && !punct && !opening) fputc(' ', f);
    fputs(t, f);
    *at_line_start = 0;
}

static void sequence_to_text(const int *seq, int n, char *out, size_t cap) {
    size_t pos = 0;
    int line_start = 1;

    for (int i = 0; i < n && pos + 2 < cap; ++i) {
        const char *t = vocab[seq[i]].text;
        if (strcmp(t, "\n") == 0) {
            out[pos++] = ' ';
            line_start = 1;
            continue;
        }

        int punct = strlen(t) == 1 && strchr(".,;:!?)]", t[0]);
        int opening = strlen(t) == 1 && strchr("([", t[0]);
        if (!line_start && !punct && !opening && pos + 1 < cap)
            out[pos++] = ' ';

        size_t len = strlen(t);
        if (pos + len >= cap) len = cap - pos - 1;
        memcpy(out + pos, t, len);
        pos += len;
        line_start = 0;
    }
    out[pos] = '\0';
}

static void history_append(uint64_t ep, int source_pos,
                           const int *ctx, int ctx_n,
                           const int *oracle, const int *attempt, int n,
                           const ScoreVector *scores,
                           float avg_recursive_depth) {
    FILE *f = fopen("netta.history.tsv", "ab");
    if (!f) return;

    char ctx_text[512], oracle_text[512], attempt_text[512];
    sequence_to_text(ctx, ctx_n, ctx_text, sizeof(ctx_text));
    sequence_to_text(oracle, n, oracle_text, sizeof(oracle_text));
    sequence_to_text(attempt, n, attempt_text, sizeof(attempt_text));

    float avg[SCORE_DIM] = {0};
    for (int i = 0; i < n; ++i) {
        float x[SCORE_DIM];
        score_to_array(scores[i], x);
        for (int d = 0; d < SCORE_DIM; ++d) avg[d] += x[d];
    }
    for (int d = 0; d < SCORE_DIM; ++d) avg[d] /= (float)n;

    fprintf(f,
        "%llu\t%d\t%s\t%s\t%s\t"
        "%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\n",
        (unsigned long long)ep, source_pos,
        ctx_text, oracle_text, attempt_text,
        avg[0], avg[1], avg[2], avg[3], avg[4], avg[5], avg[6], avg[7],
        avg_recursive_depth);
    fclose(f);
}

static void save_state(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "netta: cannot save state: %s\n", strerror(errno));
        return;
    }

    StateHeader h;
    h.magic = STATE_MAGIC;
    h.version = STATE_VERSION;
    h.vocab_size = (uint32_t)vocab_size;
    h.edge_count = (uint32_t)edge_count;
    h.episodes = episode_count;
    h.core = core;
    h.basin_count = basin_count;
    h.basin_cursor = basin_cursor;

    fwrite(&h, sizeof(h), 1, f);
    fwrite(vocab, sizeof(Token), (size_t)vocab_size, f);
    fwrite(edges, sizeof(Edge), (size_t)edge_count, f);
    fwrite(phrase_counts, sizeof(uint32_t), PHRASE_TABLE, f);
    fwrite(basin_memory, sizeof(float), BASIN_MEMORY * EMBED_DIM, f);
    fwrite(basin_uses, sizeof(uint32_t), BASIN_MEMORY, f);
    fclose(f);
}

static int load_state(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    StateHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1 ||
        h.magic != STATE_MAGIC ||
        h.version != STATE_VERSION ||
        h.vocab_size != (uint32_t)vocab_size) {
        fclose(f);
        return 0;
    }

    Token saved[MAX_VOCAB];
    if (fread(saved, sizeof(Token), h.vocab_size, f) != h.vocab_size) {
        fclose(f);
        return 0;
    }

    /* Refuse state if vocabulary identity changed. */
    for (uint32_t i = 0; i < h.vocab_size; ++i) {
        if (strncmp(saved[i].text, vocab[i].text, MAX_TOKEN_LEN) != 0) {
            fclose(f);
            return 0;
        }
    }

    for (uint32_t i = 0; i < h.vocab_size; ++i)
        memcpy(vocab[i].emb, saved[i].emb, sizeof(vocab[i].emb));

    if (h.edge_count > MAX_EDGES ||
        fread(edges, sizeof(Edge), h.edge_count, f) != h.edge_count) {
        fclose(f);
        return 0;
    }

    edge_count = (int)h.edge_count;
    episode_count = h.episodes;
    core = h.core;
    basin_count = h.basin_count;
    basin_cursor = h.basin_cursor;

    if (fread(phrase_counts, sizeof(uint32_t), PHRASE_TABLE, f) != PHRASE_TABLE ||
        fread(basin_memory, sizeof(float), BASIN_MEMORY * EMBED_DIM, f) !=
            BASIN_MEMORY * EMBED_DIM ||
        fread(basin_uses, sizeof(uint32_t), BASIN_MEMORY, f) != BASIN_MEMORY) {
        fclose(f);
        return 0;
    }

    memset(first_edge, 0xFF, sizeof(first_edge));
    for (int e = edge_count - 1; e >= 0; --e) {
        int from = (int)edges[e].from;
        edges[e].next_from = first_edge[from];
        first_edge[from] = e;
    }

    fclose(f);
    return 1;
}

static void print_score_average(const ScoreVector *scores, int n) {
    float a[SCORE_DIM] = {0};
    for (int i = 0; i < n; ++i) {
        float x[SCORE_DIM];
        score_to_array(scores[i], x);
        for (int d = 0; d < SCORE_DIM; ++d) a[d] += x[d];
    }
    for (int d = 0; d < SCORE_DIM; ++d) a[d] /= (float)n;

    printf("  coherence: local=%.3f source=%.3f oracle=%.3f semantic=%.3f "
           "intent=%.3f novelty=%.3f rollout=%.3f anti-repeat=%.3f\n",
           a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7]);
}


static float trajectory_value(ScoreVector s) {
    return 0.15f * s.syntax_local +
           0.19f * s.source_grounding +
           0.14f * s.oracle_parity +
           0.18f * s.semantic_continuity +
           0.16f * s.intent_fidelity +
           0.05f * s.novelty +
           0.09f * s.rollout_stability +
           0.04f * s.anti_repetition;
}

static void trajectory_revalue(TrajectoryStep *trace, int n) {
    const float lambda = 0.72f;

    for (int i = 0; i < n; ++i) {
        float future = 0.0f;
        float norm = 0.0f;
        float eligibility = 1.0f;

        for (int j = i; j < n; ++j) {
            future += eligibility * trajectory_value(trace[j].immediate);
            norm += eligibility;
            eligibility *= lambda;
        }
        if (norm > 0.0f) future /= norm;

        float immediate = trajectory_value(trace[i].immediate);
        float delayed_delta = future - immediate;

        int e = get_edge(trace[i].prev, trace[i].chosen, 1);
        if (e < 0) continue;

        float old_quote = edges[e].quote;
        edges[e].momentum =
            0.86f * edges[e].momentum + 0.14f * delayed_delta;
        edges[e].quote =
            0.94f * edges[e].quote + 0.06f * delayed_delta;

        float delayed_debt = fmaxf(0.0f, -delayed_delta);
        float delayed_credit = fmaxf(0.0f, delayed_delta);

        edges[e].debt =
            0.96f * edges[e].debt +
            0.04f * (edges[e].debt +
                     delayed_debt - 0.35f * delayed_credit);
        edges[e].debt = clampf(edges[e].debt, 0.0f, 4.0f);

        edges[e].volatility =
            0.95f * edges[e].volatility +
            0.05f * fabsf(edges[e].quote - old_quote);

        if (delayed_delta >= 0.0f)
            edges[e].support += 0.20f * delayed_delta;
        else
            edges[e].opposition += 0.25f * (-delayed_delta);

        edges[e].last_mark_episode = episode_count;
    }
}

static void run_episode(int verbose) {
    int max_start = corpus_n - CONTEXT - ROLLOUT - 1;
    int source_pos = (int)(rng_u64() % (uint64_t)max_start);

    int seq[CONTEXT + ROLLOUT];
    int oracle[ROLLOUT];
    int attempt[ROLLOUT];
    ScoreVector scores[ROLLOUT];
    TrajectoryStep trace[ROLLOUT];
    float episode_emb[EMBED_DIM] = {0};

    memcpy(seq, &corpus[source_pos], CONTEXT * sizeof(int));
    int seq_n = CONTEXT;

    float intent[EMBED_DIM];
    float intent_debt = 1.0f;
    intent_from_context(seq, CONTEXT, intent);

    int oracle_seq[CONTEXT + ROLLOUT];
    memcpy(oracle_seq, seq, CONTEXT * sizeof(int));
    int oracle_seq_n = CONTEXT;

    uint64_t depth_start = recursive_depth_total;
    uint64_t calls_start = recursive_call_total;

    for (int step = 0; step < ROLLOUT; ++step) {
        int truth = corpus[source_pos + CONTEXT + step];
        int oracle_ctx_n = oracle_seq_n < CONTEXT ? oracle_seq_n : CONTEXT;
        int oracle_tok = oracle_next_context(
            &oracle_seq[oracle_seq_n - oracle_ctx_n], oracle_ctx_n);
        oracle[step] = oracle_tok;
        oracle_seq[oracle_seq_n++] = oracle_tok;

        float pred[SCORE_DIM];
        int ctx_n = seq_n < CONTEXT ? seq_n : CONTEXT;
        int *ctx = &seq[seq_n - ctx_n];

        int chosen = choose_candidate(ctx, ctx_n, oracle_tok, truth,
                                      intent, intent_debt,
                                      &scores[step], pred);
        attempt[step] = chosen;

        trace[step].prev = ctx[ctx_n - 1];
        trace[step].chosen = chosen;
        trace[step].oracle = oracle_tok;
        trace[step].immediate = scores[step];
        memcpy(trace[step].predicted, pred, SCORE_DIM * sizeof(float));

        seq[seq_n++] = chosen;

        learn_local(ctx, ctx_n, chosen, oracle_tok,
                    intent, intent_debt, scores[step], pred);
        intent_update(intent, &intent_debt, chosen);

        for (int d = 0; d < EMBED_DIM; ++d)
            episode_emb[d] += vocab[chosen].emb[d];

        if (step >= 3) {
            float local_basin[EMBED_DIM] = {0};
            for (int k = step - 3; k <= step; ++k)
                for (int d = 0; d < EMBED_DIM; ++d)
                    local_basin[d] += vocab[attempt[k]].emb[d] * 0.25f;
            basin_remember(local_basin);
        }
    }

    trajectory_revalue(trace, ROLLOUT);

    for (int d = 0; d < EMBED_DIM; ++d)
        episode_emb[d] /= (float)ROLLOUT;
    basin_remember(episode_emb);

    episode_count++;
    uint64_t depth_used = recursive_depth_total - depth_start;
    uint64_t calls_used = recursive_call_total - calls_start;
    float avg_recursive_depth =
        calls_used ? (float)depth_used / (float)calls_used : 0.0f;

    history_append(episode_count, source_pos,
                   &corpus[source_pos], CONTEXT,
                   oracle, attempt, ROLLOUT, scores,
                   avg_recursive_depth);

    if (verbose) {
        int line = 1;
        printf("\n[episode %llu]\n  source context: ",
               (unsigned long long)episode_count);
        for (int i = 0; i < CONTEXT; ++i)
            token_print(stdout, corpus[source_pos + i], &line);

        line = 1;
        printf("\n  hidden truth:   ");
        for (int i = 0; i < ROLLOUT; ++i)
            token_print(stdout, corpus[source_pos + CONTEXT + i], &line);

        line = 1;
        printf("\n  postgpt mirror: ");
        for (int i = 0; i < ROLLOUT; ++i)
            token_print(stdout, oracle[i], &line);

        line = 1;
        printf("\n  netta attempt:  ");
        for (int i = 0; i < ROLLOUT; ++i)
            token_print(stdout, attempt[i], &line);
        printf("\n");
        print_score_average(scores, ROLLOUT);
        printf("  recursive depth: %.2f shared-block passes per evaluation\n",
               avg_recursive_depth);
    }
}

static void interactive_prompt(const char *text) {
    int prompt[CONTEXT];
    int n = 0;

    char copy[1024];
    snprintf(copy, sizeof(copy), "%s", text);

    char *save = NULL;
    for (char *p = strtok_r(copy, " \t\r\n", &save);
         p && n < CONTEXT;
         p = strtok_r(NULL, " \t\r\n", &save)) {
        for (char *q = p; *q; ++q)
            if ((unsigned char)*q < 128) *q = (char)tolower((unsigned char)*q);
        int id = token_id(p, 0);
        if (id >= 0) prompt[n++] = id;
    }

    if (n == 0) {
        fprintf(stderr, "netta: prompt contains no known tokens\n");
        return;
    }

    float intent[EMBED_DIM];
    float intent_debt = 1.0f;
    intent_from_context(prompt, n, intent);

    int generated[64];
    TrajectoryStep live_trace[ROLLOUT];
    int live_n = 0;
    memcpy(generated, prompt, n * sizeof(int));
    int gen_n = n;

    int oracle_seq[64];
    memcpy(oracle_seq, generated, gen_n * sizeof(int));
    int oracle_seq_n = gen_n;

    for (int step = 0; step < 32 && gen_n < 64; ++step) {
        int oracle_ctx_n = oracle_seq_n < CONTEXT ? oracle_seq_n : CONTEXT;
        int oracle = oracle_next_context(
            &oracle_seq[oracle_seq_n - oracle_ctx_n], oracle_ctx_n);
        oracle_seq[oracle_seq_n++] = oracle;

        int ctx_n = gen_n < CONTEXT ? gen_n : CONTEXT;
        ScoreVector score;
        float pred[SCORE_DIM];

        /* In interactive mode truth is unknown; the oracle is used only
           as a coherence reference, never copied as mandatory truth. */
        int *live_ctx = &generated[gen_n - ctx_n];
        int chosen = choose_candidate(live_ctx, ctx_n,
                                      oracle, oracle, intent, intent_debt,
                                      &score, pred);

        live_trace[live_n].prev = live_ctx[ctx_n - 1];
        live_trace[live_n].chosen = chosen;
        live_trace[live_n].oracle = oracle;
        live_trace[live_n].immediate = score;
        memcpy(live_trace[live_n].predicted, pred,
               SCORE_DIM * sizeof(float));
        live_n++;

        generated[gen_n++] = chosen;
        learn_local(live_ctx, ctx_n, chosen, oracle,
                    intent, intent_debt, score, pred);
        intent_update(intent, &intent_debt, chosen);

        if (live_n == ROLLOUT) {
            trajectory_revalue(live_trace, live_n);
            live_n = 0;
        }
    }

    if (live_n > 0)
        trajectory_revalue(live_trace, live_n);

    int line = 1;
    printf("\nnetta> ");
    for (int i = 0; i < gen_n; ++i)
        token_print(stdout, generated[i], &line);
    printf("\n");
}

int main(int argc, char **argv) {
    const char *corpus_path = argc > 1 ? argv[1] : "netta.txt";
    long steps = 1000;
    const char *prompt = NULL;
    int reset = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc)
            steps = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--prompt") == 0 && i + 1 < argc)
            prompt = argv[++i];
        else if (strcmp(argv[i], "--reset") == 0)
            reset = 1;
    }

    rng_state ^= (uint64_t)time(NULL);

    printf("NETTA — NETTA's Experiential Text Training Architecture\n");
    printf("sovereign recursive coherence game; no backpropagation\n\n");

    if (!load_corpus(corpus_path)) return 1;

    printf("corpus: %d tokens, %d vocabulary items\n", corpus_n, vocab_size);

    build_source_graph();
    core_init();

    if (!reset && load_state("netta.state")) {
        printf("state: resumed after %llu episodes\n",
               (unsigned long long)episode_count);
    } else {
        printf("state: new organism\n");
        FILE *f = fopen("netta.history.tsv", "wb");
        if (f) {
            fputs("episode\tsource_pos\tcontext\toracle\tnetta\t"
                  "local\tsource\toracle_parity\tsemantic\tintent\tnovelty\t"
                  "rollout\tanti_repetition\tavg_recursive_depth\n", f);
            fclose(f);
        }
    }

    if (prompt) {
        interactive_prompt(prompt);
        save_state("netta.state");
        return 0;
    }

    if (steps < 0) {
        /* Sovereign mode: live until interrupted. */
        for (;;) {
            int verbose = (episode_count < 10 || episode_count % 100 == 0);
            run_episode(verbose);
            if (episode_count % 100 == 0)
                save_state("netta.state");
        }
    }

    for (long i = 0; i < steps; ++i) {
        int verbose = (i < 5 || (i + 1) % 100 == 0);
        run_episode(verbose);
        if ((i + 1) % 100 == 0)
            save_state("netta.state");
    }

    save_state("netta.state");
    printf("\ncomplete: %llu lifetime episodes\n",
           (unsigned long long)episode_count);
    printf("memory: netta.state\nledger: netta.history.tsv\n");
    return 0;
}
