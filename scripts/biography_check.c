/* Body 31: the recognition. An independent, read-only reader of the biography
   language. It shares no organism code, takes no state, changes nothing, and
   influences no resume or behaviour: it reads one biography file and reports.

   It refuses a biography whose records are not newline-sealed, carry a NUL,
   open with no known type, or -- for the s (signature), w (citation) and i
   (arrival) records it parses -- do not match their canonical grammar, tab
   for tab. The eleven other record types are admitted by type only, as the
   organism admits them today; that partial closure is named, not hidden.
   The organism does not yet parse w rows: on those this reader is stricter,
   which is a difference to be closed by a later turn, not papered over.

   Recognition: a w row (a citation of a court's word about a candidate
   stream) is recognised when a PRIOR s row (this life's own signature of a
   speech) carries the same candidate digest and the same byte count. The
   report gives every recognised w with its multiplicity -- how many prior
   signatures match -- and every unrecognised w as an external fact. Order is
   causal: a signature after the citation does not recognise it.

   rc 0: the language is accepted and the report is printed; rc 1: a record
   is refused by name and line; rc 2: invocation or I/O failure. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static uint64_t fnv_more(uint64_t h, const char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint8_t)p[i];
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

/* canonical unsigned decimal: no sign, no leading zero, within 64 bits */
static int canon_u64(const char *s, uint64_t *out) {
    if (!*s || (s[0] == '0' && s[1]) || strlen(s) > 20) return 0;
    for (const char *p = s; *p; ++p)
        if (*p < '0' || *p > '9') return 0;
    errno = 0;
    char *end;
    unsigned long long v = strtoull(s, &end, 10);
    if (errno == ERANGE || *end) return 0;
    *out = (uint64_t)v;
    return 1;
}

/* split on exact tab bytes; the newline is already stripped */
static int split_tabs(char *line, char **f, int max) {
    int k = 0;
    f[k++] = line;
    for (char *p = line; *p; ++p)
        if (*p == '\t') {
            *p = 0;
            if (k == max) return -1;
            f[k++] = p + 1;
        }
    return k;
}

static int record_type_ok(const char *line) {
    const char *p = line;
    if (*p >= '0' && *p <= '9') {
        while (*p >= '0' && *p <= '9') p++;
        if (line[0] == '0' && p != line + 1) return 0;
    } else if (*p && strchr("abdimqrstuvw", *p)) {
        p++;
    } else {
        return 0;
    }
    return *p == '\t';
}

typedef struct { uint64_t line, episode, bytes; char digest[17]; } Sig;

static Sig *sigs;
static size_t sig_n, sig_cap;

static void sig_push(uint64_t line, uint64_t episode, const char *digest,
                     uint64_t bytes) {
    if (sig_n == sig_cap) {
        sig_cap = sig_cap ? sig_cap * 2 : 64;
        sigs = (Sig *)realloc(sigs, sig_cap * sizeof *sigs);
        if (!sigs) { fprintf(stderr, "biography reader: oom\n"); exit(2); }
    }
    Sig *s = &sigs[sig_n++];
    s->line = line; s->episode = episode; s->bytes = bytes;
    memcpy(s->digest, digest, 17);
}

static int parse_s(char **f, int k, uint64_t *ep, char digest[17],
                   uint64_t *bytes) {
    uint64_t seed, lived;
    if (k != 9 || strcmp(f[0], "s") || !canon_u64(f[1], ep) ||
            !lower_hex(f[2], 16) || !canon_u64(f[3], bytes) || !*bytes ||
            !canon_u64(f[4], &seed) ||
            (strcmp(f[5], "uni") && strcmp(f[5], "bi") &&
             strcmp(f[5], "tri")) ||
            (strcmp(f[6], "supported-backoff") &&
             strcmp(f[6], "laplace-red")) ||
            !lower_hex(f[7], 4) || !canon_u64(f[8], &lived) || !lived)
        return 0;
    memcpy(digest, f[2], 17);
    return 1;
}

static int parse_w(char **f, int k, uint64_t *ep, char digest[17],
                   uint64_t *bytes) {
    uint64_t shores, verdicts;
    if (k != 9 || strcmp(f[0], "w") || !canon_u64(f[1], ep) ||
            !lower_hex(f[2], 16) || !canon_u64(f[3], bytes) ||
            *bytes < 2 || *bytes > 16384 ||
            (strcmp(f[4], "cold") && !lower_hex(f[4], 4)) ||
            !canon_u64(f[5], &shores) || shores < 1 || shores > 32 ||
            !lower_hex(f[6], 16) || !lower_hex(f[7], 16) ||
            !canon_u64(f[8], &verdicts) || verdicts != shores)
        return 0;
    memcpy(digest, f[2], 17);
    return 1;
}

static int parse_i(char **f, int k) {
    uint64_t ep, id, len;
    return k == 6 && !strcmp(f[0], "i") && canon_u64(f[1], &ep) &&
           canon_u64(f[2], &id) && id <= 2147483647ULL &&
           lower_hex(f[3], 16) && lower_hex(f[4], 16) &&
           canon_u64(f[5], &len);
}

static int refuse(uint64_t line, const char *why) {
    printf("biography refused: line %llu %s\n", (unsigned long long)line,
           why);
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: biography_check BIOGRAPHY\n");
        return 2;
    }
    FILE *f = fopen(argv[1], "rb");
    if (!f) {
        fprintf(stderr, "biography reader: cannot open %s\n", argv[1]);
        return 2;
    }
    struct stat st;
    if (fstat(fileno(f), &st) != 0 || !S_ISREG(st.st_mode)) {
        fprintf(stderr, "biography reader: %s is not a regular file\n",
                argv[1]);
        return 2;
    }
    char *line = NULL;
    size_t cap = 0;
    ssize_t n;
    uint64_t lineno = 0, chain = 0xcbf29ce484222325ULL;
    uint64_t n_s = 0, n_w = 0, recognised = 0, external = 0;
    while ((n = getline(&line, &cap, f)) >= 0) {
        lineno++;
        chain = fnv_more(chain, line, (size_t)n);
        if (n == 0 || line[n - 1] != '\n')
            return refuse(lineno, "is not newline-sealed");
        if (strlen(line) != (size_t)n)
            return refuse(lineno, "carries a NUL byte");
        line[n - 1] = 0;
        if (!record_type_ok(line))
            return refuse(lineno, "opens with no known record type");
        char *fields[16];
        int k = split_tabs(line, fields, 16);
        uint64_t ep, bytes;
        char digest[17];
        if (line[0] == 's') {
            if (!parse_s(fields, k, &ep, digest, &bytes))
                return refuse(lineno, "s record is not canonical");
            sig_push(lineno, ep, digest, bytes);
            n_s++;
        } else if (line[0] == 'w') {
            if (!parse_w(fields, k, &ep, digest, &bytes))
                return refuse(lineno, "w record is not canonical");
            n_w++;
            uint64_t m = 0;
            for (size_t i = 0; i < sig_n; ++i)
                if (sigs[i].bytes == bytes &&
                        strcmp(sigs[i].digest, digest) == 0) m++;
            if (m) {
                recognised++;
                printf("recognised: line %llu episode %llu candidate %s "
                       "bytes %llu multiplicity %llu s-lines",
                       (unsigned long long)lineno, (unsigned long long)ep,
                       digest, (unsigned long long)bytes,
                       (unsigned long long)m);
                for (size_t i = 0; i < sig_n; ++i)
                    if (sigs[i].bytes == bytes &&
                            strcmp(sigs[i].digest, digest) == 0)
                        printf(" %llu", (unsigned long long)sigs[i].line);
                printf("\n");
            } else {
                external++;
                printf("external: line %llu episode %llu candidate %s "
                       "bytes %llu\n",
                       (unsigned long long)lineno, (unsigned long long)ep,
                       digest, (unsigned long long)bytes);
            }
        } else if (line[0] == 'i') {
            if (!parse_i(fields, k))
                return refuse(lineno, "i record is not canonical");
        }
    }
    free(line);
    if (ferror(f) || fclose(f) != 0) {
        fprintf(stderr, "biography reader: cannot read %s\n", argv[1]);
        return 2;
    }
    printf("recognition: s=%llu w=%llu recognised=%llu external=%llu\n",
           (unsigned long long)n_s, (unsigned long long)n_w,
           (unsigned long long)recognised, (unsigned long long)external);
    printf("biography: %llu records, chain %016llx\n",
           (unsigned long long)lineno, (unsigned long long)chain);
    free(sigs);
    return 0;
}
