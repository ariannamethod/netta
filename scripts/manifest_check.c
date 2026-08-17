/* Independent reader for the body-29 speech manifest. It accepts exactly one
   canonical, newline-sealed manifest on stdin and recomputes the byte count
   and candidate digest from SPEECH_FILE. It shares no organism code.

   The stream proves its own count and digest. Seed, hand, law, opening,
   lived-bytes, and episode remain speaker statements until a later body gives
   an independent reader enough state to rederive them. rc 0 accepts, rc 1
   refuses, and rc 2 is an invocation or I/O failure. */
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_CAP 512

struct manifest {
    char digest[17];
    uint64_t bytes;
    uint64_t seed;
    char hand[4];
    char law[18];
    char opening[5];
    uint64_t lived_bytes;
    uint64_t episode;
};

static uint64_t fnv_more(uint64_t h, const unsigned char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static int lower_hex(const char *s, size_t n) {
    if (strlen(s) != n) return 0;
    for (size_t i = 0; i < n; ++i)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f'))) return 0;
    return 1;
}

static int token_u64(const char *s, uint64_t *out) {
    if (!*s || *s == '-' || *s == '+') return 0;
    errno = 0;
    char *end;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno == ERANGE || end == s || *end) return 0;
    *out = (uint64_t)v;
    return 1;
}

static int read_record(char line[LINE_CAP]) {
    if (!fgets(line, LINE_CAP, stdin)) return ferror(stdin) ? -1 : 0;
    size_t n = strlen(line);
    if (!n || line[n - 1] != '\n') return -2;
    line[--n] = 0;
    if (n && line[n - 1] == '\r') line[--n] = 0;
    return 1;
}

static int parse_manifest(const char *line, struct manifest *m) {
    char bytes[32] = {0}, seed[32] = {0};
    char lived[32] = {0}, episode[32] = {0};
    int used = 0;
    int got = sscanf(line,
        "spoke: candidate-digest=%16[0-9a-f] bytes=%31s seed=%31s "
        "hand=%3[a-z] law=%17[a-z-] opening=%4[0-9a-f] "
        "lived-bytes=%31s episode=%31s%n",
        m->digest, bytes, seed, m->hand, m->law, m->opening,
        lived, episode, &used);
    if (got != 8 || line[used] != 0 || !lower_hex(m->digest, 16) ||
            !token_u64(bytes, &m->bytes) || !token_u64(seed, &m->seed) ||
            (strcmp(m->hand, "uni") && strcmp(m->hand, "bi") &&
             strcmp(m->hand, "tri")) ||
            (strcmp(m->law, "supported-backoff") &&
             strcmp(m->law, "laplace-red")) ||
            !lower_hex(m->opening, 4) ||
            !token_u64(lived, &m->lived_bytes) ||
            !token_u64(episode, &m->episode))
        return 0;

    char canonical[LINE_CAP];
    int n = snprintf(canonical, sizeof canonical,
        "spoke: candidate-digest=%s bytes=%llu seed=%llu hand=%s law=%s "
        "opening=%s lived-bytes=%llu episode=%llu",
        m->digest, (unsigned long long)m->bytes,
        (unsigned long long)m->seed, m->hand, m->law, m->opening,
        (unsigned long long)m->lived_bytes,
        (unsigned long long)m->episode);
    return n >= 0 && (size_t)n < sizeof canonical && !strcmp(line, canonical);
}

static int read_speech(const char *path, uint64_t *bytes, uint64_t *digest) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    unsigned char buf[4096];
    uint64_t total = 0, h = 0xcbf29ce484222325ULL;
    size_t n;
    while ((n = fread(buf, 1, sizeof buf, f)) != 0) {
        if (UINT64_MAX - total < (uint64_t)n) {
            fclose(f);
            return 0;
        }
        total += (uint64_t)n;
        h = fnv_more(h, buf, n);
    }
    int ok = !ferror(f);
    if (fclose(f) != 0) ok = 0;
    if (!ok) return 0;
    *bytes = total;
    *digest = h;
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: manifest_check SPEECH_FILE\n");
        return 2;
    }

    char line[LINE_CAP];
    struct manifest m = {{0}, 0, 0, {0}, {0}, {0}, 0, 0};
    int rr = read_record(line);
    if (rr != 1) {
        fprintf(stderr, "manifest refused: canonical record required\n");
        return rr < 0 && ferror(stdin) ? 2 : 1;
    }
    int extra = fgetc(stdin);
    if (extra != EOF || ferror(stdin) || !parse_manifest(line, &m)) {
        fprintf(stderr, "manifest refused: canonical record required\n");
        return ferror(stdin) ? 2 : 1;
    }

    uint64_t bytes, digest;
    if (!read_speech(argv[1], &bytes, &digest)) {
        fprintf(stderr, "manifest reader: cannot read speech\n");
        return 2;
    }
    char digest_hex[17];
    snprintf(digest_hex, sizeof digest_hex, "%016llx",
             (unsigned long long)digest);
    if (bytes != m.bytes || strcmp(digest_hex, m.digest)) {
        fprintf(stderr, "manifest refused: stream witness differs\n");
        return 1;
    }

    printf("manifest accepted: %llu bytes digest %s\n",
           (unsigned long long)bytes, digest_hex);
    return 0;
}
