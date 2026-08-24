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

static int school_byte_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
           c == '\f' || (c >= 0x1c && c <= 0x1f);
}

/* School shapes are tokens joined by exactly one ASCII space. Tokens may
   contain any other byte, including NUL; tabs are delimiters and therefore
   cannot be shape bytes. */
static int school_shape_canonical(const char *s, size_t n, uint64_t arity) {
    uint64_t tokens = 1;
    size_t i;
    if (n == 0 || s[0] == ' ' || s[n - 1] == ' ') return 0;
    for (i = 0; i < n; ++i) {
        unsigned char c = (unsigned char)s[i];
        if (c == ' ') {
            if (i == 0 || s[i - 1] == ' ') return 0;
            tokens++;
        } else if (school_byte_space(c)) {
            return 0;
        }
    }
    return tokens == arity;
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

typedef struct { uint64_t meals, chain; } MainPrefix;
static MainPrefix *prefixes;
static size_t prefix_n, prefix_cap;

static int prefix_add(uint64_t meals, uint64_t chain) {
    if (prefix_n == prefix_cap) {
        size_t next = prefix_cap ? prefix_cap * 2 : 32;
        MainPrefix *grown = realloc(prefixes, next * sizeof(*grown));
        if (!grown) return 0;
        prefixes = grown;
        prefix_cap = next;
    }
    prefixes[prefix_n++] = (MainPrefix){meals, chain};
    return 1;
}

static int prefix_has(uint64_t meals, uint64_t chain) {
    size_t i;
    for (i = 0; i < prefix_n; ++i)
        if (prefixes[i].meals == meals && prefixes[i].chain == chain) return 1;
    return 0;
}

static int pprefix_add(uint64_t records, uint64_t chain); /* parliament pin */

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
        if (!pprefix_add(lineno, chain)) refuse_props(lineno, "memory");

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
            uint64_t after, main_chain, d, alive, dead;
            if (!w_count) refuse_props(lineno, "P before props law record");
            if (nf != 6) refuse_props(lineno, "P arity");
            if (!parse_u64(fl[1], fn[1], &after) || after == 0)
                refuse_props(lineno, "P after-meals");
            if (!parse_hex16(fl[2], fn[2], &main_chain))
                refuse_props(lineno, "P main chain");
            if (!parse_u64(fl[3], fn[3], &alive)) refuse_props(lineno, "P alive count");
            if (!parse_u64(fl[4], fn[4], &dead)) refuse_props(lineno, "P dead count");
            if (!parse_hex16(fl[5], fn[5], &d)) refuse_props(lineno, "P snapshot digest");
            if (!prefix_has(after, main_chain)) refuse_props(lineno, "P main prefix");
            if (after < last_after) refuse_props(lineno, "P after-meals is not monotonic");
            last_after = after;
            p_count++;
        } else {
            refuse_props(lineno, "unknown proposals record type");
        }
    }
    if (ferror(pf)) refuse_props(lineno + 1, "cannot read proposals");
    fclose(pf);
    free(line);
    if (!w_count) refuse_props(lineno, "proposals has no law record");
    printf("proposals: %llu records (W %llu, P %llu), chain %016llx\n",
           lineno, w_count, p_count, (unsigned long long)chain);
}

/* body 3: the school chain, judged by the second hand. Grammar, chain,
   order and arithmetic only; re-deriving bits from the field is the
   examiner's work. Absence of the file is not a fault. */
static const char *school_path = ".mycelium.school";

_Noreturn static void refuse_school(unsigned long long line, const char *what) {
    fprintf(stderr, "ledger_check: school line %llu: %s\n", line, what);
    exit(1);
}

typedef struct {
    uint64_t after, last_meal, base_total, cand_total;
    int verdicted, passed;
    char *shape;
    size_t slen;
} SchoolHyp;

static const char *school_verdict(uint64_t base_total, uint64_t cand_total) {
    if (base_total >= cand_total && base_total - cand_total >= 8000000ULL)
        return "pass";
    if (base_total > cand_total) return "weak";
    return "fail";
}

static void check_school(uint64_t main_meals) {
    FILE *sf = fopen(school_path, "rb");
    if (!sf) return;
    uint64_t chain = 0xcbf29ce484222325ULL;
    unsigned long long lineno = 0, h_count = 0, r_count = 0, o_count = 0,
                       v_count = 0, l_count = 0;
    int law_seen = 0;
    uint64_t pending_glyph = 0;
    SchoolHyp *hyps = NULL;
    size_t hyp_cap = 0;
    char *line = NULL;
    size_t cap = 0;
    for (;;) {
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(sf)) != EOF) {
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
        if (!sealed) refuse_school(lineno, "record is unsealed (no newline)");
        if (len + 1 >= RECORD_MAX) refuse_school(lineno, "record exceeds 4096-byte law");
        if (len < 18) refuse_school(lineno, "record too short for a chain field");
        if (line[len - 17] != '\t') refuse_school(lineno, "no tab before the chain field");
        uint64_t claimed;
        if (!parse_hex16(line + len - 16, 16, &claimed))
            refuse_school(lineno, "chain field is not lowercase hex16");
        size_t plen = len - 17;
        chain = hash_fold((unsigned char *)line, plen, chain);
        if (chain != claimed) refuse_school(lineno, "chain does not fold");

        const char *fl[10];
        size_t fn[10];
        int nf = fields_split(line, plen, fl, fn, 10);
        if (nf < 1 || fn[0] != 1) refuse_school(lineno, "no record type");
        char type = fl[0][0];
        if (type == 'S') {
            if (lineno != 1 || nf != 3 || !field_eq(fl[1], fn[1], "1") ||
                    !field_eq(fl[2], fn[2], "body3-school-v2"))
                refuse_school(lineno, "school law record");
            law_seen = 1;
            continue;
        }
        if (!law_seen) refuse_school(lineno, "record before school law");
        if (pending_glyph && type != 'L')
            refuse_school(lineno, "passed hypothesis must be legalised immediately");
        if (type == 'H') {
            uint64_t id, arity, slen, after, mc;
            if (nf != 9) refuse_school(lineno, "H arity");
            if (!parse_u64(fl[1], fn[1], &id) || id != h_count + 1)
                refuse_school(lineno, "H id is not sequential");
            if (!parse_u64(fl[2], fn[2], &arity) || (arity != 2 && arity != 3))
                refuse_school(lineno, "H shape arity");
            if (!parse_u64(fl[3], fn[3], &slen) || fn[4] != slen || slen == 0)
                refuse_school(lineno, "H shape length");
            if (!school_shape_canonical(fl[4], fn[4], arity))
                refuse_school(lineno, "H shape is not canonical for its arity");
            if (!parse_u64(fl[5], fn[5], &after)) refuse_school(lineno, "H after-meals");
            if (!parse_hex16(fl[6], fn[6], &mc)) refuse_school(lineno, "H main chain");
            if (!prefix_has(after, mc)) refuse_school(lineno, "H main prefix");
            if (after > UINT64_MAX - 8) refuse_school(lineno, "H window overflows");
            if (!field_eq(fl[7], fn[7], "8") ||
                    !field_eq(fl[8], fn[8], "laplace-unigram-lmf-u6-libm-v1"))
                refuse_school(lineno, "H window or baseline law");
            {
                size_t i;
                for (i = 0; i < h_count; ++i) {
                    int same = hyps[i].slen == fn[4] &&
                               !memcmp(hyps[i].shape, fl[4], fn[4]);
                    if (same && !hyps[i].verdicted)
                        refuse_school(lineno, "H shape already open");
                    if (same && hyps[i].passed == 2)
                        refuse_school(lineno, "H shape already legalised");
                }
            }
            if (h_count == hyp_cap) {
                size_t next = hyp_cap ? hyp_cap * 2 : 16;
                SchoolHyp *p = realloc(hyps, next * sizeof *p);
                if (!p) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
                hyps = p;
                hyp_cap = next;
            }
            hyps[h_count] = (SchoolHyp){0};
            hyps[h_count].after = after;
            hyps[h_count].shape = malloc(fn[4]);
            if (!hyps[h_count].shape) {
                fprintf(stderr, "ledger_check: memory\n");
                exit(1);
            }
            memcpy(hyps[h_count].shape, fl[4], fn[4]);
            hyps[h_count].slen = fn[4];
            h_count++;
        } else if (type == 'R') {
            uint64_t slen, after, mc;
            int open = 0, legal = 0;
            size_t i;
            if (nf != 6) refuse_school(lineno, "R arity");
            if (!field_eq(fl[1], fn[1], "not-proposed") &&
                    !field_eq(fl[1], fn[1], "already-enrolled") &&
                    !field_eq(fl[1], fn[1], "already-legalised"))
                refuse_school(lineno, "R reason");
            if (!parse_u64(fl[2], fn[2], &slen) || fn[3] != slen || slen == 0)
                refuse_school(lineno, "R shape length");
            if (!school_shape_canonical(fl[3], fn[3], 2) &&
                    !school_shape_canonical(fl[3], fn[3], 3))
                refuse_school(lineno, "R shape is not canonical");
            if (!parse_u64(fl[4], fn[4], &after) ||
                    !parse_hex16(fl[5], fn[5], &mc))
                refuse_school(lineno, "R state grammar");
            if (!prefix_has(after, mc)) refuse_school(lineno, "R main prefix");
            for (i = 0; i < h_count; ++i) {
                int same = hyps[i].slen == fn[3] &&
                           !memcmp(hyps[i].shape, fl[3], fn[3]);
                if (same && !hyps[i].verdicted) open = 1;
                if (same && hyps[i].passed == 2) legal = 1;
            }
            if ((field_eq(fl[1], fn[1], "already-legalised") && !legal) ||
                    (field_eq(fl[1], fn[1], "already-enrolled") &&
                     (legal || !open)) ||
                    (field_eq(fl[1], fn[1], "not-proposed") && (legal || open)))
                refuse_school(lineno, "R reason does not match school state");
            r_count++;
        } else if (type == 'O') {
            uint64_t id, meal, frags, base_mb, cand_mb, expect;
            if (nf != 6) refuse_school(lineno, "O arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > h_count)
                refuse_school(lineno, "O names no hypothesis");
            if (!parse_u64(fl[2], fn[2], &meal) ||
                    !parse_u64(fl[3], fn[3], &frags) ||
                    !parse_u64(fl[4], fn[4], &base_mb) ||
                    !parse_u64(fl[5], fn[5], &cand_mb))
                refuse_school(lineno, "O field grammar");
            expect = hyps[id - 1].last_meal ? hyps[id - 1].last_meal + 1
                                            : hyps[id - 1].after + 1;
            if (hyps[id - 1].verdicted || meal != expect ||
                    meal > hyps[id - 1].after + 8)
                refuse_school(lineno, "O outside the window's order");
            if (meal > main_meals) refuse_school(lineno, "O prices an unarrived meal");
            if (UINT64_MAX - hyps[id - 1].base_total < base_mb ||
                    UINT64_MAX - hyps[id - 1].cand_total < cand_mb)
                refuse_school(lineno, "O totals overflow");
            hyps[id - 1].last_meal = meal;
            hyps[id - 1].base_total += base_mb;
            hyps[id - 1].cand_total += cand_mb;
            o_count++;
        } else if (type == 'V') {
            uint64_t id, bt, ct;
            if (nf != 5) refuse_school(lineno, "V arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > h_count)
                refuse_school(lineno, "V names no hypothesis");
            if (!parse_u64(fl[3], fn[3], &bt) || !parse_u64(fl[4], fn[4], &ct))
                refuse_school(lineno, "V field grammar");
            if (hyps[id - 1].verdicted ||
                    hyps[id - 1].last_meal != hyps[id - 1].after + 8)
                refuse_school(lineno, "V before the window closed");
            if (bt != hyps[id - 1].base_total || ct != hyps[id - 1].cand_total)
                refuse_school(lineno, "V totals do not match the observations");
            if (!field_eq(fl[2], fn[2], school_verdict(bt, ct)))
                refuse_school(lineno, "V verdict class");
            hyps[id - 1].verdicted = 1;
            hyps[id - 1].passed = field_eq(fl[2], fn[2], "pass");
            if (hyps[id - 1].passed) pending_glyph = id;
            v_count++;
        } else if (type == 'L') {
            uint64_t id, glyph;
            if (nf != 3) refuse_school(lineno, "L arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > h_count)
                refuse_school(lineno, "L names no hypothesis");
            if (!parse_u64(fl[2], fn[2], &glyph) || glyph != l_count + 1)
                refuse_school(lineno, "L glyph is not sequential");
            if (!hyps[id - 1].verdicted || hyps[id - 1].passed != 1 ||
                    pending_glyph != id)
                refuse_school(lineno, "L without a pass");
            hyps[id - 1].passed = 2; /* legalised; a second L must refuse */
            pending_glyph = 0;
            l_count++;
        } else {
            refuse_school(lineno, "unknown school record type");
        }
    }
    if (ferror(sf)) refuse_school(lineno + 1, "cannot read school");
    fclose(sf);
    free(line);
    if (!law_seen) refuse_school(lineno, "school has no law record");
    printf("school: %llu records (S 1, H %llu, R %llu, O %llu, V %llu, L %llu), "
           "chain %016llx\n",
           lineno, h_count, r_count, o_count, v_count, l_count,
           (unsigned long long)chain);
    {
        size_t i;
        for (i = 0; i < h_count; ++i) free(hyps[i].shape);
    }
    free(hyps);
}

/* body 4: the parliament, judged by the second hand. The reader
   reproduces every known ballot from the pinned grave, ledger,
   proposals and school with its own text pipeline -- no shared
   implementation with the writer. Only the deliberately opaque DARK
   lane is semantic input it cannot recompute, and its powerlessness
   is fully checked. */
static const char *parl_path = ".mycelium.parliament";

_Noreturn static void refuse_parl(unsigned long long line, const char *what) {
    fprintf(stderr, "ledger_check: parliament line %llu: %s\n", line, what);
    exit(1);
}

/* ---- the reader's own byte-law text pipeline, for the rent juror ---- */

static int r_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' ||
           c == '\f' || (c >= 0x1c && c <= 0x1f);
}

static int r_stop(const char *w, size_t n) {
    static const char *stops[] = {
        "the","a","an","and","or","but","of","to","in","on","at","is","are",
        "was","were","be","been","it","its","this","that","these","those","i",
        "you","he","she","we","they","me","him","her","us","them","my","your",
        "his","our","their","as","by","for","with","from","into","over",
        "under","so","not","no","do","does","did","have","has","had","will",
        "would","can","could","he's","it's","there","here","then","than",
        "when","where","what","which","who","whom","whose","all","any","some",
        "more","most", NULL};
    size_t i;
    for (i = 0; stops[i]; ++i)
        if (strlen(stops[i]) == n && !memcmp(stops[i], w, n)) return 1;
    return 0;
}

typedef struct { char *bytes; size_t len; } RToken;
typedef struct { uint64_t meal; RToken *toks; size_t ntoks; } RFrag;
static RFrag *rfrags;
static size_t rfrag_n, rfrag_cap;

static void rtokens_free(RToken *toks, size_t ntoks) {
    size_t i;
    for (i = 0; i < ntoks; ++i) free(toks[i].bytes);
    free(toks);
}

static int rfrag_push(uint64_t meal, RToken *toks, size_t ntoks) {
    if (rfrag_n == rfrag_cap) {
        size_t next = rfrag_cap ? rfrag_cap * 2 : 64;
        RFrag *p = realloc(rfrags, next * sizeof *p);
        if (!p) return 0;
        rfrags = p;
        rfrag_cap = next;
    }
    rfrags[rfrag_n++] = (RFrag){meal, toks, ntoks};
    return 1;
}

/* tokenize one sentence into a fresh token array; returns count */
static size_t r_tokenize(const char *s, size_t n, RToken **out) {
    static const char strip[] = ".,!?;:\"'()[]{}-*_";
    RToken *toks = NULL;
    size_t ntoks = 0, cap = 0, i = 0;
    *out = NULL;
    while (i < n) {
        while (i < n && r_space((unsigned char)s[i])) i++;
        size_t j = i;
        while (j < n && !r_space((unsigned char)s[j])) j++;
        if (j > i) {
            size_t a = i, b = j;
            while (a < b && strchr(strip, s[a]) && s[a]) a++;
            while (b > a && strchr(strip, s[b - 1]) && s[b - 1]) b--;
            size_t wl = b - a;
            if (wl > 2) {
                char *w = malloc(wl + 1);
                if (!w) {
                    rtokens_free(toks, ntoks);
                    return (size_t)-1;
                }
                size_t k;
                for (k = 0; k < wl; ++k) {
                    char c = s[a + k];
                    if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
                    w[k] = c;
                }
                w[wl] = 0;
                if (r_stop(w, wl)) {
                    free(w);
                } else {
                    if (ntoks == cap) {
                        size_t next = cap ? cap * 2 : 8;
                        RToken *p = realloc(toks, next * sizeof *p);
                        if (!p) {
                            free(w);
                            rtokens_free(toks, ntoks);
                            return (size_t)-1;
                        }
                        toks = p;
                        cap = next;
                    }
                    toks[ntoks++] = (RToken){w, wl};
                }
            }
        }
        i = j;
    }
    *out = toks;
    return ntoks;
}

/* segment a speech blob into sentences and store fragments (>=2 tokens) */
static int r_eat_blob(uint64_t meal, const unsigned char *text, size_t n) {
    size_t start = 0, i;
    /* collapse to blocks split on newline, blanks, newline */
    for (i = 0; i <= n; ++i) {
        int block_end = 0;
        size_t next_start = 0;
        if (i == n) {
            block_end = 1;
            next_start = n;
        } else if (text[i] == '\n') {
            size_t j = i + 1;
            while (j < n && text[j] != '\n' && r_space(text[j])) j++;
            if (j < n && text[j] == '\n') {
                block_end = 1;
                next_start = j + 1;
            }
        }
        if (!block_end) continue;
        /* whitespace-collapse the block, then cut on .!? + space */
        size_t bl = i - start;
        char *line = malloc(bl + 1);
        if (!line) return 0;
        size_t li = 0, k = 0;
        while (k < bl) {
            while (k < bl && r_space(text[start + k])) k++;
            size_t w0 = k;
            while (k < bl && !r_space(text[start + k])) k++;
            if (k > w0) {
                if (li) line[li++] = ' ';
                memcpy(line + li, text + start + w0, k - w0);
                li += k - w0;
            }
        }
        line[li] = 0;
        size_t p = 0, q;
        for (q = 0; q < li; ++q) {
            char c = line[q];
            int cut = (c == '.' || c == '!' || c == '?') &&
                      (q + 1 == li || line[q + 1] == ' ');
            if (!cut && q + 1 < li) continue;
            size_t e = cut ? q + 1 : li;
            while (p < e && line[p] == ' ') p++;
            size_t sl = e - p;
            while (sl && line[p + sl - 1] == ' ') sl--;
            if (sl > 12) {
                RToken *toks = NULL;
                size_t ntoks = r_tokenize(line + p, sl, &toks);
                if (ntoks == (size_t)-1) { free(line); return 0; }
                if (ntoks >= 2) {
                    if (!rfrag_push(meal, toks, ntoks)) {
                        rtokens_free(toks, ntoks);
                        free(line);
                        return 0;
                    }
                } else {
                    rtokens_free(toks, ntoks);
                }
            }
            p = e;
            while (p < li && line[p] == ' ') p++;
            if (cut) q = p ? p - 1 : 0;
        }
        free(line);
        start = next_start;
        if (i == n) break;
        i = next_start ? next_start - 1 : i;
    }
    return 1;
}

static int r_shape_match_tokens(const char *shape, size_t shape_len,
                                const RToken *toks, size_t ntoks) {
    size_t pos = 0, i;
    for (i = 0; i < ntoks; ++i) {
        if (pos > shape_len || toks[i].len > shape_len - pos) return 0;
        if (memcmp(shape + pos, toks[i].bytes, toks[i].len)) return 0;
        pos += toks[i].len;
        if (i + 1 < ntoks) {
            if (pos >= shape_len || shape[pos] != ' ') return 0;
            pos++;
        }
    }
    return pos == shape_len;
}

/* rent at a pinned meal: adjacent pair/triple support over distinct meals */
static int r_shape_alive(const char *shape, size_t shape_len, uint64_t after) {
    uint64_t support = 0, last = 0, last_counted = 0;
    size_t f, i;
    for (f = 0; f < rfrag_n; ++f) {
        if (rfrags[f].meal > after) continue;
        int hit = 0;
        for (i = 0; i + 1 < rfrags[f].ntoks && !hit; ++i) {
            if (r_shape_match_tokens(shape, shape_len, &rfrags[f].toks[i], 2))
                hit = 1;
            if (!hit && i + 2 < rfrags[f].ntoks) {
                if (r_shape_match_tokens(shape, shape_len,
                                         &rfrags[f].toks[i], 3))
                    hit = 1;
            }
        }
        if (hit) {
            if (rfrags[f].meal != last_counted) {
                support++;
                last_counted = rfrags[f].meal;
            }
            if (rfrags[f].meal > last) last = rfrags[f].meal;
        }
    }
    return support >= 2 && after - last < 16;
}

/* ---- proposals prefixes, collected for the parliament's pin ---- */
static MainPrefix *pprefixes;
static size_t pprefix_n, pprefix_cap;

static int pprefix_add(uint64_t records, uint64_t chain) {
    if (pprefix_n == pprefix_cap) {
        size_t next = pprefix_cap ? pprefix_cap * 2 : 16;
        MainPrefix *p = realloc(pprefixes, next * sizeof *p);
        if (!p) return 0;
        pprefixes = p;
        pprefix_cap = next;
    }
    pprefixes[pprefix_n++] = (MainPrefix){records, chain};
    return 1;
}

static int pprefix_has(uint64_t records, uint64_t chain) {
    size_t i;
    if (records == 0) return chain == 0xcbf29ce484222325ULL;
    for (i = 0; i < pprefix_n; ++i)
        if (pprefixes[i].meals == records && pprefixes[i].chain == chain)
            return 1;
    return 0;
}

/* ---- a minimal school-at-prefix parser for ballot recomputation.
   Full grammar is already enforced by check_school over the whole file;
   this pass rebuilds STATE at a pinned record count and verifies the
   pinned chain. ---- */

typedef struct {
    char *shape;
    size_t shape_len;
    uint64_t arity, after, base_total, cand_total;
    int verdicted, passed;
} PHyp;

typedef struct {
    PHyp *hyps;
    size_t nhyps;
    size_t *legal; /* hyp index per glyph, in L order */
    size_t nlegal;
    uint64_t chain, records;
} PSchool;

static void pschool_free(PSchool *ps) {
    size_t i;
    for (i = 0; i < ps->nhyps; ++i) free(ps->hyps[i].shape);
    free(ps->hyps);
    free(ps->legal);
}

static int parse_school_prefix(uint64_t limit, PSchool *ps) {
    memset(ps, 0, sizeof *ps);
    ps->chain = 0xcbf29ce484222325ULL;
    FILE *sf = fopen(school_path, "rb");
    if (!sf) return 0;
    char *line = NULL;
    size_t cap = 0;
    size_t hyp_cap = 0, legal_cap = 0;
    for (;;) {
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(sf)) != EOF) {
            if (c == '\n') { sealed = 1; break; }
            if (len + 2 > cap) {
                cap = cap ? cap * 2 : 256;
                line = realloc(line, cap);
                if (!line) { fclose(sf); return 0; }
            }
            line[len++] = (char)c;
        }
        if (len == 0 && !sealed) break;
        if (!sealed || len < 18) { fclose(sf); free(line); return 0; }
        size_t plen = len - 17;
        ps->chain = hash_fold((unsigned char *)line, plen, ps->chain);
        ps->records++;
        const char *fl[10];
        size_t fn[10];
        int nf = fields_split(line, plen, fl, fn, 10);
        if (nf < 1) { fclose(sf); free(line); return 0; }
        if (fn[0] == 1 && fl[0][0] == 'H' && nf == 9) {
            uint64_t arity, slen, after;
            if (parse_u64(fl[2], fn[2], &arity) &&
                parse_u64(fl[3], fn[3], &slen) && fn[4] == slen &&
                parse_u64(fl[5], fn[5], &after)) {
                if (ps->nhyps == hyp_cap) {
                    size_t next = hyp_cap ? hyp_cap * 2 : 16;
                    PHyp *p = realloc(ps->hyps, next * sizeof *p);
                    if (!p) { fclose(sf); free(line); return 0; }
                    ps->hyps = p;
                    hyp_cap = next;
                }
                PHyp *h = &ps->hyps[ps->nhyps];
                memset(h, 0, sizeof *h);
                h->shape = malloc(fn[4] ? fn[4] : 1);
                if (!h->shape) { fclose(sf); free(line); return 0; }
                memcpy(h->shape, fl[4], fn[4]);
                h->shape_len = fn[4];
                h->arity = arity;
                h->after = after;
                ps->nhyps++;
            }
        } else if (fn[0] == 1 && fl[0][0] == 'O' && nf == 6) {
            uint64_t id, base_mb, cand_mb;
            if (parse_u64(fl[1], fn[1], &id) && id >= 1 && id <= ps->nhyps &&
                parse_u64(fl[4], fn[4], &base_mb) &&
                parse_u64(fl[5], fn[5], &cand_mb)) {
                ps->hyps[id - 1].base_total += base_mb;
                ps->hyps[id - 1].cand_total += cand_mb;
            }
        } else if (fn[0] == 1 && fl[0][0] == 'V' && nf == 5) {
            uint64_t id;
            if (parse_u64(fl[1], fn[1], &id) && id >= 1 && id <= ps->nhyps) {
                ps->hyps[id - 1].verdicted = 1;
                ps->hyps[id - 1].passed = field_eq(fl[2], fn[2], "pass");
            }
        } else if (fn[0] == 1 && fl[0][0] == 'L' && nf == 3) {
            uint64_t id;
            if (parse_u64(fl[1], fn[1], &id) && id >= 1 && id <= ps->nhyps) {
                if (ps->nlegal == legal_cap) {
                    size_t next = legal_cap ? legal_cap * 2 : 8;
                    size_t *p = realloc(ps->legal, next * sizeof *p);
                    if (!p) { fclose(sf); free(line); return 0; }
                    ps->legal = p;
                    legal_cap = next;
                }
                ps->legal[ps->nlegal++] = id - 1;
            }
        }
        if (limit && ps->records == limit) break;
    }
    fclose(sf);
    free(line);
    return 1;
}

static uint64_t r_unit_id(uint64_t arity, const char *shape, size_t n) {
    unsigned char head[2];
    head[0] = (unsigned char)arity;
    head[1] = 0;
    uint64_t h = hash_fold(head, 2, 0xcbf29ce484222325ULL);
    return hash_fold((const unsigned char *)shape, n, h);
}

/* ---- the parliament check proper ---- */

typedef struct {
    uint64_t pid, glyph;
    int unheard, opaque;
    char version[33];
    char unit_hex[17];
    uint64_t school_records, school_chain, after_meals, main_chain;
    uint64_t prop_records, prop_chain;
    char findings[3][64];
    size_t njur;
    int has_v;
    char verdict[9];
    uint64_t scar_closing;
} PBallot;

/* parliament prefixes and admissions, collected for the mint's pins */
static MainPrefix *parl_prefixes;
static size_t parl_prefix_n, parl_prefix_cap;
typedef struct {
    uint64_t at_records;
    char version[33];
    char unit_hex[17];
    char verdict[9];
} ParlAdmission;
static ParlAdmission *parl_admissions;
static size_t parl_adm_n, parl_adm_cap;

static int parl_prefix_add(uint64_t records, uint64_t chain) {
    if (parl_prefix_n == parl_prefix_cap) {
        size_t next = parl_prefix_cap ? parl_prefix_cap * 2 : 16;
        MainPrefix *p = realloc(parl_prefixes, next * sizeof *p);
        if (!p) return 0;
        parl_prefixes = p;
        parl_prefix_cap = next;
    }
    parl_prefixes[parl_prefix_n++] = (MainPrefix){records, chain};
    return 1;
}

static int parl_prefix_has(uint64_t records, uint64_t chain) {
    size_t i;
    for (i = 0; i < parl_prefix_n; ++i)
        if (parl_prefixes[i].meals == records && parl_prefixes[i].chain == chain)
            return 1;
    return 0;
}

static const ParlAdmission *parl_citizen_at(uint64_t records,
                                            const char *version,
                                            const char *unit_hex) {
    size_t i;
    for (i = 0; i < parl_adm_n; ++i)
        if (parl_admissions[i].at_records <= records &&
                !strcmp(parl_admissions[i].version, version) &&
                !strcmp(parl_admissions[i].unit_hex, unit_hex))
            return &parl_admissions[i];
    return NULL;
}

static void check_parliament(void) {
    FILE *pf = fopen(parl_path, "rb");
    if (!pf) return;
    uint64_t chain = 0xcbf29ce484222325ULL;
    unsigned long long lineno = 0, p_count = 0, j_count = 0, vv_count = 0;
    int law_seen = 0;
    PBallot *bs = NULL;
    size_t bs_n = 0, bs_cap = 0;
    char *line = NULL;
    size_t cap = 0;
    static const char *jur[3] = {"exam", "record", "rent"};
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
        if (!sealed) refuse_parl(lineno, "record is unsealed (no newline)");
        if (len + 1 >= RECORD_MAX) refuse_parl(lineno, "record exceeds 4096-byte law");
        if (len < 18) refuse_parl(lineno, "record too short for a chain field");
        if (line[len - 17] != '\t') refuse_parl(lineno, "no tab before the chain field");
        uint64_t claimed;
        if (!parse_hex16(line + len - 16, 16, &claimed))
            refuse_parl(lineno, "chain field is not lowercase hex16");
        size_t plen = len - 17;
        chain = hash_fold((unsigned char *)line, plen, chain);
        if (chain != claimed) refuse_parl(lineno, "chain does not fold");
        if (!parl_prefix_add(lineno, chain)) refuse_parl(lineno, "memory");

        const char *fl[12];
        size_t fn[12];
        int nf = fields_split(line, plen, fl, fn, 12);
        if (nf < 1 || fn[0] != 1) refuse_parl(lineno, "no record type");
        char type = fl[0][0];
        if (type == 'Q') {
            if (lineno != 1 || nf != 3 || !field_eq(fl[1], fn[1], "1") ||
                    !field_eq(fl[2], fn[2], "body4-parl-v2"))
                refuse_parl(lineno, "parliament law record");
            law_seen = 1;
            continue;
        }
        if (!law_seen) refuse_parl(lineno, "record before parliament law");
        if (type == 'P') {
            if (bs_n && (!bs[bs_n - 1].has_v || bs[bs_n - 1].njur != 3))
                refuse_parl(lineno, "a new petition before the last ballot closed");
            if (nf != 11) refuse_parl(lineno, "P arity");
            if (bs_n == bs_cap) {
                size_t next = bs_cap ? bs_cap * 2 : 8;
                PBallot *p = realloc(bs, next * sizeof *p);
                if (!p) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
                bs = p;
                bs_cap = next;
            }
            PBallot *b = &bs[bs_n];
            memset(b, 0, sizeof *b);
            if (!parse_u64(fl[1], fn[1], &b->pid) || b->pid != bs_n + 1)
                refuse_parl(lineno, "P petition id is not sequential");
            if (!parse_u64(fl[2], fn[2], &b->glyph) || b->glyph == 0)
                refuse_parl(lineno, "P glyph");
            if (fn[3] == 1 && fl[3][0] == '-' && fn[4] == 1 && fl[4][0] == '-') {
                b->unheard = 1;
                snprintf(b->unit_hex, sizeof b->unit_hex, "-");
                snprintf(b->version, sizeof b->version, "-");
            } else {
                uint64_t uid;
                if (!parse_hex16(fl[3], fn[3], &uid) ||
                        !label_valid(fl[4], fn[4]) || fn[4] > 32)
                    refuse_parl(lineno, "P identity grammar");
                snprintf(b->unit_hex, sizeof b->unit_hex, "%.16s", fl[3]);
                snprintf(b->version, sizeof b->version, "%.*s", (int)fn[4], fl[4]);
                if (!field_eq(fl[4], fn[4], "pair-triple-v1")) b->opaque = 1;
            }
            if (!parse_u64(fl[5], fn[5], &b->school_records) ||
                    b->school_records == 0 ||
                    !parse_hex16(fl[6], fn[6], &b->school_chain) ||
                    !parse_u64(fl[7], fn[7], &b->after_meals) ||
                    !parse_hex16(fl[8], fn[8], &b->main_chain) ||
                    !parse_u64(fl[9], fn[9], &b->prop_records) ||
                    !parse_hex16(fl[10], fn[10], &b->prop_chain))
                refuse_parl(lineno, "P field grammar");
            if (!(b->after_meals == 0 && b->main_chain == 0xcbf29ce484222325ULL) &&
                    !prefix_has(b->after_meals, b->main_chain))
                refuse_parl(lineno, "P main prefix");
            if (!pprefix_has(b->prop_records, b->prop_chain))
                refuse_parl(lineno, "P proposals prefix");
            bs_n++;
            p_count++;
        } else if (type == 'J') {
            uint64_t pid;
            if (nf != 4 || !parse_u64(fl[1], fn[1], &pid))
                refuse_parl(lineno, "J field grammar");
            if (!bs_n || pid != bs[bs_n - 1].pid)
                refuse_parl(lineno, "J outside its ballot");
            PBallot *b = &bs[bs_n - 1];
            if (b->has_v || b->njur >= 3)
                refuse_parl(lineno, "J after the ballot closed");
            if (!field_eq(fl[2], fn[2], jur[b->njur]))
                refuse_parl(lineno, "J juror out of canonical order");
            if (fn[3] == 0 || fn[3] >= sizeof b->findings[0])
                refuse_parl(lineno, "J finding grammar");
            snprintf(b->findings[b->njur], sizeof b->findings[0], "%.*s",
                     (int)fn[3], fl[3]);
            b->njur++;
            j_count++;
        } else if (type == 'V') {
            uint64_t pid;
            if (nf != 3 || !parse_u64(fl[1], fn[1], &pid))
                refuse_parl(lineno, "V field grammar");
            if (!bs_n || pid != bs[bs_n - 1].pid)
                refuse_parl(lineno, "V outside its ballot");
            PBallot *b = &bs[bs_n - 1];
            if (b->has_v || b->njur != 3)
                refuse_parl(lineno, "V before all three jurors");
            if (!field_eq(fl[2], fn[2], "PASS") &&
                    !field_eq(fl[2], fn[2], "WEAKEN") &&
                    !field_eq(fl[2], fn[2], "FREEZE") &&
                    !field_eq(fl[2], fn[2], "SCAR") &&
                    !field_eq(fl[2], fn[2], "DARK") &&
                    !field_eq(fl[2], fn[2], "SILENCE"))
                refuse_parl(lineno, "V verdict class");
            snprintf(b->verdict, sizeof b->verdict, "%.*s", (int)fn[2], fl[2]);
            b->has_v = 1;
            vv_count++;
            if (!b->unheard && (!strcmp(b->verdict, "PASS") ||
                                !strcmp(b->verdict, "WEAKEN"))) {
                if (parl_adm_n == parl_adm_cap) {
                    size_t next = parl_adm_cap ? parl_adm_cap * 2 : 8;
                    ParlAdmission *p = realloc(parl_admissions, next * sizeof *p);
                    if (!p) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
                    parl_admissions = p;
                    parl_adm_cap = next;
                }
                ParlAdmission *a = &parl_admissions[parl_adm_n++];
                a->at_records = lineno;
                snprintf(a->version, sizeof a->version, "%s", b->version);
                snprintf(a->unit_hex, sizeof a->unit_hex, "%s", b->unit_hex);
                snprintf(a->verdict, sizeof a->verdict, "%s", b->verdict);
            }
        } else {
            refuse_parl(lineno, "unknown parliament record type");
        }
    }
    fclose(pf);
    free(line);
    if (!law_seen) refuse_parl(lineno, "parliament has no law record");

    /* reproduce every complete ballot from its pinned truth */
    size_t bi, hi;
    for (bi = 0; bi < bs_n; ++bi) {
        PBallot *b = &bs[bi];
        int complete = b->has_v && b->njur == 3;
        if (!complete) {
            if (bi + 1 != bs_n)
                refuse_parl(lineno, "an abandoned ballot before the tip");
            continue;
        }
        PSchool ps;
        if (!parse_school_prefix(b->school_records, &ps))
            refuse_parl(lineno, "cannot reread the pinned school prefix");
        if (ps.records != b->school_records || ps.chain != b->school_chain)
            refuse_parl(lineno, "a pinned school prefix does not reproduce");
        if (b->unheard) {
            if (b->glyph <= ps.nlegal)
                refuse_parl(lineno, "silence claimed for a glyph the prefix holds");
            size_t k;
            for (k = 0; k < 3; ++k)
                if (strcmp(b->findings[k], "unheard"))
                    refuse_parl(lineno, "false finding");
            if (strcmp(b->verdict, "SILENCE")) refuse_parl(lineno, "false verdict");
            pschool_free(&ps);
            continue;
        }
        if (b->opaque) {
            if (b->glyph > ps.nlegal)
                refuse_parl(lineno, "an opaque petition needs an existing glyph");
            size_t k;
            for (k = 0; k < 3; ++k)
                if (strcmp(b->findings[k], "opaque"))
                    refuse_parl(lineno, "false finding");
            if (strcmp(b->verdict, "DARK")) refuse_parl(lineno, "false verdict");
            pschool_free(&ps);
            continue;
        }
        if (b->glyph > ps.nlegal)
            refuse_parl(lineno, "a known petition names a glyph its prefix lacks");
        PHyp *gh = &ps.hyps[ps.legal[b->glyph - 1]];
        uint64_t uid = r_unit_id(gh->arity, gh->shape, gh->shape_len);
        char uid_hex[17];
        snprintf(uid_hex, sizeof uid_hex, "%016llx", (unsigned long long)uid);
        if (strcmp(uid_hex, b->unit_hex))
            refuse_parl(lineno, "a pinned unit id does not reproduce");
        uint64_t gain = gh->base_total - gh->cand_total;
        const char *exam = gain >= 16000000ULL ? "strong" : "adequate";
        uint64_t closing = 0;
        for (hi = 0; hi < ps.nhyps; ++hi) {
            PHyp *h = &ps.hyps[hi];
            if (r_unit_id(h->arity, h->shape, h->shape_len) != uid) continue;
            if (h->verdicted && !h->passed) {
                uint64_t cend = h->after + 8;
                if (cend > closing) closing = cend;
            }
        }
        char record[64];
        if (closing)
            snprintf(record, sizeof record, "scarred:%llu",
                     (unsigned long long)closing);
        else
            snprintf(record, sizeof record, "clean");
        const char *rent =
            r_shape_alive(gh->shape, gh->shape_len, b->after_meals)
                ? "alive" : "starved";
        if (strcmp(b->findings[0], exam) || strcmp(b->findings[1], record) ||
                strcmp(b->findings[2], rent))
            refuse_parl(lineno, "false finding");
        const char *want;
        if (closing && b->after_meals < closing + 16) want = "SCAR";
        else if (!strcmp(rent, "starved")) want = "FREEZE";
        else if (!strcmp(exam, "strong") && !closing) want = "PASS";
        else want = "WEAKEN";
        if (strcmp(b->verdict, want)) refuse_parl(lineno, "false verdict");
        b->scar_closing = closing;
        pschool_free(&ps);
    }

    /* history laws visible in the chain itself */
    for (bi = 0; bi < bs_n; ++bi) {
        PBallot *b = &bs[bi];
        if (!b->has_v) continue;
        size_t bj;
        for (bj = bi + 1; bj < bs_n; ++bj) {
            PBallot *later = &bs[bj];
            if (later->unheard || b->unheard) continue;
            if (strcmp(later->version, b->version) ||
                    strcmp(later->unit_hex, b->unit_hex))
                continue;
            if (!strcmp(b->verdict, "PASS") || !strcmp(b->verdict, "WEAKEN"))
                refuse_parl(lineno, "an admitted identity was petitioned again");
            if (!strcmp(b->verdict, "DARK") && later->opaque)
                refuse_parl(lineno, "a dark identity returned without a new law");
            if (!strcmp(b->verdict, "SCAR") && b->scar_closing &&
                    later->after_meals < b->scar_closing + 16)
                refuse_parl(lineno, "a petition inside a scar window");
            if (!strcmp(b->verdict, "FREEZE") &&
                    later->after_meals < b->after_meals + 8)
                refuse_parl(lineno, "a petition inside a freeze window");
        }
    }
    free(bs);
    printf("parliament: %llu records (Q 1, P %llu, J %llu, V %llu), "
           "chain %016llx\n",
           lineno, p_count, j_count, vv_count, (unsigned long long)chain);
}

/* body 5: the mint's integer side, owned fully by the independent hand.
   The reader recomputes the dataset law, the baseline, the verdict and
   the governed budget from the pinned prefixes with its own text
   pipeline; it does NOT retrain and does not re-price loss -- a weight
   digest is the receipt of one training sitting on one installed
   binary. */
static const char *notes_path = ".mycelium.notes";
static const char *notes_dir = ".mycelium.notes.d";

_Noreturn static void refuse_notes(unsigned long long line, const char *what) {
    fprintf(stderr, "ledger_check: notes line %llu: %s\n", line, what);
    exit(1);
}

/* does the fragment hold a complete shape occurrence; do the
   occurrences cover every token (nothing would survive the mask) */
static void r_frag_occurrence(const RFrag *f, const char *shape,
                              size_t shape_len, size_t arity,
                              int *any, int *all_covered) {
    size_t i, k;
    char *covered = calloc(f->ntoks ? f->ntoks : 1, 1);
    if (!covered) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
    *any = 0;
    if (arity && f->ntoks >= arity)
        for (i = 0; i + arity <= f->ntoks; ++i)
            if (r_shape_match_tokens(shape, shape_len, &f->toks[i], arity)) {
                *any = 1;
                for (k = 0; k < arity; ++k) covered[i + k] = 1;
            }
    *all_covered = 1;
    for (i = 0; i < f->ntoks; ++i)
        if (!covered[i]) { *all_covered = 0; break; }
    free(covered);
}

/* the sealed dataset law in integers only: masked positives with
   survivors, negatives trimmed to equal count, stratified 1-in-4
   holdout per label.  The counts do not depend on the hash order, so
   the reader never sorts.  -1 = too few negatives, -2 = a split lost
   a whole label; a lawful writer refuses both before any T exists. */
static int r_note_dataset(const char *shape, size_t shape_len, uint64_t arity,
                          uint64_t after, uint64_t *pos, uint64_t *neg,
                          uint64_t *hpos, uint64_t *hneg) {
    uint64_t pn = 0, nn = 0;
    size_t i;
    for (i = 0; i < rfrag_n; ++i) {
        const RFrag *f = &rfrags[i];
        if (f->meal > after) continue;
        int any, all_covered;
        r_frag_occurrence(f, shape, shape_len, (size_t)arity, &any,
                          &all_covered);
        if (any) {
            if (all_covered) continue; /* nothing survives the mask */
            pn++;
        } else {
            nn++;
        }
    }
    if (nn < pn) return -1;
    nn = pn;
    *pos = pn;
    *neg = nn;
    *hpos = pn ? (pn + 3) / 4 : 0;
    *hneg = *hpos;
    if (!*hpos || !*hneg || pn - *hpos == 0 || nn - *hneg == 0) return -2;
    return 1;
}

typedef struct {
    char unit_hex[17];
    char version[33];
    uint64_t parl_records, after_meals, main_chain, forge_h, forge_l, seed;
    uint64_t last_t_meal, last_hits, last_base;
    const char *shape;
    size_t shape_len;
    uint64_t arity;
    int open_t, has_t, alive;
} RNote;

static void check_notes(void) {
    FILE *nf = fopen(notes_path, "rb");
    if (!nf) return;
    uint64_t chain = 0xcbf29ce484222325ULL;
    unsigned long long lineno = 0, m_count = 0, t_count = 0, e_count = 0,
                       r_count = 0, z_count = 0;
    int law_seen = 0;
    RNote *ns = NULL;
    size_t ns_n = 0, ns_cap = 0;
    char *line = NULL;
    size_t cap = 0;
    PSchool ps;
    int have_school = parse_school_prefix(0, &ps);
    for (;;) {
        size_t len = 0;
        int c, sealed = 0;
        while ((c = fgetc(nf)) != EOF) {
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
        if (!sealed) refuse_notes(lineno, "record is unsealed (no newline)");
        if (len + 1 >= RECORD_MAX) refuse_notes(lineno, "record exceeds 4096-byte law");
        if (len < 18) refuse_notes(lineno, "record too short for a chain field");
        if (line[len - 17] != '\t') refuse_notes(lineno, "no tab before the chain field");
        uint64_t claimed;
        if (!parse_hex16(line + len - 16, 16, &claimed))
            refuse_notes(lineno, "chain field is not lowercase hex16");
        size_t plen = len - 17;
        chain = hash_fold((unsigned char *)line, plen, chain);
        if (chain != claimed) refuse_notes(lineno, "notes chain broken");

        const char *fl[16];
        size_t fn[16];
        int nfld = fields_split(line, plen, fl, fn, 16);
        if (nfld < 1 || fn[0] != 1) refuse_notes(lineno, "no record type");
        char type = fl[0][0];
        if (type == 'N') {
            if (lineno != 1 || nfld != 3 || !field_eq(fl[1], fn[1], "1") ||
                    !field_eq(fl[2], fn[2], "body5-notes-v1"))
                refuse_notes(lineno, "notes law record");
            law_seen = 1;
            continue;
        }
        if (!law_seen) refuse_notes(lineno, "record before notes law");
        if (type == 'M') {
            uint64_t id, uid, pr_, pc, am, mc, fh, flg, sd;
            size_t k;
            if (nfld != 11) refuse_notes(lineno, "M arity");
            if (!parse_u64(fl[1], fn[1], &id) || id != ns_n + 1)
                refuse_notes(lineno, "M note id is not sequential");
            if (!parse_hex16(fl[2], fn[2], &uid) || !label_valid(fl[3], fn[3]) ||
                    fn[3] > 32)
                refuse_notes(lineno, "M identity grammar");
            if (!parse_u64(fl[4], fn[4], &pr_) || pr_ == 0 ||
                    !parse_hex16(fl[5], fn[5], &pc) ||
                    !parse_u64(fl[6], fn[6], &am) ||
                    !parse_hex16(fl[7], fn[7], &mc) ||
                    !parse_hex16(fl[8], fn[8], &fh) ||
                    !parse_hex16(fl[9], fn[9], &flg) ||
                    !parse_hex16(fl[10], fn[10], &sd))
                refuse_notes(lineno, "M field grammar");
            if (!parl_prefix_has(pr_, pc)) refuse_notes(lineno, "M parliament prefix");
            if (!prefix_has(am, mc)) refuse_notes(lineno, "M main prefix");
            if (ns_n == ns_cap) {
                size_t next = ns_cap ? ns_cap * 2 : 8;
                RNote *p = realloc(ns, next * sizeof *p);
                if (!p) { fprintf(stderr, "ledger_check: memory\n"); exit(1); }
                ns = p;
                ns_cap = next;
            }
            RNote *n = &ns[ns_n++];
            memset(n, 0, sizeof *n);
            snprintf(n->unit_hex, sizeof n->unit_hex, "%.*s", (int)fn[2], fl[2]);
            snprintf(n->version, sizeof n->version, "%.*s", (int)fn[3], fl[3]);
            n->parl_records = pr_;
            n->after_meals = am;
            n->main_chain = mc;
            n->forge_h = fh;
            n->forge_l = flg;
            n->seed = sd;
            n->alive = 1;
            if (!parl_citizen_at(pr_, n->version, n->unit_hex))
                refuse_notes(lineno, "M names no citizen at its pinned hour");
            for (k = 0; k + 1 < ns_n; ++k)
                if (!strcmp(ns[k].version, n->version) &&
                        !strcmp(ns[k].unit_hex, n->unit_hex))
                    refuse_notes(lineno, "identity already minted");
            if (have_school) {
                size_t g;
                for (g = 0; g < ps.nlegal; ++g) {
                    PHyp *h = &ps.hyps[ps.legal[g]];
                    char hexbuf[17];
                    snprintf(hexbuf, sizeof hexbuf, "%016llx",
                             (unsigned long long)r_unit_id(h->arity, h->shape,
                                                           h->shape_len));
                    if (!strcmp(hexbuf, n->unit_hex)) {
                        n->shape = h->shape;
                        n->shape_len = h->shape_len;
                        n->arity = h->arity;
                        break;
                    }
                }
                if (!n->shape)
                    refuse_notes(lineno, "citizen unit has no glyph shape");
                if (!r_shape_alive(n->shape, n->shape_len, am))
                    refuse_notes(lineno, "mint pinned at a starved hour");
            }
            m_count++;
        } else if (type == 'T') {
            uint64_t id, am, mc, fh, flg, sd, steps, loss, pos, neg, hp, hn,
                hits, base, wd;
            if (nfld != 16) refuse_notes(lineno, "T arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > ns_n)
                refuse_notes(lineno, "T names no note");
            RNote *n = &ns[id - 1];
            if (id != ns_n) refuse_notes(lineno, "T outside the open note");
            if (n->open_t) refuse_notes(lineno, "T before the last training closed");
            if (!n->alive) refuse_notes(lineno, "T for a note in the morgue");
            if (!parse_u64(fl[2], fn[2], &am) || !parse_hex16(fl[3], fn[3], &mc) ||
                    !parse_hex16(fl[4], fn[4], &fh) ||
                    !parse_hex16(fl[5], fn[5], &flg) ||
                    !parse_hex16(fl[6], fn[6], &sd) ||
                    !parse_u64(fl[7], fn[7], &steps) ||
                    !parse_u64(fl[8], fn[8], &loss) ||
                    !parse_u64(fl[9], fn[9], &pos) ||
                    !parse_u64(fl[10], fn[10], &neg) ||
                    !parse_u64(fl[11], fn[11], &hp) ||
                    !parse_u64(fl[12], fn[12], &hn) ||
                    !parse_u64(fl[13], fn[13], &hits) ||
                    !parse_u64(fl[14], fn[14], &base) ||
                    !parse_hex16(fl[15], fn[15], &wd))
                refuse_notes(lineno, "T field grammar");
            if (!prefix_has(am, mc)) refuse_notes(lineno, "T main prefix");
            if (!n->has_t) {
                if (am != n->after_meals || mc != n->main_chain ||
                        fh != n->forge_h || flg != n->forge_l || sd != n->seed)
                    refuse_notes(lineno,
                                 "first training does not repeat the mint pin");
            } else if (am < n->last_t_meal + 8)
                refuse_notes(lineno, "training inside the cooldown");
            if (steps != 512 && steps != 256)
                refuse_notes(lineno, "T steps outside the governed budgets");
            {
                const ParlAdmission *a =
                    parl_citizen_at(n->parl_records, n->version, n->unit_hex);
                uint64_t budget =
                    a && !strcmp(a->verdict, "PASS") ? 512 : 256;
                if (steps != budget)
                    refuse_notes(lineno,
                                 "T budget does not match the citizen's verdict");
            }
            if (base != (hp > hn ? hp : hn)) refuse_notes(lineno, "false baseline");
            if (hits > hp + hn) refuse_notes(lineno, "holdout hits exceed the holdout");
            if (have_school) {
                uint64_t rp, rn, rhp, rhn;
                if (r_note_dataset(n->shape, n->shape_len, n->arity, am,
                                   &rp, &rn, &rhp, &rhn) < 1 ||
                        rp != pos || rn != neg || rhp != hp || rhn != hn)
                    refuse_notes(lineno, "dataset drifted from its pin");
            }
            {
                char path[4096];
                unsigned char blob[97 * 4 + 1];
                size_t bn, d;
                FILE *bf;
                snprintf(path, sizeof path, "%s/%.16s", notes_dir, fl[15]);
                bf = fopen(path, "rb");
                if (!bf) refuse_notes(lineno, "weight blob is missing");
                bn = fread(blob, 1, sizeof blob, bf);
                if (ferror(bf)) {
                    fclose(bf);
                    refuse_notes(lineno, "weight blob unreadable");
                }
                fclose(bf);
                if (bn != 97 * 4)
                    refuse_notes(lineno, "weight blob is not canonical");
                if (hash_fold(blob, bn, 0xcbf29ce484222325ULL) != wd)
                    refuse_notes(lineno, "weight blob digest mismatch");
                for (d = 0; d < 97; ++d) {
                    float v;
                    memcpy(&v, blob + d * 4, 4);
                    if (!(v == v) || v > 3.4e38f || v < -3.4e38f)
                        refuse_notes(lineno,
                                     "weight blob holds a non-finite float");
                }
            }
            n->open_t = 1;
            n->has_t = 1;
            n->last_t_meal = am;
            n->last_hits = hits;
            n->last_base = base;
            t_count++;
        } else if (type == 'E') {
            uint64_t id;
            const char *want;
            if (nfld != 3) refuse_notes(lineno, "E arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > ns_n)
                refuse_notes(lineno, "E names no note");
            RNote *n = &ns[id - 1];
            if (!n->open_t) refuse_notes(lineno, "E without an open training");
            want = n->last_hits > n->last_base ? "LIT" : "DIM";
            if (!field_eq(fl[2], fn[2], want)) refuse_notes(lineno, "false verdict");
            n->open_t = 0;
            e_count++;
        } else if (type == 'R' || type == 'Z') {
            uint64_t id, am, mc;
            if (nfld != 4) refuse_notes(lineno, "rent arity");
            if (!parse_u64(fl[1], fn[1], &id) || id == 0 || id > ns_n ||
                    !parse_u64(fl[2], fn[2], &am) || !parse_hex16(fl[3], fn[3], &mc))
                refuse_notes(lineno, "rent field grammar");
            if (!prefix_has(am, mc)) refuse_notes(lineno, "rent main prefix");
            RNote *n = &ns[id - 1];
            if (type == 'R') {
                if (!n->alive) refuse_notes(lineno, "R for a note already in the morgue");
                if (n->shape && r_shape_alive(n->shape, n->shape_len, am))
                    refuse_notes(lineno, "R pinned at a well-fed hour");
                n->alive = 0;
                r_count++;
            } else {
                if (n->alive) refuse_notes(lineno, "Z for a note not in the morgue");
                if (n->shape && !r_shape_alive(n->shape, n->shape_len, am))
                    refuse_notes(lineno, "Z pinned at a starved hour");
                n->alive = 1;
                z_count++;
            }
        } else {
            refuse_notes(lineno, "unknown notes record type");
        }
    }
    fclose(nf);
    free(line);
    free(ns);
    if (have_school) pschool_free(&ps);
    if (!law_seen) refuse_notes(lineno, "notes has no law record");
    printf("notes: %llu records (N 1, M %llu, T %llu, E %llu, R %llu, Z %llu), "
           "chain %016llx\n",
           lineno, m_count, t_count, e_count, r_count, z_count,
           (unsigned long long)chain);
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
            /* the blob must still hold the sealed bytes; its tokens feed
               the parliament's independent rent recomputation */
            char blob[4096];
            snprintf(blob, sizeof blob, "%s/%.16s", grave_dir, fl[4]);
            FILE *bf = fopen(blob, "rb");
            if (!bf) refuse(lineno, "grave blob is missing");
            unsigned char *body = malloc(sbytes ? (size_t)sbytes : 1);
            if (!body) { fclose(bf); refuse(lineno, "memory"); }
            uint64_t bn = fread(body, 1, (size_t)sbytes, bf);
            int extra = fgetc(bf) != EOF;
            if (ferror(bf)) { fclose(bf); refuse(lineno, "grave blob unreadable"); }
            fclose(bf);
            if (bn != sbytes || extra)
                refuse(lineno, "grave blob byte count mismatch");
            if (hash_fold(body, (size_t)bn, 0xcbf29ce484222325ULL) != sdig)
                refuse(lineno, "grave blob digest mismatch");
            if (!r_eat_blob((uint64_t)g_count + 1, body, (size_t)bn))
                refuse(lineno, "memory");
            free(body);
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
        if (!prefix_add((uint64_t)g_count, chain)) refuse(lineno, "memory");
    }
    fclose(lf);
    if (!v_count) refuse(lineno, "ledger has no version record");

    printf("ledger_check: %llu records (V %llu, G %llu, U %llu), chain %016llx\n",
           lineno, v_count, g_count, u_count, (unsigned long long)chain);
    check_proposals();
    check_school((uint64_t)g_count);
    check_parliament();
    check_notes();
    printf("scope: internal consistency of the supplied file; a prefix cut at "
           "a record boundary needs an external witness\n");
    size_t i;
    for (i = 0; i < witness_n; ++i) free(witnesses[i].sline);
    free(witnesses);
    free(labels);
    free(prefixes);
    free(line);
    return 0;
}
