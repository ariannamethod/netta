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

#define CTX          16
#define MAX_ISLANDS  32
#define ACTIONS      256
#define STATE_MAGIC  "NETTAZR0"
#define STATE_VER    6u

#define MAX_UNITS      4096
#define UNIT_MAX_LEN   16
#define BIRTH_SUPPORT  64
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
    char     name[256];
} Island;

static Island islands[MAX_ISLANDS];
static int island_count = 0;

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
    rewind(f);
    Island *isl = &islands[island_count];
    isl->len = (uint64_t)sz;
    isl->bytes = (uint8_t *)malloc(isl->len ? isl->len : 1);
    if (!isl->bytes) { fprintf(stderr, "netta: oom\n"); exit(1); }
    if (isl->len && fread(isl->bytes, 1, isl->len, f) != isl->len) {
        fprintf(stderr, "netta: short read on %s\n", path); exit(1);
    }
    fclose(f);
    isl->digest = fnv1a64(isl->bytes, isl->len, FNV_SEED);
    snprintf(isl->name, sizeof isl->name, "%s", path);
    island_count++;
}

/* ----------------------------------------------------------- biography */

static FILE   *bio_file;
static uint64_t bio_chain = FNV_SEED;
static uint64_t bio_lines = 0;

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
} Unit;

static Unit units[MAX_UNITS];
static int  unit_count = 0;
static uint64_t births_rejected_cap = 0;

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
    for (int u = 0; u < unit_count; ++u)
        if (units[u].len == len && memcmp(units[u].bytes, b, len) == 0)
            return u;
    return -1;
}

static int unit_prefix_alive(const uint8_t *b, uint32_t len) {
    /* is b[0..len) a strict or full prefix of any living unit? */
    for (int u = 0; u < unit_count; ++u)
        if (units[u].len >= len && memcmp(units[u].bytes, b, len) == 0)
            return 1;
    return 0;
}

static uint64_t episode_no;   /* forward: defined with state below */

static void unit_birth(uint32_t a, uint32_t b, uint64_t support) {
    uint8_t buf[2 * UNIT_MAX_LEN]; uint8_t t1[1], t2[1];
    uint32_t la = move_len(a), lb = move_len(b);
    if (la + lb > UNIT_MAX_LEN) return;
    memcpy(buf, move_bytes(a, t1), la);
    memcpy(buf + la, move_bytes(b, t2), lb);
    uint32_t L = la + lb;
    for (uint32_t i = 0; i < L; ++i)
        if (is_ws(buf[i])) return;            /* v1 boundary discipline */
    if (unit_find(buf, L) >= 0) return;       /* same bytes, same unit  */
    if (unit_count >= MAX_UNITS) { births_rejected_cap++; return; }
    Unit *u = &units[unit_count];
    memcpy(u->bytes, buf, L);
    u->len = L;
    u->born_episode = episode_no;
    u->support_at_birth = support;
    u->uses = 0;
    char line[128], hex[2 * UNIT_MAX_LEN + 1];
    for (uint32_t i = 0; i < L; ++i)
        snprintf(hex + 2 * i, 3, "%02x", buf[i]);
    snprintf(line, sizeof line, "b\t%llu\t%d\t%u\t%s\t%llu\n",
             (unsigned long long)episode_no, unit_count, L, hex,
             (unsigned long long)support);
    bio_append(line);
    unit_count++;
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
            if (p->cnt == BIRTH_SUPPORT) unit_birth(prev, cur, p->cnt);
            return;
        }
    }
    fprintf(stderr, "netta: pair table full\n"); exit(1);
}

/* matcher: segments the lived truth tape into moves (greedy longest
   unit match), feeds adjacent-move pairs, emits macro receipts. */
static uint8_t  mbuf[UNIT_MAX_LEN];
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

/* the seat: candidates 0 = atomic-uni (newborn), 1 = byte-bi,
   2 = byte-tri. Granted by the lived prequential record alone. */
static int actor_current = 0;
static int actor_lock = -1;        /* CLI: -1 earned, 0/1/2 locked */
static uint64_t ep_actor[3] = {0, 0, 0};
static const char *actor_name[3] = {"uni", "bi", "tri"};

#define ACTOR_MIN_BYTES 1000
#define ACTOR_GAIN      0.1
#define ACTOR_KEEP      0.05

static void actor_elect(void) {
    if (actor_lock >= 0) { actor_current = actor_lock; return; }
    if (atomic_bytes_lived < ACTOR_MIN_BYTES) { actor_current = 0; return; }
    double lv[3];
    lv[0] = atomic_bits_lived / (double)atomic_bytes_lived;
    lv[1] = bilm_bytes ? bilm_bits / (double)bilm_bytes : 1e9;
    lv[2] = trilm_bytes ? trilm_bits / (double)trilm_bytes : 1e9;
    /* mandate: a sitting non-newborn actor must keep a KEEP lead over
       the newborn or vacate the seat */
    if (actor_current != 0 && lv[0] - lv[actor_current] < ACTOR_KEEP)
        actor_current = 0;
    /* strongest eligible challenger (GAIN lead over the newborn) */
    int ch = -1;
    for (int c = 1; c <= 2; ++c) {
        if (c == actor_current) continue;
        if (lv[0] - lv[c] < ACTOR_GAIN) continue;
        if (ch < 0 || lv[c] < lv[ch]) ch = c;
    }
    if (ch >= 0) {
        if (actor_current == 0) actor_current = ch;
        else if (lv[actor_current] - lv[ch] >= ACTOR_KEEP)
            actor_current = ch;
    }
}

static void emit_move(uint32_t m, uint64_t pos, int isl) {
    /* prequential shadow price, strictly before the count update */
    double denom = (double)move_total + (double)(ACTIONS + unit_count);
    double pm = ((double)move_count[m] + 1.0) / denom;
    double nll = -log2(pm);
    unitlm_bits += nll;
    unitlm_bytes += move_len(m);
    move_count[m]++;
    move_total++;
    if (m >= ACTIONS) {
        Unit *u = &units[m - ACTIONS];
        u->uses++;
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
        double d2 = (double)mv_out[pv] +
                    (double)(ACTIONS + unit_count);
        double p2 = ((double)pair_get(pv, m) + 1.0) / d2;
        mvlm_bits += -log2(p2);
        mvlm_bytes += move_len(m);
        mv_out[pv]++;
        pair_feed(pv, m);
    }
    prev_move = (int64_t)m;
}

static void matcher_flush_front(void) {
    /* longest living unit equal to a prefix of mbuf; else one atomic */
    uint32_t best = 1; int best_u = -1;
    for (uint32_t k = mlen; k >= 2; --k) {
        int u = unit_find(mbuf, k);
        if (u >= 0) { best = k; best_u = u; break; }
    }
    if (best_u >= 0) emit_move((uint32_t)(ACTIONS + best_u),
                               mstart_pos, mstart_isl);
    else emit_move((uint32_t)mbuf[0], mstart_pos, mstart_isl);
    memmove(mbuf, mbuf + best, mlen - best);
    mlen -= best;
    mstart_pos += best;
}

static void matcher_feed(uint8_t truth, uint64_t pos, int isl) {
    if (mlen == 0) { mstart_pos = pos; mstart_isl = isl; }
    mbuf[mlen++] = truth;
    while (mlen > 0 &&
           (mlen == UNIT_MAX_LEN || !unit_prefix_alive(mbuf, mlen))) {
        if (mlen < UNIT_MAX_LEN && unit_prefix_alive(mbuf, mlen)) break;
        if (mlen == UNIT_MAX_LEN && unit_find(mbuf, mlen) >= 0) {
            emit_move((uint32_t)(ACTIONS + unit_find(mbuf, mlen)),
                      mstart_pos, mstart_isl);
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

static void state_save(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "netta: cannot write %s\n", path); exit(1); }
    uint32_t ver = STATE_VER;
    uint32_t nisl = (uint32_t)island_count;
    uint32_t nunits = (uint32_t)unit_count;
    if (fwrite(STATE_MAGIC, 1, 8, f) != 8 ||
        fwrite(&ver, sizeof ver, 1, f) != 1 ||
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
        fwrite(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fwrite(ep_actor, sizeof ep_actor[0], 3, f) != 3 ||
        fwrite(tri_row, sizeof tri_row[0], 65536, f) != 65536 ||
        fwrite(&trilm_bits, sizeof trilm_bits, 1, f) != 1 ||
        fwrite(&trilm_bytes, sizeof trilm_bytes, 1, f) != 1 ||
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
    for (int i = 0; i < island_count; ++i) {
        if (fwrite(&islands[i].digest, sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&islands[i].len, sizeof(uint64_t), 1, f) != 1) {
            fprintf(stderr, "netta: state write failed\n"); exit(1);
        }
    }
    if (fclose(f) != 0) {
        fprintf(stderr, "netta: state close failed\n"); exit(1);
    }
}

static int state_load(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char magic[8];
    uint32_t ver, nisl, nunits;
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
    for (int u = 0; u < unit_count; ++u)
        if (units[u].len == 0 || units[u].len > UNIT_MAX_LEN) {
            fprintf(stderr, "netta: %s carries a malformed unit; "
                            "refusing\n", path);
            exit(1);
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
        uint64_t h = fnv1a64((const uint8_t *)&e.key, sizeof e.key,
                             FNV_SEED);
        for (uint64_t probe = 0; probe < PAIR_SLOTS; ++probe) {
            Pair *p = &pairs[(h + probe) & (PAIR_SLOTS - 1)];
            if (p->cnt == 0) { *p = e; pair_used++; break; }
        }
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
        fread(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fread(ep_actor, sizeof ep_actor[0], 3, f) != 3 ||
        fread(tri_row, sizeof tri_row[0], 65536, f) != 65536 ||
        fread(&trilm_bits, sizeof trilm_bits, 1, f) != 1 ||
        fread(&trilm_bytes, sizeof trilm_bytes, 1, f) != 1 ||
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
        uint64_t h = fnv1a64((const uint8_t *)&e.key, sizeof e.key,
                             FNV_SEED);
        for (uint64_t probe = 0; probe < TRI_SLOTS; ++probe) {
            Pair *p = &tri[(h + probe) & (TRI_SLOTS - 1)];
            if (p->cnt == 0) { *p = e; tri_used++; break; }
        }
    }
    if (nisl != (uint32_t)island_count) {
        fprintf(stderr, "netta: state carries %u islands, world has %d; "
                        "refusing\n", nisl, island_count);
        exit(1);
    }
    for (int i = 0; i < island_count; ++i) {
        uint64_t d, l;
        if (fread(&d, sizeof d, 1, f) != 1 ||
            fread(&l, sizeof l, 1, f) != 1) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
        if (d != islands[i].digest || l != islands[i].len) {
            fprintf(stderr, "netta: island %d does not match the life in "
                            "%s; refusing\n", i, path);
            exit(1);
        }
    }
    fclose(f);
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

static double run_episode(int isl_id, uint64_t steps) {
    Island *isl = &islands[isl_id];
    if (isl->len < CTX + steps + 1) {
        fprintf(stderr, "netta: island %d too small for %llu steps\n",
                isl_id, (unsigned long long)steps);
        exit(1);
    }
    uint64_t span = isl->len - CTX - steps;
    uint64_t start = CTX + (rng_next() % (span ? span : 1));
    double bits = 0.0;
    char line[256];
    episode_no++;
    actor_elect();
    ep_actor[actor_current]++;
    snprintf(line, sizeof line, "a\t%llu\t%s\n",
             (unsigned long long)episode_no, actor_name[actor_current]);
    bio_append(line);
    for (uint64_t s = 0; s < steps; ++s) {
        uint64_t pos = start + s;
        uint64_t ctx_digest =
            fnv1a64(isl->bytes + pos - CTX, CTX, FNV_SEED);
        uint64_t rng_before = rng_state;
        uint8_t pv = isl->bytes[pos - 1];
        uint32_t tctx = ((uint32_t)isl->bytes[pos - 2] << 8) | pv;
        double p_uni[ACTIONS], p_bi[ACTIONS], p_tri[ACTIONS];
        policy(p_uni);
        {   /* byte-bigram row, before any update of this step */
            double d2 = (double)bi_row[pv] + (double)ACTIONS;
            double sum = 0.0;
            for (int b = 0; b < ACTIONS; ++b) {
                p_bi[b] = ((double)bi_count[pv][b] + 1.0) / d2;
                sum += p_bi[b];
            }
            if (fabs(sum - 1.0) > 1e-6) {
                fprintf(stderr, "netta: bigram row is not a "
                                "distribution (sum=%.9f)\n", sum);
                exit(1);
            }
        }
        {   /* byte-trigram row, before any update of this step */
            double d3 = (double)tri_row[tctx] + (double)ACTIONS;
            double sum = 0.0;
            for (int b = 0; b < ACTIONS; ++b) {
                p_tri[b] = ((double)tri_get(tctx, (uint8_t)b) + 1.0)
                           / d3;
                sum += p_tri[b];
            }
            if (fabs(sum - 1.0) > 1e-6) {
                fprintf(stderr, "netta: trigram row is not a "
                                "distribution (sum=%.9f)\n", sum);
                exit(1);
            }
        }
        const double *p_act = actor_current == 2 ? p_tri
                            : actor_current == 1 ? p_bi : p_uni;
        int action = sample(p_act);
        int truth = isl->bytes[pos];
        double loss = -log2(p_act[truth]);
        snprintf(line, sizeof line,
                 "%llu\t%llu\t%d\t%llu\t%016llx\t%d\t%d\t%.6f\t%016llx\t"
                 "atomic\t1\t%s\n",
                 (unsigned long long)episode_no, (unsigned long long)s,
                 isl_id, (unsigned long long)pos,
                 (unsigned long long)ctx_digest, action, truth, loss,
                 (unsigned long long)rng_before,
                 actor_name[actor_current]);
        bio_append(line);
        /* model prices stay prequential and actor-independent */
        atomic_bits_lived += -log2(p_uni[truth]);
        atomic_bytes_lived++;
        bilm_bits += -log2(p_bi[truth]);
        bilm_bytes++;
        trilm_bits += -log2(p_tri[truth]);
        trilm_bytes++;
        counts[truth]++;
        counts_total++;
        steps_total++;
        bits += loss;
        bi_count[pv][truth]++;
        bi_row[pv]++;
        tri_add(tctx, (uint8_t)truth);
        if (units_enabled)
            matcher_feed((uint8_t)truth, pos, isl_id);
    }
    if (units_enabled) matcher_end_episode();
    return bits / (double)steps;
}

/* ---------------------------------------------------------------- main */

int main(int argc, char **argv) {
    const char *state_path = "netta0.state";
    const char *bio_path = "netta0.bio.tsv";
    uint64_t seed = 1, episodes = 1, steps = 64;
    int isl_id = 0, reset = 0;
    int paths_n = 0;
    const char *paths[MAX_ISLANDS];

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc)
            seed = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--episodes") && i + 1 < argc)
            episodes = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--steps") && i + 1 < argc)
            steps = strtoull(argv[++i], NULL, 10);
        else if (!strcmp(argv[i], "--island") && i + 1 < argc)
            isl_id = atoi(argv[++i]);
        else if (!strcmp(argv[i], "--state") && i + 1 < argc)
            state_path = argv[++i];
        else if (!strcmp(argv[i], "--bio") && i + 1 < argc)
            bio_path = argv[++i];
        else if (!strcmp(argv[i], "--reset"))
            reset = 1;
        else if (!strcmp(argv[i], "--no-units"))
            units_enabled = 0;
        else if (!strcmp(argv[i], "--actor-lock") && i + 1 < argc) {
            ++i;
            if (!strcmp(argv[i], "uni")) actor_lock = 0;
            else if (!strcmp(argv[i], "bi")) actor_lock = 1;
            else if (!strcmp(argv[i], "tri")) actor_lock = 2;
            else {
                fprintf(stderr,
                        "netta: --actor-lock takes uni|bi|tri\n");
                exit(1);
            }
        }
        else if (argv[i][0] == '-') {
            fprintf(stderr, "netta: unknown flag %s\n", argv[i]);
            exit(1);
        } else if (paths_n < MAX_ISLANDS)
            paths[paths_n++] = argv[i];
    }

    printf("NETTA ZERO\n");
    if (paths_n == 0) {
        fprintf(stderr, "usage: netta <island.bytes>... [--seed N] "
                        "[--episodes N] [--steps N] [--island N] "
                        "[--state P] [--bio P] [--reset] [--no-units]\n");
        exit(1);
    }
    pairs = (Pair *)calloc(PAIR_SLOTS, sizeof(Pair));
    if (!pairs) { fprintf(stderr, "netta: oom\n"); exit(1); }
    tri = (Pair *)calloc(TRI_SLOTS, sizeof(Pair));
    if (!tri) { fprintf(stderr, "netta: oom\n"); exit(1); }
    for (int i = 0; i < paths_n; ++i) island_load(paths[i]);
    for (int i = 0; i < island_count; ++i)
        printf("island %d: %s len=%llu digest=%016llx\n", i,
               islands[i].name, (unsigned long long)islands[i].len,
               (unsigned long long)islands[i].digest);
    if (isl_id < 0 || isl_id >= island_count) {
        fprintf(stderr, "netta: no island %d\n", isl_id);
        exit(1);
    }

    rng_state = seed;
    int resumed = 0;
    if (!reset) resumed = state_load(state_path);
    bio_open(bio_path, reset || !resumed);
    /* this-life baselines: the price of THIS stretch of life, not the
       cumulative one -- the transfer court reads these deltas */
    double  tl_ab = atomic_bits_lived, tl_bb = bilm_bits,
            tl_tb = trilm_bits, tl_ub = unitlm_bits, tl_mb = mvlm_bits;
    uint64_t tl_aby = atomic_bytes_lived, tl_bby = bilm_bytes,
             tl_tby = trilm_bytes, tl_uby = unitlm_bytes,
             tl_mby = mvlm_bytes;
    printf("%s: episode %llu, %llu lived bytes, %d units\n",
           resumed ? "resumed" : "born",
           (unsigned long long)episode_no,
           (unsigned long long)counts_total, unit_count);

    double sum_bits = 0.0;
    for (uint64_t e = 0; e < episodes; ++e)
        sum_bits += run_episode(isl_id, steps);
    if (fflush(bio_file) != 0 || fclose(bio_file) != 0) {
        fprintf(stderr, "netta: biography close failed\n"); exit(1);
    }
    state_save(state_path);

    if (episodes)
        printf("bits per raw byte: %.6f\n", sum_bits / (double)episodes);
    if (units_enabled && steps_total) {
        uint64_t decisions = steps_total - macro_bytes + macro_events;
        printf("units: %d living, %llu macro events over %llu bytes, "
               "decisions per lived byte %.4f\n",
               unit_count, (unsigned long long)macro_events,
               (unsigned long long)macro_bytes,
               (double)decisions / (double)steps_total);
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
    if (trilm_bytes)
        printf("model byte-tri bits/byte %.6f\n",
               trilm_bits / (double)trilm_bytes);
    printf("actor episodes: uni %llu, bi %llu, tri %llu%s\n",
           (unsigned long long)ep_actor[0],
           (unsigned long long)ep_actor[1],
           (unsigned long long)ep_actor[2],
           actor_lock >= 0 ? " (locked)" : "");
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
            if (w->len >= L)
                for (uint64_t o = 0; o + L <= w->len; ++o)
                    if (memcmp(w->bytes + o, units[u].bytes, L) == 0) {
                        found = 1; break;
                    }
            rec += found;
        }
        printf("units recognisable on island %d: %d of %d\n",
               i, rec, unit_count);
    }
    printf("biography: %llu lines, chain %016llx\n",
           (unsigned long long)bio_lines, (unsigned long long)bio_chain);
    return 0;
}
