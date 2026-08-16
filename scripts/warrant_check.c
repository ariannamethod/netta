/* The warrant's external hand. Reads a pattern-court transcript on
   stdin, re-derives every verdict from the printed operands alone,
   recomputes every receipt against the law-digest the transcript
   names, and refuses the transcript on any divergence, naming the
   line. It shares no code with the organism: an independent FNV-1a-64
   over public constants. rc 0 accepts; rc 1 refuses; rc 2 is misuse.
   Honest limit: FNV is a witness, not a signature -- this hand catches
   transcription, tampering, and law drift, not an adversary who
   rebuilds the hash from source. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

static uint64_t fnv(const char *p, size_t n) {
    uint64_t h = 0xcbf29ce484222325ULL;
    for (size_t i = 0; i < n; ++i) {
        h ^= (uint8_t)p[i];
        h *= 0x100000001b3ULL;
    }
    return h;
}

static int field_i64(const char *line, const char *key, int64_t *out) {
    const char *p = strstr(line, key);
    if (!p) return 0;
    *out = strtoll(p + strlen(key), NULL, 10);
    return 1;
}

static int field_frac(const char *line, const char *key,
                      uint64_t *a, uint64_t *b) {
    const char *p = strstr(line, key);
    if (!p) return 0;
    char *end;
    *a = strtoull(p + strlen(key), &end, 10);
    if (*end != '/') return 0;
    *b = strtoull(end + 1, NULL, 10);
    return 1;
}

int main(void) {
    char line[1024];
    char law_digest_hex[17] = {0};
    int have_law = 0, lineno = 0, courts = 0;
    while (fgets(line, sizeof line, stdin)) {
        lineno++;
        size_t len = strlen(line);
        while (len && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[--len] = 0;
        if (!strncmp(line, "court law: ", 11)) {
            const char *d = strstr(line, " law-digest=");
            if (!d || strlen(d + 12) != 16) {
                printf("warrant refused: line %d law header malformed\n",
                       lineno);
                return 1;
            }
            uint64_t mine = fnv(line + 11, (size_t)(d - line - 11));
            char mine_hex[17];
            snprintf(mine_hex, sizeof mine_hex, "%016llx",
                     (unsigned long long)mine);
            if (strcmp(mine_hex, d + 12) != 0) {
                printf("warrant refused: line %d law text does not "
                       "match its digest\n", lineno);
                return 1;
            }
            memcpy(law_digest_hex, d + 12, 17);
            have_law = 1;
            continue;
        }
        if (strncmp(line, "court ", 6) != 0) continue;
        if (!have_law) {
            printf("warrant refused: line %d verdict before any law\n",
                   lineno);
            return 1;
        }
        char *rp = strstr(line, " receipt=");
        if (!rp || strlen(rp + 9) != 16) {
            printf("warrant refused: line %d has no receipt\n", lineno);
            return 1;
        }
        char sealed[1200];
        int sn = snprintf(sealed, sizeof sealed, "%.*s law-digest=%s",
                          (int)(rp - line), line, law_digest_hex);
        if (sn < 0 || (size_t)sn >= sizeof sealed) {
            printf("warrant refused: line %d overflows the seal\n",
                   lineno);
            return 1;
        }
        char mine_hex[17];
        snprintf(mine_hex, sizeof mine_hex, "%016llx",
                 (unsigned long long)fnv(sealed, (size_t)sn));
        if (strcmp(mine_hex, rp + 9) != 0) {
            printf("warrant refused: line %d receipt does not match "
                   "its operands\n", lineno);
            return 1;
        }
        uint64_t cov, n, ch, shore;
        int64_t gm;
        const char *vp = strstr(line, " verdict=");
        if (!field_frac(line, "matched-bytes=", &cov, &n) ||
                !field_i64(line, "gap-micro=", &gm) ||
                !field_frac(line, "changed=", &ch, &shore) || !vp) {
            printf("warrant refused: line %d operands missing\n",
                   lineno);
            return 1;
        }
        const char *mine = (ch == 0 || gm == 0) ? "abstain"
                         : (2 * cov >= n)       ? "replay"
                         : (gm >= 500000)       ? "order"
                                                : "stranger";
        size_t vl = strlen(mine);
        if (strncmp(vp + 9, mine, vl) != 0 ||
                (vp[9 + vl] != ' ' && vp[9 + vl] != 0)) {
            printf("warrant refused: line %d verdict does not follow "
                   "from its operands (expected %s)\n", lineno, mine);
            return 1;
        }
        courts++;
    }
    if (!courts) {
        printf("warrant refused: no court lines\n");
        return 2;
    }
    printf("warrant accepted: %d verdicts under law %s\n",
           courts, law_digest_hex);
    return 0;
}
