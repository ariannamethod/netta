/* Public red constructor for warrant tests. This deliberately demonstrates
   the boundary named by body 26: FNV is a reproducible witness, not a secret
   or a signature. It can hash one record, reseal one court base line under a
   named law digest, or compute a docket over newline-sealed records on stdin.
   It validates nothing and is never linked into Netta or warrant_check. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint64_t more(uint64_t h, const char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint8_t)p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static int record(char *line, size_t cap) {
    if (!fgets(line, (int)cap, stdin)) return 0;
    size_t n = strlen(line);
    if (!n || line[n - 1] != '\n') return -1;
    line[--n] = 0;
    if (n && line[n - 1] == '\r') line[--n] = 0;
    return 1;
}

int main(int argc, char **argv) {
    const uint64_t seed = 0xcbf29ce484222325ULL;
    char line[1024];
    if (argc == 2 && strcmp(argv[1], "hash") == 0) {
        if (record(line, sizeof line) != 1) return 2;
        printf("%016llx\n",
               (unsigned long long)more(seed, line, strlen(line)));
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "bytes") == 0) {
        uint64_t h = seed;
        unsigned char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof buf, stdin)) != 0)
            h = more(h, (const char *)buf, n);
        if (ferror(stdin)) return 2;
        printf("%016llx\n", (unsigned long long)h);
        return 0;
    }
    if (argc == 3 && strcmp(argv[1], "receipt") == 0) {
        if (record(line, sizeof line) != 1 || strlen(argv[2]) != 16) return 2;
        char sealed[1200];
        int n = snprintf(sealed, sizeof sealed, "%s law-digest=%s",
                         line, argv[2]);
        if (n < 0 || (size_t)n >= sizeof sealed) return 2;
        printf("%s receipt=%016llx\n", line,
               (unsigned long long)more(seed, sealed, (size_t)n));
        return 0;
    }
    if (argc == 2 && strcmp(argv[1], "docket") == 0) {
        uint64_t h = seed;
        int seen = 0, rr;
        while ((rr = record(line, sizeof line)) == 1) {
            h = more(h, line, strlen(line));
            h = more(h, "\n", 1);
            seen++;
        }
        if (rr < 0 || !seen) return 2;
        printf("%016llx\n", (unsigned long long)h);
        return 0;
    }
    return 2;
}
