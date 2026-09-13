/* transfer4_confirm.c -- pre-selection Court 4 single-world executor.
   This file wraps the frozen development builder without editing it.
   It emits raw artifacts and grades nothing. */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define main transfer4_development_main
#include "transfer4.c"
#undef main

void transfer4_confirm_core_run(const uint8_t *source, size_t source_bytes,
                                const uint8_t *world, size_t world_bytes,
                                const uint8_t oracle[256], int output_dir_fd,
                                double G[22][5]);

typedef struct {
    uint32_t h[8];
    uint64_t bits;
    uint8_t block[64];
    size_t used;
} C4Sha256;

static const char C4_SOURCE_SHA[65] =
    "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb";
#define C4_SOURCE_BYTES 447545u

static uint32_t c4_rotr(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32u - n));
}

static void c4_sha_transform(C4Sha256 *s, const uint8_t block[64]) {
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
        uint32_t a = c4_rotr(w[i - 15], 7) ^ c4_rotr(w[i - 15], 18) ^
                     (w[i - 15] >> 3);
        uint32_t b = c4_rotr(w[i - 2], 17) ^ c4_rotr(w[i - 2], 19) ^
                     (w[i - 2] >> 10);
        w[i] = w[i - 16] + a + w[i - 7] + b;
    }
    uint32_t a = s->h[0], b = s->h[1], c = s->h[2], d = s->h[3];
    uint32_t e = s->h[4], f = s->h[5], g = s->h[6], h = s->h[7];
    for (int i = 0; i < 64; i++) {
        uint32_t s1 = c4_rotr(e, 6) ^ c4_rotr(e, 11) ^ c4_rotr(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t t1 = h + s1 + ch + K[i] + w[i];
        uint32_t s0 = c4_rotr(a, 2) ^ c4_rotr(a, 13) ^ c4_rotr(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    s->h[0] += a; s->h[1] += b; s->h[2] += c; s->h[3] += d;
    s->h[4] += e; s->h[5] += f; s->h[6] += g; s->h[7] += h;
}

static void c4_sha_init(C4Sha256 *s) {
    static const uint32_t H[8] = {
        0x6a09e667u,0xbb67ae85u,0x3c6ef372u,0xa54ff53au,
        0x510e527fu,0x9b05688cu,0x1f83d9abu,0x5be0cd19u
    };
    memcpy(s->h, H, sizeof(H));
    s->bits = 0;
    s->used = 0;
}

static void c4_sha_update(C4Sha256 *s, const void *data, size_t n) {
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
            c4_sha_transform(s, s->block);
            s->used = 0;
        }
    }
}

static void c4_sha_final(C4Sha256 *s, uint8_t out[32]) {
    uint64_t bits = s->bits;
    s->block[s->used++] = 0x80;
    if (s->used > 56) {
        while (s->used < 64) s->block[s->used++] = 0;
        c4_sha_transform(s, s->block);
        s->used = 0;
    }
    while (s->used < 56) s->block[s->used++] = 0;
    for (int i = 7; i >= 0; i--)
        s->block[s->used++] = (uint8_t)(bits >> (8 * i));
    c4_sha_transform(s, s->block);
    for (int i = 0; i < 8; i++) {
        out[4 * i] = (uint8_t)(s->h[i] >> 24);
        out[4 * i + 1] = (uint8_t)(s->h[i] >> 16);
        out[4 * i + 2] = (uint8_t)(s->h[i] >> 8);
        out[4 * i + 3] = (uint8_t)s->h[i];
    }
}

static void c4_sha_bytes(const void *data, size_t n, uint8_t out[32]) {
    C4Sha256 s;
    c4_sha_init(&s);
    c4_sha_update(&s, data, n);
    c4_sha_final(&s, out);
}

static void c4_hex(const uint8_t *bytes, size_t n, char *out) {
    static const char h[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        out[2 * i] = h[bytes[i] >> 4];
        out[2 * i + 1] = h[bytes[i] & 15];
    }
    out[2 * n] = 0;
}

static int c4_lower_hex64(const char *s) {
    if (!s || strlen(s) != 64) return 0;
    for (int i = 0; i < 64; i++)
        if (!((s[i] >= '0' && s[i] <= '9') ||
              (s[i] >= 'a' && s[i] <= 'f')))
            return 0;
    return 1;
}

static uint64_t c4_be64(const uint8_t b[8]) {
    uint64_t v = 0;
    for (int i = 0; i < 8; i++) v = (v << 8) | b[i];
    return v;
}

static void c4_put_be64(uint64_t v, uint8_t b[8]) {
    for (int i = 7; i >= 0; i--) {
        b[i] = (uint8_t)v;
        v >>= 8;
    }
}

static void c4_sha_self_test(void) {
    static const char empty_want[] =
        "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855";
    static const char abc_want[] =
        "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    uint8_t d[32];
    char hex[65];
    c4_sha_bytes("", 0, d); c4_hex(d, 32, hex);
    if (strcmp(hex, empty_want)) die("SHA-256 empty-string self-test failed");
    c4_sha_bytes("abc", 3, d); c4_hex(d, 32, hex);
    if (strcmp(hex, abc_want)) die("SHA-256 abc self-test failed");
}

static uint8_t *c4_read_file(const char *path, size_t *n) {
    struct stat st;
    uint8_t *p;
    size_t off = 0;
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) die("cannot open regular input without following links");
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        (uintmax_t)st.st_size > SIZE_MAX - 1) {
        close(fd); die("input is not a sizeable regular file");
    }
    *n = (size_t)st.st_size;
    p = malloc(*n + 1);
    if (!p) { close(fd); die("oom capturing input"); }
    while (off < *n) {
        ssize_t nr = read(fd, p + off, *n - off);
        if (nr <= 0) { free(p); close(fd); die("cannot capture complete input"); }
        off += (size_t)nr;
    }
    {
        uint8_t extra;
        ssize_t nr = read(fd, &extra, 1);
        if (nr != 0) { free(p); close(fd); die("input grew while being captured"); }
    }
    if (close(fd) != 0) { free(p); die("cannot close captured input"); }
    p[*n] = 0;
    return p;
}

#define C4_RECEIPT_LINE 4096
#define C4_RECEIPT_PATH 2048

typedef struct {
    char builder_sha[65];
    char verifier_sha[65];
    uint8_t *exclusions_raw;
    size_t exclusions_bytes;
    char receipt_sha[65];
    size_t receipt_bytes;
} C4Roots;

typedef struct {
    char base_sha[65];
    char receipt_sha[65];
    size_t base_bytes;
    size_t ff_bytes;
    size_t receipt_bytes;
} C4BaseCommit;

typedef struct {
    char roots_sha[65];
    char commit_sha[65];
    char receipt_sha[65];
    size_t roots_bytes;
    size_t commit_bytes;
    size_t receipt_bytes;
} C4BaseFreeze;

typedef struct {
    char roots_sha[65];
    char commit_sha[65];
    char freeze_sha[65];
    char builder_sha[65];
    char verifier_sha[65];
    char base_sha[65];
    char class_sha[65];
    char class_name[16];
    char seed_sha[65];
    size_t roots_bytes;
    size_t commit_bytes;
    size_t freeze_bytes;
    size_t base_bytes;
    int class_index;
    uint64_t world_seed;
    uint64_t half_tail_seed;
    char receipt_sha[65];
    size_t receipt_bytes;
} C4Selection;

typedef struct {
    uint8_t *raw;
    size_t bytes;
    size_t pos;
} C4Text;

static C4Text c4_text_from_raw(uint8_t *raw, size_t bytes) {
    C4Text t;
    if (!bytes || raw[bytes - 1] != '\n' || memchr(raw, 0, bytes) ||
        memchr(raw, '\r', bytes))
        die("captured text is not strict NUL-free LF text");
    t.raw = raw; t.bytes = bytes; t.pos = 0;
    return t;
}

static C4Text c4_text_load(const char *path) {
    size_t bytes;
    uint8_t *raw = c4_read_file(path, &bytes);
    return c4_text_from_raw(raw, bytes);
}

static char *c4_text_next(C4Text *t) {
    uint8_t *lf;
    char *line;
    size_t len;
    if (t->pos == t->bytes) return NULL;
    line = (char *)t->raw + t->pos;
    lf = memchr(t->raw + t->pos, '\n', t->bytes - t->pos);
    if (!lf) die("captured text line is not LF-terminated");
    len = (size_t)(lf - (t->raw + t->pos));
    if (len >= C4_RECEIPT_LINE) die("captured text line is too long");
    *lf = 0;
    t->pos += len + 1;
    return line;
}

static char *c4_text_field(C4Text *t, const char *want) {
    char *line = c4_text_next(t);
    char *tab;
    if (!line) die("missing receipt field");
    tab = strchr(line, '\t');
    if (!tab || strchr(tab + 1, '\t')) die("malformed two-column receipt row");
    *tab++ = 0;
    if (strcmp(line, want)) die("unexpected receipt field order");
    return tab;
}

static void c4_copy_value(char *dst, size_t cap, const char *src) {
    size_t n = strlen(src);
    if (n >= cap) die("receipt field too long");
    memcpy(dst, src, n + 1);
}

static void c4_text_digest(const C4Text *t, char hex[65]) {
    uint8_t d[32];
    c4_sha_bytes(t->raw, t->bytes, d);
    c4_hex(d, 32, hex);
}

static size_t c4_parse_size(const char *s) {
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

static uint64_t c4_parse_hex16(const char *s) {
    if (!s || strlen(s) != 16) die("invalid be64 receipt field");
    uint64_t v = 0;
    for (int i = 0; i < 16; i++) {
        unsigned char c = (unsigned char)s[i];
        unsigned x = 0;
        if (c >= '0' && c <= '9') x = c - '0';
        else if (c >= 'a' && c <= 'f') x = c - 'a' + 10;
        else die("invalid be64 receipt hex");
        v = (v << 4) | x;
    }
    return v;
}

static int c4_parent_segment(const char *path) {
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

static void c4_resolve_receipt_path(const char *receipt, const char *relative,
                                    char out[C4_RECEIPT_PATH]) {
    if (!*relative || relative[0] == '/' || c4_parent_segment(relative))
        die("unsafe path in roots receipt");
    const char *slash = strrchr(receipt, '/');
    int rc;
    if (slash) {
        size_t dirn = (size_t)(slash - receipt);
        rc = snprintf(out, C4_RECEIPT_PATH, "%.*s/%s",
                      (int)dirn, receipt, relative);
    } else {
        rc = snprintf(out, C4_RECEIPT_PATH, "%s", relative);
    }
    if (rc < 0 || rc >= C4_RECEIPT_PATH) die("resolved receipt path too long");
}

static void c4_verify_prior_binding(uint8_t *raw, size_t bytes,
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
    C4Text t = c4_text_from_raw(raw, bytes);
    char *line = c4_text_next(&t);
    size_t row = 0;
    if (!line || strcmp(line, "stage\tpath\tbytes\tsha256"))
        die("prior freeze receipt header mismatch");
    while ((line = c4_text_next(&t)) != NULL) {
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
            !c4_lower_hex64(sha))
            die("prior freeze row identity mismatch");
        size_t pinned_bytes = c4_parse_size(size_text);
        if (row > 0 &&
            (pinned_bytes != root_bytes[row] || strcmp(sha, root_sha[row])))
            die("development root contradicts prior freeze receipt");
        row++;
    }
    if (row != 7) die("prior freeze receipt row count mismatch");
}

static void c4_verify_roots(const char *path, C4Roots *roots) {
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
    C4Text t;
    char *line;
    size_t captured_bytes[18] = {0};
    char captured_sha[18][65] = {{0}};
    uint8_t *prior_raw = NULL;
    size_t prior_bytes = 0;
    memset(roots, 0, sizeof(*roots));
    t = c4_text_load(path);
    roots->receipt_bytes = t.bytes;
    c4_text_digest(&t, roots->receipt_sha);
    line = c4_text_next(&t);
    if (!line || strcmp(line, "role\tpath\tbytes\tsha256"))
        die("invalid roots receipt header");
    size_t row = 0;
    while ((line = c4_text_next(&t)) != NULL) {
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
        if (!*role || !*p1 || !c4_lower_hex64(p3))
            die("invalid roots receipt value");
        if (row >= sizeof(roles) / sizeof(roles[0]) ||
            strcmp(role, roles[row]))
            die("missing, duplicate, unknown, or out-of-order roots role");
        size_t want_bytes = c4_parse_size(p2);
        char resolved[C4_RECEIPT_PATH], got_sha[65];
        size_t got_bytes;
        uint8_t *got;
        uint8_t got_digest[32];
        c4_resolve_receipt_path(path, p1, resolved);
        got = c4_read_file(resolved, &got_bytes);
        c4_sha_bytes(got, got_bytes, got_digest);
        c4_hex(got_digest, 32, got_sha);
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
            got = NULL;
        } else if (row == 7) {
            if (got_bytes != 26929 ||
                strcmp(p3, "f823478054705091300b6a5f0ce4a0b43af50f019cea32dc27e8aa10889ba908"))
                die("execution law root is not frozen");
        } else if (row == 8) {
            if (got_bytes != 869 ||
                strcmp(p3, "7bcdc748e2cfe9a60112ef36b8718f504620f2f16682f32da0e0bd553c077501"))
                die("execution law receipt root is not frozen");
        } else if (row == 10) {
            roots->exclusions_raw = got;
            roots->exclusions_bytes = got_bytes;
            got = NULL;
        } else if (row == 11) {
            memcpy(roots->builder_sha, p3, 65);
        } else if (row == 13) {
            memcpy(roots->verifier_sha, p3, 65);
        }
        free(got);
        row++;
    }
    free(t.raw);
    if (row != sizeof(roles) / sizeof(roles[0]))
        die("missing roots receipt role");
    if (!prior_raw) die("missing captured prior freeze receipt");
    c4_verify_prior_binding(prior_raw, prior_bytes, captured_bytes,
                            captured_sha);
    free(prior_raw);
}

static void c4_check_exclusions(C4Roots *roots, size_t base_bytes,
                                const char *base_sha) {
    C4Text t;
    char *line;
    if (!roots->exclusions_raw) die("missing captured confirmatory exclusions");
    t = c4_text_from_raw(roots->exclusions_raw, roots->exclusions_bytes);
    line = c4_text_next(&t);
    if (!line || strcmp(line, "label\tbytes\tsha256"))
        die("invalid confirmatory exclusions header");
    size_t rows = 0;
    while ((line = c4_text_next(&t)) != NULL) {
        char *label = line;
        char *p1 = strchr(label, '\t');
        if (!p1) die("malformed confirmatory exclusions row");
        *p1++ = 0;
        char *p2 = strchr(p1, '\t');
        if (!p2 || strchr(p2 + 1, '\t'))
            die("malformed confirmatory exclusions row");
        *p2++ = 0;
        if (!*label || !c4_lower_hex64(p2))
            die("invalid confirmatory exclusions value");
        size_t excluded_bytes = c4_parse_size(p1);
        if (base_bytes == excluded_bytes && !strcmp(base_sha, p2))
            die("base is ineligible: exact copy of development material");
        rows++;
    }
    if (!rows) die("empty confirmatory exclusions");
}

static int c4_ff_is_ws(uint8_t c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

static uint64_t c4_ff_hash(const uint8_t *s, uint32_t len) {
    uint64_t h = 0xcbf29ce484222325ull;
    for (uint32_t i = 0; i < len; i++)
        h = (h ^ s[i]) * 0x100000001b3ull;
    return h;
}

static size_t c4_ff_preflight_length(const uint8_t *src, size_t n) {
    enum { WH = 1 << 16 };
    struct C4FfWord {
        uint8_t s[64];
        uint32_t len;
        uint64_t cnt;
        size_t first;
        int used;
    };
    struct C4FfWord *wh = calloc(WH, sizeof(*wh));
    if (!wh) die("oom false-friend capacity preflight");
    if (n > (SIZE_MAX - 16) / 2) {
        free(wh);
        die("base is ineligible: false-friend capacity ceiling overflows");
    }
    size_t i = 0;
    while (i < n) {
        if (c4_ff_is_ws(src[i])) {
            i++;
            continue;
        }
        size_t j = i;
        while (j < n && !c4_ff_is_ws(src[j])) j++;
        size_t span = j - i;
        if (span < 64) {
            uint32_t len = (uint32_t)span;
            uint32_t slot = (uint32_t)(c4_ff_hash(src + i, len) >> 48);
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
            if (!found) {
                free(wh);
                die("base is ineligible: false-friend vocabulary exceeds capacity");
            }
        }
        i = j;
    }
    int picked[16];
    for (int k = 0; k < 16; k++) {
        int best = -1;
        for (int s = 0; s < WH; s++) {
            if (!wh[s].used) continue;
            int duplicate = 0;
            for (int q = 0; q < k; q++)
                if (picked[q] == s) duplicate = 1;
            if (duplicate) continue;
            if (best < 0 || wh[s].cnt > wh[best].cnt ||
                (wh[s].cnt == wh[best].cnt &&
                 wh[s].first < wh[best].first))
                best = s;
        }
        if (best < 0) {
            free(wh);
            die("base is ineligible: false-friend world has fewer than 16 words");
        }
        picked[k] = best;
    }
    size_t out = 0;
    i = 0;
    while (i < n) {
        size_t add;
        if (c4_ff_is_ws(src[i])) {
            add = 1;
            i++;
        } else {
            size_t j = i;
            while (j < n && !c4_ff_is_ws(src[j])) j++;
            size_t span = j - i;
            int hit = -1;
            if (span < 64) {
                uint32_t len = (uint32_t)span;
                for (int k = 0; k < 16; k++) {
                    int p = picked[k];
                    if (wh[p].len == len &&
                        !memcmp(wh[p].s, src + i, len)) {
                        hit = k;
                        break;
                    }
                }
            }
            if (hit < 0) add = span;
            else {
                int partner = (hit % 2 == 0) ? hit + 1 : hit - 1;
                add = wh[picked[partner]].len;
            }
            i = j;
        }
        if (add > SIZE_MAX - out) {
            free(wh);
            die("base is ineligible: false-friend output length overflows");
        }
        out += add;
    }
    free(wh);
    if (out > n * 2 + 16)
        die("base is ineligible: false-friend output exceeds capacity ceiling");
    return out;
}

static uint8_t *c4_safe_build_ff(const uint8_t *src, size_t n,
                                 size_t *outn) {
    size_t expected = c4_ff_preflight_length(src, n);
    uint8_t *out = build_ff(src, n, outn);
    if (*outn != expected) {
        free(out);
        die("false-friend preflight and construction lengths disagree");
    }
    return out;
}

static void c4_read_base_commit(const char *path, C4BaseCommit *commit) {
    C4Text t;
    char *value;
    memset(commit, 0, sizeof(*commit));
    t = c4_text_load(path);
    commit->receipt_bytes = t.bytes;
    c4_text_digest(&t, commit->receipt_sha);
    value = c4_text_next(&t);
    if (!value || strcmp(value, "field\tvalue"))
        die("invalid base commitment header");
    value = c4_text_field(&t, "status");
    if (strcmp(value, "base-committed-class-not-computed"))
        die("invalid base commitment status");
    value = c4_text_field(&t, "base_text_bytes");
    commit->base_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "ff_transformed_bytes");
    commit->ff_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "base_text_sha256");
    c4_copy_value(commit->base_sha, sizeof(commit->base_sha), value);
    if (!c4_lower_hex64(commit->base_sha)) die("invalid committed base hash");
    if (c4_text_next(&t)) die("extra base commitment row");
    free(t.raw);
}

static void c4_read_base_freeze(const char *path, C4BaseFreeze *freeze) {
    C4Text t;
    char *value;
    memset(freeze, 0, sizeof(*freeze));
    t = c4_text_load(path);
    freeze->receipt_bytes = t.bytes;
    c4_text_digest(&t, freeze->receipt_sha);
    value = c4_text_next(&t);
    if (!value || strcmp(value, "field\tvalue"))
        die("invalid base freeze header");
    value = c4_text_field(&t, "status");
    if (strcmp(value, "base-commit-frozen-class-not-computed"))
        die("invalid base freeze status");
    value = c4_text_field(&t, "roots_receipt_bytes");
    freeze->roots_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "roots_receipt_sha256");
    c4_copy_value(freeze->roots_sha, sizeof(freeze->roots_sha), value);
    value = c4_text_field(&t, "base_commit_bytes");
    freeze->commit_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "base_commit_sha256");
    c4_copy_value(freeze->commit_sha, sizeof(freeze->commit_sha), value);
    if (!c4_lower_hex64(freeze->roots_sha) ||
        !c4_lower_hex64(freeze->commit_sha))
        die("invalid base freeze hash");
    if (c4_text_next(&t)) die("extra base freeze row");
    free(t.raw);
}

static void c4_check_base_freeze(const C4Roots *roots,
                                 const C4BaseCommit *commit,
                                 const C4BaseFreeze *freeze) {
    if (freeze->roots_bytes != roots->receipt_bytes ||
        strcmp(freeze->roots_sha, roots->receipt_sha) ||
        freeze->commit_bytes != commit->receipt_bytes ||
        strcmp(freeze->commit_sha, commit->receipt_sha))
        die("base freeze receipt disagrees with frozen inputs");
}

static void c4_read_selection(const char *path, const char *expected_status,
                              C4Selection *selection) {
    C4Text t;
    char *value;
    memset(selection, 0, sizeof(*selection));
    t = c4_text_load(path);
    selection->receipt_bytes = t.bytes;
    c4_text_digest(&t, selection->receipt_sha);
    value = c4_text_next(&t);
    if (!value || strcmp(value, "field\tvalue"))
        die("invalid selection receipt header");
    value = c4_text_field(&t, "status");
    if (strcmp(value, expected_status)) die("invalid selection status");
    value = c4_text_field(&t, "roots_receipt_bytes");
    selection->roots_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "roots_receipt_sha256");
    c4_copy_value(selection->roots_sha, sizeof(selection->roots_sha), value);
    value = c4_text_field(&t, "base_commit_bytes");
    selection->commit_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "base_commit_sha256");
    c4_copy_value(selection->commit_sha, sizeof(selection->commit_sha), value);
    value = c4_text_field(&t, "base_freeze_bytes");
    selection->freeze_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "base_freeze_sha256");
    c4_copy_value(selection->freeze_sha, sizeof(selection->freeze_sha), value);
    value = c4_text_field(&t, "builder_source_sha256");
    c4_copy_value(selection->builder_sha, sizeof(selection->builder_sha), value);
    value = c4_text_field(&t, "verifier_source_sha256");
    c4_copy_value(selection->verifier_sha, sizeof(selection->verifier_sha), value);
    value = c4_text_field(&t, "base_text_bytes");
    selection->base_bytes = c4_parse_size(value);
    value = c4_text_field(&t, "base_text_sha256");
    c4_copy_value(selection->base_sha, sizeof(selection->base_sha), value);
    value = c4_text_field(&t, "class_digest_sha256");
    c4_copy_value(selection->class_sha, sizeof(selection->class_sha), value);
    value = c4_text_field(&t, "class_index");
    size_t ci = c4_parse_size(value);
    if (ci > 3) die("invalid selection class index");
    selection->class_index = (int)ci;
    value = c4_text_field(&t, "class");
    c4_copy_value(selection->class_name, sizeof(selection->class_name), value);
    value = c4_text_field(&t, "seed_digest_sha256");
    c4_copy_value(selection->seed_sha, sizeof(selection->seed_sha), value);
    value = c4_text_field(&t, "world_seed_be64");
    selection->world_seed = c4_parse_hex16(value);
    value = c4_text_field(&t, "half_tail_seed_be64");
    selection->half_tail_seed = c4_parse_hex16(value);
    if (c4_text_next(&t)) die("extra selection receipt row");
    free(t.raw);
    if (!c4_lower_hex64(selection->roots_sha) ||
        !c4_lower_hex64(selection->commit_sha) ||
        !c4_lower_hex64(selection->freeze_sha) ||
        !c4_lower_hex64(selection->builder_sha) ||
        !c4_lower_hex64(selection->verifier_sha) ||
        !c4_lower_hex64(selection->base_sha) ||
        !c4_lower_hex64(selection->class_sha) ||
        !c4_lower_hex64(selection->seed_sha))
        die("invalid selection receipt hash");
}

enum C4Class { C4_CIPHER = 0, C4_PLAIN = 1, C4_HALF = 2, C4_FF = 3 };

static const char *c4_class_name(enum C4Class c) {
    static const char *names[4] = {"cipher", "plain", "half", "ff"};
    return c >= C4_CIPHER && c <= C4_FF ? names[c] : "invalid";
}

static int c4_parse_class(const char *s, enum C4Class *out) {
    for (int i = 0; i < 4; i++)
        if (!strcmp(s, c4_class_name((enum C4Class)i))) {
            *out = (enum C4Class)i;
            return 1;
        }
    return 0;
}

static void c4_derive_selection(const char *builder_sha,
                                const char *verifier_sha,
                                const char *base_sha,
                                enum C4Class *cls, uint64_t *world_seed,
                                char class_hex[65], char seed_hex[65]) {
    static const char class_tag[] = "NETTA-C4-CLASS-v2\n";
    static const char seed_tag[] = "NETTA-C4-SEED-v2\n";
    uint8_t class_digest[32], seed_digest[32];
    C4Sha256 sha;
    c4_sha_init(&sha);
    c4_sha_update(&sha, class_tag, sizeof(class_tag) - 1);
    c4_sha_update(&sha, builder_sha, 64);
    c4_sha_update(&sha, verifier_sha, 64);
    c4_sha_update(&sha, base_sha, 64);
    c4_sha_final(&sha, class_digest);
    c4_hex(class_digest, 32, class_hex);
    *cls = (enum C4Class)(class_digest[31] & 3u);
    c4_sha_init(&sha);
    c4_sha_update(&sha, seed_tag, sizeof(seed_tag) - 1);
    c4_sha_update(&sha, builder_sha, 64);
    c4_sha_update(&sha, verifier_sha, 64);
    c4_sha_update(&sha, base_sha, 64);
    c4_sha_final(&sha, seed_digest);
    c4_hex(seed_digest, 32, seed_hex);
    *world_seed = c4_be64(seed_digest);
}

static uint64_t c4_half_tail_seed(uint64_t world_seed) {
    static const char tag[] = "NETTA-C4-HALF-TAIL-v1";
    uint8_t seed[8], d[32];
    C4Sha256 s;
    c4_put_be64(world_seed, seed);
    c4_sha_init(&s);
    c4_sha_update(&s, tag, sizeof(tag) - 1);
    c4_sha_update(&s, seed, sizeof(seed));
    c4_sha_final(&s, d);
    return c4_be64(d);
}

static void c4_selection_self_test(void) {
    static const char builder_sha[65] =
        "0000000000000000000000000000000000000000000000000000000000000000";
    static const char verifier_sha[65] =
        "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
    static const char base_want[65] =
        "f318964f23519b415bd3ef68ad9c03843a8af33e2d95f152a8a9797ef373d73f";
    static const char class_want[65] =
        "b0c142688cb55362d4952ef0bc42904537ce7ceac7d1b52df3e20cf4342c6be1";
    static const char seed_want[65] =
        "f476f3a4bcc1d16f666acb811ef8c732e9c6a9574461a448d780cff225afb4de";
    static const char base_text[] = "NETTA-C4-SELECTION-TEST-v1";
    uint8_t base_digest[32];
    char base_hex[65], class_hex[65], seed_hex[65];
    enum C4Class cls;
    uint64_t world_seed;
    c4_sha_bytes(base_text, sizeof(base_text) - 1, base_digest);
    c4_hex(base_digest, 32, base_hex);
    c4_derive_selection(builder_sha, verifier_sha, base_hex, &cls,
                        &world_seed, class_hex, seed_hex);
    if (strcmp(base_hex, base_want) || strcmp(class_hex, class_want) ||
        cls != C4_PLAIN || strcmp(seed_hex, seed_want) ||
        world_seed != 0xf476f3a4bcc1d16full ||
        c4_half_tail_seed(world_seed) != 0x4372270db242db11ull)
        die("confirmatory selection self-test failed");
}

static void c4_validate_receipts(const char *roots_path,
                                 const char *commit_path,
                                 const char *freeze_path,
                                 const char *selection_path,
                                 const char *base_path,
                                 const char *expected_status,
                                 C4Roots *roots, C4BaseCommit *commit,
                                 C4BaseFreeze *freeze,
                                 C4Selection *selection,
                                 enum C4Class *cls, uint64_t *world_seed,
                                 char class_hex[65], char seed_hex[65],
                                 char base_hex[65], uint8_t **base_out,
                                 size_t *base_n_out) {
    c4_verify_roots(roots_path, roots);
    c4_read_base_commit(commit_path, commit);
    c4_read_base_freeze(freeze_path, freeze);
    c4_check_base_freeze(roots, commit, freeze);
    c4_read_selection(selection_path, expected_status, selection);
    size_t base_n;
    uint8_t *base = c4_read_file(base_path, &base_n);
    uint8_t base_digest[32];
    c4_sha_bytes(base, base_n, base_digest);
    c4_hex(base_digest, 32, base_hex);
    if (base_n < RUN_BYTES)
        die("base is ineligible: raw length is below RUN_BYTES");
    c4_check_exclusions(roots, base_n, base_hex);
    size_t ff_n;
    uint8_t *ff_probe = c4_safe_build_ff(base, base_n, &ff_n);
    free(ff_probe);
    if (ff_n < RUN_BYTES)
        die("base is ineligible: false-friend length is below RUN_BYTES");
    if (base_n != commit->base_bytes || ff_n != commit->ff_bytes ||
        strcmp(base_hex, commit->base_sha))
        die("base text differs from frozen base commitment");
    if (selection->roots_bytes != roots->receipt_bytes ||
        strcmp(selection->roots_sha, roots->receipt_sha) ||
        selection->commit_bytes != commit->receipt_bytes ||
        strcmp(selection->commit_sha, commit->receipt_sha) ||
        selection->freeze_bytes != freeze->receipt_bytes ||
        strcmp(selection->freeze_sha, freeze->receipt_sha) ||
        strcmp(selection->builder_sha, roots->builder_sha) ||
        strcmp(selection->verifier_sha, roots->verifier_sha) ||
        selection->base_bytes != commit->base_bytes ||
        strcmp(selection->base_sha, commit->base_sha))
        die("selection receipt disagrees with frozen inputs");
    c4_derive_selection(roots->builder_sha, roots->verifier_sha, base_hex, cls,
                        world_seed, class_hex, seed_hex);
    if (selection->class_index != (int)*cls ||
        strcmp(selection->class_name, c4_class_name(*cls)) ||
        strcmp(selection->class_sha, class_hex) ||
        strcmp(selection->seed_sha, seed_hex) ||
        selection->world_seed != *world_seed ||
        selection->half_tail_seed != c4_half_tail_seed(*world_seed))
        die("selection receipt derivation mismatch");
    *base_out = base;
    *base_n_out = base_n;
}

static uint8_t *c4_construct_world(enum C4Class cls, uint64_t world_seed,
                                   const uint8_t *base, size_t basen,
                                   uint8_t oracle[256],
                                   uint64_t *half_tail_seed_out) {
    if (basen < RUN_BYTES) die("base text shorter than RUN_BYTES");
    for (int i = 0; i < 256; i++) oracle[i] = (uint8_t)i;
    *half_tail_seed_out = 0;
    if (cls == C4_PLAIN) {
        uint8_t *w = malloc(RUN_BYTES);
        if (!w) die("oom plain confirmatory world");
        memcpy(w, base, RUN_BYTES);
        return w;
    }
    if (cls == C4_FF) {
        size_t ffn;
        uint8_t *w = c4_safe_build_ff(base, basen, &ffn);
        if (ffn < RUN_BYTES)
            die("false-friend confirmatory world shorter than RUN_BYTES");
        return w;
    }
    fy_perm(cipher_pi, world_seed);
    memcpy(oracle, cipher_pi, 256);
    if (cls == C4_CIPHER)
        return build_cipher(base, RUN_BYTES);
    if (cls == C4_HALF) {
        uint8_t *w = malloc(RUN_BYTES);
        if (!w) die("oom half confirmatory world");
        for (size_t i = 0; i < HALF_BOUNDARY; i++)
            w[i] = cipher_pi[base[i]];
        *half_tail_seed_out = c4_half_tail_seed(world_seed);
        uint8_t *tail = build_ghost_bytes(base, basen,
                                          RUN_BYTES - HALF_BOUNDARY,
                                          *half_tail_seed_out);
        memcpy(w + HALF_BOUNDARY, tail, RUN_BYTES - HALF_BOUNDARY);
        free(tail);
        return w;
    }
    die("invalid confirmatory class");
    return NULL;
}

static int c4_outdir_fd = -1;

static FILE *c4_open_out(const char *name) {
    struct stat st;
    int fd;
    FILE *f;
    if (c4_outdir_fd < 0) die("confirmatory output directory is not held");
    fd = openat(c4_outdir_fd, name,
                O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
    if (fd < 0 || fstat(fd, &st) != 0 || !S_ISREG(st.st_mode)) {
        if (fd >= 0) close(fd);
        die("cannot create exclusive regular confirmatory artifact");
    }
    f = fdopen(fd, "wb");
    if (!f) {
        close(fd);
        die("cannot stream exclusive confirmatory artifact");
    }
    return f;
}

static void c4_write_oracle(const uint8_t oracle[256]) {
    FILE *f = c4_open_out("oracle_confirmatory.tsv");
    fprintf(f, "source\tdestination\n");
    for (int i = 0; i < 256; i++) fprintf(f, "%d\t%u\n", i, oracle[i]);
    fclose(f);
}

static void c4_write_g(double G[NARMS][5]) {
    static const size_t horizons[5] = {1024, 4096, 16384, 65536, RUN_BYTES};
    FILE *f = c4_open_out("builder_G_confirmatory.tsv");
    fprintf(f, "status\tarm\thorizon\tG_bits\n");
    for (int a = 0; a < NARMS; a++)
        for (int h = 0; h < 5; h++)
            fprintf(f, "NOT_VERDICT\t%s\t%zu\t%.17g\n",
                    ARM_NAMES[a], horizons[h], G[a][h]);
    fclose(f);
}

static void c4_write_manifest(const char *mode, enum C4Class cls,
                              const char *builder_sha,
                              const char *verifier_sha,
                              const char *base_sha,
                              const char *class_digest,
                              const char *seed_digest,
                              const char *roots_receipt_sha,
                              const char *base_commit_sha,
                              const char *base_freeze_sha,
                              const char *selection_receipt_sha,
                              uint64_t world_seed, uint64_t half_tail_seed,
                              size_t source_bytes, size_t base_bytes) {
    FILE *f = c4_open_out("confirmatory_selection.tsv");
    fprintf(f, "field\tvalue\n");
    fprintf(f, "status\traw-builder-artifacts-not-verdict\n");
    fprintf(f, "mode\t%s\n", mode);
    fprintf(f, "class\t%s\n", c4_class_name(cls));
    fprintf(f, "class_index\t%d\n", (int)cls);
    fprintf(f, "source_sha256\t%s\n", C4_SOURCE_SHA);
    fprintf(f, "source_bytes\t%zu\n", source_bytes);
    fprintf(f, "builder_source_sha256\t%s\n", builder_sha);
    fprintf(f, "verifier_source_sha256\t%s\n", verifier_sha);
    fprintf(f, "base_text_sha256\t%s\n", base_sha);
    fprintf(f, "class_digest_sha256\t%s\n", class_digest);
    fprintf(f, "seed_digest_sha256\t%s\n", seed_digest);
    fprintf(f, "roots_receipt_sha256\t%s\n", roots_receipt_sha);
    fprintf(f, "base_commit_sha256\t%s\n", base_commit_sha);
    fprintf(f, "base_freeze_sha256\t%s\n", base_freeze_sha);
    fprintf(f, "selection_receipt_sha256\t%s\n", selection_receipt_sha);
    fprintf(f, "world_seed_be64\t%016llx\n",
            (unsigned long long)world_seed);
    if (cls == C4_HALF)
        fprintf(f, "half_tail_seed_be64\t%016llx\n",
                (unsigned long long)half_tail_seed);
    else
        fprintf(f, "half_tail_seed_be64\tNOT_USED\n");
    fprintf(f, "base_bytes\t%zu\n", base_bytes);
    fprintf(f, "world_bytes\t%d\n", RUN_BYTES);
    fclose(f);
}

static int c4_single_world(const char *mode, enum C4Class cls,
                           uint64_t world_seed, const char *apath,
                           const char *bpath, uint8_t *base_snapshot,
                           size_t base_snapshot_n, const char *builder_sha,
                           const char *verifier_sha,
                           const char *class_digest_hex,
                           const char *seed_digest_hex,
                           const char *roots_receipt_sha,
                           const char *base_commit_sha,
                           const char *base_freeze_sha,
                           const char *selection_receipt_sha,
                           const char *expected_base_sha,
                           const char *out_path) {
    size_t an, basen = base_snapshot_n;
    uint8_t *A = c4_read_file(apath, &an);
    uint8_t *base = base_snapshot ? base_snapshot : c4_read_file(bpath, &basen);
    uint8_t base_digest[32];
    uint8_t source_digest[32];
    char base_hex[65], source_hex[65];
    c4_sha_bytes(A, an, source_digest);
    c4_hex(source_digest, 32, source_hex);
    if (an != C4_SOURCE_BYTES || strcmp(source_hex, C4_SOURCE_SHA))
        die("SOURCE_A differs from the frozen source organism");
    c4_sha_bytes(base, basen, base_digest);
    c4_hex(base_digest, 32, base_hex);
    if (expected_base_sha && strcmp(base_hex, expected_base_sha))
        die("base text changed between selection hash and construction");
    if (strlen(out_path) >= C4_RECEIPT_PATH)
        die("output path too long");
    if (mkdir(out_path, 0700) != 0)
        die("output directory must not already exist and must be created by this run");
    c4_outdir_fd = open(out_path, O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
    if (c4_outdir_fd < 0) die("cannot hold owned output directory");
    {
        struct stat st;
        if (fstat(c4_outdir_fd, &st) != 0 || !S_ISDIR(st.st_mode))
            die("owned output path is not a directory");
    }
    uint8_t oracle[256];
    uint64_t half_tail_seed;
    uint8_t *world = c4_construct_world(cls, world_seed, base, basen,
                                        oracle, &half_tail_seed);
    FILE *wf = c4_open_out("W_confirmatory.bin");
    if (fwrite(world, 1, RUN_BYTES, wf) != RUN_BYTES)
        die("write confirmatory world");
    fclose(wf);
    c4_write_oracle(oracle);
    c4_write_manifest(mode, cls, builder_sha, verifier_sha, base_hex,
                      class_digest_hex, seed_digest_hex, roots_receipt_sha,
                      base_commit_sha, base_freeze_sha,
                      selection_receipt_sha, world_seed, half_tail_seed, an,
                      basen);
    double G[NARMS][5];
    transfer4_confirm_core_run(A, an, world, RUN_BYTES, oracle,
                               c4_outdir_fd, G);
    c4_write_g(G);
    fprintf(stderr, "%s class %s: world %d B | seed %016llx | NOT VERDICT\n",
            mode, c4_class_name(cls), RUN_BYTES,
            (unsigned long long)world_seed);
    free(A); free(base); free(world);
    if (close(c4_outdir_fd) != 0) die("cannot close owned output directory");
    c4_outdir_fd = -1;
    return 0;
}

static void c4_usage(void) {
    fprintf(stderr,
        "usage:\n"
        "  transfer4_confirm --development SOURCE_A BASE_D --out DIR\n"
        "  transfer4_confirm --branch-test CLASS SOURCE_A BASE_D --out DIR\n"
        "  transfer4_confirm --receipt-test BASE_TEXT --roots ROOTS.tsv "
        "--base-commit BASE_COMMIT.tsv --base-freeze BASE_FREEZE.tsv "
        "--selection SELECTION.tsv\n"
        "  transfer4_confirm --confirmatory SOURCE_A BASE_TEXT "
        "--roots ROOTS.tsv --base-commit BASE_COMMIT.tsv "
        "--base-freeze BASE_FREEZE.tsv --selection SELECTION.tsv "
        "--out DIR\n");
    exit(1);
}

int main(int argc, char **argv) {
    c4_sha_self_test();
    c4_selection_self_test();
    if (argc < 2) c4_usage();
    if (!strcmp(argv[1], "--development")) {
        if (argc != 6 || strcmp(argv[4], "--out")) c4_usage();
        return transfer4_development_main(argc - 1, argv + 1);
    }

    if (!strcmp(argv[1], "--branch-test")) {
        enum C4Class cls = C4_CIPHER;
        if (argc != 7 || !c4_parse_class(argv[2], &cls) ||
            strcmp(argv[5], "--out"))
            c4_usage();
        return c4_single_world("branch-test-not-confirmatory", cls,
                               0x43344252414e4348ull, argv[3], argv[4],
                               NULL, 0,
                               "NOT_USED", "NOT_USED", "NOT_USED",
                               "NOT_USED", "NOT_USED", "NOT_USED",
                               "NOT_USED", "NOT_USED", NULL, argv[6]);
    }

    if (!strcmp(argv[1], "--receipt-test")) {
        if (argc != 11 || strcmp(argv[3], "--roots") ||
            strcmp(argv[5], "--base-commit") ||
            strcmp(argv[7], "--base-freeze") ||
            strcmp(argv[9], "--selection"))
            c4_usage();
        C4Roots roots;
        C4BaseCommit commit;
        C4BaseFreeze freeze;
        C4Selection selection;
        enum C4Class cls;
        uint64_t world_seed;
        uint8_t *base_snapshot;
        size_t base_snapshot_n;
        char class_hex[65], seed_hex[65], base_hex[65];
        c4_validate_receipts(argv[4], argv[6], argv[8], argv[10], argv[2],
                             "synthetic-selected-not-confirmatory",
                             &roots, &commit, &freeze, &selection, &cls,
                             &world_seed, class_hex, seed_hex, base_hex,
                             &base_snapshot, &base_snapshot_n);
        fprintf(stderr,
                "synthetic receipt test: class %s | seed %016llx | no world run\n",
                c4_class_name(cls), (unsigned long long)world_seed);
        free(base_snapshot);
        free(roots.exclusions_raw);
        return 0;
    }

    if (!strcmp(argv[1], "--confirmatory")) {
        const char *roots_path = NULL, *commit_path = NULL;
        const char *freeze_path = NULL, *selection_path = NULL;
        const char *out_path = NULL;
        if (argc != 14) c4_usage();
        for (int i = 4; i < argc; i += 2) {
            if (!strcmp(argv[i], "--roots")) roots_path = argv[i + 1];
            else if (!strcmp(argv[i], "--base-commit")) commit_path = argv[i + 1];
            else if (!strcmp(argv[i], "--base-freeze")) freeze_path = argv[i + 1];
            else if (!strcmp(argv[i], "--selection")) selection_path = argv[i + 1];
            else if (!strcmp(argv[i], "--out")) out_path = argv[i + 1];
            else c4_usage();
        }
        if (!roots_path || !commit_path || !freeze_path ||
            !selection_path || !out_path) {
            fprintf(stderr, "transfer: missing confirmatory receipt or output path\n");
            return 1;
        }
        C4Roots roots;
        C4BaseCommit commit;
        C4BaseFreeze freeze;
        C4Selection selection;
        uint8_t *base_snapshot;
        size_t base_snapshot_n;
        char class_hex[65], seed_hex[65], base_hex[65];
        enum C4Class cls;
        uint64_t world_seed;
        c4_validate_receipts(roots_path, commit_path, freeze_path,
                             selection_path, argv[3], "selected-not-run",
                             &roots, &commit, &freeze, &selection, &cls,
                             &world_seed, class_hex, seed_hex, base_hex,
                             &base_snapshot, &base_snapshot_n);
        {
            int rc = c4_single_world("confirmatory", cls, world_seed,
                                     argv[2], argv[3], base_snapshot,
                                     base_snapshot_n, roots.builder_sha,
                                     roots.verifier_sha, class_hex, seed_hex,
                                     roots.receipt_sha, commit.receipt_sha,
                                     freeze.receipt_sha, selection.receipt_sha,
                                     base_hex, out_path);
            free(roots.exclusions_raw);
            return rc;
        }
    }
    c4_usage();
    return 1;
}
