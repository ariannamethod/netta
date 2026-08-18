/* Public red constructor for biography grammar tests. It recomputes only
   the state's line-count and FNV chain fields from a supplied biography.
   This proves those witnesses are integrity receipts, not secret signatures,
   and lets bio_verify meet malformed but correctly re-sealed event lines. */
#define _POSIX_C_SOURCE 200809L

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FNV_SEED 0xcbf29ce484222325ULL
#define BIO_LINES_OFFSET 40
#define BIO_CHAIN_OFFSET 48

static uint64_t fnv_more(uint64_t h, const unsigned char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    FILE *bio = fopen(argv[2], "rb");
    if (!bio) return 2;
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    uint64_t lines = 0, chain = FNV_SEED;
    while ((n = getline(&line, &cap, bio)) >= 0) {
        chain = fnv_more(chain, (const unsigned char *)line, (size_t)n);
        lines++;
    }
    free(line);
    if (ferror(bio) || fclose(bio) != 0) return 2;

    FILE *state = fopen(argv[1], "r+b");
    if (!state) return 2;
    char magic[8];
    if (fread(magic, 1, sizeof magic, state) != sizeof magic ||
            memcmp(magic, "NETTAZR0", sizeof magic) != 0 ||
            fseek(state, BIO_LINES_OFFSET, SEEK_SET) != 0 ||
            fwrite(&lines, sizeof lines, 1, state) != 1 ||
            fseek(state, BIO_CHAIN_OFFSET, SEEK_SET) != 0 ||
            fwrite(&chain, sizeof chain, 1, state) != 1 ||
            fflush(state) != 0 || fclose(state) != 0)
        return 2;
    return 0;
}
