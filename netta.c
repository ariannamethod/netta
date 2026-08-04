/*
 * netta.c — NETTA's Empirical Topological Training Architecture
 *
 * A sovereign recursive language learner.
 * No backpropagation.  
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
 * The source corpus, oracle line, and Netta attempt are
 * kept separate. Negative experience is remembered but never promoted
 * into source truth.
 */

#define _POSIX_C_SOURCE 200809L

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MAX_VOCAB       16384
#define MAX_TOKEN_LEN   48
#define MAX_CORPUS      300000
#define MAX_EDGES       500000
#define MAX_TRIGRAMS    350000
#define TRI_BUCKETS     262144
#define EMBED_DIM       24
#define HIDDEN_DIM      32
#define STATE_DIM       24
#define SCORE_DIM       12
#define CONTEXT         16
#define ROLLOUT         8
#define CANDIDATES      32
#define PHRASE_TABLE    131072
#define BASIN_MEMORY    256
#define TOP_SUCCESSORS  12
#define PROPHECY_HORIZONS 3
#define FUTURE_TOPK      12
#define MAX_GLYPHS       128
#define GLYPH_TOPK       12
#define GLYPH_ACTION_TOPK 16
#define GLYPH_RANDOM_BUCKETS MAX_GLYPHS
#define GLYPH_SKETCH     256
#define GLYPH_NURSERY    32
#define GLYPH_SEED_HITS  4
#define GLYPH_LINK_MAX_AGE 64
#define CURRICULUM_REGION_SPAN 256
#define MAX_CURRICULUM_REGIONS ((MAX_CORPUS + CURRICULUM_REGION_SPAN - 1) / CURRICULUM_REGION_SPAN)
#define CURRICULUM_WARMUP_PASSES 2
#define CURRICULUM_UNIFORM_MILLIS 350
#define COHERENCE_PROBE_SEED 0x6E6574746150524FULL
#define REPLAY_CAPACITY  1024
#define DREAM_INTERVAL   64
#define NREM_DREAMS      12
#define REM_DREAMS       8
#define STATE_MAGIC     0x4E455454u /* NETT */
#define STATE_VERSION   41u

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

    /* Search-improved contextual policy, distilled without backprop. */
    float policy_quote;
    float policy_debt;
    float policy_momentum;
    float policy_volatility;
    uint32_t policy_visits;
    uint64_t policy_last_episode;

    int32_t next_bucket;
} TrigramEdge;

typedef struct {
    float syntax_local;
    float source_grounding;
    float oracle_parity;
    float semantic_continuity;
    float intent_fidelity;
    float search_policy;
    float causal_glyph;
    float prophecy_fulfillment;
    float world_state_stability;
    float novelty;
    float rollout_stability;
    float anti_repetition;
} ScoreVector;

typedef struct {
    int token[PROPHECY_HORIZONS][FUTURE_TOPK];
    float prob[PROPHECY_HORIZONS][FUTURE_TOPK];
    uint8_t n[PROPHECY_HORIZONS];
    float debt[PROPHECY_HORIZONS];
    float confidence[PROPHECY_HORIZONS];
    float paid;
    float overdue;
    uint32_t steps;
} ProphecyStack;

typedef struct {
    int prev;
    int chosen;
    int oracle;
    float predicted[SCORE_DIM];
    ScoreVector immediate;
    float world_debt_before;
    float world_debt_after;
    int glyph_id;
    float glyph_gain;
    float glyph_distance;
} TrajectoryStep;

typedef struct {
    TrajectoryStep steps[ROLLOUT];
    float intent_start[EMBED_DIM];
    float intent_end[EMBED_DIM];
    float priority;
    float surprise;
    float debt;
    float novelty;
    float world_debt_start;
    float world_debt_end;
    uint64_t episode;
    uint32_t replays;
    uint8_t valid;
} ReplayEpisode;


typedef struct {
    /* Predictive-state signature used for assignment. */
    int sig_token[PROPHECY_HORIZONS][GLYPH_TOPK];
    float sig_prob[PROPHECY_HORIZONS][GLYPH_TOPK];
    uint8_t sig_n[PROPHECY_HORIZONS];
    float sig_debt[PROPHECY_HORIZONS];
    float sig_conf[PROPHECY_HORIZONS];
    float sig_future_emb[PROPHECY_HORIZONS][EMBED_DIM];

    /* Destinies actually observed after contexts assigned to this glyph. */
    int future_token[PROPHECY_HORIZONS][GLYPH_TOPK];
    float future_weight[PROPHECY_HORIZONS][GLYPH_TOPK];
    uint8_t future_n[PROPHECY_HORIZONS];
    float future_total[PROPHECY_HORIZONS];
    float future_sketch[PROPHECY_HORIZONS][GLYPH_SKETCH];
    float future_emb_sum[PROPHECY_HORIZONS][EMBED_DIM];

    /* Search-improved action memory inside the abstract state. */
    int action_token[GLYPH_ACTION_TOPK];
    float action_quote[GLYPH_ACTION_TOPK];
    float action_debt[GLYPH_ACTION_TOPK];
    uint32_t action_visits[GLYPH_ACTION_TOPK];
    uint8_t action_n;

    float context_centroid[EMBED_DIM];
    float assignment_distance_ema;
    float predictive_gain_ema;
    float progress_ema;
    float topology_error_ema;
    float topology_utility_ema;
    uint32_t topology_wins;
    uint32_t uses;
    uint64_t born_episode;
    uint64_t last_seen;
    int16_t parent_a;
    int16_t parent_b;
    uint8_t generation;
    uint8_t active;
} CausalGlyph;

typedef struct {
    int sig_token[PROPHECY_HORIZONS][GLYPH_TOPK];
    float sig_prob[PROPHECY_HORIZONS][GLYPH_TOPK];
    uint8_t sig_n[PROPHECY_HORIZONS];
    float sig_debt[PROPHECY_HORIZONS];
    float sig_conf[PROPHECY_HORIZONS];
    float sig_future_emb[PROPHECY_HORIZONS][EMBED_DIM];
    float context_centroid[EMBED_DIM];
    uint32_t hits;
    uint64_t last_seen;
    uint8_t active;
} GlyphSeed;

typedef struct {
    float fast;
    float slow;
    float progress;
    float surprise;
    float debt;
    float priority;
    uint32_t visits;
    uint64_t last_seen;
} CurriculumRegion;

typedef struct {
    float readout_lr;
    float input_lr;
    float recurrent_lr;
    float surprise_threshold;
    float surprise_width;
    float error_clip;
    float input_decay;
    float recurrent_decay;
    float debt_modulation;
    float embedding_lr;
    float quote_rate;
    float debt_rate;
    float neighbor_gain;
} PlasticityRule;

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
    uint32_t trigram_count;
    uint64_t episodes;
    Core core;
    int32_t basin_count;
    int32_t basin_cursor;
    int32_t replay_count;
    int32_t replay_cursor;
    uint64_t dream_cycles;
    uint64_t nrem_replays;
    uint64_t rem_replays;
    uint64_t experiment_seed;
    uint64_t runtime_rng_state;
    uint32_t glyph_mode;
    uint32_t glyph_count;
    uint64_t glyph_births;
    uint64_t glyph_recycles;
    uint64_t glyph_merges;
    uint64_t glyph_seed_births;
    uint64_t glyph_seed_promotions;
    uint32_t curriculum_count;
    uint32_t curriculum_enabled;
    uint32_t neighbor_enabled;
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

/* Sparse corpus-derived future distributions: 1, 2-5, and 6-16 tokens. */
static int future_top_tokens[MAX_VOCAB][PROPHECY_HORIZONS][FUTURE_TOPK];
static float future_top_weight[MAX_VOCAB][PROPHECY_HORIZONS][FUTURE_TOPK];
static float future_top_prob[MAX_VOCAB][PROPHECY_HORIZONS][FUTURE_TOPK];
static uint8_t future_top_n[MAX_VOCAB][PROPHECY_HORIZONS];
static float future_conf[MAX_VOCAB][PROPHECY_HORIZONS];
static float global_future_weight[PROPHECY_HORIZONS][MAX_VOCAB];
static float global_future_total[PROPHECY_HORIZONS];

enum { GLYPH_OFF = 0, GLYPH_CAUSAL = 1, GLYPH_RANDOM = 2 };
static CausalGlyph causal_glyphs[MAX_GLYPHS];
static GlyphSeed glyph_nursery[GLYPH_NURSERY];
static uint16_t glyph_link_age[MAX_GLYPHS][MAX_GLYPHS];
static float global_glyph_sketch[PROPHECY_HORIZONS][GLYPH_SKETCH];
static int glyph_mode = GLYPH_CAUSAL;
static int glyph_count = 0;
static uint64_t glyph_births = 0;
static uint64_t glyph_recycles = 0;
static uint64_t glyph_merges = 0;
static uint64_t glyph_seed_births = 0;
static uint64_t glyph_seed_promotions = 0;
static float glyph_birth_threshold = 0.31f;
static int neighbor_enabled = 1;

static CurriculumRegion curriculum_regions[MAX_CURRICULUM_REGIONS];
static int curriculum_count = 0;
static int curriculum_enabled = 1;
static int curriculum_region_span = CURRICULUM_REGION_SPAN;
static int last_curriculum_region = -1;
static float last_curriculum_priority = 0.0f;
static float last_curriculum_progress = 0.0f;
static float last_episode_coherence = 0.0f;

static PlasticityRule baseline_plasticity_rule;
static int evaluation_mode = 0;

static Core core;
static uint64_t episode_count = 0;
static uint64_t rng_state = 0x9E3779B97F4A7C15ull;
static uint64_t experiment_seed = 42ull;
static int policy_enabled = 1;
/* Ablation controls for the two channels by which Netta's own moves reach
   the geometry of her world. Both on by default: this is a measurement. */
static int agent_embedding_enabled = 1;
static int dream_embedding_enabled = 1;
/*
 * Weight of the only counter that spans lives rather than steps. Four
 * per-structure fatigue organs failed to touch phrase culture; this one
 * term, raised from 0.042, cut it by 18.7% on 3/3 seeds. Repetition is a
 * property of a trajectory, so only a trajectory-level counter can see it.
 */
static float ngram_freshness_weight = 0.168f;
static int prophecy_stack_enabled = 1;
static int dreams_enabled = 1;
static uint64_t recursive_depth_total = 0;
static uint64_t recursive_call_total = 0;

static ReplayEpisode replay_buffer[REPLAY_CAPACITY];
static int replay_count = 0;
static int replay_cursor = 0;
static uint64_t dream_cycles = 0;
static uint64_t nrem_replays = 0;
static uint64_t rem_replays = 0;

/* Anti-cheat memory: generated phrase trajectories and recent semantic basins. */
static uint32_t phrase_counts[PHRASE_TABLE];
static uint32_t global_ngram_counts[PHRASE_TABLE];
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

static uint64_t mix64(uint64_t x) {
    x ^= x >> 30;
    x *= 0xbf58476d1ce4e5b9ULL;
    x ^= x >> 27;
    x *= 0x94d049bb133111ebULL;
    x ^= x >> 31;
    return x;
}

static float clampf(float x, float lo, float hi);

static void curriculum_init(int max_start) {
    int count =
        (max_start + curriculum_region_span - 1) /
        curriculum_region_span;
    if (count < 1) count = 1;
    if (count > MAX_CURRICULUM_REGIONS)
        count = MAX_CURRICULUM_REGIONS;

    if (curriculum_count == 0) {
        memset(curriculum_regions, 0, sizeof(curriculum_regions));
        for (int i = 0; i < count; ++i) {
            curriculum_regions[i].fast = 0.5f;
            curriculum_regions[i].slow = 0.5f;
            curriculum_regions[i].priority = 0.5f;
        }
    }
    curriculum_count = count;
}

static float curriculum_priority_for(int region) {
    if (region < 0 || region >= curriculum_count) return 0.0f;
    CurriculumRegion *r = &curriculum_regions[region];

    float raw_progress =
        0.65f * fabsf(r->fast - r->slow) +
        0.35f * fmaxf(0.0f, r->fast - r->slow);
    float progress =
        1.0f - expf(-raw_progress / 0.025f);
    float frontier =
        clampf(4.0f * r->fast * (1.0f - r->fast), 0.0f, 1.0f);
    float staleness =
        r->visits == 0 ? 1.0f :
        1.0f - expf(-(float)(episode_count - r->last_seen) / 192.0f);
    float novelty = 1.0f / sqrtf(1.0f + (float)r->visits);
    float surprise =
        1.0f - expf(-r->surprise / 0.10f);
    float learnable_debt =
        expf(-2.2f * fabsf(r->debt - 0.55f));

    return clampf(
        0.42f * progress +
        0.18f * frontier +
        0.14f * staleness +
        0.08f * novelty +
        0.10f * surprise +
        0.08f * learnable_debt,
        0.0f, 1.0f);
}

static uint64_t curriculum_hash(uint64_t episode, uint64_t stream) {
    return mix64(experiment_seed ^
                 0xC6BC279692B5CC83ULL ^
                 (episode + 1) * 0x9E3779B97F4A7C15ULL ^
                 stream * 0xD1B54A32D192ED03ULL);
}

static int curriculum_source_position(uint64_t episode, int max_start) {
    if (curriculum_count <= 0)
        curriculum_init(max_start);

    uint64_t warmup =
        (uint64_t)CURRICULUM_WARMUP_PASSES *
        (uint64_t)curriculum_count;
    int in_warmup = episode < warmup;
    int use_uniform =
        !curriculum_enabled ||
        in_warmup ||
        (curriculum_hash(episode, 1) % 1000ULL) <
            CURRICULUM_UNIFORM_MILLIS;

    int region;
    if (in_warmup) {
        /*
         * Two complete deterministic surveys before frontier selection.
         * An offset changes the starting neighbourhood by seed, while the
         * modulo walk guarantees every region is visited equally often.
         */
        uint64_t offset =
            curriculum_hash(0, 0xC011AB1EULL) %
            (uint64_t)curriculum_count;
        region = (int)((episode + offset) %
                       (uint64_t)curriculum_count);
    } else if (use_uniform) {
        region = (int)(curriculum_hash(episode, 2) %
                       (uint64_t)curriculum_count);
    } else {
        /*
         * Prioritized replay without winner-take-all collapse.
         * Regions are sampled in proportion to softened learning-frontier
         * mass, while the permanent uniform branch preserves world coverage.
         */
        double total = 0.0;
        for (int i = 0; i < curriculum_count; ++i) {
            float p = curriculum_priority_for(i);
            total += 0.02 + (double)p * sqrt((double)p + 1e-12);
        }

        double u =
            (double)(curriculum_hash(episode, 0xF20AULL) >> 11) *
            (1.0 / 9007199254740992.0);
        double target = u * total;
        double cumulative = 0.0;
        region = curriculum_count - 1;
        for (int i = 0; i < curriculum_count; ++i) {
            float p = curriculum_priority_for(i);
            cumulative +=
                0.02 + (double)p * sqrt((double)p + 1e-12);
            if (cumulative >= target) {
                region = i;
                break;
            }
        }
    }

    int lo = region * curriculum_region_span;
    int hi = lo + curriculum_region_span;
    if (hi > max_start) hi = max_start;
    if (hi <= lo) {
        lo = 0;
        hi = max_start;
        region = 0;
    }
    int width = hi - lo;
    int pos = lo + (int)(curriculum_hash(episode, 999) %
                         (uint64_t)(width > 0 ? width : 1));

    last_curriculum_region = region;
    last_curriculum_priority = curriculum_priority_for(region);
    last_curriculum_progress = curriculum_regions[region].progress;
    return pos;
}

static void curriculum_update(int source_pos,
                              float outcome,
                              float debt,
                              float surprise) {
    if (curriculum_count <= 0) return;
    int region = source_pos / curriculum_region_span;
    if (region < 0 || region >= curriculum_count) return;

    CurriculumRegion *r = &curriculum_regions[region];
    float old_fast = r->fast;
    if (r->visits == 0) {
        r->fast = outcome;
        r->slow = outcome;
    } else {
        r->fast = 0.84f * r->fast + 0.16f * outcome;
        r->slow = 0.985f * r->slow + 0.015f * outcome;
    }
    float signed_progress = r->fast - r->slow;
    float absolute_progress = fabsf(signed_progress);
    r->progress =
        0.80f * r->progress +
        0.20f * (0.70f * absolute_progress +
                 0.30f * fmaxf(0.0f, signed_progress));
    r->surprise =
        0.90f * r->surprise +
        0.10f * (0.55f * fabsf(outcome - old_fast) +
                 0.45f * surprise);
    r->debt = r->visits == 0 ? debt :
              0.94f * r->debt + 0.06f * debt;
    r->visits++;
    r->last_seen = episode_count;
    r->priority = curriculum_priority_for(region);

    last_curriculum_region = region;
    last_curriculum_priority = r->priority;
    last_curriculum_progress = r->progress;
    last_episode_coherence = outcome;
}

static float randn(float scale) {
    float u1 = randf() + 1e-7f;
    float u2 = randf();
    return scale * sqrtf(-2.0f * logf(u1)) * cosf(6.28318530718f * u2);
}

static float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

static float screening_utility(const float score[SCORE_DIM]);
static PlasticityRule plasticity_effective_rule(void);
static float glyph_semantic_authority(const CausalGlyph *g);

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

/* A world that claims probability continues beyond what it has seen may
   not silently discard what it has seen. Both walls are counted. */
static uint64_t corpus_dropped_vocab = 0;
static uint64_t corpus_dropped_capacity = 0;

static void emit_token(char *buf, int *len) {
    if (*len <= 0) return;
    if (corpus_n >= MAX_CORPUS) {
        corpus_dropped_capacity++;
        *len = 0;
        return;
    }
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
    } else {
        corpus_dropped_vocab++;
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

    while ((c = fgetc(f)) != EOF) {
        if (isspace((unsigned char)c)) {
            emit_token(buf, &len);
        } else if (is_punct_token(c)) {
            emit_token(buf, &len);
            char p[2] = {(char)c, '\0'};
            if (corpus_n >= MAX_CORPUS) {
                corpus_dropped_capacity++;
                continue;
            }
            int id = token_id(p, 1);
            if (id >= 0) {
                corpus[corpus_n++] = id;
                vocab[id].count++;
            } else {
                corpus_dropped_vocab++;
            }
        } else if (len < MAX_TOKEN_LEN - 1) {
            buf[len++] = (char)c;
        }
    }
    emit_token(buf, &len);
    fclose(f);

    if (corpus_dropped_vocab)
        fprintf(stderr,
                "netta: %llu corpus token(s) dropped — vocabulary full at "
                "%d entries; this world is smaller than its text\n",
                (unsigned long long)corpus_dropped_vocab, MAX_VOCAB);
    if (corpus_dropped_capacity)
        fprintf(stderr,
                "netta: %llu corpus token(s) dropped — corpus capacity "
                "full at %d tokens\n",
                (unsigned long long)corpus_dropped_capacity, MAX_CORPUS);

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
    trigrams[t].policy_quote = 0.5f;
    trigrams[t].policy_debt = 1.0f;
    trigrams[t].policy_momentum = 0.0f;
    trigrams[t].policy_volatility = 0.0f;
    trigrams[t].policy_visits = 0;
    trigrams[t].policy_last_episode = 0;
    trigrams[t].next_bucket = first_trigram[bucket];
    first_trigram[bucket] = t;
    return t;
}

static uint32_t source_trigram_count(int a, int b, int c) {
    int t = get_trigram(a, b, c, 0);
    return t >= 0 ? trigrams[t].count : 0;
}

static float context_policy_score(const int *ctx, int ctx_n, int candidate) {
    if (!policy_enabled || ctx_n < 2) return 0.5f;

    int t = get_trigram(ctx[ctx_n - 2], ctx[ctx_n - 1], candidate, 0);
    if (t < 0 || trigrams[t].policy_visits == 0) return 0.5f;

    TrigramEdge *p = &trigrams[t];
    float confidence = 1.0f - expf(-(float)p->policy_visits / 8.0f);
    uint64_t age = episode_count > p->policy_last_episode ?
        episode_count - p->policy_last_episode : 0;
    float freshness = 1.0f / sqrtf(1.0f + (float)age / 512.0f);
    float marked = p->policy_quote + 0.22f * p->policy_momentum -
                   0.18f * p->policy_debt - 0.12f * p->policy_volatility;
    marked = clampf(marked, 0.0f, 1.0f);
    return clampf(0.5f + confidence * freshness * (marked - 0.5f),
                  0.0f, 1.0f);
}

static void policy_mark(int a, int b, int candidate, float target) {
    if (!policy_enabled || evaluation_mode) return;
    int t = get_trigram(a, b, candidate, 1);
    if (t < 0) return;

    TrigramEdge *p = &trigrams[t];
    float old_quote = p->policy_quote;
    float delta = target - old_quote;
    p->policy_momentum = 0.82f * p->policy_momentum + 0.18f * delta;
    p->policy_quote = 0.90f * p->policy_quote + 0.10f * target;
    p->policy_debt = 0.94f * p->policy_debt + 0.06f * fabsf(delta);
    p->policy_volatility = 0.92f * p->policy_volatility +
        0.08f * fabsf(p->policy_quote - old_quote);
    p->policy_visits++;
    p->policy_last_episode = episode_count;
}

static void policy_improve_context(const int *ctx, int ctx_n,
                                   const int *tokens,
                                   const float deep[][SCORE_DIM],
                                   int n, float *target_out) {
    if (target_out) for (int i = 0; i < n; ++i) target_out[i] = 0.5f;
    if (!policy_enabled || ctx_n < 2 || n <= 0) return;

    float utility[8];
    float max_u = -1e30f;
    for (int i = 0; i < n; ++i) {
        utility[i] = screening_utility(deep[i]);
        if (utility[i] > max_u) max_u = utility[i];
    }

    /* Search policy is a soft distribution, not a winner-take-all medal. */
    float temperature = 0.10f;
    float total = 0.0f;
    for (int i = 0; i < n; ++i) {
        utility[i] = expf((utility[i] - max_u) / temperature);
        total += utility[i];
    }
    if (total <= 1e-12f) total = 1.0f;

    int a = ctx[ctx_n - 2];
    int b = ctx[ctx_n - 1];
    for (int i = 0; i < n; ++i) {
        float target = utility[i] / total;
        policy_mark(a, b, tokens[i], target);
        if (target_out) target_out[i] = target;
    }
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


static void future_top_update(int source, int horizon, int future, float weight) {
    int n = future_top_n[source][horizon];
    for (int i = 0; i < n; ++i) {
        if (future_top_tokens[source][horizon][i] == future) {
            future_top_weight[source][horizon][i] += weight;
            return;
        }
    }

    if (n < FUTURE_TOPK) {
        future_top_tokens[source][horizon][n] = future;
        future_top_weight[source][horizon][n] = weight;
        future_top_n[source][horizon]++;
        return;
    }

    int weakest = 0;
    for (int i = 1; i < FUTURE_TOPK; ++i)
        if (future_top_weight[source][horizon][i] <
            future_top_weight[source][horizon][weakest])
            weakest = i;

    /* Space-Saving: preserve recurring possibilities without full tables. */
    float floor = future_top_weight[source][horizon][weakest];
    future_top_tokens[source][horizon][weakest] = future;
    future_top_weight[source][horizon][weakest] = floor + weight;
}

static void build_future_fields(void) {
    memset(future_top_tokens, 0xFF, sizeof(future_top_tokens));
    memset(future_top_weight, 0, sizeof(future_top_weight));
    memset(future_top_prob, 0, sizeof(future_top_prob));
    memset(future_top_n, 0, sizeof(future_top_n));
    memset(future_conf, 0, sizeof(future_conf));
    memset(global_future_weight, 0, sizeof(global_future_weight));
    memset(global_future_total, 0, sizeof(global_future_total));
    memset(global_glyph_sketch, 0, sizeof(global_glyph_sketch));

    static float evidence[MAX_VOCAB][PROPHECY_HORIZONS];
    memset(evidence, 0, sizeof(evidence));

    for (int i = 0; i < corpus_n; ++i) {
        int source = corpus[i];
        int hi = i + 17;
        if (hi > corpus_n) hi = corpus_n;
        for (int j = i + 1; j < hi; ++j) {
            int offset = j - i;
            int h = offset == 1 ? 0 : (offset <= 5 ? 1 : 2);
            float w = h == 0 ? 1.0f :
                (h == 1 ? 1.0f / sqrtf((float)(offset - 1)) :
                          0.55f / sqrtf((float)(offset - 5)));
            int future = corpus[j];
            evidence[source][h] += w;
            global_future_weight[h][future] += w;
            global_future_total[h] += w;
            future_top_update(source, h, future, w);
        }
    }

    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        for (int t = 0; t < vocab_size; ++t) {
            uint64_t key = (uint64_t)(uint32_t)t ^
                           (0x9E3779B97F4A7C15ULL * (uint64_t)(h + 1));
            int bucket = (int)(mix64(key) & (GLYPH_SKETCH - 1));
            global_glyph_sketch[h][bucket] += global_future_weight[h][t];
        }
    }

    for (int t = 0; t < vocab_size; ++t) {
        for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
            int n = future_top_n[t][h];
            float raw_total = 0.0f;
            for (int i = 0; i < n; ++i)
                raw_total += future_top_weight[t][h][i];
            if (raw_total <= 0.0f) continue;

            float adjusted[FUTURE_TOPK];
            float adjusted_total = 0.0f;
            float specificity = 0.0f;
            for (int i = 0; i < n; ++i) {
                int future = future_top_tokens[t][h][i];
                float raw_p = future_top_weight[t][h][i] / raw_total;
                float base_p = global_future_total[h] > 0.0f ?
                    global_future_weight[h][future] /
                    global_future_total[h] : 1e-9f;
                float lift = raw_p / (base_p + 1e-9f);
                float information = log1pf(clampf(lift, 0.0f, 1000.0f));
                adjusted[i] = raw_p * information;
                adjusted_total += adjusted[i];
                specificity += raw_p * logf(lift + 1e-9f);
            }
            if (adjusted_total <= 1e-12f) adjusted_total = 1.0f;

            float entropy = 0.0f, max_p = 0.0f;
            for (int i = 0; i < n; ++i) {
                float q = adjusted[i] / adjusted_total;
                future_top_prob[t][h][i] = q;
                if (q > max_p) max_p = q;
                entropy -= q * logf(q + 1e-12f);
            }
            float max_entropy = n > 1 ? logf((float)n) : 1.0f;
            float concentration = n > 1 ?
                1.0f - entropy / max_entropy : 1.0f;
            float mass = 1.0f - expf(-evidence[t][h] / 12.0f);
            float distinctiveness = clampf(0.5f + 0.20f * specificity,
                                             0.0f, 1.0f);
            future_conf[t][h] = clampf(
                mass * (0.40f * max_p + 0.28f * concentration +
                        0.32f * distinctiveness), 0.02f, 1.0f);
        }
    }
}

static void stack_add_distribution(ProphecyStack *stack, int horizon,
                                   int source, int source_horizon,
                                   float weight) {
    int n = future_top_n[source][source_horizon];
    for (int i = 0; i < n; ++i) {
        int tok = future_top_tokens[source][source_horizon][i];
        float mass = weight * future_top_prob[source][source_horizon][i];
        int found = -1;
        for (int j = 0; j < stack->n[horizon]; ++j)
            if (stack->token[horizon][j] == tok) { found = j; break; }
        if (found >= 0) {
            stack->prob[horizon][found] += mass;
            continue;
        }
        int m = stack->n[horizon];
        if (m < FUTURE_TOPK) {
            stack->token[horizon][m] = tok;
            stack->prob[horizon][m] = mass;
            stack->n[horizon]++;
        } else {
            int weakest = 0;
            for (int j = 1; j < FUTURE_TOPK; ++j)
                if (stack->prob[horizon][j] < stack->prob[horizon][weakest])
                    weakest = j;
            if (mass > stack->prob[horizon][weakest]) {
                stack->token[horizon][weakest] = tok;
                stack->prob[horizon][weakest] = mass;
            }
        }
    }
}

static void stack_normalize(ProphecyStack *stack, int h) {
    float total = 0.0f;
    for (int i = 0; i < stack->n[h]; ++i) total += stack->prob[h][i];
    if (total <= 1e-12f) return;
    for (int i = 0; i < stack->n[h]; ++i) stack->prob[h][i] /= total;
}

static void prophecy_stack_init(const int *ctx, int n, ProphecyStack *stack) {
    memset(stack, 0, sizeof(*stack));
    if (!prophecy_stack_enabled) return;
    static const int span[PROPHECY_HORIZONS] = {2, 5, 8};
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        int start = n - span[h];
        if (start < 0) start = 0;
        float conf_total = 0.0f, weight_total = 0.0f;
        for (int i = start; i < n; ++i) {
            int tok = ctx[i];
            float recency = 0.20f + 0.80f *
                (float)(i - start + 1) / (float)(n - start);
            float w = recency * (0.15f + 0.85f * future_conf[tok][h]);
            stack_add_distribution(stack, h, tok, h, w);
            conf_total += w * future_conf[tok][h];
            weight_total += w;
        }
        stack_normalize(stack, h);
        stack->confidence[h] = weight_total > 0.0f ?
            clampf(conf_total / weight_total, 0.02f, 1.0f) : 0.02f;
        stack->debt[h] = 0.45f + 0.55f * stack->confidence[h];
    }
}

static float stack_token_prob(const ProphecyStack *stack, int h, int tok) {
    if (!prophecy_stack_enabled || !stack) return 0.0f;
    for (int i = 0; i < stack->n[h]; ++i)
        if (stack->token[h][i] == tok) return stack->prob[h][i];
    return 0.0f;
}

static float distribution_overlap(const ProphecyStack *stack, int h,
                                  int source, int source_horizon) {
    if (!prophecy_stack_enabled || !stack) return 0.0f;
    float overlap = 0.0f;
    int n = future_top_n[source][source_horizon];
    for (int i = 0; i < n; ++i) {
        int tok = future_top_tokens[source][source_horizon][i];
        float a = future_top_prob[source][source_horizon][i];
        float b = stack_token_prob(stack, h, tok);
        overlap += a < b ? a : b;
    }
    return clampf(overlap, 0.0f, 1.0f);
}

static float prophecy_stack_total_debt(const ProphecyStack *stack) {
    if (!prophecy_stack_enabled || !stack) return 0.0f;
    return stack->debt[0] + 0.55f * stack->debt[1] +
           0.25f * stack->debt[2];
}

static void prophecy_stack_preview(const ProphecyStack *stack, int candidate,
                                   float *fulfillment,
                                   float *stability,
                                   float *after_debt) {
    if (!prophecy_stack_enabled || !stack) {
        if (fulfillment) *fulfillment = 0.5f;
        if (stability) *stability = 0.5f;
        if (after_debt) *after_debt = 0.0f;
        return;
    }

    float top_near = stack->n[0] ? stack->prob[0][0] : 1.0f;
    for (int i = 1; i < stack->n[0]; ++i)
        if (stack->prob[0][i] > top_near) top_near = stack->prob[0][i];
    float near_exact = stack_token_prob(stack, 0, candidate) /
                       (top_near + 1e-9f);
    float near_semantic = 0.0f;
    if (stack->n[0] > 0) {
        for (int i = 0; i < stack->n[0]; ++i)
            near_semantic += stack->prob[0][i] *
                fmaxf(0.0f, cosine(vocab[candidate].emb,
                                    vocab[stack->token[0][i]].emb,
                                    EMBED_DIM));
    }
    float clause = 0.52f * distribution_overlap(stack, 1, candidate, 0) +
                   0.45f * distribution_overlap(stack, 1, candidate, 1) +
                   0.03f * near_semantic;
    float discourse = 0.68f * distribution_overlap(stack, 2, candidate, 2) +
                      0.30f * distribution_overlap(stack, 2, candidate, 1) +
                      0.02f * near_semantic;
    float near = 0.96f * clampf(near_exact, 0.0f, 1.0f) +
                 0.04f * near_semantic;
    float paid = 0.52f * near + 0.30f * clause + 0.18f * discourse;

    float debt_after[PROPHECY_HORIZONS];
    float score[PROPHECY_HORIZONS] = {near, clause, discourse};
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        float pressure = 0.10f + 0.12f * stack->confidence[h];
        debt_after[h] = clampf(
            (1.0f - pressure) * stack->debt[h] +
            pressure * (1.0f - score[h]), 0.0f, 2.0f);
    }
    float total_before = prophecy_stack_total_debt(stack);
    float total_after = debt_after[0] + 0.55f * debt_after[1] +
                        0.25f * debt_after[2];
    float debt_progress = total_before - total_after;
    float progress_score = clampf(0.5f + 3.0f * debt_progress,
                                  0.0f, 1.0f);
    float absolute_score = 1.0f / (1.0f + total_after);
    float stable = 0.78f * progress_score + 0.22f * absolute_score;
    if (fulfillment) *fulfillment = clampf(paid, 0.0f, 1.0f);
    if (stability) *stability = clampf(stable, 0.0f, 1.0f);
    if (after_debt) *after_debt = total_after;
}

static void prophecy_stack_step(ProphecyStack *stack, int chosen,
                                float *paid_out, float *after_out) {
    if (!prophecy_stack_enabled || !stack) return;
    float paid = 0.0f, stable = 0.0f, after = 0.0f;
    prophecy_stack_preview(stack, chosen, &paid, &stable, &after);

    ProphecyStack next;
    memset(&next, 0, sizeof(next));
    /* Residual old obligations move closer while the chosen word opens new ones. */
    for (int i = 0; i < stack->n[1]; ++i) {
        int tok = stack->token[1][i];
        int m = next.n[0]++;
        if (m < FUTURE_TOPK) {
            next.token[0][m] = tok;
            next.prob[0][m] = 0.28f * stack->prob[1][i];
        } else next.n[0] = FUTURE_TOPK;
    }
    stack_add_distribution(&next, 0, chosen, 0, 0.72f);
    for (int i = 0; i < stack->n[2]; ++i) {
        int tok = stack->token[2][i];
        int m = next.n[1]++;
        if (m < FUTURE_TOPK) {
            next.token[1][m] = tok;
            next.prob[1][m] = 0.22f * stack->prob[2][i];
        } else next.n[1] = FUTURE_TOPK;
    }
    stack_add_distribution(&next, 1, chosen, 1, 0.78f);
    for (int i = 0; i < stack->n[2]; ++i) {
        int m = next.n[2]++;
        if (m < FUTURE_TOPK) {
            next.token[2][m] = stack->token[2][i];
            next.prob[2][m] = 0.30f * stack->prob[2][i];
        } else next.n[2] = FUTURE_TOPK;
    }
    stack_add_distribution(&next, 2, chosen, 2, 0.70f);
    /* Destiny settles the exact same obligations used during preview. */
    float top_near = stack->n[0] ? stack->prob[0][0] : 1.0f;
    for (int i = 1; i < stack->n[0]; ++i)
        if (stack->prob[0][i] > top_near) top_near = stack->prob[0][i];
    float near_exact = stack_token_prob(stack, 0, chosen) /
                       (top_near + 1e-9f);
    float near_semantic = 0.0f;
    for (int i = 0; i < stack->n[0]; ++i)
        near_semantic += stack->prob[0][i] *
            fmaxf(0.0f, cosine(vocab[chosen].emb,
                                vocab[stack->token[0][i]].emb,
                                EMBED_DIM));
    float paid_score[PROPHECY_HORIZONS];
    paid_score[0] = 0.96f * clampf(near_exact, 0.0f, 1.0f) +
                    0.04f * near_semantic;
    paid_score[1] = 0.52f * distribution_overlap(stack, 1, chosen, 0) +
                    0.45f * distribution_overlap(stack, 1, chosen, 1) +
                    0.03f * near_semantic;
    paid_score[2] = 0.68f * distribution_overlap(stack, 2, chosen, 2) +
                    0.30f * distribution_overlap(stack, 2, chosen, 1) +
                    0.02f * near_semantic;

    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        stack_normalize(&next, h);
        float pressure = 0.10f + 0.12f * stack->confidence[h];
        next.debt[h] = clampf((1.0f - pressure) * stack->debt[h] +
                              pressure * (1.0f - paid_score[h]),
                              0.0f, 2.0f);
        next.confidence[h] = clampf(
            0.70f * stack->confidence[h] +
            0.30f * future_conf[chosen][h], 0.02f, 1.0f);
    }
    next.paid = stack->paid + paid;
    next.overdue = stack->overdue + after;
    next.steps = stack->steps + 1;
    *stack = next;
    if (paid_out) *paid_out = paid;
    if (after_out) *after_out = prophecy_stack_total_debt(stack);
}


/* ───────────────────── Emergent causal glyphs ─────────────────────
 *
 * A glyph is not a human semantic label. It is an online equivalence class
 * of histories that promise similar observable futures. The causal control
 * clusters ProphecyStack signatures; the random control gets exactly the
 * same memory and update rules but hashes contexts into arbitrary buckets.
 */

static void context_embedding(const int *ctx, int n, float *out);

static float glyph_sparse_lookup(const int *tok, const float *prob,
                                 int n, int target) {
    for (int i = 0; i < n; ++i)
        if (tok[i] == target) return prob[i];
    return 0.0f;
}

static void glyph_sparse_normalize(float *prob, int n) {
    float total = 0.0f;
    for (int i = 0; i < n; ++i) total += prob[i];
    if (total <= 1e-12f) return;
    for (int i = 0; i < n; ++i) prob[i] /= total;
}

static void glyph_sparse_add(int *tok, float *value, uint8_t *n,
                             int target, float mass, int cap) {
    if (mass <= 0.0f || target < 0) return;
    for (int i = 0; i < *n; ++i) {
        if (tok[i] == target) {
            value[i] += mass;
            return;
        }
    }
    if (*n < cap) {
        int i = (*n)++;
        tok[i] = target;
        value[i] = mass;
        return;
    }
    int weakest = 0;
    for (int i = 1; i < cap; ++i)
        if (value[i] < value[weakest]) weakest = i;
    if (mass > value[weakest]) {
        tok[weakest] = target;
        value[weakest] = mass;
    }
}

static float glyph_sparse_js(const int *a_tok, const float *a_prob, int an,
                             const int *b_tok, const float *b_prob, int bn) {
    float js = 0.0f;
    for (int i = 0; i < an; ++i) {
        float p = a_prob[i];
        float q = glyph_sparse_lookup(b_tok, b_prob, bn, a_tok[i]);
        float m = 0.5f * (p + q);
        if (p > 1e-12f) js += 0.5f * p * logf(p / (m + 1e-12f));
        if (q > 1e-12f) js += 0.5f * q * logf(q / (m + 1e-12f));
    }
    for (int i = 0; i < bn; ++i) {
        if (glyph_sparse_lookup(a_tok, a_prob, an, b_tok[i]) > 0.0f) continue;
        float q = b_prob[i];
        float m = 0.5f * q;
        if (q > 1e-12f) js += 0.5f * q * logf(q / (m + 1e-12f));
    }
    return clampf(js / 0.69314718056f, 0.0f, 1.0f);
}


static void glyph_stack_future_embedding(const ProphecyStack *stack,
                                         int h, float *out) {
    memset(out, 0, EMBED_DIM * sizeof(float));
    if (!stack || h < 0 || h >= PROPHECY_HORIZONS) return;
    for (int i = 0; i < stack->n[h]; ++i) {
        int tok = stack->token[h][i];
        if (tok < 0 || tok >= vocab_size) continue;
        float p = stack->prob[h][i];
        for (int d = 0; d < EMBED_DIM; ++d)
            out[d] += p * vocab[tok].emb[d];
    }
    normalize(out, EMBED_DIM);
}

static float glyph_seed_distance(const ProphecyStack *stack,
                                 const GlyphSeed *g) {
    if (!g || !g->active || !stack) return 1.0f;
    static const float hw[PROPHECY_HORIZONS] = {0.50f, 0.31f, 0.19f};
    float js = 0.0f, semantic = 0.0f, debt = 0.0f, conf = 0.0f;
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        float emb[EMBED_DIM];
        glyph_stack_future_embedding(stack, h, emb);
        js += hw[h] * glyph_sparse_js(
            stack->token[h], stack->prob[h], stack->n[h],
            g->sig_token[h], g->sig_prob[h], g->sig_n[h]);
        semantic += hw[h] *
            0.5f * (1.0f - cosine(emb, g->sig_future_emb[h], EMBED_DIM));
        debt += hw[h] * fabsf(stack->debt[h] - g->sig_debt[h]) * 0.5f;
        conf += hw[h] * fabsf(stack->confidence[h] - g->sig_conf[h]);
    }
    return clampf(0.42f * js + 0.38f * semantic +
                  0.13f * debt + 0.07f * conf, 0.0f, 1.0f);
}

static float glyph_signature_distance(const ProphecyStack *stack,
                                      const CausalGlyph *g) {
    if (!g || !g->active || !stack) return 1.0f;
    static const float hw[PROPHECY_HORIZONS] = {0.50f, 0.31f, 0.19f};
    float js = 0.0f, semantic = 0.0f, debt = 0.0f, conf = 0.0f;
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        float emb[EMBED_DIM];
        glyph_stack_future_embedding(stack, h, emb);
        js += hw[h] * glyph_sparse_js(
            stack->token[h], stack->prob[h], stack->n[h],
            g->sig_token[h], g->sig_prob[h], g->sig_n[h]);
        semantic += hw[h] *
            0.5f * (1.0f - cosine(emb, g->sig_future_emb[h], EMBED_DIM));
        debt += hw[h] * fabsf(stack->debt[h] - g->sig_debt[h]) * 0.5f;
        conf += hw[h] * fabsf(stack->confidence[h] - g->sig_conf[h]);
    }
    return clampf(0.42f * js + 0.38f * semantic +
                  0.13f * debt + 0.07f * conf, 0.0f, 1.0f);
}

static float glyph_pair_distance(const CausalGlyph *a,
                                 const CausalGlyph *b) {
    if (!a || !b || !a->active || !b->active) return 1.0f;
    static const float hw[PROPHECY_HORIZONS] = {0.50f, 0.31f, 0.19f};
    float js = 0.0f, semantic = 0.0f, debt = 0.0f, conf = 0.0f;
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        js += hw[h] * glyph_sparse_js(
            a->sig_token[h], a->sig_prob[h], a->sig_n[h],
            b->sig_token[h], b->sig_prob[h], b->sig_n[h]);
        semantic += hw[h] *
            0.5f * (1.0f - cosine(a->sig_future_emb[h],
                                  b->sig_future_emb[h], EMBED_DIM));
        debt += hw[h] * fabsf(a->sig_debt[h] - b->sig_debt[h]) * 0.5f;
        conf += hw[h] * fabsf(a->sig_conf[h] - b->sig_conf[h]);
    }
    return clampf(0.42f * js + 0.38f * semantic +
                  0.13f * debt + 0.07f * conf, 0.0f, 1.0f);
}

static int glyph_weakest_slot(void) {
    for (int i = 0; i < MAX_GLYPHS; ++i)
        if (!causal_glyphs[i].active) return i;

    int weakest = 0;
    float weakest_score = 1e30f;
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        CausalGlyph *g = &causal_glyphs[i];
        float age = (float)(episode_count > g->last_seen ?
                            episode_count - g->last_seen : 0);
        float gain = clampf(g->predictive_gain_ema, -1.0f, 2.0f);
        float retained = (1.0f + (float)g->uses) *
                         (0.55f + 0.20f * gain) /
                         (1.0f + age / 192.0f);
        if (retained < weakest_score) {
            weakest_score = retained;
            weakest = i;
        }
    }
    return weakest;
}

static void glyph_copy_signature(CausalGlyph *g,
                                 const ProphecyStack *stack) {
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        g->sig_n[h] = stack ? stack->n[h] : 0;
        for (int i = 0; i < g->sig_n[h]; ++i) {
            g->sig_token[h][i] = stack->token[h][i];
            g->sig_prob[h][i] = stack->prob[h][i];
        }
        g->sig_debt[h] = stack ? stack->debt[h] : 0.5f;
        g->sig_conf[h] = stack ? stack->confidence[h] : 0.02f;
        if (stack)
            glyph_stack_future_embedding(stack, h, g->sig_future_emb[h]);
    }
}

static void glyph_topology_clear(int gid);

static int glyph_birth_at(int slot, const ProphecyStack *stack,
                          const int *ctx, int ctx_n) {
    if (slot < 0 || slot >= MAX_GLYPHS) return -1;
    int recycled = causal_glyphs[slot].active;
    if (recycled) {
        glyph_recycles++;
    } else {
        glyph_count++;
    }

    glyph_topology_clear(slot);
    CausalGlyph *g = &causal_glyphs[slot];
    memset(g, 0, sizeof(*g));
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        for (int i = 0; i < GLYPH_TOPK; ++i) {
            g->sig_token[h][i] = -1;
            g->future_token[h][i] = -1;
        }
    }
    for (int i = 0; i < GLYPH_ACTION_TOPK; ++i)
        g->action_token[i] = -1;

    glyph_copy_signature(g, stack);
    if (ctx && ctx_n > 0)
        context_embedding(ctx, ctx_n, g->context_centroid);
    g->uses = 1;
    g->born_episode = episode_count;
    g->last_seen = episode_count;
    g->parent_a = -1;
    g->parent_b = -1;
    g->active = 1;
    glyph_births++;
    return slot;
}

static void glyph_adapt_signature(int gid,
                                  const ProphecyStack *stack,
                                  const int *ctx, int ctx_n,
                                  float distance,
                                  float scale,
                                  int direct_observation) {
    if (gid < 0 || gid >= MAX_GLYPHS || !stack) return;
    CausalGlyph *g = &causal_glyphs[gid];
    if (!g->active) return;

    float base_lr = clampf(
        1.0f / sqrtf((float)g->uses + 2.0f),
        0.025f, 0.18f);
    float lr = clampf(base_lr * scale, 0.001f, 0.18f);

    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        for (int i = 0; i < g->sig_n[h]; ++i)
            g->sig_prob[h][i] *= (1.0f - lr);
        for (int i = 0; i < stack->n[h]; ++i)
            glyph_sparse_add(g->sig_token[h], g->sig_prob[h],
                             &g->sig_n[h], stack->token[h][i],
                             lr * stack->prob[h][i], GLYPH_TOPK);
        glyph_sparse_normalize(g->sig_prob[h], g->sig_n[h]);
        g->sig_debt[h] =
            (1.0f - lr) * g->sig_debt[h] + lr * stack->debt[h];
        g->sig_conf[h] =
            (1.0f - lr) * g->sig_conf[h] + lr * stack->confidence[h];

        float emb[EMBED_DIM];
        glyph_stack_future_embedding(stack, h, emb);
        for (int d = 0; d < EMBED_DIM; ++d)
            g->sig_future_emb[h][d] =
                (1.0f - lr) * g->sig_future_emb[h][d] + lr * emb[d];
        normalize(g->sig_future_emb[h], EMBED_DIM);
    }

    if (ctx && ctx_n > 0) {
        float cemb[EMBED_DIM];
        context_embedding(ctx, ctx_n, cemb);
        for (int d = 0; d < EMBED_DIM; ++d)
            g->context_centroid[d] =
                (1.0f - lr) * g->context_centroid[d] + lr * cemb[d];
        normalize(g->context_centroid, EMBED_DIM);
    }

    if (direct_observation) {
        g->assignment_distance_ema =
            g->uses > 1 ?
            0.96f * g->assignment_distance_ema +
            0.04f * distance : distance;
        g->topology_error_ema =
            0.97f * g->topology_error_ema +
            0.03f * distance;
        g->topology_wins++;
        g->uses++;
        g->last_seen = episode_count;
    }
}

static void glyph_update_signature(int gid, const ProphecyStack *stack,
                                   const int *ctx, int ctx_n,
                                   float distance) {
    glyph_adapt_signature(gid, stack, ctx, ctx_n,
                          distance, 1.0f, 1);
}

static void glyph_topology_clear(int gid) {
    if (gid < 0 || gid >= MAX_GLYPHS) return;
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        glyph_link_age[gid][i] = 0;
        glyph_link_age[i][gid] = 0;
    }
}

static void glyph_topology_connect(int a, int b) {
    if (!neighbor_enabled || glyph_mode != GLYPH_CAUSAL ||
        a < 0 || b < 0 || a == b ||
        a >= MAX_GLYPHS || b >= MAX_GLYPHS)
        return;
    glyph_link_age[a][b] = 1;
    glyph_link_age[b][a] = 1;
}

static void glyph_topology_observe(int best, int second,
                                   float best_distance,
                                   float second_distance,
                                   const ProphecyStack *stack,
                                   const int *ctx, int ctx_n) {
    if (!neighbor_enabled || glyph_mode != GLYPH_CAUSAL ||
        best < 0 || best >= MAX_GLYPHS)
        return;

    for (int n = 0; n < MAX_GLYPHS; ++n) {
        uint16_t age = glyph_link_age[best][n];
        if (!age) continue;
        if (age >= GLYPH_LINK_MAX_AGE ||
            !causal_glyphs[n].active) {
            glyph_link_age[best][n] = 0;
            glyph_link_age[n][best] = 0;
        } else {
            glyph_link_age[best][n] = age + 1;
            glyph_link_age[n][best] = age + 1;
        }
    }

    if (second >= 0)
        glyph_topology_connect(best, second);

    CausalGlyph *winner = &causal_glyphs[best];
    float gap = second >= 0 ?
        clampf(second_distance - best_distance, 0.0f, 1.0f) :
        0.0f;
    winner->topology_utility_ema =
        0.97f * winner->topology_utility_ema + 0.03f * gap;

    PlasticityRule rule = plasticity_effective_rule();
    float neighbor_gain = rule.neighbor_gain;

    for (int n = 0; n < MAX_GLYPHS; ++n) {
        uint16_t age = glyph_link_age[best][n];
        if (!age || !causal_glyphs[n].active) continue;

        float age_weight =
            expf(-(float)(age - 1) / 24.0f);
        float similarity =
            1.0f - glyph_pair_distance(&causal_glyphs[best],
                                       &causal_glyphs[n]);
        float winner_authority =
            glyph_semantic_authority(winner);
        float neighbor_authority =
            glyph_semantic_authority(&causal_glyphs[n]);
        float earned_bridge = sqrtf(
            (0.02f + 0.98f * winner_authority) *
            (0.02f + 0.98f * neighbor_authority));
        float scale =
            neighbor_gain * age_weight *
            clampf(similarity, 0.0f, 1.0f) *
            earned_bridge;
        if (scale > 0.0005f)
            glyph_adapt_signature(n, stack, ctx, ctx_n,
                                  best_distance, scale, 0);
    }
}


static int glyph_topology_edge_count(void) {
    int count = 0;
    for (int a = 0; a < MAX_GLYPHS; ++a)
        for (int b = a + 1; b < MAX_GLYPHS; ++b)
            if (glyph_link_age[a][b]) count++;
    return count;
}

static uint64_t glyph_context_hash(const int *ctx, int ctx_n) {
    uint64_t h = 0xCBF29CE484222325ULL;
    int start = ctx_n - 8;
    if (start < 0) start = 0;
    for (int i = start; i < ctx_n; ++i) {
        h ^= mix64((uint64_t)(uint32_t)ctx[i] +
                   0x9E3779B97F4A7C15ULL * (uint64_t)(i - start + 1));
        h *= 0x100000001B3ULL;
    }
    return mix64(h);
}

static int glyph_seed_slot(void) {
    for (int i = 0; i < GLYPH_NURSERY; ++i)
        if (!glyph_nursery[i].active) return i;
    int weakest = 0;
    float weakest_score = 1e30f;
    for (int i = 0; i < GLYPH_NURSERY; ++i) {
        GlyphSeed *g = &glyph_nursery[i];
        float age = (float)(episode_count > g->last_seen ?
                            episode_count - g->last_seen : 0);
        float retained = (float)g->hits / (1.0f + age / 64.0f);
        if (retained < weakest_score) {
            weakest_score = retained;
            weakest = i;
        }
    }
    return weakest;
}

static void glyph_seed_init(int slot, const ProphecyStack *stack,
                            const int *ctx, int ctx_n) {
    GlyphSeed *g = &glyph_nursery[slot];
    memset(g, 0, sizeof(*g));
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        g->sig_n[h] = stack->n[h];
        for (int i = 0; i < stack->n[h]; ++i) {
            g->sig_token[h][i] = stack->token[h][i];
            g->sig_prob[h][i] = stack->prob[h][i];
        }
        g->sig_debt[h] = stack->debt[h];
        g->sig_conf[h] = stack->confidence[h];
        glyph_stack_future_embedding(stack, h, g->sig_future_emb[h]);
    }
    if (ctx && ctx_n > 0)
        context_embedding(ctx, ctx_n, g->context_centroid);
    g->hits = 1;
    g->last_seen = episode_count;
    g->active = 1;
    glyph_seed_births++;
}

static void glyph_seed_update(GlyphSeed *g, const ProphecyStack *stack,
                              const int *ctx, int ctx_n) {
    float lr = clampf(1.0f / sqrtf((float)g->hits + 2.0f),
                      0.08f, 0.28f);
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        for (int i = 0; i < g->sig_n[h]; ++i)
            g->sig_prob[h][i] *= (1.0f - lr);
        for (int i = 0; i < stack->n[h]; ++i)
            glyph_sparse_add(g->sig_token[h], g->sig_prob[h],
                             &g->sig_n[h], stack->token[h][i],
                             lr * stack->prob[h][i], GLYPH_TOPK);
        glyph_sparse_normalize(g->sig_prob[h], g->sig_n[h]);
        g->sig_debt[h] =
            (1.0f - lr) * g->sig_debt[h] + lr * stack->debt[h];
        g->sig_conf[h] =
            (1.0f - lr) * g->sig_conf[h] + lr * stack->confidence[h];

        float emb[EMBED_DIM];
        glyph_stack_future_embedding(stack, h, emb);
        for (int d = 0; d < EMBED_DIM; ++d)
            g->sig_future_emb[h][d] =
                (1.0f - lr) * g->sig_future_emb[h][d] + lr * emb[d];
        normalize(g->sig_future_emb[h], EMBED_DIM);
    }
    if (ctx && ctx_n > 0) {
        float cemb[EMBED_DIM];
        context_embedding(ctx, ctx_n, cemb);
        for (int d = 0; d < EMBED_DIM; ++d)
            g->context_centroid[d] =
                (1.0f - lr) * g->context_centroid[d] + lr * cemb[d];
        normalize(g->context_centroid, EMBED_DIM);
    }
    g->hits++;
    g->last_seen = episode_count;
}

static int glyph_promote_seed(int seed_slot) {
    GlyphSeed *seed = &glyph_nursery[seed_slot];
    if (!seed->active || seed->hits < GLYPH_SEED_HITS) return -1;

    ProphecyStack stack;
    memset(&stack, 0, sizeof(stack));
    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        stack.n[h] = seed->sig_n[h];
        for (int i = 0; i < stack.n[h]; ++i) {
            stack.token[h][i] = seed->sig_token[h][i];
            stack.prob[h][i] = seed->sig_prob[h][i];
        }
        stack.debt[h] = seed->sig_debt[h];
        stack.confidence[h] = seed->sig_conf[h];
    }

    int slot = glyph_weakest_slot();
    int gid = glyph_birth_at(slot, &stack, NULL, 0);
    if (gid >= 0) {
        CausalGlyph *g = &causal_glyphs[gid];
        memcpy(g->context_centroid, seed->context_centroid,
               sizeof(g->context_centroid));
        for (int h = 0; h < PROPHECY_HORIZONS; ++h)
            memcpy(g->sig_future_emb[h], seed->sig_future_emb[h],
                   sizeof(g->sig_future_emb[h]));
        g->uses = seed->hits;
        glyph_seed_promotions++;
    }
    memset(seed, 0, sizeof(*seed));
    return gid;
}

static int glyph_nursery_observe(const int *ctx, int ctx_n,
                                 const ProphecyStack *stack) {
    int best = -1;
    float best_distance = 1e30f;
    for (int i = 0; i < GLYPH_NURSERY; ++i) {
        if (!glyph_nursery[i].active) continue;
        float d = glyph_seed_distance(stack, &glyph_nursery[i]);
        if (d < best_distance) {
            best_distance = d;
            best = i;
        }
    }

    const float nursery_threshold = 0.18f;
    if (best < 0 || best_distance > nursery_threshold) {
        best = glyph_seed_slot();
        glyph_seed_init(best, stack, ctx, ctx_n);
    } else {
        glyph_seed_update(&glyph_nursery[best], stack, ctx, ctx_n);
    }

    if (glyph_nursery[best].hits >= GLYPH_SEED_HITS)
        return glyph_promote_seed(best);
    return -1;
}

static int glyph_assign(const int *ctx, int ctx_n,
                        const ProphecyStack *stack,
                        float *distance_out) {
    if (distance_out) *distance_out = 0.0f;
    if (glyph_mode == GLYPH_OFF || !stack || !prophecy_stack_enabled)
        return -1;

    if (glyph_mode == GLYPH_RANDOM) {
        int slot = (int)(glyph_context_hash(ctx, ctx_n) %
                         GLYPH_RANDOM_BUCKETS);
        int born = !causal_glyphs[slot].active;
        if (born)
            glyph_birth_at(slot, stack, ctx, ctx_n);
        float d = glyph_signature_distance(stack, &causal_glyphs[slot]);
        if (!born)
            glyph_update_signature(slot, stack, ctx, ctx_n, d);
        if (distance_out) *distance_out = d;
        return slot;
    }

    if (glyph_count == 0) {
        int gid = glyph_birth_at(glyph_weakest_slot(), stack, ctx, ctx_n);
        if (distance_out) *distance_out = 0.0f;
        return gid;
    }

    int best = -1, second = -1;
    float best_distance = 1e30f, second_distance = 1e30f;
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        if (!causal_glyphs[i].active) continue;
        float d = glyph_signature_distance(stack, &causal_glyphs[i]);
        if (d < best_distance) {
            second = best;
            second_distance = best_distance;
            best_distance = d;
            best = i;
        } else if (d < second_distance) {
            second_distance = d;
            second = i;
        }
    }

    float threshold = glyph_birth_threshold +
        0.035f * expf(-(float)episode_count / 900.0f);

    if (best_distance > threshold) {
        int promoted = glyph_nursery_observe(ctx, ctx_n, stack);
        if (promoted >= 0)
            glyph_topology_connect(promoted, best);
        if (distance_out) *distance_out = best_distance;
        return promoted;
    }

    glyph_update_signature(best, stack, ctx, ctx_n, best_distance);
    glyph_topology_observe(best, second,
                           best_distance, second_distance,
                           stack, ctx, ctx_n);
    if (distance_out) *distance_out = best_distance;
    return best;
}


/*
 * Read-only projection of an agent-generated history into the learned world
 * abstraction.  This function may consult causal glyphs, but it may never
 * create or update one.  Source trajectories teach the world's physics;
 * counterfactual trajectories only ask the world what state they resemble.
 */
static int glyph_lookup(const int *ctx, int ctx_n,
                        const ProphecyStack *stack,
                        float *distance_out) {
    if (distance_out) *distance_out = 0.0f;
    if (glyph_mode == GLYPH_OFF || !stack || !prophecy_stack_enabled)
        return -1;

    if (glyph_mode == GLYPH_RANDOM) {
        int slot = (int)(glyph_context_hash(ctx, ctx_n) %
                         GLYPH_RANDOM_BUCKETS);
        if (!causal_glyphs[slot].active)
            return -1;
        float d = glyph_signature_distance(stack, &causal_glyphs[slot]);
        if (distance_out) *distance_out = d;
        return slot;
    }

    int best = -1;
    float best_distance = 1e30f;
    for (int i = 0; i < MAX_GLYPHS; ++i) {
        if (!causal_glyphs[i].active) continue;
        float d = glyph_signature_distance(stack, &causal_glyphs[i]);
        if (d < best_distance) {
            best_distance = d;
            best = i;
        }
    }

    /*
     * Counterfactual states are allowed a wider projection radius than
     * source-state birth.  Beyond it Netta admits that this part of the
     * imagined world has no learned glyph yet.
     */
    float birth_threshold = glyph_birth_threshold +
        0.035f * expf(-(float)episode_count / 900.0f);
    float projection_threshold = clampf(1.35f * birth_threshold,
                                        birth_threshold, 0.58f);
    if (best < 0 || best_distance > projection_threshold) {
        if (distance_out) *distance_out = best_distance;
        return -1;
    }

    if (distance_out) *distance_out = best_distance;
    return best;
}

static int glyph_sketch_bucket(int h, int tok) {
    uint64_t key = (uint64_t)(uint32_t)tok ^
                   (0x9E3779B97F4A7C15ULL * (uint64_t)(h + 1));
    return (int)(mix64(key) & (GLYPH_SKETCH - 1));
}

static float glyph_global_future_prob(int h, int tok) {
    if (h < 0 || h >= PROPHECY_HORIZONS ||
        tok < 0 || tok >= vocab_size ||
        global_future_total[h] <= 1e-12f)
        return 1.0f / (float)GLYPH_SKETCH;
    int bucket = glyph_sketch_bucket(h, tok);
    return global_glyph_sketch[h][bucket] /
           (global_future_total[h] + 1e-12f);
}

static float glyph_future_prob(int gid, int h, int tok) {
    float base = glyph_global_future_prob(h, tok);
    if (gid < 0 || gid >= MAX_GLYPHS ||
        !causal_glyphs[gid].active)
        return base;

    CausalGlyph *g = &causal_glyphs[gid];
    int bucket = glyph_sketch_bucket(h, tok);
    float local = g->future_sketch[h][bucket] /
                  (g->future_total[h] + 1e-12f);
    float maturity = g->future_total[h] /
                     (g->future_total[h] + 10.0f);
    return (1.0f - maturity) * base + maturity * local;
}

static void glyph_future_add(CausalGlyph *g, int h, int tok, float weight) {
    if (!g || h < 0 || h >= PROPHECY_HORIZONS ||
        tok < 0 || weight <= 0.0f) return;
    glyph_sparse_add(g->future_token[h], g->future_weight[h],
                     &g->future_n[h], tok, weight, GLYPH_TOPK);
    int bucket = glyph_sketch_bucket(h, tok);
    g->future_sketch[h][bucket] += weight;
    for (int d = 0; d < EMBED_DIM; ++d)
        g->future_emb_sum[h][d] += weight * vocab[tok].emb[d];
    g->future_total[h] += weight;
}

static float glyph_predictive_gain(int gid, int truth) {
    if (gid < 0 || truth < 0 || truth >= vocab_size) return 0.0f;
    CausalGlyph *g = &causal_glyphs[gid];
    if (!g->active || g->future_total[0] < 2.0f) return 0.0f;
    float pg = glyph_future_prob(gid, 0, truth);
    float pb = glyph_global_future_prob(0, truth);
    return clampf(logf((pg + 1e-9f) / (pb + 1e-9f)),
                  -6.0f, 6.0f);
}

static void glyph_observe_destiny(int gid, const int *future, int n,
                                  float predictive_gain,
                                  float debt_progress) {
    if (gid < 0 || gid >= MAX_GLYPHS || !future || n <= 0) return;
    CausalGlyph *g = &causal_glyphs[gid];
    if (!g->active) return;

    int limit = n < 16 ? n : 16;
    for (int i = 0; i < limit; ++i) {
        int h = i == 0 ? 0 : (i < 5 ? 1 : 2);
        float w = h == 0 ? 1.0f :
            (h == 1 ? 1.0f / sqrtf((float)i) :
                      0.55f / sqrtf((float)(i - 4)));
        glyph_future_add(g, h, future[i], w);
    }
    g->predictive_gain_ema =
        0.96f * g->predictive_gain_ema + 0.04f * predictive_gain;
    g->progress_ema =
        0.94f * g->progress_ema + 0.06f * debt_progress;
}

static int glyph_action_slot(CausalGlyph *g, int tok, int create) {
    for (int i = 0; i < g->action_n; ++i)
        if (g->action_token[i] == tok) return i;
    if (!create) return -1;
    if (g->action_n < GLYPH_ACTION_TOPK) {
        int i = g->action_n++;
        g->action_token[i] = tok;
        g->action_quote[i] = 0.5f;
        g->action_debt[i] = 0.5f;
        g->action_visits[i] = 0;
        return i;
    }
    int weakest = 0;
    float weakest_score = 1e30f;
    for (int i = 0; i < GLYPH_ACTION_TOPK; ++i) {
        float score = g->action_quote[i] -
                      0.25f * g->action_debt[i] +
                      0.02f * log1pf((float)g->action_visits[i]);
        if (score < weakest_score) {
            weakest_score = score;
            weakest = i;
        }
    }
    g->action_token[weakest] = tok;
    g->action_quote[weakest] = 0.5f;
    g->action_debt[weakest] = 0.5f;
    g->action_visits[weakest] = 0;
    return weakest;
}

static float glyph_action_score(int gid, int tok) {
    if (gid < 0 || gid >= MAX_GLYPHS ||
        !causal_glyphs[gid].active) return 0.5f;
    CausalGlyph *g = &causal_glyphs[gid];
    int i = glyph_action_slot(g, tok, 0);
    if (i < 0 || g->action_visits[i] == 0) return 0.5f;
    float confidence =
        1.0f - expf(-(float)g->action_visits[i] / 6.0f);
    return clampf(0.5f + confidence *
                  (g->action_quote[i] -
                   0.22f * g->action_debt[i] - 0.5f),
                  0.0f, 1.0f);
}

static void glyph_action_mark(int gid, int tok, float target) {
    if (gid < 0 || gid >= MAX_GLYPHS ||
        !causal_glyphs[gid].active) return;
    CausalGlyph *g = &causal_glyphs[gid];
    int i = glyph_action_slot(g, tok, 1);
    float old = g->action_quote[i];
    float delta = target - old;
    g->action_quote[i] =
        0.90f * g->action_quote[i] + 0.10f * target;
    g->action_debt[i] =
        0.94f * g->action_debt[i] + 0.06f * fabsf(delta);
    g->action_visits[i]++;
}

static float glyph_future_overlap(int gid, int gh,
                                  int source, int sh) {
    if (gid < 0 || gid >= MAX_GLYPHS ||
        !causal_glyphs[gid].active ||
        source < 0 || source >= vocab_size)
        return 0.0f;
    float overlap = 0.0f;
    int n = future_top_n[source][sh];
    for (int i = 0; i < n; ++i) {
        int tok = future_top_tokens[source][sh][i];
        float a = future_top_prob[source][sh][i];
        float b = glyph_future_prob(gid, gh, tok);
        overlap += a < b ? a : b;
    }
    return clampf(overlap, 0.0f, 1.0f);
}

static float glyph_semantic_authority(const CausalGlyph *g) {
    if (!g || !g->active) return 0.0f;
    float evidence = 1.0f - expf(-(float)g->uses / 24.0f);
    float gain_gate = clampf(
        (g->predictive_gain_ema - 0.02f) / 0.32f, 0.0f, 1.0f);
    float assignment_reliability =
        expf(-2.2f *
             clampf(g->assignment_distance_ema, 0.0f, 1.0f));
    return clampf(
        evidence * gain_gate * assignment_reliability,
        0.0f, 1.0f);
}

static float glyph_candidate_score_single(int gid, int candidate) {
    if (glyph_mode == GLYPH_OFF || gid < 0 ||
        gid >= MAX_GLYPHS || !causal_glyphs[gid].active)
        return 0.5f;

    CausalGlyph *g = &causal_glyphs[gid];
    float max_near = glyph_global_future_prob(0, candidate);
    for (int i = 0; i < g->future_n[0]; ++i) {
        float p = glyph_future_prob(gid, 0, g->future_token[0][i]);
        if (p > max_near) max_near = p;
    }
    float near_exact = glyph_future_prob(gid, 0, candidate) /
                       (max_near + 1e-9f);
    float near_semantic = 0.5f;
    if (vec_norm(g->future_emb_sum[0], EMBED_DIM) > 1e-6f)
        near_semantic =
            0.5f * (1.0f + cosine(vocab[candidate].emb,
                                  g->future_emb_sum[0], EMBED_DIM));
    float near = 0.58f * near_exact + 0.42f * near_semantic;

    float clause = 0.55f * glyph_future_overlap(gid, 1, candidate, 0) +
                   0.45f * glyph_future_overlap(gid, 1, candidate, 1);
    float discourse =
        0.62f * glyph_future_overlap(gid, 2, candidate, 2) +
        0.38f * glyph_future_overlap(gid, 2, candidate, 1);
    float action = glyph_action_score(gid, candidate);

    float raw = 0.39f * near + 0.23f * clause +
                0.13f * discourse + 0.25f * action;

    /*
     * Earned semantic voice.
     *
     * A causal abstraction does not gain authority merely by existing for a
     * long time.  Its online, prequential prediction must first beat the
     * corpus-wide future prior.  Until then the glyph is remembered and
     * trained, but its vote is exactly neutral.
     */
    float authority = glyph_semantic_authority(g);

    return clampf(0.5f + authority * (raw - 0.5f),
                  0.0f, 1.0f);
}


static float glyph_candidate_score(int gid, int candidate) {
    float own = glyph_candidate_score_single(gid, candidate);
    if (!neighbor_enabled || glyph_mode != GLYPH_CAUSAL ||
        gid < 0 || gid >= MAX_GLYPHS ||
        !causal_glyphs[gid].active)
        return own;

    PlasticityRule rule = plasticity_effective_rule();
    float total = 1.0f;
    float value = own;

    for (int n = 0; n < MAX_GLYPHS; ++n) {
        uint16_t age = glyph_link_age[gid][n];
        if (!age || !causal_glyphs[n].active) continue;

        float similarity =
            1.0f - glyph_pair_distance(&causal_glyphs[gid],
                                       &causal_glyphs[n]);
        float age_weight =
            expf(-(float)(age - 1) / 24.0f);
        float w = rule.neighbor_gain * age_weight *
                  clampf(similarity, 0.0f, 1.0f);
        if (w <= 0.002f) continue;

        float neighbor =
            glyph_candidate_score_single(n, candidate);
        value += w * neighbor;
        total += w;
    }
    return clampf(value / total, 0.0f, 1.0f);
}

static void glyph_merge_into(int keep, int remove) {
    if (keep < 0 || remove < 0 || keep == remove) return;
    CausalGlyph *a = &causal_glyphs[keep];
    CausalGlyph *b = &causal_glyphs[remove];
    if (!a->active || !b->active) return;

    float wa = (float)(a->uses + 1);
    float wb = (float)(b->uses + 1);
    float total = wa + wb;
    float alpha = wb / total;

    for (int h = 0; h < PROPHECY_HORIZONS; ++h) {
        for (int i = 0; i < a->sig_n[h]; ++i)
            a->sig_prob[h][i] *= (1.0f - alpha);
        for (int i = 0; i < b->sig_n[h]; ++i)
            glyph_sparse_add(a->sig_token[h], a->sig_prob[h],
                             &a->sig_n[h], b->sig_token[h][i],
                             alpha * b->sig_prob[h][i], GLYPH_TOPK);
        glyph_sparse_normalize(a->sig_prob[h], a->sig_n[h]);
        a->sig_debt[h] =
            (wa * a->sig_debt[h] + wb * b->sig_debt[h]) / total;
        a->sig_conf[h] =
            (wa * a->sig_conf[h] + wb * b->sig_conf[h]) / total;

        for (int i = 0; i < b->future_n[h]; ++i)
            glyph_future_add(a, h, b->future_token[h][i],
                             b->future_weight[h][i]);
    }

    for (int i = 0; i < b->action_n; ++i) {
        int slot = glyph_action_slot(a, b->action_token[i], 1);
        float visits_a = (float)a->action_visits[slot];
        float visits_b = (float)b->action_visits[i];
        float denom = visits_a + visits_b;
        if (denom > 0.0f) {
            a->action_quote[slot] =
                (visits_a * a->action_quote[slot] +
                 visits_b * b->action_quote[i]) / denom;
            a->action_debt[slot] =
                (visits_a * a->action_debt[slot] +
                 visits_b * b->action_debt[i]) / denom;
        }
        a->action_visits[slot] += b->action_visits[i];
    }

    for (int d = 0; d < EMBED_DIM; ++d)
        a->context_centroid[d] =
            (wa * a->context_centroid[d] +
             wb * b->context_centroid[d]) / total;
    normalize(a->context_centroid, EMBED_DIM);
    a->predictive_gain_ema =
        (wa * a->predictive_gain_ema +
         wb * b->predictive_gain_ema) / total;
    a->progress_ema =
        (wa * a->progress_ema + wb * b->progress_ema) / total;
    a->uses += b->uses;
    a->generation =
        (uint8_t)((a->generation > b->generation ?
                   a->generation : b->generation) + 1);
    a->parent_a = (int16_t)keep;
    a->parent_b = (int16_t)remove;

    if (neighbor_enabled) {
        for (int n = 0; n < MAX_GLYPHS; ++n) {
            if (n == keep || n == remove) continue;
            uint16_t age = glyph_link_age[remove][n];
            if (!age) continue;
            uint16_t existing = glyph_link_age[keep][n];
            uint16_t merged_age =
                existing && existing < age ? existing : age;
            glyph_link_age[keep][n] = merged_age;
            glyph_link_age[n][keep] = merged_age;
        }
        glyph_topology_clear(remove);
    }

    memset(b, 0, sizeof(*b));
    glyph_count--;
    glyph_merges++;
}

static void glyph_maintenance(void) {
    if (glyph_mode != GLYPH_CAUSAL || glyph_count < 12) return;
    int ai = -1, bi = -1;
    float best = 1.0f;
    for (int a = 0; a < MAX_GLYPHS; ++a) {
        if (!causal_glyphs[a].active || causal_glyphs[a].uses < 8) continue;
        for (int b = a + 1; b < MAX_GLYPHS; ++b) {
            if (!causal_glyphs[b].active || causal_glyphs[b].uses < 8) continue;
            float d = glyph_pair_distance(&causal_glyphs[a],
                                          &causal_glyphs[b]);
            if (d < best) {
                best = d;
                ai = a;
                bi = b;
            }
        }
    }
    if (ai >= 0 && best < 0.055f) {
        int keep = causal_glyphs[ai].uses >= causal_glyphs[bi].uses ?
                   ai : bi;
        int remove = keep == ai ? bi : ai;
        glyph_merge_into(keep, remove);
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
    build_future_fields();

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


static PlasticityRule plasticity_baseline(void) {
    PlasticityRule r;
    r.readout_lr = 0.025f;
    r.input_lr = 0.00035f;
    r.recurrent_lr = 0.00008f;
    r.surprise_threshold = 0.04f;
    r.surprise_width = 0.24f;
    r.error_clip = 0.20f;
    r.input_decay = 0.999999f;
    r.recurrent_decay = 0.9999995f;
    r.debt_modulation = 0.35f;
    r.embedding_lr = 0.008f;
    r.quote_rate = 0.10f;
    r.debt_rate = 0.06f;
    r.neighbor_gain = 0.12f;
    return r;
}

/* One immutable local rule. The evolutionary population that once blended
   into this value earned zero authority across nine generations on three
   seeds and has left main. */
static PlasticityRule plasticity_effective_rule(void) {
    return baseline_plasticity_rule;
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
static float global_ngram_freshness(const int *ctx, int ctx_n,
                                    int candidate);
static float counterfactual_rollout_score(const int *ctx, int ctx_n,
                                          int candidate,
                                          const float *intent,
                                          float current_intent_debt,
                                          const ProphecyStack *world);

static ScoreVector observe_score(const int *ctx, int ctx_n, int candidate,
                                 int oracle, int truth,
                                 const float *intent,
                                 float intent_debt,
                                 const ProphecyStack *world,
                                 int glyph_id) {
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
    s.search_policy = context_policy_score(ctx, ctx_n, candidate);
    s.causal_glyph = glyph_candidate_score(glyph_id, candidate);
    prophecy_stack_preview(world, candidate,
                           &s.prophecy_fulfillment,
                           &s.world_state_stability, NULL);

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
        counterfactual_rollout_score(ctx, ctx_n, candidate, intent, intent_debt, world);
    s.rollout_stability =
        0.45f * immediate_future + 0.55f * imagined_future;

    int rep = repeated_recently(ctx, ctx_n, candidate);
    float cycle = cycle_freshness(ctx, ctx_n, candidate);
    float semantic_cycle =
        semantic_cycle_freshness(ctx, ctx_n, candidate);
    float global_fresh =
        global_ngram_freshness(ctx, ctx_n, candidate);
    s.anti_repetition =
        (1.0f / (1.0f + (float)rep)) *
        cycle * semantic_cycle * global_fresh;
    s.novelty *=
        (0.65f + 0.35f * cycle) *
        (0.55f + 0.45f * semantic_cycle) *
        (0.55f + 0.45f * global_fresh);

    if (!token_is_content_id(candidate)) {
        s.source_grounding *= 0.25f;
        s.oracle_parity *= 0.25f;
        s.semantic_continuity *= 0.10f;
        s.intent_fidelity *= 0.10f;
        s.search_policy *= 0.10f;
        s.causal_glyph *= 0.10f;
        s.prophecy_fulfillment *= 0.10f;
        s.world_state_stability *= 0.10f;
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
    out[5] = s.search_policy;
    out[6] = s.causal_glyph;
    out[7] = s.prophecy_fulfillment;
    out[8] = s.world_state_stability;
    out[9] = s.novelty;
    out[10] = s.rollout_stability;
    out[11] = s.anti_repetition;
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


static uint32_t global_ngram_hash(const int *ctx, int ctx_n, int candidate) {
    uint32_t h = 2166136261u;
    int start = ctx_n - 3;
    if (start < 0) start = 0;
    for (int i = start; i < ctx_n; ++i) {
        h ^= (uint32_t)(ctx[i] + 0x9e3779b9u);
        h *= 16777619u;
    }
    h ^= (uint32_t)(candidate + 0x85ebca6bu);
    h *= 16777619u;
    return h & (PHRASE_TABLE - 1);
}

static float global_ngram_freshness(const int *ctx, int ctx_n, int candidate) {
    uint32_t c = global_ngram_counts[
        global_ngram_hash(ctx, ctx_n, candidate)];
    return 1.0f / sqrtf(1.0f + (float)c);
}

static void global_ngram_remember(const int *ctx, int ctx_n, int candidate) {
    uint32_t h = global_ngram_hash(ctx, ctx_n, candidate);
    if (global_ngram_counts[h] < UINT32_MAX)
        global_ngram_counts[h]++;
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
                                   const float *intent,
                                   const ProphecyStack *world) {
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
    float global_fresh =
        global_ngram_freshness(ctx, ctx_n, candidate);
    float market = 0.5f + 0.5f * learned_relation_score(prev, candidate);
    float fulfillment = 0.5f, stability = 0.5f;
    prophecy_stack_preview(world, candidate, &fulfillment, &stability, NULL);

    return 0.22f * evidence +
           0.14f * direction +
           0.18f * intent_score +
           0.14f * fulfillment +
           0.12f * stability +
           0.06f * cycle +
           0.04f * semantic_cycle +
           0.04f * global_fresh +
           0.08f * market;
}

static int simulated_policy_next(const int *ctx, int ctx_n,
                                 const float *intent,
                                 const ProphecyStack *world) {
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
            float value = simulated_token_value(ctx, ctx_n, tok, intent, world);
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
        float value = simulated_token_value(ctx, ctx_n, tok, intent, world);
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
                                          float current_intent_debt,
                                          const ProphecyStack *world) {
    int sim_ctx[CONTEXT + 5];
    int keep = ctx_n < CONTEXT ? ctx_n : CONTEXT;
    memcpy(sim_ctx, &ctx[ctx_n - keep], keep * sizeof(int));
    int sim_n = keep;

    float sim_intent[EMBED_DIM];
    memcpy(sim_intent, intent, sizeof(sim_intent));
    float sim_debt = current_intent_debt;
    ProphecyStack sim_world;
    if (world) sim_world = *world;
    else memset(&sim_world, 0, sizeof(sim_world));

    float total = simulated_token_value(sim_ctx, sim_n, candidate,
                                        sim_intent, &sim_world);
    float worst_cycle = cycle_freshness(sim_ctx, sim_n, candidate);
    sim_ctx[sim_n++] = candidate;
    intent_update(sim_intent, &sim_debt, candidate);
    prophecy_stack_step(&sim_world, candidate, NULL, NULL);

    const int horizon = 3;
    for (int step = 0; step < horizon; ++step) {
        int current_n = sim_n < CONTEXT ? sim_n : CONTEXT;
        int *current = &sim_ctx[sim_n - current_n];
        int next = simulated_policy_next(current, current_n,
                                         sim_intent, &sim_world);

        float value = simulated_token_value(
            current, current_n, next, sim_intent, &sim_world);
        float cyc = cycle_freshness(current, current_n, next);
        if (cyc < worst_cycle) worst_cycle = cyc;

        total += value;
        sim_ctx[sim_n++] = next;
        intent_update(sim_intent, &sim_debt, next);
        prophecy_stack_step(&sim_world, next, NULL, NULL);
    }

    float mean = total / (float)(horizon + 1);
    float world_debt = prophecy_stack_total_debt(&sim_world);
    return mean * (0.55f + 0.45f * worst_cycle) /
           (1.0f + 0.15f * sim_debt + 0.08f * world_debt);
}

static void prior_score(const int *ctx, int ctx_n, int candidate, int oracle,
                        const float *intent, const ProphecyStack *world,
                        int glyph_id, float out[SCORE_DIM]) {
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
    out[5] = context_policy_score(ctx, ctx_n, candidate);
    out[6] = glyph_candidate_score(glyph_id, candidate);
    prophecy_stack_preview(world, candidate, &out[7], &out[8], NULL);

    if (!content) {
        out[1] *= 0.20f;
        out[3] *= 0.15f;
        out[4] *= 0.10f;
        out[5] *= 0.10f;
        out[6] *= 0.10f;
        out[7] *= 0.10f;
        out[8] *= 0.10f;
    }

    float freq = (float)vocab[candidate].count / (float)(corpus_n + 1);
    float lexical_novelty = clampf(1.0f - 20.0f * freq, 0.0f, 1.0f);
    float phrase_novelty = phrase_freshness(ctx, ctx_n, candidate);
    float basin_novelty = semantic_basin_freshness(vocab[candidate].emb);
    float cycle = cycle_freshness(ctx, ctx_n, candidate);
    float semantic_cycle =
        semantic_cycle_freshness(ctx, ctx_n, candidate);
    float global_fresh =
        global_ngram_freshness(ctx, ctx_n, candidate);
    out[9] = lexical_novelty * phrase_novelty * basin_novelty *
             (0.65f + 0.35f * cycle) *
             (0.55f + 0.45f * semantic_cycle) *
             (0.55f + 0.45f * global_fresh);
    if (!content) out[9] *= 0.05f;

    int rollout_ctx[CONTEXT + 1];
    int rollout_n = ctx_n < CONTEXT ? ctx_n : CONTEXT;
    memcpy(rollout_ctx, &ctx[ctx_n - rollout_n],
           rollout_n * sizeof(int));
    rollout_ctx[rollout_n++] = candidate;
    int next = oracle_next_context(rollout_ctx, rollout_n);
    float mirror_future =
        0.5f * (1.0f + cosine(vocab[next].emb,
                              vocab[oracle].emb, EMBED_DIM));
    out[10] = mirror_future * (0.5f + 0.5f * basin_novelty);

    int rep = repeated_recently(ctx, ctx_n, candidate);
    out[11] = (1.0f / (1.0f + (float)rep)) *
              phrase_novelty * cycle * semantic_cycle * global_fresh;
    if (!content) out[11] *= 0.10f;
}



static float screening_utility(const float score[SCORE_DIM]) {
    return 0.10f * score[0] +
           0.06f * score[1] +
           0.07f * score[2] +
           0.11f * score[3] +
           0.08f * score[4] +
           0.11f * score[5] +
           0.10f * score[6] +
           0.12f * score[7] +
           0.11f * score[8] +
           0.05f * score[9] +
           0.05f * score[10] +
           0.04f * score[11];
}

static float survival_utility(const float score[SCORE_DIM]) {
    return 0.11f * score[0] +  /* local language physics */
           0.12f * score[1] +  /* lived source grounding */
           0.11f * score[2] +  /* coherence mirror parity */
           0.13f * score[3] +  /* semantic/order continuity */
           0.07f * score[4] +  /* discourse intent */
           0.10f * score[6] +  /* emergent causal state */
           0.13f * score[7] +  /* prophecy fulfillment */
           0.14f * score[8] +  /* world-state stability */
           0.09f * score[10];  /* imagined future */
}



static int choose_candidate(const int *ctx, int ctx_n, int oracle, int truth,
                            const float *intent, float intent_debt,
                            const ProphecyStack *world, int glyph_id,
                            int explore, ScoreVector *observed_out,
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
                (int)trigrams[t].b == prev &&
                (trigrams[t].count > 0 ||
                 (policy_enabled && trigrams[t].policy_visits > 0 &&
                  context_policy_score(ctx, ctx_n,
                                       (int)trigrams[t].c) > 0.53f)))
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
        prior_score(ctx, ctx_n, cand, oracle, intent, world, glyph_id, prior);
        core_predict_recursive(ctx_emb, intent, intent_debt,
                               prev, cand, oracle, prior, mlp, 0);

        for (int d = 0; d < SCORE_DIM; ++d) {
            float m = 0.5f * (mlp[d] + 1.0f);
            mix[d] = (1.0f - mlp_gate) * prior[d] + mlp_gate * m;
        }

        float world_debt = prophecy_stack_total_debt(world);
        float utility =
            screening_utility(mix) +
            0.09f * clampf(intent_debt, 0.0f, 1.5f) * mix[4] +
            0.08f * clampf(world_debt, 0.0f, 2.0f) * mix[7] +
            0.08f * mix[6];
        /* Global freshness is not a reward. It is applied only after
           predictive-survival finalists have been established. */
        float global_fresh =
            global_ngram_freshness(ctx, ctx_n, cand);
        (void)global_fresh;
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
        prior_score(ctx, ctx_n, fallback, oracle, intent, world, glyph_id, prior);
        core_predict_recursive(ctx_emb, intent, intent_debt,
                               prev, fallback, oracle, prior, mlp, 0);
        *observed_out =
            observe_score(ctx, ctx_n, fallback, oracle, truth,
                          intent, intent_debt, world, glyph_id);
        if (policy_enabled) {
            float fallback_target = 1.0f;
            policy_mark(ctx[ctx_n - 2], ctx[ctx_n - 1],
                        fallback, fallback_target);
            observed_out->search_policy = fallback_target;
        }
        memcpy(predicted_out, mlp, sizeof(mlp));
        return fallback;
    }

    /*
     * Only finalists receive expensive imagination. This is selective
     * test-time compute rather than exhaustive fantasy.
     */
    int best_idx = 0;
    float final_deep[FINALISTS][SCORE_DIM];
    float final_survival[FINALISTS];
    if (final_n > FINALISTS) final_n = FINALISTS;
    float max_survival = -1e30f;
    for (int i = 0; i < final_n; ++i) {
        memcpy(final_deep[i], final_mix[i], sizeof(final_deep[i]));
        float future = counterfactual_rollout_score(
            ctx, ctx_n, final_tok[i], intent, intent_debt, world);
        final_deep[i][10] = 0.58f * final_deep[i][10] + 0.42f * future;
        final_survival[i] = survival_utility(final_deep[i]);
        if (final_survival[i] > max_survival)
            max_survival = final_survival[i];
    }

    /* Coherence-constrained diversity: novelty may choose only among
       alternatives whose predictive survival is effectively equivalent. */
    float world_debt = prophecy_stack_total_debt(world);
    float margin = 0.032f / (0.80f + 0.35f * world_debt);
    float best_choice = -1e30f;
    for (int i = 0; i < final_n; ++i) {
        if (final_survival[i] + margin < max_survival)
            continue;
        float global_fresh =
            global_ngram_freshness(ctx, ctx_n, final_tok[i]);
        float choice =
            final_survival[i] +
            0.020f * final_deep[i][5] +
            0.018f * final_deep[i][6] +
            0.018f * final_deep[i][9] +
            ngram_freshness_weight * global_fresh +
            0.018f * final_deep[i][11];

        if (choice > best_choice) {
            best_choice = choice;
            best_idx = i;
        }
    }

    float policy_target[FINALISTS];
    policy_improve_context(ctx, ctx_n, final_tok, final_deep,
                           final_n, policy_target);

    int best = final_tok[best_idx];

    /* Prefer a viable content finalist over weak punctuation. */
    if (!token_is_content_id(best)) {
        for (int i = 0; i < final_n; ++i) {
            if (token_is_content_id(final_tok[i]) &&
                final_deep[i][0] > 0.30f) {
                best_idx = i;
                best = final_tok[i];
                break;
            }
        }
    }

    /* AlphaZero-style self-play exploration: search remains the teacher,
       but waking Netta sometimes samples its improved policy instead of
       taking argmax. Interactive generation stays greedy. */
    if (explore && policy_enabled && final_n > 1) {
        float exploration =
            0.18f * expf(-(float)episode_count / 2200.0f) + 0.035f;
        if (randf() < exploration) {
            float noisy[FINALISTS];
            float total = 0.0f;
            for (int i = 0; i < final_n; ++i) {
                noisy[i] = 0.94f * policy_target[i] +
                           0.06f / (float)final_n;
                total += noisy[i];
            }
            float r = randf() * total, acc = 0.0f;
            for (int i = 0; i < final_n; ++i) {
                acc += noisy[i];
                if (r <= acc) {
                    best_idx = i;
                    best = final_tok[i];
                    break;
                }
            }
        }
    }

    *observed_out =
        observe_score(ctx, ctx_n, best, oracle, truth,
                      intent, intent_debt, world, glyph_id);
    if (policy_enabled)
        observed_out->search_policy = policy_target[best_idx];
    memcpy(predicted_out, final_mlp[best_idx], SCORE_DIM * sizeof(float));
    return best;
}

static void learn_local(const int *ctx, int ctx_n, int chosen, int oracle,
                        const float *intent, float intent_debt,
                        const ProphecyStack *world, int glyph_id,
                        ScoreVector observed, const float predicted[SCORE_DIM]) {
    float target[SCORE_DIM];
    score_to_array(observed, target);
    PlasticityRule rule = plasticity_effective_rule();

    float ctx_emb[EMBED_DIM];
    context_embedding(ctx, ctx_n, ctx_emb);

    /* Recreate hidden activation and locally update only the readout.
       This is reward-modulated Hebbian learning, not backprop. */
    float dummy[SCORE_DIM];
    float local_prior[SCORE_DIM];
    int prev_for_depth = ctx[ctx_n - 1];
    prior_score(ctx, ctx_n, chosen, oracle, intent, world, glyph_id, local_prior);
    core_predict_recursive(ctx_emb, intent, intent_debt,
                           prev_for_depth, chosen, oracle,
                           local_prior, dummy, 1);

    float rate = rule.readout_lr;
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

    float surprise_width =
        rule.surprise_width > 1e-5f ? rule.surprise_width : 1e-5f;
    float surprise_gate =
        clampf((global_surprise - rule.surprise_threshold) /
               surprise_width, 0.0f, 1.0f);
    float modulation = clampf(
        global_error +
        rule.debt_modulation * (observed.world_state_stability - 0.5f),
        -rule.error_clip, rule.error_clip) * surprise_gate;
    float input_rate = rule.input_lr;
    float recurrent_rate = rule.recurrent_lr;

    for (int h = 0; h < HIDDEN_DIM; ++h) {
        for (int i = 0; i < input_n; ++i) {
            core.wxh[h][i] =
                rule.input_decay * core.wxh[h][i] +
                input_rate * modulation * core.hidden[h] * local_input[i];
        }
        for (int j = 0; j < HIDDEN_DIM; ++j) {
            core.whh[h][j] =
                rule.recurrent_decay * core.whh[h][j] +
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
            (1.0f - rule.quote_rate) * edges[e].quote +
            rule.quote_rate * signed_surprise;
        edges[e].debt =
            (1.0f - rule.debt_rate) * edges[e].debt +
            rule.debt_rate * debt_now;
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
    float erate = rule.embedding_lr * direction;

    if (agent_embedding_enabled) {
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
    }
    phrase_remember(ctx, ctx_n, chosen);
    global_ngram_remember(ctx, ctx_n, chosen);
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
                           const int *truth,
                           const int *oracle, const int *attempt, int n,
                           const ScoreVector *scores,
                           float avg_recursive_depth,
                           float world_debt_start,
                           float world_debt_end,
                           float avg_glyph_gain,
                           float avg_glyph_distance) {
    FILE *f = fopen("netta.history.tsv", "ab");
    if (!f) return;

    char ctx_text[512], truth_text[512], oracle_text[512], attempt_text[512];
    sequence_to_text(ctx, ctx_n, ctx_text, sizeof(ctx_text));
    /* A dialogue has no hidden truth: source_pos -1 marks a lived turn
       rather than a game against the text. */
    if (truth) sequence_to_text(truth, n, truth_text, sizeof(truth_text));
    else truth_text[0] = '\0';
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
        "%llu\t%d\t%s\t%s\t%s\t%s\t"
        "%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t"
        "%.5f\t%.5f\t%.5f\t%.5f\t%.5f\t%d\t%llu\t%llu\t%llu",
        (unsigned long long)ep, source_pos,
        ctx_text, truth_text, oracle_text, attempt_text,
        avg[0], avg[1], avg[2], avg[3], avg[4], avg[5], avg[6], avg[7],
        avg[8], avg[9], avg[10], avg[11], avg_recursive_depth,
        world_debt_start, world_debt_end,
        avg_glyph_gain, avg_glyph_distance,
        glyph_count,
        (unsigned long long)glyph_births,
        (unsigned long long)glyph_recycles,
        (unsigned long long)glyph_merges);
    fprintf(f,
        "\t%d\t%.5f\t%.5f\t%.5f\t%d\n",
        last_curriculum_region,
        last_curriculum_priority,
        last_curriculum_progress,
        last_episode_coherence,
        glyph_topology_edge_count());
    fclose(f);
}

/*
 * Two separate fingerprints of the glyph population. The source-predictive
 * one covers everything only real source trajectories are allowed to teach;
 * the experiential one covers action memory, which a counterfactual street
 * may legitimately move. Interaction must change the second and not the
 * first — that boundary is otherwise invisible.
 */
static uint64_t fnv_bytes(uint64_t h, const void *data, size_t n) {
    const unsigned char *p = (const unsigned char *)data;
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001B3ULL;
    }
    return h;
}

static void glyph_fingerprints(uint64_t *source_out, uint64_t *action_out) {
    uint64_t s = 0xCBF29CE484222325ULL;
    uint64_t a = 0xCBF29CE484222325ULL;

    s = fnv_bytes(s, &glyph_count, sizeof(glyph_count));
    s = fnv_bytes(s, &glyph_births, sizeof(glyph_births));
    s = fnv_bytes(s, &glyph_seed_promotions, sizeof(glyph_seed_promotions));

    for (int i = 0; i < MAX_GLYPHS; ++i) {
        const CausalGlyph *g = &causal_glyphs[i];
        s = fnv_bytes(s, &g->active, sizeof(g->active));
        if (!g->active) continue;
        s = fnv_bytes(s, g->sig_token, sizeof(g->sig_token));
        s = fnv_bytes(s, g->sig_prob, sizeof(g->sig_prob));
        s = fnv_bytes(s, g->sig_n, sizeof(g->sig_n));
        s = fnv_bytes(s, g->sig_debt, sizeof(g->sig_debt));
        s = fnv_bytes(s, g->sig_conf, sizeof(g->sig_conf));
        s = fnv_bytes(s, g->sig_future_emb, sizeof(g->sig_future_emb));
        s = fnv_bytes(s, g->future_token, sizeof(g->future_token));
        s = fnv_bytes(s, g->future_weight, sizeof(g->future_weight));
        s = fnv_bytes(s, g->future_n, sizeof(g->future_n));
        s = fnv_bytes(s, g->future_total, sizeof(g->future_total));
        s = fnv_bytes(s, g->future_sketch, sizeof(g->future_sketch));
        s = fnv_bytes(s, g->future_emb_sum, sizeof(g->future_emb_sum));
        s = fnv_bytes(s, g->context_centroid, sizeof(g->context_centroid));
        s = fnv_bytes(s, &g->uses, sizeof(g->uses));
        s = fnv_bytes(s, &g->predictive_gain_ema,
                      sizeof(g->predictive_gain_ema));
        s = fnv_bytes(s, &g->assignment_distance_ema,
                      sizeof(g->assignment_distance_ema));

        a = fnv_bytes(a, g->action_token, sizeof(g->action_token));
        a = fnv_bytes(a, g->action_quote, sizeof(g->action_quote));
        a = fnv_bytes(a, g->action_debt, sizeof(g->action_debt));
        a = fnv_bytes(a, g->action_visits, sizeof(g->action_visits));
        a = fnv_bytes(a, &g->action_n, sizeof(g->action_n));
    }

    if (source_out) *source_out = s;
    if (action_out) *action_out = a;
}

static volatile sig_atomic_t stop_requested = 0;

static void request_stop(int sig) {
    (void)sig;
    stop_requested = 1;
}

static size_t state_expected_size(uint32_t vocab_n, uint32_t edge_n,
                                  uint32_t tri_n) {
    return sizeof(StateHeader) +
           (size_t)vocab_n * sizeof(Token) +
           (size_t)edge_n * sizeof(Edge) +
           (size_t)tri_n * sizeof(TrigramEdge) +
           (size_t)PHRASE_TABLE * sizeof(uint32_t) * 2 +
           (size_t)BASIN_MEMORY * EMBED_DIM * sizeof(float) +
           (size_t)BASIN_MEMORY * sizeof(uint32_t) +
           (size_t)REPLAY_CAPACITY * sizeof(ReplayEpisode) +
           (size_t)MAX_GLYPHS * sizeof(CausalGlyph) +
           (size_t)GLYPH_NURSERY * sizeof(GlyphSeed) +
           (size_t)MAX_GLYPHS * MAX_GLYPHS * sizeof(uint16_t) +
           (size_t)MAX_CURRICULUM_REGIONS * sizeof(CurriculumRegion) +
           sizeof(PlasticityRule);
}

/* A snapshot becomes visible only once it is whole on disk. */
static void fsync_parent_dir(const char *path) {
    char dir[512];
    const char *slash = strrchr(path, '/');
    if (slash && slash != path) {
        size_t n = (size_t)(slash - path);
        if (n >= sizeof(dir)) n = sizeof(dir) - 1;
        memcpy(dir, path, n);
        dir[n] = '\0';
    } else {
        snprintf(dir, sizeof(dir), "%s", slash ? "/" : ".");
    }
    int fd = open(dir, O_RDONLY);
    if (fd >= 0) {
        fsync(fd);
        close(fd);
    }
}

static void save_state(const char *path) {
    char tmp[512];
    snprintf(tmp, sizeof(tmp), "%s.tmp", path);

    FILE *f = fopen(tmp, "wb");
    if (!f) {
        fprintf(stderr, "netta: cannot save state: %s\n", strerror(errno));
        return;
    }

    /* Alignment padding is part of the published bytes: leaving it
       uninitialised made the snapshot hash depend on stack residue. */
    StateHeader h;
    memset(&h, 0, sizeof(h));
    h.magic = STATE_MAGIC;
    h.version = STATE_VERSION;
    h.vocab_size = (uint32_t)vocab_size;
    h.edge_count = (uint32_t)edge_count;
    h.trigram_count = (uint32_t)trigram_count_total;
    h.episodes = episode_count;
    h.core = core;
    h.basin_count = basin_count;
    h.basin_cursor = basin_cursor;
    h.replay_count = replay_count;
    h.replay_cursor = replay_cursor;
    h.dream_cycles = dream_cycles;
    h.nrem_replays = nrem_replays;
    h.rem_replays = rem_replays;
    h.experiment_seed = experiment_seed;
    h.runtime_rng_state = rng_state;
    h.glyph_mode = (uint32_t)glyph_mode;
    h.glyph_count = (uint32_t)glyph_count;
    h.glyph_births = glyph_births;
    h.glyph_recycles = glyph_recycles;
    h.glyph_merges = glyph_merges;
    h.glyph_seed_births = glyph_seed_births;
    h.glyph_seed_promotions = glyph_seed_promotions;
    h.curriculum_count = (uint32_t)curriculum_count;
    h.curriculum_enabled = (uint32_t)curriculum_enabled;
    h.neighbor_enabled = (uint32_t)neighbor_enabled;

    int ok = 1;
    ok = ok && fwrite(&h, sizeof(h), 1, f) == 1;
    ok = ok && fwrite(vocab, sizeof(Token), (size_t)vocab_size, f) ==
               (size_t)vocab_size;
    ok = ok && fwrite(edges, sizeof(Edge), (size_t)edge_count, f) ==
               (size_t)edge_count;
    ok = ok && fwrite(trigrams, sizeof(TrigramEdge),
                      (size_t)trigram_count_total, f) ==
               (size_t)trigram_count_total;
    ok = ok && fwrite(phrase_counts, sizeof(uint32_t),
                      PHRASE_TABLE, f) == PHRASE_TABLE;
    ok = ok && fwrite(global_ngram_counts, sizeof(uint32_t),
                      PHRASE_TABLE, f) == PHRASE_TABLE;
    ok = ok && fwrite(basin_memory, sizeof(float),
                      BASIN_MEMORY * EMBED_DIM, f) ==
               BASIN_MEMORY * EMBED_DIM;
    ok = ok && fwrite(basin_uses, sizeof(uint32_t),
                      BASIN_MEMORY, f) == BASIN_MEMORY;
    ok = ok && fwrite(replay_buffer, sizeof(ReplayEpisode),
                      REPLAY_CAPACITY, f) == REPLAY_CAPACITY;
    ok = ok && fwrite(causal_glyphs, sizeof(CausalGlyph),
                      MAX_GLYPHS, f) == MAX_GLYPHS;
    ok = ok && fwrite(glyph_nursery, sizeof(GlyphSeed),
                      GLYPH_NURSERY, f) == GLYPH_NURSERY;
    ok = ok && fwrite(glyph_link_age, sizeof(uint16_t),
                      MAX_GLYPHS * MAX_GLYPHS, f) ==
               MAX_GLYPHS * MAX_GLYPHS;
    ok = ok && fwrite(curriculum_regions, sizeof(CurriculumRegion),
                      MAX_CURRICULUM_REGIONS, f) ==
               MAX_CURRICULUM_REGIONS;
    ok = ok && fwrite(&baseline_plasticity_rule,
                      sizeof(PlasticityRule), 1, f) == 1;

    if (ok) ok = fflush(f) == 0 && fsync(fileno(f)) == 0;
    if (fclose(f) != 0) ok = 0;

    if (!ok) {
        fprintf(stderr, "netta: state not published: %s\n", strerror(errno));
        remove(tmp);
        return;
    }
    if (rename(tmp, path) != 0) {
        fprintf(stderr, "netta: state rename failed: %s\n", strerror(errno));
        remove(tmp);
        return;
    }
    fsync_parent_dir(path);
}

static int load_state(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;

    StateHeader h;
    if (fread(&h, sizeof(h), 1, f) != 1 ||
        h.magic != STATE_MAGIC ||
        h.version != STATE_VERSION ||
        h.vocab_size != (uint32_t)vocab_size ||
        h.glyph_mode != (uint32_t)glyph_mode) {
        fclose(f);
        return 0;
    }

    /*
     * A truncated or foreign snapshot is refused before a single byte of
     * the living organism is overwritten.
     */
    long after_header = ftell(f);
    if (after_header < 0 || fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return 0;
    }
    long file_size = ftell(f);
    if (file_size < 0 ||
        (size_t)file_size != state_expected_size(h.vocab_size,
                                                 h.edge_count,
                                                 h.trigram_count) ||
        fseek(f, after_header, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }

    Token *saved =
        (Token *)malloc((size_t)h.vocab_size * sizeof(Token));
    if (!saved) {
        fclose(f);
        return 0;
    }
    if (fread(saved, sizeof(Token), h.vocab_size, f) != h.vocab_size) {
        free(saved);
        fclose(f);
        return 0;
    }

    /* Refuse state if vocabulary identity changed. */
    for (uint32_t i = 0; i < h.vocab_size; ++i) {
        if (strncmp(saved[i].text, vocab[i].text, MAX_TOKEN_LEN) != 0) {
            free(saved);
            fclose(f);
            return 0;
        }
    }

    if (h.edge_count > MAX_EDGES ||
        fread(edges, sizeof(Edge), h.edge_count, f) != h.edge_count) {
        free(saved);
        fclose(f);
        return 0;
    }

    edge_count = (int)h.edge_count;
    if (h.trigram_count > MAX_TRIGRAMS ||
        fread(trigrams, sizeof(TrigramEdge), h.trigram_count, f) !=
            h.trigram_count) {
        free(saved);
        fclose(f);
        return 0;
    }
    trigram_count_total = (int)h.trigram_count;
    episode_count = h.episodes;
    core = h.core;
    basin_count = h.basin_count;
    basin_cursor = h.basin_cursor;
    replay_count = h.replay_count;
    replay_cursor = h.replay_cursor;
    dream_cycles = h.dream_cycles;
    nrem_replays = h.nrem_replays;
    rem_replays = h.rem_replays;
    experiment_seed = h.experiment_seed;
    rng_state = h.runtime_rng_state;
    if (rng_state == 0) rng_state = 1;
    glyph_count = (int)h.glyph_count;
    glyph_births = h.glyph_births;
    glyph_recycles = h.glyph_recycles;
    glyph_merges = h.glyph_merges;
    glyph_seed_births = h.glyph_seed_births;
    glyph_seed_promotions = h.glyph_seed_promotions;
    curriculum_count = (int)h.curriculum_count;
    curriculum_enabled = (int)h.curriculum_enabled;
    neighbor_enabled = (int)h.neighbor_enabled;

    if (fread(phrase_counts, sizeof(uint32_t), PHRASE_TABLE, f) != PHRASE_TABLE ||
        fread(global_ngram_counts, sizeof(uint32_t), PHRASE_TABLE, f) !=
            PHRASE_TABLE ||
        fread(basin_memory, sizeof(float), BASIN_MEMORY * EMBED_DIM, f) !=
            BASIN_MEMORY * EMBED_DIM ||
        fread(basin_uses, sizeof(uint32_t), BASIN_MEMORY, f) != BASIN_MEMORY ||
        fread(replay_buffer, sizeof(ReplayEpisode),
              REPLAY_CAPACITY, f) != REPLAY_CAPACITY ||
        fread(causal_glyphs, sizeof(CausalGlyph),
              MAX_GLYPHS, f) != MAX_GLYPHS ||
        fread(glyph_nursery, sizeof(GlyphSeed),
              GLYPH_NURSERY, f) != GLYPH_NURSERY ||
        fread(glyph_link_age, sizeof(uint16_t),
              MAX_GLYPHS * MAX_GLYPHS, f) !=
              MAX_GLYPHS * MAX_GLYPHS ||
        fread(curriculum_regions, sizeof(CurriculumRegion),
              MAX_CURRICULUM_REGIONS, f) !=
              MAX_CURRICULUM_REGIONS ||
        fread(&baseline_plasticity_rule,
              sizeof(PlasticityRule), 1, f) != 1) {
        free(saved);
        fclose(f);
        return 0;
    }

    /* Learned geometry is applied last: every earlier refusal leaves the
       freshly built vocabulary untouched. */
    for (uint32_t i = 0; i < h.vocab_size; ++i) {
        memcpy(vocab[i].emb, saved[i].emb, sizeof(vocab[i].emb));
        memcpy(vocab[i].left_emb, saved[i].left_emb,
               sizeof(vocab[i].left_emb));
        memcpy(vocab[i].right_emb, saved[i].right_emb,
               sizeof(vocab[i].right_emb));
    }
    free(saved);

    /*
     * get_edge/get_trigram prepend newly-created entries. Rebuild in
     * ascending creation order so the final linked-list order matches the
     * uninterrupted process exactly. Descending reconstruction silently
     * reversed equal-probability iteration and changed oracle destiny after
     * a restart even with an identical RNG state.
     */
    memset(first_edge, 0xFF, sizeof(first_edge));
    for (int e = 0; e < edge_count; ++e) {
        int from = (int)edges[e].from;
        edges[e].next_from = first_edge[from];
        first_edge[from] = e;
    }

    memset(first_trigram, 0xFF, sizeof(first_trigram));
    for (int t = 0; t < trigram_count_total; ++t) {
        unsigned bucket = trigram_bucket((int)trigrams[t].a,
                                         (int)trigrams[t].b);
        trigrams[t].next_bucket = first_trigram[bucket];
        first_trigram[bucket] = t;
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
           "intent=%.3f policy=%.3f glyph=%.3f prophecy=%.3f world=%.3f "
           "novelty=%.3f rollout=%.3f anti-repeat=%.3f\n",
           a[0], a[1], a[2], a[3], a[4], a[5], a[6], a[7],
           a[8], a[9], a[10], a[11]);
}


static float trajectory_value(ScoreVector s) {
    return 0.13f * s.syntax_local +
           0.16f * s.source_grounding +
           0.12f * s.oracle_parity +
           0.15f * s.semantic_continuity +
           0.10f * s.intent_fidelity +
           0.09f * s.search_policy +
           0.09f * s.causal_glyph +
           0.10f * s.prophecy_fulfillment +
           0.10f * s.world_state_stability +
           0.03f * s.novelty +
           0.06f * s.rollout_stability +
           0.02f * s.anti_repetition;
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


static float replay_episode_priority(const TrajectoryStep *trace, int n,
                                     const float *intent_start,
                                     const float *intent_end,
                                     float world_debt_start,
                                     float world_debt_end) {
    float surprise = 0.0f;
    float debt = 0.0f;
    float novelty = 0.0f;

    for (int i = 0; i < n; ++i) {
        float target[SCORE_DIM];
        score_to_array(trace[i].immediate, target);

        for (int d = 0; d < SCORE_DIM; ++d) {
            float prophecy = 0.5f * (trace[i].predicted[d] + 1.0f);
            surprise += fabsf(target[d] - prophecy);
        }

        int e = find_edge(trace[i].prev, trace[i].chosen);
        if (e >= 0)
            debt += edges[e].debt + 0.5f * edges[e].volatility;

        novelty += trace[i].immediate.novelty;
    }

    surprise /= (float)(n * SCORE_DIM);
    debt /= (float)n;
    novelty /= (float)n;

    float unresolved_intent =
        0.5f * (1.0f - cosine(intent_start, intent_end, EMBED_DIM));

    float unresolved_world = clampf(
        world_debt_end + fmaxf(0.0f, world_debt_end - world_debt_start),
        0.0f, 2.0f) / 2.0f;
    return 0.28f * surprise +
           0.25f * clampf(debt / 2.0f, 0.0f, 1.0f) +
           0.17f * unresolved_intent +
           0.18f * unresolved_world +
           0.12f * novelty;
}

static void replay_store(const TrajectoryStep *trace, int n,
                         const float *intent_start,
                         const float *intent_end,
                         float world_debt_start,
                         float world_debt_end,
                         uint64_t episode) {
    int slot;
    if (replay_count < REPLAY_CAPACITY) {
        slot = replay_count++;
    } else {
        slot = replay_cursor;
        float weakest = 1e30f;
        for (int k = 0; k < 24; ++k) {
            int idx = (replay_cursor + k * 37) % REPLAY_CAPACITY;
            ReplayEpisode *r = &replay_buffer[idx];
            float age =
                (float)(episode_count > r->episode ?
                        episode_count - r->episode : 0);
            float retained =
                r->priority / (1.0f + 0.0008f * age + 0.12f * r->replays);
            if (retained < weakest) {
                weakest = retained;
                slot = idx;
            }
        }
        replay_cursor = (slot + 1) % REPLAY_CAPACITY;
    }

    ReplayEpisode *r = &replay_buffer[slot];
    memset(r, 0, sizeof(*r));
    memcpy(r->steps, trace, (size_t)n * sizeof(TrajectoryStep));
    memcpy(r->intent_start, intent_start, EMBED_DIM * sizeof(float));
    memcpy(r->intent_end, intent_end, EMBED_DIM * sizeof(float));
    r->episode = episode;
    r->world_debt_start = world_debt_start;
    r->world_debt_end = world_debt_end;
    r->priority = replay_episode_priority(
        trace, n, intent_start, intent_end,
        world_debt_start, world_debt_end);
    r->surprise = r->priority;
    r->valid = 1;
}

static int replay_select_priority(int exclude) {
    int best = -1;
    float best_score = -1e30f;

    for (int i = 0; i < replay_count; ++i) {
        if (i == exclude || !replay_buffer[i].valid) continue;

        ReplayEpisode *r = &replay_buffer[i];
        float age =
            (float)(episode_count > r->episode ?
                    episode_count - r->episode : 0);
        float freshness = 1.0f / sqrtf(1.0f + age / 256.0f);
        float scarcity = 1.0f / sqrtf(1.0f + (float)r->replays);
        float jitter = 0.92f + 0.16f * randf();
        float score = r->priority * freshness * scarcity * jitter;

        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

static int replay_select_related(const ReplayEpisode *anchor, int exclude) {
    int best = -1;
    float best_score = -1e30f;

    for (int i = 0; i < replay_count; ++i) {
        if (i == exclude || !replay_buffer[i].valid) continue;

        ReplayEpisode *r = &replay_buffer[i];
        float related =
            0.5f * (1.0f + cosine(anchor->intent_start,
                                  r->intent_start, EMBED_DIM));
        float different_ending =
            0.5f * (1.0f - cosine(anchor->intent_end,
                                  r->intent_end, EMBED_DIM));
        float score =
            0.58f * related +
            0.27f * different_ending +
            0.15f * r->priority +
            0.03f * randf();

        if (score > best_score) {
            best_score = score;
            best = i;
        }
    }
    return best;
}

static void nrem_consolidate(ReplayEpisode *r) {
    trajectory_revalue(r->steps, ROLLOUT);

    float dream_strength =
        clampf(r->priority / (1.0f + 0.18f * r->replays),
               0.0f, 1.0f);

    for (int i = 0; i < ROLLOUT; ++i) {
        TrajectoryStep *step = &r->steps[i];
        int e = get_edge(step->prev, step->chosen, 1);
        if (e < 0) continue;

        float value = trajectory_value(step->immediate);
        float signed_value = 2.0f * value - 1.0f;
        if (step->glyph_id >= 0)
            glyph_action_mark(step->glyph_id, step->chosen, value);

        edges[e].quote =
            0.985f * edges[e].quote +
            0.015f * dream_strength * signed_value;

        if (signed_value >= 0.0f)
            edges[e].support +=
                0.025f * dream_strength * signed_value;
        else
            edges[e].opposition +=
                0.035f * dream_strength * (-signed_value);

        float weakness =
            clampf(edges[e].debt + edges[e].volatility, 0.0f, 2.0f);
        float rate = 0.0012f * dream_strength * (0.4f + 0.6f * weakness);

        if (dream_embedding_enabled) {
            for (int d = 0; d < EMBED_DIM; ++d) {
                vocab[step->prev].right_emb[d] +=
                    rate * vocab[step->chosen].emb[d];
                vocab[step->chosen].left_emb[d] +=
                    rate * vocab[step->prev].emb[d];
            }
            normalize(vocab[step->prev].right_emb, EMBED_DIM);
            normalize(vocab[step->chosen].left_emb, EMBED_DIM);
        }
    }

    r->priority *= 0.84f;
    r->replays++;
    nrem_replays++;
}

static void rem_recombine(ReplayEpisode *a, ReplayEpisode *b) {
    TrajectoryStep hybrid[ROLLOUT];
    int cut = 2 + (int)(rng_u64() % (ROLLOUT - 3));

    for (int i = 0; i < cut; ++i)
        hybrid[i] = a->steps[i];

    for (int i = cut; i < ROLLOUT; ++i) {
        hybrid[i] = b->steps[i];

        int bridge_prev = hybrid[i - 1].chosen;
        int bridge_next = hybrid[i].chosen;

        float direction =
            directional_compatibility(bridge_prev, bridge_next);
        float semantic =
            0.5f * (1.0f + cosine(vocab[bridge_prev].emb,
                                  vocab[bridge_next].emb, EMBED_DIM));
        int e = get_edge(bridge_prev, bridge_next, 1);
        if (e >= 0) {
            float plausibility =
                0.62f * direction + 0.38f * semantic;

            if (plausibility > 0.62f) {
                edges[e].quote +=
                    0.018f * (plausibility - 0.62f);
                edges[e].debt *= 0.992f;
                edges[e].support +=
                    0.012f * (plausibility - 0.62f);
            } else {
                edges[e].opposition +=
                    0.020f * (0.62f - plausibility);
                edges[e].debt =
                    clampf(edges[e].debt +
                           0.015f * (0.62f - plausibility),
                           0.0f, 4.0f);
            }
        }
    }

    trajectory_revalue(hybrid, ROLLOUT);

    a->priority *= 0.93f;
    b->priority *= 0.95f;
    a->replays++;
    b->replays++;
    rem_replays++;
}

static void dream_replay_cycle(void) {
    if (replay_count < 32) return;

    dream_cycles++;

    for (int i = 0; i < NREM_DREAMS; ++i) {
        int idx = replay_select_priority(-1);
        if (idx >= 0)
            nrem_consolidate(&replay_buffer[idx]);
    }

    for (int i = 0; i < REM_DREAMS; ++i) {
        int a_idx = replay_select_priority(-1);
        if (a_idx < 0) break;
        int b_idx =
            replay_select_related(&replay_buffer[a_idx], a_idx);
        if (b_idx >= 0)
            rem_recombine(&replay_buffer[a_idx],
                          &replay_buffer[b_idx]);
    }
}


static float episode_external_coherence(const int *source_context,
                                        const int *truth,
                                        const int *attempt,
                                        const ScoreVector *scores,
                                        float debt_start,
                                        float debt_end,
                                        float *surprise_out,
                                        const TrajectoryStep *trace) {
    int chain[2 + ROLLOUT];
    chain[0] = source_context[CONTEXT - 2];
    chain[1] = source_context[CONTEXT - 1];
    for (int i = 0; i < ROLLOUT; ++i)
        chain[i + 2] = attempt[i];

    float bigram = 0.0f;
    int bigram_n = 0;
    for (int i = 0; i + 1 < 2 + ROLLOUT; ++i) {
        int e = find_edge(chain[i], chain[i + 1]);
        bigram += e >= 0 && edges[e].source_count > 0 ? 1.0f : 0.0f;
        bigram_n++;
    }
    if (bigram_n) bigram /= (float)bigram_n;

    float trigram = 0.0f;
    int trigram_n = 0;
    for (int i = 0; i + 2 < 2 + ROLLOUT; ++i) {
        trigram += source_trigram_count(
            chain[i], chain[i + 1], chain[i + 2]) > 0 ? 1.0f : 0.0f;
        trigram_n++;
    }
    if (trigram_n) trigram /= (float)trigram_n;

    float source = 0.0f, oracle_score = 0.0f;
    float semantic = 0.0f, prophecy = 0.0f;
    float world = 0.0f, rollout = 0.0f;
    float surprise = 0.0f;
    for (int i = 0; i < ROLLOUT; ++i) {
        source += scores[i].source_grounding;
        oracle_score += scores[i].oracle_parity;
        semantic += scores[i].semantic_continuity;
        prophecy += scores[i].prophecy_fulfillment;
        world += scores[i].world_state_stability;
        rollout += scores[i].rollout_stability;

        float target[SCORE_DIM];
        score_to_array(scores[i], target);
        for (int d = 0; d < SCORE_DIM; ++d) {
            float pred = 0.5f * (trace[i].predicted[d] + 1.0f);
            surprise += fabsf(target[d] - pred);
        }
    }
    source /= (float)ROLLOUT;
    oracle_score /= (float)ROLLOUT;
    semantic /= (float)ROLLOUT;
    prophecy /= (float)ROLLOUT;
    world /= (float)ROLLOUT;
    rollout /= (float)ROLLOUT;
    surprise /= (float)(ROLLOUT * SCORE_DIM);

    float first = attempt[0] == truth[0] ? 1.0f : 0.0f;
    float debt_progress =
        clampf(0.5f + 2.0f * (debt_start - debt_end),
               0.0f, 1.0f);

    float outcome =
        0.16f * first +
        0.13f * bigram +
        0.15f * trigram +
        0.13f * source +
        0.08f * oracle_score +
        0.10f * semantic +
        0.08f * prophecy +
        0.07f * world +
        0.05f * rollout +
        0.05f * debt_progress;

    if (surprise_out) *surprise_out = surprise;
    return clampf(outcome, 0.0f, 1.0f);
}


static void run_episode(int verbose) {
    int max_start = corpus_n - CONTEXT - 17;
    int source_pos =
        curriculum_source_position(episode_count, max_start);

    int seq[CONTEXT + ROLLOUT];
    int oracle[ROLLOUT];
    int attempt[ROLLOUT];
    ScoreVector scores[ROLLOUT];
    TrajectoryStep trace[ROLLOUT];
    float episode_emb[EMBED_DIM] = {0};
    float glyph_gain_sum = 0.0f;
    float glyph_distance_sum = 0.0f;
    int glyph_steps = 0;

    memcpy(seq, &corpus[source_pos], CONTEXT * sizeof(int));
    int seq_n = CONTEXT;

    float intent[EMBED_DIM];
    float intent_start[EMBED_DIM];
    ProphecyStack world;
    float intent_debt = 1.0f;
    intent_from_context(seq, CONTEXT, intent);
    memcpy(intent_start, intent, sizeof(intent_start));
    prophecy_stack_init(seq, CONTEXT, &world);
    float world_debt_start = prophecy_stack_total_debt(&world);

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

        float world_before = prophecy_stack_total_debt(&world);

        /*
         * Two realities, never silently mixed:
         *
         * source_state is the actual observed trajectory of the text island.
         * It alone may birth/update a causal glyph and teach its future.
         *
         * world is Netta's counterfactual trajectory after her own moves.
         * It may project into the learned glyph space and use its policy,
         * but it may never rewrite the world's predictive physics.
         */
        const int *source_ctx = &corpus[source_pos + step];
        ProphecyStack source_state;
        prophecy_stack_init(source_ctx, CONTEXT, &source_state);

        float glyph_distance = 0.0f;
        int source_glyph_id =
            glyph_assign(source_ctx, CONTEXT, &source_state,
                         &glyph_distance);
        float glyph_gain =
            glyph_predictive_gain(source_glyph_id, truth);

        int agent_glyph_id =
            glyph_lookup(ctx, ctx_n, &world, NULL);

        int chosen = choose_candidate(ctx, ctx_n, oracle_tok, truth,
                                      intent, intent_debt, &world,
                                      agent_glyph_id,
                                      1, &scores[step], pred);
        attempt[step] = chosen;

        trace[step].prev = ctx[ctx_n - 1];
        trace[step].chosen = chosen;
        trace[step].oracle = oracle_tok;
        trace[step].immediate = scores[step];
        trace[step].world_debt_before = world_before;
        trace[step].glyph_id = agent_glyph_id;
        trace[step].glyph_gain = glyph_gain;
        trace[step].glyph_distance = glyph_distance;
        memcpy(trace[step].predicted, pred, SCORE_DIM * sizeof(float));

        seq[seq_n++] = chosen;

        /*
         * Destiny is previewed before any plastic update. Every competing
         * plasticity rule therefore receives the same pre-action state and
         * the same observed consequence.
         */
        ProphecyStack world_next = world;
        prophecy_stack_step(&world_next, chosen, NULL, NULL);
        float world_after = prophecy_stack_total_debt(&world_next);
        float debt_progress = world_before - world_after;
        (void)debt_progress;

        learn_local(ctx, ctx_n, chosen, oracle_tok,
                    intent, intent_debt, &world, agent_glyph_id,
                    scores[step], pred);
        intent_update(intent, &intent_debt, chosen);
        world = world_next;
        trace[step].world_debt_after = world_after;

        float score_arr[SCORE_DIM];
        score_to_array(scores[step], score_arr);
        float debt_target =
            clampf(0.5f + 2.0f * debt_progress, 0.0f, 1.0f);
        float action_target = clampf(
            0.72f * survival_utility(score_arr) +
            0.28f * debt_target, 0.0f, 1.0f);
        glyph_action_mark(agent_glyph_id, chosen, action_target);

        int future_index = source_pos + CONTEXT + step;
        int future_n = corpus_n - future_index;
        glyph_observe_destiny(source_glyph_id,
                              &corpus[future_index], future_n,
                              glyph_gain, debt_progress);
        if (source_glyph_id >= 0) {
            glyph_gain_sum += glyph_gain;
            glyph_distance_sum += glyph_distance;
            glyph_steps++;
        }

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

    float world_debt_end = prophecy_stack_total_debt(&world);
    float episode_surprise = 0.0f;
    float episode_coherence =
        episode_external_coherence(
            &corpus[source_pos],
            &corpus[source_pos + CONTEXT],
            attempt, scores,
            world_debt_start, world_debt_end,
            &episode_surprise, trace);

    replay_store(trace, ROLLOUT, intent_start, intent,
                 world_debt_start, world_debt_end,
                 episode_count + 1);

    for (int d = 0; d < EMBED_DIM; ++d)
        episode_emb[d] /= (float)ROLLOUT;
    basin_remember(episode_emb);

    episode_count++;
    curriculum_update(source_pos, episode_coherence,
                      world_debt_end, episode_surprise);

    if (dreams_enabled && episode_count % DREAM_INTERVAL == 0)
        dream_replay_cycle();
    if (episode_count % DREAM_INTERVAL == 0)
        glyph_maintenance();

    float avg_glyph_gain =
        glyph_steps ? glyph_gain_sum / (float)glyph_steps : 0.0f;
    float avg_glyph_distance =
        glyph_steps ? glyph_distance_sum / (float)glyph_steps : 0.0f;

    uint64_t depth_used = recursive_depth_total - depth_start;
    uint64_t calls_used = recursive_call_total - calls_start;
    float avg_recursive_depth =
        calls_used ? (float)depth_used / (float)calls_used : 0.0f;

    history_append(episode_count, source_pos,
                   &corpus[source_pos], CONTEXT,
                   &corpus[source_pos + CONTEXT],
                   oracle, attempt, ROLLOUT, scores,
                   avg_recursive_depth,
                   world_debt_start, world_debt_end,
                   avg_glyph_gain, avg_glyph_distance);

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
        printf("\n  oracle: ");
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
        printf("  world debt: start=%.3f end=%.3f\n",
               world_debt_start, world_debt_end);
        printf("  curriculum: region=%d priority=%.3f progress=%.3f "
               "coherence=%.3f\n",
               last_curriculum_region,
               last_curriculum_priority,
               last_curriculum_progress,
               last_episode_coherence);
        printf("  plasticity: neighbor=%.3f\n",
               plasticity_effective_rule().neighbor_gain);
        printf("  glyph topology: edges=%d\n",
               glyph_topology_edge_count());
        printf("  glyphs: mode=%s active=%d births=%llu recycle=%llu merge=%llu "
               "gain=%.3f distance=%.3f\n",
               glyph_mode == GLYPH_CAUSAL ? "causal" :
               (glyph_mode == GLYPH_RANDOM ? "random" : "off"),
               glyph_count,
               (unsigned long long)glyph_births,
               (unsigned long long)glyph_recycles,
               (unsigned long long)glyph_merges,
               avg_glyph_gain, avg_glyph_distance);
        printf("  glyph nursery: seeds=%llu promotions=%llu\n",
               (unsigned long long)glyph_seed_births,
               (unsigned long long)glyph_seed_promotions);
        printf("  dreams: cycles=%llu nrem=%llu rem=%llu replay_memories=%d\n",
               (unsigned long long)dream_cycles,
               (unsigned long long)nrem_replays,
               (unsigned long long)rem_replays,
               replay_count);
    }
}


static void run_probe_suite(int count) {
    if (count <= 0) return;
    int max_start = corpus_n - CONTEXT - 17;
    if (max_start <= 0) return;

    Core saved_core = core;
    uint64_t saved_rng = rng_state;
    uint64_t saved_depth = recursive_depth_total;
    uint64_t saved_calls = recursive_call_total;
    int saved_eval = evaluation_mode;
    evaluation_mode = 1;

    FILE *f = fopen("netta.probe.tsv", "wb");
    if (f) {
        fputs("probe\tsource_pos\tfirst_token\ttoken_accuracy\texact_sequence\t"
              "bigram_validity\ttrigram_validity\tsource_grounding\t"
              "oracle_parity\tsemantic\tprophecy_fulfillment\t"
              "world_stability\trollout\tdebt_delta\tcoherence\n", f);
    }

    double sum_first = 0.0, sum_token = 0.0, sum_exact = 0.0;
    double sum_bigram = 0.0, sum_trigram = 0.0;
    double sum_source = 0.0, sum_oracle = 0.0, sum_semantic = 0.0;
    double sum_prophecy = 0.0, sum_world = 0.0, sum_rollout = 0.0;
    double sum_debt = 0.0, sum_coherence = 0.0;

    for (int probe = 0; probe < count; ++probe) {
        uint64_t position_hash = mix64(
            COHERENCE_PROBE_SEED ^
            ((uint64_t)probe + 1ULL) *
            0xD1B54A32D192ED03ULL);
        int source_pos =
            (int)(position_hash % (uint64_t)max_start);

        rng_state = mix64(
            COHERENCE_PROBE_SEED ^
            0xA24BAED4963EE407ULL ^
            (uint64_t)probe * 0x9FB21C651E98DF25ULL);
        if (!rng_state) rng_state = 1;

        int seq[CONTEXT + ROLLOUT];
        int attempt[ROLLOUT];
        ScoreVector scores[ROLLOUT];
        TrajectoryStep trace[ROLLOUT];

        memcpy(seq, &corpus[source_pos], CONTEXT * sizeof(int));
        int seq_n = CONTEXT;

        float intent[EMBED_DIM];
        float intent_debt = 1.0f;
        ProphecyStack world;
        intent_from_context(seq, CONTEXT, intent);
        prophecy_stack_init(seq, CONTEXT, &world);
        float debt_start = prophecy_stack_total_debt(&world);

        int oracle_seq[CONTEXT + ROLLOUT];
        memcpy(oracle_seq, seq, CONTEXT * sizeof(int));
        int oracle_seq_n = CONTEXT;

        for (int step = 0; step < ROLLOUT; ++step) {
            int truth = corpus[source_pos + CONTEXT + step];
            int oracle_ctx_n =
                oracle_seq_n < CONTEXT ? oracle_seq_n : CONTEXT;
            int oracle_tok = oracle_next_context(
                &oracle_seq[oracle_seq_n - oracle_ctx_n],
                oracle_ctx_n);
            oracle_seq[oracle_seq_n++] = oracle_tok;

            int ctx_n = seq_n < CONTEXT ? seq_n : CONTEXT;
            int *ctx = &seq[seq_n - ctx_n];
            float pred[SCORE_DIM];
            int gid = glyph_lookup(ctx, ctx_n, &world, NULL);

            int chosen = choose_candidate(
                ctx, ctx_n, oracle_tok, truth,
                intent, intent_debt, &world, gid,
                0, &scores[step], pred);
            attempt[step] = chosen;

            trace[step].prev = ctx[ctx_n - 1];
            trace[step].chosen = chosen;
            trace[step].oracle = oracle_tok;
            trace[step].immediate = scores[step];
            trace[step].world_debt_before =
                prophecy_stack_total_debt(&world);
            trace[step].glyph_id = gid;
            trace[step].glyph_gain = 0.0f;
            trace[step].glyph_distance = 0.0f;
            memcpy(trace[step].predicted, pred,
                   SCORE_DIM * sizeof(float));

            seq[seq_n++] = chosen;
            prophecy_stack_step(&world, chosen, NULL, NULL);
            trace[step].world_debt_after =
                prophecy_stack_total_debt(&world);
            intent_update(intent, &intent_debt, chosen);
        }

        float debt_end = prophecy_stack_total_debt(&world);
        float surprise = 0.0f;
        float coherence = episode_external_coherence(
            &corpus[source_pos],
            &corpus[source_pos + CONTEXT],
            attempt, scores, debt_start, debt_end,
            &surprise, trace);

        float first =
            attempt[0] == corpus[source_pos + CONTEXT] ? 1.0f : 0.0f;
        float token_acc = 0.0f;
        int exact = 1;
        for (int i = 0; i < ROLLOUT; ++i) {
            int same =
                attempt[i] == corpus[source_pos + CONTEXT + i];
            token_acc += same ? 1.0f : 0.0f;
            if (!same) exact = 0;
        }
        token_acc /= (float)ROLLOUT;

        int chain[2 + ROLLOUT];
        chain[0] = corpus[source_pos + CONTEXT - 2];
        chain[1] = corpus[source_pos + CONTEXT - 1];
        for (int i = 0; i < ROLLOUT; ++i)
            chain[i + 2] = attempt[i];

        float bigram = 0.0f;
        for (int i = 0; i + 1 < 2 + ROLLOUT; ++i) {
            int e = find_edge(chain[i], chain[i + 1]);
            bigram += e >= 0 && edges[e].source_count > 0 ?
                      1.0f : 0.0f;
        }
        bigram /= (float)(ROLLOUT + 1);

        float trigram = 0.0f;
        for (int i = 0; i + 2 < 2 + ROLLOUT; ++i)
            trigram += source_trigram_count(
                chain[i], chain[i + 1], chain[i + 2]) > 0 ?
                1.0f : 0.0f;
        trigram /= (float)ROLLOUT;

        float source = 0.0f, oracle_avg = 0.0f, semantic = 0.0f;
        float prophecy = 0.0f, world_avg = 0.0f, rollout = 0.0f;
        for (int i = 0; i < ROLLOUT; ++i) {
            source += scores[i].source_grounding;
            oracle_avg += scores[i].oracle_parity;
            semantic += scores[i].semantic_continuity;
            prophecy += scores[i].prophecy_fulfillment;
            world_avg += scores[i].world_state_stability;
            rollout += scores[i].rollout_stability;
        }
        source /= (float)ROLLOUT;
        oracle_avg /= (float)ROLLOUT;
        semantic /= (float)ROLLOUT;
        prophecy /= (float)ROLLOUT;
        world_avg /= (float)ROLLOUT;
        rollout /= (float)ROLLOUT;
        float debt_delta = debt_end - debt_start;

        sum_first += first;
        sum_token += token_acc;
        sum_exact += exact ? 1.0 : 0.0;
        sum_bigram += bigram;
        sum_trigram += trigram;
        sum_source += source;
        sum_oracle += oracle_avg;
        sum_semantic += semantic;
        sum_prophecy += prophecy;
        sum_world += world_avg;
        sum_rollout += rollout;
        sum_debt += debt_delta;
        sum_coherence += coherence;

        if (f) {
            fprintf(f,
                    "%d\t%d\t%.6f\t%.6f\t%d\t%.6f\t%.6f\t"
                    "%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t"
                    "%.6f\t%.6f\n",
                    probe + 1, source_pos, first, token_acc, exact,
                    bigram, trigram, source, oracle_avg, semantic,
                    prophecy, world_avg, rollout, debt_delta,
                    coherence);
        }

        core = saved_core;
    }

    if (f) fclose(f);

    double denom = (double)count;
    printf("\n[READ-ONLY COHERENCE PROBE] n=%d\n", count);
    printf("  first-token accuracy: %.4f\n", sum_first / denom);
    printf("  token accuracy:       %.4f\n", sum_token / denom);
    printf("  exact 8-token line:   %.4f\n", sum_exact / denom);
    printf("  corpus bigrams:       %.4f\n", sum_bigram / denom);
    printf("  corpus trigrams:      %.4f\n", sum_trigram / denom);
    printf("  source grounding:     %.4f\n", sum_source / denom);
    printf("  oracle parity:        %.4f\n", sum_oracle / denom);
    printf("  semantic continuity:  %.4f\n", sum_semantic / denom);
    printf("  prophecy fulfillment: %.4f\n", sum_prophecy / denom);
    printf("  world stability:      %.4f\n", sum_world / denom);
    printf("  rollout stability:    %.4f\n", sum_rollout / denom);
    printf("  world debt delta:     %.4f\n", sum_debt / denom);
    printf("  coherence outcome:    %.4f\n", sum_coherence / denom);
    printf("  probe ledger: netta.probe.tsv\n");

    core = saved_core;
    rng_state = saved_rng;
    recursive_depth_total = saved_depth;
    recursive_call_total = saved_calls;
    evaluation_mode = saved_eval;
}

/*
 * Prompt text is cut by the same physics as the corpus: punctuation is a
 * token, ASCII is lowered, UTF-8 survives. An unknown word is reported and
 * dropped — human input is an observation, never a new fact of the world.
 */
static int prompt_emit(const char *word, int *prompt, int *n, int *unknown) {
    if (word[0] == '\0') return 1;
    int id = token_id(word, 0);
    if (id < 0) {
        fprintf(stderr, "netta: unknown prompt token '%s' ignored\n", word);
        (*unknown)++;
        return 1;
    }
    if (*n >= CONTEXT) return 0;
    prompt[(*n)++] = id;
    return 1;
}

static int tokenize_prompt(const char *text, int *prompt, int *unknown) {
    char buf[MAX_TOKEN_LEN];
    int len = 0;
    int n = 0;

    for (const char *p = text; *p; ++p) {
        unsigned char c = (unsigned char)*p;

        if (isspace(c) || is_punct_token(c)) {
            buf[len] = '\0';
            if (len > 0 && !prompt_emit(buf, prompt, &n, unknown)) return n;
            len = 0;
            if (is_punct_token(c)) {
                char single[2] = {(char)c, '\0'};
                if (!prompt_emit(single, prompt, &n, unknown)) return n;
            }
            continue;
        }

        if (len < MAX_TOKEN_LEN - 1)
            buf[len++] = c < 128 ? (char)tolower(c) : (char)c;
    }

    buf[len] = '\0';
    if (len > 0) prompt_emit(buf, prompt, &n, unknown);
    return n;
}

static void interactive_prompt(const char *text) {
    int prompt[CONTEXT];
    int unknown = 0;
    int n = tokenize_prompt(text, prompt, &unknown);

    if (n == 0) {
        fprintf(stderr, "netta: prompt contains no known tokens\n");
        return;
    }
    if (unknown)
        fprintf(stderr, "netta: %d prompt token(s) unknown to this world\n",
                unknown);

    float intent[EMBED_DIM];
    float intent_debt = 1.0f;
    intent_from_context(prompt, n, intent);
    ProphecyStack world;
    prophecy_stack_init(prompt, n, &world);

    int generated[64];
    TrajectoryStep live_trace[ROLLOUT];
    int live_n = 0;
    memcpy(generated, prompt, n * sizeof(int));
    int gen_n = n;

    int oracle_seq[64];
    memcpy(oracle_seq, generated, gen_n * sizeof(int));
    int oracle_seq_n = gen_n;

    ScoreVector turn_scores[64];
    int turn_oracle[64];
    int turn_attempt[64];
    float turn_distance_sum = 0.0f;
    int turn_n = 0;
    float turn_debt_start = prophecy_stack_total_debt(&world);
    uint64_t turn_depth_start = recursive_depth_total;
    uint64_t turn_calls_start = recursive_call_total;

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
        float glyph_distance = 0.0f;
        /* A counterfactual street may ask the world what state it
           resembles; it may not teach the world a new one. */
        int glyph_id = glyph_lookup(live_ctx, ctx_n, &world,
                                    &glyph_distance);
        int chosen = choose_candidate(live_ctx, ctx_n,
                                      oracle, oracle, intent, intent_debt,
                                      &world, glyph_id, 0, &score, pred);

        live_trace[live_n].prev = live_ctx[ctx_n - 1];
        live_trace[live_n].chosen = chosen;
        live_trace[live_n].oracle = oracle;
        live_trace[live_n].immediate = score;
        live_trace[live_n].glyph_id = glyph_id;
        live_trace[live_n].glyph_gain = 0.0f;
        live_trace[live_n].glyph_distance = glyph_distance;
        memcpy(live_trace[live_n].predicted, pred,
               SCORE_DIM * sizeof(float));
        live_n++;

        if (turn_n < 64) {
            turn_scores[turn_n] = score;
            turn_oracle[turn_n] = oracle;
            turn_attempt[turn_n] = chosen;
            turn_distance_sum += glyph_distance;
            turn_n++;
        }

        generated[gen_n++] = chosen;
        float world_before = prophecy_stack_total_debt(&world);
        learn_local(live_ctx, ctx_n, chosen, oracle,
                    intent, intent_debt, &world, glyph_id, score, pred);
        intent_update(intent, &intent_debt, chosen);
        prophecy_stack_step(&world, chosen, NULL, NULL);
        float world_after = prophecy_stack_total_debt(&world);

        float score_arr[SCORE_DIM];
        score_to_array(score, score_arr);
        float action_target = clampf(
            0.72f * survival_utility(score_arr) +
            0.28f * clampf(0.5f + 2.0f *
                           (world_before - world_after), 0.0f, 1.0f),
            0.0f, 1.0f);
        glyph_action_mark(glyph_id, chosen, action_target);

        if (live_n == ROLLOUT) {
            trajectory_revalue(live_trace, live_n);
            live_n = 0;
        }
    }

    if (live_n > 0)
        trajectory_revalue(live_trace, live_n);

    /*
     * A conversation moves edges, embeddings and phrase memory exactly as a
     * game does, and is published into the state file. It therefore belongs
     * in the biography: without this line a lived turn changed the organism
     * with no record that it happened.
     */
    if (turn_n > 0) {
        uint64_t depth_used = recursive_depth_total - turn_depth_start;
        uint64_t calls_used = recursive_call_total - turn_calls_start;
        history_append(episode_count, -1,
                       prompt, n, NULL,
                       turn_oracle, turn_attempt, turn_n, turn_scores,
                       calls_used ? (float)depth_used / (float)calls_used
                                  : 0.0f,
                       turn_debt_start,
                       prophecy_stack_total_debt(&world),
                       0.0f,
                       turn_distance_sum / (float)turn_n);
    }

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
    int seed_given = 0;
    int probe_count = 0;
    int override_policy = -1;
    int override_stack = -1;
    int override_dream = -1;
    int override_neighbor = -1;
    int override_curriculum = -1;
    int override_glyph = -1;
    int glyph_hash_only = 0;

    for (int i = 2; i < argc; ++i) {
        if (strcmp(argv[i], "--steps") == 0 && i + 1 < argc)
            steps = strtol(argv[++i], NULL, 10);
        else if (strcmp(argv[i], "--prompt") == 0 && i + 1 < argc)
            prompt = argv[++i];
        else if (strcmp(argv[i], "--probe") == 0 && i + 1 < argc)
            probe_count = atoi(argv[++i]);
        else if (strcmp(argv[i], "--reset") == 0)
            reset = 1;
        else if (strcmp(argv[i], "--seed") == 0 && i + 1 < argc) {
            experiment_seed = strtoull(argv[++i], NULL, 10);
            seed_given = 1;
        } else if (strcmp(argv[i], "--no-policy") == 0)
            policy_enabled = 0;
        else if (strcmp(argv[i], "--no-agent-emb") == 0)
            agent_embedding_enabled = 0;
        else if (strcmp(argv[i], "--no-dream-emb") == 0)
            dream_embedding_enabled = 0;
        else if (strcmp(argv[i], "--ngram-weight") == 0 && i + 1 < argc)
            ngram_freshness_weight = strtof(argv[++i], NULL);
        else if (strcmp(argv[i], "--no-stack") == 0) {
            prophecy_stack_enabled = 0;
            override_stack = 0;
        }
        else if (strcmp(argv[i], "--no-dream") == 0) {
            dreams_enabled = 0;
            override_dream = 0;
        }
        else if (strcmp(argv[i], "--no-neighbor") == 0) {
            neighbor_enabled = 0;
            override_neighbor = 0;
        }
        else if (strcmp(argv[i], "--uniform-curriculum") == 0) {
            curriculum_enabled = 0;
            override_curriculum = 0;
        }
        else if (strcmp(argv[i], "--no-glyph") == 0) {
            glyph_mode = GLYPH_OFF;
            override_glyph = GLYPH_OFF;
        }
        else if (strcmp(argv[i], "--random-glyph") == 0) {
            glyph_mode = GLYPH_RANDOM;
            override_glyph = GLYPH_RANDOM;
        }
        else if (strcmp(argv[i], "--glyph-threshold") == 0 &&
                 i + 1 < argc)
            glyph_birth_threshold = strtof(argv[++i], NULL);
        else if (strcmp(argv[i], "--glyph-hash") == 0)
            glyph_hash_only = 1;
    }

    if (!prophecy_stack_enabled)
        glyph_mode = GLYPH_OFF;

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = request_stop;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    if (!seed_given)
        experiment_seed = (uint64_t)time(NULL);
    rng_state = mix64(experiment_seed ^ 0xA0761D6478BD642FULL);
    if (rng_state == 0) rng_state = 0x9E3779B97F4A7C15ULL;

    printf("NETTA — NETTA's Experiential Text Training Architecture\n");
    printf("sovereign recursive coherence game; no backpropagation\n");

    if (!load_corpus(corpus_path)) return 1;

    printf("corpus: %d tokens, %d vocabulary items "
           "(%.1f%% of capacity)\n",
           corpus_n, vocab_size,
           100.0 * (double)vocab_size / (double)MAX_VOCAB);
    if (vocab_size > (int)(0.90 * (double)MAX_VOCAB))
        fprintf(stderr,
                "netta: vocabulary above 90%% capacity; "
                "new words may become inedible\n");

    curriculum_init(corpus_n - CONTEXT - 17);
    build_source_graph();
    core_init();
    baseline_plasticity_rule = plasticity_baseline();

    int resumed = !reset && load_state("netta.state");
    if (resumed) {
        if (override_policy >= 0) policy_enabled = override_policy;
        if (override_stack >= 0) prophecy_stack_enabled = override_stack;
        if (override_dream >= 0) dreams_enabled = override_dream;
        if (override_neighbor >= 0) neighbor_enabled = override_neighbor;
        if (override_curriculum >= 0)
            curriculum_enabled = override_curriculum;
        if (override_glyph >= 0) glyph_mode = override_glyph;
        if (!prophecy_stack_enabled) glyph_mode = GLYPH_OFF;
        printf("state: resumed after %llu episodes\n",
               (unsigned long long)episode_count);
    } else {
        printf("state: new organism\n");
        FILE *f = fopen("netta.history.tsv", "wb");
        if (f) {
            fputs("episode\tsource_pos\tcontext\ttruth\toracle\tnetta\t"
                  "local\tsource\toracle_parity\tsemantic\tintent\tsearch_policy\t"
                  "causal_glyph\tprophecy_fulfillment\tworld_stability\tnovelty\t"
                  "rollout\tanti_repetition\tavg_recursive_depth\t"
                  "world_debt_start\tworld_debt_end\tglyph_gain\tglyph_distance\t"
                  "glyph_count\tglyph_births\tglyph_recycles\tglyph_merges\t"
                  "curriculum_region\tcurriculum_priority\tcurriculum_progress\t"
                  "episode_coherence\tglyph_topology_edges\n", f);
            fclose(f);
        }
    }

    printf("active seed=%llu search_policy=%s prophecy_stack=%s dreams=%s "
           "causal_glyphs=%s neighbors=%s curriculum=%s "
           "threshold=%.3f\n\n",
           (unsigned long long)experiment_seed,
           policy_enabled ? "on" : "off",
           prophecy_stack_enabled ? "on" : "off",
           dreams_enabled ? "on" : "off",
           glyph_mode == GLYPH_CAUSAL ? "causal" :
           (glyph_mode == GLYPH_RANDOM ? "random-control" : "off"),
           neighbor_enabled ? "on" : "off",
           curriculum_enabled ? "learning-progress" : "uniform",
           glyph_birth_threshold);

    if (glyph_hash_only) {
        uint64_t source_hash = 0, action_hash = 0;
        glyph_fingerprints(&source_hash, &action_hash);
        printf("glyph source-predictive hash: %016llx\n",
               (unsigned long long)source_hash);
        printf("glyph action-memory hash:     %016llx\n",
               (unsigned long long)action_hash);
        printf("glyphs=%d births=%llu promotions=%llu\n",
               glyph_count,
               (unsigned long long)glyph_births,
               (unsigned long long)glyph_seed_promotions);
        return 0;
    }

    if (prompt) {
        interactive_prompt(prompt);
        save_state("netta.state");
        if (probe_count > 0)
            run_probe_suite(probe_count);
        return 0;
    }

    if (steps < 0) {
        /* Sovereign mode: live until interrupted. */
        while (!stop_requested) {
            int verbose = (episode_count < 10 || episode_count % 100 == 0);
            run_episode(verbose);
            if (episode_count % 100 == 0)
                save_state("netta.state");
        }
        save_state("netta.state");
        printf("\ncomplete: %llu lifetime episodes\n",
               (unsigned long long)episode_count);
        printf("memory: netta.state\nledger: netta.history.tsv\n");
        return 0;
    }

    for (long i = 0; i < steps && !stop_requested; ++i) {
        int verbose = (i < 5 || (i + 1) % 100 == 0);
        run_episode(verbose);
        if ((i + 1) % 100 == 0)
            save_state("netta.state");
    }

    save_state("netta.state");
    if (probe_count > 0)
        run_probe_suite(probe_count);
    printf("\ncomplete: %llu lifetime episodes\n",
           (unsigned long long)episode_count);
    printf("memory: netta.state\nledger: netta.history.tsv\n");
    return 0;
}
