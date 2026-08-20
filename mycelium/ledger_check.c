/* ledger_check.c -- the independent reader of the mycelium ledger.
   Plain C, shares no code with mycelium.cpp: its own hash, its own
   parsing, its own refusals. It verifies what the ledger claims to be:
   an append-only chain of G/F/U records whose G events name grave blobs
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

int main(int argc, char **argv) {
    if (argc > 1) ledger_path = argv[1];
    if (argc > 2) grave_dir = argv[2];

    FILE *lf = fopen(ledger_path, "rb");
    if (!lf) {
        fprintf(stderr, "ledger_check: cannot open ledger %s\n", ledger_path);
        return 1;
    }

    uint64_t chain = 0xcbf29ce484222325ULL;
    unsigned long long lineno = 0, g_count = 0, f_count = 0, u_count = 0;
    int pending_f = 0;
    char pend_label[33] = "";
    char pend_sdig[17] = "";

    char *line = NULL;
    size_t cap = 0;
    for (;;) {
        /* read one line, byte by byte, so an unsealed tail is caught */
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(lf)) != EOF) {
            if (c == '\n') { sealed = 1; break; }
            if (len + 2 > cap) {
                cap = cap ? cap * 2 : 256;
                line = realloc(line, cap);
                if (!line) { fprintf(stderr, "ledger_check: memory\n"); return 1; }
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

        const char *fl[8];
        size_t fn[8];
        int nf = fields_split(line, plen, fl, fn, 8);
        if (nf < 1 || fn[0] != 1) refuse(lineno, "no record type");
        char type = fl[0][0];

        if (type == 'G') {
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
            if (memcmp(fl[7], "s\t", 2) != 0)
                refuse(lineno, "the attested line is not an s event");
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
            if (pending_f) refuse(lineno, "G arrives while a meal is unsealed");
            pending_f = 1;
            snprintf(pend_label, sizeof pend_label, "%.*s", (int)fn[1], fl[1]);
            snprintf(pend_sdig, sizeof pend_sdig, "%.16s", fl[4]);
            g_count++;
        } else if (type == 'F') {
            if (nf != 5) refuse(lineno, "F arity");
            uint64_t sdig, frags, toks;
            if (!label_valid(fl[1], fn[1])) refuse(lineno, "F label");
            if (!parse_hex16(fl[2], fn[2], &sdig)) refuse(lineno, "F speech digest");
            if (!parse_u64(fl[3], fn[3], &frags)) refuse(lineno, "F fragment count");
            if (!parse_u64(fl[4], fn[4], &toks)) refuse(lineno, "F token count");
            if (!pending_f) refuse(lineno, "F without a G before it");
            if (strlen(pend_label) != fn[1] ||
                memcmp(pend_label, fl[1], fn[1]) != 0)
                refuse(lineno, "F label does not match its G");
            if (memcmp(pend_sdig, fl[2], 16) != 0)
                refuse(lineno, "F speech digest does not match its G");
            pending_f = 0;
            f_count++;
        } else if (type == 'U') {
            uint64_t d, k, touched;
            if (nf != 6) refuse(lineno, "U arity");
            if (!parse_hex16(fl[1], fn[1], &d)) refuse(lineno, "U prompt digest");
            if (!parse_hex16(fl[2], fn[2], &d)) refuse(lineno, "U corpse digest");
            if (!parse_u64(fl[3], fn[3], &k) || k == 0) refuse(lineno, "U k");
            if (!parse_u64(fl[4], fn[4], &touched) || touched == 0)
                refuse(lineno, "U touched");
            u_count++;
        } else {
            refuse(lineno, "unknown record type");
        }
    }
    fclose(lf);
    free(line);
    if (pending_f) refuse(lineno, "ledger ends between G and F");

    printf("ledger_check: %llu records (G %llu, F %llu, U %llu), chain %016llx\n",
           lineno, g_count, f_count, u_count, (unsigned long long)chain);
    printf("scope: internal consistency of the supplied file; a prefix cut at "
           "a record boundary needs an external witness\n");
    return 0;
}
