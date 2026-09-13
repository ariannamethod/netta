/* court4_select.c -- Court 4 pre-selection commitment and draw tool.
   It emits receipts only. It never constructs, prices, or judges a world. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define S_LINE 4096
#define S_PATH 2048
#define C4_RUN_BYTES 131072u

typedef struct {
    uint32_t h[8];
    uint64_t bits;
    uint8_t block[64];
    size_t used;
} Sha256;

typedef struct {
    char builder_sha[65];
    char verifier_sha[65];
    uint8_t *exclusions_raw;
    size_t exclusions_bytes;
    char receipt_sha[65];
    size_t receipt_bytes;
} Roots;

typedef struct {
    char base_sha[65];
    char receipt_sha[65];
    size_t base_bytes;
    size_t ff_bytes;
    size_t receipt_bytes;
} BaseCommit;

typedef struct {
    char roots_sha[65];
    char commit_sha[65];
    char receipt_sha[65];
    size_t roots_bytes;
    size_t commit_bytes;
    size_t receipt_bytes;
} BaseFreeze;

static void die(const char *m) {
    fprintf(stderr, "court4_select: %s\n", m);
    exit(1);
}

static uint32_t rotr(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32u - n));
}

static void sha_transform(Sha256 *s, const uint8_t block[64]) {
    static const uint32_t K[64] = {
        0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,
        0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
        0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,
        0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
        0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,
        0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
        0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,
        0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
        0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,
        0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
        0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,
        0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
        0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,
        0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
        0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,
        0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u
    };
    uint32_t w[64];
    for (int i = 0; i < 16; i++)
        w[i] = ((uint32_t)block[4 * i] << 24) |
               ((uint32_t)block[4 * i + 1] << 16) |
               ((uint32_t)block[4 * i + 2] << 8) |
               (uint32_t)block[4 * i + 3];
    for (int i = 16; i < 64; i++) {
        uint32_t a = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^
                     (w[i - 15] >> 3);
        uint32_t b = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^
                     (w[i - 2] >> 10);
        w[i] = w[i - 16] + a + w[i - 7] + b;
    }
    uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3];
    uint32_t e = s->h[4], f = s->h[5], g = s->h[6], h = s->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + s1 + ch + K[i] + w[i];
        uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d;
    s->h[4] += e; s->h[5] += f; s->h[6] += g; s->h[7] += h;
}

static void sha_init(Sha256 *s) {
    static const uint32_t H[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    memcpy(s->h, H, sizeof(H));
    s->bits = 0;
    s->used = 0;
}

static void sha_update(Sha256 *s, const void *data, size_t n) {
    const uint8_t *p = data;
    if (n > (UINT64_MAX - s->bits) / 8u) die("SHA-256 input too long");
    s->bits += (uint64_t)n * 8u;
    while (n) {
        size_t take = 64 - s->used;
        if (take > n) take = n;
        memcpy(s->block + s->used, p, take);
        s->used += take;
        p += take;
        n -= take;
        if (s->used == 64) {
            sha_transform(s, s->block);
            s->used = 0;
        }
    }
}

static void sha_final(Sha256 *s, uint8_t out[32]) {
    uint64_t bits = s->bits;
    s->block[s->used++] = 0x80;
    if (s->used > 56) {
        while (s->used < 64) s->block[s->used++] = 0;
        sha_transform(s, s->block);
        s->used = 0;
    }
    while (s->used < 56) s->block[s->used++] = 0;
    for (int i = 7; i >= 0; i--)
        s->block[s->used++] = (uint8_t)(bits >> (8 * i));
    sha_transform(s, s->block);
    for (int i = 0; i < 8; i++) {
        out[4 * i] = (uint8_t)(s->h[i] >> 24);
        out[4 * i + 1] = (uint8_t)(s->h[i] >> 16);
        out[4 * i + 2] = (uint8_t)(s->h[i] >> 8);
        out[4 * i + 3] = (uint8_t)s->h[i];
    }
}

static void sha_bytes(const void *data, size_t n, uint8_t out[32]) {
    Sha256 s;
    sha_init(&s);
    sha_update(&s, data, n);
    sha_final(&s, out);
}

static void hex_encode(const uint8_t *bytes, size_t n, char *out) {
    static const char h[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = h[bytes[i] >> 4];
        out[2 * i + 1] = h[bytes[i] & 15];
    }
    out[2 * n] = 0;
}

static int lower_hex64(const char *s) {
    if (!s || strlen(s) != 64) return 0;
    for (int i = 0; i < 64; i++)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f')))
            return 0;
    return 1;
}

static uint64_t be64(const uint8_t b[8]) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | b[i];
    return v;
}

static void put_be64(uint64_t v, uint8_t b[8]) {
    for (int i = 7; i >= 0; i--) {
        b[i] = (uint8_t)v;
        v >>= 8;
    }
}

static uint8_t *read_file(const char *path, size_t *n) {
    struct stat st;
    size_t off = 0;
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) die("cannot open regular input without following links");
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        (uintmax_t)st.st_size > SIZE_MAX - 1) {
        close(fd);
        die("input is not a sizeable regular file");
    }
    *n = (size_t)st.st_size;
    uint8_t *p = malloc(*n + 1);
    if (!p) { close(fd); die("cannot allocate input snapshot"); }
    while (off < *n) {
        ssize_t nr = read(fd, p + off, *n - off);
        if (nr < 0 && errno == EINTR) continue;
        if (nr <= 0) { free(p); close(fd); die("cannot capture complete input"); }
        off += (size_t)nr;
    }
    {
        uint8_t extra;
        ssize_t nr;
        do { nr = read(fd, &extra, 1); } while (nr < 0 && errno == EINTR);
        if (nr != 0) { free(p); close(fd); die("input grew while being captured"); }
    }
    if (close(fd) != 0) { free(p); die("cannot close captured input"); }
    p[*n] = 0;
    return p;
}

static uint8_t *read_text_file(const char *path, size_t *n) {
    uint8_t *p = read_file(path, n);
    if (!*n) { free(p); die("empty text input"); }
    if (memchr(p, 0, *n)) { free(p); die("embedded NUL in text input"); }
    if (p[*n - 1] != '\n') { free(p); die("text input is not LF-terminated"); }
    return p;
}

static char *next_line(uint8_t *raw, size_t n, size_t *pos) {
    if (*pos >= n) return NULL;
    char *line = (char *)raw + *pos;
    uint8_t *nl = memchr(raw + *pos, '\n', n - *pos);
    if (!nl) die("text input is not LF-terminated");
    if (memchr(raw + *pos, '\r', (size_t)(nl - raw - *pos)))
        die("CR byte in LF-only input");
    *nl = 0;
    *pos = (size_t)(nl - raw) + 1;
    return line;
}

static int ff_is_ws(uint8_t c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static uint64_t ff_hash(const uint8_t *s, uint32_t len) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t i = 0; i < len; i++)
        h = (h ^ s[i]) * 0x100000001b3ull;
    return h;
}

/* Independent length-only implementation of the frozen 16-word FF law. */
static size_t ff_transformed_length(const uint8_t *src, size_t n) {
    enum { WH = 1 << 16 };
    static struct {
        uint8_t s[64];
        uint32_t len;
        uint64_t cnt;
        size_t first;
        int used;
    } wh[WH];
    uint8_t words[16][64];
    uint32_t lens[16];
    int picked[16];
    memset(wh, 0, sizeof(wh));

    size_t i = 0;
    while (i < n) {
        if (ff_is_ws(src[i])) {
            i++;
            continue;
        }
        size_t j = i;
        while (j < n && !ff_is_ws(src[j])) j++;
        size_t span = j - i;
        if (span < 64) {
            uint32_t len = (uint32_t)span;
            uint32_t slot = (uint32_t)(ff_hash(src + i, len) >> 48);
            int found = 0;
            for (uint32_t probe = 0; probe < WH; probe++) {
                if (!wh[slot].used) {
                    memcpy(wh[slot].s, src + i, len);
                    wh[slot].len = len;
                    wh[slot].cnt = 1;
                    wh[slot].first = i;
                    wh[slot].used = 1;
                    found = 1;
                    break;
                }
                if (wh[slot].len == len &&
                    !memcmp(wh[slot].s, src + i, len)) {
                    wh[slot].cnt++;
                    found = 1;
                    break;
                }
                slot = (slot + 1) & (WH - 1);
            }
            if (!found)
                die("base is ineligible: false-friend vocabulary exceeds capacity");
        }
        i = j;
    }

    for (int k = 0; k < 16; k++) {
        int best = -1;
        for (int s = 0; s < WH; s++) {
            if (!wh[s].used) continue;
            int dup = 0;
            for (int q = 0; q < k; q++)
                if (picked[q] == s) dup = 1;
            if (dup) continue;
            if (best < 0 || wh[s].cnt > wh[best].cnt ||
                (wh[s].cnt == wh[best].cnt &&
                 wh[s].first < wh[best].first))
                best = s;
        }
        if (best < 0)
            die("base is ineligible: false-friend law has fewer than 16 words");
        picked[k] = best;
        memcpy(words[k], wh[best].s, wh[best].len);
        lens[k] = wh[best].len;
    }

    size_t out = 0;
    i = 0;
    while (i < n) {
        size_t add;
        if (ff_is_ws(src[i])) {
            add = 1;
            i++;
        } else {
            size_t j = i;
            while (j < n && !ff_is_ws(src[j])) j++;
            size_t span = j - i;
            int hit = -1;
            if (span < 64) for (int k = 0; k < 16; k++)
                if (lens[k] == span && !memcmp(words[k], src + i, span)) {
                    hit = k;
                    break;
                }
            add = hit < 0 ? span : lens[(hit % 2 == 0) ? hit + 1 : hit - 1];
            i = j;
        }
        if (add > SIZE_MAX - out)
            die("base is ineligible: false-friend output length overflow");
        out += add;
    }
    return out;
}

static size_t ff_capacity_ceiling(size_t base_bytes) {
    if (base_bytes > (SIZE_MAX - 16) / 2)
        die("base is ineligible: false-friend capacity formula overflow");
    return base_bytes * 2 + 16;
}

static uint8_t *read_eligible_base(const char *path, size_t *base_bytes,
                                   size_t *ff_bytes, char base_sha[65]) {
    uint8_t *base = read_file(path, base_bytes);
    if (*base_bytes < C4_RUN_BYTES)
        die("base is ineligible: raw length is below RUN_BYTES");
    *ff_bytes = ff_transformed_length(base, *base_bytes);
    if (*ff_bytes < C4_RUN_BYTES)
        die("base is ineligible: false-friend length is below RUN_BYTES");
    if (*ff_bytes > ff_capacity_ceiling(*base_bytes))
        die("base is ineligible: false-friend length exceeds capacity ceiling");
    uint8_t digest[32];
    sha_bytes(base, *base_bytes, digest);
    hex_encode(digest, 32, base_sha);
    return base;
}

static size_t parse_size(const char *s) {
    if (!s || !*s) die("empty decimal receipt field");
    size_t v = 0;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p < '0' || *p > '9') die("non-decimal receipt field");
        size_t d = (size_t)(*p - '0');
        if (v > (SIZE_MAX - d) / 10) die("decimal receipt field overflow");
        v = v * 10 + d;
    }
    char canon[64];
    snprintf(canon, sizeof(canon), "%zu", v);
    if (strcmp(canon, s)) die("non-canonical decimal receipt field");
    return v;
}

static int parent_segment(const char *path) {
    const char *p = path;
    while (*p) {
        const char *q = strchr(p, '/');
        size_t n = q ? (size_t)(q - p) : strlen(p);
        if (n == 2 && p[0] == '.' && p[1] == '.') return 1;
        if (!q) break;
        p = q + 1;
    }
    return 0;
}

static void resolve_path(const char *receipt, const char *relative,
                         char out[S_PATH]) {
    if (!*relative || relative[0] == '/' || parent_segment(relative))
        die("unsafe path in roots receipt");
    const char *slash = strrchr(receipt, '/');
    int rc;
    if (slash) {
        size_t dirn = (size_t)(slash - receipt);
        rc = snprintf(out, S_PATH, "%.*s/%s", (int)dirn, receipt, relative);
    } else {
        rc = snprintf(out, S_PATH, "%s", relative);
    }
    if (rc < 0 || rc >= S_PATH) die("resolved receipt path too long");
}

static void verify_prior_binding(uint8_t *raw, size_t bytes,
                                 const size_t root_bytes[18],
                                 const char root_sha[18][65]) {
    static const char *stages[7] = {
        "0-prior-receipt", "1-court2-snapshot", "1-court3-snapshot",
        "1-court4-protocol", "2-builder", "3-gap-addendum", "4-verifier"
    };
    static const char *paths[7] = {
        "COURT4_ADDENDUM_FREEZE.tsv", "COURT2_SNAPSHOT.md",
        "COURT3_SNAPSHOT.md", "TRANSFER4_DRAFT.md", "transfer4.c",
        "COURT4_GAP_ADDENDUM.md", "transfer4_check.c"
    };
    size_t pos = 0, row = 0;
    char *line = next_line(raw, bytes, &pos);
    if (!line || strcmp(line, "stage\tpath\tbytes\tsha256"))
        die("prior freeze receipt header mismatch");
    while ((line = next_line(raw, bytes, &pos)) != NULL) {
        char *stage = line, *path, *size_text, *sha;
        char *tab = strchr(stage, '\t');
        if (!tab) die("malformed prior freeze row");
        *tab++ = 0; path = tab;
        tab = strchr(path, '\t');
        if (!tab) die("malformed prior freeze row");
        *tab++ = 0; size_text = tab;
        tab = strchr(size_text, '\t');
        if (!tab || strchr(tab + 1, '\t')) die("malformed prior freeze row");
        *tab++ = 0; sha = tab;
        if (row >= 7 || strcmp(stage, stages[row]) || strcmp(path, paths[row]) ||
            !lower_hex64(sha))
            die("prior freeze row identity mismatch");
        size_t pinned_bytes = parse_size(size_text);
        if (row > 0 &&
            (pinned_bytes != root_bytes[row] || strcmp(sha, root_sha[row])))
            die("development root contradicts prior freeze receipt");
        row++;
    }
    if (row != 7) die("prior freeze receipt row count mismatch");
}

static void verify_roots(const char *path, Roots *roots) {
    static const char *roles[] = {
        "prior_freeze", "court2_snapshot", "court3_snapshot",
        "court4_protocol", "development_builder", "gap_addendum",
        "development_verifier", "execution_law",
        "execution_law_receipt", "repair_addendum",
        "confirmatory_exclusions",
        "confirmatory_builder", "confirmatory_builder_core",
        "confirmatory_verifier",
        "confirmatory_selector",
        "builder_regression_manifest", "verifier_regression_manifest",
        "selector_test_manifest"
    };
    size_t captured_bytes[18] = {0};
    char captured_sha[18][65] = {{0}};
    uint8_t *prior_raw = NULL;
    size_t prior_bytes = 0;
    memset(roots, 0, sizeof(*roots));
    uint8_t *raw = read_text_file(path, &roots->receipt_bytes);
    uint8_t digest[32]; size_t pos = 0, row = 0;
    sha_bytes(raw, roots->receipt_bytes, digest);
    hex_encode(digest, 32, roots->receipt_sha);
    char *line = next_line(raw, roots->receipt_bytes, &pos);
    if (!line || strcmp(line, "role\tpath\tbytes\tsha256"))
        die("invalid roots receipt header");
    while ((line = next_line(raw, roots->receipt_bytes, &pos)) != NULL) {
        char *role = line;
        char *p1 = strchr(role, '\t');
        if (!p1) die("malformed roots receipt row");
        *p1++ = 0;
        char *p2 = strchr(p1, '\t');
        if (!p2) die("malformed roots receipt row");
        *p2++ = 0;
        char *p3 = strchr(p2, '\t');
        if (!p3 || strchr(p3 + 1, '\t')) die("malformed roots receipt row");
        *p3++ = 0;
        if (!*role || !*p1 || !lower_hex64(p3)) die("invalid roots value");
        if (row >= sizeof(roles) / sizeof(roles[0]) ||
            strcmp(role, roles[row]))
            die("missing, duplicate, unknown, or out-of-order roots role");
        size_t want_bytes = parse_size(p2);
        char resolved[S_PATH], got_sha[65];
        size_t got_bytes; uint8_t *got;
        resolve_path(path, p1, resolved);
        got = read_file(resolved, &got_bytes);
        sha_bytes(got, got_bytes, digest); hex_encode(digest, 32, got_sha);
        if (got_bytes != want_bytes || strcmp(got_sha, p3))
            die("roots receipt file mismatch");
        captured_bytes[row] = got_bytes;
        memcpy(captured_sha[row], p3, 65);
        if (row == 0) {
            if (got_bytes != 761 ||
                strcmp(p3, "a054b759a13fab35181e9cd9e4f5fa5abad7fd9263419ef2cd9296a99fd9f250"))
                die("prior freeze root is not the frozen parent receipt");
            prior_raw = got;
            prior_bytes = got_bytes;
        } else if (row == 7) {
            if (got_bytes != 26929 ||
                strcmp(p3, "f823478054705091300b6a5f0ce4a0b43af50f019cea32dc27e8aa10889ba908"))
                die("execution law root is not frozen");
            free(got);
        } else if (row == 8) {
            if (got_bytes != 869 ||
                strcmp(p3, "7bcdc748e2cfe9a60112ef36b8718f504620f2f16682f32da0e0bd553c077501"))
                die("execution law receipt root is not frozen");
            free(got);
        } else if (row == 10) {
            if (!got_bytes || memchr(got, 0, got_bytes) || got[got_bytes - 1] != '\n')
                die("confirmatory exclusions are not strict LF text");
            roots->exclusions_raw = got;
            roots->exclusions_bytes = got_bytes;
        } else if (row == 11) {
            memcpy(roots->builder_sha, p3, 65);
            free(got);
        } else if (row == 13) {
            memcpy(roots->verifier_sha, p3, 65);
            free(got);
        } else if (row != 0) free(got);
        row++;
    }
    free(raw);
    if (row != sizeof(roles) / sizeof(roles[0]))
        die("missing roots receipt role");
    if (!prior_raw) die("missing captured prior freeze receipt");
    verify_prior_binding(prior_raw, prior_bytes, captured_bytes, captured_sha);
    free(prior_raw);
}

static void check_exclusions(Roots *roots, size_t base_bytes,
                             const char *base_sha) {
    size_t pos = 0;
    char *line = next_line(roots->exclusions_raw, roots->exclusions_bytes, &pos);
    if (!line || strcmp(line, "label\tbytes\tsha256"))
        die("invalid confirmatory exclusions header");
    size_t rows = 0;
    while ((line = next_line(roots->exclusions_raw, roots->exclusions_bytes, &pos)) != NULL) {
        char *label = line;
        char *p1 = strchr(label, '\t');
        if (!p1) die("malformed confirmatory exclusions row");
        *p1++ = 0;
        char *p2 = strchr(p1, '\t');
        if (!p2 || strchr(p2 + 1, '\t'))
            die("malformed confirmatory exclusions row");
        *p2++ = 0;
        if (!*label || !lower_hex64(p2))
            die("invalid confirmatory exclusions value");
        size_t excluded_bytes = parse_size(p1);
        if (base_bytes == excluded_bytes && !strcmp(base_sha, p2))
            die("base is ineligible: exact copy of development material");
        rows++;
    }
    if (!rows) die("empty confirmatory exclusions");
}

static void read_field(uint8_t *raw, size_t n, size_t *pos,
                       const char *want, char *value, size_t cap) {
    char *line = next_line(raw, n, pos);
    if (!line) die("missing receipt field");
    char *tab = strchr(line, '\t');
    if (!tab || strchr(tab + 1, '\t')) die("malformed two-column row");
    *tab = 0;
    if (strcmp(line, want)) die("unexpected receipt field order");
    if (strlen(tab + 1) >= cap) die("receipt field too long");
    memcpy(value, tab + 1, strlen(tab + 1) + 1);
}

static void read_base_commit(const char *path, BaseCommit *commit) {
    memset(commit, 0, sizeof(*commit));
    uint8_t *raw = read_text_file(path, &commit->receipt_bytes), digest[32];
    size_t pos = 0;
    sha_bytes(raw, commit->receipt_bytes, digest);
    hex_encode(digest, 32, commit->receipt_sha);
    char value[S_LINE];
    char *header = next_line(raw, commit->receipt_bytes, &pos);
    if (!header || strcmp(header, "field\tvalue"))
        die("invalid base commitment header");
    read_field(raw, commit->receipt_bytes, &pos, "status", value, sizeof(value));
    if (strcmp(value, "base-committed-class-not-computed"))
        die("invalid base commitment status");
    read_field(raw, commit->receipt_bytes, &pos, "base_text_bytes", value, sizeof(value));
    commit->base_bytes = parse_size(value);
    read_field(raw, commit->receipt_bytes, &pos, "ff_transformed_bytes", value, sizeof(value));
    commit->ff_bytes = parse_size(value);
    read_field(raw, commit->receipt_bytes, &pos, "base_text_sha256", commit->base_sha,
               sizeof(commit->base_sha));
    if (!lower_hex64(commit->base_sha)) die("invalid committed base hash");
    if (next_line(raw, commit->receipt_bytes, &pos) != NULL) die("extra base commitment row");
    free(raw);
}

static void read_base_freeze(const char *path, BaseFreeze *freeze) {
    memset(freeze, 0, sizeof(*freeze));
    uint8_t *raw = read_text_file(path, &freeze->receipt_bytes), digest[32];
    size_t pos = 0;
    sha_bytes(raw, freeze->receipt_bytes, digest);
    hex_encode(digest, 32, freeze->receipt_sha);
    char value[S_LINE];
    char *header = next_line(raw, freeze->receipt_bytes, &pos);
    if (!header || strcmp(header, "field\tvalue"))
        die("invalid base freeze header");
    read_field(raw, freeze->receipt_bytes, &pos, "status", value, sizeof(value));
    if (strcmp(value, "base-commit-frozen-class-not-computed"))
        die("invalid base freeze status");
    read_field(raw, freeze->receipt_bytes, &pos, "roots_receipt_bytes", value, sizeof(value));
    freeze->roots_bytes = parse_size(value);
    read_field(raw, freeze->receipt_bytes, &pos, "roots_receipt_sha256", freeze->roots_sha,
               sizeof(freeze->roots_sha));
    read_field(raw, freeze->receipt_bytes, &pos, "base_commit_bytes", value, sizeof(value));
    freeze->commit_bytes = parse_size(value);
    read_field(raw, freeze->receipt_bytes, &pos, "base_commit_sha256", freeze->commit_sha,
               sizeof(freeze->commit_sha));
    if (!lower_hex64(freeze->roots_sha) ||
        !lower_hex64(freeze->commit_sha))
        die("invalid base freeze hash");
    if (next_line(raw, freeze->receipt_bytes, &pos) != NULL) die("extra base freeze row");
    free(raw);
}

static void check_base_freeze(const Roots *roots, const BaseCommit *commit,
                              const BaseFreeze *freeze) {
    if (freeze->roots_bytes != roots->receipt_bytes ||
        strcmp(freeze->roots_sha, roots->receipt_sha) ||
        freeze->commit_bytes != commit->receipt_bytes ||
        strcmp(freeze->commit_sha, commit->receipt_sha))
        die("base freeze receipt disagrees with frozen inputs");
}

static void derive_selection(const char *builder_sha, const char *verifier_sha,
                             const char *base_sha, int *class_index,
                             uint64_t *world_seed, char class_hex[65],
                             char seed_hex[65]) {
    static const char class_tag[] = "NETTA-C4-CLASS-v2\n";
    static const char seed_tag[] = "NETTA-C4-SEED-v2\n";
    uint8_t d[32];
    Sha256 s;
    sha_init(&s);
    sha_update(&s, class_tag, sizeof(class_tag) - 1);
    sha_update(&s, builder_sha, 64);
    sha_update(&s, verifier_sha, 64);
    sha_update(&s, base_sha, 64);
    sha_final(&s, d);
    hex_encode(d, 32, class_hex);
    *class_index = d[31] & 3;
    sha_init(&s);
    sha_update(&s, seed_tag, sizeof(seed_tag) - 1);
    sha_update(&s, builder_sha, 64);
    sha_update(&s, verifier_sha, 64);
    sha_update(&s, base_sha, 64);
    sha_final(&s, d);
    hex_encode(d, 32, seed_hex);
    *world_seed = be64(d);
}

static uint64_t half_tail_seed(uint64_t world_seed) {
    static const char tag[] = "NETTA-C4-HALF-TAIL-v1";
    uint8_t seed[8], d[32];
    Sha256 s;
    put_be64(world_seed, seed);
    sha_init(&s);
    sha_update(&s, tag, sizeof(tag) - 1);
    sha_update(&s, seed, sizeof(seed));
    sha_final(&s, d);
    return be64(d);
}

static const char *class_name(int i) {
    static const char *names[4] = {"cipher", "plain", "half", "ff"};
    return i >= 0 && i < 4 ? names[i] : "invalid";
}

static void self_test(void) {
    static const char empty_want[] =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    static const char abc_want[] =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    static const char builder_sha[] =
        "0000000000000000000000000000000000000000000000000000000000000000";
    static const char verifier_sha[] =
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    static const char base_want[] =
        "f318964f23519b415bd3ef68ad9c03843a8af33e2d95f152a8a9797ef373d73f";
    static const char class_want[] =
        "b0c142688cb55362d4952ef0bc42904537ce7ceac7d1b52df3e20cf4342c6be1";
    static const char seed_want[] =
        "f476f3a4bcc1d16f666acb811ef8c732e9c6a9574461a448d780cff225afb4de";
    static const char base_text[] = "NETTA-C4-SELECTION-TEST-v1";
    uint8_t d[32];
    char hex[65], base_hex[65], class_hex[65], seed_hex[65];
    sha_bytes("", 0, d); hex_encode(d, 32, hex);
    if (strcmp(hex, empty_want)) die("SHA-256 empty self-test failed");
    sha_bytes("abc", 3, d); hex_encode(d, 32, hex);
    if (strcmp(hex, abc_want)) die("SHA-256 abc self-test failed");
    sha_bytes(base_text, sizeof(base_text) - 1, d);
    hex_encode(d, 32, base_hex);
    int ci;
    uint64_t seed;
    derive_selection(builder_sha, verifier_sha, base_hex, &ci, &seed,
                     class_hex, seed_hex);
    if (strcmp(base_hex, base_want) || strcmp(class_hex, class_want) ||
        ci != 1 || strcmp(seed_hex, seed_want) ||
        seed != 0xf476f3a4bcc1d16full ||
        half_tail_seed(seed) != 0x4372270db242db11ull)
        die("selection self-test failed");

    /* Capacity KAT: the most frequent one-byte word is paired with a rare
     * 63-byte word. The transform is valid but deliberately exceeds the
     * frozen 2*base+16 ceiling, so both commit and draw must refuse it. */
    {
        static const char *tail[14] = {
            "b","c","d","e","f","g","h","i","j","k","l","m","n","o"
        };
        uint8_t *fixture = (uint8_t *)malloc(400000); size_t n = 0, ff;
        char longword[64];
        if (!fixture) die("capacity self-test allocation failed");
        memset(longword, 'x', 63); longword[63] = 0;
#define APPEND_WORD(w_) do { size_t z_ = strlen(w_); memcpy(fixture + n, (w_), z_); n += z_; fixture[n++] = ' '; } while (0)
        for (int r = 0; r < 150000; r++) APPEND_WORD("a");
        for (int r = 0; r < 15; r++) APPEND_WORD(longword);
        for (int k = 0; k < 14; k++)
            for (int r = 0; r < 14 - k; r++) APPEND_WORD(tail[k]);
#undef APPEND_WORD
        ff = ff_transformed_length(fixture, n);
        if (ff <= ff_capacity_ceiling(n))
            die("false-friend capacity self-test failed to cross the ceiling");
        free(fixture);
    }
}

static FILE *open_new(const char *path) {
    FILE *f = fopen(path, "wx");
    if (!f) die("output exists or cannot be created");
    return f;
}

static int commit_base(const char *base_path, const char *roots_path,
                       const char *out_path) {
    Roots roots;
    verify_roots(roots_path, &roots);
    char base_sha[65];
    size_t base_bytes, ff_bytes;
    uint8_t *base = read_eligible_base(base_path, &base_bytes, &ff_bytes,
                                       base_sha);
    free(base);
    check_exclusions(&roots, base_bytes, base_sha);
    free(roots.exclusions_raw);
    FILE *f = open_new(out_path);
    fprintf(f, "field\tvalue\n");
    fprintf(f, "status\tbase-committed-class-not-computed\n");
    fprintf(f, "base_text_bytes\t%zu\n", base_bytes);
    fprintf(f, "ff_transformed_bytes\t%zu\n", ff_bytes);
    fprintf(f, "base_text_sha256\t%s\n", base_sha);
    if (fclose(f) != 0) die("cannot close base commitment");
    fprintf(stderr,
            "base committed: raw %zu B | ff %zu B | %s | class not computed\n",
            base_bytes, ff_bytes, base_sha);
    return 0;
}

static int freeze_base(const char *roots_path, const char *commit_path,
                       const char *out_path) {
    Roots roots;
    BaseCommit commit;
    verify_roots(roots_path, &roots);
    read_base_commit(commit_path, &commit);
    FILE *f = open_new(out_path);
    fprintf(f, "field\tvalue\n");
    fprintf(f, "status\tbase-commit-frozen-class-not-computed\n");
    fprintf(f, "roots_receipt_bytes\t%zu\n", roots.receipt_bytes);
    fprintf(f, "roots_receipt_sha256\t%s\n", roots.receipt_sha);
    fprintf(f, "base_commit_bytes\t%zu\n", commit.receipt_bytes);
    fprintf(f, "base_commit_sha256\t%s\n", commit.receipt_sha);
    if (fclose(f) != 0) die("cannot close base freeze receipt");
    free(roots.exclusions_raw);
    fprintf(stderr, "base commitment frozen | class not computed\n");
    return 0;
}

static int draw(const char *status, const char *base_path,
                const char *commit_path, const char *freeze_path,
                const char *roots_path, const char *out_path) {
    Roots roots;
    BaseCommit commit;
    BaseFreeze freeze;
    verify_roots(roots_path, &roots);
    read_base_commit(commit_path, &commit);
    read_base_freeze(freeze_path, &freeze);
    check_base_freeze(&roots, &commit, &freeze);
    char base_sha[65];
    size_t base_bytes, ff_bytes;
    uint8_t *base = read_eligible_base(base_path, &base_bytes, &ff_bytes,
                                       base_sha);
    free(base);
    check_exclusions(&roots, base_bytes, base_sha);
    free(roots.exclusions_raw);
    if (base_bytes != commit.base_bytes || ff_bytes != commit.ff_bytes ||
        strcmp(base_sha, commit.base_sha))
        die("base differs from frozen commitment");
    int ci;
    uint64_t seed;
    char class_hex[65], seed_hex[65];
    derive_selection(roots.builder_sha, roots.verifier_sha, base_sha,
                     &ci, &seed, class_hex, seed_hex);
    FILE *f = open_new(out_path);
    fprintf(f, "field\tvalue\n");
    fprintf(f, "status\t%s\n", status);
    fprintf(f, "roots_receipt_bytes\t%zu\n", roots.receipt_bytes);
    fprintf(f, "roots_receipt_sha256\t%s\n", roots.receipt_sha);
    fprintf(f, "base_commit_bytes\t%zu\n", commit.receipt_bytes);
    fprintf(f, "base_commit_sha256\t%s\n", commit.receipt_sha);
    fprintf(f, "base_freeze_bytes\t%zu\n", freeze.receipt_bytes);
    fprintf(f, "base_freeze_sha256\t%s\n", freeze.receipt_sha);
    fprintf(f, "builder_source_sha256\t%s\n", roots.builder_sha);
    fprintf(f, "verifier_source_sha256\t%s\n", roots.verifier_sha);
    fprintf(f, "base_text_bytes\t%zu\n", base_bytes);
    fprintf(f, "base_text_sha256\t%s\n", base_sha);
    fprintf(f, "class_digest_sha256\t%s\n", class_hex);
    fprintf(f, "class_index\t%d\n", ci);
    fprintf(f, "class\t%s\n", class_name(ci));
    fprintf(f, "seed_digest_sha256\t%s\n", seed_hex);
    fprintf(f, "world_seed_be64\t%016llx\n", (unsigned long long)seed);
    fprintf(f, "half_tail_seed_be64\t%016llx\n",
            (unsigned long long)half_tail_seed(seed));
    if (fclose(f) != 0) die("cannot close selection receipt");
    fprintf(stderr, "selection written: class %d %s | seed %016llx | no world run\n",
            ci, class_name(ci), (unsigned long long)seed);
    return 0;
}

static void usage(void) {
    fprintf(stderr,
        "usage:\n"
        "  court4_select --self-test\n"
        "  court4_select --commit-base BASE_TEXT --roots ROOTS.tsv "
        "--out BASE_COMMIT.tsv\n"
        "  court4_select --freeze-base --base-commit BASE_COMMIT.tsv "
        "--roots ROOTS.tsv --out BASE_FREEZE.tsv\n"
        "  court4_select --draw BASE_TEXT --base-commit BASE_COMMIT.tsv "
        "--base-freeze BASE_FREEZE.tsv --roots ROOTS.tsv "
        "--out SELECTION.tsv\n"
        "  court4_select --synthetic-draw BASE_TEXT "
        "--base-commit BASE_COMMIT.tsv --base-freeze BASE_FREEZE.tsv "
        "--roots ROOTS.tsv "
        "--out SELECTION.tsv\n");
    exit(1);
}

int main(int argc, char **argv) {
    self_test();
    if (argc == 2 && !strcmp(argv[1], "--self-test")) {
        fprintf(stderr, "court4_select: SHA/C2/C3 and FF capacity self-tests passed\n");
        return 0;
    }
    if (argc == 7 && !strcmp(argv[1], "--commit-base") &&
        !strcmp(argv[3], "--roots") && !strcmp(argv[5], "--out"))
        return commit_base(argv[2], argv[4], argv[6]);
    if (argc == 8 && !strcmp(argv[1], "--freeze-base") &&
        !strcmp(argv[2], "--base-commit") &&
        !strcmp(argv[4], "--roots") && !strcmp(argv[6], "--out"))
        return freeze_base(argv[5], argv[3], argv[7]);
    if (argc == 11 &&
        (!strcmp(argv[1], "--draw") ||
         !strcmp(argv[1], "--synthetic-draw")) &&
        !strcmp(argv[3], "--base-commit") &&
        !strcmp(argv[5], "--base-freeze") &&
        !strcmp(argv[7], "--roots") && !strcmp(argv[9], "--out")) {
        const char *status = !strcmp(argv[1], "--draw")
            ? "selected-not-run" : "synthetic-selected-not-confirmatory";
        return draw(status, argv[2], argv[4], argv[6], argv[8], argv[10]);
    }
    usage();
    return 1;
}
