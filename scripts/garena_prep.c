/*
 * garena_prep.c -- the second hand of the sealed Gutenberg arena.
 *
 * An independent C implementation of the sealed corpus preparation, so
 * the arena's normalization and shuffled twin rest on two hands in two
 * languages. The rules are the sealed ones (research/GUTENBERG_ARENA.md):
 * CRLF and lone CR map to LF; the body is the bytes strictly between the
 * single "*** START OF THE PROJECT GUTENBERG EBOOK" line and the single
 * following "*** END OF THE PROJECT GUTENBERG EBOOK" line; the shuffled
 * twin is a Fisher-Yates permutation from the last index to one on an
 * explicit SplitMix64 stream, refused unless the byte histogram and the
 * length are conserved.
 *
 *   cc -O2 -std=c11 -Wall -Wextra -Wpedantic garena_prep.c -o garena_prep
 *   garena_prep normalize RAW BODY
 *   garena_prep shuffle BODY TWIN
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define SHUFFLE_SEED 0x4e45545441475241ULL
#define START_MARK "*** START OF THE PROJECT GUTENBERG EBOOK"
#define END_MARK   "*** END OF THE PROJECT GUTENBERG EBOOK"

static uint8_t *read_all(const char *path, size_t *n) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "garena_prep: cannot open %s\n", path); exit(1); }
    if (fseek(f, 0, SEEK_END) != 0) { fprintf(stderr, "garena_prep: seek\n"); exit(1); }
    long sz = ftell(f);
    if (sz < 0) { fprintf(stderr, "garena_prep: size\n"); exit(1); }
    rewind(f);
    uint8_t *buf = malloc(sz ? (size_t)sz : 1);
    if (!buf) { fprintf(stderr, "garena_prep: oom\n"); exit(1); }
    if (sz && fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
        fprintf(stderr, "garena_prep: short read\n"); exit(1);
    }
    fclose(f);
    *n = (size_t)sz;
    return buf;
}

static void write_all(const char *path, const uint8_t *buf, size_t n) {
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "garena_prep: cannot write %s\n", path); exit(1); }
    if (n && fwrite(buf, 1, n, f) != n) {
        fprintf(stderr, "garena_prep: short write\n"); exit(1);
    }
    if (fclose(f) != 0) { fprintf(stderr, "garena_prep: close\n"); exit(1); }
}

static int do_normalize(const char *in, const char *out) {
    size_t rn;
    uint8_t *raw = read_all(in, &rn);

    /* CRLF and lone CR to LF */
    uint8_t *unix_b = malloc(rn ? rn : 1);
    if (!unix_b) { fprintf(stderr, "garena_prep: oom\n"); exit(1); }
    size_t un = 0;
    for (size_t i = 0; i < rn; ++i) {
        if (raw[i] == '\r') {
            unix_b[un++] = '\n';
            if (i + 1 < rn && raw[i + 1] == '\n') i++;
        } else {
            unix_b[un++] = raw[i];
        }
    }
    free(raw);

    /* the body lies strictly between the single START line and the
       single following END line */
    size_t start_from = 0, end_from = 0;
    int starts = 0, ends = 0;
    size_t line = 0;
    size_t slen = strlen(START_MARK), elen = strlen(END_MARK);
    while (line < un) {
        size_t eol = line;
        while (eol < un && unix_b[eol] != '\n') eol++;
        size_t next = eol < un ? eol + 1 : un;
        size_t len = next - line;
        if (len >= slen && memcmp(unix_b + line, START_MARK, slen) == 0) {
            if (!starts) start_from = next;
            starts++;
        } else if (starts && len >= elen &&
                   memcmp(unix_b + line, END_MARK, elen) == 0) {
            if (!ends) end_from = line;
            ends++;
        }
        line = next;
    }
    if (starts != 1) {
        fprintf(stderr, "garena_prep: expected exactly one START line, saw %d\n",
                starts);
        exit(1);
    }
    if (ends != 1 || end_from <= start_from) {
        fprintf(stderr, "garena_prep: expected exactly one END line after START\n");
        exit(1);
    }
    write_all(out, unix_b + start_from, end_from - start_from);
    printf("normalize %s: raw %zu -> body %zu bytes\n", in, rn,
           end_from - start_from);
    free(unix_b);
    return 0;
}

static uint64_t sm_state;
static uint64_t sm_next(void) {
    sm_state += 0x9E3779B97F4A7C15ULL;
    uint64_t z = sm_state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static int do_shuffle(const char *in, const char *out) {
    size_t n;
    uint8_t *body = read_all(in, &n);
    uint64_t hist[256] = {0}, twin_hist[256] = {0};
    for (size_t i = 0; i < n; ++i) hist[body[i]]++;

    sm_state = SHUFFLE_SEED;
    for (size_t span = n; span > 1; --span) {
        size_t i = span - 1;
        uint64_t j = sm_next() % (uint64_t)span;
        uint8_t t = body[i]; body[i] = body[j]; body[j] = t;
    }
    for (size_t i = 0; i < n; ++i) twin_hist[body[i]]++;
    for (int b = 0; b < 256; ++b)
        if (hist[b] != twin_hist[b]) {
            fprintf(stderr,
                    "garena_prep: shuffled twin does not conserve the byte "
                    "histogram\n");
            exit(1);
        }
    write_all(out, body, n);
    printf("shuffle %s: %zu bytes, histogram conserved\n", in, n);
    free(body);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 4 && strcmp(argv[1], "normalize") == 0)
        return do_normalize(argv[2], argv[3]);
    if (argc == 4 && strcmp(argv[1], "shuffle") == 0)
        return do_shuffle(argv[2], argv[3]);
    fprintf(stderr,
            "usage: garena_prep normalize RAW BODY | shuffle BODY TWIN\n");
    return 1;
}
