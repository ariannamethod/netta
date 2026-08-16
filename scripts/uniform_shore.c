/*
 * uniform_shore.c -- a reproducible structureless byte shore.
 *
 * This is a control generator, not a Netta organ.  It writes the low byte
 * of each SplitMix64 draw so a uniform-shore experiment has a portable seed,
 * length, and exact byte recipe instead of depending on /dev/urandom.
 *
 *   cc -O2 -std=c11 -Wall -Wextra -Wpedantic uniform_shore.c -o uniform_shore
 *   uniform_shore LENGTH SEED > shore.bytes
 */

#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static uint64_t next(uint64_t *state) {
    uint64_t z = (*state += 0x9e3779b97f4a7c15ULL);
    z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
    z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
    return z ^ (z >> 31);
}

static uint64_t parse(const char *name, const char *s) {
    char *end = NULL;
    errno = 0;
    unsigned long long value = strtoull(s, &end, 0);
    if (errno || !end || *end) {
        fprintf(stderr, "uniform_shore: invalid %s\n", name);
        exit(1);
    }
    return (uint64_t)value;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: uniform_shore LENGTH SEED\n");
        return 1;
    }
    uint64_t remaining = parse("length", argv[1]);
    uint64_t state = parse("seed", argv[2]);
    uint8_t buf[4096];
    while (remaining) {
        size_t n = remaining < sizeof buf ? (size_t)remaining : sizeof buf;
        for (size_t i = 0; i < n; ++i) buf[i] = (uint8_t)next(&state);
        if (fwrite(buf, 1, n, stdout) != n) {
            fprintf(stderr, "uniform_shore: write failed\n");
            return 1;
        }
        remaining -= n;
    }
    if (fflush(stdout) != 0) {
        fprintf(stderr, "uniform_shore: flush failed\n");
        return 1;
    }
    return 0;
}
