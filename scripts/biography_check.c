/* Body 31: the recognition. An independent, read-only reader of the biography
   language. It shares no organism code, takes no state, changes nothing, and
   influences no resume or behaviour: it reads one biography file and reports.

   It refuses a biography whose records exceed 1024 bytes, are not
   newline-sealed, carry a NUL or CR, open with no known type, contain too
   many fields, or -- for the s (signature), w (citation) and i (arrival)
   records it parses -- do not match their canonical grammar, tab for tab.
   The ten other record types are admitted by type only, as the organism
   admits them today; that partial closure is named, not hidden.
   The organism does not yet parse w rows: on those this reader is stricter,
   which is a difference to be closed by a later turn, not papered over.

   Recognition: a w row (a citation of a court's word about a candidate
   stream) is recognised when a PRIOR s row in the supplied file carries the
   same candidate digest and the same byte count. The report gives every
   recognised w with its multiplicity -- how many prior signatures match --
   and every unrecognised w as an external fact. Order is causal inside the
   file: a signature after the citation does not recognise it. With no state
   witness, one supplied file is only treated as one life; its identity is
   explicitly reported as unverified.

   rc 0: the language is accepted and the report is printed; rc 1: a record
   is refused by name and line; rc 2: invocation or I/O failure. */
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define BIO_RECORD_MAX 1024

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
        size_t next_cap = sig_cap ? sig_cap * 2 : 64;
        if (next_cap < sig_cap || next_cap > SIZE_MAX / sizeof *sigs) {
            fprintf(stderr, "biography reader: oom\n");
            exit(2);
        }
        Sig *next = (Sig *)realloc(sigs, next_cap * sizeof *sigs);
        if (!next) { fprintf(stderr, "biography reader: oom\n"); exit(2); }
        sigs = next;
        sig_cap = next_cap;
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
           canon_u64(f[2], &id) && id <= 1023 &&
           lower_hex(f[3], 16) && lower_hex(f[4], 16) &&
           canon_u64(f[5], &len);
}

/* the remaining ten grammars of BIOGRAPHY.md, implemented on this side of
   the language with this reader's own hands */

static int u64_shape(const char *s) {
    uint64_t v;
    return canon_u64(s, &v);
}

static int u64_at_most(const char *s, uint64_t max) {
    uint64_t v;
    return canon_u64(s, &v) && v <= max;
}

static int fix6_shape(const char *s) {
    const char *dot = strchr(s, '.');
    if (!dot || dot == s || dot - s > 20 || strlen(dot + 1) != 6) return 0;
    if (s[0] == '0' && dot - s != 1) return 0;
    for (const char *p = s; *p; ++p)
        if (p != dot && (*p < '0' || *p > '9')) return 0;
    static const char max[] = "18446744073709551615";
    size_t whole_n = (size_t)(dot - s);
    if (whole_n == sizeof max - 1 && memcmp(s, max, whole_n) > 0) return 0;
    return 1;
}

static int fix6_at_most_eight(const char *s) {
    return fix6_shape(s) && s[1] == '.' &&
           (s[0] < '8' || (s[0] == '8' && !strcmp(s + 1, ".000000")));
}

static int actor_word(const char *s) {
    return !strcmp(s, "uni") || !strcmp(s, "bi") || !strcmp(s, "tri") ||
           !strcmp(s, "mv") || !strcmp(s, "null");
}

static int byte_actor_word(const char *s) {
    return !strcmp(s, "uni") || !strcmp(s, "bi") || !strcmp(s, "tri");
}

static int seated_word(const char *s) {
    return byte_actor_word(s) || !strcmp(s, "mv");
}

static int challenger_word(const char *s) {
    return byte_actor_word(s) || !strcmp(s, "null");
}

static int parse_step(char **f, int k) {
    return k == 12 && u64_shape(f[0]) && u64_shape(f[1]) &&
           u64_at_most(f[2], 1023) && u64_shape(f[3]) &&
           lower_hex(f[4], 16) &&
           u64_at_most(f[5], 255) && u64_at_most(f[6], 255) &&
           fix6_shape(f[7]) && lower_hex(f[8], 16) &&
           !strcmp(f[9], "atomic") && !strcmp(f[10], "1") &&
           actor_word(f[11]);
}

static int parse_rest(char **f, int k) {
    uint64_t len, support, idle, move, advance, target, policy, lived;
    switch (f[0][0]) {
    case 'a':
        return k == 3 && u64_shape(f[1]) &&
               (actor_word(f[2]) || !strcmp(f[2], "mvp"));
    case 'b':
        return k == 6 && u64_shape(f[1]) && u64_at_most(f[2], 4095) &&
               canon_u64(f[3], &len) && len >= 2 && len <= 16 &&
               lower_hex(f[4], (size_t)(2 * len)) &&
               canon_u64(f[5], &support) && support >= 64 &&
               support % 64 == 0;
    case 'u':
        return k == 4 && u64_shape(f[1]) && u64_at_most(f[2], 4095) &&
               canon_u64(f[3], &support) && support >= 64 &&
               support % 64 == 0;
    case 'd':
        return k == 5 && u64_shape(f[1]) && u64_at_most(f[2], 4095) &&
               u64_shape(f[3]) && canon_u64(f[4], &idle) && idle >= 16384;
    case 'm':
        return k == 7 && u64_shape(f[1]) && u64_at_most(f[2], 31) &&
               u64_shape(f[3]) && u64_at_most(f[4], 4095) &&
               canon_u64(f[5], &len) && len >= 2 && len <= 16 &&
               fix6_shape(f[6]);
    case 'v':
        if (k != 9 && k != 10) return 0;
        if (!u64_shape(f[1]) || !u64_at_most(f[2], 1023) ||
                !u64_shape(f[3]) || !canon_u64(f[4], &move) || move > 4351 ||
                !canon_u64(f[5], &len) || len < 1 || len > 16 ||
                (move < 256 ? len != 1 : len < 2) ||
                !canon_u64(f[6], &advance) || advance < 1 || advance > len ||
                !fix6_shape(f[7]) || !canon_u64(f[8], &target) ||
                target > 4351)
            return 0;
        return k == 9 || (canon_u64(f[9], &policy) && policy <= 4351);
    case 't':
        if (k < 3 || !u64_shape(f[1]) || !u64_at_most(f[2], 1023)) return 0;
        if (k == 4) return !strcmp(f[3], "eligible");
        if (k == 6) return !strcmp(f[3], "chart") &&
                           canon_u64(f[4], &lived) && lived < 1000 &&
                           u64_shape(f[5]);
        if (k == 7) return !strcmp(f[3], "earned") &&
                           fix6_at_most_eight(f[4]) &&
                           fix6_at_most_eight(f[5]) &&
                           canon_u64(f[6], &lived) && lived >= 1000;
        return 0;
    case 'r':
        if (k != 7 && k != 8) return 0;
        if (k == 8 && strcmp(f[7], "8.000000") != 0) return 0;
        return u64_shape(f[1]) && u64_at_most(f[2], 1023) &&
               strcmp(f[3], "uni") && seated_word(f[3]) &&
               (k == 7 ? byte_actor_word(f[4]) : challenger_word(f[4])) &&
               strcmp(f[3], f[4]) && fix6_shape(f[5]) &&
               fix6_shape(f[6]);
    case 'q':
        return k == 6 && u64_shape(f[1]) && u64_at_most(f[2], 1023) &&
               seated_word(f[3]) && challenger_word(f[4]) &&
               strcmp(f[3], f[4]) && !strcmp(f[5], "0");
    default:
        return 0;
    }
}

static int refuse(uint64_t line, const char *why) {
    if (printf("biography refused: line %llu %s\n",
               (unsigned long long)line, why) < 0 || fflush(stdout) != 0) {
        fprintf(stderr, "biography reader: cannot write report\n");
        return 2;
    }
    return 1;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: biography_check BIOGRAPHY\n");
        return 2;
    }
    int fd = open(argv[1], O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        fprintf(stderr, "biography reader: cannot open %s\n", argv[1]);
        return 2;
    }
    struct stat st;
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        close(fd);
        fprintf(stderr, "biography reader: %s is not a regular file\n",
                argv[1]);
        return 2;
    }
    FILE *f = fdopen(fd, "rb");
    if (!f) {
        close(fd);
        fprintf(stderr, "biography reader: cannot open %s\n", argv[1]);
        return 2;
    }
    char line[BIO_RECORD_MAX + 1];
    size_t n = 0;
    int has_nul = 0, has_cr = 0;
    uint64_t lineno = 0, chain = 0xcbf29ce484222325ULL;
    uint64_t n_s = 0, n_w = 0, recognised = 0, external = 0;
    if (printf("scope: one supplied file is treated as one life; "
               "state identity unverified\n") < 0) {
        fprintf(stderr, "biography reader: cannot write report\n");
        return 2;
    }
    for (;;) {
        int c = fgetc(f);
        if (c == EOF) {
            if (ferror(f)) {
                fclose(f);
                fprintf(stderr, "biography reader: cannot read %s\n",
                        argv[1]);
                return 2;
            }
            if (n) return refuse(lineno + 1, "is not newline-sealed");
            break;
        }
        if (n == BIO_RECORD_MAX)
            return refuse(lineno + 1, "exceeds 1024 bytes");
        line[n++] = (char)c;
        if (c == 0) has_nul = 1;
        if (c == '\r') has_cr = 1;
        if (c != '\n') continue;
        lineno++;
        chain = fnv_more(chain, line, n);
        if (has_nul)
            return refuse(lineno, "carries a NUL byte");
        if (has_cr)
            return refuse(lineno, "carries a carriage return");
        line[n - 1] = 0;
        if (!record_type_ok(line))
            return refuse(lineno, "opens with no known record type");
        char *fields[16];
        int k = split_tabs(line, fields, 16);
        if (k < 0) return refuse(lineno, "has too many fields");
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
        } else if (line[0] >= '0' && line[0] <= '9') {
            if (!parse_step(fields, k))
                return refuse(lineno, "step record is not canonical");
        } else {
            if (!parse_rest(fields, k))
                return refuse(lineno, "record is not canonical for its type");
        }
        n = 0;
        has_nul = has_cr = 0;
    }
    if (fclose(f) != 0) {
        fprintf(stderr, "biography reader: cannot read %s\n", argv[1]);
        return 2;
    }
    printf("recognition: s=%llu w=%llu recognised=%llu external=%llu\n",
           (unsigned long long)n_s, (unsigned long long)n_w,
           (unsigned long long)recognised, (unsigned long long)external);
    printf("biography: %llu records, chain %016llx\n",
           (unsigned long long)lineno, (unsigned long long)chain);
    if (fflush(stdout) != 0 || ferror(stdout)) {
        fprintf(stderr, "biography reader: cannot write report\n");
        free(sigs);
        return 2;
    }
    free(sigs);
    return 0;
}
