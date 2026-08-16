/* The warrant's independent external hand. It accepts only complete body-27
   sitting dockets: exact v2 law, candidate-bearing header, the declared
   number of canonical court lines in order, and a matching close digest.
   Concatenated complete sittings are legal. It shares no organism code.

   FNV-1a-64 remains a public witness, not a signature: this catches broken
   transport, non-canonical records, line tampering, law drift, and fabricated
   books made from valid leaves. It does not stop an adversary who rebuilds
   every public witness from source. rc 0 accepts; rc 1 refuses; rc 2 means
   that no complete sitting was supplied. */
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_CAP 1024
#define COURT_MAX_BYTES 16384ULL
#define COURT_MAX_SHORES 32

static const char COURT_LAW[] =
    "pattern-court law v2: abstain if changed=0 or gap-micro=0; "
    "replay if 2*matched-bytes>=bytes; order if gap-micro>=500000; "
    "stranger otherwise";

static uint64_t fnv_seed(void) {
    return 0xcbf29ce484222325ULL;
}

static uint64_t fnv_more(uint64_t h, const char *p, size_t n) {
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint8_t)p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static uint64_t fnv(const char *p, size_t n) {
    return fnv_more(fnv_seed(), p, n);
}

static uint64_t docket_record(uint64_t h, const char *record) {
    h = fnv_more(h, record, strlen(record));
    return fnv_more(h, "\n", 1);
}

static int lower_hex(const char *s, size_t n) {
    if (strlen(s) != n) return 0;
    for (size_t i = 0; i < n; ++i)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f'))) return 0;
    return 1;
}

static int context_ok(const char *s) {
    return strcmp(s, "cold") == 0 || lower_hex(s, 4);
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

static int token_i64(const char *s, int64_t *out) {
    if (!*s || *s == '+') return 0;
    errno = 0;
    char *end;
    long long v = strtoll(s, &end, 10);
    if (errno == ERANGE || end == s || *end) return 0;
    *out = (int64_t)v;
    return 1;
}

static int token_double(const char *s, double *out) {
    if (!*s || *s == '+') return 0;
    errno = 0;
    char *end;
    double v = strtod(s, &end);
    if (errno == ERANGE || end == s || *end || !isfinite(v)) return 0;
    *out = v;
    return 1;
}

/* A transcript record is newline-sealed. A full buffer without a newline is
   either overlong or an EOF fragment; both are refused, never split. */
static int read_record(char *line, size_t cap, int *lineno) {
    if (!fgets(line, (int)cap, stdin)) return ferror(stdin) ? -1 : 0;
    (*lineno)++;
    size_t len = strlen(line);
    if (!len || line[len - 1] != '\n') {
        char sink[256];
        while (fgets(sink, sizeof sink, stdin))
            if (strchr(sink, '\n')) break;
        return -2;
    }
    line[--len] = 0;
    if (len && line[len - 1] == '\r') line[--len] = 0;
    return 1;
}

static int parse_sitting(const char *line, const char *law_hex,
                         char candidate_hex[17], uint64_t *bytes,
                         char context[8], int *shores) {
    char digest[17] = {0}, ctx[8] = {0}, named_law[17] = {0};
    char nb_token[32] = {0}, ns_token[32] = {0};
    uint64_t nb = 0, ns64 = 0;
    int used = 0;
    int got = sscanf(line,
        "court sitting: candidate-digest=%16[0-9a-f] bytes=%31s "
        "context=%7s shores=%31s law-digest=%16[0-9a-f]%n",
        digest, nb_token, ctx, ns_token, named_law, &used);
    if (got != 5 || line[used] != 0 || !lower_hex(digest, 16) ||
            !token_u64(nb_token, &nb) || !token_u64(ns_token, &ns64) ||
            !context_ok(ctx) || ns64 < 1 || ns64 > COURT_MAX_SHORES ||
            nb < 2 || nb > COURT_MAX_BYTES ||
            !lower_hex(named_law, 16) || strcmp(named_law, law_hex) != 0)
        return 0;
    int ns = (int)ns64;
    char canonical[256];
    int n = snprintf(canonical, sizeof canonical,
        "court sitting: candidate-digest=%s bytes=%llu context=%s "
        "shores=%d law-digest=%s", digest, (unsigned long long)nb,
        ctx, ns, named_law);
    if (n < 0 || (size_t)n >= sizeof canonical || strcmp(line, canonical))
        return 0;
    memcpy(candidate_hex, digest, 17);
    memcpy(context, ctx, strlen(ctx) + 1);
    *bytes = (uint64_t)nb;
    *shores = ns;
    return 1;
}

static int parse_court(const char *line, const char *law_hex,
                       int expected_id, uint64_t sitting_bytes,
                       const char *sitting_context) {
    int used = 0;
    char digest[17] = {0}, context[8] = {0};
    char verdict[9] = {0}, receipt[17] = {0};
    char id_token[32] = {0}, bytes_token[32] = {0};
    char p_token[32] = {0}, q_token[32] = {0}, matched_token[32] = {0};
    char covered_token[32] = {0}, denominator_token[32] = {0};
    char gap_token[32] = {0}, gm_token[32] = {0};
    char changed_token[32] = {0}, shore_token[32] = {0};
    uint64_t id64 = 0, bytes = 0, covered = 0, denominator = 0;
    uint64_t changed = 0, shore = 0;
    int64_t gap_micro = 0;
    double p = 0.0, q = 0.0, matched = 0.0, gap = 0.0;
    int got = sscanf(line,
        "court %31[^:]: digest=%16[0-9a-f] context=%7s bytes=%31s "
        "P=%31s Q=%31s matched16=%31[^%%]%% "
        "matched-bytes=%31[0-9]/%31[0-9] G=%31s gap-micro=%31s "
        "changed=%31[0-9]/%31[0-9] verdict=%8[a-z] "
        "receipt=%16[0-9a-f]%n",
        id_token, digest, context, bytes_token, p_token, q_token,
        matched_token, covered_token, denominator_token, gap_token,
        gm_token, changed_token, shore_token, verdict, receipt, &used);
    if (got != 15 || line[used] != 0 ||
            !token_u64(id_token, &id64) || id64 > INT_MAX ||
            !token_u64(bytes_token, &bytes) ||
            !token_double(p_token, &p) || !token_double(q_token, &q) ||
            !token_double(matched_token, &matched) ||
            !token_u64(covered_token, &covered) ||
            !token_u64(denominator_token, &denominator) ||
            !token_double(gap_token, &gap) ||
            !token_i64(gm_token, &gap_micro) ||
            !token_u64(changed_token, &changed) ||
            !token_u64(shore_token, &shore) ||
            (int)id64 != expected_id ||
            !lower_hex(digest, 16) || !context_ok(context) ||
            strcmp(context, sitting_context) != 0 ||
            bytes != sitting_bytes || denominator != sitting_bytes ||
            denominator == 0 || covered > denominator || changed > shore ||
            !isfinite(p) || !isfinite(q) || !isfinite(matched) ||
            !isfinite(gap) || p < 0.0 || q < 0.0 || matched < 0.0 ||
            signbit(p) || signbit(q) || signbit(matched) ||
            (gap_micro == 0 && signbit(gap)) ||
            !lower_hex(receipt, 16))
        return 0;

    int id = (int)id64;
    char base[640], canonical[704];
    int bn = snprintf(base, sizeof base,
        "court %d: digest=%s context=%s bytes=%llu "
        "P=%.6f Q=%.6f matched16=%.4f%% matched-bytes=%llu/%llu "
        "G=%.6f gap-micro=%lld changed=%llu/%llu verdict=%s",
        id, digest, context, (unsigned long long)bytes, p, q, matched,
        (unsigned long long)covered, (unsigned long long)denominator,
        gap, (long long)gap_micro, (unsigned long long)changed,
        (unsigned long long)shore, verdict);
    int cn = snprintf(canonical, sizeof canonical, "%s receipt=%s",
                      base, receipt);
    if (bn < 0 || (size_t)bn >= sizeof base || cn < 0 ||
            (size_t)cn >= sizeof canonical || strcmp(line, canonical))
        return 0;

    char seen_match[32], want_match[32], seen_gap[32], want_gap[32];
    snprintf(seen_match, sizeof seen_match, "%.4f", matched);
    snprintf(want_match, sizeof want_match, "%.4f",
             100.0 * (double)covered / (double)denominator);
    snprintf(seen_gap, sizeof seen_gap, "%.6f", gap);
    snprintf(want_gap, sizeof want_gap, "%.6f",
             (double)gap_micro / 1000000.0);
    if (strcmp(seen_match, want_match) || strcmp(seen_gap, want_gap) ||
            fabs((q - p) - gap) > 0.000002)
        return 0;

    const char *want = (changed == 0 || gap_micro == 0) ? "abstain"
                     : (covered >= (denominator + 1) / 2) ? "replay"
                     : (gap_micro >= 500000) ? "order"
                                             : "stranger";
    if (strcmp(verdict, want)) return 0;

    char sealed[768], mine_hex[17];
    int sn = snprintf(sealed, sizeof sealed, "%s law-digest=%s",
                      base, law_hex);
    if (sn < 0 || (size_t)sn >= sizeof sealed) return 0;
    snprintf(mine_hex, sizeof mine_hex, "%016llx",
             (unsigned long long)fnv(sealed, (size_t)sn));
    return strcmp(receipt, mine_hex) == 0;
}

static int parse_close(const char *line, int want_count,
                       uint64_t want_docket) {
    int used = 0;
    char count_token[32] = {0}, docket[17] = {0}, want_hex[17];
    uint64_t count64 = 0;
    int got = sscanf(line,
                     "court close: verdicts=%31s docket=%16[0-9a-f]%n",
                     count_token, docket, &used);
    snprintf(want_hex, sizeof want_hex, "%016llx",
             (unsigned long long)want_docket);
    if (got != 2 || line[used] != 0 ||
            !token_u64(count_token, &count64) || count64 > INT_MAX ||
            (int)count64 != want_count ||
            !lower_hex(docket, 16) || strcmp(docket, want_hex)) return 0;
    char canonical[96];
    snprintf(canonical, sizeof canonical,
             "court close: verdicts=%d docket=%s", (int)count64, docket);
    return strcmp(line, canonical) == 0;
}

int main(void) {
    enum { WANT_BANNER, WANT_LAW, WANT_SITTING, WANT_COURTS } state =
        WANT_BANNER;
    char line[LINE_CAP], law_hex[17], expected_law[256];
    char candidate_hex[17], sitting_context[8];
    uint64_t sitting_bytes = 0, docket = 0;
    int sitting_shores = 0, seen = 0, lineno = 0;
    int courts = 0, sittings = 0;

    uint64_t law_digest = fnv(COURT_LAW, strlen(COURT_LAW));
    snprintf(law_hex, sizeof law_hex, "%016llx",
             (unsigned long long)law_digest);
    snprintf(expected_law, sizeof expected_law,
             "court law: %s law-digest=%s", COURT_LAW, law_hex);

    int rr;
    while ((rr = read_record(line, sizeof line, &lineno)) > 0) {
        if (state == WANT_BANNER) {
            if (strcmp(line, "NETTA ZERO")) {
                printf("warrant refused: line %d expected NETTA ZERO\n",
                       lineno);
                return 1;
            }
            state = WANT_LAW;
        } else if (state == WANT_LAW) {
            if (strcmp(line, expected_law)) {
                printf("warrant refused: line %d law is not canonical v2\n",
                       lineno);
                return 1;
            }
            state = WANT_SITTING;
        } else if (state == WANT_SITTING) {
            if (!parse_sitting(line, law_hex, candidate_hex, &sitting_bytes,
                               sitting_context, &sitting_shores)) {
                printf("warrant refused: line %d sitting malformed\n",
                       lineno);
                return 1;
            }
            docket = docket_record(fnv_seed(), line);
            seen = 0;
            state = WANT_COURTS;
        } else if (!strncmp(line, "court close: ", 13)) {
            if (seen != sitting_shores || !parse_close(line, seen, docket)) {
                printf("warrant refused: line %d close does not seal sitting\n",
                       lineno);
                return 1;
            }
            sittings++;
            state = WANT_BANNER;
        } else {
            if (seen >= sitting_shores ||
                    !parse_court(line, law_hex, seen, sitting_bytes,
                                 sitting_context)) {
                printf("warrant refused: line %d court record malformed\n",
                       lineno);
                return 1;
            }
            docket = docket_record(docket, line);
            seen++;
            courts++;
        }
    }
    if (rr == -1) {
        printf("warrant refused: input read failed\n");
        return 1;
    }
    if (rr == -2) {
        printf("warrant refused: line %d is overlong or not newline-sealed\n",
               lineno);
        return 1;
    }
    if (state != WANT_BANNER) {
        printf("warrant refused: premature EOF inside a sitting\n");
        return 1;
    }
    if (!sittings) {
        printf("warrant refused: no complete sittings\n");
        return 2;
    }
    printf("warrant accepted: %d verdicts under law %s in %d complete "
           "sittings\n", courts, law_hex, sittings);
    return 0;
}
