/* ledger_check.c -- the independent reader of the mycelium ledger.
   Plain C, shares no code with mycelium.cpp: its own hash, its own
   parsing, its own refusals. It verifies what the ledger claims to be:
   an append-only chain of V/G/U records whose G events name grave blobs
   that still hold the exact bytes they were sealed with. It changes
   nothing and holds no office over the writer.

   Verdict: per-type counts and the final chain on accept (rc 0);
   the first fault by name and line on refuse (rc 1).

   Scope, printed: a chain that verifies proves internal consistency of
   the file as supplied; a truncation at a record boundary yields a
   self-consistent prefix that only an external witness can catch.

   usage: ledger_check [ledger [grave-dir]]                              */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RECORD_MAX 4096
#define LEDGER_LAW "body1-byte-v1"

static const char *ledger_path = ".mycelium.ledger";
static const char *grave_dir = ".mycelium.grave";

static void refuse(unsigned long long line, const char *what) {
    fprintf(stderr, "ledger_check: line %llu: %s\n", line, what);
    exit(1);
}

static uint64_t hash_fold(const unsigned char *p, size_t n, uint64_t h) {
    size_t i;
    for (i = 0; i < n; i++) {
        h ^= p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static int parse_hex16(const char *s, size_t n, uint64_t *out) {
    uint64_t v = 0;
    size_t i;
    if (n != 16) return 0;
    for (i = 0; i < 16; i++) {
        char c = s[i];
        int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else return 0;
        v = (v << 4) | (uint64_t)d;
    }
    *out = v;
    return 1;
}

static int parse_u64(const char *s, size_t n, uint64_t *out) {
    uint64_t v = 0;
    size_t i;
    if (n == 0 || n > 20) return 0;
    if (n > 1 && s[0] == '0') return 0;
    for (i = 0; i < n; i++) {
        char c = s[i];
        if (c < '0' || c > '9') return 0;
        if (v > (UINT64_MAX - (uint64_t)(c - '0')) / 10) return 0;
        v = v * 10 + (uint64_t)(c - '0');
    }
    *out = v;
    return 1;
}

/* split payload into at most max fields on tabs; returns field count.
   fields[i] points into payload, lens[i] is its length. */
static int fields_split(const char *payload, size_t plen,
                        const char **fields, size_t *lens, int max) {
    int nf = 0;
    size_t start = 0, i;
    for (i = 0; i <= plen; i++) {
        if (i == plen || payload[i] == '\t') {
            if (nf < max) {
                fields[nf] = payload + start;
                lens[nf] = i - start;
            }
            nf++;
            start = i + 1;
            if (i == plen) break;
        }
    }
    return nf;
}

static int label_valid(const char *s, size_t n) {
    size_t i;
    if (n == 0 || n > 32) return 0;
    for (i = 0; i < n; i++) {
        char c = s[i];
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
              c == '_' || c == '-'))
            return 0;
    }
    return 1;
}

static int field_eq(const char *s, size_t n, const char *literal) {
    return strlen(literal) == n && !memcmp(s, literal, n);
}

static int lower_hex_n(const char *s, size_t n, size_t width) {
    size_t i;
    if (n != width) return 0;
    for (i = 0; i < n; ++i)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f'))) return 0;
    return 1;
}

static int parse_s_event(const char *s, size_t n, uint64_t want_digest,
                         uint64_t want_bytes) {
    const char *f[9];
    size_t z[9];
    uint64_t episode, digest, bytes, seed, lived;
    int k = fields_split(s, n, f, z, 9);
    if (k != 9 || !field_eq(f[0], z[0], "s") ||
            !parse_u64(f[1], z[1], &episode) ||
            !parse_hex16(f[2], z[2], &digest) ||
            !parse_u64(f[3], z[3], &bytes) || bytes == 0 ||
            !parse_u64(f[4], z[4], &seed) ||
            !(field_eq(f[5], z[5], "uni") || field_eq(f[5], z[5], "bi") ||
              field_eq(f[5], z[5], "tri")) ||
            !(field_eq(f[6], z[6], "supported-backoff") ||
              field_eq(f[6], z[6], "laplace-red")) ||
            !lower_hex_n(f[7], z[7], 4) ||
            !parse_u64(f[8], z[8], &lived) || lived == 0)
        return 0;
    return digest == want_digest && bytes == want_bytes;
}

typedef struct { uint64_t prior, ord; char *sline; size_t slen; } Witness;
typedef struct { char name[33]; } Label;

static Witness *witnesses;
static size_t witness_n, witness_cap;
static Label *labels;
static size_t label_n, label_cap;

static int witness_add(uint64_t prior, uint64_t ord, const char *s, size_t n) {
    size_t i;
    for (i = 0; i < witness_n; ++i)
        if (witnesses[i].prior == prior && witnesses[i].ord == ord &&
                witnesses[i].slen == n && !memcmp(witnesses[i].sline, s, n))
            return 0;
    if (witness_n == witness_cap) {
        size_t next = witness_cap ? witness_cap * 2 : 64;
        Witness *p = (Witness *)realloc(witnesses, next * sizeof *p);
        if (!p) return -1;
        witnesses = p;
        witness_cap = next;
    }
    char *copy = (char *)malloc(n ? n : 1);
    if (!copy) return -1;
    if (n) memcpy(copy, s, n);
    witnesses[witness_n++] = (Witness){prior, ord, copy, n};
    return 1;
}

static int label_known(const char *s, size_t n) {
    size_t i;
    for (i = 0; i < label_n; ++i)
        if (strlen(labels[i].name) == n && !memcmp(labels[i].name, s, n)) return 1;
    return 0;
}

static int label_add(const char *s, size_t n) {
    if (label_known(s, n)) return 1;
    if (label_n == label_cap) {
        size_t next = label_cap ? label_cap * 2 : 16;
        Label *p = (Label *)realloc(labels, next * sizeof *p);
        if (!p) return 0;
        labels = p;
        label_cap = next;
    }
    memcpy(labels[label_n].name, s, n);
    labels[label_n].name[n] = 0;
    label_n++;
    return 1;
}

static int label_lt(const char *a, size_t an, const char *b, size_t bn) {
    size_t n = an < bn ? an : bn;
    int c = memcmp(a, b, n);
    if (c) return c < 0;
    return an < bn;
}

static int sources_valid(const char *s, size_t n, uint64_t touched) {
    size_t start = 0, count = 0, i, j;
    size_t prev_start = 0, prev_len = 0;
    int have_prev = 0;
    if (!n) return 0;
    while (start <= n) {
        size_t end = start;
        while (end < n && s[end] != ',') end++;
        if (!label_valid(s + start, end - start) ||
                !label_known(s + start, end - start)) return 0;
        if (have_prev &&
                !label_lt(s + prev_start, prev_len, s + start, end - start))
            return 0;
        prev_start = start;
        prev_len = end - start;
        have_prev = 1;
        for (i = 0, j = 0; i < start; ++i)
            if (s[i] == ',') {
                size_t prior_start = j;
                size_t prior_end = i;
                if (prior_end - prior_start == end - start &&
                        !memcmp(s + prior_start, s + start, end - start)) return 0;
                j = i + 1;
            }
        count++;
        if (end == n) break;
        start = end + 1;
    }
    return count == touched;
}

/* body 2: the proposer's receipts, judged by the second hand. Their own
   file, their own chain, their own law string; absence is not a fault. */
static const char *props_path = ".mycelium.proposals";

static void refuse_props(unsigned long long line, const char *what) {
    fprintf(stderr, "ledger_check: proposals line %llu: %s\n", line, what);
    exit(1);
}

static void check_proposals(void) {
    FILE *pf = fopen(props_path, "rb");
    if (!pf) return;
    uint64_t chain = 0xcbf29ce484222325ULL;
    uint64_t last_after = 0;
    unsigned long long lineno = 0, w_count = 0, p_count = 0;
    char *line = NULL;
    size_t cap = 0;
    for (;;) {
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(pf)) != EOF) {
            if (c == '\n') { sealed = 1; break; }
            if (len + 2 > cap) {
                cap = cap ? cap * 2 : 256;
                line = realloc(line, cap);
                if (!line) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
            }
            line[len++] = (char)c;
        }
        if (len == 0 && !sealed) break;
        lineno++;
        if (!sealed) refuse_props(lineno, "record is unsealed (no newline)");
        if (len + 1 >= RECORD_MAX) refuse_props(lineno, "record exceeds 4096-byte law");
        if (len < 18) refuse_props(lineno, "record too short for a chain field");
        if (line[len - 17] != '\t') refuse_props(lineno, "no tab before the chain field");
        uint64_t claimed;
        if (!parse_hex16(line + len - 16, 16, &claimed))
            refuse_props(lineno, "chain field is not lowercase hex16");
        size_t plen = len - 17;
        chain = hash_fold((unsigned char *)line, plen, chain);
        if (chain != claimed) refuse_props(lineno, "chain does not fold");

        const char *fl[8];
        size_t fn[8];
        int nf = fields_split(line, plen, fl, fn, 8);
        if (nf < 1 || fn[0] != 1) refuse_props(lineno, "no record type");
        if (fl[0][0] == 'W') {
            if (lineno != 1 || nf != 3 || !field_eq(fl[1], fn[1], "1") ||
                    !field_eq(fl[2], fn[2], "body2-props-v1"))
                refuse_props(lineno, "props law record");
            w_count++;
        } else if (fl[0][0] == 'P') {
            uint64_t after, d, alive, dead;
            if (!w_count) refuse_props(lineno, "P before props law record");
            if (nf != 6) refuse_props(lineno, "P arity");
            if (!parse_u64(fl[1], fn[1], &after) || after == 0)
                refuse_props(lineno, "P after-meals");
            if (!parse_hex16(fl[2], fn[2], &d)) refuse_props(lineno, "P main chain");
            if (!parse_u64(fl[3], fn[3], &alive)) refuse_props(lineno, "P alive count");
            if (!parse_u64(fl[4], fn[4], &dead)) refuse_props(lineno, "P dead count");
            if (!parse_hex16(fl[5], fn[5], &d)) refuse_props(lineno, "P snapshot digest");
            if (after < last_after) refuse_props(lineno, "P after-meals is not monotonic");
            last_after = after;
            p_count++;
        } else {
            refuse_props(lineno, "unknown proposals record type");
        }
    }
    fclose(pf);
    free(line);
    if (!w_count) refuse_props(lineno, "proposals has no law record");
    printf("proposals: %llu records (W %llu, P %llu), chain %016llx\n",
           lineno, w_count, p_count, (unsigned long long)chain);
}

int main(int argc, char **argv) {
    if (argc > 3) {
        fprintf(stderr, "usage: ledger_check [ledger [grave-dir]]\n");
        return 1;
    }
    if (argc > 1) ledger_path = argv[1];
    if (argc > 2) grave_dir = argv[2];

    FILE *lf = fopen(ledger_path, "rb");
    if (!lf) {
        fprintf(stderr, "ledger_check: cannot open ledger %s\n", ledger_path);
        return 1;
    }

    uint64_t chain = 0xcbf29ce484222325ULL;
    unsigned long long lineno = 0, v_count = 0, g_count = 0, u_count = 0;

    char *line = NULL;
    size_t cap = 0;
    for (;;) {
        /* read one line, byte by byte, so an unsealed tail is caught */
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(lf)) != EOF) {
            if (c == '\n') { sealed = 1; break; }
            if (len + 1 >= RECORD_MAX) refuse(lineno + 1, "record exceeds 4096-byte law");
            if (len + 2 > cap) {
                size_t next = cap ? cap * 2 : 256;
                char *grown = realloc(line, next);
                if (!grown) {
                    fclose(lf);
                    free(line);
                    fprintf(stderr, "ledger_check: memory\n");
                    return 1;
                }
                line = grown;
                cap = next;
            }
            line[len++] = (char)c;
        }
        if (len == 0 && !sealed) break; /* clean EOF */
        lineno++;
        if (!sealed) refuse(lineno, "record is unsealed (no newline)");
        if (len < 18) refuse(lineno, "record too short for a chain field");
        if (line[len - 17] != '\t') refuse(lineno, "no tab before the chain field");
        uint64_t claimed;
        if (!parse_hex16(line + len - 16, 16, &claimed))
            refuse(lineno, "chain field is not lowercase hex16");
        size_t plen = len - 17;
        chain = hash_fold((unsigned char *)line, plen, chain);
        if (chain != claimed) refuse(lineno, "chain does not fold");

        const char *fl[10];
        size_t fn[10];
        int nf = fields_split(line, plen, fl, fn, 10);
        if (nf < 1 || fn[0] != 1) refuse(lineno, "no record type");
        char type = fl[0][0];

        if (type == 'V') {
            if (lineno != 1 || v_count || nf != 3 ||
                    !field_eq(fl[1], fn[1], "1") ||
                    !field_eq(fl[2], fn[2], LEDGER_LAW))
                refuse(lineno, "version record");
            v_count++;
        } else if (type == 'G') {
            if (!v_count) refuse(lineno, "G before version record");
            /* G label prior ord sdig sbytes slen s-line(slen bytes, may hold tabs) */
            if (nf < 8) refuse(lineno, "G arity");
            uint64_t prior, ord, sdig, sbytes, slen;
            if (!label_valid(fl[1], fn[1])) refuse(lineno, "G label");
            if (!parse_hex16(fl[2], fn[2], &prior)) refuse(lineno, "G prior digest");
            if (!parse_u64(fl[3], fn[3], &ord) || ord == 0) refuse(lineno, "G ordinal");
            if (!parse_hex16(fl[4], fn[4], &sdig)) refuse(lineno, "G speech digest");
            if (!parse_u64(fl[5], fn[5], &sbytes) || sbytes == 0)
                refuse(lineno, "G speech bytes");
            if (!parse_u64(fl[6], fn[6], &slen) || slen == 0)
                refuse(lineno, "G s-line length");
            size_t head = (size_t)(fl[6] - line) + fn[6] + 1;
            if (head > plen || plen - head != slen)
                refuse(lineno, "s line length does not match slen");
            if (!parse_s_event(line + head, (size_t)slen, sdig, sbytes))
                refuse(lineno, "the attested s line is not canonical for its grave blob");
            int wa = witness_add(prior, ord, line + head, (size_t)slen);
            if (wa < 0) refuse(lineno, "memory");
            if (!wa) refuse(lineno, "attestation witness appears twice");
            if (!label_add(fl[1], fn[1])) refuse(lineno, "memory");
            /* the blob must still hold the sealed bytes */
            char blob[4096];
            snprintf(blob, sizeof blob, "%s/%.16s", grave_dir, fl[4]);
            FILE *bf = fopen(blob, "rb");
            if (!bf) refuse(lineno, "grave blob is missing");
            uint64_t bh = 0xcbf29ce484222325ULL, bn = 0;
            unsigned char buf[65536];
            size_t r;
            while ((r = fread(buf, 1, sizeof buf, bf)) > 0) {
                bh = hash_fold(buf, r, bh);
                bn += r;
            }
            if (ferror(bf)) { fclose(bf); refuse(lineno, "grave blob unreadable"); }
            fclose(bf);
            if (bn != sbytes) refuse(lineno, "grave blob byte count mismatch");
            if (bh != sdig) refuse(lineno, "grave blob digest mismatch");
            g_count++;
        } else if (type == 'U') {
            uint64_t d, k, touched;
            if (!v_count) refuse(lineno, "U before version record");
            if (nf != 6) refuse(lineno, "U arity");
            if (!parse_hex16(fl[1], fn[1], &d)) refuse(lineno, "U prompt digest");
            if (!parse_hex16(fl[2], fn[2], &d)) refuse(lineno, "U corpse digest");
            if (!parse_u64(fl[3], fn[3], &k) || k == 0) refuse(lineno, "U k");
            if (!parse_u64(fl[4], fn[4], &touched) || touched == 0 || touched > k)
                refuse(lineno, "U touched");
            if (!sources_valid(fl[5], fn[5], touched)) refuse(lineno, "U sources");
            u_count++;
        } else {
            refuse(lineno, "unknown record type");
        }
    }
    fclose(lf);
    if (!v_count) refuse(lineno, "ledger has no version record");

    printf("ledger_check: %llu records (V %llu, G %llu, U %llu), chain %016llx\n",
           lineno, v_count, g_count, u_count, (unsigned long long)chain);
    check_proposals();
    printf("scope: internal consistency of the supplied file; a prefix cut at "
           "a record boundary needs an external witness\n");
    size_t i;
    for (i = 0; i < witness_n; ++i) free(witnesses[i].sline);
    free(witnesses);
    free(labels);
    free(line);
    return 0;
}
