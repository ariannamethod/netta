#define _POSIX_C_SOURCE 200809L

/*
 * NETTA ZERO
 *
 * An immutable byte tape is the world. A byte is the irreducible action.
 * Body 2 adds an earned vocabulary: recurring lived byte sequences are
 * promoted into named units (online BPE over the lived tape), and a unit
 * never retokenizes the world -- it coexists with its atomic expansion.
 *
 * Unit law, declared before this code was written:
 *   - Netta always samples atomic bytes; a unit peeks at no future and
 *     receives no bit of discount (Z3: an equivalent path has identical
 *     per-byte loss -- here identically, because atomic receipts are the
 *     only judged receipts);
 *   - a unit is recognition over the lived truth tape: when consecutive
 *     lived truth bytes equal a living unit (greedy longest match), one
 *     additional macro line enters the biography; atomic lines are never
 *     altered or replaced;
 *   - growth is BPE over lived moves: adjacent-move pairs are counted,
 *     a pair lived >= 64 times births the concatenated unit if its bytes
 *     are exact, length <= 16, and it contains no whitespace byte (first
 *     version boundary discipline);
 *   - unit identity is its bytes; two birth paths cannot create two
 *     units; pairs never span an episode boundary;
 *   - all judgment stays in raw bytes.
 *
 *   cc -O2 -std=c11 -Wall -Wextra -Wpedantic netta.c -lm -o netta
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include <unistd.h>

#define CTX          16
#define MAX_ISLANDS  32     /* islands present in one convoy */
#define MAX_REGISTRY 1024   /* islands one life may ever meet */
#define ACTIONS      256
#define STATE_MAGIC  "NETTAZR0"
#define STATE_VER    20u

#define MAX_UNITS      4096
#define UNIT_MAX_LEN   16
#define BIRTH_SUPPORT  64
#define UNIT_TTL       16384   /* lived bytes without recognition = death
                                  (first-version rent discipline) */
#define PAIR_SLOTS     (1u << 18)

/* ---------------------------------------------------------------- hash */

static uint64_t fnv1a64(const uint8_t *p, uint64_t n, uint64_t h) {
    for (uint64_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}
#define FNV_SEED 0xcbf29ce484222325ULL
#define FNV_WITNESS_SEED 0x6c62272e07bb0142ULL

static uint64_t fnv1a64_reverse(const uint8_t *p, uint64_t n, uint64_t h) {
    while (n) {
        h ^= p[--n];
        h *= 0x100000001b3ULL;
    }
    return h;
}

/* ----------------------------------------------------------------- rng */

static uint64_t rng_state;

static uint64_t rng_next(void) {
    uint64_t z = (rng_state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

/* --------------------------------------------------------------- world */

typedef struct {
    uint8_t *bytes;
    uint64_t len;
    uint64_t digest;
    uint64_t witness;
    char     name[256];
} Island;

static Island islands[MAX_ISLANDS];
static int island_count = 0;
static uint64_t reg_digest[MAX_REGISTRY];
static uint64_t reg_witness[MAX_REGISTRY];
static uint64_t reg_len[MAX_REGISTRY];
static int reg_count = 0;
static uint64_t episode_no;   /* forward: persisted state below */
static int fixed_start_set = 0;
static uint64_t fixed_start = 0;

static void island_load(const char *path) {
    if (island_count >= MAX_ISLANDS) {
        fprintf(stderr, "netta: too many islands (max %d)\n", MAX_ISLANDS);
        exit(1);
    }
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "netta: cannot open %s\n", path); exit(1); }
    if (fseek(f, 0, SEEK_END) != 0) {
        fprintf(stderr, "netta: cannot seek %s\n", path); exit(1);
    }
    long sz = ftell(f);
    if (sz < 0) { fprintf(stderr, "netta: cannot size %s\n", path); exit(1); }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "netta: cannot rewind %s\n", path); exit(1);
    }
    Island *isl = &islands[island_count];
    isl->len = (uint64_t)sz;
    isl->bytes = (uint8_t *)malloc(isl->len ? isl->len : 1);
    if (!isl->bytes) { fprintf(stderr, "netta: oom\n"); exit(1); }
    if (isl->len && fread(isl->bytes, 1, isl->len, f) != isl->len) {
        fprintf(stderr, "netta: short read on %s\n", path); exit(1);
    }
    fclose(f);
    isl->digest = fnv1a64(isl->bytes, isl->len, FNV_SEED);
    isl->witness = fnv1a64_reverse(isl->bytes, isl->len,
                                   FNV_WITNESS_SEED);
    snprintf(isl->name, sizeof isl->name, "%s", path);
    island_count++;
}

/* ----------------------------------------------------------- biography */

static FILE   *bio_file;
static uint64_t bio_chain = FNV_SEED;
static uint64_t bio_lines = 0;

static void bio_verify(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "netta: biography %s is missing; refusing resume\n",
                path);
        exit(1);
    }
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    uint64_t chain = FNV_SEED, lines = 0;
    int arrivals = 0, malformed = 0;
    while ((n = getline(&line, &cap, f)) >= 0) {
        chain = fnv1a64((const uint8_t *)line, (uint64_t)n, chain);
        lines++;
        if (n == 0 || line[n - 1] != '\n') malformed = 1;
        if (n >= 2 && line[0] == 'i' && line[1] == '\t') {
            unsigned long long ep, digest, witness, len;
            int id, used = -1;
            if (sscanf(line, "i\t%llu\t%d\t%16llx\t%16llx\t%llu%n",
                       &ep, &id, &digest, &witness, &len, &used) != 5 ||
                (ssize_t)used != n - 1 || id != arrivals || id < 0 ||
                id >= reg_count || ep > episode_no ||
                digest != reg_digest[id] ||
                witness != reg_witness[id] || len != reg_len[id])
                malformed = 1;
            arrivals++;
        }
    }
    free(line);
    if (ferror(f) || fclose(f) != 0) {
        fprintf(stderr, "netta: cannot verify biography %s\n", path);
        exit(1);
    }
    if (malformed || arrivals != reg_count) {
        fprintf(stderr,
                "netta: biography %s does not conserve the island registry; "
                "refusing\n", path);
        exit(1);
    }
    if (chain != bio_chain || lines != bio_lines) {
        fprintf(stderr,
                "netta: biography %s does not match state "
                "(lines %llu/%llu, chain %016llx/%016llx); refusing\n",
                path, (unsigned long long)lines,
                (unsigned long long)bio_lines,
                (unsigned long long)chain,
                (unsigned long long)bio_chain);
        exit(1);
    }
}

static void bio_open(const char *path, int reset) {
    bio_file = fopen(path, reset ? "wb" : "ab");
    if (!bio_file) {
        fprintf(stderr, "netta: cannot open biography %s\n", path);
        exit(1);
    }
}

static void bio_append(const char *line) {
    size_t n = strlen(line);
    if (fwrite(line, 1, n, bio_file) != n) {
        fprintf(stderr, "netta: biography write failed\n"); exit(1);
    }
    bio_chain = fnv1a64((const uint8_t *)line, (uint64_t)n, bio_chain);
    bio_lines++;
}

/* --------------------------------------------------------------- units */

typedef struct {
    uint8_t  bytes[UNIT_MAX_LEN];
    uint32_t len;
    uint64_t born_episode;
    uint64_t support_at_birth;
    uint64_t uses;
    uint64_t last_use;
    uint64_t dead;
} Unit;

static Unit units[MAX_UNITS];
static int  unit_count = 0;
static int  unit_living = 0;
static uint64_t births_rejected_cap = 0;
static uint64_t unit_deaths = 0;
static void live_mass_add_unit(int u);
static void live_mass_remove_unit(int u);

typedef struct { uint64_t key; uint64_t cnt; } Pair;
static Pair *pairs;              /* PAIR_SLOTS, heap */
static uint64_t pair_used = 0;

static int units_enabled = 1;

static int is_ws(uint8_t b) {
    return b == 0x20 || b == 0x09 || b == 0x0a || b == 0x0d;
}

/* move id: 0..255 atomic byte, 256+k unit k */
static uint32_t move_len(uint32_t m) {
    return m < ACTIONS ? 1u : units[m - ACTIONS].len;
}
static const uint8_t *move_bytes(uint32_t m, uint8_t *tmp) {
    if (m < ACTIONS) { tmp[0] = (uint8_t)m; return tmp; }
    return units[m - ACTIONS].bytes;
}

static int unit_find(const uint8_t *b, uint32_t len) {
    /* identity scan: the dead keep their name so no second unit can
       ever be born with the same bytes */
    for (int u = 0; u < unit_count; ++u)
        if (units[u].len == len && memcmp(units[u].bytes, b, len) == 0)
            return u;
    return -1;
}

static int unit_find_living(const uint8_t *b, uint32_t len) {
    int u = unit_find(b, len);
    return (u >= 0 && !units[u].dead) ? u : -1;
}

static int unit_prefix_alive(const uint8_t *b, uint32_t len) {
    /* is b[0..len) a strict or full prefix of any living unit? */
    for (int u = 0; u < unit_count; ++u)
        if (!units[u].dead && units[u].len >= len &&
            memcmp(units[u].bytes, b, len) == 0)
            return 1;
    return 0;
}

static uint64_t steps_total;

static void unit_birth(uint32_t a, uint32_t b, uint64_t support) {
    uint8_t buf[2 * UNIT_MAX_LEN]; uint8_t t1[1], t2[1];
    uint32_t la = move_len(a), lb = move_len(b);
    if (la + lb > UNIT_MAX_LEN) return;
    memcpy(buf, move_bytes(a, t1), la);
    memcpy(buf + la, move_bytes(b, t2), lb);
    uint32_t L = la + lb;
    for (uint32_t i = 0; i < L; ++i)
        if (is_ws(buf[i])) return;            /* v1 boundary discipline */
    int prior = unit_find(buf, L);            /* same bytes, same unit  */
    if (prior >= 0) {
        if (!units[prior].dead) return;
        /* resurrection: the pair earned the support again, the identity
           returns under its own name, and only then may its frozen history
           vote in the living model again */
        live_mass_add_unit(prior);
        units[prior].dead = 0;
        units[prior].last_use = steps_total;
        unit_living++;
        char rl[96];
        snprintf(rl, sizeof rl, "u\t%llu\t%d\t%llu\n",
                 (unsigned long long)episode_no, prior,
                 (unsigned long long)support);
        bio_append(rl);
        return;
    }
    if (unit_count >= MAX_UNITS) { births_rejected_cap++; return; }
    Unit *u = &units[unit_count];
    memcpy(u->bytes, buf, L);
    u->len = L;
    u->born_episode = episode_no;
    u->support_at_birth = support;
    u->uses = 0;
    u->last_use = steps_total;
    u->dead = 0;
    char line[128], hex[2 * UNIT_MAX_LEN + 1];
    for (uint32_t i = 0; i < L; ++i)
        snprintf(hex + 2 * i, 3, "%02x", buf[i]);
    snprintf(line, sizeof line, "b\t%llu\t%d\t%u\t%s\t%llu\n",
             (unsigned long long)episode_no, unit_count, L, hex,
             (unsigned long long)support);
    bio_append(line);
    unit_count++;
    unit_living++;
}

static uint64_t pair_get(uint32_t prev, uint32_t cur) {
    uint64_t key = ((uint64_t)prev << 32) | cur;
    uint64_t h = fnv1a64((const uint8_t *)&key, sizeof key, FNV_SEED);
    for (uint64_t probe = 0; probe < PAIR_SLOTS; ++probe) {
        Pair *p = &pairs[(h + probe) & (PAIR_SLOTS - 1)];
        if (p->cnt == 0) return 0;
        if (p->key == key) return p->cnt;
    }
    return 0;
}

static void pair_feed(uint32_t prev, uint32_t cur) {
    uint64_t key = ((uint64_t)prev << 32) | cur;
    uint64_t h = fnv1a64((const uint8_t *)&key, sizeof key, FNV_SEED);
    for (uint64_t probe = 0; probe < PAIR_SLOTS; ++probe) {
        Pair *p = &pairs[(h + probe) & (PAIR_SLOTS - 1)];
        if (p->cnt == 0) {
            if (pair_used * 10 >= (uint64_t)PAIR_SLOTS * 9) {
                fprintf(stderr, "netta: pair table full\n"); exit(1);
            }
            p->key = key; p->cnt = 1; pair_used++;
            return;
        }
        if (p->key == key) {
            p->cnt++;
            /* every renewed BIRTH_SUPPORT of lived adjacency is a claim:
               a first birth, a no-op for the living, or a resurrection */
            if (p->cnt % BIRTH_SUPPORT == 0) unit_birth(prev, cur, p->cnt);
            return;
        }
    }
    fprintf(stderr, "netta: pair table full\n"); exit(1);
}

/* matcher: segments the lived truth tape into moves (greedy longest
   unit match), feeds adjacent-move pairs, emits macro receipts. */
static uint8_t  mbuf[UNIT_MAX_LEN];
static double   mbuf_atomic_bits[UNIT_MAX_LEN];
static uint32_t mlen = 0;
static int64_t  prev_move = -1;
static uint64_t mstart_pos = 0;     /* world offset of mbuf[0] */
static int      mstart_isl = 0;
static uint64_t macro_events = 0, macro_bytes = 0;
static uint64_t moves_emitted = 0;

/* shadow unit-LM: a semi-Markov unigram over moves of the segmented
   lived tape. Prequential: each move is priced before its count is
   updated. Same ruler as the game -- bits per raw byte. The alphabet
   is nonstationary by law: births widen the Laplace denominator. With
   no living units the segmentation is trivial and this model is
   identical to the atomic one -- its built-in red twin. */
static uint64_t move_count[ACTIONS + MAX_UNITS];
static uint64_t move_total = 0;
/* Body 13: history is not authority. Tombstones retain the frozen totals
   above for identity and resurrection, while current distributions use only
   counts whose destination remains in the living alphabet. */
static uint64_t move_live_total = 0;
static uint64_t mv_live_out[ACTIONS + MAX_UNITS];
static int tombstone_silence_enabled = 1;
static double   unitlm_bits = 0.0;
static uint64_t unitlm_bytes = 0;
static double   atomic_bits_lived = 0.0;
static uint64_t atomic_bytes_lived = 0;

/* body 4: two context-bearing shadow oracles (postgpt lineage), same
   ruler, prequential. The byte bigram conditions on the previous WORLD
   byte, so no seam can exist across episodes by construction. The move
   bigram's statistics ARE the nursery's pair counts -- the oracle and
   the vocabulary growth are one tissue; it only adds an outgoing total
   per move and pays its price before pair_feed updates the pair. */
static uint64_t bi_count[ACTIONS][ACTIONS];
static uint64_t bi_row[ACTIONS];
static double   bilm_bits = 0.0;
static uint64_t bilm_bytes = 0;
static uint64_t mv_out[ACTIONS + MAX_UNITS];
static double   mvlm_bits = 0.0;
static uint64_t mvlm_bytes = 0;
static double   mvlm_ref_bits = 0.0;
static uint64_t mvlm_ref_bytes = 0;
/* mv's PLAYED record: the emission seat is earned in this discipline
   only -- pricing moves in shadow does not transfer to the right to
   emit them (measured in this body: seating mv on its shadow record
   collapsed earned lives; generation errors pay the full move price
   for one byte of advance). The shadow record merely opens probation. */
static double   mvp_bits = 0.0;
static uint64_t mvp_bytes = 0;
static double   mvp_ref_bits[3] = {0.0, 0.0, 0.0};
static uint64_t mvp_ref_bytes = 0;
/* Forced mv is a counterfactual control, never mandate evidence. These
   this-run counters are deliberately absent from state. */
static double   mvc_bits = 0.0;
static uint64_t mvc_bytes = 0;
static double   mvc_ref_bits[3] = {0.0, 0.0, 0.0};

/* body 5: the right to act is earned. Candidates are the byte models
   only: atomic-uni (the newborn actor) and byte-bi. The right is
   granted by the lived prequential record alone -- byte-bi acts when
   its cumulative lived bits/byte beats atomic by >= 0.1 after >= 1000
   lived bytes; it steps down if the lead falls under 0.05 (hysteresis);
   elections happen only at episode boundaries and are biography events.
   The judge prices the ACTING model in the receipt; the shadow prices
   of all models stay prequential and untouched by who acts. Where the
   right is not earned, intervention must be exactly zero. */
/* body 6: the trigram floor of the oracle ladder. Contexts are byte
   pairs (65536 of them, a direct row-total array); the (context, byte)
   counts live in an open-addressed table like the pair counts, failing
   loudly when full. Prequential, same ruler, and a third candidate for
   the seat. */
#define TRI_SLOTS (1u << 20)
static Pair    *tri;               /* key = ctx<<8 | byte */
static uint64_t tri_used = 0;
static uint64_t tri_row[65536];
static double   trilm_bits = 0.0;
static uint64_t trilm_bytes = 0;

static uint64_t tri_get(uint32_t ctx, uint8_t b) {
    uint64_t key = ((uint64_t)ctx << 8) | b;
    uint64_t h = fnv1a64((const uint8_t *)&key, sizeof key, FNV_SEED);
    for (uint64_t probe = 0; probe < TRI_SLOTS; ++probe) {
        Pair *p = &tri[(h + probe) & (TRI_SLOTS - 1)];
        if (p->cnt == 0) return 0;
        if (p->key == key) return p->cnt;
    }
    return 0;
}

static void tri_add(uint32_t ctx, uint8_t b) {
    uint64_t key = ((uint64_t)ctx << 8) | b;
    uint64_t h = fnv1a64((const uint8_t *)&key, sizeof key, FNV_SEED);
    for (uint64_t probe = 0; probe < TRI_SLOTS; ++probe) {
        Pair *p = &tri[(h + probe) & (TRI_SLOTS - 1)];
        if (p->cnt == 0) {
            if (tri_used * 10 >= (uint64_t)TRI_SLOTS * 9) {
                fprintf(stderr, "netta: trigram table full\n"); exit(1);
            }
            p->key = key; p->cnt = 1; tri_used++;
            tri_row[ctx]++;
            return;
        }
        if (p->key == key) { p->cnt++; tri_row[ctx]++; return; }
    }
    fprintf(stderr, "netta: trigram table full\n"); exit(1);
}

/* body 16: the neural core enters in shadow. NETTA's own lineage, read
   from the buried prototype: no backpropagation. A recurrent state over
   innate byte embeddings and a delta-rule readout prices every lived
   truth byte on the same ruler, learns strictly after the receipt,
   holds no candidacy, and writes no biography line: a witness with a
   record and no power. The first surprise-gated Hebbian rule remains as
   the explicit --core-hebb-v1 red arm, but is quarantined by default
   after losing matched period-5 through period-8 controls. The grave's
   scars are law here: weights are clamped and finite, the hidden state
   is watched for saturation, and innate embeddings are regenerated from
   a fixed seed rather than trusted to writable state. */
#define CORE_EMBED   24
#define CORE_HIDDEN  32
#define CORE_INIT_SEED 0x4e455454414e4e31ULL
#define CORE_LR_OUT  0.05f
#define CORE_LR_IN   0.01f
#define CORE_DECAY   0.9995f
#define CORE_CLIP    1.0
#define CORE_GATE    0.5
#define CORE_WCLAMP  4.0f
static float  core_E[ACTIONS][CORE_EMBED];
static float  core_Wxh[CORE_HIDDEN][CORE_EMBED];
static float  core_Whh[CORE_HIDDEN][CORE_HIDDEN];
static float  core_Who[ACTIONS][CORE_HIDDEN];
static float  core_h[CORE_HIDDEN];   /* episode-local, never persisted */
static double core_bits = 0.0;
static uint64_t core_bytes = 0;
static double core_nll_ema = 8.0;    /* the prophecy baseline */
static double core_sat_sum = 0.0;
static uint64_t core_sat_n = 0;
static int    core_enabled = 1;
static int    core_hebb_enabled = 0; /* v1 quarantined behind its red arm */
static uint64_t core_hebb_pos = 0, core_hebb_neg = 0;

/* body 17: the plasticity jury. The eight rows are the complete court --
   there is no mutation stream behind them. Every shadow is born from the
   identical frozen reservoir and changes only its own recurrent memory. */
#define JURY_GENOMES 8
typedef struct {
    float eta_in, eta_rec, gate, mod_clip, decay, target, homeostasis;
} JuryGene;

static const JuryGene jury_gene[JURY_GENOMES] = {
    {0.0f,       0.0f,       0.5f, 1.00f, 0.0f,     1.00f, 0.0f},
    {0.0003125f, 0.0003125f, 0.5f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0006250f, 0.0006250f, 0.5f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0012500f, 0.0012500f, 0.5f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0006250f, 0.0003125f, 0.5f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0003125f, 0.0006250f, 0.5f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0006250f, 0.0006250f, 1.0f, 0.25f, 0.000001f, 0.35f,
     0.000244140625f},
    {0.0006250f, 0.0006250f, 0.5f, 0.50f, 0.000001f, 0.25f,
     0.000488281250f},
};

typedef struct {
    float Wxh[CORE_HIDDEN][CORE_EMBED];
    float Whh[CORE_HIDDEN][CORE_HIDDEN];
    float Who[ACTIONS][CORE_HIDDEN];
    float h[CORE_HIDDEN];          /* episode-local, never persisted */
    double nll_ema;
    double bits;
    uint64_t bytes;
    double row_abs_sum[CORE_HIDDEN];
    uint64_t health_bytes;
    uint64_t near_sat;
    uint64_t clamp_hits;
    uint64_t gates_pos, gates_neg;
} JuryCore;

static JuryCore jury_core[JURY_GENOMES];
static int jury_enabled = 0;

static float core_clampw(float w) {
    if (w > CORE_WCLAMP) return CORE_WCLAMP;
    if (w < -CORE_WCLAMP) return -CORE_WCLAMP;
    return w;
}

static float core_rand(uint64_t *s) {
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    z ^= z >> 31;
    return (float)(z >> 40) / 8388608.0f - 1.0f;
}

static void core_init(void) {
    /* the innate identity of every byte comes from a dedicated
       deterministic stream; the life's rng is never consumed, so no
       inherited gate number moves */
    uint64_t s = CORE_INIT_SEED;
    for (int b = 0; b < ACTIONS; ++b)
        for (int d = 0; d < CORE_EMBED; ++d)
            core_E[b][d] = 0.5f * core_rand(&s);
    for (int j = 0; j < CORE_HIDDEN; ++j)
        for (int d = 0; d < CORE_EMBED; ++d)
            core_Wxh[j][d] = 0.3f * core_rand(&s);
    for (int j = 0; j < CORE_HIDDEN; ++j)
        for (int k = 0; k < CORE_HIDDEN; ++k)
            core_Whh[j][k] = 0.2f * core_rand(&s);
    memset(core_Who, 0, sizeof core_Who);  /* the newborn readout
                                              prices exact ignorance */
}

static void jury_init(void) {
    memset(jury_core, 0, sizeof jury_core);
    for (int g = 0; g < JURY_GENOMES; ++g) {
        memcpy(jury_core[g].Wxh, core_Wxh, sizeof core_Wxh);
        memcpy(jury_core[g].Whh, core_Whh, sizeof core_Whh);
        memcpy(jury_core[g].Who, core_Who, sizeof core_Who);
        jury_core[g].nll_ema = 8.0;
    }
}

static void core_advance(uint8_t byte) {
    float hn[CORE_HIDDEN];
    for (int j = 0; j < CORE_HIDDEN; ++j) {
        float a = 0.0f;
        for (int d = 0; d < CORE_EMBED; ++d)
            a += core_Wxh[j][d] * core_E[byte][d];
        for (int k = 0; k < CORE_HIDDEN; ++k)
            a += core_Whh[j][k] * core_h[k];
        hn[j] = tanhf(a);
    }
    memcpy(core_h, hn, sizeof hn);
}

static void core_warm(const Island *isl, uint64_t start) {
    /* the observable wake builds the state; nothing is priced or
       learned twice */
    memset(core_h, 0, sizeof core_h);
    for (uint64_t p = start - CTX; p < start; ++p)
        core_advance(isl->bytes[p]);
}

static void core_absorb(uint8_t truth) {
    /* prophecy first: the truth byte is priced from the current state */
    float logits[ACTIONS];
    float mx = -1e30f;
    for (int o = 0; o < ACTIONS; ++o) {
        float a = 0.0f;
        for (int j = 0; j < CORE_HIDDEN; ++j)
            a += core_Who[o][j] * core_h[j];
        logits[o] = a;
        if (a > mx) mx = a;
    }
    double denom = 0.0;
    for (int o = 0; o < ACTIONS; ++o)
        denom += exp((double)logits[o] - mx);
    double p_truth = exp((double)logits[truth] - mx) / denom;
    double nll = -log2(p_truth);
    if (!isfinite(nll)) {
        fprintf(stderr, "netta: core price is not finite\n"); exit(1);
    }
    core_bits += nll;
    core_bytes++;

    /* first law of the grave: the delta rule on the readout alone */
    float h_old[CORE_HIDDEN];
    memcpy(h_old, core_h, sizeof h_old);
    for (int o = 0; o < ACTIONS; ++o) {
        float p_o = (float)(exp((double)logits[o] - mx) / denom);
        float err = (o == truth ? 1.0f : 0.0f) - p_o;
        for (int j = 0; j < CORE_HIDDEN; ++j)
            core_Who[o][j] =
                core_clampw(core_Who[o][j] +
                            CORE_LR_OUT * err * h_old[j]);
    }

    /* destiny arrives, the state moves on */
    core_advance(truth);
    double sat = 0.0;
    for (int j = 0; j < CORE_HIDDEN; ++j) sat += fabsf(core_h[j]);
    core_sat_sum += sat / (double)CORE_HIDDEN;
    core_sat_n++;

    /* The quarantined second law: surprise-gated Hebbian plasticity on
       the dynamics. Better than the prophecy baseline potentiates the
       lived co-activation, worse depresses it; small surprises move
       nothing. We always count its proposed gates, but mutate only on
       the explicit red arm. */
    double surprise = core_nll_ema - nll;
    if (fabs(surprise) > CORE_GATE) {
        if (surprise > 0.0) core_hebb_pos++;
        else core_hebb_neg++;
        float mod = (float)(surprise / 8.0);
        if (mod > (float)CORE_CLIP) mod = (float)CORE_CLIP;
        if (mod < -(float)CORE_CLIP) mod = -(float)CORE_CLIP;
        if (core_hebb_enabled)
            for (int j = 0; j < CORE_HIDDEN; ++j) {
                for (int d = 0; d < CORE_EMBED; ++d)
                    core_Wxh[j][d] =
                        core_clampw(CORE_DECAY * core_Wxh[j][d] +
                                    CORE_LR_IN * mod * core_h[j] *
                                    core_E[truth][d]);
                for (int k = 0; k < CORE_HIDDEN; ++k)
                    core_Whh[j][k] =
                        core_clampw(CORE_DECAY * core_Whh[j][k] +
                                    CORE_LR_IN * mod * core_h[j] * h_old[k]);
            }
    }

    /* third law: the prophecy baseline floats, fast quote slow memory */
    core_nll_ema = 0.82 * core_nll_ema + 0.18 * nll;
}

static uint64_t core_state_witness(void) {
    /* Neural memory has no count conservation equation. Bind every mutable
       byte and its prequential record into one partial-forgery witness;
       coherent reconstruction of the witness is whole-life fabrication,
       the same boundary named by the registry audit. Innate embeddings are
       excluded because they no longer live on the wire at all. */
    uint64_t h = FNV_WITNESS_SEED;
    h = fnv1a64((const uint8_t *)core_Wxh, sizeof core_Wxh, h);
    h = fnv1a64((const uint8_t *)core_Whh, sizeof core_Whh, h);
    h = fnv1a64((const uint8_t *)core_Who, sizeof core_Who, h);
    h = fnv1a64((const uint8_t *)&core_nll_ema,
                sizeof core_nll_ema, h);
    h = fnv1a64((const uint8_t *)&core_bits, sizeof core_bits, h);
    h = fnv1a64((const uint8_t *)&core_bytes, sizeof core_bytes, h);
    return h;
}

static float jury_clampw(float value, uint64_t *hits) {
    if (!isfinite(value)) {
        fprintf(stderr, "netta: jury weight is not finite\n"); exit(1);
    }
    if (value > CORE_WCLAMP) {
        if (hits) (*hits)++;
        return CORE_WCLAMP;
    }
    if (value < -CORE_WCLAMP) {
        if (hits) (*hits)++;
        return -CORE_WCLAMP;
    }
    return value;
}

static void jury_advance_one(JuryCore *c, uint8_t byte) {
    float hn[CORE_HIDDEN];
    for (int j = 0; j < CORE_HIDDEN; ++j) {
        float a = 0.0f;
        for (int d = 0; d < CORE_EMBED; ++d)
            a += c->Wxh[j][d] * core_E[byte][d];
        for (int k = 0; k < CORE_HIDDEN; ++k)
            a += c->Whh[j][k] * c->h[k];
        hn[j] = tanhf(a);
    }
    memcpy(c->h, hn, sizeof hn);
}

static void jury_warm(const Island *isl, uint64_t start) {
    for (int g = 0; g < JURY_GENOMES; ++g) {
        JuryCore *c = &jury_core[g];
        memset(c->h, 0, sizeof c->h);
        for (uint64_t p = start - CTX; p < start; ++p)
            jury_advance_one(c, isl->bytes[p]);
    }
}

static void jury_absorb(uint8_t truth) {
    for (int g = 0; g < JURY_GENOMES; ++g) {
        JuryCore *c = &jury_core[g];
        const JuryGene *gene = &jury_gene[g];
        float logits[ACTIONS];
        float mx = -1e30f;
        for (int o = 0; o < ACTIONS; ++o) {
            float a = 0.0f;
            for (int j = 0; j < CORE_HIDDEN; ++j)
                a += c->Who[o][j] * c->h[j];
            logits[o] = a;
            if (a > mx) mx = a;
        }
        double denom = 0.0;
        for (int o = 0; o < ACTIONS; ++o)
            denom += exp((double)logits[o] - mx);
        double p_truth = exp((double)logits[truth] - mx) / denom;
        double nll = -log2(p_truth);
        if (!isfinite(nll)) {
            fprintf(stderr, "netta: jury price is not finite\n"); exit(1);
        }
        c->bits += nll;
        c->bytes++;

        float h_old[CORE_HIDDEN];
        memcpy(h_old, c->h, sizeof h_old);
        for (int o = 0; o < ACTIONS; ++o) {
            float p_o = (float)(exp((double)logits[o] - mx) / denom);
            float err = (o == truth ? 1.0f : 0.0f) - p_o;
            for (int j = 0; j < CORE_HIDDEN; ++j)
                c->Who[o][j] = jury_clampw(
                    c->Who[o][j] + CORE_LR_OUT * err * h_old[j],
                    NULL); /* fixed readout is outside the recurrent trial */
        }

        jury_advance_one(c, truth);
        c->health_bytes++;
        for (int j = 0; j < CORE_HIDDEN; ++j) {
            double ah = fabsf(c->h[j]);
            c->row_abs_sum[j] += ah;
            if (ah >= 0.98) c->near_sat++;
        }

        double surprise = c->nll_ema - nll;
        int gate_open = fabs(surprise) > gene->gate;
        float mod = (float)(surprise / 8.0);
        if (mod > gene->mod_clip) mod = gene->mod_clip;
        if (mod < -gene->mod_clip) mod = -gene->mod_clip;
        if (gate_open) {
            if (surprise > 0.0) c->gates_pos++;
            else c->gates_neg++;
        }
        for (int j = 0; j < CORE_HIDDEN; ++j) {
            float excess = fabsf(c->h[j]) - gene->target;
            if (excess < 0.0f) excess = 0.0f;
            float scale = 1.0f - gene->decay - gene->homeostasis * excess;
            if (scale < 0.0f) scale = 0.0f;
            for (int d = 0; d < CORE_EMBED; ++d) {
                float add = gate_open ? gene->eta_in * mod * c->h[j] *
                                        core_E[truth][d] : 0.0f;
                c->Wxh[j][d] = jury_clampw(
                    scale * c->Wxh[j][d] + add, &c->clamp_hits);
            }
            for (int k = 0; k < CORE_HIDDEN; ++k) {
                float add = gate_open ? gene->eta_rec * mod * c->h[j] *
                                        h_old[k] : 0.0f;
                c->Whh[j][k] = jury_clampw(
                    scale * c->Whh[j][k] + add, &c->clamp_hits);
            }
        }
        c->nll_ema = 0.82 * c->nll_ema + 0.18 * nll;
    }
}

static uint64_t jury_state_witness(void) {
    uint64_t h = FNV_WITNESS_SEED;
    h = fnv1a64((const uint8_t *)jury_gene, sizeof jury_gene, h);
    for (int g = 0; g < JURY_GENOMES; ++g) {
        JuryCore *c = &jury_core[g];
        h = fnv1a64((const uint8_t *)c->Wxh, sizeof c->Wxh, h);
        h = fnv1a64((const uint8_t *)c->Whh, sizeof c->Whh, h);
        h = fnv1a64((const uint8_t *)c->Who, sizeof c->Who, h);
        h = fnv1a64((const uint8_t *)&c->nll_ema, sizeof c->nll_ema, h);
        h = fnv1a64((const uint8_t *)&c->bits, sizeof c->bits, h);
        h = fnv1a64((const uint8_t *)&c->bytes, sizeof c->bytes, h);
        h = fnv1a64((const uint8_t *)c->row_abs_sum,
                    sizeof c->row_abs_sum, h);
        h = fnv1a64((const uint8_t *)&c->health_bytes,
                    sizeof c->health_bytes, h);
        h = fnv1a64((const uint8_t *)&c->near_sat,
                    sizeof c->near_sat, h);
        h = fnv1a64((const uint8_t *)&c->clamp_hits,
                    sizeof c->clamp_hits, h);
        h = fnv1a64((const uint8_t *)&c->gates_pos,
                    sizeof c->gates_pos, h);
        h = fnv1a64((const uint8_t *)&c->gates_neg,
                    sizeof c->gates_neg, h);
    }
    return h;
}

static uint32_t neural_law(void) {
    return (core_enabled ? 1u : 0u) |
           (core_hebb_enabled ? 2u : 0u) |
           (jury_enabled ? 4u : 0u);
}

/* the seat: candidates 0 = atomic-uni (newborn), 1 = byte-bi,
   2 = byte-tri, 3 = mv (the semi-Markov move-player, body 8: emits
   whole moves -- bytes or earned units -- priced by the move bigram
   whose statistics are the nursery's own pair counts). Granted by the
   lived prequential record alone. */
static int actor_current = 0;
static int actor_lock = -1;        /* CLI: -1 earned, 0..3 locked */
static int move_nav_enabled = 1;   /* body 9: search the observable wake */
#define ACTOR_NULL 4
static uint64_t ep_actor[5] = {0, 0, 0, 0, 0};
static const char *actor_name[5] = {"uni", "bi", "tri", "mv", "null"};
static uint64_t move_nav_steps = 0, move_nav_unit_anchors = 0;

/* body 10: the island court. Global models travel; records are local.
   Every island keeps its own prequential record of every witness on the
   bytes it lived, and the move player's local played/ref evidence. A
   seat elected on the whole biography must still satisfy the island it
   is about to act on: once the island has lived ACTOR_MIN_BYTES, a
   seated actor whose local lead over the local newborn record falls
   under ACTOR_KEEP is revoked here only -- the episode is acted by the
   best locally eligible byte witness, and the global seat is untouched.
   No local evidence, no local verdict: authority travels on comity
   until the island can judge. */
static double   isl_bits[MAX_REGISTRY][3];
static uint64_t isl_lived[MAX_REGISTRY];
static double   isl_mvp_bits[MAX_REGISTRY];
static uint64_t isl_mvp_bytes[MAX_REGISTRY];
static double   isl_mvp_ref_bits[MAX_REGISTRY][3];
static uint64_t isl_mvp_ref_bytes[MAX_REGISTRY];
/* The probation door is jurisdiction too. These are the move shadow and
   its atomic witness on the exact same canonical moves, partitioned by
   island; a trial cannot travel on another island's promise. */
static double   isl_mvlm_bits[MAX_REGISTRY];
static double   isl_mvlm_ref_bits[MAX_REGISTRY];
static uint64_t isl_mvlm_bytes[MAX_REGISTRY];

/* body 14: the island registry. An island's identity is its content --
   forward digest, reverse witness, and length -- never its position in
   today's command line. The registry is the append-only journal of every
   island this life has
   met: records live under registry ids, an arrival is a biography
   event, and an island absent from today's convoy keeps its memory. A
   changed file is by construction a different island; arrivals are
   loud, never silent. */
static int present_reg[MAX_ISLANDS];
static int arrivals_pending[MAX_ISLANDS];
static int arrivals_pending_n = 0;

static int registry_find(uint64_t digest, uint64_t witness, uint64_t len) {
    for (int r = 0; r < reg_count; ++r)
        if (reg_digest[r] == digest && reg_witness[r] == witness &&
            reg_len[r] == len) return r;
    return -1;
}

static int same_island_identity(const Island *a, const Island *b) {
    return a->digest == b->digest && a->witness == b->witness &&
           a->len == b->len;
}

static int registry_add(const Island *isl) {
    if (reg_count >= MAX_REGISTRY) {
        fprintf(stderr, "netta: island registry full (max %d)\n",
                MAX_REGISTRY);
        exit(1);
    }
    reg_digest[reg_count] = isl->digest;
    reg_witness[reg_count] = isl->witness;
    reg_len[reg_count] = isl->len;
    arrivals_pending[arrivals_pending_n++] = reg_count;
    return reg_count++;
}

static int island_identity_less(const Island *a, const Island *b) {
    if (a->digest != b->digest) return a->digest < b->digest;
    if (a->witness != b->witness) return a->witness < b->witness;
    return a->len < b->len;
}

static void registry_resolve_convoy(void) {
    for (int i = 0; i < island_count; ++i) {
        present_reg[i] = registry_find(islands[i].digest,
                                       islands[i].witness, islands[i].len);
        for (int j = 0; j < i; ++j)
            if (same_island_identity(&islands[i], &islands[j]) &&
                (islands[i].len != islands[j].len ||
                 memcmp(islands[i].bytes, islands[j].bytes,
                        islands[i].len) != 0)) {
                fprintf(stderr, "netta: island identity collision; refusing\n");
                exit(1);
            }
    }

    /* Simultaneous acquaintances are ordered by content, never by their
       seats in today's command line or by the island selected for play.
       Identity and navigation are deliberately separate laws. */
    for (;;) {
        int best = -1;
        for (int i = 0; i < island_count; ++i)
            if (present_reg[i] < 0 &&
                (best < 0 || island_identity_less(&islands[i],
                                                  &islands[best])))
                best = i;
        if (best < 0) break;
        int r = registry_add(&islands[best]);
        for (int i = 0; i < island_count; ++i)
            if (present_reg[i] < 0 &&
                same_island_identity(&islands[i], &islands[best]))
                present_reg[i] = r;
    }
}
static int island_court_enabled = 1;
static int local_probation_enabled = 1;
/* body 11: fixed uniform null is a local hand, never a global seat. A
   travelled byte hand must earn ACTOR_GAIN over its eight-bit floor, and an
   episode cannot carry blind comity beyond ACTOR_MIN_BYTES. */
static int birth_floor_enabled = 1;
static uint64_t revocations = 0;
static uint64_t null_refusals = 0;
/* body 12: the vocabulary pays rent. A unit unrecognised for UNIT_TTL
   lived bytes dies; its identity and frozen counts remain, its slot in
   the living alphabet is released, and renewed pair support resurrects
   the same name. */
static int unit_death_enabled = 1;

#define ACTOR_MIN_BYTES 1000
#define ACTOR_GAIN      0.1
#define ACTOR_KEEP      0.05

/* body 15: the atlas is an earned navigation policy, not another actor.
   It may choose only among identities physically present in today's
   convoy and able to hold the requested episode. Before an island has a
   full local record, the least-lived shore has priority; afterwards the
   cheapest already-measured byte witness is the expected structural
   rhyme. Registry id breaks exact ties, so argv order cannot steer it. */
static int atlas_enabled = 0;
static uint64_t atlas_decisions = 0;

static int island_can_hold(int i, uint64_t steps) {
    Island *isl = &islands[i];
    if (isl->len < CTX + 2 || steps > isl->len - CTX - 1)
        return 0;
    if (fixed_start_set &&
        (fixed_start < CTX || fixed_start >= isl->len ||
         steps > isl->len - fixed_start))
        return 0;
    return 1;
}

static double atlas_score(int reg) {
    double best = 8.0;              /* honest ignorance is the ceiling */
    for (int c = 0; c < 3; ++c) {
        double score = isl_bits[reg][c] / (double)isl_lived[reg];
        if (score < best) best = score;
    }
    return best;
}

static int atlas_choose(int fallback, uint64_t steps) {
    int candidate[MAX_ISLANDS], n = 0, unique_present = 0;
    for (int i = 0; i < island_count; ++i) {
        int seen = 0;
        for (int j = 0; j < i; ++j)
            if (present_reg[j] == present_reg[i]) { seen = 1; break; }
        if (seen) continue;
        unique_present++;
        if (island_can_hold(i, steps)) candidate[n++] = i;
    }
    if (unique_present == 1) return fallback; /* exact one-world no-op */
    if (n == 0) return fallback;              /* old refusal names why */
    if (n == 1) {
        int r = present_reg[candidate[0]];
        char line[96];
        snprintf(line, sizeof line, "t\t%llu\t%d\teligible\n",
                 (unsigned long long)(episode_no + 1), r);
        bio_append(line);
        atlas_decisions++;
        printf("atlas: episode %llu registry %d sole eligible shore\n",
               (unsigned long long)(episode_no + 1), r);
        return candidate[0];
    }

    int best = -1, chart = 0;
    for (int j = 0; j < n; ++j) {
        int i = candidate[j], r = present_reg[i];
        if (isl_lived[r] >= ACTOR_MIN_BYTES) continue;
        if (!chart || isl_lived[r] < isl_lived[present_reg[best]] ||
            (isl_lived[r] == isl_lived[present_reg[best]] &&
             r < present_reg[best]))
            best = i;
        chart = 1;
    }
    if (!chart) {
        for (int j = 0; j < n; ++j) {
            int i = candidate[j], r = present_reg[i];
            if (best < 0 || atlas_score(r) < atlas_score(present_reg[best]) ||
                (atlas_score(r) == atlas_score(present_reg[best]) &&
                 r < present_reg[best]))
                best = i;
        }
    }

    int r = present_reg[best];
    char line[144];
    if (chart) {
        uint64_t runner = UINT64_MAX;
        for (int j = 0; j < n; ++j)
            if (candidate[j] != best &&
                isl_lived[present_reg[candidate[j]]] < runner)
                runner = isl_lived[present_reg[candidate[j]]];
        snprintf(line, sizeof line,
                 "t\t%llu\t%d\tchart\t%llu\t%llu\n",
                 (unsigned long long)(episode_no + 1), r,
                 (unsigned long long)isl_lived[r],
                 (unsigned long long)runner);
    } else {
        double runner = 1e9;
        for (int j = 0; j < n; ++j)
            if (candidate[j] != best &&
                atlas_score(present_reg[candidate[j]]) < runner)
                runner = atlas_score(present_reg[candidate[j]]);
        snprintf(line, sizeof line,
                 "t\t%llu\t%d\tearned\t%.6f\t%.6f\t%llu\n",
                 (unsigned long long)(episode_no + 1), r, atlas_score(r), runner,
                 (unsigned long long)isl_lived[r]);
    }
    bio_append(line);
    atlas_decisions++;
    printf("atlas: episode %llu registry %d %s ",
           (unsigned long long)(episode_no + 1), r,
           chart ? "chart" : "earned");
    if (chart)
        printf("lived %llu\n", (unsigned long long)isl_lived[r]);
    else
        printf("score %.6f over %llu bytes\n", atlas_score(r),
               (unsigned long long)isl_lived[r]);
    return best;
}

static void actor_elect(void) {
    if (actor_lock >= 0) { actor_current = actor_lock; return; }
    if (atomic_bytes_lived < ACTOR_MIN_BYTES) { actor_current = 0; return; }
    double lv[3];
    lv[0] = atomic_bits_lived / (double)atomic_bytes_lived;
    lv[1] = bilm_bytes ? bilm_bits / (double)bilm_bytes : 1e9;
    lv[2] = trilm_bytes ? trilm_bits / (double)trilm_bytes : 1e9;

    /* First determine the byte seat on its common lifetime record. If mv
       is sitting, this is the byte successor that would take its place. */
    int byte_seat = actor_current >= 0 && actor_current <= 2
                  ? actor_current : 0;
    if (byte_seat != 0 && lv[0] - lv[byte_seat] < ACTOR_KEEP)
        byte_seat = 0;
    int ch = -1;
    for (int c = 1; c <= 2; ++c) {
        if (c == byte_seat) continue;
        if (lv[0] - lv[c] < ACTOR_GAIN) continue;
        if (ch < 0 || lv[c] < lv[ch]) ch = c;
    }
    if (ch >= 0) {
        if (byte_seat == 0 || lv[byte_seat] - lv[ch] >= ACTOR_KEEP)
            byte_seat = ch;
    }

    /* Move authority is judged only against byte witnesses recorded on the
       exact same played bytes. Lifetime byte records and probation records
       are different event bases and are not compared. */
    if (units_enabled && mvp_bytes >= ACTOR_MIN_BYTES &&
        mvp_ref_bytes == mvp_bytes) {
        double mv = mvp_bits / (double)mvp_bytes;
        double best_ref = mvp_ref_bits[0] / (double)mvp_ref_bytes;
        for (int c = 1; c <= 2; ++c) {
            double ref = mvp_ref_bits[c] / (double)mvp_ref_bytes;
            if (ref < best_ref) best_ref = ref;
        }
        if (actor_current == 3) {
            actor_current = best_ref - mv >= ACTOR_KEEP ? 3 : byte_seat;
        } else {
            actor_current = best_ref - mv >= ACTOR_GAIN ? 3 : byte_seat;
        }
    } else {
        actor_current = byte_seat;
    }
}

static int island_court(int isl, uint64_t episode_steps) {
    /* the island's verdict on the globally elected seat, for this
       episode only; a revocation is a biography event, not a seat
       change -- the mandate travels on, the island refuses the hand */
    int seated = actor_current;
    if (!island_court_enabled || actor_lock >= 0)
        return seated;

    /* The body-10 red arm remains an exact switch: without the birth floor,
       the old court waits for a complete local record and treats global uni
       as the newborn hand. */
    if (!birth_floor_enabled) {
        if (seated == 0 || isl_lived[isl] < ACTOR_MIN_BYTES) return seated;
        double lu = isl_bits[isl][0] / (double)isl_lived[isl];
        double lseat;
        int fails;
        if (seated == 3) {
            if (isl_mvp_bytes[isl] < ACTOR_MIN_BYTES ||
                isl_mvp_ref_bytes[isl] != isl_mvp_bytes[isl])
                return seated;
            lseat = isl_mvp_bits[isl] / (double)isl_mvp_bytes[isl];
            double br = isl_mvp_ref_bits[isl][0] /
                        (double)isl_mvp_ref_bytes[isl];
            for (int c = 1; c <= 2; ++c) {
                double r = isl_mvp_ref_bits[isl][c] /
                           (double)isl_mvp_ref_bytes[isl];
                if (r < br) br = r;
            }
            fails = br - lseat < ACTOR_KEEP;
        } else {
            lseat = isl_bits[isl][seated] / (double)isl_lived[isl];
            fails = lu - lseat < ACTOR_KEEP;
        }
        if (!fails) return seated;
        int ch = 0;
        for (int c = 1; c <= 2; ++c) {
            if (c == seated) continue;
            double lc = isl_bits[isl][c] / (double)isl_lived[isl];
            if (lu - lc < ACTOR_GAIN) continue;
            if (ch == 0 ||
                lc < isl_bits[isl][ch] / (double)isl_lived[isl])
                ch = c;
        }
        revocations++;
        char line[160];
        snprintf(line, sizeof line, "r\t%llu\t%d\t%s\t%s\t%.6f\t%.6f\n",
                 (unsigned long long)episode_no, isl, actor_name[seated],
                 actor_name[ch], lu, lseat);
        bio_append(line);
        return ch;
    }

    /* Body 11: fixed uniform ignorance is an island birthright, not a global
       seat. It matters only when a model has actually travelled: the first
       island and every one-island life keep their byte-identical old body. */
    if (steps_total == isl_lived[isl]) return seated;

    uint64_t lived = isl_lived[isl];
    if (lived < ACTOR_MIN_BYTES &&
        episode_steps <= ACTOR_MIN_BYTES - lived)
        return seated;                    /* bounded blind comity */
    if (lived == 0) {
        /* An indivisible episode would cross the byte budget before the
           island had one receipt to judge. The safe causal hand is null for
           that whole episode; no future byte is borrowed for the verdict. */
        revocations++;
        null_refusals++;
        char line[128];
        snprintf(line, sizeof line, "q\t%llu\t%d\t%s\tnull\t0\n",
                 (unsigned long long)episode_no, isl, actor_name[seated]);
        bio_append(line);
        return ACTOR_NULL;
    }

    double lu = isl_bits[isl][0] / (double)isl_lived[isl];
    double lseat;
    int fails;
    int seat_unpriced = 0;
    if (seated == 3) {
        uint64_t played = isl_mvp_bytes[isl];
        if (played < ACTOR_MIN_BYTES &&
            episode_steps <= ACTOR_MIN_BYTES - played)
            return seated;                /* bounded move-hand comity */
        if (played == 0) {
            fails = 1;
            seat_unpriced = 1;
            lseat = 0.0;                   /* never emitted as a score */
        } else {
            lseat = isl_mvp_bits[isl] / (double)played;
            double br = isl_mvp_ref_bits[isl][0] /
                        (double)isl_mvp_ref_bytes[isl];
            for (int c = 1; c <= 2; ++c) {
                double r = isl_mvp_ref_bits[isl][c] /
                           (double)isl_mvp_ref_bytes[isl];
                if (r < br) br = r;
            }
            fails = br - lseat < ACTOR_KEEP;
        }
    } else if (seated == 0) {
        lseat = lu;
        fails = 0;
    } else {
        lseat = isl_bits[isl][seated] / (double)isl_lived[isl];
        fails = lu - lseat < ACTOR_KEEP;
    }

    int ch = seated;
    if (fails) {
        ch = 0;
        for (int c = 1; c <= 2; ++c) {
            if (c == seated) continue;
            double lc = isl_bits[isl][c] / (double)isl_lived[isl];
            if (lu - lc < ACTOR_GAIN) continue;
            if (ch == 0 ||
                lc < isl_bits[isl][ch] / (double)isl_lived[isl])
                ch = c;
        }
    }

    double lhand = ch == 3 ? lseat
                 : isl_bits[isl][ch] / (double)isl_lived[isl];
    if (8.0 - lhand < ACTOR_GAIN) ch = ACTOR_NULL;
    if (ch == seated) return seated;

    revocations++;
    if (ch == ACTOR_NULL) null_refusals++;
    char line[176];
    if (seat_unpriced)
        snprintf(line, sizeof line, "q\t%llu\t%d\t%s\t%s\t0\n",
                 (unsigned long long)episode_no, isl, actor_name[seated],
                 actor_name[ch]);
    else
        snprintf(line, sizeof line,
                 "r\t%llu\t%d\t%s\t%s\t%.6f\t%.6f\t8.000000\n",
                 (unsigned long long)episode_no, isl, actor_name[seated],
                 actor_name[ch], lu, lseat);
    bio_append(line);
    return ch;
}

static uint32_t truth_move(Island *isl, uint64_t pos, uint64_t room) {
    uint32_t best_len = 1;
    int best_u = -1;
    for (int u = 0; u < unit_count; ++u) {
        uint32_t len = units[u].len;
        if (units[u].dead || len <= best_len || (uint64_t)len > room ||
            pos > isl->len - len)
            continue;
        if (memcmp(isl->bytes + pos, units[u].bytes, len) == 0) {
            best_len = len;
            best_u = u;
        }
    }
    return best_u >= 0 ? (uint32_t)(ACTIONS + best_u)
                       : (uint32_t)isl->bytes[pos];
}

typedef struct {
    uint32_t move;
    double score;
    int live;
} Route;

static double move_prob(int64_t prev, uint32_t cur, int alive) {
    double denom;
    uint64_t cnt;
    if (prev >= 0) {
        uint64_t row = tombstone_silence_enabled
                     ? mv_live_out[(uint32_t)prev]
                     : mv_out[(uint32_t)prev];
        denom = (double)row + (double)alive;
        cnt = pair_get((uint32_t)prev, cur);
    } else {
        uint64_t total = tombstone_silence_enabled
                       ? move_live_total : move_total;
        denom = (double)total + (double)alive;
        cnt = move_count[cur];
    }
    return ((double)cnt + 1.0) / denom;
}

static double move_logp(int64_t prev, uint32_t cur, int alive) {
    return log(move_prob(prev, cur, alive));
}

static int move_matches(const Island *isl, uint64_t pos, uint64_t room,
                        uint32_t move) {
    uint8_t tmp[1];
    uint32_t len = move_len(move);
    if ((uint64_t)len > room || (uint64_t)len > isl->len ||
        pos > isl->len - len) return 0;
    return memcmp(isl->bytes + pos, move_bytes(move, tmp), len) == 0;
}

static void route_offer(Route routes[UNIT_MAX_LEN + 1], uint32_t move,
                        double score) {
    uint32_t len = move_len(move);
    Route *r = &routes[len];
    if (!r->live || score > r->score ||
        (score == r->score && move < r->move)) {
        r->move = move;
        r->score = score;
        r->live = 1;
    }
}

/* Body 9's first navigation organ is a bounded Viterbi search over the
   already-observed byte wake. It reconstructs the most probable exact
   semi-Markov route through the previous CTX bytes, then uses the final
   move as the current move-bigram state. The search can inspect unit forms,
   learned counts, and bytes strictly before pos. It never reads the target
   byte or any proposed move's future span. Atomic moves guarantee that an
   exact route always exists. */
static uint32_t move_route_anchor(const Island *isl, uint64_t pos,
                                  int range, int alphabet) {
    Route routes[CTX + 1][UNIT_MAX_LEN + 1];
    memset(routes, 0, sizeof routes);
    uint64_t base = pos - CTX;

    for (int m = 0; m < range; ++m) {
        if (m >= ACTIONS && units[m - ACTIONS].dead) continue;
        uint32_t len = move_len((uint32_t)m);
        if (len <= CTX && move_matches(isl, base, CTX, (uint32_t)m))
            route_offer(routes[len], (uint32_t)m,
                        move_logp(-1, (uint32_t)m, alphabet));
    }
    for (uint32_t off = 1; off < CTX; ++off) {
        for (uint32_t pl = 1; pl <= UNIT_MAX_LEN; ++pl) {
            Route *prior = &routes[off][pl];
            if (!prior->live) continue;
            uint64_t room = CTX - off;
            for (int m = 0; m < range; ++m) {
                if (m >= ACTIONS && units[m - ACTIONS].dead) continue;
                uint32_t cur = (uint32_t)m;
                uint32_t len = move_len(cur);
                if ((uint64_t)len <= room &&
                    move_matches(isl, base + off, room, cur))
                    route_offer(routes[off + len], cur,
                                prior->score +
                                move_logp((int64_t)prior->move, cur,
                                          alphabet));
            }
        }
    }
    Route *best = NULL;
    for (uint32_t len = 1; len <= UNIT_MAX_LEN; ++len) {
        Route *r = &routes[CTX][len];
        if (r->live && (!best || r->score > best->score ||
            (r->score == best->score && r->move < best->move)))
            best = r;
    }
    if (!best) {
        fprintf(stderr, "netta: no exact route through observable wake\n");
        exit(1);
    }
    return best->move;
}

static void build_move_dist(int64_t prev, int range, int alphabet,
                            int search,
                            double move_p[ACTIONS + MAX_UNITS]) {
    /* The searched policy multiplies each current prior by the strongest
       one-move continuation available from that candidate, then renormalizes.
       This is a model-only rollout: candidate identities and learned pair
       counts are visible, future world bytes are not. The external court
       prices the resulting policy, so a confident wrong attractor remains
       expensive and cannot earn authority from its internal score. Dead
       units hold probability zero. Their frozen counts remain historical
       evidence, but body 13 removes that mass from every living denominator;
       resurrection restores it under the same identity. */
    double sum = 0.0;
    int tombs = range - alphabet;
    for (int c = 0; c < range; ++c) {
        if (c >= ACTIONS && units[c - ACTIONS].dead) {
            move_p[c] = 0.0;
            continue;
        }
        move_p[c] = move_prob(prev, (uint32_t)c, alphabet);
        if (search) {
            double best_future = 0.0;
            for (int d = 0; d < range; ++d) {
                if (d >= ACTIONS && units[d - ACTIONS].dead) continue;
                double p = move_prob(c, (uint32_t)d, alphabet);
                if (p > best_future) best_future = p;
            }
            move_p[c] *= best_future;
        }
        sum += move_p[c];
    }
    if (search || tombs) {
        for (int c = 0; c < range; ++c) move_p[c] /= sum;
        sum = 0.0;
        for (int c = 0; c < range; ++c) sum += move_p[c];
    }
    if (fabs(sum - 1.0) > 1e-6) {
        fprintf(stderr, "netta: move policy is not a distribution "
                        "(sum=%.9f)\n", sum);
        exit(1);
    }
}

static void emit_move(uint32_t m, uint64_t pos, int isl,
                      double atomic_ref_bits) {
    /* prequential shadow price, strictly before the count update; both the
       Laplace alphabet and its evidence mass are living */
    uint64_t live_or_history = tombstone_silence_enabled
                             ? move_live_total : move_total;
    double denom = (double)live_or_history +
                   (double)(ACTIONS + unit_living);
    double pm = ((double)move_count[m] + 1.0) / denom;
    double nll = -log2(pm);
    unitlm_bits += nll;
    unitlm_bytes += move_len(m);
    move_count[m]++;
    move_total++;
    move_live_total++;
    if (m >= ACTIONS) {
        Unit *u = &units[m - ACTIONS];
        u->uses++;
        u->last_use = steps_total;
        macro_events++;
        macro_bytes += u->len;
        char line[128];
        snprintf(line, sizeof line, "m\t%llu\t%d\t%llu\t%d\t%u\t%.6f\n",
                 (unsigned long long)episode_no, isl,
                 (unsigned long long)pos, (int)(m - ACTIONS), u->len,
                 nll);
        bio_append(line);
    }
    moves_emitted++;
    if (prev_move >= 0) {
        /* move-bigram shadow price, strictly before pair_feed updates
           the pair count that is its own statistic */
        uint32_t pv = (uint32_t)prev_move;
        uint64_t live_row_or_history = tombstone_silence_enabled
                                     ? mv_live_out[pv] : mv_out[pv];
        double d2 = (double)live_row_or_history +
                    (double)(ACTIONS + unit_living);
        double p2 = ((double)pair_get(pv, m) + 1.0) / d2;
        double move_bits = -log2(p2);
        mvlm_bits += move_bits;
        mvlm_bytes += move_len(m);
        mvlm_ref_bits += atomic_ref_bits;
        mvlm_ref_bytes += move_len(m);
        isl_mvlm_bits[isl] += move_bits;
        isl_mvlm_ref_bits[isl] += atomic_ref_bits;
        isl_mvlm_bytes[isl] += move_len(m);
        mv_out[pv]++;
        mv_live_out[pv]++;
        pair_feed(pv, m);
    }
    prev_move = (int64_t)m;
}

static void matcher_flush_front(void) {
    /* longest living unit equal to a prefix of mbuf; else one atomic */
    uint32_t best = 1; int best_u = -1;
    for (uint32_t k = mlen; k >= 2; --k) {
        int u = unit_find_living(mbuf, k);
        if (u >= 0) { best = k; best_u = u; break; }
    }
    double atomic_ref_bits = 0.0;
    for (uint32_t j = 0; j < best; ++j)
        atomic_ref_bits += mbuf_atomic_bits[j];
    if (best_u >= 0) emit_move((uint32_t)(ACTIONS + best_u),
                               mstart_pos, mstart_isl, atomic_ref_bits);
    else emit_move((uint32_t)mbuf[0], mstart_pos, mstart_isl,
                   atomic_ref_bits);
    memmove(mbuf, mbuf + best, mlen - best);
    memmove(mbuf_atomic_bits, mbuf_atomic_bits + best,
            (mlen - best) * sizeof mbuf_atomic_bits[0]);
    mlen -= best;
    mstart_pos += best;
}

static void matcher_feed(uint8_t truth, uint64_t pos, int isl,
                         double atomic_bits) {
    if (mlen == 0) { mstart_pos = pos; mstart_isl = isl; }
    mbuf[mlen] = truth;
    mbuf_atomic_bits[mlen] = atomic_bits;
    mlen++;
    while (mlen > 0 &&
           (mlen == UNIT_MAX_LEN || !unit_prefix_alive(mbuf, mlen))) {
        if (mlen < UNIT_MAX_LEN && unit_prefix_alive(mbuf, mlen)) break;
        if (mlen == UNIT_MAX_LEN && unit_find_living(mbuf, mlen) >= 0) {
            double atomic_ref_bits = 0.0;
            for (uint32_t j = 0; j < mlen; ++j)
                atomic_ref_bits += mbuf_atomic_bits[j];
            emit_move((uint32_t)(ACTIONS + unit_find_living(mbuf, mlen)),
                      mstart_pos, mstart_isl, atomic_ref_bits);
            mlen = 0;
            break;
        }
        matcher_flush_front();
    }
}

static void matcher_end_episode(void) {
    while (mlen > 0) matcher_flush_front();
    prev_move = -1;   /* pairs never span an episode boundary */
}

/* --------------------------------------------------------------- state */

static uint64_t counts[ACTIONS];
static uint64_t counts_total = 0;
static uint64_t episode_no = 0;
static uint64_t steps_total = 0;

static void state_refuse(const char *path, const char *why) {
    fprintf(stderr, "netta: %s is inconsistent (%s); refusing\n", path, why);
    exit(1);
}

static void checked_add(uint64_t *sum, uint64_t value,
                        const char *path, const char *what) {
    if (UINT64_MAX - *sum < value) state_refuse(path, what);
    *sum += value;
}

static void live_mass_add_unit(int u) {
    uint32_t move = (uint32_t)(ACTIONS + u);
    if (UINT64_MAX - move_live_total < move_count[move]) {
        fprintf(stderr, "netta: live move mass overflow\n"); exit(1);
    }
    move_live_total += move_count[move];
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt && (uint32_t)pairs[i].key == move) {
            uint32_t prev = (uint32_t)(pairs[i].key >> 32);
            if (UINT64_MAX - mv_live_out[prev] < pairs[i].cnt) {
                fprintf(stderr, "netta: live row mass overflow\n"); exit(1);
            }
            mv_live_out[prev] += pairs[i].cnt;
        }
}

static void live_mass_remove_unit(int u) {
    uint32_t move = (uint32_t)(ACTIONS + u);
    if (move_live_total < move_count[move]) {
        fprintf(stderr, "netta: live move mass underflow\n"); exit(1);
    }
    move_live_total -= move_count[move];
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt && (uint32_t)pairs[i].key == move) {
            uint32_t prev = (uint32_t)(pairs[i].key >> 32);
            if (mv_live_out[prev] < pairs[i].cnt) {
                fprintf(stderr, "netta: live row mass underflow\n"); exit(1);
            }
            mv_live_out[prev] -= pairs[i].cnt;
        }
}

static void live_mass_rebuild(const char *path) {
    move_live_total = 0;
    memset(mv_live_out, 0, sizeof mv_live_out);
    for (int m = 0; m < ACTIONS + unit_count; ++m)
        if (m < ACTIONS || !units[m - ACTIONS].dead)
            checked_add(&move_live_total, move_count[m], path,
                        "live move mass overflow");
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt) {
            uint32_t prev = (uint32_t)(pairs[i].key >> 32);
            uint32_t cur = (uint32_t)pairs[i].key;
            if (cur < ACTIONS || !units[cur - ACTIONS].dead)
                checked_add(&mv_live_out[prev], pairs[i].cnt, path,
                            "live row mass overflow");
        }
}

static void score_agrees(const char *path, const char *what,
                         long double local_sum, double global) {
    /* The same positive prequential prices are accumulated globally and in
       island partitions. Their addition order can differ across voyages, so
       allow rounding noise but no score large enough to alter a verdict. */
    long double scale = fabsl(local_sum);
    if (fabsl((long double)global) > scale)
        scale = fabsl((long double)global);
    if (scale < 1.0L) scale = 1.0L;
    if (fabsl(local_sum - (long double)global) > 1e-9L + 1e-10L * scale)
        state_refuse(path, what);
}

static void state_save(const char *path) {
    size_t tmp_n = strlen(path) + 12;
    char *tmp = (char *)malloc(tmp_n);
    if (!tmp) { fprintf(stderr, "netta: oom\n"); exit(1); }
    snprintf(tmp, tmp_n, "%s.tmp.XXXXXX", path);
    int fd = mkstemp(tmp);
    if (fd < 0) {
        fprintf(stderr, "netta: cannot create state sibling for %s\n", path);
        exit(1);
    }
    FILE *f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        fprintf(stderr, "netta: cannot open state sibling for %s\n", path);
        exit(1);
    }
    uint32_t ver = STATE_VER;
    uint32_t law = neural_law();
    uint32_t nisl = (uint32_t)reg_count;
    uint32_t nunits = (uint32_t)unit_count;
    uint64_t core_witness = core_state_witness();
    if (fwrite(STATE_MAGIC, 1, 8, f) != 8 ||
        fwrite(&ver, sizeof ver, 1, f) != 1 ||
        fwrite(&law, sizeof law, 1, f) != 1 ||
        fwrite(&rng_state, sizeof rng_state, 1, f) != 1 ||
        fwrite(&episode_no, sizeof episode_no, 1, f) != 1 ||
        fwrite(&steps_total, sizeof steps_total, 1, f) != 1 ||
        fwrite(&bio_lines, sizeof bio_lines, 1, f) != 1 ||
        fwrite(&bio_chain, sizeof bio_chain, 1, f) != 1 ||
        fwrite(&counts_total, sizeof counts_total, 1, f) != 1 ||
        fwrite(counts, sizeof counts[0], ACTIONS, f) != ACTIONS ||
        fwrite(&nunits, sizeof nunits, 1, f) != 1) {
        fprintf(stderr, "netta: state write failed\n"); exit(1);
    }
    for (int u = 0; u < unit_count; ++u)
        if (fwrite(&units[u], sizeof(Unit), 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    uint64_t npairs = 0;
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt) npairs++;
    if (fwrite(&npairs, sizeof npairs, 1, f) != 1) {
        fprintf(stderr, "netta: state write failed\n"); exit(1);
    }
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt &&
            fwrite(&pairs[i], sizeof(Pair), 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    if (fwrite(&macro_events, sizeof macro_events, 1, f) != 1 ||
        fwrite(&macro_bytes, sizeof macro_bytes, 1, f) != 1 ||
        fwrite(&moves_emitted, sizeof moves_emitted, 1, f) != 1 ||
        fwrite(&births_rejected_cap, sizeof births_rejected_cap, 1, f)
            != 1 ||
        fwrite(move_count, sizeof move_count[0], ACTIONS + MAX_UNITS, f)
            != ACTIONS + MAX_UNITS ||
        fwrite(&move_total, sizeof move_total, 1, f) != 1 ||
        fwrite(&unitlm_bits, sizeof unitlm_bits, 1, f) != 1 ||
        fwrite(&unitlm_bytes, sizeof unitlm_bytes, 1, f) != 1 ||
        fwrite(&atomic_bits_lived, sizeof atomic_bits_lived, 1, f) != 1 ||
        fwrite(&atomic_bytes_lived, sizeof atomic_bytes_lived, 1, f)
            != 1 ||
        fwrite(bi_count, sizeof(uint64_t), ACTIONS * ACTIONS, f)
            != ACTIONS * ACTIONS ||
        fwrite(bi_row, sizeof bi_row[0], ACTIONS, f) != ACTIONS ||
        fwrite(&bilm_bits, sizeof bilm_bits, 1, f) != 1 ||
        fwrite(&bilm_bytes, sizeof bilm_bytes, 1, f) != 1 ||
        fwrite(mv_out, sizeof mv_out[0], ACTIONS + MAX_UNITS, f)
            != ACTIONS + MAX_UNITS ||
        fwrite(&mvlm_bits, sizeof mvlm_bits, 1, f) != 1 ||
        fwrite(&mvlm_bytes, sizeof mvlm_bytes, 1, f) != 1 ||
        fwrite(&mvlm_ref_bits, sizeof mvlm_ref_bits, 1, f) != 1 ||
        fwrite(&mvlm_ref_bytes, sizeof mvlm_ref_bytes, 1, f) != 1 ||
        fwrite(&mvp_bits, sizeof mvp_bits, 1, f) != 1 ||
        fwrite(&mvp_bytes, sizeof mvp_bytes, 1, f) != 1 ||
        fwrite(mvp_ref_bits, sizeof mvp_ref_bits[0], 3, f) != 3 ||
        fwrite(&mvp_ref_bytes, sizeof mvp_ref_bytes, 1, f) != 1 ||
        fwrite(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fwrite(ep_actor, sizeof ep_actor[0], 5, f) != 5 ||
        fwrite(tri_row, sizeof tri_row[0], 65536, f) != 65536 ||
        fwrite(&trilm_bits, sizeof trilm_bits, 1, f) != 1 ||
        fwrite(&trilm_bytes, sizeof trilm_bytes, 1, f) != 1 ||
        fwrite(core_Wxh, sizeof(float), CORE_HIDDEN * CORE_EMBED, f)
            != CORE_HIDDEN * CORE_EMBED ||
        fwrite(core_Whh, sizeof(float), CORE_HIDDEN * CORE_HIDDEN, f)
            != CORE_HIDDEN * CORE_HIDDEN ||
        fwrite(core_Who, sizeof(float), ACTIONS * CORE_HIDDEN, f)
            != ACTIONS * CORE_HIDDEN ||
        fwrite(&core_nll_ema, sizeof core_nll_ema, 1, f) != 1 ||
        fwrite(&core_bits, sizeof core_bits, 1, f) != 1 ||
        fwrite(&core_bytes, sizeof core_bytes, 1, f) != 1 ||
        fwrite(&core_witness, sizeof core_witness, 1, f) != 1 ||
        fwrite(&nisl, sizeof nisl, 1, f) != 1) {
        fprintf(stderr, "netta: state write failed\n"); exit(1);
    }
    uint64_t ntri = 0;
    for (uint64_t i = 0; i < TRI_SLOTS; ++i) if (tri[i].cnt) ntri++;
    if (fwrite(&ntri, sizeof ntri, 1, f) != 1) {
        fprintf(stderr, "netta: state write failed\n"); exit(1);
    }
    for (uint64_t i = 0; i < TRI_SLOTS; ++i)
        if (tri[i].cnt && fwrite(&tri[i], sizeof(Pair), 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    for (int i = 0; i < reg_count; ++i) {
        if (fwrite(&reg_digest[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&reg_witness[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&reg_len[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&isl_lived[i], sizeof isl_lived[0], 1, f) != 1 ||
            fwrite(isl_bits[i], sizeof(double), 3, f) != 3 ||
            fwrite(&isl_mvp_bits[i], sizeof(double), 1, f) != 1 ||
            fwrite(&isl_mvp_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(isl_mvp_ref_bits[i], sizeof(double), 3, f) != 3 ||
            fwrite(&isl_mvp_ref_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&isl_mvlm_bits[i], sizeof(double), 1, f) != 1 ||
            fwrite(&isl_mvlm_ref_bits[i], sizeof(double), 1, f) != 1 ||
            fwrite(&isl_mvlm_bytes[i], sizeof(uint64_t), 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    }
    if (jury_enabled) {
        uint64_t witness = jury_state_witness();
        if (fwrite(jury_gene, sizeof jury_gene[0], JURY_GENOMES, f)
                != JURY_GENOMES) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
        for (int g = 0; g < JURY_GENOMES; ++g) {
            JuryCore *c = &jury_core[g];
            if (fwrite(c->Wxh, sizeof(float), CORE_HIDDEN * CORE_EMBED, f)
                    != CORE_HIDDEN * CORE_EMBED ||
                fwrite(c->Whh, sizeof(float),
                       CORE_HIDDEN * CORE_HIDDEN, f)
                    != CORE_HIDDEN * CORE_HIDDEN ||
                fwrite(c->Who, sizeof(float), ACTIONS * CORE_HIDDEN, f)
                    != ACTIONS * CORE_HIDDEN ||
                fwrite(&c->nll_ema, sizeof c->nll_ema, 1, f) != 1 ||
                fwrite(&c->bits, sizeof c->bits, 1, f) != 1 ||
                fwrite(&c->bytes, sizeof c->bytes, 1, f) != 1 ||
                fwrite(c->row_abs_sum, sizeof c->row_abs_sum[0],
                       CORE_HIDDEN, f) != CORE_HIDDEN ||
                fwrite(&c->health_bytes, sizeof c->health_bytes, 1, f) != 1 ||
                fwrite(&c->near_sat, sizeof c->near_sat, 1, f) != 1 ||
                fwrite(&c->clamp_hits, sizeof c->clamp_hits, 1, f) != 1 ||
                fwrite(&c->gates_pos, sizeof c->gates_pos, 1, f) != 1 ||
                fwrite(&c->gates_neg, sizeof c->gates_neg, 1, f) != 1) {
                fprintf(stderr, "netta: state write failed\n"); exit(1);
            }
        }
        if (fwrite(&witness, sizeof witness, 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    }
    if (fclose(f) != 0) {
        fprintf(stderr, "netta: state close failed\n"); exit(1);
    }
    if (rename(tmp, path) != 0) {
        fprintf(stderr, "netta: cannot publish state %s\n", path); exit(1);
    }
    free(tmp);
}

static int state_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) return 0;
        fprintf(stderr, "netta: cannot open state %s; refusing\n", path);
        exit(1);
    }
    char magic[8];
    uint32_t ver, law, nisl, nunits;
    uint64_t stored_core_witness;
    if (fread(magic, 1, 8, f) != 8 || memcmp(magic, STATE_MAGIC, 8) != 0) {
        fprintf(stderr,
                "netta: %s is not NETTA ZERO state; refusing to touch it\n",
                path);
        exit(1);
    }
    if (fread(&ver, sizeof ver, 1, f) != 1 || ver != STATE_VER) {
        fprintf(stderr, "netta: %s has unknown version; refusing\n", path);
        exit(1);
    }
    if (fread(&law, sizeof law, 1, f) != 1)
        state_refuse(path, "truncated neural invocation law");
    if (law != neural_law())
        state_refuse(path, "neural invocation law changed");
    if (fread(&rng_state, sizeof rng_state, 1, f) != 1 ||
        fread(&episode_no, sizeof episode_no, 1, f) != 1 ||
        fread(&steps_total, sizeof steps_total, 1, f) != 1 ||
        fread(&bio_lines, sizeof bio_lines, 1, f) != 1 ||
        fread(&bio_chain, sizeof bio_chain, 1, f) != 1 ||
        fread(&counts_total, sizeof counts_total, 1, f) != 1 ||
        fread(counts, sizeof counts[0], ACTIONS, f) != ACTIONS ||
        fread(&nunits, sizeof nunits, 1, f) != 1) {
        fprintf(stderr, "netta: %s truncated; refusing\n", path);
        exit(1);
    }
    if (nunits > MAX_UNITS) {
        fprintf(stderr, "netta: %s carries %u units (max %d); refusing\n",
                path, nunits, MAX_UNITS);
        exit(1);
    }
    for (uint32_t u = 0; u < nunits; ++u)
        if (fread(&units[u], sizeof(Unit), 1, f) != 1) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
    unit_count = (int)nunits;
    unit_living = 0;
    for (int u = 0; u < unit_count; ++u) {
        if (units[u].len == 0 || units[u].len > UNIT_MAX_LEN) {
            fprintf(stderr, "netta: %s carries a malformed unit; "
                            "refusing\n", path);
            exit(1);
        }
        if (units[u].born_episode > episode_no ||
            units[u].support_at_birth < BIRTH_SUPPORT)
            state_refuse(path, "unit provenance");
        if (units[u].dead > 1 || units[u].last_use > steps_total)
            state_refuse(path, "unit rent record");
        if (!units[u].dead) unit_living++;
        for (uint32_t j = 0; j < units[u].len; ++j)
            if (is_ws(units[u].bytes[j]))
                state_refuse(path, "unit crosses a boundary");
        for (int v = 0; v < u; ++v)
            if (units[v].len == units[u].len &&
                memcmp(units[v].bytes, units[u].bytes,
                       units[u].len) == 0)
                state_refuse(path, "duplicate unit identity");
    }
    uint64_t npairs;
    if (fread(&npairs, sizeof npairs, 1, f) != 1 || npairs > PAIR_SLOTS) {
        fprintf(stderr, "netta: %s truncated; refusing\n", path);
        exit(1);
    }
    for (uint64_t i = 0; i < npairs; ++i) {
        Pair e;
        if (fread(&e, sizeof e, 1, f) != 1 || e.cnt == 0) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
        uint32_t prev = (uint32_t)(e.key >> 32);
        uint32_t cur = (uint32_t)e.key;
        if (prev >= ACTIONS + nunits || cur >= ACTIONS + nunits)
            state_refuse(path, "pair names a nonexistent move");
        uint64_t h = fnv1a64((const uint8_t *)&e.key, sizeof e.key,
                             FNV_SEED);
        int inserted = 0;
        for (uint64_t probe = 0; probe < PAIR_SLOTS; ++probe) {
            Pair *p = &pairs[(h + probe) & (PAIR_SLOTS - 1)];
            if (p->cnt == 0) {
                *p = e; pair_used++; inserted = 1; break;
            }
            if (p->key == e.key)
                state_refuse(path, "duplicate pair key");
        }
        if (!inserted) state_refuse(path, "pair table has no free slot");
    }
    if (fread(&macro_events, sizeof macro_events, 1, f) != 1 ||
        fread(&macro_bytes, sizeof macro_bytes, 1, f) != 1 ||
        fread(&moves_emitted, sizeof moves_emitted, 1, f) != 1 ||
        fread(&births_rejected_cap, sizeof births_rejected_cap, 1, f)
            != 1 ||
        fread(move_count, sizeof move_count[0], ACTIONS + MAX_UNITS, f)
            != ACTIONS + MAX_UNITS ||
        fread(&move_total, sizeof move_total, 1, f) != 1 ||
        fread(&unitlm_bits, sizeof unitlm_bits, 1, f) != 1 ||
        fread(&unitlm_bytes, sizeof unitlm_bytes, 1, f) != 1 ||
        fread(&atomic_bits_lived, sizeof atomic_bits_lived, 1, f) != 1 ||
        fread(&atomic_bytes_lived, sizeof atomic_bytes_lived, 1, f)
            != 1 ||
        fread(bi_count, sizeof(uint64_t), ACTIONS * ACTIONS, f)
            != ACTIONS * ACTIONS ||
        fread(bi_row, sizeof bi_row[0], ACTIONS, f) != ACTIONS ||
        fread(&bilm_bits, sizeof bilm_bits, 1, f) != 1 ||
        fread(&bilm_bytes, sizeof bilm_bytes, 1, f) != 1 ||
        fread(mv_out, sizeof mv_out[0], ACTIONS + MAX_UNITS, f)
            != ACTIONS + MAX_UNITS ||
        fread(&mvlm_bits, sizeof mvlm_bits, 1, f) != 1 ||
        fread(&mvlm_bytes, sizeof mvlm_bytes, 1, f) != 1 ||
        fread(&mvlm_ref_bits, sizeof mvlm_ref_bits, 1, f) != 1 ||
        fread(&mvlm_ref_bytes, sizeof mvlm_ref_bytes, 1, f) != 1 ||
        fread(&mvp_bits, sizeof mvp_bits, 1, f) != 1 ||
        fread(&mvp_bytes, sizeof mvp_bytes, 1, f) != 1 ||
        fread(mvp_ref_bits, sizeof mvp_ref_bits[0], 3, f) != 3 ||
        fread(&mvp_ref_bytes, sizeof mvp_ref_bytes, 1, f) != 1 ||
        fread(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fread(ep_actor, sizeof ep_actor[0], 5, f) != 5 ||
        fread(tri_row, sizeof tri_row[0], 65536, f) != 65536 ||
        fread(&trilm_bits, sizeof trilm_bits, 1, f) != 1 ||
        fread(&trilm_bytes, sizeof trilm_bytes, 1, f) != 1 ||
        fread(core_Wxh, sizeof(float), CORE_HIDDEN * CORE_EMBED, f)
            != CORE_HIDDEN * CORE_EMBED ||
        fread(core_Whh, sizeof(float), CORE_HIDDEN * CORE_HIDDEN, f)
            != CORE_HIDDEN * CORE_HIDDEN ||
        fread(core_Who, sizeof(float), ACTIONS * CORE_HIDDEN, f)
            != ACTIONS * CORE_HIDDEN ||
        fread(&core_nll_ema, sizeof core_nll_ema, 1, f) != 1 ||
        fread(&core_bits, sizeof core_bits, 1, f) != 1 ||
        fread(&core_bytes, sizeof core_bytes, 1, f) != 1 ||
        fread(&stored_core_witness, sizeof stored_core_witness, 1, f) != 1 ||
        fread(&nisl, sizeof nisl, 1, f) != 1) {
        fprintf(stderr, "netta: %s truncated; refusing\n", path);
        exit(1);
    }
    uint64_t ntri;
    if (fread(&ntri, sizeof ntri, 1, f) != 1 || ntri > TRI_SLOTS) {
        fprintf(stderr, "netta: %s truncated; refusing\n", path);
        exit(1);
    }
    for (uint64_t i = 0; i < ntri; ++i) {
        Pair e;
        if (fread(&e, sizeof e, 1, f) != 1 || e.cnt == 0) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
        if (e.key > 0xffffffULL)
            state_refuse(path, "trigram key is outside the byte world");
        uint64_t h = fnv1a64((const uint8_t *)&e.key, sizeof e.key,
                             FNV_SEED);
        int inserted = 0;
        for (uint64_t probe = 0; probe < TRI_SLOTS; ++probe) {
            Pair *p = &tri[(h + probe) & (TRI_SLOTS - 1)];
            if (p->cnt == 0) {
                *p = e; tri_used++; inserted = 1; break;
            }
            if (p->key == e.key)
                state_refuse(path, "duplicate trigram key");
        }
        if (!inserted) state_refuse(path, "trigram table has no free slot");
    }
    if (nisl > (uint32_t)MAX_REGISTRY)
        state_refuse(path, "registry larger than the constitution");
    reg_count = (int)nisl;
    for (int i = 0; i < reg_count; ++i) {
        if (fread(&reg_digest[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(&reg_witness[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(&reg_len[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(&isl_lived[i], sizeof isl_lived[0], 1, f) != 1 ||
            fread(isl_bits[i], sizeof(double), 3, f) != 3 ||
            fread(&isl_mvp_bits[i], sizeof(double), 1, f) != 1 ||
            fread(&isl_mvp_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(isl_mvp_ref_bits[i], sizeof(double), 3, f) != 3 ||
            fread(&isl_mvp_ref_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(&isl_mvlm_bits[i], sizeof(double), 1, f) != 1 ||
            fread(&isl_mvlm_ref_bits[i], sizeof(double), 1, f) != 1 ||
            fread(&isl_mvlm_bytes[i], sizeof(uint64_t), 1, f) != 1) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
        for (int r = 0; r < i; ++r)
            if (reg_digest[r] == reg_digest[i] &&
                reg_witness[r] == reg_witness[i] &&
                reg_len[r] == reg_len[i]) {
            fprintf(stderr, "netta: %s carries a duplicate island identity; "
                            "refusing\n", path);
                exit(1);
            }
    }
    if (jury_enabled) {
        JuryGene stored_gene[JURY_GENOMES];
        uint64_t stored_jury_witness;
        if (fread(stored_gene, sizeof stored_gene[0], JURY_GENOMES, f)
                != JURY_GENOMES)
            state_refuse(path, "truncated jury genes");
        if (memcmp(stored_gene, jury_gene, sizeof stored_gene) != 0)
            state_refuse(path, "jury genes disagree with the sealed court");
        for (int g = 0; g < JURY_GENOMES; ++g) {
            JuryCore *c = &jury_core[g];
            if (fread(c->Wxh, sizeof(float), CORE_HIDDEN * CORE_EMBED, f)
                    != CORE_HIDDEN * CORE_EMBED ||
                fread(c->Whh, sizeof(float), CORE_HIDDEN * CORE_HIDDEN, f)
                    != CORE_HIDDEN * CORE_HIDDEN ||
                fread(c->Who, sizeof(float), ACTIONS * CORE_HIDDEN, f)
                    != ACTIONS * CORE_HIDDEN ||
                fread(&c->nll_ema, sizeof c->nll_ema, 1, f) != 1 ||
                fread(&c->bits, sizeof c->bits, 1, f) != 1 ||
                fread(&c->bytes, sizeof c->bytes, 1, f) != 1 ||
                fread(c->row_abs_sum, sizeof c->row_abs_sum[0],
                      CORE_HIDDEN, f) != CORE_HIDDEN ||
                fread(&c->health_bytes, sizeof c->health_bytes, 1, f) != 1 ||
                fread(&c->near_sat, sizeof c->near_sat, 1, f) != 1 ||
                fread(&c->clamp_hits, sizeof c->clamp_hits, 1, f) != 1 ||
                fread(&c->gates_pos, sizeof c->gates_pos, 1, f) != 1 ||
                fread(&c->gates_neg, sizeof c->gates_neg, 1, f) != 1)
                state_refuse(path, "truncated jury memory");
        }
        if (fread(&stored_jury_witness, sizeof stored_jury_witness, 1, f)
                != 1)
            state_refuse(path, "truncated jury witness");
        if (stored_jury_witness != jury_state_witness())
            state_refuse(path, "jury memory disagrees with its witness");
    }
    int extra = fgetc(f);
    if (extra != EOF || ferror(f))
        state_refuse(path, "trailing or unreadable bytes");
    if (fclose(f) != 0) state_refuse(path, "close failure");

    if (actor_current < 0 || actor_current > 3)
        state_refuse(path, "actor seat");
    uint64_t sum = 0;
    for (int b = 0; b < ACTIONS; ++b)
        checked_add(&sum, counts[b], path, "atomic count overflow");
    if (sum != counts_total || counts_total != steps_total ||
        atomic_bytes_lived != steps_total || bilm_bytes != steps_total ||
        trilm_bytes != steps_total)
        state_refuse(path, "raw-byte totals disagree");
    if (unitlm_bytes > steps_total || mvlm_bytes > unitlm_bytes ||
        mvlm_ref_bytes != mvlm_bytes ||
        mvp_bytes > steps_total || mvp_ref_bytes != mvp_bytes ||
        macro_bytes > steps_total ||
        macro_events > moves_emitted)
        state_refuse(path, "move totals exceed lived truth");
    sum = 0;
    for (int m = 0; m < ACTIONS + MAX_UNITS; ++m)
        checked_add(&sum, move_count[m], path, "move count overflow");
    if (sum != move_total || move_total != moves_emitted)
        state_refuse(path, "move totals disagree");
    for (int m = ACTIONS + unit_count; m < ACTIONS + MAX_UNITS; ++m)
        if (move_count[m] != 0 || mv_out[m] != 0)
            state_refuse(path, "unborn move carries statistics");
    uint64_t actor_eps = 0;
    for (int a = 0; a < 5; ++a)
        checked_add(&actor_eps, ep_actor[a], path,
                    "actor episode overflow");
    if (actor_eps != episode_no)
        state_refuse(path, "actor episodes disagree");
    uint64_t isl_lived_sum = 0, isl_mv_sum = 0, isl_shadow_sum = 0;
    long double isl_score_sum[3] = {0.0L, 0.0L, 0.0L};
    long double isl_mv_score_sum = 0.0L;
    long double isl_mv_ref_sum[3] = {0.0L, 0.0L, 0.0L};
    long double isl_shadow_score_sum = 0.0L;
    long double isl_shadow_ref_sum = 0.0L;
    for (int i = 0; i < reg_count; ++i) {
        checked_add(&isl_lived_sum, isl_lived[i], path,
                    "island lived overflow");
        checked_add(&isl_mv_sum, isl_mvp_bytes[i], path,
                    "island move overflow");
        checked_add(&isl_shadow_sum, isl_mvlm_bytes[i], path,
                    "island shadow overflow");
        if (isl_mvp_ref_bytes[i] != isl_mvp_bytes[i] ||
            isl_mvp_bytes[i] > isl_lived[i])
            state_refuse(path, "island move evidence disagrees");
        for (int c = 0; c < 3; ++c)
            if (!isfinite(isl_bits[i][c]) || isl_bits[i][c] < 0.0 ||
                !isfinite(isl_mvp_ref_bits[i][c]) ||
                isl_mvp_ref_bits[i][c] < 0.0)
                state_refuse(path, "non-finite island record");
            else {
                isl_score_sum[c] += (long double)isl_bits[i][c];
                isl_mv_ref_sum[c] +=
                    (long double)isl_mvp_ref_bits[i][c];
            }
        if (!isfinite(isl_mvp_bits[i]) || isl_mvp_bits[i] < 0.0)
            state_refuse(path, "non-finite island record");
        isl_mv_score_sum += (long double)isl_mvp_bits[i];
        if (!isfinite(isl_mvlm_bits[i]) || isl_mvlm_bits[i] < 0.0 ||
            !isfinite(isl_mvlm_ref_bits[i]) ||
            isl_mvlm_ref_bits[i] < 0.0 ||
            isl_mvlm_bytes[i] > isl_lived[i])
            state_refuse(path, "island shadow record");
        isl_shadow_score_sum += (long double)isl_mvlm_bits[i];
        isl_shadow_ref_sum += (long double)isl_mvlm_ref_bits[i];
    }
    if (isl_lived_sum != steps_total || isl_mv_sum != mvp_bytes ||
        isl_shadow_sum != mvlm_bytes)
        state_refuse(path, "island records disagree with the life");
    score_agrees(path, "island atomic score disagrees",
                 isl_score_sum[0], atomic_bits_lived);
    score_agrees(path, "island bigram score disagrees",
                 isl_score_sum[1], bilm_bits);
    score_agrees(path, "island trigram score disagrees",
                 isl_score_sum[2], trilm_bits);
    score_agrees(path, "island move score disagrees",
                 isl_mv_score_sum, mvp_bits);
    for (int c = 0; c < 3; ++c)
        score_agrees(path, "island move reference score disagrees",
                     isl_mv_ref_sum[c], mvp_ref_bits[c]);
    score_agrees(path, "island shadow score disagrees",
                 isl_shadow_score_sum, mvlm_bits);
    score_agrees(path, "island shadow reference score disagrees",
                 isl_shadow_ref_sum, mvlm_ref_bits);
    if (!isfinite(unitlm_bits) || unitlm_bits < 0.0 ||
        !isfinite(atomic_bits_lived) || atomic_bits_lived < 0.0 ||
        !isfinite(bilm_bits) || bilm_bits < 0.0 ||
        !isfinite(mvlm_bits) || mvlm_bits < 0.0 ||
        !isfinite(mvlm_ref_bits) || mvlm_ref_bits < 0.0 ||
        !isfinite(mvp_bits) || mvp_bits < 0.0 ||
        !isfinite(mvp_ref_bits[0]) || mvp_ref_bits[0] < 0.0 ||
        !isfinite(mvp_ref_bits[1]) || mvp_ref_bits[1] < 0.0 ||
        !isfinite(mvp_ref_bits[2]) || mvp_ref_bits[2] < 0.0 ||
        !isfinite(trilm_bits) || trilm_bits < 0.0)
        state_refuse(path, "non-finite model record");
    if (!isfinite(core_bits) || core_bits < 0.0 ||
        core_bytes > steps_total ||
        !isfinite(core_nll_ema) || core_nll_ema < 0.0 ||
        core_nll_ema > 16.0)
        state_refuse(path, "core record");
    for (int j = 0; j < CORE_HIDDEN; ++j) {
        for (int d = 0; d < CORE_EMBED; ++d)
            if (!isfinite(core_Wxh[j][d]) ||
                fabsf(core_Wxh[j][d]) > CORE_WCLAMP)
                state_refuse(path, "core weight out of law");
        for (int k = 0; k < CORE_HIDDEN; ++k)
            if (!isfinite(core_Whh[j][k]) ||
                fabsf(core_Whh[j][k]) > CORE_WCLAMP)
                state_refuse(path, "core weight out of law");
    }
    for (int b = 0; b < ACTIONS; ++b) {
        for (int d = 0; d < CORE_EMBED; ++d)
            if (!isfinite(core_E[b][d]))
                state_refuse(path, "core weight out of law");
        for (int j = 0; j < CORE_HIDDEN; ++j)
            if (!isfinite(core_Who[b][j]) ||
                fabsf(core_Who[b][j]) > CORE_WCLAMP)
                state_refuse(path, "core weight out of law");
    }
    if (stored_core_witness != core_state_witness())
        state_refuse(path, "core memory disagrees with its witness");
    if (jury_enabled) {
        for (int g = 0; g < JURY_GENOMES; ++g) {
            JuryCore *c = &jury_core[g];
            if (!isfinite(c->bits) || c->bits < 0.0 ||
                !isfinite(c->nll_ema) || c->nll_ema < 0.0 ||
                c->nll_ema > 16.0 || c->bytes != steps_total ||
                c->health_bytes != c->bytes ||
                c->bytes > UINT64_MAX / CORE_HIDDEN ||
                c->near_sat > c->bytes * CORE_HIDDEN)
                state_refuse(path, "jury record");
            if (c->gates_pos > c->bytes || c->gates_neg > c->bytes ||
                c->gates_pos > c->bytes - c->gates_neg)
                state_refuse(path, "jury gate record");
            for (int j = 0; j < CORE_HIDDEN; ++j) {
                if (!isfinite(c->row_abs_sum[j]) ||
                    c->row_abs_sum[j] < 0.0 ||
                    c->row_abs_sum[j] > (double)c->health_bytes + 1e-6)
                    state_refuse(path, "jury health record");
                for (int d = 0; d < CORE_EMBED; ++d)
                    if (!isfinite(c->Wxh[j][d]) ||
                        fabsf(c->Wxh[j][d]) > CORE_WCLAMP)
                        state_refuse(path, "jury weight out of law");
                for (int k = 0; k < CORE_HIDDEN; ++k)
                    if (!isfinite(c->Whh[j][k]) ||
                        fabsf(c->Whh[j][k]) > CORE_WCLAMP)
                        state_refuse(path, "jury weight out of law");
            }
            for (int b = 0; b < ACTIONS; ++b)
                for (int j = 0; j < CORE_HIDDEN; ++j)
                    if (!isfinite(c->Who[b][j]) ||
                        fabsf(c->Who[b][j]) > CORE_WCLAMP)
                        state_refuse(path, "jury weight out of law");
        }
    }
    for (int pv = 0; pv < ACTIONS; ++pv) {
        uint64_t row = 0;
        for (int b = 0; b < ACTIONS; ++b)
            checked_add(&row, bi_count[pv][b], path,
                        "bigram row overflow");
        if (row != bi_row[pv]) state_refuse(path, "bigram row total");
    }
    uint64_t *pair_rows = (uint64_t *)calloc(ACTIONS + MAX_UNITS,
                                              sizeof(uint64_t));
    uint64_t *tri_rows = (uint64_t *)calloc(65536, sizeof(uint64_t));
    if (!pair_rows || !tri_rows) { fprintf(stderr, "netta: oom\n"); exit(1); }
    for (uint64_t i = 0; i < PAIR_SLOTS; ++i)
        if (pairs[i].cnt) {
            uint32_t prev = (uint32_t)(pairs[i].key >> 32);
            checked_add(&pair_rows[prev], pairs[i].cnt, path,
                        "pair row overflow");
        }
    for (int m = 0; m < ACTIONS + MAX_UNITS; ++m)
        if (pair_rows[m] != mv_out[m])
            state_refuse(path, "move-bigram row total");
    live_mass_rebuild(path);
    for (uint64_t i = 0; i < TRI_SLOTS; ++i)
        if (tri[i].cnt) {
            uint32_t ctx = (uint32_t)(tri[i].key >> 8);
            checked_add(&tri_rows[ctx], tri[i].cnt, path,
                        "trigram row overflow");
        }
    for (uint32_t ctx = 0; ctx < 65536; ++ctx)
        if (tri_rows[ctx] != tri_row[ctx])
            state_refuse(path, "trigram row total");
    free(pair_rows);
    free(tri_rows);
    return 1;
}

/* ---------------------------------------------------------------- game */

static void policy(double p[ACTIONS]) {
    double denom = (double)counts_total + (double)ACTIONS;
    double sum = 0.0;
    for (int b = 0; b < ACTIONS; ++b) {
        p[b] = ((double)counts[b] + 1.0) / denom;
        sum += p[b];
    }
    if (fabs(sum - 1.0) > 1e-6) {
        fprintf(stderr, "netta: policy is not a distribution (sum=%.9f)\n",
                sum);
        exit(1);
    }
}

static int sample(const double p[ACTIONS]) {
    double r = (double)(rng_next() >> 11) * (1.0 / 9007199254740992.0);
    double acc = 0.0;
    for (int b = 0; b < ACTIONS; ++b) {
        acc += p[b];
        if (r < acc) return b;
    }
    return ACTIONS - 1;
}

static void build_dists(Island *isl, uint64_t pos, double p_uni[ACTIONS],
                        double p_bi[ACTIONS], double p_tri[ACTIONS]) {
    uint8_t pv = isl->bytes[pos - 1];
    uint32_t tctx = ((uint32_t)isl->bytes[pos - 2] << 8) | pv;
    policy(p_uni);
    double d2 = (double)bi_row[pv] + (double)ACTIONS;
    double s2 = 0.0;
    for (int b = 0; b < ACTIONS; ++b) {
        p_bi[b] = ((double)bi_count[pv][b] + 1.0) / d2;
        s2 += p_bi[b];
    }
    if (fabs(s2 - 1.0) > 1e-6) {
        fprintf(stderr, "netta: bigram row is not a distribution "
                        "(sum=%.9f)\n", s2);
        exit(1);
    }
    double d3 = (double)tri_row[tctx] + (double)ACTIONS;
    double s3 = 0.0;
    for (int b = 0; b < ACTIONS; ++b) {
        p_tri[b] = ((double)tri_get(tctx, (uint8_t)b) + 1.0) / d3;
        s3 += p_tri[b];
    }
    if (fabs(s3 - 1.0) > 1e-6) {
        fprintf(stderr, "netta: trigram row is not a distribution "
                        "(sum=%.9f)\n", s3);
        exit(1);
    }
}

static void absorb_truth(int isl_id, Island *isl, uint64_t pos,
                         const double p_uni[ACTIONS],
                         const double p_bi[ACTIONS],
                         const double p_tri[ACTIONS]) {
    /* model prices stay prequential and actor-independent; every organ
       learns from the lived truth byte, blind to which move lived it */
    int truth = isl->bytes[pos];
    uint8_t pv = isl->bytes[pos - 1];
    uint32_t tctx = ((uint32_t)isl->bytes[pos - 2] << 8) | pv;
    double atomic_nll = -log2(p_uni[truth]);
    atomic_bits_lived += atomic_nll;
    atomic_bytes_lived++;
    bilm_bits += -log2(p_bi[truth]);
    bilm_bytes++;
    trilm_bits += -log2(p_tri[truth]);
    trilm_bytes++;
    isl_bits[isl_id][0] += atomic_nll;
    isl_bits[isl_id][1] += -log2(p_bi[truth]);
    isl_bits[isl_id][2] += -log2(p_tri[truth]);
    isl_lived[isl_id]++;
    counts[truth]++;
    counts_total++;
    steps_total++;
    bi_count[pv][truth]++;
    bi_row[pv]++;
    tri_add(tctx, (uint8_t)truth);
    if (core_enabled)
        core_absorb((uint8_t)truth);
    if (jury_enabled)
        jury_absorb((uint8_t)truth);
    if (units_enabled)
        matcher_feed((uint8_t)truth, pos, isl_id, atomic_nll);
}

static double run_episode(int isl_id, uint64_t steps) {
    Island *isl = &islands[isl_id];
    int reg = present_reg[isl_id];   /* records live under the identity */
    if (steps == 0 || isl->len <= CTX ||
        steps > isl->len - CTX - 1) {
        fprintf(stderr, "netta: island %d too small for %llu steps\n",
                isl_id, (unsigned long long)steps);
        exit(1);
    }
    uint64_t span = isl->len - CTX - steps;
    uint64_t start;
    if (fixed_start_set) {
        if (fixed_start < CTX || fixed_start >= isl->len ||
            steps > isl->len - fixed_start) {
            fprintf(stderr,
                    "netta: fixed start %llu cannot hold %llu steps on "
                    "island %d\n", (unsigned long long)fixed_start,
                    (unsigned long long)steps, isl_id);
            exit(1);
        }
        start = fixed_start;
    } else {
        start = CTX + (rng_next() % (span ? span : 1));
    }
    if (core_enabled) core_warm(isl, start);
    if (jury_enabled) jury_warm(isl, start);
    double bits = 0.0;
    char line[256];
    episode_no++;
    if (units_enabled && unit_death_enabled)
        for (int u = 0; u < unit_count; ++u) {
            if (units[u].dead ||
                steps_total - units[u].last_use < UNIT_TTL)
                continue;
            /* the vocabulary pays rent in recognition; a unit that the
               lived truth has stopped naming releases the alphabet */
            live_mass_remove_unit(u);
            units[u].dead = 1;
            unit_living--;
            unit_deaths++;
            snprintf(line, sizeof line, "d\t%llu\t%d\t%llu\t%llu\n",
                     (unsigned long long)episode_no, u,
                     (unsigned long long)units[u].uses,
                     (unsigned long long)(steps_total -
                                          units[u].last_use));
            bio_append(line);
        }
    actor_elect();
    int acting = island_court(reg, steps);
    int probation = 0;
    double door_move_bits = local_probation_enabled
                          ? isl_mvlm_bits[reg] : mvlm_bits;
    double door_ref_bits = local_probation_enabled
                         ? isl_mvlm_ref_bits[reg] : mvlm_ref_bits;
    uint64_t door_bytes = local_probation_enabled
                        ? isl_mvlm_bytes[reg] : mvlm_bytes;
    if (actor_lock < 0 && units_enabled && acting >= 0 && acting <= 2 &&
        episode_no % 8 == 7 && door_bytes >= ACTOR_MIN_BYTES &&
        door_ref_bits / (double)door_bytes -
        door_move_bits / (double)door_bytes >= ACTOR_GAIN) {
        /* the shadow record opens a rare, deterministic probation
           episode; only the record played here can earn the seat.
           Probation borrows the body, never the seat: the elected
           incumbent keeps its mandate through the trial. The promise is
           judged on this island unless the audit valve restores the old
           travelling door. */
        acting = 3;
        probation = 1;
    }
    ep_actor[acting]++;
    snprintf(line, sizeof line, "a\t%llu\t%s\n",
             (unsigned long long)episode_no,
             probation ? "mvp" : actor_name[acting]);
    bio_append(line);
    if (acting == 3) {
        /* semi-Markov walk: the actor emits whole moves; the world
           advances by the matched prefix, never less than one byte, so
           a wrong long move is never cheaper than the same wrong bytes
           (the price is paid in full, the advance shrinks). */
        uint64_t s = 0;
        int64_t player_prev = -1; /* generation carries its own history */
        double p_uni[ACTIONS], p_bi[ACTIONS], p_tri[ACTIONS];
        while (s < steps) {
            uint64_t pos = start + s;
            int range = ACTIONS + unit_count;
            int alphabet = ACTIONS + unit_living;
            double r, acc;
            double move_p[ACTIONS + MAX_UNITS];
            uint32_t m = 0;
            int64_t policy_prev = player_prev;
            if (move_nav_enabled) {
                policy_prev = (int64_t)move_route_anchor(isl, pos, range,
                                                         alphabet);
                move_nav_steps++;
                if (policy_prev >= ACTIONS) move_nav_unit_anchors++;
            }
            build_move_dist(policy_prev, range, alphabet,
                            move_nav_enabled, move_p);
            /* one rng draw per move; walk the cumulative mass */
            r = (double)(rng_next() >> 11) *
                (1.0 / 9007199254740992.0);
            acc = 0.0;
            m = 0;                       /* float-tail fallback: the last
                                            living move */
            for (int c = range - 1; c >= 0; --c)
                if (c < ACTIONS || !units[c - ACTIONS].dead) {
                    m = (uint32_t)c;
                    break;
                }
            for (int c = 0; c < range; ++c) {
                acc += move_p[c];
                if (r < acc) { m = (uint32_t)c; break; }
            }
            uint8_t tmp[1];
            const uint8_t *mb = move_bytes(m, tmp);
            uint32_t L = move_len(m);
            uint64_t room = steps - s;
            uint32_t matched = 0;
            while (matched < L && (uint64_t)matched < room &&
                   mb[matched] == isl->bytes[pos + matched])
                matched++;
            uint32_t advance = matched ? matched : 1;
            if ((uint64_t)advance > room) advance = (uint32_t)room;
            /* The emitted move is causal, but the judge prices external
               truth. Greedy truth_move is the already-declared canonical
               segmentation; pricing the sampled move would reward a
               confident lie identically on a matching and alien island. */
            uint32_t target = truth_move(isl, pos, room);
            double nll = -log2(move_p[target]);
            if (move_nav_enabled)
                snprintf(line, sizeof line,
                         "v\t%llu\t%d\t%llu\t%u\t%u\t%u\t%.6f\t%u\t%lld\n",
                         (unsigned long long)episode_no, reg,
                         (unsigned long long)pos, m, L, advance, nll, target,
                         (long long)policy_prev);
            else
                snprintf(line, sizeof line,
                         "v\t%llu\t%d\t%llu\t%u\t%u\t%u\t%.6f\t%u\n",
                         (unsigned long long)episode_no, reg,
                         (unsigned long long)pos, m, L, advance, nll, target);
            bio_append(line);
            bits += nll;
            if (actor_lock == 3) {
                mvc_bits += nll;
                mvc_bytes += advance;
            } else {
                mvp_bits += nll;
                mvp_bytes += advance;
                isl_mvp_bits[reg] += nll;
                isl_mvp_bytes[reg] += advance;
            }
            player_prev = (int64_t)m;
            for (uint32_t j = 0; j < advance; ++j) {
                build_dists(isl, pos + j, p_uni, p_bi, p_tri);
                int truth = isl->bytes[pos + j];
                if (actor_lock == 3) {
                    mvc_ref_bits[0] += -log2(p_uni[truth]);
                    mvc_ref_bits[1] += -log2(p_bi[truth]);
                    mvc_ref_bits[2] += -log2(p_tri[truth]);
                } else {
                    mvp_ref_bits[0] += -log2(p_uni[truth]);
                    mvp_ref_bits[1] += -log2(p_bi[truth]);
                    mvp_ref_bits[2] += -log2(p_tri[truth]);
                    mvp_ref_bytes++;
                    isl_mvp_ref_bits[reg][0] += -log2(p_uni[truth]);
                    isl_mvp_ref_bits[reg][1] += -log2(p_bi[truth]);
                    isl_mvp_ref_bits[reg][2] += -log2(p_tri[truth]);
                    isl_mvp_ref_bytes[reg]++;
                }
                absorb_truth(reg, isl, pos + j, p_uni, p_bi, p_tri);
            }
            s += advance;
        }
        if (units_enabled) matcher_end_episode();
        return bits / (double)steps;
    }
    for (uint64_t s = 0; s < steps; ++s) {
        uint64_t pos = start + s;
        uint64_t ctx_digest =
            fnv1a64(isl->bytes + pos - CTX, CTX, FNV_SEED);
        uint64_t rng_before = rng_state;
        double p_uni[ACTIONS], p_bi[ACTIONS], p_tri[ACTIONS];
        build_dists(isl, pos, p_uni, p_bi, p_tri);
        const double *p_act = acting == 2 ? p_tri
                            : acting == 1 ? p_bi
                            : acting == 0 ? p_uni : NULL;
        int action = acting == ACTOR_NULL
                   ? (int)(rng_next() >> 56) : sample(p_act);
        int truth = isl->bytes[pos];
        double loss = acting == ACTOR_NULL ? 8.0 : -log2(p_act[truth]);
        snprintf(line, sizeof line,
                 "%llu\t%llu\t%d\t%llu\t%016llx\t%d\t%d\t%.6f\t%016llx\t"
                 "atomic\t1\t%s\n",
                 (unsigned long long)episode_no, (unsigned long long)s,
                 reg, (unsigned long long)pos,
                 (unsigned long long)ctx_digest, action, truth, loss,
                 (unsigned long long)rng_before,
                 actor_name[acting]);
        bio_append(line);
        absorb_truth(reg, isl, pos, p_uni, p_bi, p_tri);
        bits += loss;
    }
    if (units_enabled) matcher_end_episode();
    return bits / (double)steps;
}

/* ---------------------------------------------------------------- main */

static uint64_t parse_u64(const char *flag, const char *s) {
    char *end = NULL;
    unsigned long long value;
    if (!s[0] || s[0] == '-') {
        fprintf(stderr, "netta: %s takes a non-negative integer\n", flag);
        exit(1);
    }
    errno = 0;
    value = strtoull(s, &end, 10);
    if (errno == ERANGE || !end || *end != '\0') {
        fprintf(stderr, "netta: invalid integer for %s: %s\n", flag, s);
        exit(1);
    }
    return (uint64_t)value;
}

static int parse_int(const char *flag, const char *s) {
    char *end = NULL;
    long value;
    errno = 0;
    value = strtol(s, &end, 10);
    if (errno == ERANGE || !s[0] || !end || *end != '\0' ||
        value < INT_MIN || value > INT_MAX) {
        fprintf(stderr, "netta: invalid integer for %s: %s\n", flag, s);
        exit(1);
    }
    return (int)value;
}

static int same_file(const char *a, const char *b) {
    struct stat sa, sb;
    if (strcmp(a, b) == 0) return 1;
    if (stat(a, &sa) != 0 || stat(b, &sb) != 0) return 0;
    return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
}

static int file_exists(const char *path) {
    struct stat s;
    if (stat(path, &s) == 0) return 1;
    if (errno == ENOENT) return 0;
    fprintf(stderr, "netta: cannot inspect %s; refusing\n", path);
    exit(1);
}

/* body 18: the mouth. Speech is a read-only instrument: the mouth
   prices nothing, learns nothing, appends nothing, saves nothing, and
   draws from its own dedicated stream -- the life cannot tell that it
   spoke. The elected seat speaks; an mv seat falls to its best byte
   witness, because the first mouth emits bytes. A newborn has nothing
   to say: speech is the product of a lived state, so the mouth demands
   a resumed life and refuses to meet new islands. The prompt warms the
   two bytes of context a byte hand can carry; deeper prompt use
   arrives only with unit and core speech, in a later body. */
static uint64_t speak_bytes = 0;
static uint64_t speak_seed = 1;
static const char *prompt_path = NULL;
static int speak_requested = 0;
static int speak_seed_set = 0;
/* Body 19 keeps the eighteenth body's law as an explicit red arm.  Laplace
   smoothing is honest when an external byte may be unseen; sampling its
   pseudo-counts as if they had been lived is a different act. */
static int speak_laplace = 0;
static const char *ear_path = NULL;
static const char *ear_context_path = NULL;
static int ear_twin_requested = 0;
static const char *court_path = NULL;
static int life_control_named = 0;

/* Body 22's repair: a question has one byte law even though mouth and ear
   retain different public verbs.  For a regular file only the final two
   bytes are semantically relevant, so read and verify that tail twice while
   its open-file identity and length remain fixed.  A non-regular input is a
   stream: EOF seals it and the final two observed bytes become the question.
   This keeps FIFOs and /dev/stdin honest without scanning an irrelevant
   multi-gigabyte prefix of an ordinary file. */
static void question_tail(const char *path, const char *name,
                          const char *law_name, uint8_t *p2, uint8_t *p1) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "netta: cannot open %s %s\n", name, path);
        exit(1);
    }
    struct stat before;
    if (fstat(fileno(f), &before) != 0) {
        fclose(f);
        fprintf(stderr, "netta: cannot read %s %s\n", name, path);
        exit(1);
    }

    if (S_ISREG(before.st_mode)) {
        if (before.st_size < 2) {
            if (fclose(f) != 0) {
                fprintf(stderr, "netta: cannot read %s %s\n", name, path);
                exit(1);
            }
            fprintf(stderr, "netta: %s needs at least two bytes\n",
                    law_name);
            exit(1);
        }
        uint8_t first[2], second[2];
        struct stat middle, after;
        int failed = fseeko(f, (off_t)-2, SEEK_END) != 0 ||
                     fread(first, 1, 2, f) != 2 ||
                     fstat(fileno(f), &middle) != 0 ||
                     before.st_dev != middle.st_dev ||
                     before.st_ino != middle.st_ino ||
                     before.st_size != middle.st_size ||
                     fseeko(f, (off_t)-2, SEEK_END) != 0 ||
                     fread(second, 1, 2, f) != 2 ||
                     fstat(fileno(f), &after) != 0 ||
                     middle.st_dev != after.st_dev ||
                     middle.st_ino != after.st_ino ||
                     middle.st_size != after.st_size;
        int changed = !failed && memcmp(first, second, 2) != 0;
        int close_failed = fclose(f) != 0;
        if (failed || close_failed) {
            fprintf(stderr, "netta: cannot read %s %s\n", name, path);
            exit(1);
        }
        if (changed) {
            fprintf(stderr, "netta: %s changed while being read\n", name);
            exit(1);
        }
        *p2 = first[0];
        *p1 = first[1];
        return;
    }

    int c, have = 0;
    while ((c = fgetc(f)) != EOF) {
        *p2 = *p1;
        *p1 = (uint8_t)c;
        if (have < 2) have++;
    }
    int read_failed = ferror(f);
    int close_failed = fclose(f) != 0;
    if (read_failed || close_failed) {
        fprintf(stderr, "netta: cannot read %s %s\n", name, path);
        exit(1);
    }
    if (have < 2) {
        fprintf(stderr, "netta: %s needs at least two bytes\n", law_name);
        exit(1);
    }
}

static uint64_t speak_next(uint64_t *s) {
    uint64_t z = (*s += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static uint64_t speak_bounded(uint64_t *s, uint64_t bound) {
    uint64_t threshold = (uint64_t)(0 - bound) % bound;
    for (;;) {
        uint64_t r = speak_next(s);
        if (r >= threshold) return r % bound;
    }
}

static int speak_supported(int hand, uint8_t p2, uint8_t p1,
                           uint64_t *s, int *used) {
    uint32_t tctx = ((uint32_t)p2 << 8) | p1;
    uint64_t total;

    /* Supported backoff is generative physiology, not a speech verdict.
       A hand emits only a continuation it actually lived at its deepest
       available context; an empty row backs off instead of inventing 256
       equally weighted memories. Integer draws make that support guarantee
       exact even when cumulative floating-point probabilities round down. */
    if (hand == 2 && tri_row[tctx]) {
        *used = 2;
        total = tri_row[tctx];
    } else if (hand >= 1 && bi_row[p1]) {
        *used = 1;
        total = bi_row[p1];
    } else {
        *used = 0;
        total = counts_total;
    }

    uint64_t draw = speak_bounded(s, total);
    for (int b = 0; b < ACTIONS; ++b) {
        uint64_t n = *used == 2 ? tri_get(tctx, (uint8_t)b)
                   : *used == 1 ? bi_count[p1][b] : counts[b];
        if (draw < n) return b;
        draw -= n;
    }
    fprintf(stderr, "netta: spoken support totals disagree\n");
    exit(1);
}

static void speak_laplace_distribution(int hand, uint8_t p2, uint8_t p1,
                                       double p_act[ACTIONS]) {
    if (hand == 1) {
        double d2 = (double)bi_row[p1] + (double)ACTIONS;
        for (int b = 0; b < ACTIONS; ++b)
            p_act[b] = ((double)bi_count[p1][b] + 1.0) / d2;
    } else if (hand == 2) {
        uint32_t tctx = ((uint32_t)p2 << 8) | p1;
        double d3 = (double)tri_row[tctx] + (double)ACTIONS;
        for (int b = 0; b < ACTIONS; ++b)
            p_act[b] = ((double)tri_get(tctx, (uint8_t)b) + 1.0) / d3;
    } else {
        policy(p_act);
    }
}

static void speak(void) {
    /* an unprompted mouth opens at the life's most-lived context; a
       cold zero context on a narrow world would wander outside the
       lived manifold with no road back */
    uint8_t p2 = 0, p1 = 0;
    uint64_t best = 0;
    for (int pv = 0; pv < ACTIONS; ++pv)
        for (int b = 0; b < ACTIONS; ++b)
            if (bi_count[pv][b] > best) {
                best = bi_count[pv][b];
                p2 = (uint8_t)pv;
                p1 = (uint8_t)b;
            }
    if (prompt_path) {
        /* body 22: the question's law, shared with the ear. A byte hand
           carries two context positions; a shorter prompt would leave a
           hidden byte of the most-lived opening inside the question. A
           question is at least two bytes, or it is absent and the
           opening is named cold. */
        question_tail(prompt_path, "prompt", "a prompt", &p2, &p1);
    }
    actor_elect();
    int hand = actor_current;
    if (hand == 3) {
        double lu = atomic_bits_lived / (double)atomic_bytes_lived;
        double lb = bilm_bytes ? bilm_bits / (double)bilm_bytes : 1e9;
        double lt = trilm_bytes ? trilm_bits / (double)trilm_bytes : 1e9;
        hand = 0;
        if (lb < lu) { hand = 1; lu = lb; }
        if (lt < lu) hand = 2;
    }
    fprintf(stderr, "speak: %llu bytes, hand %s, seed %llu, law %s\n",
            (unsigned long long)speak_bytes, actor_name[hand],
            (unsigned long long)speak_seed,
            speak_laplace ? "laplace-red" : "supported-backoff");
    uint64_t s = speak_seed;
    uint64_t support_order[3] = {0, 0, 0};
    for (uint64_t i = 0; i < speak_bytes; ++i) {
        int b;
        if (!speak_laplace) {
            int used;
            b = speak_supported(hand, p2, p1, &s, &used);
            support_order[used]++;
        } else {
            double p_act[ACTIONS];
            speak_laplace_distribution(hand, p2, p1, p_act);
            double r = (double)(speak_next(&s) >> 11) *
                       (1.0 / 9007199254740992.0);
            double acc = 0.0;
            b = ACTIONS - 1;
            for (int c = 0; c < ACTIONS; ++c) {
                acc += p_act[c];
                if (r < acc) { b = c; break; }
            }
        }
        putchar(b);
        p2 = p1; p1 = (uint8_t)b;
    }
    if (fflush(stdout) != 0) {
        fprintf(stderr, "netta: speak flush failed\n"); exit(1);
    }
    if (!speak_laplace)
        fprintf(stderr, "speak support: uni %llu, bi %llu, tri %llu\n",
                (unsigned long long)support_order[0],
                (unsigned long long)support_order[1],
                (unsigned long long)support_order[2]);
}

/* body 20: the island's ear. Every island can carry its own statistical
   judge, grown from its immutable tape and nothing else: a foreign prior
   cannot be smuggled through an object that is a pure function of the
   sealed shore. The ear prices a spoken stream with the island's own
   Laplace ladder -- pricing is where charging ignorance is honest -- and
   takes an exact substring census against the tape, which is fully known.
   A match is evidence of shared bytes, not by itself evidence of copying:
   the instrument holds no office, wires into no election, opens no state
   and writes nothing. */
#define EAR_MAX (1u << 14)
#define EAR_MATCH_MIN 16

/* The candidate is the bounded side of the exact-match problem.  Its suffix
   automaton has fewer than 2*n states and is scanned once by every shore.
   This replaces the first ear's O(shore*candidate) DP: the public 16384-byte
   ceiling must remain usable on a full real-text island, not merely correct
   on a tiny test world.  Transition zero means absent, so stored ids are
   shifted by one. */
typedef struct {
    size_t cap;
    uint32_t states, last;
    uint32_t *next, *len, *prefix, *best, *heard, *order, *count;
    int32_t *link;
} EarSam;

#define EAR_SAM_NONE UINT32_MAX

static uint32_t ear_sam_get(const EarSam *a, uint32_t state, uint8_t b) {
    uint32_t v = a->next[(size_t)state * ACTIONS + b];
    return v ? v - 1 : EAR_SAM_NONE;
}

static void ear_sam_set(EarSam *a, uint32_t state, uint8_t b,
                        uint32_t target) {
    a->next[(size_t)state * ACTIONS + b] = target + 1;
}

static void ear_sam_build(EarSam *a, const uint8_t *sp, size_t n) {
    memset(a, 0, sizeof *a);
    a->cap = n * 2;
    a->next = (uint32_t *)calloc(a->cap * ACTIONS, sizeof *a->next);
    a->len = (uint32_t *)calloc(a->cap, sizeof *a->len);
    a->prefix = (uint32_t *)calloc(n, sizeof *a->prefix);
    a->best = (uint32_t *)calloc(a->cap, sizeof *a->best);
    a->heard = (uint32_t *)calloc(a->cap, sizeof *a->heard);
    a->order = (uint32_t *)calloc(a->cap, sizeof *a->order);
    a->count = (uint32_t *)calloc(n + 1, sizeof *a->count);
    a->link = (int32_t *)malloc(a->cap * sizeof *a->link);
    if (!a->next || !a->len || !a->prefix || !a->best || !a->heard ||
            !a->order || !a->count || !a->link) {
        fprintf(stderr, "netta: oom\n");
        exit(1);
    }
    a->states = 1;
    a->last = 0;
    a->link[0] = -1;
    for (size_t i = 0; i < n; ++i) {
        uint8_t b = sp[i];
        if (a->states >= a->cap) {
            fprintf(stderr, "netta: ear automaton full\n");
            exit(1);
        }
        uint32_t cur = a->states++;
        a->len[cur] = a->len[a->last] + 1;
        int32_t p = (int32_t)a->last;
        while (p >= 0 && ear_sam_get(a, (uint32_t)p, b) == EAR_SAM_NONE) {
            ear_sam_set(a, (uint32_t)p, b, cur);
            p = a->link[p];
        }
        if (p < 0) {
            a->link[cur] = 0;
        } else {
            uint32_t q = ear_sam_get(a, (uint32_t)p, b);
            if (a->len[p] + 1 == a->len[q]) {
                a->link[cur] = (int32_t)q;
            } else {
                if (a->states >= a->cap) {
                    fprintf(stderr, "netta: ear automaton full\n");
                    exit(1);
                }
                uint32_t clone = a->states++;
                a->len[clone] = a->len[p] + 1;
                a->link[clone] = a->link[q];
                memcpy(&a->next[(size_t)clone * ACTIONS],
                       &a->next[(size_t)q * ACTIONS],
                       ACTIONS * sizeof *a->next);
                while (p >= 0 && ear_sam_get(a, (uint32_t)p, b) == q) {
                    ear_sam_set(a, (uint32_t)p, b, clone);
                    p = a->link[p];
                }
                a->link[q] = (int32_t)clone;
                a->link[cur] = (int32_t)clone;
            }
        }
        a->last = cur;
        a->prefix[i] = cur;
    }

    /* Counting sort by state length gives both suffix-link propagation
       orders without comparison-dependent behaviour. */
    for (uint32_t q = 0; q < a->states; ++q) a->count[a->len[q]]++;
    for (size_t i = 1; i <= n; ++i) a->count[i] += a->count[i - 1];
    for (uint32_t q = a->states; q-- > 0;)
        a->order[--a->count[a->len[q]]] = q;
}

static void ear_sam_match(EarSam *a, const Island *isl,
                          uint32_t maxend[EAR_MAX]) {
    memset(a->best, 0, a->states * sizeof *a->best);
    memset(a->heard, 0, a->states * sizeof *a->heard);
    uint32_t v = 0, length = 0;
    for (uint64_t i = 0; i < isl->len; ++i) {
        uint8_t b = isl->bytes[i];
        uint32_t to = ear_sam_get(a, v, b);
        while (v && to == EAR_SAM_NONE) {
            v = (uint32_t)a->link[v];
            if (length > a->len[v]) length = a->len[v];
            to = ear_sam_get(a, v, b);
        }
        if (to != EAR_SAM_NONE) {
            v = to;
            length++;
        } else {
            v = 0;
            length = 0;
        }
        if (length > a->best[v]) a->best[v] = length;
    }

    /* Every substring represented by a state shares its candidate end
       positions.  Propagate shore matches down suffix links, then carry the
       best suffix forward to each saved candidate-prefix state. */
    for (uint32_t oi = a->states; oi-- > 1;) {
        uint32_t q = a->order[oi];
        uint32_t p = (uint32_t)a->link[q];
        uint32_t carry = a->best[q];
        if (carry > a->len[p]) carry = a->len[p];
        if (carry > a->best[p]) a->best[p] = carry;
    }
    for (uint32_t oi = 1; oi < a->states; ++oi) {
        uint32_t q = a->order[oi];
        uint32_t p = (uint32_t)a->link[q];
        a->heard[q] = a->best[q] > a->heard[p] ?
                      a->best[q] : a->heard[p];
    }
    for (uint32_t i = 0; i < a->len[a->last]; ++i)
        maxend[i] = a->heard[a->prefix[i]];
}

static void ear_sam_free(EarSam *a) {
    free(a->next); free(a->len); free(a->prefix); free(a->best);
    free(a->heard); free(a->order); free(a->count); free(a->link);
}

static void ear_grow(const Island *isl) {
    memset(counts, 0, sizeof counts);
    counts_total = 0;
    memset(bi_count, 0, sizeof bi_count);
    memset(bi_row, 0, sizeof bi_row);
    memset(tri, 0, TRI_SLOTS * sizeof(Pair));
    memset(tri_row, 0, sizeof tri_row);
    tri_used = 0;
    for (uint64_t i = 0; i < isl->len; ++i) {
        uint8_t b = isl->bytes[i];
        counts[b]++;
        counts_total++;
        if (i >= 1) {
            uint8_t p1 = isl->bytes[i - 1];
            bi_count[p1][b]++;
            bi_row[p1]++;
            if (i >= 2)
                tri_add(((uint32_t)isl->bytes[i - 2] << 8) | p1, b);
        }
    }
}

static double ear_price(const Island *isl, const uint8_t *sp, size_t n,
                        int contextual, uint8_t cp2, uint8_t cp1) {
    ear_grow(isl);
    double bits = 0.0;
    uint8_t p2 = cp2, p1 = cp1;
    for (size_t i = 0; i < n; ++i) {
        uint8_t b = sp[i];
        double p;
        if (!contextual && i == 0)
            p = ((double)counts[b] + 1.0) /
                ((double)counts_total + (double)ACTIONS);
        else if (!contextual && i == 1)
            p = ((double)bi_count[p1][b] + 1.0) /
                ((double)bi_row[p1] + (double)ACTIONS);
        else {
            uint32_t t = ((uint32_t)p2 << 8) | p1;
            p = ((double)tri_get(t, b) + 1.0) /
                ((double)tri_row[t] + (double)ACTIONS);
        }
        bits += -log2(p);
        p2 = p1;
        p1 = b;
    }
    return bits;
}

#define EAR_TWIN_SEED 0x4e45545441475241ULL

/* Body 23: the Gutenberg arena's already sealed shuffle, grown locally for
   any island.  The null preserves the whole byte census while breaking
   positional order; changed positions are counted so a degenerate shore can
   never masquerade as a destroyed structure. */
static void ear_make_twin(const Island *isl, Island *twin,
                          uint64_t *changed) {
    memset(twin, 0, sizeof *twin);
    twin->len = isl->len;
    twin->bytes = (uint8_t *)malloc(isl->len ? (size_t)isl->len : 1);
    if (!twin->bytes) {
        fprintf(stderr, "netta: oom\n");
        exit(1);
    }
    if (isl->len) memcpy(twin->bytes, isl->bytes, (size_t)isl->len);
    uint64_t true_hist[ACTIONS] = {0}, twin_hist[ACTIONS] = {0};
    for (uint64_t i = 0; i < isl->len; ++i) true_hist[isl->bytes[i]]++;

    uint64_t s = EAR_TWIN_SEED;
    for (uint64_t span = twin->len; span > 1; --span) {
        uint64_t i = span - 1;
        uint64_t j = speak_next(&s) % span;
        uint8_t t = twin->bytes[i];
        twin->bytes[i] = twin->bytes[j];
        twin->bytes[j] = t;
    }
    *changed = 0;
    for (uint64_t i = 0; i < twin->len; ++i) {
        twin_hist[twin->bytes[i]]++;
        if (twin->bytes[i] != isl->bytes[i]) (*changed)++;
    }
    for (int b = 0; b < ACTIONS; ++b) {
        if (true_hist[b] != twin_hist[b]) {
            fprintf(stderr,
                    "netta: an island twin failed to conserve its census\n");
            exit(1);
        }
    }
    twin->digest = fnv1a64(twin->bytes, twin->len, FNV_SEED);
}

static void ear(void) {
    FILE *f = fopen(ear_path, "rb");
    if (!f) {
        fprintf(stderr, "netta: cannot open %s\n", ear_path);
        exit(1);
    }
    /* Read one byte past the public bound.  feof() is not set when fread
       fills its request exactly, so using it as the boundary witness would
       refuse EAR_MAX itself and conflate it with a truncated longer stream. */
    static uint8_t sp[EAR_MAX + 1];
    size_t n = fread(sp, 1, sizeof sp, f);
    int read_failed = ferror(f);
    int close_failed = fclose(f) != 0;
    if (read_failed || close_failed) {
        fprintf(stderr, "netta: cannot read %s\n", ear_path);
        exit(1);
    }
    if (n > EAR_MAX) {
        fprintf(stderr, "netta: the ear listens to at most %u bytes "
                        "at a sitting\n", EAR_MAX);
        exit(1);
    }
    if (n < 2) {
        fprintf(stderr, "netta: the ear needs at least two bytes\n");
        exit(1);
    }

    /* Body 21: the ear remembers the question.  A named context is a third
       immutable input, never life state: only its final two bytes enter the
       same trigram context the mouth carries.  Two bytes are required so no
       hidden cold byte or state-grown default can enter the hearing. */
    uint8_t cp2 = 0, cp1 = 0;
    int contextual = 0;
    if (ear_context_path) {
        question_tail(ear_context_path, "ear context", "ear context",
                      &cp2, &cp1);
        contextual = 1;
    }
    static uint32_t maxend[EAR_MAX];
    EarSam match;
    ear_sam_build(&match, sp, n);
    for (int k = 0; k < island_count; ++k) {
        Island *isl = &islands[k];
        double bits = ear_price(isl, sp, n, contextual, cp2, cp1);
        ear_sam_match(&match, isl, maxend);
        uint64_t longest = 0, covered = 0;
        size_t painted = 0;
        for (size_t s = 0; s < n; ++s) {
            if (maxend[s] > longest) longest = maxend[s];
            if (maxend[s] >= EAR_MATCH_MIN) {
                size_t le = s + 1 - maxend[s];
                if (le < painted) le = painted;
                covered += s + 1 - le;
                painted = s + 1;
            }
        }
        printf("ear %d: digest=%016llx context=", k,
               (unsigned long long)isl->digest);
        if (contextual) printf("%02x%02x", cp2, cp1);
        else printf("cold");
        printf(" bytes=%llu bits=%.6f "
               "bits/byte=%.6f longest-match=%llu matched16=%.1f%%",
               (unsigned long long)n,
               bits, bits / (double)n, (unsigned long long)longest,
               100.0 * (double)covered / (double)n);
        if (ear_twin_requested) {
            Island twin;
            uint64_t changed;
            ear_make_twin(isl, &twin, &changed);
            double twin_bits = ear_price(&twin, sp, n, contextual, cp2, cp1);
            printf(" twin-digest=%016llx twin-changed=%llu/%llu "
                   "twin-bits=%.6f twin-bits/byte=%.6f",
                   (unsigned long long)twin.digest,
                   (unsigned long long)changed,
                   (unsigned long long)twin.len,
                   twin_bits, twin_bits / (double)n);
            free(twin.bytes);
        }
        putchar('\n');
    }
    ear_sam_free(&match);
}

/* body 25: the pattern court's public warrant. Body 24 froze the
   four-word lattice from named calibration worlds before any candidate
   was read. This body leaves those words and thresholds untouched, but
   makes every deciding operand public and makes the structural null earn
   jurisdiction in the case actually heard. The court decides on integers:
   exact matched bytes and a once-rounded gap in microbits per byte.
   ABSTAIN when the twin changed nothing or moved the heard coordinate by
   less than the public resolution, REPLAY at exact half coverage, ORDER
   at 0.5 bits/byte, STRANGER otherwise. Verdicts remain measurement
   patterns, never causal accusations, and the court still holds no office:
   no election, no speech authority, no state, no writes. */
#define COURT_ORDER_MICRO 500000LL

/* body 26: every warrant names the law it was judged under. The
   canonical one-line text is hashed into a law-digest; amending a
   threshold changes the text, the digest, and thereby every receipt,
   so no verdict can pretend continuity with a law it never faced. */
static const char COURT_LAW[] =
    "pattern-court law v2: abstain if changed=0 or gap-micro=0; "
    "replay if 2*matched-bytes>=bytes; order if gap-micro>=500000; "
    "stranger otherwise";
static uint64_t court_law_digest;

static void court(void) {
    court_law_digest = fnv1a64((const uint8_t *)COURT_LAW,
                               (uint64_t)strlen(COURT_LAW), FNV_SEED);
    printf("court law: %s law-digest=%016llx\n", COURT_LAW,
           (unsigned long long)court_law_digest);
    FILE *f = fopen(court_path, "rb");
    if (!f) {
        fprintf(stderr, "netta: cannot open %s\n", court_path);
        exit(1);
    }
    static uint8_t sp[EAR_MAX + 1];
    size_t n = fread(sp, 1, sizeof sp, f);
    int read_failed = ferror(f);
    int close_failed = fclose(f) != 0;
    if (read_failed || close_failed) {
        fprintf(stderr, "netta: cannot read %s\n", court_path);
        exit(1);
    }
    if (n > EAR_MAX) {
        fprintf(stderr, "netta: the court hears at most %u bytes "
                        "at a sitting\n", EAR_MAX);
        exit(1);
    }
    if (n < 2) {
        fprintf(stderr, "netta: the court needs at least two bytes\n");
        exit(1);
    }
    uint8_t cp2 = 0, cp1 = 0;
    int contextual = 0;
    if (ear_context_path) {
        question_tail(ear_context_path, "ear context", "ear context",
                      &cp2, &cp1);
        contextual = 1;
    }
    static uint32_t maxend[EAR_MAX];
    EarSam match;
    ear_sam_build(&match, sp, n);
    for (int k = 0; k < island_count; ++k) {
        Island *isl = &islands[k];
        double bits = ear_price(isl, sp, n, contextual, cp2, cp1);
        ear_sam_match(&match, isl, maxend);
        uint64_t covered = 0;
        size_t painted = 0;
        for (size_t s = 0; s < n; ++s) {
            if (maxend[s] >= EAR_MATCH_MIN) {
                size_t le = s + 1 - maxend[s];
                if (le < painted) le = painted;
                covered += s + 1 - le;
                painted = s + 1;
            }
        }
        Island twin;
        uint64_t changed;
        ear_make_twin(isl, &twin, &changed);
        double twin_bits = ear_price(&twin, sp, n, contextual, cp2, cp1);
        free(twin.bytes);
        double p = bits / (double)n;
        double q = twin_bits / (double)n;
        double m = 100.0 * (double)covered / (double)n;
        int64_t gap_micro = (int64_t)llround(
            1000000.0 * (twin_bits - bits) / (double)n);
        double g = (double)gap_micro / 1000000.0;
        const char *verdict = changed == 0 || gap_micro == 0 ? "abstain"
                            : 2 * covered >= n                ? "replay"
                            : gap_micro >= COURT_ORDER_MICRO  ? "order"
                                                              : "stranger";
        /* body 26: the warrant's receipt. The line is sealed together
           with the law-digest it was judged under, so a transcribed
           warrant can be refused by an external hand on any changed
           operand, verdict, or silently amended law. */
        char line[512], ctx[8];
        if (contextual)
            snprintf(ctx, sizeof ctx, "%02x%02x", cp2, cp1);
        else
            snprintf(ctx, sizeof ctx, "cold");
        int ln = snprintf(line, sizeof line,
               "court %d: digest=%016llx context=%s bytes=%llu "
               "P=%.6f Q=%.6f matched16=%.4f%% "
               "matched-bytes=%llu/%llu G=%.6f gap-micro=%lld "
               "changed=%llu/%llu verdict=%s",
               k, (unsigned long long)isl->digest, ctx,
               (unsigned long long)n, p, q, m,
               (unsigned long long)covered, (unsigned long long)n, g,
               (long long)gap_micro, (unsigned long long)changed,
               (unsigned long long)isl->len, verdict);
        if (ln < 0 || (size_t)ln >= sizeof line) {
            fprintf(stderr, "netta: warrant line overflow\n");
            exit(1);
        }
        char sealed[640];
        int sn = snprintf(sealed, sizeof sealed, "%s law-digest=%016llx",
                          line, (unsigned long long)court_law_digest);
        if (sn < 0 || (size_t)sn >= sizeof sealed) {
            fprintf(stderr, "netta: warrant seal overflow\n");
            exit(1);
        }
        uint64_t receipt = fnv1a64((const uint8_t *)sealed,
                                   (uint64_t)sn, FNV_SEED);
        printf("%s receipt=%016llx\n", line,
               (unsigned long long)receipt);
    }
    ear_sam_free(&match);
}

int main(int argc, char **argv) {
    const char *state_path = "netta0.state";
    const char *bio_path = "netta0.bio.tsv";
    uint64_t seed = 1, episodes = 1, steps = 64;
    int isl_id = 0, reset = 0;
    int paths_n = 0;
    const char *paths[MAX_ISLANDS];

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc) {
            life_control_named = 1;
            seed = parse_u64("--seed", argv[++i]);
        }
        else if (!strcmp(argv[i], "--episodes") && i + 1 < argc) {
            life_control_named = 1;
            episodes = parse_u64("--episodes", argv[++i]);
        }
        else if (!strcmp(argv[i], "--steps") && i + 1 < argc) {
            life_control_named = 1;
            steps = parse_u64("--steps", argv[++i]);
        }
        else if (!strcmp(argv[i], "--island") && i + 1 < argc) {
            life_control_named = 1;
            isl_id = parse_int("--island", argv[++i]);
        }
        else if (!strcmp(argv[i], "--start") && i + 1 < argc) {
            life_control_named = 1;
            fixed_start = parse_u64("--start", argv[++i]);
            fixed_start_set = 1;
        }
        else if (!strcmp(argv[i], "--state") && i + 1 < argc) {
            life_control_named = 1;
            state_path = argv[++i];
        }
        else if (!strcmp(argv[i], "--bio") && i + 1 < argc) {
            life_control_named = 1;
            bio_path = argv[++i];
        }
        else if (!strcmp(argv[i], "--reset")) {
            life_control_named = 1;
            reset = 1;
        }
        else if (!strcmp(argv[i], "--atlas")) {
            life_control_named = 1;
            atlas_enabled = 1;
        }
        else if (!strcmp(argv[i], "--no-units")) {
            life_control_named = 1;
            units_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-mv-nav")) {
            life_control_named = 1;
            move_nav_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-island-court")) {
            life_control_named = 1;
            island_court_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-birth-floor")) {
            life_control_named = 1;
            birth_floor_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-local-probation")) {
            life_control_named = 1;
            local_probation_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-unit-death")) {
            life_control_named = 1;
            unit_death_enabled = 0;
        }
        else if (!strcmp(argv[i], "--keep-dead-mass")) {
            life_control_named = 1;
            tombstone_silence_enabled = 0;
        }
        else if (!strcmp(argv[i], "--no-core")) {
            life_control_named = 1;
            core_enabled = 0;
        }
        else if (!strcmp(argv[i], "--core-hebb-v1")) {
            life_control_named = 1;
            core_hebb_enabled = 1;
        }
        else if (!strcmp(argv[i], "--jury")) {
            life_control_named = 1;
            jury_enabled = 1;
        }
        else if (!strcmp(argv[i], "--speak") && i + 1 < argc) {
            speak_requested = 1;
            speak_bytes = parse_u64("--speak", argv[++i]);
        }
        else if (!strcmp(argv[i], "--speak-seed") && i + 1 < argc) {
            speak_seed_set = 1;
            speak_seed = parse_u64("--speak-seed", argv[++i]);
        }
        else if (!strcmp(argv[i], "--prompt-file") && i + 1 < argc)
            prompt_path = argv[++i];
        else if (!strcmp(argv[i], "--speak-laplace"))
            speak_laplace = 1;
        else if (!strcmp(argv[i], "--ear") && i + 1 < argc)
            ear_path = argv[++i];
        else if (!strcmp(argv[i], "--ear-context") && i + 1 < argc)
            ear_context_path = argv[++i];
        else if (!strcmp(argv[i], "--ear-twin"))
            ear_twin_requested = 1;
        else if (!strcmp(argv[i], "--court") && i + 1 < argc)
            court_path = argv[++i];
        else if (!strcmp(argv[i], "--actor-lock") && i + 1 < argc) {
            life_control_named = 1;
            ++i;
            if (!strcmp(argv[i], "uni")) actor_lock = 0;
            else if (!strcmp(argv[i], "bi")) actor_lock = 1;
            else if (!strcmp(argv[i], "tri")) actor_lock = 2;
            else if (!strcmp(argv[i], "mv")) actor_lock = 3;
            else {
                fprintf(stderr,
                        "netta: --actor-lock takes uni|bi|tri|mv\n");
                exit(1);
            }
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "netta: unknown flag %s\n", argv[i]);
            exit(1);
        } else if (paths_n < MAX_ISLANDS)
            paths[paths_n++] = argv[i];
    }

    if (ear_context_path && !ear_path && !court_path) {
        fprintf(stderr, "netta: --ear-context requires --ear or --court\n");
        exit(1);
    }
    if (ear_twin_requested && !ear_path) {
        fprintf(stderr, "netta: --ear-twin requires --ear\n");
        exit(1);
    }
    if (court_path) {
        if (ear_path || ear_twin_requested || speak_requested ||
                speak_seed_set || prompt_path || speak_laplace ||
                life_control_named) {
            fprintf(stderr,
                    "netta: the court accepts only shores, --court, and "
                    "--ear-context\n");
            exit(1);
        }
        if (paths_n == 0) {
            fprintf(stderr, "netta: the court needs a shore\n");
            exit(1);
        }
    }
    if (ear_path) {
        if (speak_requested || speak_seed_set || prompt_path ||
                speak_laplace || life_control_named) {
            fprintf(stderr,
                    "netta: the ear accepts only shores, --ear, and "
                    "--ear-context/--ear-twin\n");
            exit(1);
        }
        if (paths_n == 0) {
            fprintf(stderr, "netta: the ear needs a shore\n");
            exit(1);
        }
    }
    if (actor_lock == 3 && !units_enabled) {
        fprintf(stderr,
                "netta: --actor-lock mv requires units enabled\n");
        exit(1);
    }
    if (!speak_requested && (speak_seed_set || prompt_path || speak_laplace)) {
        fprintf(stderr, "netta: speech flags require --speak\n");
        exit(1);
    }
    if (speak_requested) fprintf(stderr, "NETTA ZERO\n");
    else printf("NETTA ZERO\n");
    if (paths_n == 0 && !speak_requested) {
        fprintf(stderr, "usage: netta <island.bytes>... [--seed N] "
                        "[--episodes N] [--steps N] [--island N] "
                        "[--start OFFSET] [--state P] [--bio P] [--reset] "
                        "[--atlas] [--no-core] [--core-hebb-v1] [--jury] "
                        "[--speak N] [--speak-seed N] [--prompt-file P] "
                        "[--speak-laplace] [--ear P] [--ear-context P] "
                        "[--ear-twin] [--court P] "
                        "[--no-units] [--no-mv-nav] [--no-island-court] "
                        "[--no-birth-floor] [--no-local-probation] "
                        "[--no-unit-death] [--keep-dead-mass] "
                        "[--actor-lock uni|bi|tri|mv]\n");
        exit(1);
    }
    pairs = (Pair *)calloc(PAIR_SLOTS, sizeof(Pair));
    if (!pairs) { fprintf(stderr, "netta: oom\n"); exit(1); }
    tri = (Pair *)calloc(TRI_SLOTS, sizeof(Pair));
    if (!tri) { fprintf(stderr, "netta: oom\n"); exit(1); }
    for (int i = 0; i < paths_n; ++i) island_load(paths[i]);
    /* A hearing cannot inspect ambient life defaults.  Enter the ear before
       any state/biography alias check or other life-only filesystem query. */
    if (court_path) {
        court();
        return 0;
    }
    if (ear_path) {
        ear();
        return 0;
    }
    if (same_file(state_path, bio_path)) {
        fprintf(stderr, "netta: state and biography must be distinct\n");
        exit(1);
    }
    for (int i = 0; i < paths_n; ++i) {
        if (same_file(paths[i], state_path) || same_file(paths[i], bio_path)) {
            fprintf(stderr,
                    "netta: an immutable island cannot also be state or "
                    "biography\n");
            exit(1);
        }
    }
    for (int i = 0; i < island_count; ++i)
        fprintf(speak_requested ? stderr : stdout,
                "island %d: %s len=%llu digest=%016llx\n", i,
                islands[i].name, (unsigned long long)islands[i].len,
                (unsigned long long)islands[i].digest);
    if (paths_n && (isl_id < 0 || isl_id >= island_count)) {
        fprintf(stderr, "netta: no island %d\n", isl_id);
        exit(1);
    }
    rng_state = seed;
    int resumed = 0;
    core_init();   /* innate identity is regenerated, never loaded */
    jury_init();   /* every genome begins on that exact same body */
    if (!reset) resumed = state_load(state_path);
    if (resumed) bio_verify(bio_path);
    if (!reset && !resumed && file_exists(bio_path)) {
        fprintf(stderr,
                "netta: biography %s exists without state; use --reset "
                "to begin a new life\n", bio_path);
        exit(1);
    }
    /* today's convoy is resolved against the life's registry; an
       unknown identity is an arrival, never a refusal */
    registry_resolve_convoy();
    if (speak_requested) {
        if (!resumed) {
            fprintf(stderr, "netta: the mouth needs a lived state\n");
            exit(1);
        }
        if (!atomic_bytes_lived || !counts_total) {
            fprintf(stderr, "netta: the mouth needs lived bytes\n");
            exit(1);
        }
        if (arrivals_pending_n) {
            fprintf(stderr, "netta: the mouth cannot meet new islands\n");
            exit(1);
        }
        speak();
        return 0;
    }
    bio_open(bio_path, reset || !resumed);
    for (int j = 0; j < arrivals_pending_n; ++j) {
        int r = arrivals_pending[j];
        char al[96];
        snprintf(al, sizeof al,
                 "i\t%llu\t%d\t%016llx\t%016llx\t%llu\n",
                 (unsigned long long)episode_no, r,
                 (unsigned long long)reg_digest[r],
                 (unsigned long long)reg_witness[r],
                 (unsigned long long)reg_len[r]);
        bio_append(al);
        printf("island arrival: registry %d digest %016llx witness %016llx "
               "len %llu\n",
               r, (unsigned long long)reg_digest[r],
               (unsigned long long)reg_witness[r],
               (unsigned long long)reg_len[r]);
    }
    /* this-life baselines: the price of THIS stretch of life, not the
       cumulative one -- the transfer court reads these deltas */
    double  tl_ab = atomic_bits_lived, tl_bb = bilm_bits,
            tl_tb = trilm_bits, tl_ub = unitlm_bits, tl_mb = mvlm_bits,
            tl_cb = core_bits;
    uint64_t tl_aby = atomic_bytes_lived, tl_bby = bilm_bytes,
             tl_tby = trilm_bytes, tl_uby = unitlm_bytes,
             tl_mby = mvlm_bytes, tl_cby = core_bytes;
    double tl_jbits[JURY_GENOMES];
    double tl_jrow[JURY_GENOMES][CORE_HIDDEN];
    uint64_t tl_jbytes[JURY_GENOMES], tl_jhealth[JURY_GENOMES],
             tl_jnear[JURY_GENOMES], tl_jclamps[JURY_GENOMES],
             tl_jpos[JURY_GENOMES], tl_jneg[JURY_GENOMES];
    for (int g = 0; g < JURY_GENOMES; ++g) {
        JuryCore *c = &jury_core[g];
        tl_jbits[g] = c->bits;
        tl_jbytes[g] = c->bytes;
        tl_jhealth[g] = c->health_bytes;
        tl_jnear[g] = c->near_sat;
        tl_jclamps[g] = c->clamp_hits;
        tl_jpos[g] = c->gates_pos;
        tl_jneg[g] = c->gates_neg;
        memcpy(tl_jrow[g], c->row_abs_sum, sizeof tl_jrow[g]);
    }
    printf("%s: episode %llu, %llu lived bytes, %d units\n",
           resumed ? "resumed" : "born",
           (unsigned long long)episode_no,
           (unsigned long long)counts_total, unit_count);

    double sum_bits = 0.0;
    int played_isl_id = isl_id;
    for (uint64_t e = 0; e < episodes; ++e) {
        played_isl_id = atlas_enabled ? atlas_choose(isl_id, steps) : isl_id;
        sum_bits += run_episode(played_isl_id, steps);
    }
    if (fflush(bio_file) != 0 || fclose(bio_file) != 0) {
        fprintf(stderr, "netta: biography close failed\n"); exit(1);
    }
    state_save(state_path);

    if (episodes)
        printf("bits per raw byte: %.6f\n", sum_bits / (double)episodes);
    if (units_enabled && steps_total) {
        uint64_t decisions = steps_total - macro_bytes + macro_events;
        printf("units: %d living of %d born, %llu macro events over "
               "%llu bytes, decisions per lived byte %.4f\n",
               unit_living, unit_count, (unsigned long long)macro_events,
               (unsigned long long)macro_bytes,
               (double)decisions / (double)steps_total);
        if (unit_deaths)
            printf("units: %llu deaths this run\n",
                   (unsigned long long)unit_deaths);
        if (unit_count != unit_living) {
            uint64_t ghost_pairs = 0, live_pairs = 0;
            for (int m = 0; m < ACTIONS + unit_count; ++m) {
                ghost_pairs += mv_out[m];
                live_pairs += mv_live_out[m];
            }
            printf("tombstones: %llu unigram events and %llu transition "
                   "events excluded from living probability\n",
                   (unsigned long long)(move_total - move_live_total),
                   (unsigned long long)(ghost_pairs - live_pairs));
        }
        if (births_rejected_cap)
            printf("units: %llu births rejected at cap %d\n",
                   (unsigned long long)births_rejected_cap, MAX_UNITS);
        if (unitlm_bytes && atomic_bytes_lived)
            printf("unit-LM bits per raw byte: %.6f (atomic %.6f, "
                   "lived %llu bytes)\n",
                   unitlm_bits / (double)unitlm_bytes,
                   atomic_bits_lived / (double)atomic_bytes_lived,
                   (unsigned long long)unitlm_bytes);
    }
    if (atomic_bytes_lived)
        printf("model atomic-uni bits/byte %.6f\n",
               atomic_bits_lived / (double)atomic_bytes_lived);
    if (units_enabled && unitlm_bytes)
        printf("model unit-uni bits/byte %.6f\n",
               unitlm_bits / (double)unitlm_bytes);
    if (bilm_bytes)
        printf("model byte-bi bits/byte %.6f\n",
               bilm_bits / (double)bilm_bytes);
    if (units_enabled && mvlm_bytes)
        printf("model move-bi bits/byte %.6f\n",
               mvlm_bits / (double)mvlm_bytes);
    if (units_enabled && isl_mvlm_bytes[present_reg[played_isl_id]])
        printf("island %d shadow move-bi %.6f, atomic ref %.6f over "
               "%llu bytes\n", present_reg[played_isl_id],
               isl_mvlm_bits[present_reg[played_isl_id]] /
               (double)isl_mvlm_bytes[present_reg[played_isl_id]],
               isl_mvlm_ref_bits[present_reg[played_isl_id]] /
               (double)isl_mvlm_bytes[present_reg[played_isl_id]],
               (unsigned long long)isl_mvlm_bytes[present_reg[played_isl_id]]);
    if (trilm_bytes)
        printf("model byte-tri bits/byte %.6f\n",
               trilm_bits / (double)trilm_bytes);
    if (core_enabled && core_bytes) {
        int degen = 0;
        for (int b = 0; b < ACTIONS; ++b) {
            float nrm = 0.0f;
            for (int d = 0; d < CORE_EMBED; ++d)
                nrm += core_E[b][d] * core_E[b][d];
            if (nrm < 1e-6f) degen++;
        }
        printf("model core bits/byte %.6f\n",
               core_bits / (double)core_bytes);
        printf("core health: mean |h| %.4f, %d degenerate embeddings\n",
               core_sat_n ? core_sat_sum / (double)core_sat_n : 0.0,
               degen);
        printf("core plasticity: v1 gates +%llu -%llu (%s)\n",
               (unsigned long long)core_hebb_pos,
               (unsigned long long)core_hebb_neg,
               core_hebb_enabled ? "active" : "quarantined");
    }
    if (jury_enabled)
        for (int g = 0; g < JURY_GENOMES; ++g) {
            JuryCore *c = &jury_core[g];
            double row_max = 0.0;
            for (int j = 0; j < CORE_HIDDEN; ++j) {
                double mean = c->health_bytes ?
                    c->row_abs_sum[j] / (double)c->health_bytes : 0.0;
                if (mean > row_max) row_max = mean;
            }
            double near = c->health_bytes ?
                (double)c->near_sat /
                ((double)c->health_bytes * CORE_HIDDEN) : 0.0;
            printf("jury genome %d bits/byte %.6f max-row-abs-h %.6f "
                   "near-sat %.9f clamps %llu gates +%llu -%llu\n",
                   g, c->bytes ? c->bits / (double)c->bytes : 0.0,
                   row_max, near, (unsigned long long)c->clamp_hits,
                   (unsigned long long)c->gates_pos,
                   (unsigned long long)c->gates_neg);
        }
    if (mvp_bytes)
        printf("mv played record: %.6f bits/byte over %llu bytes\n",
               mvp_bits / (double)mvp_bytes,
               (unsigned long long)mvp_bytes);
    if (mvp_ref_bytes)
        printf("mv matched byte refs: uni %.6f, bi %.6f, tri %.6f over "
               "%llu bytes\n",
               mvp_ref_bits[0] / (double)mvp_ref_bytes,
               mvp_ref_bits[1] / (double)mvp_ref_bytes,
               mvp_ref_bits[2] / (double)mvp_ref_bytes,
               (unsigned long long)mvp_ref_bytes);
    if (mvc_bytes)
        printf("mv control record: %.6f bits/byte over %llu bytes; "
               "matched refs uni %.6f, bi %.6f, tri %.6f\n",
               mvc_bits / (double)mvc_bytes,
               (unsigned long long)mvc_bytes,
               mvc_ref_bits[0] / (double)mvc_bytes,
               mvc_ref_bits[1] / (double)mvc_bytes,
               mvc_ref_bits[2] / (double)mvc_bytes);
    if (move_nav_steps)
        printf("mv navigation: %llu searched decisions, %llu unit anchors\n",
               (unsigned long long)move_nav_steps,
               (unsigned long long)move_nav_unit_anchors);
    printf("actor episodes: uni %llu, bi %llu, tri %llu, mv %llu, "
           "null %llu%s\n",
           (unsigned long long)ep_actor[0],
           (unsigned long long)ep_actor[1],
           (unsigned long long)ep_actor[2],
           (unsigned long long)ep_actor[3],
           (unsigned long long)ep_actor[4],
           actor_lock >= 0 ? " (locked)" : "");
    if (revocations)
        printf("island court: %llu local revocations this run\n",
               (unsigned long long)revocations);
    if (null_refusals)
        printf("island birth floor: %llu null refusals this run\n",
               (unsigned long long)null_refusals);
    if (atlas_decisions)
        printf("atlas: %llu autonomous choices this run\n",
               (unsigned long long)atlas_decisions);
    if (atomic_bytes_lived > tl_aby)
        printf("this-life model atomic-uni bits/byte %.6f\n",
               (atomic_bits_lived - tl_ab) /
               (double)(atomic_bytes_lived - tl_aby));
    if (bilm_bytes > tl_bby)
        printf("this-life model byte-bi bits/byte %.6f\n",
               (bilm_bits - tl_bb) / (double)(bilm_bytes - tl_bby));
    if (trilm_bytes > tl_tby)
        printf("this-life model byte-tri bits/byte %.6f\n",
               (trilm_bits - tl_tb) / (double)(trilm_bytes - tl_tby));
    if (core_enabled && core_bytes > tl_cby)
        printf("this-life model core bits/byte %.6f\n",
               (core_bits - tl_cb) / (double)(core_bytes - tl_cby));
    if (jury_enabled)
        for (int g = 0; g < JURY_GENOMES; ++g) {
            JuryCore *c = &jury_core[g];
            uint64_t lived = c->health_bytes - tl_jhealth[g];
            double row_max = 0.0;
            for (int j = 0; j < CORE_HIDDEN; ++j) {
                double mean = lived ?
                    (c->row_abs_sum[j] - tl_jrow[g][j]) /
                    (double)lived : 0.0;
                if (mean > row_max) row_max = mean;
            }
            uint64_t priced = c->bytes - tl_jbytes[g];
            double near = lived ?
                (double)(c->near_sat - tl_jnear[g]) /
                ((double)lived * CORE_HIDDEN) : 0.0;
            printf("this-life jury genome %d bits/byte %.6f "
                   "max-row-abs-h %.6f near-sat %.9f clamps %llu "
                   "gates +%llu -%llu\n",
                   g, priced ? (c->bits - tl_jbits[g]) / (double)priced : 0.0,
                   row_max, near,
                   (unsigned long long)(c->clamp_hits - tl_jclamps[g]),
                   (unsigned long long)(c->gates_pos - tl_jpos[g]),
                   (unsigned long long)(c->gates_neg - tl_jneg[g]));
        }
    if (units_enabled && unitlm_bytes > tl_uby)
        printf("this-life model unit-uni bits/byte %.6f\n",
               (unitlm_bits - tl_ub) / (double)(unitlm_bytes - tl_uby));
    if (units_enabled && mvlm_bytes > tl_mby)
        printf("this-life model move-bi bits/byte %.6f\n",
               (mvlm_bits - tl_mb) / (double)(mvlm_bytes - tl_mby));
    for (int i = 0; i < island_count; ++i) {
        int rec = 0;
        for (int u = 0; u < unit_count; ++u) {
            Island *w = &islands[i];
            uint32_t L = units[u].len;
            int found = 0;
            if (units[u].dead) continue;
            if (w->len >= L)
                for (uint64_t o = 0; o + L <= w->len; ++o)
                    if (memcmp(w->bytes + o, units[u].bytes, L) == 0) {
                        found = 1; break;
                    }
            rec += found;
        }
        printf("units recognisable on island %d: %d of %d\n",
               i, rec, unit_living);
    }
    printf("biography: %llu lines, chain %016llx\n",
           (unsigned long long)bio_lines, (unsigned long long)bio_chain);
    return 0;
}
