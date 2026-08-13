/*
 * NETTA ZERO
 *
 * An immutable byte tape is the world. A byte is the irreducible action.
 * The first body proves only this: it can see, act, be judged, remember,
 * and restart without changing what one unit of language means.
 *
 * Contains: length-safe island loading and hashing; immutable byte
 * positions; 256 atomic actions; a minimal externally judged next-byte
 * game; deterministic RNG; an immutable biography; restart-safe state.
 * Contains nothing else by decree (Z0-Z2 boundary).
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
#define STATE_VER    1u

/* ---------------------------------------------------------------- hash */

static uint64_t fnv1a64(const uint8_t *p, uint64_t n, uint64_t h) {
    /* h: pass 0xcbf29ce484222325 to start a fresh digest, or a previous
       digest to chain. */
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
    /* splitmix64 */
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
    /* Immutable: append-only. The chain digests every line so any later
       mutation of the file is detectable against the persisted chain. */
    size_t n = strlen(line);
    if (fwrite(line, 1, n, bio_file) != n) {
        fprintf(stderr, "netta: biography write failed\n"); exit(1);
    }
    bio_chain = fnv1a64((const uint8_t *)line, (uint64_t)n, bio_chain);
    bio_lines++;
}

/* --------------------------------------------------------------- state */

static uint64_t counts[ACTIONS];   /* lived truth-byte frequencies */
static uint64_t counts_total = 0;
static uint64_t episode_no = 0;
static uint64_t steps_total = 0;

static void state_save(const char *path) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "netta: cannot write %s\n", path); exit(1); }
    uint32_t ver = STATE_VER;
    uint32_t nisl = (uint32_t)island_count;
    if (fwrite(STATE_MAGIC, 1, 8, f) != 8 ||
        fwrite(&ver, sizeof ver, 1, f) != 1 ||
        fwrite(&rng_state, sizeof rng_state, 1, f) != 1 ||
        fwrite(&episode_no, sizeof episode_no, 1, f) != 1 ||
        fwrite(&steps_total, sizeof steps_total, 1, f) != 1 ||
        fwrite(&bio_lines, sizeof bio_lines, 1, f) != 1 ||
        fwrite(&bio_chain, sizeof bio_chain, 1, f) != 1 ||
        fwrite(&counts_total, sizeof counts_total, 1, f) != 1 ||
        fwrite(counts, sizeof counts[0], ACTIONS, f) != ACTIONS ||
        fwrite(&nisl, sizeof nisl, 1, f) != 1) {
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
    /* Returns 1 on resume, 0 if no state file exists. Any unreadable or
       foreign file refuses loudly: this body loads NETTA ZERO state only. */
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    char magic[8];
    uint32_t ver, nisl;
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
        fread(&nisl, sizeof nisl, 1, f) != 1) {
        fprintf(stderr, "netta: %s truncated; refusing\n", path);
        exit(1);
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
    /* Laplace-smoothed lived byte frequencies. A newborn has no counts:
       exactly uniform, exactly 8 bits per raw byte against any truth. */
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
    for (uint64_t s = 0; s < steps; ++s) {
        uint64_t pos = start + s;
        uint64_t ctx_digest =
            fnv1a64(isl->bytes + pos - CTX, CTX, FNV_SEED);
        uint64_t rng_before = rng_state;
        double p[ACTIONS];
        policy(p);
        int action = sample(p);
        int truth = isl->bytes[pos];
        double loss = -log2(p[truth]);
        snprintf(line, sizeof line,
                 "%llu\t%llu\t%d\t%llu\t%016llx\t%d\t%d\t%.6f\t%016llx\t"
                 "atomic\t1\n",
                 (unsigned long long)episode_no, (unsigned long long)s,
                 isl_id, (unsigned long long)pos,
                 (unsigned long long)ctx_digest, action, truth, loss,
                 (unsigned long long)rng_before);
        bio_append(line);
        /* learning strictly after the receipt: the judge saw the newborn
           distribution, not the one the truth just taught. */
        counts[truth]++;
        counts_total++;
        steps_total++;
        bits += loss;
    }
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
                        "[--state P] [--bio P] [--reset]\n");
        exit(1);
    }
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
    printf("%s: episode %llu, %llu lived bytes\n",
           resumed ? "resumed" : "born",
           (unsigned long long)episode_no,
           (unsigned long long)counts_total);

    double sum_bits = 0.0;
    for (uint64_t e = 0; e < episodes; ++e)
        sum_bits += run_episode(isl_id, steps);
    if (fflush(bio_file) != 0 || fclose(bio_file) != 0) {
        fprintf(stderr, "netta: biography close failed\n"); exit(1);
    }
    state_save(state_path);

    printf("bits per raw byte: %.6f\n", sum_bits / (double)episodes);
    printf("biography: %llu lines, chain %016llx\n",
           (unsigned long long)bio_lines, (unsigned long long)bio_chain);
    return 0;
}
