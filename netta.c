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
#define MAX_ISLANDS  32
#define ACTIONS      256
#define STATE_MAGIC  "NETTAZR0"
#define STATE_VER    12u

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
    uint8_t buf[8192];
    uint64_t chain = FNV_SEED, lines = 0;
    size_t n;
    int last = -1;
    while ((n = fread(buf, 1, sizeof buf, f)) != 0) {
        chain = fnv1a64(buf, (uint64_t)n, chain);
        for (size_t i = 0; i < n; ++i)
            if (buf[i] == '\n') lines++;
        last = buf[n - 1];
    }
    if (ferror(f) || fclose(f) != 0) {
        fprintf(stderr, "netta: cannot verify biography %s\n", path);
        exit(1);
    }
    if (chain != bio_chain || lines != bio_lines ||
        (bio_lines != 0 && last != '\n')) {
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

/* the seat: candidates 0 = atomic-uni (newborn), 1 = byte-bi,
   2 = byte-tri, 3 = mv (the semi-Markov move-player, body 8: emits
   whole moves -- bytes or earned units -- priced by the move bigram
   whose statistics are the nursery's own pair counts). Granted by the
   lived prequential record alone. */
static int actor_current = 0;
static int actor_lock = -1;        /* CLI: -1 earned, 0..3 locked */
static int move_nav_enabled = 1;   /* body 9: search the observable wake */
static uint64_t ep_actor[4] = {0, 0, 0, 0};
static const char *actor_name[4] = {"uni", "bi", "tri", "mv"};
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
static double   isl_bits[MAX_ISLANDS][3];
static uint64_t isl_lived[MAX_ISLANDS];
static double   isl_mvp_bits[MAX_ISLANDS];
static uint64_t isl_mvp_bytes[MAX_ISLANDS];
static double   isl_mvp_ref_bits[MAX_ISLANDS][3];
static uint64_t isl_mvp_ref_bytes[MAX_ISLANDS];
static int island_court_enabled = 1;
static uint64_t revocations = 0;

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

static int island_court(int isl) {
    /* the island's verdict on the globally elected seat, for this
       episode only; a revocation is a biography event, not a seat
       change -- the mandate travels on, the island refuses the hand */
    int seated = actor_current;
    if (!island_court_enabled || actor_lock >= 0 || seated == 0)
        return seated;
    if (isl_lived[isl] < ACTOR_MIN_BYTES) return seated;
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
        if (ch == 0 || lc < isl_bits[isl][ch] / (double)isl_lived[isl])
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

static uint32_t truth_move(Island *isl, uint64_t pos, uint64_t room) {
    uint32_t best_len = 1;
    int best_u = -1;
    for (int u = 0; u < unit_count; ++u) {
        uint32_t len = units[u].len;
        if (len <= best_len || (uint64_t)len > room || pos > isl->len - len)
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
        denom = (double)mv_out[(uint32_t)prev] + (double)alive;
        cnt = pair_get((uint32_t)prev, cur);
    } else {
        denom = (double)move_total + (double)alive;
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
static uint32_t move_route_anchor(const Island *isl, uint64_t pos, int alive) {
    Route routes[CTX + 1][UNIT_MAX_LEN + 1];
    memset(routes, 0, sizeof routes);
    uint64_t base = pos - CTX;

    for (int m = 0; m < alive; ++m) {
        uint32_t len = move_len((uint32_t)m);
        if (len <= CTX && move_matches(isl, base, CTX, (uint32_t)m))
            route_offer(routes[len], (uint32_t)m,
                        move_logp(-1, (uint32_t)m, alive));
    }
    for (uint32_t off = 1; off < CTX; ++off) {
        for (uint32_t pl = 1; pl <= UNIT_MAX_LEN; ++pl) {
            Route *prior = &routes[off][pl];
            if (!prior->live) continue;
            uint64_t room = CTX - off;
            for (int m = 0; m < alive; ++m) {
                uint32_t cur = (uint32_t)m;
                uint32_t len = move_len(cur);
                if ((uint64_t)len <= room &&
                    move_matches(isl, base + off, room, cur))
                    route_offer(routes[off + len], cur,
                                prior->score +
                                move_logp((int64_t)prior->move, cur, alive));
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

static void build_move_dist(int64_t prev, int alive, int search,
                            double move_p[ACTIONS + MAX_UNITS]) {
    /* The searched policy multiplies each current prior by the strongest
       one-move continuation available from that candidate, then renormalizes.
       This is a model-only rollout: candidate identities and learned pair
       counts are visible, future world bytes are not. The external court
       prices the resulting policy, so a confident wrong attractor remains
       expensive and cannot earn authority from its internal score. */
    double sum = 0.0;
    for (int c = 0; c < alive; ++c) {
        move_p[c] = move_prob(prev, (uint32_t)c, alive);
        if (search) {
            double best_future = 0.0;
            for (int d = 0; d < alive; ++d) {
                double p = move_prob(c, (uint32_t)d, alive);
                if (p > best_future) best_future = p;
            }
            move_p[c] *= best_future;
        }
        sum += move_p[c];
    }
    if (search) {
        for (int c = 0; c < alive; ++c) move_p[c] /= sum;
        sum = 0.0;
        for (int c = 0; c < alive; ++c) sum += move_p[c];
    }
    if (fabs(sum - 1.0) > 1e-6) {
        fprintf(stderr, "netta: move policy is not a distribution "
                        "(sum=%.9f)\n", sum);
        exit(1);
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
static int fixed_start_set = 0;
static uint64_t fixed_start = 0;

static void state_refuse(const char *path, const char *why) {
    fprintf(stderr, "netta: %s is inconsistent (%s); refusing\n", path, why);
    exit(1);
}

static void checked_add(uint64_t *sum, uint64_t value,
                        const char *path, const char *what) {
    if (UINT64_MAX - *sum < value) state_refuse(path, what);
    *sum += value;
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
        fwrite(&mvp_bits, sizeof mvp_bits, 1, f) != 1 ||
        fwrite(&mvp_bytes, sizeof mvp_bytes, 1, f) != 1 ||
        fwrite(mvp_ref_bits, sizeof mvp_ref_bits[0], 3, f) != 3 ||
        fwrite(&mvp_ref_bytes, sizeof mvp_ref_bytes, 1, f) != 1 ||
        fwrite(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fwrite(ep_actor, sizeof ep_actor[0], 4, f) != 4 ||
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
            fwrite(&islands[i].len, sizeof(uint64_t), 1, f) != 1 ||
            fwrite(&isl_lived[i], sizeof isl_lived[0], 1, f) != 1 ||
            fwrite(isl_bits[i], sizeof(double), 3, f) != 3 ||
            fwrite(&isl_mvp_bits[i], sizeof(double), 1, f) != 1 ||
            fwrite(&isl_mvp_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fwrite(isl_mvp_ref_bits[i], sizeof(double), 3, f) != 3 ||
            fwrite(&isl_mvp_ref_bytes[i], sizeof(uint64_t), 1, f) != 1) {
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
    for (int u = 0; u < unit_count; ++u) {
        if (units[u].len == 0 || units[u].len > UNIT_MAX_LEN) {
            fprintf(stderr, "netta: %s carries a malformed unit; "
                            "refusing\n", path);
            exit(1);
        }
        if (units[u].born_episode > episode_no ||
            units[u].support_at_birth < BIRTH_SUPPORT)
            state_refuse(path, "unit provenance");
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
        fread(&mvp_bits, sizeof mvp_bits, 1, f) != 1 ||
        fread(&mvp_bytes, sizeof mvp_bytes, 1, f) != 1 ||
        fread(mvp_ref_bits, sizeof mvp_ref_bits[0], 3, f) != 3 ||
        fread(&mvp_ref_bytes, sizeof mvp_ref_bytes, 1, f) != 1 ||
        fread(&actor_current, sizeof actor_current, 1, f) != 1 ||
        fread(ep_actor, sizeof ep_actor[0], 4, f) != 4 ||
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
    if (nisl != (uint32_t)island_count) {
        fprintf(stderr, "netta: state carries %u islands, world has %d; "
                        "refusing\n", nisl, island_count);
        exit(1);
    }
    for (int i = 0; i < island_count; ++i) {
        uint64_t d, l;
        if (fread(&d, sizeof d, 1, f) != 1 ||
            fread(&l, sizeof l, 1, f) != 1 ||
            fread(&isl_lived[i], sizeof isl_lived[0], 1, f) != 1 ||
            fread(isl_bits[i], sizeof(double), 3, f) != 3 ||
            fread(&isl_mvp_bits[i], sizeof(double), 1, f) != 1 ||
            fread(&isl_mvp_bytes[i], sizeof(uint64_t), 1, f) != 1 ||
            fread(isl_mvp_ref_bits[i], sizeof(double), 3, f) != 3 ||
            fread(&isl_mvp_ref_bytes[i], sizeof(uint64_t), 1, f) != 1) {
            fprintf(stderr, "netta: %s truncated; refusing\n", path);
            exit(1);
        }
        if (d != islands[i].digest || l != islands[i].len) {
            fprintf(stderr, "netta: island %d does not match the life in "
                            "%s; refusing\n", i, path);
            exit(1);
        }
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
            state_refuse(path, "dead move carries statistics");
    uint64_t actor_eps = 0;
    for (int a = 0; a < 4; ++a)
        checked_add(&actor_eps, ep_actor[a], path,
                    "actor episode overflow");
    if (actor_eps != episode_no)
        state_refuse(path, "actor episodes disagree");
    uint64_t isl_lived_sum = 0, isl_mv_sum = 0;
    for (int i = 0; i < island_count; ++i) {
        checked_add(&isl_lived_sum, isl_lived[i], path,
                    "island lived overflow");
        checked_add(&isl_mv_sum, isl_mvp_bytes[i], path,
                    "island move overflow");
        if (isl_mvp_ref_bytes[i] != isl_mvp_bytes[i] ||
            isl_mvp_bytes[i] > isl_lived[i])
            state_refuse(path, "island move evidence disagrees");
        for (int c = 0; c < 3; ++c)
            if (!isfinite(isl_bits[i][c]) || isl_bits[i][c] < 0.0 ||
                !isfinite(isl_mvp_ref_bits[i][c]) ||
                isl_mvp_ref_bits[i][c] < 0.0)
                state_refuse(path, "non-finite island record");
        if (!isfinite(isl_mvp_bits[i]) || isl_mvp_bits[i] < 0.0)
            state_refuse(path, "non-finite island record");
    }
    if (isl_lived_sum != steps_total || isl_mv_sum != mvp_bytes)
        state_refuse(path, "island records disagree with the life");
    if (!isfinite(unitlm_bits) || unitlm_bits < 0.0 ||
        !isfinite(atomic_bits_lived) || atomic_bits_lived < 0.0 ||
        !isfinite(bilm_bits) || bilm_bits < 0.0 ||
        !isfinite(mvlm_bits) || mvlm_bits < 0.0 ||
        !isfinite(mvp_bits) || mvp_bits < 0.0 ||
        !isfinite(mvp_ref_bits[0]) || mvp_ref_bits[0] < 0.0 ||
        !isfinite(mvp_ref_bits[1]) || mvp_ref_bits[1] < 0.0 ||
        !isfinite(mvp_ref_bits[2]) || mvp_ref_bits[2] < 0.0 ||
        !isfinite(trilm_bits) || trilm_bits < 0.0)
        state_refuse(path, "non-finite model record");
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
    atomic_bits_lived += -log2(p_uni[truth]);
    atomic_bytes_lived++;
    bilm_bits += -log2(p_bi[truth]);
    bilm_bytes++;
    trilm_bits += -log2(p_tri[truth]);
    trilm_bytes++;
    isl_bits[isl_id][0] += -log2(p_uni[truth]);
    isl_bits[isl_id][1] += -log2(p_bi[truth]);
    isl_bits[isl_id][2] += -log2(p_tri[truth]);
    isl_lived[isl_id]++;
    counts[truth]++;
    counts_total++;
    steps_total++;
    bi_count[pv][truth]++;
    bi_row[pv]++;
    tri_add(tctx, (uint8_t)truth);
    if (units_enabled)
        matcher_feed((uint8_t)truth, pos, isl_id);
}

static double run_episode(int isl_id, uint64_t steps) {
    Island *isl = &islands[isl_id];
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
    double bits = 0.0;
    char line[256];
    episode_no++;
    actor_elect();
    int acting = island_court(isl_id);
    int probation = 0;
    if (actor_lock < 0 && units_enabled && acting != 3 &&
        episode_no % 8 == 7 &&
        atomic_bytes_lived >= ACTOR_MIN_BYTES && mvlm_bytes &&
        atomic_bits_lived / (double)atomic_bytes_lived -
        mvlm_bits / (double)mvlm_bytes >= ACTOR_GAIN) {
        /* the shadow record opens a rare, deterministic probation
           episode; only the record played here can earn the seat.
           Probation borrows the body, never the seat: the elected
           incumbent keeps its mandate through the trial. */
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
            int alive = ACTIONS + unit_count;
            double r, acc;
            double move_p[ACTIONS + MAX_UNITS];
            uint32_t m = 0;
            int64_t policy_prev = player_prev;
            if (move_nav_enabled) {
                policy_prev = (int64_t)move_route_anchor(isl, pos, alive);
                move_nav_steps++;
                if (policy_prev >= ACTIONS) move_nav_unit_anchors++;
            }
            build_move_dist(policy_prev, alive, move_nav_enabled, move_p);
            /* one rng draw per move; walk the cumulative mass */
            r = (double)(rng_next() >> 11) *
                (1.0 / 9007199254740992.0);
            acc = 0.0;
            m = (uint32_t)(alive - 1);   /* float-tail fallback */
            for (int c = 0; c < alive; ++c) {
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
                         (unsigned long long)episode_no, isl_id,
                         (unsigned long long)pos, m, L, advance, nll, target,
                         (long long)policy_prev);
            else
                snprintf(line, sizeof line,
                         "v\t%llu\t%d\t%llu\t%u\t%u\t%u\t%.6f\t%u\n",
                         (unsigned long long)episode_no, isl_id,
                         (unsigned long long)pos, m, L, advance, nll, target);
            bio_append(line);
            bits += nll;
            if (actor_lock == 3) {
                mvc_bits += nll;
                mvc_bytes += advance;
            } else {
                mvp_bits += nll;
                mvp_bytes += advance;
                isl_mvp_bits[isl_id] += nll;
                isl_mvp_bytes[isl_id] += advance;
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
                    isl_mvp_ref_bits[isl_id][0] += -log2(p_uni[truth]);
                    isl_mvp_ref_bits[isl_id][1] += -log2(p_bi[truth]);
                    isl_mvp_ref_bits[isl_id][2] += -log2(p_tri[truth]);
                    isl_mvp_ref_bytes[isl_id]++;
                }
                absorb_truth(isl_id, isl, pos + j, p_uni, p_bi, p_tri);
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
                            : acting == 1 ? p_bi : p_uni;
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
                 actor_name[acting]);
        bio_append(line);
        absorb_truth(isl_id, isl, pos, p_uni, p_bi, p_tri);
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

int main(int argc, char **argv) {
    const char *state_path = "netta0.state";
    const char *bio_path = "netta0.bio.tsv";
    uint64_t seed = 1, episodes = 1, steps = 64;
    int isl_id = 0, reset = 0;
    int paths_n = 0;
    const char *paths[MAX_ISLANDS];

    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--seed") && i + 1 < argc)
            seed = parse_u64("--seed", argv[++i]);
        else if (!strcmp(argv[i], "--episodes") && i + 1 < argc)
            episodes = parse_u64("--episodes", argv[++i]);
        else if (!strcmp(argv[i], "--steps") && i + 1 < argc)
            steps = parse_u64("--steps", argv[++i]);
        else if (!strcmp(argv[i], "--island") && i + 1 < argc)
            isl_id = parse_int("--island", argv[++i]);
        else if (!strcmp(argv[i], "--start") && i + 1 < argc) {
            fixed_start = parse_u64("--start", argv[++i]);
            fixed_start_set = 1;
        }
        else if (!strcmp(argv[i], "--state") && i + 1 < argc)
            state_path = argv[++i];
        else if (!strcmp(argv[i], "--bio") && i + 1 < argc)
            bio_path = argv[++i];
        else if (!strcmp(argv[i], "--reset"))
            reset = 1;
        else if (!strcmp(argv[i], "--no-units"))
            units_enabled = 0;
        else if (!strcmp(argv[i], "--no-mv-nav"))
            move_nav_enabled = 0;
        else if (!strcmp(argv[i], "--no-island-court"))
            island_court_enabled = 0;
        else if (!strcmp(argv[i], "--actor-lock") && i + 1 < argc) {
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

    if (actor_lock == 3 && !units_enabled) {
        fprintf(stderr,
                "netta: --actor-lock mv requires units enabled\n");
        exit(1);
    }
    printf("NETTA ZERO\n");
    if (paths_n == 0) {
        fprintf(stderr, "usage: netta <island.bytes>... [--seed N] "
                        "[--episodes N] [--steps N] [--island N] "
                        "[--start OFFSET] [--state P] [--bio P] [--reset] "
                        "[--no-units] [--no-mv-nav] [--no-island-court] "
                        "[--actor-lock uni|bi|tri|mv]\n");
        exit(1);
    }
    pairs = (Pair *)calloc(PAIR_SLOTS, sizeof(Pair));
    if (!pairs) { fprintf(stderr, "netta: oom\n"); exit(1); }
    tri = (Pair *)calloc(TRI_SLOTS, sizeof(Pair));
    if (!tri) { fprintf(stderr, "netta: oom\n"); exit(1); }
    for (int i = 0; i < paths_n; ++i) island_load(paths[i]);
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
    if (resumed) bio_verify(bio_path);
    if (!reset && !resumed && file_exists(bio_path)) {
        fprintf(stderr,
                "netta: biography %s exists without state; use --reset "
                "to begin a new life\n", bio_path);
        exit(1);
    }
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
    printf("actor episodes: uni %llu, bi %llu, tri %llu, mv %llu%s\n",
           (unsigned long long)ep_actor[0],
           (unsigned long long)ep_actor[1],
           (unsigned long long)ep_actor[2],
           (unsigned long long)ep_actor[3],
           actor_lock >= 0 ? " (locked)" : "");
    if (revocations)
        printf("island court: %llu local revocations this run\n",
               (unsigned long long)revocations);
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
