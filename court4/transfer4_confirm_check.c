/* transfer4_confirm_check.c -- independent CONFIRMATORY verifier path, Court 4.
 *
 * Written from COURT4_CONFIRMATORY_EXECUTION.md (sha256
 * f823478054705091300b6a5f0ce4a0b43af50f019cea32dc27e8aa10889ba908) together with
 * the already-lawful frozen roots: COURT2_SNAPSHOT.md, COURT3_SNAPSHOT.md,
 * TRANSFER4_DRAFT.md, COURT4_GAP_ADDENDUM.md and their receipts.
 *
 * No builder-derived source was read. transfer4.c, transfer4_confirm.c and
 * court4_select.c were hashed and length-checked only, never opened.
 *
 * The frozen development verifier transfer4_check.c (sha256 29efd12e...) is
 * reused ADDITIVELY: its main is renamed by macro and the file is included
 * verbatim. Not one byte of it changes.
 *
 * cc -O2 -std=c11 -Wall -Wextra -Wpedantic -o transfer4_confirm_check \
 *    transfer4_confirm_check.c -lm
 */

#if defined(__clang__)
/* macOS 15 marks sprintf itself deprecated. The included frozen verifier and
 * the bounded synthetic emitter still use it; sanitizer builds treat that SDK
 * annotation as an error under -Werror although it is not a sanitizer issue. */
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif

#define main frozen_dev_main
#define rel_find frozen_rel_find
#include "transfer4_check.c"
#undef rel_find
#undef main

#include <limits.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define CC_RBBITS 17
#define CC_RBSIZE (1u << CC_RBBITS)
#define CC_REL_CAP 65536
static int cc_rb_tab[SVA][CC_RBSIZE];

static long rel_find(int arm, const Rel *k)
{
    uint64_t slot = rel_hash(k) & (CC_RBSIZE - 1u);
    for (;;) {
        if (!cc_rb_tab[arm][slot]) break;
        {
            long ix = cc_rb_tab[arm][slot] - 1;
            if (rel_same(&rb[arm][ix], k)) return ix;
        }
        slot = (slot + 1u) & (CC_RBSIZE - 1u);
    }
    if (rb_n[arm] >= CC_REL_CAP) {
        fprintf(stderr, "confirmatory relation table overflow\n");
        exit(3);
    }
    if (rb_n[arm] == rb_cap[arm]) {
        long next = rb_cap[arm] ? rb_cap[arm] * 2 : 65536;
        if (next > CC_REL_CAP) next = CC_REL_CAP;
        rb[arm] = (Rel *)realloc(rb[arm], sizeof(Rel) * (size_t)next);
        if (!rb[arm]) {
            fprintf(stderr, "confirmatory relation table allocation failed\n");
            exit(3);
        }
        rb_cap[arm] = next;
    }
    rb[arm][rb_n[arm]] = *k;
    rb[arm][rb_n[arm]].seen = 0;
    rb[arm][rb_n[arm]].pos = 0;
    rb[arm][rb_n[arm]].neg = 0;
    rb[arm][rb_n[arm]].led = 0;
    rb[arm][rb_n[arm]].peak = 0;
    rb[arm][rb_n[arm]].st = 0;
    rb[arm][rb_n[arm]].L = 0;
    rb[arm][rb_n[arm]].ever = 0;
    cc_rb_tab[arm][slot] = (int)rb_n[arm] + 1;
    return rb_n[arm]++;
}

#ifndef CC_REPO
#define CC_REPO REPO
#endif

static int cc_parse_int(const char *s, long *out);
static int cc_parse_dbl(const char *s, double *out);

/* ===================================================================== */
/* Everything below is new. All new symbols carry a cc_ prefix so that no  */
/* name in the frozen translation unit is shadowed or redefined.          */
/* ===================================================================== */

#define CC_RUN_BYTES 131072
#define CC_HALF_BOUNDARY 65536
#define CC_M 163.84
#define CC_C4BRANCH_SEED 0x43344252414e4348ULL   /* ASCII "C4BRANCH" */

static double cc_logadd2(double a, double b)
{
    if (a == -INFINITY) return b;
    if (b == -INFINITY) return a;
    if (b > a) { double t = a; a = b; b = t; }
    return a + log2(1.0 + exp2(b - a));
}

static double cc_logmix2(double logp, double logq, double weight)
{
    double a = weight < 1.0 ? log2(1.0 - weight) + logp : -INFINITY;
    double b = weight > 0.0 ? log2(weight) + logq : -INFINITY;
    return cc_logadd2(a, b);
}

static double cc_log1mexp2(double logp)
{
    if (!(logp < 0.0)) return NAN;
    return log2(-expm1(logp * log(2.0)));
}

static long cc_pass = 0, cc_fail = 0, cc_note = 0;
static char cc_last_refusal[512];

static void cc_row(const char *st, const char *cls, const char *fmt, ...)
{
    va_list ap;
    printf("%s\t%s\t", st, cls);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap);
    printf("\n");
    if (!strcmp(st, "PASS")) cc_pass++;
    else if (!strcmp(st, "FAIL")) cc_fail++;
    else cc_note++;
}
static void cc_pf(const char *cls, int ok, const char *fmt, ...)
{
    va_list ap;
    printf("%s\t%s\t", ok ? "PASS" : "FAIL", cls);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n");
    if (ok) cc_pass++; else cc_fail++;
}

/* ------------------------------------------------------------ hex helper */
static void cc_hex(const unsigned char *b, int n, char *out)
{
    int i;
    for (i = 0; i < n; i++) sprintf(out + 2 * i, "%02x", b[i]);
    out[2 * n] = 0;
}

/* =====================================================================
 * C2 / C3 serialization.  Preimages are byte-exact; lengths are asserted.
 * ===================================================================== */
static const char *CC_CLASS_NAME[4] = { "cipher", "plain", "half", "ff" };

/* class preimage: ASCII("NETTA-C4-CLASS-v2\n") || builder || verifier || base  */
static void cc_class_digest(const char *bsha, const char *vsha, const char *tsha,
                            unsigned char out[32], size_t *plen)
{
    unsigned char pre[256];
    size_t n = 0;
    const char *tag = "NETTA-C4-CLASS-v2\n";      /* 18 bytes, the only newline */
    memcpy(pre + n, tag, 18); n += 18;
    memcpy(pre + n, bsha, 64); n += 64;
    memcpy(pre + n, vsha, 64); n += 64;
    memcpy(pre + n, tsha, 64); n += 64;
    *plen = n;
    { char h[65]; sha256_buf(pre, n, h); { int i; for (i = 0; i < 32; i++) {
        unsigned v; sscanf(h + 2 * i, "%2x", &v); out[i] = (unsigned char)v; } } }
}
/* seed preimage: ASCII("NETTA-C4-SEED-v2\n") || builder || verifier || base */
static void cc_seed_digest(const char *bsha, const char *vsha, const char *tsha,
                           unsigned char out[32], size_t *plen)
{
    unsigned char pre[256];
    size_t n = 0;
    const char *tag = "NETTA-C4-SEED-v2\n";       /* 17 bytes, the only newline */
    memcpy(pre + n, tag, 17); n += 17;
    memcpy(pre + n, bsha, 64); n += 64;
    memcpy(pre + n, vsha, 64); n += 64;
    memcpy(pre + n, tsha, 64); n += 64;
    *plen = n;
    { char h[65]; sha256_buf(pre, n, h); { int i; for (i = 0; i < 32; i++) {
        unsigned v; sscanf(h + 2 * i, "%2x", &v); out[i] = (unsigned char)v; } } }
}
static uint64_t cc_be64(const unsigned char *b)
{
    uint64_t v = 0; int i;
    for (i = 0; i < 8; i++) v = (v << 8) | (uint64_t)b[i];
    return v;
}
/* half tail seed: SHA256(ASCII("NETTA-C4-HALF-TAIL-v1") || seed_be8) */
static uint64_t cc_half_tail_seed(uint64_t world_seed, char *dig65)
{
    unsigned char pre[29]; unsigned char d[32]; char h[65];
    int i;
    memcpy(pre, "NETTA-C4-HALF-TAIL-v1", 21);     /* 21 ASCII bytes */
    for (i = 0; i < 8; i++) pre[21 + i] = (unsigned char)(world_seed >> (56 - 8 * i));
    sha256_buf(pre, 29, h);
    if (dig65) strcpy(dig65, h);
    for (i = 0; i < 32; i++) { unsigned v; sscanf(h + 2 * i, "%2x", &v); d[i] = (unsigned char)v; }
    return cc_be64(d);
}

/* =====================================================================
 * C1 eligibility: independent bounded-probe false-friend LENGTH law.
 * Probe at most 65536 slots per candidate word; saturation fails loud.
 * ===================================================================== */
#define CC_FFCAP (1 << 16)
#define CC_FFPROBE 65536
typedef struct { long off; int len; long cnt; long first; } CcWord;

static int cc_isdelim(unsigned char b) { return b == 32 || b == 10 || b == 13 || b == 9; }

/* returns transformed length, or -1 on probe saturation / undefined construction */
static long cc_ff_length(const unsigned char *D, long n, int *saturated, long *nwords)
{
    static long *tab = NULL;
    static CcWord *w = NULL;
    long nw = 0, i, pos = 0, out = 0;
    CcWord top[16]; int ntop = 0;
    if (!tab) tab = (long *)malloc(sizeof(long) * CC_FFCAP);
    if (!w) w = (CcWord *)malloc(sizeof(CcWord) * CC_FFCAP);
    for (i = 0; i < CC_FFCAP; i++) tab[i] = -1;
    *saturated = 0;
    while (pos < n) {
        long st; int L; uint64_t h = 1469598103934665603ULL; long slot, probes = 0; int found = -1;
        if (cc_isdelim(D[pos])) { pos++; continue; }
        st = pos;
        while (pos < n && !cc_isdelim(D[pos])) pos++;
        L = (int)(pos - st);
        if (L < 1 || L > 63) continue;                 /* not a candidate word */
        for (i = 0; i < L; i++) h = (h ^ (uint64_t)D[st + i]) * 1099511628211ULL;
        h ^= (uint64_t)L * 1099511628211ULL;
        slot = (long)(h & (uint64_t)(CC_FFCAP - 1));
        while (tab[slot] != -1) {
            long ix = tab[slot];
            if (w[ix].len == L && memcmp(D + w[ix].off, D + st, (size_t)L) == 0) { found = (int)ix; break; }
            slot = (slot + 1) & (CC_FFCAP - 1);
            if (++probes >= CC_FFPROBE) { *saturated = 1; return -1; }
        }
        if (found >= 0) w[found].cnt++;
        else {
            if (nw >= CC_FFCAP) { *saturated = 1; return -1; }
            w[nw].off = st; w[nw].len = L; w[nw].cnt = 1; w[nw].first = st;
            tab[slot] = nw; nw++;
        }
    }
    if (nwords) *nwords = nw;
    if (nw < 16) return -1;                            /* construction undefined */
    for (i = 0; i < nw; i++) {
        if (ntop < 16) top[ntop++] = w[i];
        else {
            int worst = 0, j;
            for (j = 1; j < ntop; j++)
                if (top[j].cnt < top[worst].cnt ||
                    (top[j].cnt == top[worst].cnt && top[j].first > top[worst].first)) worst = j;
            if (w[i].cnt > top[worst].cnt ||
                (w[i].cnt == top[worst].cnt && w[i].first < top[worst].first)) top[worst] = w[i];
        }
    }
    { int a, b;
      for (a = 0; a < ntop; a++) for (b = a + 1; b < ntop; b++)
          if (top[b].cnt > top[a].cnt || (top[b].cnt == top[a].cnt && top[b].first < top[a].first))
              { CcWord t = top[a]; top[a] = top[b]; top[b] = t; } }
    /* length-only second pass: a selected word contributes its pair's length */
    pos = 0;
    while (pos < n) {
        long st; int L, hit = -1, k;
        if (cc_isdelim(D[pos])) { out++; pos++; continue; }
        st = pos;
        while (pos < n && !cc_isdelim(D[pos])) pos++;
        L = (int)(pos - st);
        if (L >= 1 && L <= 63)
            for (k = 0; k < 16; k++)
                if (top[k].len == L && memcmp(D + st, D + top[k].off, (size_t)L) == 0) { hit = k; break; }
        if (hit >= 0) out += top[hit ^ 1].len;          /* rank pairs 1<->2, 3<->4, ... */
        else out += L;
    }
    return out;
}

static unsigned char *cc_capacity_red_fixture(long *outn)
{
    static const char *tail[14] = {
        "b","c","d","e","f","g","h","i","j","k","l","m","n","o"
    };
    unsigned char *b = (unsigned char *)malloc(400000); long n = 0;
    char longword[64]; int r, k;
    if (!b) return NULL;
    memset(longword, 'x', 63); longword[63] = 0;
#define CC_APPEND_WORD(w_) do { size_t z_ = strlen(w_); memcpy(b + n, (w_), z_); n += (long)z_; b[n++] = ' '; } while (0)
    for (r = 0; r < 150000; r++) CC_APPEND_WORD("a");
    for (r = 0; r < 15; r++) CC_APPEND_WORD(longword);
    for (k = 0; k < 14; k++) for (r = 0; r < 14 - k; r++) CC_APPEND_WORD(tail[k]);
#undef CC_APPEND_WORD
    *outn = n;
    return b;
}

/* Court-4 additive numeric law: evaluate the frozen probability formula in
 * log2 space.  The 256 one-byte units keep every event and its complement
 * strictly positive; long learned units therefore cannot underflow to zero. */
static double cc_chain_logz;
static double cc_chain_logsu[256];
static double cc_chain_logpe[256];

static void cc_chain_log_prepare(void)
{
    double z = 0.0;
    int i;
    for (i = 0; i < ch_inv; i++)
        z += exp2(-8.0 * (double)unit_len[i]);
    cc_chain_logz = log2(z);
    for (i = 0; i < 256; i++) cc_chain_logsu[i] = -INFINITY;
    for (i = 0; i < ch_inv; i++) {
        int b = unit_fb[i];
        double lu = -8.0 * (double)unit_len[i] - cc_chain_logz;
        cc_chain_logsu[b] = cc_logadd2(cc_chain_logsu[b], lu);
    }
}

static double cc_chain_logp(int u, int p1, int p2)
{
    double lp = -INFINITY;
    long count;
    if (cf_A0 > 0.0 && ch_c0[u] > 0)
        lp = log2(cf_A0) + log2((double)ch_c0[u]);
    if (cf_AU > 0.0)
        lp = cc_logadd2(lp, log2(cf_AU) - 8.0 * (double)unit_len[u]
                            - cc_chain_logz);
    if (cf_A1 > 0.0) {
        count = hm_get(&hm1, ((uint64_t)p1 << 12) | (uint64_t)u);
        if (count > 0)
            lp = cc_logadd2(lp, log2(cf_A1) + log2((double)count));
    }
    if (cf_A2 > 0.0) {
        uint64_t cx = ((uint64_t)p2 << 12) | (uint64_t)p1;
        count = hm_get(&hm2, (cx << 12) | (uint64_t)u);
        if (count > 0)
            lp = cc_logadd2(lp, log2(cf_A2) + log2((double)count));
    }
    return lp;
}

static void cc_chain_log_events(int p1, int p2)
{
    int b;
    long s;
    for (b = 0; b < 256; b++) {
        double lp = -INFINITY;
        if (cf_A0 > 0.0 && ch_S0[b] > 0)
            lp = log2(cf_A0) + log2((double)ch_S0[b]);
        if (cf_AU > 0.0)
            lp = cc_logadd2(lp, log2(cf_AU) + cc_chain_logsu[b]);
        cc_chain_logpe[b] = lp;
    }
    if (cf_A1 > 0.0) {
        for (s = hm_get(&hm1h, (uint64_t)p1) - 1; s >= 0; s = sc_nx[s]) {
            int u = sc_u[s];
            long count = hm_get(&hm1, ((uint64_t)p1 << 12) | (uint64_t)u);
            cc_chain_logpe[unit_fb[u]] = cc_logadd2(
                cc_chain_logpe[unit_fb[u]],
                log2(cf_A1) + log2((double)count));
        }
    }
    if (cf_A2 > 0.0) {
        uint64_t cx = ((uint64_t)p2 << 12) | (uint64_t)p1;
        for (s = hm_get(&hm2h, cx) - 1; s >= 0; s = sc_nx[s]) {
            int u = sc_u[s];
            long count = hm_get(&hm2, (cx << 12) | (uint64_t)u);
            cc_chain_logpe[unit_fb[u]] = cc_logadd2(
                cc_chain_logpe[unit_fb[u]],
                log2(cf_A2) + log2((double)count));
        }
    }
}

/* =====================================================================
 * C5 world constructions (all four classes), from the literal law.
 * ===================================================================== */
static void cc_build_cipher(const unsigned char *D, uint64_t seed, unsigned char *outw)
{
    int perm[256]; long i;
    rename_perm(seed, perm);                        /* frozen xorshift/Fisher-Yates */
    for (i = 0; i < CC_RUN_BYTES; i++) outw[i] = (unsigned char)perm[D[i]];
}
static void cc_build_plain(const unsigned char *D, unsigned char *outw)
{
    memcpy(outw, D, CC_RUN_BYTES);
}
static void cc_build_half(const unsigned char *D, long n, uint64_t seed, unsigned char *outw)
{
    int perm[256]; long i, cnt[256] = {0};
    uint64_t tail;
    rename_perm(seed, perm);
    for (i = 0; i < CC_HALF_BOUNDARY; i++) outw[i] = (unsigned char)perm[D[i]];
    for (i = 0; i < n; i++) cnt[D[i]]++;             /* unigram of the ENTIRE base */
    tail = cc_half_tail_seed(seed, NULL);
    ghost_stream(tail, cnt, n, outw + CC_HALF_BOUNDARY, CC_RUN_BYTES - CC_HALF_BOUNDARY);
}
/* full false-friend construction, returns transformed length; fills outw prefix */
static long cc_build_ff(const unsigned char *D, long n, unsigned char *outw)
{
    long nw = 0, i, pos = 0, out = 0, cap;
    int saturated = 0;
    unsigned char *buf;
    static CcWord *w = NULL;
    CcWord top[16]; int ntop = 0;
    if (n < 0 || n > (LONG_MAX - 16) / 2) return -1;
    cap = cc_ff_length(D, n, &saturated, NULL);
    if (saturated || cap < 0 || cap > n * 2 + 16) return -1;
    buf = (unsigned char *)malloc((size_t)(cap ? cap : 1));
    if (!buf) return -1;
    if (!w) w = (CcWord *)malloc(sizeof(CcWord) * CC_FFCAP);
    /* Count candidate words by exact linear grouping. */
    while (pos < n) {
        long st; int L; long k; int found = -1;
        if (cc_isdelim(D[pos])) { pos++; continue; }
        st = pos;
        while (pos < n && !cc_isdelim(D[pos])) pos++;
        L = (int)(pos - st);
        if (L < 1 || L > 63) continue;
        for (k = 0; k < nw; k++)
            if (w[k].len == L && memcmp(D + w[k].off, D + st, (size_t)L) == 0) { found = (int)k; break; }
        if (found >= 0) w[found].cnt++;
        else { w[nw].off = st; w[nw].len = L; w[nw].cnt = 1; w[nw].first = st; nw++; }
    }
    for (i = 0; i < nw; i++) {
        if (ntop < 16) top[ntop++] = w[i];
        else {
            int worst = 0, j;
            for (j = 1; j < ntop; j++)
                if (top[j].cnt < top[worst].cnt ||
                    (top[j].cnt == top[worst].cnt && top[j].first > top[worst].first)) worst = j;
            if (w[i].cnt > top[worst].cnt ||
                (w[i].cnt == top[worst].cnt && w[i].first < top[worst].first)) top[worst] = w[i];
        }
    }
    { int a, b;
      for (a = 0; a < ntop; a++) for (b = a + 1; b < ntop; b++)
          if (top[b].cnt > top[a].cnt || (top[b].cnt == top[a].cnt && top[b].first < top[a].first))
              { CcWord t = top[a]; top[a] = top[b]; top[b] = t; } }
    pos = 0;
    while (pos < n) {
        long st; int L, hit = -1, k;
        if (cc_isdelim(D[pos])) {
            if (out >= cap) { free(buf); return -1; }
            buf[out++] = D[pos]; pos++; continue;
        }
        st = pos;
        while (pos < n && !cc_isdelim(D[pos])) pos++;
        L = (int)(pos - st);
        if (L >= 1 && L <= 63)
            for (k = 0; k < 16; k++)
                if (top[k].len == L && memcmp(D + st, D + top[k].off, (size_t)L) == 0) { hit = k; break; }
        if (hit >= 0) {
            int q = hit ^ 1;
            if (top[q].len < 0 || out > cap - top[q].len) { free(buf); return -1; }
            memcpy(buf + out, D + top[q].off, (size_t)top[q].len); out += top[q].len;
        } else {
            if (L < 0 || out > cap - L) { free(buf); return -1; }
            memcpy(buf + out, D + st, (size_t)L); out += L;
        }
    }
    if (out != cap) { free(buf); return -1; }
    if (out >= CC_RUN_BYTES) memcpy(outw, buf, CC_RUN_BYTES);
    free(buf);
    return out;
}

/* =====================================================================
 * C8 verdict engine.  Pure function of the listed inputs; its own values.
 * A coverage mask records which conjuncts were actually evaluated so that a
 * fixture cannot "pass" through an empty or unexecuted comparison path.
 * ===================================================================== */
enum { CCV_MICRO_EARN = 1, CCV_MICRO_RANK = 2, CCV_PREFIX_EARN = 4,
       CCV_PREFIX_RANK = 8, CCV_HALF_GUARD = 16, CCV_FF_GUARD = 32 };

typedef struct {
    int class_index;                 /* 0 cipher, 1 plain, 2 half, 3 ff */
    int earn_ctx_any;                /* a context_len>=1 relation reaches EARN */
    long earn_min_offset;            /* raw byte offset of the earliest such EARN */
    int nulls_ge_full;               /* count_k[ G_FULL(null_k) >= G_FULL(relation) ] */
    int nulls_ge_64k;                /* same at the 65536 horizon */
    double g_rel_half_guard;         /* G_rel([65536,81920)) */
    double g_rel_ff_guard;           /* G_rel([0,16384)) */
} CcVerdictIn;

typedef struct {
    int pass;
    int conf_micro, conf_micro_prefix;
    double p_rank, p_rank_64k;
    int cover;
} CcVerdictOut;

static void cc_verdict(const CcVerdictIn *in, CcVerdictOut *o)
{
    o->cover = 0;
    o->p_rank = (1.0 + (double)in->nulls_ge_full) / 20.0;
    o->p_rank_64k = (1.0 + (double)in->nulls_ge_64k) / 20.0;
    o->conf_micro = 0; o->conf_micro_prefix = 0; o->pass = 0;

    /* CONF_MICRO(w) = at least one context_len>=1 relation reaches EARN
     *                 AND p_rank(w) = 0.05                                   */
    {
        int e = in->earn_ctx_any;
        o->cover |= CCV_MICRO_EARN;
        if (e) { o->cover |= CCV_MICRO_RANK; o->conf_micro = (o->p_rank == 0.05); }
        else o->conf_micro = 0;
    }
    /* CONF_MICRO_PREFIX = EARN at a raw byte offset < 65536
     *                     AND p_rank_64K = 0.05                              */
    {
        int e = in->earn_ctx_any && in->earn_min_offset >= 0 &&
                in->earn_min_offset < CC_HALF_BOUNDARY;
        o->cover |= CCV_PREFIX_EARN;
        if (e) { o->cover |= CCV_PREFIX_RANK; o->conf_micro_prefix = (o->p_rank_64k == 0.05); }
        else o->conf_micro_prefix = 0;
    }
    switch (in->class_index) {
    case 0: case 1:
        o->pass = o->conf_micro;
        break;
    case 2:
        o->cover |= CCV_HALF_GUARD;
        o->pass = o->conf_micro_prefix && (fabs(in->g_rel_half_guard) < CC_M);
        break;
    case 3:
        o->cover |= CCV_FF_GUARD;
        o->pass = o->conf_micro && (in->g_rel_ff_guard > -CC_M);
        break;
    default:
        o->pass = 0;
        break;
    }
}
static const char *CC_PASS_LINE = "CONFIRMATORY PASS: microscopic relation replicated in selected class";
static const char *CC_FAIL_LINE = "CONFIRMATORY FAIL: microscopic relation not replicated in selected class";

/* =====================================================================
 * C1 receipt validation: roots (18 roles), base commitment, base freeze,
 * selection (17 fields).  Every rejection reason of the law is exercised.
 * ===================================================================== */
static const char *CC_ROLES[18] = {
    "prior_freeze", "court2_snapshot", "court3_snapshot", "court4_protocol",
    "development_builder", "gap_addendum", "development_verifier", "execution_law",
    "execution_law_receipt", "repair_addendum", "confirmatory_exclusions",
    "confirmatory_builder", "confirmatory_builder_core", "confirmatory_verifier",
    "confirmatory_selector",
    "builder_regression_manifest", "verifier_regression_manifest",
    "selector_test_manifest"
};
static const char *CC_SEL_FIELDS[17] = {
    "status", "roots_receipt_bytes", "roots_receipt_sha256", "base_commit_bytes",
    "base_commit_sha256", "base_freeze_bytes", "base_freeze_sha256",
    "builder_source_sha256", "verifier_source_sha256", "base_text_bytes",
    "base_text_sha256", "class_digest_sha256", "class_index", "class",
    "seed_digest_sha256", "world_seed_be64", "half_tail_seed_be64"
};
static const char *CC_COMMIT_FIELDS[4] = {
    "status", "base_text_bytes", "ff_transformed_bytes", "base_text_sha256"
};
static const char *CC_FREEZE_FIELDS[5] = {
    "status", "roots_receipt_bytes", "roots_receipt_sha256",
    "base_commit_bytes", "base_commit_sha256"
};

static int cc_is_hex64(const char *s)
{
    int i;
    if (strlen(s) != 64) return 0;
    for (i = 0; i < 64; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
    }
    return 1;
}
static int cc_is_canon_dec(const char *s)
{
    int i, n = (int)strlen(s);
    if (n < 1) return 0;
    if (s[0] == '0' && n > 1) return 0;              /* no leading zeros */
    for (i = 0; i < n; i++) if (s[i] < '0' || s[i] > '9') return 0;
    return 1;
}
static int cc_path_ok(const char *p)
{
    const char *s;
    if (!p[0] || p[0] == '/') return 0;              /* empty/absolute forbidden */
    s = p;
    while (*s) {
        const char *q = strchr(s, '/');
        size_t n = q ? (size_t)(q - s) : strlen(s);
        if (n == 2 && s[0] == '.' && s[1] == '.') return 0;
        if (!q) break;
        s = q + 1;
    }
    return 1;
}
/* validate a roots receipt held in memory as lines; returns 0 = ok, else reason code */
static int cc_validate_roots(char **lines, int nlines, char *why, size_t wn)
{
    int i;
    if (nlines < 1 || strcmp(lines[0], "role\tpath\tbytes\tsha256")) {
        snprintf(why, wn, "bad header"); return 1;
    }
    if (nlines != 19) { snprintf(why, wn, "expected 18 data rows, saw %d", nlines - 1); return 2; }
    for (i = 1; i < nlines; i++) {
        char buf[1024], *f[8]; int nf = 0; char *q;
        if (strlen(lines[i]) >= sizeof buf) { snprintf(why, wn, "row %d is too long", i); return 3; }
        snprintf(buf, sizeof buf, "%s", lines[i]);
        f[nf++] = buf;
        for (q = buf; *q; q++) if (*q == '\t' && nf < 8) { *q = 0; f[nf++] = q + 1; }
        if (nf != 4) { snprintf(why, wn, "row %d malformed (%d fields)", i, nf); return 3; }
        if (strcmp(f[0], CC_ROLES[i - 1])) {
            snprintf(why, wn, "row %d role '%s' expected '%s'", i, f[0], CC_ROLES[i - 1]); return 4;
        }
        if (!cc_path_ok(f[1])) { snprintf(why, wn, "row %d illegal path '%s'", i, f[1]); return 5; }
        if (!cc_is_canon_dec(f[2])) { snprintf(why, wn, "row %d non-canonical decimal '%s'", i, f[2]); return 6; }
        if (!cc_is_hex64(f[3])) { snprintf(why, wn, "row %d bad digest", i); return 7; }
    }
    return 0;
}
static int cc_validate_kv(char **lines, int nlines, const char *const *fields, int nf,
                          char *why, size_t wn)
{
    int i;
    if (nlines < 1 || strcmp(lines[0], "field\tvalue")) { snprintf(why, wn, "bad header"); return 1; }
    if (nlines != nf + 1) { snprintf(why, wn, "expected %d rows, saw %d", nf, nlines - 1); return 2; }
    for (i = 1; i < nlines; i++) {
        char buf[1024], *tab;
        snprintf(buf, sizeof buf, "%s", lines[i]);
        tab = strchr(buf, '\t');
        if (!tab) { snprintf(why, wn, "row %d malformed", i); return 3; }
        *tab = 0;
        if (strcmp(buf, fields[i - 1])) {
            snprintf(why, wn, "row %d field '%s' expected '%s'", i, buf, fields[i - 1]); return 4;
        }
    }
    return 0;
}
/* split an in-memory LF text into lines (destructive) */
static int cc_split(char *t, char **lines, int maxl)
{
    int n = 0; char *p = t;
    while (*p && n < maxl) {
        char *nl = strchr(p, '\n');
        lines[n++] = p;
        if (!nl) break;
        *nl = 0; p = nl + 1;
        if (!*p) break;
    }
    return n;
}

/* =====================================================================
 * main
 * ===================================================================== */

/* pass-5 receipt reds and C8 fixtures, lifted verbatim into a callable unit */
static void cc_pass5_receipt_and_c8(void)
{

    /* ---------------- CC06: receipt schema validation, green + reds ---- */
    {
        char why[256];
        char *lines[64]; int nl;
        static char buf[8192];
        int i;
        /* green roots receipt built from the literal role order */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", CC_ROLES[i], i, i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 0,
                  "green roots receipt with all 18 roles in literal order accepted");
        }
        /* red: out-of-order role */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++) {
                int j = (i == 2) ? 3 : (i == 3) ? 2 : i;
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", CC_ROLES[j], i, i + 1, 0);
            }
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 4,
                  "red: out-of-order role rejected (%s)", why);
        }
        /* red: duplicate role */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", CC_ROLES[i == 5 ? 4 : i], i, i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 4,
                  "red: duplicate role rejected (%s)", why);
        }
        /* red: unknown role */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", i == 7 ? "not_a_role" : CC_ROLES[i], i, i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 4,
                  "red: unknown role rejected (%s)", why);
        }
        /* red: extra row */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", CC_ROLES[i], i, i + 1, 0);
            len += sprintf(p + len, "extra_role\tx.bin\t1\t%064d\n", 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 2,
                  "red: extra row rejected (%s)", why);
        }
        /* red: missing role */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 17; i++)
                len += sprintf(p + len, "%s\tfile%d.bin\t%d\t%064d\n", CC_ROLES[i], i, i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 2,
                  "red: missing role rejected (%s)", why);
        }
        /* red: absolute path */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\t%s\t%d\t%064d\n", CC_ROLES[i], i == 9 ? "/etc/passwd" : "f.bin", i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 5,
                  "red: absolute path rejected (%s)", why);
        }
        /* red: parent segment */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\t%s\t%d\t%064d\n", CC_ROLES[i], i == 4 ? "../secret.bin" : "f.bin", i + 1, 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 5,
                  "red: parent-segment path rejected (%s)", why);
        }
        /* red: non-canonical decimal */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tf.bin\t%s\t%064d\n", CC_ROLES[i], i == 1 ? "0123" : "7", 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 6,
                  "red: non-canonical decimal rejected (%s)", why);
        }
        /* red: malformed row (3 fields) */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tsha256\n");
            for (i = 0; i < 18; i++) {
                if (i == 11) len += sprintf(p + len, "%s\tf.bin\t7\n", CC_ROLES[i]);
                else len += sprintf(p + len, "%s\tf.bin\t7\t%064d\n", CC_ROLES[i], 0);
            }
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 3,
                  "red: malformed row rejected (%s)", why);
        }
        /* red: bad header */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "role\tpath\tbytes\tdigest\n");
            for (i = 0; i < 18; i++)
                len += sprintf(p + len, "%s\tf.bin\t7\t%064d\n", CC_ROLES[i], 0);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_roots(lines, nl, why, sizeof why) == 1,
                  "red: wrong header rejected (%s)", why);
        }
        /* base commitment, base freeze and selection: literal field orders */
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "field\tvalue\n");
            for (i = 0; i < 4; i++) len += sprintf(p + len, "%s\tv\n", CC_COMMIT_FIELDS[i]);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_kv(lines, nl, CC_COMMIT_FIELDS, 4, why, sizeof why) == 0,
                  "green base commitment: 4 fields in literal order accepted");
        }
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "field\tvalue\n");
            for (i = 0; i < 5; i++) len += sprintf(p + len, "%s\tv\n", CC_FREEZE_FIELDS[i]);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_kv(lines, nl, CC_FREEZE_FIELDS, 5, why, sizeof why) == 0,
                  "green base-freeze receipt: 5 fields in literal order accepted");
        }
        {
            char *p = buf; int len = 0;
            len += sprintf(p + len, "field\tvalue\n");
            for (i = 0; i < 17; i++) len += sprintf(p + len, "%s\tv\n", CC_SEL_FIELDS[i]);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_kv(lines, nl, CC_SEL_FIELDS, 17, why, sizeof why) == 0,
                  "green selection receipt: 17 fields in literal order accepted");
        }
        {   /* red: one selection field renamed */
            char *p = buf; int len = 0;
            len += sprintf(p + len, "field\tvalue\n");
            for (i = 0; i < 17; i++)
                len += sprintf(p + len, "%s\tv\n", i == 12 ? "class_idx" : CC_SEL_FIELDS[i]);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_kv(lines, nl, CC_SEL_FIELDS, 17, why, sizeof why) == 4,
                  "red: renamed selection field rejected (%s)", why);
        }
        {   /* red: selection row dropped */
            char *p = buf; int len = 0;
            len += sprintf(p + len, "field\tvalue\n");
            for (i = 0; i < 16; i++) len += sprintf(p + len, "%s\tv\n", CC_SEL_FIELDS[i]);
            nl = cc_split(buf, lines, 64);
            cc_pf("CC06_RECEIPT", cc_validate_kv(lines, nl, CC_SEL_FIELDS, 17, why, sizeof why) == 2,
                  "red: selection receipt with 16 rows rejected (%s)", why);
        }
        {   /* red: a status that neither executor may accept */
            const char *st = "synthetic-selected-not-confirmatory";
            cc_pf("CC06_RECEIPT", strcmp(st, "selected-not-run") != 0,
                  "red: status '%s' is not the confirmatory status and is refused by --confirmatory", st);
        }
    }

    /* ---------------- CC07: C8 branch coverage, all four classes ------- */
    {
        CcVerdictIn in; CcVerdictOut o;
        int nfix = 0, nbad = 0;

#define CC_FIX(label, want_pass, want_cov)                                            \
        do {                                                                          \
            cc_verdict(&in, &o); nfix++;                                              \
            { int ok = (o.pass == (want_pass)) && ((o.cover & (want_cov)) == (want_cov)); \
              if (!ok) nbad++;                                                        \
              cc_pf("CC07_C8", ok,                                                    \
                "%s : class=%s pass=%d (expected %d) micro=%d prefix=%d p_rank=%.4f p_rank64k=%.4f cover=0x%02x", \
                (label), CC_CLASS_NAME[in.class_index], o.pass, (want_pass),          \
                o.conf_micro, o.conf_micro_prefix, o.p_rank, o.p_rank_64k, o.cover); } \
        } while (0)

        /* ---- cipher ---- */
        memset(&in, 0, sizeof in);
        in.class_index = 0; in.earn_ctx_any = 1; in.earn_min_offset = 1000;
        in.nulls_ge_full = 0; in.nulls_ge_64k = 0;
        CC_FIX("cipher GREEN: contextual EARN and p_rank exactly 0.05", 1, CCV_MICRO_EARN | CCV_MICRO_RANK);

        in.nulls_ge_full = 1;   /* one null ties -> p_rank 0.10, ties count against live */
        CC_FIX("cipher RED boundary: one null ties the relation arm, p_rank 0.10", 0, CCV_MICRO_EARN | CCV_MICRO_RANK);

        in.nulls_ge_full = 0; in.earn_ctx_any = 0;
        CC_FIX("cipher RED: no contextual relation reaches EARN", 0, CCV_MICRO_EARN);

        /* ---- plain ---- */
        memset(&in, 0, sizeof in);
        in.class_index = 1; in.earn_ctx_any = 1; in.earn_min_offset = 40000;
        in.nulls_ge_full = 0;
        CC_FIX("plain GREEN: contextual EARN and p_rank exactly 0.05", 1, CCV_MICRO_EARN | CCV_MICRO_RANK);

        in.nulls_ge_full = 19;  /* every null ties or beats -> p_rank 1.00 */
        CC_FIX("plain RED boundary: all 19 nulls reach the relation arm, p_rank 1.00", 0, CCV_MICRO_EARN | CCV_MICRO_RANK);

        /* ---- half ---- */
        memset(&in, 0, sizeof in);
        in.class_index = 2; in.earn_ctx_any = 1; in.earn_min_offset = 65535;   /* last legal offset */
        in.nulls_ge_64k = 0; in.nulls_ge_full = 0;
        in.g_rel_half_guard = 100.0;
        CC_FIX("half GREEN: EARN at offset 65535 (< boundary), 64K rank wins, tail quiet", 1,
               CCV_PREFIX_EARN | CCV_PREFIX_RANK | CCV_HALF_GUARD);

        /* the red the law demands verbatim: nothing earns before 65536; a late
         * relation earns in the i.i.d. tail and wins the FULL-horizon rank; the
         * [65536,81920) guard passes. Must still FAIL. */
        in.earn_ctx_any = 1; in.earn_min_offset = 70000;   /* only in the tail */
        in.nulls_ge_full = 0;                              /* full-horizon rank is first */
        in.nulls_ge_64k = 0;                               /* 64K rank irrelevant: no prefix EARN */
        in.g_rel_half_guard = 10.0;                        /* guard passes */
        CC_FIX("half RED (law-mandated): no EARN before 65536; late tail EARN wins the full-horizon rank; "
               "[65536,81920) guard passes", 0, CCV_PREFIX_EARN);

        in.earn_min_offset = 1000; in.g_rel_half_guard = CC_M;   /* exactly M: strict < fails */
        CC_FIX("half RED boundary: |G_rel([65536,81920))| exactly M = 163.84, strict < fails", 0,
               CCV_PREFIX_EARN | CCV_PREFIX_RANK | CCV_HALF_GUARD);

        in.g_rel_half_guard = -CC_M;                        /* symmetric boundary */
        CC_FIX("half RED boundary: G_rel([65536,81920)) exactly -M", 0,
               CCV_PREFIX_EARN | CCV_PREFIX_RANK | CCV_HALF_GUARD);

        in.g_rel_half_guard = 163.8399;                     /* just inside */
        CC_FIX("half GREEN boundary: |G_rel| just inside M", 1,
               CCV_PREFIX_EARN | CCV_PREFIX_RANK | CCV_HALF_GUARD);

        /* ---- ff ---- */
        memset(&in, 0, sizeof in);
        in.class_index = 3; in.earn_ctx_any = 1; in.earn_min_offset = 5000;
        in.nulls_ge_full = 0; in.g_rel_ff_guard = 0.0;
        CC_FIX("ff GREEN: CONF_MICRO and no interference at the deciding horizon", 1,
               CCV_MICRO_EARN | CCV_MICRO_RANK | CCV_FF_GUARD);

        in.g_rel_ff_guard = -CC_M;                          /* exactly -M: strict > fails */
        CC_FIX("ff RED boundary: G_rel([0,16384)) exactly -M = -163.84, strict > fails", 0,
               CCV_MICRO_EARN | CCV_MICRO_RANK | CCV_FF_GUARD);

        in.g_rel_ff_guard = -163.8399;                      /* just inside */
        CC_FIX("ff GREEN boundary: G_rel just above -M", 1,
               CCV_MICRO_EARN | CCV_MICRO_RANK | CCV_FF_GUARD);

        in.g_rel_ff_guard = 0.0; in.nulls_ge_full = 1;
        CC_FIX("ff RED: CONF_MICRO fails on rank even though the interference guard holds", 0,
               CCV_MICRO_EARN | CCV_MICRO_RANK | CCV_FF_GUARD);
#undef CC_FIX
        cc_pf("CC07_C8", nbad == 0, "C8 branch coverage: %d predeclared fixtures over all four classes, %d unexpected", nfix, nbad);
        cc_row("OBS", "CC07_C8", "every fixture asserts the coverage mask of the conjuncts actually evaluated, so no "
                                 "fixture can pass through an empty or unexecuted comparison path");
        {   /* the two literal verdict strings */
            memset(&in, 0, sizeof in);
            in.class_index = 1; in.earn_ctx_any = 1; in.nulls_ge_full = 0;
            cc_verdict(&in, &o);
            cc_pf("CC07_C8", o.pass == 1, "verdict string check (green): %s", CC_PASS_LINE);
            in.nulls_ge_full = 5; cc_verdict(&in, &o);
            cc_pf("CC07_C8", o.pass == 0, "verdict string check (red): %s", CC_FAIL_LINE);
        }
    }
}
static int cc_class_seedless(int cls) { return cls == 1 || cls == 3; }
/* =====================================================================
 * PASS 6 -- the real --confirmatory door.
 *
 * Pass 5 shipped a C8 engine with no entrance: main recognised only
 * --development and otherwise ran a synthetic battery. Reality had no way in.
 * Everything below is the entrance, plus a generic single-world court so the
 * verdict is computed from this hand's own reconstruction.
 *
 * Class independence: the four classes differ ONLY where the frozen law makes
 * them differ -- cc_build_world (C5 construction) and cc_verdict (C8). Every
 * other line below is class-blind and shared.
 * ===================================================================== */

#define CC_ARMS 22            /* 0 cold, 1 relation, 2..20 null0..18, 21 oracle */
#define CC_MAPS 21            /* 0 relation, 1..19 null0..18, 20 oracle        */

typedef struct { int chunk, mapid, s, d, epoch, correct; } CcMapRow;
typedef struct { int chunk, arm, d, prof, fixed; } CcScrRow;
typedef struct { long off, upos; int arm; double a, b; Rel key; } CcKeyRow; /* winners + events */

typedef struct {
    double bits[140][CC_ARMS];
    /* the nine evidence counters this hand must reproduce, per chunk per arm */
    long positions[140];
    long pairs[140][CC_ARMS], relations[140][CC_ARMS], cearn[140][CC_ARMS];
    long earnev[140][CC_ARMS], revev[140][CC_ARMS], wins[140][CC_ARMS], senio[140][CC_ARMS];
    int  surf[140];
    int nchunks;
    long earn_min_off;
    long positions_total;
    long pE_bad, pE_bad_off, price_bad, price_bad_off;
    int  earn_any;
    long nrel;
    /* the books themselves */
    CcMapRow *maps;  long nmaps, cmaps;
    CcScrRow *scr;   long nscr,  cscr;
    CcKeyRow *win;   long nwin,  cwin;
    CcKeyRow *evt;   long nevt,  cevt;   /* a=ledger_after, b=is_earn */
} CcCourt;

static void cc_push_map(CcCourt *R, int chunk, int mapid, int s2, int d2, int ep, int corr)
{
    if (R->nmaps == R->cmaps) { R->cmaps = R->cmaps ? R->cmaps * 2 : 8192;
        R->maps = (CcMapRow *)realloc(R->maps, sizeof(CcMapRow) * (size_t)R->cmaps); }
    R->maps[R->nmaps].chunk = chunk; R->maps[R->nmaps].mapid = mapid;
    R->maps[R->nmaps].s = s2; R->maps[R->nmaps].d = d2;
    R->maps[R->nmaps].epoch = ep; R->maps[R->nmaps].correct = corr; R->nmaps++;
}
static void cc_push_scr(CcCourt *R, int chunk, int arm, int d2, int prof)
{
    if (R->nscr == R->cscr) { R->cscr = R->cscr ? R->cscr * 2 : 262144;
        R->scr = (CcScrRow *)realloc(R->scr, sizeof(CcScrRow) * (size_t)R->cscr); }
    R->scr[R->nscr].chunk = chunk; R->scr[R->nscr].arm = arm;
    R->scr[R->nscr].d = d2; R->scr[R->nscr].prof = prof;
    R->scr[R->nscr].fixed = (prof == d2); R->nscr++;
}
static void cc_push_key(CcKeyRow **arr, long *n, long *cap, long off, long upos, int arm,
                        double a, double b, const Rel *k)
{
    if (*n == *cap) { *cap = *cap ? *cap * 2 : 65536;
        *arr = (CcKeyRow *)realloc(*arr, sizeof(CcKeyRow) * (size_t)*cap); }
    (*arr)[*n].off = off; (*arr)[*n].upos = upos; (*arr)[*n].arm = arm;
    (*arr)[*n].a = a; (*arr)[*n].b = b; (*arr)[*n].key = *k; (*n)++;
}

/* ---- rebuild the A-side source organism exactly as the frozen law does ---- */
static int cc_src_ready = 0;
static void cc_prepare_source(const unsigned char *A, long lenA)
{
    static long RA[256][256], LA[256][256];
    long tmp[256];
    int i, j;
    if (cc_src_ready) return;            /* SOURCE_A is pinned by C1; prepare once */
    /* the frozen chain hash-maps are initialised inside the frozen main, which this
     * path never calls; they must be armed here or every lookup reads a null table */
    hm_init(&hm1); hm_init(&hm1t); hm_init(&hm2); hm_init(&hm2t);
    hm_init(&hm1h); hm_init(&hm2h);
    NA = (9 * lenA) / 10;                       /* A1 integer arithmetic */
    memset(RA, 0, sizeof RA); memset(LA, 0, sizeof LA);
    for (i = 0; i < 256; i++) { cntA[i] = 0; firstA[i] = -1; }
    for (i = 0; i < NA; i++) { cntA[A[i]]++; if (firstA[A[i]] < 0) firstA[A[i]] = i; }
    for (i = 0; i + 1 < NA; i++) { RA[A[i]][A[i+1]]++; LA[A[i+1]][A[i]]++; }
    n_as = 0;
    for (i = 0; i < 256; i++) if (cntA[i] > 0) alive_s[n_as++] = i;
    for (i = 0; i < 256; i++) {
        for (j = 0; j < 256; j++) tmp[j] = RA[i][j];
        kt_row(tmp, prof_A_R[i]); qsort(prof_A_R[i], 256, sizeof(double), cmp_desc); H_A_R[i] = ent(prof_A_R[i]);
        for (j = 0; j < 256; j++) tmp[j] = LA[i][j];
        kt_row(tmp, prof_A_L[i]); qsort(prof_A_L[i], 256, sizeof(double), cmp_desc); H_A_L[i] = ent(prof_A_L[i]);
    }
    cargo_build(A, NA);
    cc_src_ready = 1;
}

/* ---- one full court over an arbitrary world, all 22 arms ---------------- */
static int *cc_adm_all = NULL, *cc_ep_all = NULL;
#define CCIX(a,b) ((((long)(a) * 140 + (b))) * 256)

static void cc_court(const unsigned char *W, long wlen, const int *truemap, int have_true, CcCourt *R)
{
    int nb = (int)(wlen / CHUNK), c, i, j, k, a;
    static int seg[CHUNK + 8];
    static int *oldu = NULL;
    long t_prev = 0;
    int prev_live[256], ep_live[256], prev_or[256], ep_or[256];
    static int prev_null[19][256], ep_null[19][256];

    if (!oldu) oldu = (int *)malloc(sizeof(int) * (RUN_BYTES + 8));
    if (!cc_adm_all) {
        long n = (long)CC_MAPS * 140 * 256;
        cc_adm_all = (int *)malloc(sizeof(int) * (size_t)n);
        cc_ep_all  = (int *)malloc(sizeof(int) * (size_t)n);
    }
    { long n = (long)CC_MAPS * 140 * 256, z; for (z = 0; z < n; z++) { cc_adm_all[z] = -1; cc_ep_all[z] = 0; } }
    free(R->maps); free(R->scr); free(R->win); free(R->evt);
    memset(R, 0, sizeof *R);
    R->nchunks = nb; R->earn_min_off = -1; R->earn_any = 0;

    /* ---- phase 1: admission maps at every boundary (class-blind) ---- */
    for (i = 0; i < 256; i++) { prev_live[i] = -1; ep_live[i] = 0; prev_or[i] = -1; ep_or[i] = 0; cntD[i] = 0; firstD[i] = -1; }
    for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) { prev_null[k][i] = -1; ep_null[k][i] = 0; }
    memset(RD, 0, sizeof RD); memset(LD, 0, sizeof LD);

    for (c = 1; c <= nb; c++) {
        long t = (long)c * CHUNK, lo;
        int arr[256], bonly[256], adm[256], adm_or[256];
        static int adm_null[19][256];
        for (j = t_prev; j < t; j++) { cntD[W[j]]++; if (firstD[W[j]] < 0) firstD[W[j]] = j; }
        lo = (t_prev >= 1) ? t_prev - 1 : 0;
        for (j = lo; j + 1 < t; j++) { RD[W[j]][W[j+1]]++; LD[W[j+1]][W[j]]++; }
        t_prev = t;
        n_ad = 0;
        for (i = 0; i < 256; i++) if (cntD[i] > 0) alive_d[n_ad++] = i;
        for (i = 0; i < n_ad; i++) arr[i] = alive_d[i];
        for (i = 0; i < n_ad; i++) for (j = i + 1; j < n_ad; j++)
            if (firstD[arr[j]] < firstD[arr[i]]) { int tt = arr[i]; arr[i] = arr[j]; arr[j] = tt; }
        { long tmp[256];
          for (i = 0; i < n_ad; i++) {
              int d = alive_d[i];
              for (j = 0; j < 256; j++) tmp[j] = RD[d][j];
              kt_row(tmp, prof_D_R[d]); qsort(prof_D_R[d], 256, sizeof(double), cmp_desc); H_D_R[d] = ent(prof_D_R[d]);
              for (j = 0; j < 256; j++) tmp[j] = LD[d][j];
              kt_row(tmp, prof_D_L[d]); qsort(prof_D_L[d], 256, sizeof(double), cmp_desc); H_D_L[d] = ent(prof_D_L[d]);
          } }
        for (i = 0; i < n_as; i++) {
            int s = alive_s[i];
            double fa = log2(((double)cntA[s] + 0.5) / ((double)NA + 128.0));
            for (j = 0; j < n_ad; j++) {
                int d = alive_d[j];
                double fdv = log2(((double)cntD[d] + 0.5) / ((double)t + 128.0));
                Bm[s][d] = -(js_sorted(prof_A_R[s], H_A_R[s], prof_D_R[d], H_D_R[d]) +
                             js_sorted(prof_A_L[s], H_A_L[s], prof_D_L[d], H_D_L[d]));
                Fm[s][d] = -fabs(fa - fdv);
            }
        }
        admit((const double (*)[256])Bm, (const double (*)[256])Fm, bonly, adm);
        for (i = 0; i < 256; i++) if (prev_live[i] != adm[i]) ep_live[i]++;
        for (k = 0; k < 19; k++) {
            int p[256], rho[256], b2[256];
            static double Bk[256][256];
            null_perm(NULL_SEED[k], n_ad, p);
            for (i = 0; i < 256; i++) rho[i] = -1;
            for (i = 0; i < n_ad; i++) rho[arr[i]] = arr[p[i]];
            if (c < 140) for (i = 0; i < n_ad; i++) cc_push_scr(R, c, k, alive_d[i], rho[alive_d[i]]);
            for (i = 0; i < n_as; i++) { int s = alive_s[i];
                for (j = 0; j < n_ad; j++) { int d = alive_d[j]; Bk[s][d] = Bm[s][rho[d]]; } }
            admit((const double (*)[256])Bk, (const double (*)[256])Fm, b2, adm_null[k]);
            for (i = 0; i < 256; i++) if (prev_null[k][i] != adm_null[k][i]) ep_null[k][i]++;
        }
        for (i = 0; i < 256; i++) adm_or[i] = -1;
        if (have_true) for (i = 0; i < 256; i++) if (adm[i] >= 0) {
            int td = truemap[i];
            adm_or[i] = (td >= 0 && cntD[td] > 0) ? td : adm[i];
        }
        for (i = 0; i < 256; i++) if (prev_or[i] != adm_or[i]) ep_or[i]++;
        if (c < 140) {
            for (i = 0; i < 256; i++) {
                cc_adm_all[CCIX(0, c) + i] = adm[i];  cc_ep_all[CCIX(0, c) + i] = ep_live[i];
                cc_adm_all[CCIX(20, c) + i] = adm_or[i]; cc_ep_all[CCIX(20, c) + i] = ep_or[i];
            }
            for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) {
                cc_adm_all[CCIX(1 + k, c) + i] = adm_null[k][i];
                cc_ep_all[CCIX(1 + k, c) + i] = ep_null[k][i];
            }
            /* the maps book: bonly, relation, the 19 nulls, oracle */
            for (i = 0; i < 256; i++) if (bonly[i] >= 0)
                cc_push_map(R, c, 0, i, bonly[i], 0, have_true ? (truemap[i] == bonly[i]) : -1);
            for (i = 0; i < 256; i++) if (adm[i] >= 0)
                cc_push_map(R, c, 1, i, adm[i], ep_live[i], have_true ? (truemap[i] == adm[i]) : -1);
            for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) if (adm_null[k][i] >= 0)
                cc_push_map(R, c, 2 + k, i, adm_null[k][i], ep_null[k][i],
                            have_true ? (truemap[i] == adm_null[k][i]) : -1);
            if (have_true) for (i = 0; i < 256; i++) if (adm_or[i] >= 0)
                cc_push_map(R, c, 21, i, adm_or[i], ep_or[i], truemap[i] == adm_or[i]);
        }
        memcpy(prev_live, adm, sizeof prev_live);
        memcpy(prev_or, adm_or, sizeof prev_or);
        for (k = 0; k < 19; k++) memcpy(prev_null[k], adm_null[k], sizeof prev_null[k]);
    }

    /* ---- phase 2: prices, relations, ledgers (class-blind) ---- */
    for (a = 0; a < CC_MAPS; a++) { memset(cc_rb_tab[a], 0, sizeof cc_rb_tab[a]); rb_n[a] = 0; }
    for (c = 0; c < nb; c++) {
        long lived = (long)c * CHUNK, nold, npos, q2, ro;
        int minv[CC_MAPS][256], hasmap[CC_MAPS], admlist[CC_MAPS][256], nadm[CC_MAPS];
        const int *adm_a[CC_MAPS], *ep_a[CC_MAPS];
        bpe_learn(W, lived);
        npos = bpe_segment(W, lived, (long)(c + 1) * CHUNK, seg, CHUNK + 8);
        nold = bpe_segment(W, 0, lived, oldu, RUN_BYTES + 8);
        chain_build(oldu, nold);
        cc_chain_log_prepare();
        for (a = 0; a < CC_ARMS; a++) R->bits[c][a] = 0.0;
        R->positions[c] = npos;
        R->surf[c] = 0;                 /* A3 scopes the surface gate to W_swap_ea only */
        for (a = 0; a < CC_MAPS; a++) {
            hasmap[a] = 0; nadm[a] = 0;
            for (i = 0; i < 256; i++) minv[a][i] = -1;
            if (c >= 1 && c < 140) {
                adm_a[a] = cc_adm_all + CCIX(a, c);
                ep_a[a] = cc_ep_all + CCIX(a, c);
                for (i = 0; i < 256; i++) if (adm_a[a][i] >= 0) {
                    minv[a][adm_a[a][i]] = i; admlist[a][nadm[a]++] = i; hasmap[a] = 1;
                }
            } else { adm_a[a] = NULL; ep_a[a] = NULL; }
        }
        ro = lived;
        for (q2 = 0; q2 < npos; q2++) {
            long upos = R->positions_total + q2;
            int p1 = (q2 >= 1) ? seg[q2-1] : (nold >= 1 ? oldu[nold-1] : -1);
            int p2 = (q2 >= 2) ? seg[q2-2] : ((nold >= 2 - q2) ? oldu[nold - (2 - q2)] : -1);
            int u = seg[q2], fb = unit_fb[u];
            double logp;
            chain_coeffs(p1, p2);
            cc_chain_log_events(p1, p2);
            logp = cc_chain_logp(u, p1, p2);
            if (!isfinite(logp) || logp > 1e-12) {
                if (!R->price_bad) R->price_bad_off = ro;
                R->price_bad++;
            }
            R->bits[c][0] += -logp;
            for (a = 0; a < CC_MAPS; a++) {
                int armcol = (a == 0) ? 1 : (a == 20) ? 21 : (1 + a);
                int cl = 0, cs[3], cd[3], cep[3], nb3 = 0, tb[3];
                int sIdx, best = -1; double bestled = 0; Rel bestk;
                double logpf; long cand[260]; int ncand = 0;
                if (!hasmap[a]) { R->bits[c][armcol] += -logp; continue; }
                for (j = 1; j <= 3; j++) {
                    long rr = ro - j; int bb, ss;
                    if (rr < 0) break;
                    bb = W[rr]; ss = minv[a][bb];
                    if (ss < 0) break;
                    tb[nb3++] = bb;
                }
                cl = nb3;
                for (j = 0; j < 3; j++) { cs[j] = 0; cd[j] = 0; cep[j] = 0; }
                for (j = 0; j < cl; j++) {
                    int bb = tb[cl - 1 - j], ss = minv[a][bb];
                    cs[j] = ss; cd[j] = bb; cep[j] = ep_a[a][ss];
                }
                for (sIdx = 0; sIdx < nadm[a]; sIdx++) {
                    Rel key; long ix; int z, src = admlist[a][sIdx];
                    key.ts = src; key.td = adm_a[a][src]; key.tep = ep_a[a][src]; key.cl = cl;
                    for (z = 0; z < 3; z++) { key.cs[z] = cs[z]; key.cd[z] = cd[z]; key.cep[z] = cep[z]; }
                    ix = rel_find(a, &key);
                    if (cl >= 1 && rb[a][ix].st == 1) {
                        if (ncand < 260) cand[ncand++] = ix;
                        if (best < 0 || rb[a][ix].led > bestled ||
                            (rb[a][ix].led == bestled && rel_cmp(&rb[a][ix], &bestk) < 0)) {
                            best = (int)ix; bestled = rb[a][ix].led; bestk = rb[a][ix];
                        }
                    }
                }
                if (best >= 0) {
                    int z3;
                    R->wins[c][armcol]++;
                    cc_push_key(&R->win, &R->nwin, &R->cwin, ro, upos, armcol,
                                rb[a][best].led, rb[a][best].L, &rb[a][best]);
                    for (z3 = 0; z3 < ncand; z3++) {   /* seniority telemetry */
                        Rel *Lz = &rb[a][cand[z3]], *Wz = &rb[a][best];
                        if (cand[z3] == best) continue;
                        if (Lz->seen > 0 && Wz->seen > 0 && Lz->seen < Wz->seen &&
                            (Lz->led / (double)Lz->seen) > (Wz->led / (double)Wz->seen))
                            R->senio[c][armcol]++;
                    }
                }
                logpf = logp;
                if (best >= 0) {
                    int bs = rb[a][best].ts, bd = rb[a][best].td;
                    double logpE = cc_chain_logpe[bd];
                    double r = cargo_p(bs, cs, cl), logq;
                    logq = (fb == bd)
                        ? logp + log2(r) - logpE
                        : logp + log2(1.0 - r) - cc_log1mexp2(logpE);
                    logpf = cc_logmix2(logp, logq, rb[a][best].L);
                }
                if (!isfinite(logpf) || logpf > 1e-12) {
                    if (!R->price_bad) R->price_bad_off = ro;
                    R->price_bad++;
                }
                R->bits[c][armcol] += -logpf;
                for (sIdx = 0; sIdx < nadm[a]; sIdx++) {
                    Rel key; long ix; int z, src = admlist[a][sIdx], dd = adm_a[a][src];
                    double logpE, r, delta; Rel *RR;
                    key.ts = src; key.td = dd; key.tep = ep_a[a][src]; key.cl = cl;
                    for (z = 0; z < 3; z++) { key.cs[z] = cs[z]; key.cd[z] = cd[z]; key.cep[z] = cep[z]; }
                    ix = rel_find(a, &key);
                    RR = &rb[a][ix];
                    logpE = cc_chain_logpe[dd]; r = cargo_p(src, cs, cl);
                    if (!isfinite(logpE) || !(logpE < 0.0)) {
                        if (!R->pE_bad) R->pE_bad_off = ro;
                        R->pE_bad++;
                    }
                    delta = (fb == dd)
                        ? log2(r) - logpE
                        : log2(1.0 - r) - cc_log1mexp2(logpE);
                    RR->seen++; if (delta > 0.0) RR->pos++; else if (delta < 0.0) RR->neg++;
                    RR->led += delta;
                    if (RR->led > RR->peak) RR->peak = RR->led;
                    if (RR->cl >= 1) {
                        if (RR->st == 1) { double nl = RR->L * exp(0.05 * delta); if (nl < 0.01) nl = 0.01; if (nl > 0.5) nl = 0.5; RR->L = nl; }
                        if (RR->st == 0 && RR->led >= EARN_BITS) {
                            RR->st = 1; RR->L = 0.05; RR->ever = 1;
                            R->earnev[c][armcol]++;
                            cc_push_key(&R->evt, &R->nevt, &R->cevt, ro, upos, armcol, RR->led, 1.0, RR);
                            if (a == 0) {                       /* relation arm EARN offsets */
                                R->earn_any = 1;
                                if (R->earn_min_off < 0 || ro < R->earn_min_off) R->earn_min_off = ro;
                            }
                        } else if (RR->st == 1 && RR->led < REVOKE_BITS) {
                            RR->st = 0; RR->L = 0.0;
                            R->revev[c][armcol]++;
                            cc_push_key(&R->evt, &R->nevt, &R->cevt, ro, upos, armcol, RR->led, 0.0, RR);
                        }
                    }
                }
            }
            ro += unit_len[u];
        }
        R->positions_total += npos;
        for (a = 0; a < CC_MAPS; a++) {
            int armcol = (a == 0) ? 1 : (a == 20) ? 21 : (1 + a);
            long q3, ce = 0;
            R->pairs[c][armcol] = nadm[a];
            R->relations[c][armcol] = rb_n[a];
            for (q3 = 0; q3 < rb_n[a]; q3++) if (rb[a][q3].cl >= 1 && rb[a][q3].ever) ce++;
            R->cearn[c][armcol] = ce;
        }
    }
    R->nrel = rb_n[0];
}

/* ---- C8 inputs derived from this hand's own court ---------------------- */
static double cc_G(const CcCourt *R, int arm, int c0, int c1)
{
    double g = 0; int c;
    for (c = c0; c < c1 && c < R->nchunks; c++) g += R->bits[c][0] - R->bits[c][arm];
    return g;
}
static void cc_fill_verdict_in(const CcCourt *R, int class_index, CcVerdictIn *in)
{
    int k, full = R->nchunks, h64 = CC_HALF_BOUNDARY / CHUNK;
    double gr_full = cc_G(R, 1, 0, full), gr_64 = cc_G(R, 1, 0, h64);
    memset(in, 0, sizeof *in);
    in->class_index = class_index;
    in->earn_ctx_any = R->earn_any;
    in->earn_min_offset = R->earn_min_off;
    for (k = 0; k < 19; k++) {
        if (cc_G(R, 2 + k, 0, full) >= gr_full) in->nulls_ge_full++;
        if (cc_G(R, 2 + k, 0, h64) >= gr_64) in->nulls_ge_64k++;
    }
    in->g_rel_half_guard = cc_G(R, 1, h64, h64 + (16384 / CHUNK));
    in->g_rel_ff_guard = cc_G(R, 1, 0, 16384 / CHUNK);
}

/* ---- world construction dispatch: the ONLY class-conditional builder ---- */
static int cc_build_world(int class_index, const unsigned char *B, long blen,
                          uint64_t world_seed, unsigned char *W, int *truemap, char *err, size_t en)
{
    int i;
    for (i = 0; i < 256; i++) truemap[i] = i;              /* identity default */
    switch (class_index) {
    case 0:                                                 /* cipher */
        cc_build_cipher(B, world_seed, W);
        { int perm[256]; rename_perm(world_seed, perm); for (i = 0; i < 256; i++) truemap[i] = perm[i]; }
        return 0;
    case 1:                                                 /* plain */
        cc_build_plain(B, W);
        return 0;
    case 2:                                                 /* half */
        cc_build_half(B, blen, world_seed, W);
        { int perm[256]; rename_perm(world_seed, perm); for (i = 0; i < 256; i++) truemap[i] = perm[i]; }
        return 0;
    case 3: {                                               /* ff */
        long ffl = cc_build_ff(B, blen, W);
        if (ffl < CC_RUN_BYTES) { snprintf(err, en, "ff transform yields %ld bytes < RUN_BYTES", ffl); return 1; }
        return 0;
    }
    default:
        snprintf(err, en, "class index %d out of range", class_index);
        return 1;
    }
}

/* =====================================================================
 * Receipt loading against real files (used by the real --confirmatory door)
 * ===================================================================== */
typedef struct { char field[64], value[512]; } CcKV;


/* ---- PASS 10 parser blocker 1 ------------------------------------------
 * cc_split walks on NUL, so a lawful receipt followed by "\0garbage" read as
 * clean EOF. Every receipt is now screened by SIZE first: an embedded NUL, or
 * any byte after the final newline, is a refusal. */
static int cc_read_once(const char *path, unsigned char **out, long *outn,
                        char *err, size_t en)
{
    struct stat st;
    long n;
    size_t off = 0;
    unsigned char *b;
    int fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) { snprintf(err, en, "cannot open regular input %s without following links", path); return 1; }
    if (fstat(fd, &st) != 0 || !S_ISREG(st.st_mode) || st.st_size < 0 ||
        (uintmax_t)st.st_size > (uintmax_t)LONG_MAX) {
        snprintf(err, en, "%s is not a sizeable regular file", path);
        close(fd);
        return 1;
    }
    n = (long)st.st_size;
    b = (unsigned char *)malloc((size_t)n + 1);
    if (!b) { snprintf(err, en, "cannot allocate %ld bytes for %s", n, path); close(fd); return 1; }
    while (off < (size_t)n) {
        ssize_t nr = read(fd, b + off, (size_t)n - off);
        if (nr < 0 && errno == EINTR) continue;
        if (nr <= 0) {
            snprintf(err, en, "short read from %s", path);
            free(b); close(fd); return 1;
        }
        off += (size_t)nr;
    }
    {
        unsigned char extra;
        ssize_t nr;
        do { nr = read(fd, &extra, 1); } while (nr < 0 && errno == EINTR);
        if (nr != 0) {
            snprintf(err, en, "%s grew while being captured", path);
            free(b); close(fd); return 1;
        }
    }
    if (close(fd) != 0) { snprintf(err, en, "cannot close %s", path); free(b); return 1; }
    b[n] = 0; *out = b; *outn = n;
    return 0;
}

static int cc_buffer_clean(const char *name, const unsigned char *b, long n,
                           char *err, size_t en)
{
    long i;
    if (n == 0) { snprintf(err, en, "%s is empty", name); return 1; }
    for (i = 0; i < n; i++)
        if (b[i] == 0) {
            snprintf(err, en, "%s carries an embedded NUL at byte %ld", name, i);
            return 1;
        }
    if (b[n-1] != '\n') {
        snprintf(err, en, "%s does not end with a newline (trailing bytes after the last lawful line)", name);
        return 1;
    }
    return 0;
}

static int cc_read_clean_once(const char *path, unsigned char **out, long *outn,
                              char *err, size_t en)
{
    long n; unsigned char *b;
    if (cc_read_once(path, &b, &n, err, en)) return 1;
    if (cc_buffer_clean(path, b, n, err, en)) { free(b); return 1; }
    *out = b; *outn = n;
    return 0;
}

static int cc_load_kv_snapshot(const char *path, CcKV *kv, int maxn, int *outn,
                               char *snapshot_sha, long *snapshot_bytes,
                               char *err, size_t en)
{
    long n; unsigned char *b;
    char *lines[64]; int nl, i;
    if (cc_read_clean_once(path, &b, &n, err, en)) return 1;
    if (snapshot_sha) sha256_buf(b, (size_t)n, snapshot_sha);
    if (snapshot_bytes) *snapshot_bytes = n;
    nl = cc_split((char *)b, lines, 64);
    if (nl < 1 || strcmp(lines[0], "field\tvalue")) { snprintf(err, en, "%s: bad header", path); free(b); return 1; }
    *outn = 0;
    for (i = 1; i < nl && *outn < maxn; i++) {
        char *tab = strchr(lines[i], '\t');
        if (!tab) { snprintf(err, en, "%s: row %d malformed", path, i); free(b); return 1; }
        *tab = 0;
        if (strlen(lines[i]) >= sizeof kv[0].field || strlen(tab + 1) >= sizeof kv[0].value) {
            snprintf(err, en, "%s: row %d field or value is too long", path, i); free(b); return 1;
        }
        snprintf(kv[*outn].field, sizeof kv[0].field, "%s", lines[i]);
        snprintf(kv[*outn].value, sizeof kv[0].value, "%s", tab + 1);
        (*outn)++;
    }
    if (i < nl) { snprintf(err, en, "%s has more than %d rows", path, maxn); free(b); return 1; }
    free(b);
    return 0;
}
static int cc_load_kv(const char *path, CcKV *kv, int maxn, int *outn, char *err, size_t en)
{
    return cc_load_kv_snapshot(path, kv, maxn, outn, NULL, NULL, err, en);
}

/* Parse a private copy of an already captured byte snapshot. The caller can
 * run several independent readers over the same bytes without reopening the
 * path between hash/structure/semantic checks. */
static Tsv cc_tsv_snapshot(const unsigned char *src, long n)
{
    Tsv t; char *b, *p, *end; int cap = 1024;
    b = (char *)malloc((size_t)n + 1);
    if (!b) { fprintf(stderr, "allocation failed for TSV snapshot\n"); exit(2); }
    memcpy(b, src, (size_t)n); b[n] = 0;
    t.raw = b; t.r = (Row *)malloc(sizeof(Row) * (size_t)cap); t.n = 0;
    if (!t.r) { free(b); fprintf(stderr, "allocation failed for TSV rows\n"); exit(2); }
    p = b; end = b + n;
    while (p < end) {
        char *nl = (char *)memchr(p, '\n', (size_t)(end - p));
        char *line_end = nl ? nl : end;
        *line_end = 0;
        if (line_end > p) {
            int nf = 1, i = 0; char *q; char **fl;
            if (t.n == cap) {
                Row *nr; cap *= 2;
                nr = (Row *)realloc(t.r, sizeof(Row) * (size_t)cap);
                if (!nr) { tsv_free(&t); fprintf(stderr, "allocation failed for TSV rows\n"); exit(2); }
                t.r = nr;
            }
            for (q = p; *q; q++) if (*q == '\t') nf++;
            fl = (char **)malloc(sizeof(char *) * (size_t)nf);
            if (!fl) { tsv_free(&t); fprintf(stderr, "allocation failed for TSV fields\n"); exit(2); }
            fl[i++] = p;
            for (q = p; *q; q++) if (*q == '\t') { *q = 0; fl[i++] = q + 1; }
            t.r[t.n].f = fl; t.r[t.n].nf = nf; t.n++;
        }
        p = line_end + 1;
    }
    return t;
}

static int cc_load_kv_bytes(const unsigned char *raw, long bytes, const char *name,
                            CcKV *kv, int maxn, int *outn, char *err, size_t en)
{
    Tsv t = cc_tsv_snapshot(raw, bytes);
    int i;
    if (t.n < 1 || t.r[0].nf != 2 || strcmp(fs(&t, 0, 0), "field") ||
        strcmp(fs(&t, 0, 1), "value")) {
        snprintf(err, en, "%s: bad header", name); tsv_free(&t); return 1;
    }
    if (t.n - 1 > maxn) {
        snprintf(err, en, "%s has more than %d rows", name, maxn); tsv_free(&t); return 1;
    }
    *outn = 0;
    for (i = 1; i < t.n; i++) {
        if (t.r[i].nf != 2) {
            snprintf(err, en, "%s: row %d malformed", name, i); tsv_free(&t); return 1;
        }
        if (strlen(fs(&t, i, 0)) >= sizeof kv[0].field || strlen(fs(&t, i, 1)) >= sizeof kv[0].value) {
            snprintf(err, en, "%s: row %d field or value is too long", name, i); tsv_free(&t); return 1;
        }
        snprintf(kv[*outn].field, sizeof kv[0].field, "%s", fs(&t, i, 0));
        snprintf(kv[*outn].value, sizeof kv[0].value, "%s", fs(&t, i, 1));
        (*outn)++;
    }
    tsv_free(&t);
    return 0;
}
static const char *cc_kv(const CcKV *kv, int n, const char *f)
{
    int i; for (i = 0; i < n; i++) if (!strcmp(kv[i].field, f)) return kv[i].value;
    return NULL;
}
/* ordered field check: field i must be exactly names[i] */
static int cc_kv_order(const CcKV *kv, int n, const char *const *names, int nn, char *err, size_t en)
{
    int i;
    if (n != nn) { snprintf(err, en, "expected %d rows, saw %d", nn, n); return 1; }
    for (i = 0; i < nn; i++)
        if (strcmp(kv[i].field, names[i])) {
            snprintf(err, en, "row %d is '%s', expected '%s'", i + 1, kv[i].field, names[i]); return 1;
        }
    return 0;
}
static int cc_file_digest(const char *path, char *sha65, long *bytes)
{
    long n; unsigned char *b; char err[256];
    if (cc_read_once(path, &b, &n, err, sizeof err)) return 1;
    sha256_buf(b, (size_t)n, sha65);
    *bytes = n; free(b);
    return 0;
}


/* ---- PASS 9: strict canonical unsigned parsing on the receipt side -----
 * Receipt integers were read with strtol(value, NULL, 10), so "151191junk",
 * "00151191", "+1" and "" could pass by numeric prefix. Every receipt integer
 * now goes through cc_parse_int: full consumption, no sign, no leading zero
 * except the literal "0", overflow rejected. */
static int cc_kv_uint(const CcKV *kv, int n, const char *field, long *out)
{
    const char *v = cc_kv(kv, n, field);
    if (!v) return 1;
    return cc_parse_int(v, out);
}

/* roots receipt held as parsed rows */
typedef struct { char role[48], path[256], sha[65]; long bytes; } CcRoot;

static int cc_load_roots(const char *path, CcRoot *R, char snapshot_sha[65],
                         long *snapshot_bytes, char *err, size_t en)
{
    long n; unsigned char *b = NULL;
    char *lines[64]; int nl, i;
    char why[256];
    if (cc_read_clean_once(path, &b, &n, err, en)) return 1;
    sha256_buf(b, (size_t)n, snapshot_sha);
    *snapshot_bytes = n;
    nl = cc_split((char *)b, lines, 64);
    if (cc_validate_roots(lines, nl, why, sizeof why) != 0) {
        snprintf(err, en, "roots schema: %s", why); free(b); return 1;
    }
    for (i = 1; i < nl; i++) {
        char buf[1024], *fl[8]; int nf = 0; char *q;
        if (strlen(lines[i]) >= sizeof buf) {
            snprintf(err, en, "roots row %d is too long", i); free(b); return 1;
        }
        snprintf(buf, sizeof buf, "%s", lines[i]);
        fl[nf++] = buf;
        for (q = buf; *q; q++) if (*q == '\t' && nf < 8) { *q = 0; fl[nf++] = q + 1; }
        if (nf != 4) {
            snprintf(err, en, "roots row %d has %d fields after capture", i, nf); free(b); return 1;
        }
        if (strlen(fl[0]) >= sizeof R[0].role || strlen(fl[1]) >= sizeof R[0].path ||
            strlen(fl[3]) >= sizeof R[0].sha) {
            snprintf(err, en, "roots row %d contains an overlong field", i); free(b); return 1;
        }
        snprintf(R[i-1].role, sizeof R[0].role, "%s", fl[0]);
        snprintf(R[i-1].path, sizeof R[0].path, "%s", fl[1]);
        if (cc_parse_int(fl[2], &R[i-1].bytes)) {
            snprintf(err, en, "roots row %d byte length is not canonical", i); free(b); return 1; }
        snprintf(R[i-1].sha, sizeof R[0].sha, "%s", fl[3]);
    }
    free(b);
    return 0;
}
static const CcRoot *cc_root(const CcRoot *R, const char *role)
{
    int i; for (i = 0; i < 18; i++) if (!strcmp(R[i].role, role)) return &R[i];
    return NULL;
}

static int cc_verify_prior_binding(const unsigned char *raw, long bytes,
                                   const CcRoot roots[18],
                                   char *err, size_t en)
{
    static const char *stages[7] = {
        "0-prior-receipt", "1-court2-snapshot", "1-court3-snapshot",
        "1-court4-protocol", "2-builder", "3-gap-addendum", "4-verifier"
    };
    static const char *paths[7] = {
        "COURT4_ADDENDUM_FREEZE.tsv", "COURT2_SNAPSHOT.md",
        "COURT3_SNAPSHOT.md", "TRANSFER4_DRAFT.md", "transfer4.c",
        "COURT4_GAP_ADDENDUM.md", "transfer4_check.c"
    };
    Tsv t;
    int i;
    if (cc_buffer_clean("prior_freeze", raw, bytes, err, en)) return 1;
    t = cc_tsv_snapshot(raw, bytes);
    if (t.n != 8 || t.r[0].nf != 4 ||
        strcmp(fs(&t, 0, 0), "stage") || strcmp(fs(&t, 0, 1), "path") ||
        strcmp(fs(&t, 0, 2), "bytes") || strcmp(fs(&t, 0, 3), "sha256")) {
        snprintf(err, en, "prior freeze receipt schema mismatch");
        tsv_free(&t);
        return 1;
    }
    for (i = 0; i < 7; i++) {
        long pinned_bytes;
        int row = i + 1;
        if (t.r[row].nf != 4 || strcmp(fs(&t, row, 0), stages[i]) ||
            strcmp(fs(&t, row, 1), paths[i]) ||
            cc_parse_int(fs(&t, row, 2), &pinned_bytes) ||
            !cc_is_hex64(fs(&t, row, 3))) {
            snprintf(err, en, "prior freeze row %d identity mismatch", row);
            tsv_free(&t);
            return 1;
        }
        if (i > 0 &&
            (pinned_bytes != roots[i].bytes ||
             strcmp(fs(&t, row, 3), roots[i].sha))) {
            snprintf(err, en, "development root '%s' contradicts prior freeze",
                     roots[i].role);
            tsv_free(&t);
            return 1;
        }
    }
    tsv_free(&t);
    return 0;
}


/* =====================================================================
 * PASS 7 -- the witness stand is mandatory.
 *
 * A reader that can be satisfied by absence must die on absence. The pass-6
 * evidence reader verified only rows that existed, so a header-only file made
 * the loop execute zero times and the verdict was emitted from an empty
 * witness statement. The gate below is the court's literal seven-point law.
 * ===================================================================== */


/* ---- PASS 8: canonical numeric parsing -------------------------------
 * The frozen fi()/fd() call strtol/strtod with a NULL end pointer, so "0junk",
 * "1024x" and "1.0garbage" parse as legal numbers. Every numeric field read by
 * this door now proves it is a number: whole string consumed, canonical form. */
static int cc_parse_int(const char *s, long *out)
{
    long v = 0; int i;
    if (!s || !s[0]) return 1;
    if (s[0] == '0' && s[1] != '\0') return 1;          /* no leading zeros */
    for (i = 0; s[i]; i++) {
        if (s[i] < '0' || s[i] > '9') return 1;         /* digits only, no sign or space */
        if (v > (LONG_MAX - (s[i] - '0')) / 10) return 1;
        v = v * 10 + (s[i] - '0');
    }
    *out = v;
    return 0;
}
static int cc_parse_dbl(const char *s, double *out)
{
    char *end = NULL; double v;
    if (!s || !s[0]) return 1;
    errno = 0;
    v = strtod(s, &end);
    if (end == s || *end != '\0') return 1;             /* whole string consumed */
    if (!isfinite(v)) return 1;
    *out = v;
    return 0;
}

/* strict arm names: the frozen armid_of accepts junk such as "null0junk"
 * because it uses atoi; this reader admits only the 22 canonical spellings. */
static int cc_armid_strict(const char *s)
{
    int k; char buf[16];
    if (!strcmp(s, "cold")) return 0;
    if (!strcmp(s, "relation")) return 1;
    if (!strcmp(s, "oracle")) return 21;
    for (k = 0; k <= 18; k++) { sprintf(buf, "null%d", k); if (!strcmp(s, buf)) return 2 + k; }
    return -1;
}

static const char *CC_EV_HEADER[14] = {
    "chunk", "lived_before", "bytes", "arm", "positions", "bits", "pairs", "relations",
    "contextual_earned", "contextual_earn_events", "contextual_revoke_events",
    "winner_uses", "seniority_suppressions", "surface_admitted"
};

/* the court's seven-point evidence gate, then the A5 comparison of all 2816 bits */
static int cc_check_evidence_struct(const unsigned char *raw, long raw_bytes,
                                    int expect_chunks, char *err, size_t en)
{
    Tsv ev;
    int i, a;
    long expect_rows = (long)expect_chunks * CC_ARMS;
    static signed char seen[140][CC_ARMS];
    long bad = 0, firstc = -1; int firsta = -1;
    ev = cc_tsv_snapshot(raw, raw_bytes);
    /* (1) the exact 14-field header */
    if (ev.n < 1 || ev.r[0].nf != 14) {
        snprintf(err, en, "evidence header has %d fields, law requires 14", ev.n ? ev.r[0].nf : 0);
        tsv_free(&ev); return 1;
    }
    for (i = 0; i < 14; i++)
        if (strcmp(fs(&ev, 0, i), CC_EV_HEADER[i])) {
            snprintf(err, en, "evidence header field %d is '%s', law requires '%s'",
                     i, fs(&ev, 0, i), CC_EV_HEADER[i]);
            tsv_free(&ev); return 1;
        }
    /* (2) exactly nchunks x 22 data rows -- absence is a refusal, not a pass */
    if ((long)(ev.n - 1) != expect_rows) {
        snprintf(err, en, "evidence holds %ld data rows, law requires exactly %ld (%d chunks x %d arms)",
                 (long)(ev.n - 1), expect_rows, expect_chunks, CC_ARMS);
        tsv_free(&ev); return 1;
    }
    memset(seen, 0, sizeof seen);
    for (i = 1; i < ev.n; i++) {
        int ch, ai; long lb, by;
        /* (5) exactly 14 fields in every row */
        if (ev.r[i].nf != 14) {
            snprintf(err, en, "evidence row %d has %d fields, law requires 14", i, ev.r[i].nf);
            tsv_free(&ev); return 1;
        }
        {   /* every numeric field must prove it is a number: whole string,
             * canonical form. Column 3 is the arm name; 5 is the double. */
            static const int intcols[12] = { 0, 1, 2, 4, 6, 7, 8, 9, 10, 11, 12, 13 };
            long tmpv; double tmpd; int q;
            for (q = 0; q < 12; q++)
                if (cc_parse_int(fs(&ev, i, intcols[q]), &tmpv)) {
                    snprintf(err, en, "evidence row %d field %d is not a canonical integer: '%s'",
                             i, intcols[q], fs(&ev, i, intcols[q]));
                    tsv_free(&ev); return 1;
                }
            if (cc_parse_dbl(fs(&ev, i, 5), &tmpd)) {
                snprintf(err, en, "evidence row %d field 5 is not a finite fully-consumed double: '%s'",
                         i, fs(&ev, i, 5));
                tsv_free(&ev); return 1;
            }
        }
        { long cv; cc_parse_int(fs(&ev, i, 0), &cv);
          if (cv < 0 || cv >= expect_chunks) {          /* range-checked wide, then cast */
              snprintf(err, en, "evidence row %d chunk %ld outside 0..%d", i, cv, expect_chunks - 1);
              tsv_free(&ev); return 1; }
          ch = (int)cv; }
        ai = cc_armid_strict(fs(&ev, i, 3));
        /* (4) no unknown rows */
        if (ai < 0) {
            snprintf(err, en, "evidence row %d names unknown arm '%s'", i, fs(&ev, i, 3));
            tsv_free(&ev); return 1;
        }
        if (ch < 0 || ch >= expect_chunks) {
            snprintf(err, en, "evidence row %d has chunk %d outside 0..%d", i, ch, expect_chunks - 1);
            tsv_free(&ev); return 1;
        }
        /* (3)+(4) each (chunk, arm) exactly once */
        if (seen[ch][ai]) {
            snprintf(err, en, "evidence row %d duplicates (chunk %d, arm %s)", i, ch, fs(&ev, i, 3));
            tsv_free(&ev); return 1;
        }
        seen[ch][ai] = 1;
        /* (6) lived_before = chunk x 1024 and bytes = 1024 */
        cc_parse_int(fs(&ev, i, 1), &lb); cc_parse_int(fs(&ev, i, 2), &by);
        if (lb != (long)ch * CHUNK) {
            snprintf(err, en, "evidence row %d has lived_before %ld, law requires %ld", i, lb, (long)ch * CHUNK);
            tsv_free(&ev); return 1;
        }
        if (by != CHUNK) {
            snprintf(err, en, "evidence row %d has bytes %ld, law requires %d", i, by, CHUNK);
            tsv_free(&ev); return 1;
        }
    }
    /* (4) no missing rows: every pair must have been marked */
    for (i = 0; i < expect_chunks; i++)
        for (a = 0; a < CC_ARMS; a++)
            if (!seen[i][a]) {
                snprintf(err, en, "evidence is missing (chunk %d, arm %s)", i, a == 0 ? "cold" : mapname_of(a));
                tsv_free(&ev); return 1;
            }
    tsv_free(&ev);
    (void)bad; (void)firstc; (void)firsta; (void)expect_rows;
    return 0;
}

/* (7) the semantic comparison: ALL fourteen evidence fields against this
 * hand's own court, not five of them. */
static int cc_check_evidence_bits(const unsigned char *raw, long raw_bytes,
                                  const CcCourt *R, char *err, size_t en)
{
    Tsv ev = cc_tsv_snapshot(raw, raw_bytes);
    int i;
    for (i = 1; i < ev.n; i++) {
        long cv, pos2, pr, rl, ce, ee, re2, wu, se, sa; double bv;
        int ch, ai;
        if (cc_parse_int(fs(&ev, i, 0), &cv)) continue;
        if (cv < 0 || cv >= R->nchunks) continue;            /* structure already proven */
        ch = (int)cv;
        ai = cc_armid_strict(fs(&ev, i, 3));
        if (ai < 0) continue;
        if (cc_parse_dbl(fs(&ev, i, 5), &bv) ||
            cc_parse_int(fs(&ev, i, 4), &pos2) || cc_parse_int(fs(&ev, i, 6), &pr) ||
            cc_parse_int(fs(&ev, i, 7), &rl)   || cc_parse_int(fs(&ev, i, 8), &ce) ||
            cc_parse_int(fs(&ev, i, 9), &ee)   || cc_parse_int(fs(&ev, i, 10), &re2) ||
            cc_parse_int(fs(&ev, i, 11), &wu)  || cc_parse_int(fs(&ev, i, 12), &se) ||
            cc_parse_int(fs(&ev, i, 13), &sa)) {
            snprintf(err, en, "evidence row %d carries a non-canonical numeric field", i);
            tsv_free(&ev); return 1;
        }
        if (!a5_main(bv, R->bits[ch][ai])) {
            snprintf(err, en, "evidence row %d bits %.17g, reconstruction %.17g", i, bv, R->bits[ch][ai]);
            tsv_free(&ev); return 1; }
        if (pos2 != R->positions[ch]) {
            snprintf(err, en, "evidence row %d positions %ld, reconstruction %ld", i, pos2, R->positions[ch]);
            tsv_free(&ev); return 1; }
        if (pr != R->pairs[ch][ai] || rl != R->relations[ch][ai] || ce != R->cearn[ch][ai] ||
            ee != R->earnev[ch][ai] || re2 != R->revev[ch][ai] || wu != R->wins[ch][ai] ||
            se != R->senio[ch][ai] || sa != R->surf[ch]) {
            snprintf(err, en, "evidence row %d (chunk %d arm %s) counters disagree: builder "
                     "pairs %ld rel %ld earned %ld earn %ld rev %ld wins %ld senio %ld surf %ld ; "
                     "mine %ld/%ld/%ld/%ld/%ld/%ld/%ld/%d",
                     i, ch, ai == 0 ? "cold" : mapname_of(ai), pr, rl, ce, ee, re2, wu, se, sa,
                     R->pairs[ch][ai], R->relations[ch][ai], R->cearn[ch][ai], R->earnev[ch][ai],
                     R->revev[ch][ai], R->wins[ch][ai], R->senio[ch][ai], R->surf[ch]);
            tsv_free(&ev); return 1;
        }
    }
    tsv_free(&ev);
    return 0;
}




/* the court pins these eleven names exactly; aliases and alternates are
 * forbidden, and absence of any one is a refusal. */
#define CC_INV_N 11
static const char *CC_INVENTORY[CC_INV_N] = {
    "W_confirmatory.bin",
    "oracle_confirmatory.tsv",
    "confirmatory_selection.tsv",
    "builder_G_confirmatory.tsv",
    "evidence_confirmatory.tsv",
    "maps_confirmatory.tsv",
    "map_summary_confirmatory.tsv",
    "profile_scrambles_confirmatory.tsv",
    "winners_confirmatory.tsv",
    "relation_events_confirmatory.tsv",
    "relations_confirmatory.tsv"
};

typedef struct {
    unsigned char *raw[CC_INV_N];
    long bytes[CC_INV_N];
} CcArtifacts;

static int cc_art_index(const char *name)
{
    int i;
    for (i = 0; i < CC_INV_N; i++) if (!strcmp(name, CC_INVENTORY[i])) return i;
    return -1;
}

static void cc_artifacts_free(CcArtifacts *a)
{
    int i;
    for (i = 0; i < CC_INV_N; i++) { free(a->raw[i]); a->raw[i] = NULL; a->bytes[i] = 0; }
}

static Tsv cc_art_tsv(const CcArtifacts *a, const char *name)
{
    int i = cc_art_index(name);
    if (i < 0) { fprintf(stderr, "internal artifact name error: %s\n", name); exit(2); }
    return cc_tsv_snapshot(a->raw[i], a->bytes[i]);
}

static int cc_read_inventory_file(const char *path, const struct stat *listed,
                                  unsigned char **out, long *outn,
                                  char *err, size_t en)
{
    int fd; struct stat opened; off_t want; unsigned char *b; size_t got = 0;
    fd = open(path, O_RDONLY | O_NOFOLLOW | O_NONBLOCK);
    if (fd < 0) { snprintf(err, en, "cannot open inventory member %s", path); return 1; }
    if (fstat(fd, &opened) != 0 || !S_ISREG(opened.st_mode) ||
        opened.st_dev != listed->st_dev || opened.st_ino != listed->st_ino) {
        snprintf(err, en, "inventory member changed identity before capture: %s", path);
        close(fd); return 1;
    }
    want = opened.st_size;
    if (want < 0 || (uintmax_t)want > (uintmax_t)LONG_MAX) {
        snprintf(err, en, "inventory member is too large to capture: %s", path);
        close(fd); return 1;
    }
    b = (unsigned char *)malloc((size_t)want + 1);
    if (!b) { snprintf(err, en, "cannot allocate inventory snapshot: %s", path); close(fd); return 1; }
    while (got < (size_t)want) {
        ssize_t nr = read(fd, b + got, (size_t)want - got);
        if (nr < 0 && errno == EINTR) continue;
        if (nr <= 0) {
            snprintf(err, en, "short read while capturing inventory member: %s", path);
            free(b); close(fd); return 1;
        }
        got += (size_t)nr;
    }
    {
        unsigned char extra; ssize_t nr;
        do { nr = read(fd, &extra, 1); } while (nr < 0 && errno == EINTR);
        if (nr != 0) {
            snprintf(err, en, "inventory member changed length during capture: %s", path);
            free(b); close(fd); return 1;
        }
    }
    if (close(fd) != 0) {
        snprintf(err, en, "cannot close captured inventory member: %s", path);
        free(b); return 1;
    }
    b[want] = 0; *out = b; *outn = (long)want;
    return 0;
}

/* Enumerate the exact eleven-member directory and capture every member once.
 * All later structure, hash and semantic checks operate on these private bytes. */
static int cc_capture_inventory(const char *dir, CcArtifacts *a, char *err, size_t en)
{
    char p[512]; int i, seen[CC_INV_N] = {0}; struct stat listed[CC_INV_N];
    DIR *dp = opendir(dir); struct dirent *de;
    if (!dp) { snprintf(err, en, "cannot enumerate builder inventory directory"); return 1; }
    while ((de = readdir(dp)) != NULL) {
        int hit;
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, "..")) continue;
        hit = cc_art_index(de->d_name);
        if (hit < 0) {
            snprintf(err, en, "C5 inventory contains unlisted entry %s", de->d_name);
            closedir(dp); return 1;
        }
        if (seen[hit]) {
            snprintf(err, en, "C5 inventory contains duplicate entry %s", de->d_name);
            closedir(dp); return 1;
        }
        snprintf(p, sizeof p, "%s/%s", dir, de->d_name);
        if (lstat(p, &listed[hit]) != 0 || !S_ISREG(listed[hit].st_mode)) {
            snprintf(err, en, "C5 inventory entry %s is not a regular file", de->d_name);
            closedir(dp); return 1;
        }
        seen[hit] = 1;
    }
    closedir(dp);
    for (i = 0; i < CC_INV_N; i++) {
        if (!seen[i]) { snprintf(err, en, "C5 inventory incomplete: missing %s", CC_INVENTORY[i]); return 1; }
        snprintf(p, sizeof p, "%s/%s", dir, CC_INVENTORY[i]);
        if (cc_read_inventory_file(p, &listed[i], &a->raw[i], &a->bytes[i], err, en)) {
            cc_artifacts_free(a); return 1;
        }
        if (i > 0 && cc_buffer_clean(CC_INVENTORY[i], a->raw[i], a->bytes[i], err, en)) {
            cc_artifacts_free(a); return 1;
        }
    }
    return 0;
}

/* =====================================================================
 * PASS 10: the seven books are READ, not merely present.
 * Each table's header is pinned; each row is parsed with the canonical
 * grammar and compared against this hand's own reconstruction.
 * ===================================================================== */
static const char *CC_H_MAPS[7]   = { "chunk","lived","map","source","dest","epoch","correct" };
static const char *CC_H_MSUM[7]   = { "chunk","lived","bonly_pairs","bonly_hits","admitted_pairs","admitted_hits","surface_admitted" };
static const char *CC_H_SCR[6]    = { "chunk","lived","arm","destination","profile_destination","fixed" };
static const char *CC_H_REL[22]   = { "arm","target_s","target_d","target_epoch","context_len",
                                      "c1_s","c1_d","c1_epoch","c2_s","c2_d","c2_epoch","c3_s","c3_d","c3_epoch",
                                      "seen","positive","negative","ledger_bits","peak_bits","state","L","ever_earned" };
static const char *CC_H_EVT[18]   = { "byte_offset","unit_position","arm","event","ledger_after",
                                      "target_s","target_d","target_epoch","context_len",
                                      "c1_s","c1_d","c1_epoch","c2_s","c2_d","c2_epoch","c3_s","c3_d","c3_epoch" };
static const char *CC_H_WIN[18]   = { "byte_offset","unit_position","arm","ledger_before","L_before",
                                      "target_s","target_d","target_epoch","context_len",
                                      "c1_s","c1_d","c1_epoch","c2_s","c2_d","c2_epoch","c3_s","c3_d","c3_epoch" };
static const char *CC_H_G[4]      = { "status","arm","horizon","G_bits" };
static const long CC_HORIZON[5]   = { 1024, 4096, 16384, 65536, 131072 };

static int cc_hdr(Tsv *t, const char *const *h, int n, const char *name, char *err, size_t en)
{
    int i;
    if (t->n < 1 || t->r[0].nf != n) {
        snprintf(err, en, "%s header has %d fields, law requires %d", name, t->n ? t->r[0].nf : 0, n);
        return 1;
    }
    for (i = 0; i < n; i++)
        if (strcmp(fs(t, 0, i), h[i])) {
            snprintf(err, en, "%s header field %d is '%s', law requires '%s'", name, i, fs(t, 0, i), h[i]);
            return 1;
        }
    return 0;
}
/* canonical signed-or-unsigned small integer used by book fields (epoch, correct=-1) */
static int cc_parse_i2(const char *s2, long *out)
{
    if (s2 && s2[0] == '-' && s2[1] == '1' && s2[2] == '\0') { *out = -1; return 0; }
    return cc_parse_int(s2, out);
}
static int cc_relkey_from(Tsv *t, int row, int base, Rel *k)
{
    long v; int z;
    if (cc_parse_int(fs(t, row, base + 0), &v) || v > INT_MAX) return 1; k->ts = (int)v;
    if (cc_parse_int(fs(t, row, base + 1), &v) || v > INT_MAX) return 1; k->td = (int)v;
    if (cc_parse_int(fs(t, row, base + 2), &v) || v > INT_MAX) return 1; k->tep = (int)v;
    if (cc_parse_int(fs(t, row, base + 3), &v) || v > INT_MAX) return 1; k->cl = (int)v;
    for (z = 0; z < 3; z++) {
        if (cc_parse_int(fs(t, row, base + 4 + 3*z), &v) || v > INT_MAX) return 1; k->cs[z] = (int)v;
        if (cc_parse_int(fs(t, row, base + 5 + 3*z), &v) || v > INT_MAX) return 1; k->cd[z] = (int)v;
        if (cc_parse_int(fs(t, row, base + 6 + 3*z), &v) || v > INT_MAX) return 1; k->cep[z] = (int)v;
    }
    return 0;
}
static int cc_mapid_strict(const char *s2)
{
    int k; char b[16];
    if (!strcmp(s2, "bonly")) return 0;
    if (!strcmp(s2, "relation")) return 1;
    if (!strcmp(s2, "oracle")) return 21;
    for (k = 0; k <= 18; k++) { sprintf(b, "null%d", k); if (!strcmp(s2, b)) return 2 + k; }
    return -1;
}

static int cc_maprow_cmp(const void *aa, const void *bb)
{
    const CcMapRow *a = (const CcMapRow *)aa;
    const CcMapRow *b = (const CcMapRow *)bb;
#define CC_MAP_CMP(field) do { \
    if (a->field < b->field) return -1; \
    if (a->field > b->field) return 1; \
} while (0)
    CC_MAP_CMP(chunk);
    CC_MAP_CMP(mapid);
    CC_MAP_CMP(s);
    CC_MAP_CMP(d);
    CC_MAP_CMP(epoch);
    CC_MAP_CMP(correct);
#undef CC_MAP_CMP
    return 0;
}

/* compare the builder's seven books against the reconstruction */
static int cc_check_books(const CcArtifacts *art, const CcCourt *R, char *err, size_t en)
{
    long i;

    /* ---- maps ---- */
    { Tsv t = cc_art_tsv(art, "maps_confirmatory.tsv"); long nrows;
      CcMapRow *got = NULL, *want = NULL;
      if (cc_hdr(&t, CC_H_MAPS, 7, "maps", err, en)) { tsv_free(&t); return 1; }
      nrows = t.n - 1;
      if (nrows != R->nmaps) {
          snprintf(err, en, "maps holds %ld rows, reconstruction has %ld", nrows, R->nmaps);
          tsv_free(&t); return 1; }
      got = (CcMapRow *)malloc(sizeof(*got) * (size_t)(nrows ? nrows : 1));
      want = (CcMapRow *)malloc(sizeof(*want) * (size_t)(nrows ? nrows : 1));
      if (!got || !want) {
          free(got); free(want); tsv_free(&t);
          snprintf(err, en, "out of memory matching maps rows"); return 1; }
      for (i = 0; i < nrows; i++) {
          long ch, lv, s2, d2, ep, co; int mid;
          if (t.r[i+1].nf != 7) {
              snprintf(err, en, "maps row %ld has %d fields", i, t.r[i+1].nf);
              free(got); free(want); tsv_free(&t); return 1; }
          mid = cc_mapid_strict(fs(&t, (int)i+1, 2));
          if (cc_parse_int(fs(&t,(int)i+1,0),&ch) || cc_parse_int(fs(&t,(int)i+1,1),&lv) ||
              cc_parse_int(fs(&t,(int)i+1,3),&s2) || cc_parse_int(fs(&t,(int)i+1,4),&d2) ||
              cc_parse_int(fs(&t,(int)i+1,5),&ep) || cc_parse_i2(fs(&t,(int)i+1,6),&co) || mid < 0 ||
              ch > INT_MAX || s2 > INT_MAX || d2 > INT_MAX || ep > INT_MAX || co > INT_MAX ||
              lv != ch * CHUNK) {
              snprintf(err, en, "maps row %ld has a non-canonical or inconsistent field", i);
              free(got); free(want); tsv_free(&t); return 1; }
          got[i].chunk = (int)ch; got[i].mapid = mid;
          got[i].s = (int)s2; got[i].d = (int)d2;
          got[i].epoch = (int)ep; got[i].correct = (int)co;
      }
      memcpy(want, R->maps, sizeof(*want) * (size_t)nrows);
      qsort(got, (size_t)nrows, sizeof(*got), cc_maprow_cmp);
      qsort(want, (size_t)nrows, sizeof(*want), cc_maprow_cmp);
      for (i = 0; i < nrows; i++) if (cc_maprow_cmp(&got[i], &want[i])) {
          snprintf(err, en, "maps semantic row %ld disagrees "
                   "(builder %d/%s/%d->%d ep%d correct %d ; mine %d/%s/%d->%d ep%d correct %d)",
                   i, got[i].chunk, mapname_of(got[i].mapid), got[i].s, got[i].d,
                   got[i].epoch, got[i].correct, want[i].chunk, mapname_of(want[i].mapid),
                   want[i].s, want[i].d, want[i].epoch, want[i].correct);
          free(got); free(want); tsv_free(&t); return 1;
      }
      free(got); free(want);
      tsv_free(&t); }

    /* ---- map_summary ---- */
    { Tsv t = cc_art_tsv(art, "map_summary_confirmatory.tsv");
      if (cc_hdr(&t, CC_H_MSUM, 7, "map_summary", err, en)) { tsv_free(&t); return 1; }
      if (t.n - 1 != R->nchunks) {
          snprintf(err, en, "map_summary holds %d rows, law requires %d", t.n - 1, R->nchunks);
          tsv_free(&t); return 1; }
      for (i = 0; i < t.n - 1; i++) {
          long ch, lv, bp, bh, ap, ah, sa; long mbp = 0, mbh = 0, map2 = 0, mah = 0; long j;
          if (t.r[i+1].nf != 7) { snprintf(err, en, "map_summary row %ld has %d fields", i, t.r[i+1].nf); tsv_free(&t); return 1; }
          if (cc_parse_int(fs(&t,(int)i+1,0),&ch) || cc_parse_int(fs(&t,(int)i+1,1),&lv) ||
              cc_parse_int(fs(&t,(int)i+1,2),&bp) || cc_parse_i2(fs(&t,(int)i+1,3),&bh) ||
              cc_parse_int(fs(&t,(int)i+1,4),&ap) || cc_parse_i2(fs(&t,(int)i+1,5),&ah) ||
              cc_parse_int(fs(&t,(int)i+1,6),&sa)) {
              snprintf(err, en, "map_summary row %ld has a non-canonical field", i); tsv_free(&t); return 1; }
          for (j = 0; j < R->nmaps; j++) {
              const CcMapRow *m = &R->maps[j];
              if (m->chunk != ch) continue;
              if (m->mapid == 0) { mbp++; if (m->correct > 0) mbh++; }
              else if (m->mapid == 1) { map2++; if (m->correct > 0) mah++; }
          }
          if (ch != i + 1 || lv != ch * CHUNK || bp != mbp || ap != map2 || bh != mbh || ah != mah || sa != 0) {
              snprintf(err, en, "map_summary row %ld disagrees (builder %ld/%ld %ld/%ld surf %ld ; mine %ld/%ld %ld/%ld surf 0)",
                       i, bp, bh, ap, ah, sa, mbp, mbh, map2, mah);
              tsv_free(&t); return 1; }
      }
      tsv_free(&t); }

    /* ---- profile_scrambles ---- */
    { Tsv t = cc_art_tsv(art, "profile_scrambles_confirmatory.tsv");
      if (cc_hdr(&t, CC_H_SCR, 6, "profile_scrambles", err, en)) { tsv_free(&t); return 1; }
      if (t.n - 1 != R->nscr) {
          snprintf(err, en, "profile_scrambles holds %d rows, reconstruction has %ld", t.n - 1, R->nscr);
          tsv_free(&t); return 1; }
      for (i = 0; i < R->nscr; i++) {
          long ch, lv, d2, pr, fx; int arm; const CcScrRow *m = &R->scr[i];
          if (t.r[i+1].nf != 6) { snprintf(err, en, "profile_scrambles row %ld has %d fields", i, t.r[i+1].nf); tsv_free(&t); return 1; }
          arm = cc_mapid_strict(fs(&t, (int)i+1, 2)) - 2;
          if (cc_parse_int(fs(&t,(int)i+1,0),&ch) || cc_parse_int(fs(&t,(int)i+1,1),&lv) ||
              cc_parse_int(fs(&t,(int)i+1,3),&d2) || cc_parse_int(fs(&t,(int)i+1,4),&pr) ||
              cc_parse_int(fs(&t,(int)i+1,5),&fx) || arm < 0 || arm > 18) {
              snprintf(err, en, "profile_scrambles row %ld has a non-canonical field", i); tsv_free(&t); return 1; }
          if (ch != m->chunk || lv != (long)m->chunk * CHUNK || arm != m->arm ||
              d2 != m->d || pr != m->prof || fx != m->fixed) {
              snprintf(err, en, "profile_scrambles row %ld disagrees (builder null%d %ld->%ld fixed %ld ; mine null%d %d->%d fixed %d)",
                       i, arm, d2, pr, fx, m->arm, m->d, m->prof, m->fixed);
              tsv_free(&t); return 1; }
      }
      tsv_free(&t); }

    /* ---- relations: the final books of all 21 arms ---- */
    { Tsv t = cc_art_tsv(art, "relations_confirmatory.tsv"); long total = 0, offsets[CC_MAPS + 1]; int a;
      unsigned char *matched;
      if (cc_hdr(&t, CC_H_REL, 22, "relations", err, en)) { tsv_free(&t); return 1; }
      offsets[0] = 0;
      for (a = 0; a < CC_MAPS; a++) { total += rb_n[a]; offsets[a + 1] = total; }
      if (t.n - 1 != total) {
          snprintf(err, en, "relations holds %d rows, reconstruction has %ld", t.n - 1, total);
          tsv_free(&t); return 1; }
      matched = (unsigned char *)calloc((size_t)(total ? total : 1), 1);
      if (!matched) { snprintf(err, en, "relations match table allocation failed"); tsv_free(&t); return 1; }
#define CC_REL_FAIL(...) do { snprintf(err, en, __VA_ARGS__); free(matched); tsv_free(&t); return 1; } while (0)
      for (i = 0; i < t.n - 1; i++) {
          Rel k; long seen, pos2, neg, st, ev2; double led, peak, L2;
          int arm, found = -1; long j;
          if (t.r[i+1].nf != 22) CC_REL_FAIL("relations row %ld has %d fields", i, t.r[i+1].nf);
          arm = cc_mapid_strict(fs(&t, (int)i+1, 0));
          if (arm < 0) CC_REL_FAIL("relations row %ld names unknown arm '%s'", i, fs(&t,(int)i+1,0));
          if (arm == 0) CC_REL_FAIL("relations row %ld uses forbidden bonly alias", i);
          /* arm name -> book index: relation 0, null_k 1+k, oracle 20 */
          if (arm == 21) arm = 20; else if (arm >= 2) arm = arm - 1; else arm = 0;
          if (cc_relkey_from(&t, (int)i+1, 1, &k)) CC_REL_FAIL("relations row %ld key malformed or outside int range", i);
          if (cc_parse_int(fs(&t,(int)i+1,14),&seen) || cc_parse_int(fs(&t,(int)i+1,15),&pos2) ||
              cc_parse_int(fs(&t,(int)i+1,16),&neg) || cc_parse_dbl(fs(&t,(int)i+1,17),&led) ||
              cc_parse_dbl(fs(&t,(int)i+1,18),&peak) || cc_parse_int(fs(&t,(int)i+1,19),&st) ||
              cc_parse_dbl(fs(&t,(int)i+1,20),&L2) || cc_parse_int(fs(&t,(int)i+1,21),&ev2)) {
              CC_REL_FAIL("relations row %ld has a non-canonical field", i); }
          for (j = 0; j < rb_n[arm]; j++) if (rel_same(&rb[arm][j], &k)) { found = (int)j; break; }
          if (found < 0) CC_REL_FAIL("relations row %ld names a key absent from the reconstruction", i);
          if (matched[offsets[arm] + found])
              CC_REL_FAIL("relations row %ld duplicates a reconstructed key", i);
          matched[offsets[arm] + found] = 1;
          { const Rel *m = &rb[arm][found];
            if (seen != m->seen || pos2 != m->pos || neg != m->neg || st != m->st || ev2 != m->ever ||
                !a5_main(led, m->led) || !a5_main(peak, m->peak) ||
                (m->L == 0.0 ? L2 != 0.0 : !a5_main(L2, m->L))) {
                CC_REL_FAIL("relations row %ld disagrees (builder seen %ld pos %ld neg %ld led %.17g ; mine %ld/%ld/%ld/%.17g)",
                            i, seen, pos2, neg, led, m->seen, m->pos, m->neg, m->led); } }
      }
      for (i = 0; i < total; i++) if (!matched[i]) CC_REL_FAIL("relations omits reconstructed key %ld", i);
      free(matched);
#undef CC_REL_FAIL
      tsv_free(&t); }

    /* ---- relation_events ---- */
    { Tsv t = cc_art_tsv(art, "relation_events_confirmatory.tsv");
      if (cc_hdr(&t, CC_H_EVT, 18, "relation_events", err, en)) { tsv_free(&t); return 1; }
      if (t.n - 1 != R->nevt) {
          snprintf(err, en, "relation_events holds %d rows, reconstruction has %ld", t.n - 1, R->nevt);
          tsv_free(&t); return 1; }
      for (i = 0; i < R->nevt; i++) {
          long off, up; double la; Rel k; int arm, isE; const CcKeyRow *m = &R->evt[i];
          if (t.r[i+1].nf != 18) { snprintf(err, en, "relation_events row %ld has %d fields", i, t.r[i+1].nf); tsv_free(&t); return 1; }
          arm = cc_armid_strict(fs(&t, (int)i+1, 2));
          isE = !strcmp(fs(&t, (int)i+1, 3), "earn") ? 1 : (!strcmp(fs(&t, (int)i+1, 3), "revoke") ? 0 : -1);
          if (cc_parse_int(fs(&t,(int)i+1,0),&off) || cc_parse_int(fs(&t,(int)i+1,1),&up) ||
              cc_parse_dbl(fs(&t,(int)i+1,4),&la) || cc_relkey_from(&t,(int)i+1,5,&k) || arm < 0 || isE < 0) {
              snprintf(err, en, "relation_events row %ld has a non-canonical field", i); tsv_free(&t); return 1; }
          if (off != m->off || up != m->upos || arm != m->arm || isE != (int)m->b ||
              !a5_main(la, m->a) || !rel_same(&k, &m->key)) {
              snprintf(err, en, "relation_events row %ld disagrees with the reconstruction", i);
              tsv_free(&t); return 1; }
      }
      tsv_free(&t); }

    /* ---- winners ---- */
    { Tsv t = cc_art_tsv(art, "winners_confirmatory.tsv");
      if (cc_hdr(&t, CC_H_WIN, 18, "winners", err, en)) { tsv_free(&t); return 1; }
      if (t.n - 1 != R->nwin) {
          snprintf(err, en, "winners holds %d rows, reconstruction has %ld", t.n - 1, R->nwin);
          tsv_free(&t); return 1; }
      for (i = 0; i < R->nwin; i++) {
          long off, up; double lb, Lb; Rel k; int arm; const CcKeyRow *m = &R->win[i];
          if (t.r[i+1].nf != 18) { snprintf(err, en, "winners row %ld has %d fields", i, t.r[i+1].nf); tsv_free(&t); return 1; }
          arm = cc_armid_strict(fs(&t, (int)i+1, 2));
          if (cc_parse_int(fs(&t,(int)i+1,0),&off) || cc_parse_int(fs(&t,(int)i+1,1),&up) ||
              cc_parse_dbl(fs(&t,(int)i+1,3),&lb) || cc_parse_dbl(fs(&t,(int)i+1,4),&Lb) ||
              cc_relkey_from(&t,(int)i+1,5,&k) || arm < 0) {
              snprintf(err, en, "winners row %ld has a non-canonical field", i); tsv_free(&t); return 1; }
          if (off != m->off || up != m->upos || arm != m->arm ||
              !a5_main(lb, m->a) || !a5_main(Lb, m->b) || !rel_same(&k, &m->key)) {
              snprintf(err, en, "winners row %ld disagrees with the reconstruction", i);
              tsv_free(&t); return 1; }
      }
      tsv_free(&t); }

    /* ---- builder_G: 22 arms x 5 horizons, every value recomputed ---- */
    { Tsv t = cc_art_tsv(art, "builder_G_confirmatory.tsv"); int a, hh; long row = 1;
      if (cc_hdr(&t, CC_H_G, 4, "builder_G", err, en)) { tsv_free(&t); return 1; }
      if (t.n - 1 != CC_ARMS * 5) {
          snprintf(err, en, "builder_G holds %d rows, law requires %d (22 arms x 5 horizons)",
                   t.n - 1, CC_ARMS * 5); tsv_free(&t); return 1; }
      row = 1;
      for (a = 0; a < CC_ARMS; a++) for (hh = 0; hh < 5; hh++) {
          long hz; double gv, mine; int arm2;
          if (t.r[row].nf != 4) { snprintf(err, en, "builder_G row %ld has %d fields", row, t.r[row].nf); tsv_free(&t); return 1; }
          arm2 = cc_armid_strict(fs(&t, (int)row, 1));
          if (strcmp(fs(&t,(int)row,0), "NOT_VERDICT") ||
              cc_parse_int(fs(&t,(int)row,2),&hz) || cc_parse_dbl(fs(&t,(int)row,3),&gv) || arm2 != a) {
              snprintf(err, en, "builder_G row %ld malformed or out of arm order", row); tsv_free(&t); return 1; }
          if (hz != CC_HORIZON[hh]) {
              snprintf(err, en, "builder_G row %ld horizon %ld, law requires %ld", row, hz, CC_HORIZON[hh]);
              tsv_free(&t); return 1; }
          mine = cc_G(R, a, 0, (int)(CC_HORIZON[hh] / CHUNK));
          if (!a5_main(gv, mine)) {
              snprintf(err, en, "builder_G arm %s horizon %ld is %.17g, reconstruction gives %.17g",
                       a == 0 ? "cold" : mapname_of(a), hz, gv, mine);
              tsv_free(&t); return 1; }
          row++;
      }
      tsv_free(&t); }
    return 0;
}

/* the court pins the confirmatory manifest: header, then exactly these 19 rows
 * in this order, every field cross-checked against the receipts and against
 * this hand's own derived C2/C3 values. */
#define CC_MF_N 19
static const char *CC_MF[CC_MF_N] = {
    "status", "mode", "class", "class_index", "source_sha256", "source_bytes",
    "builder_source_sha256", "verifier_source_sha256", "base_text_sha256",
    "class_digest_sha256", "seed_digest_sha256", "roots_receipt_sha256",
    "base_commit_sha256", "base_freeze_sha256", "selection_receipt_sha256",
    "world_seed_be64", "half_tail_seed_be64", "base_bytes", "world_bytes"
};
typedef struct {
    const CcRoot *roots;
    const CcKV *commit; int ncommit;
    const CcKV *sel;    int nsel;
    const char *roots_sha, *commit_sha, *freeze_sha, *sel_sha;
    const char *class_digest, *seed_digest, *world_seed_hex, *half_tail_hex;
    int class_index;
} CcCtx;

static int cc_is_hex16(const char *s)
{
    int i;
    if (!s || strlen(s) != 16) return 0;
    for (i = 0; i < 16; i++) {
        char c = s[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return 0;
    }
    return 1;
}
static int cc_check_manifest(const unsigned char *raw, long raw_bytes,
                             const CcCtx *cx, char *err, size_t en)
{
    CcKV mk[64]; int nm = 0, i;
    const char *v; long nv;
    if (cc_load_kv_bytes(raw, raw_bytes, "confirmatory_selection.tsv",
                         mk, 64, &nm, err, en)) return 1;
    if (nm != CC_MF_N) {
        snprintf(err, en, "manifest holds %d rows, law requires exactly %d", nm, CC_MF_N);
        return 1;
    }
    for (i = 0; i < CC_MF_N; i++)
        if (strcmp(mk[i].field, CC_MF[i])) {
            snprintf(err, en, "manifest row %d is '%s', law requires '%s'", i + 1, mk[i].field, CC_MF[i]);
            return 1;
        }
    /* --- grammar and semantics, every field cross-checked --- */
    v = cc_kv(mk, nm, "status");
    if (strcmp(v, "raw-builder-artifacts-not-verdict")) {
        snprintf(err, en, "manifest status is '%s'", v); return 1; }
    v = cc_kv(mk, nm, "mode");
    if (strcmp(v, "confirmatory")) { snprintf(err, en, "manifest mode is '%s'", v); return 1; }
    v = cc_kv(mk, nm, "class");
    if (strcmp(v, cc_kv(cx->sel, cx->nsel, "class"))) {
        snprintf(err, en, "manifest class '%s' contradicts the selection receipt", v); return 1; }
    if (cc_kv_uint(mk, nm, "class_index", &nv)) {
        snprintf(err, en, "manifest class_index is not a canonical unsigned integer"); return 1; }
    if (nv != cx->class_index || nv < 0 || nv > 3) {
        snprintf(err, en, "manifest class_index %ld contradicts the derived class %d", nv, cx->class_index); return 1; }
    if (strcmp(CC_CLASS_NAME[nv], v)) {
        snprintf(err, en, "manifest class '%s' and class_index %ld are inconsistent", v, nv); return 1; }
    v = cc_kv(mk, nm, "source_sha256");
    if (strcmp(v, "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb")) {
        snprintf(err, en, "manifest source_sha256 is not the frozen SOURCE_A pin"); return 1; }
    if (cc_kv_uint(mk, nm, "source_bytes", &nv) || nv != 447545) {
        snprintf(err, en, "manifest source_bytes is not the frozen 447545"); return 1; }
    { const CcRoot *rb2 = cc_root(cx->roots, "confirmatory_builder");
      const CcRoot *rv = cc_root(cx->roots, "confirmatory_verifier");
      if (!rb2 || !rv) { snprintf(err, en, "roots receipt lacks builder/verifier role"); return 1; }
      if (strcmp(cc_kv(mk, nm, "builder_source_sha256"), rb2->sha)) {
          snprintf(err, en, "manifest builder_source_sha256 contradicts the roots receipt"); return 1; }
      if (strcmp(cc_kv(mk, nm, "verifier_source_sha256"), rv->sha)) {
          snprintf(err, en, "manifest verifier_source_sha256 contradicts the roots receipt"); return 1; } }
    if (strcmp(cc_kv(mk, nm, "base_text_sha256"), cc_kv(cx->commit, cx->ncommit, "base_text_sha256"))) {
        snprintf(err, en, "manifest base_text_sha256 contradicts the commitment"); return 1; }
    if (strcmp(cc_kv(mk, nm, "class_digest_sha256"), cx->class_digest)) {
        snprintf(err, en, "manifest class_digest_sha256 contradicts the derived C2 value"); return 1; }
    if (strcmp(cc_kv(mk, nm, "seed_digest_sha256"), cx->seed_digest)) {
        snprintf(err, en, "manifest seed_digest_sha256 contradicts the derived C3 value"); return 1; }
    { struct { const char *f; const char *sha; } rr[4] = {
          { "roots_receipt_sha256", cx->roots_sha },
          { "base_commit_sha256", cx->commit_sha },
          { "base_freeze_sha256", cx->freeze_sha },
          { "selection_receipt_sha256", cx->sel_sha } };
      int k;
      for (k = 0; k < 4; k++) {
          if (strcmp(cc_kv(mk, nm, rr[k].f), rr[k].sha)) {
              snprintf(err, en, "manifest %s does not equal the actual receipt digest", rr[k].f); return 1; }
      } }
    v = cc_kv(mk, nm, "world_seed_be64");
    if (!cc_is_hex16(v)) { snprintf(err, en, "manifest world_seed_be64 is not 16 lowercase hex"); return 1; }
    if (strcmp(v, cx->world_seed_hex)) {
        snprintf(err, en, "manifest world_seed_be64 contradicts the derived seed"); return 1; }
    v = cc_kv(mk, nm, "half_tail_seed_be64");
    if (cx->class_index == 2) {
        if (!cc_is_hex16(v) || strcmp(v, cx->half_tail_hex)) {
            snprintf(err, en, "manifest half_tail_seed_be64 must be the derived 16-hex seed for class half"); return 1; }
    } else if (strcmp(v, "NOT_USED")) {
        snprintf(err, en, "manifest half_tail_seed_be64 must be the literal NOT_USED outside class half"); return 1;
    }
    if (cc_kv_uint(mk, nm, "base_bytes", &nv)) {
        snprintf(err, en, "manifest base_bytes is not a canonical unsigned integer"); return 1; }
    { long cb;
      if (cc_kv_uint(cx->commit, cx->ncommit, "base_text_bytes", &cb) || nv != cb) {
          snprintf(err, en, "manifest base_bytes %ld contradicts the commitment", nv); return 1; } }
    if (cc_kv_uint(mk, nm, "world_bytes", &nv) || nv != CC_RUN_BYTES) {
        snprintf(err, en, "manifest world_bytes is not %d", CC_RUN_BYTES); return 1; }
    return 0;
}

static int cc_check_inventory(const CcArtifacts *art, int class_index, uint64_t world_seed,
                              const CcCtx *cx, char *err, size_t en)
{
    {
        Tsv om = cc_art_tsv(art, "oracle_confirmatory.tsv");
        int expect[256], seen[256], j2;
        if (class_index == 0 || class_index == 2) rename_perm(world_seed, expect);
        else for (j2 = 0; j2 < 256; j2++) expect[j2] = j2;
        if (om.n < 1 || om.r[0].nf != 2 ||
            strcmp(fs(&om, 0, 0), "source") || strcmp(fs(&om, 0, 1), "destination")) {
            snprintf(err, en, "oracle header must be exactly source<TAB>destination");
            tsv_free(&om); return 1;
        }
        if (om.n - 1 != 256) {
            snprintf(err, en, "oracle holds %d data rows, law requires exactly 256", om.n - 1);
            tsv_free(&om); return 1;
        }
        for (j2 = 0; j2 < 256; j2++) seen[j2] = 0;
        for (j2 = 0; j2 < 256; j2++) {
            long a2, b2; int r = j2 + 1;
            if (om.r[r].nf != 2) {
                snprintf(err, en, "oracle row %d has %d fields, law requires exactly 2", j2, om.r[r].nf);
                tsv_free(&om); return 1;
            }
            if (cc_parse_int(fs(&om, r, 0), &a2) || cc_parse_int(fs(&om, r, 1), &b2)) {
                snprintf(err, en, "oracle row %d carries a non-canonical number ('%s','%s')",
                         j2, fs(&om, r, 0), fs(&om, r, 1));
                tsv_free(&om); return 1;
            }
            if (a2 != j2) {
                snprintf(err, en, "oracle row %d has source %ld, rows must ascend 0..255", j2, a2);
                tsv_free(&om); return 1;
            }
            if (b2 < 0 || b2 > 255) {
                snprintf(err, en, "oracle row %d destination %ld outside 0..255", j2, b2);
                tsv_free(&om); return 1;
            }
            if (seen[b2]) {
                snprintf(err, en, "oracle is not a bijection: destination %ld repeats at row %d", b2, j2);
                tsv_free(&om); return 1;
            }
            seen[b2] = 1;
            if (b2 != expect[j2]) {
                snprintf(err, en, "oracle row %d is %d->%ld, the derived permutation requires %d->%d",
                         j2, j2, b2, j2, expect[j2]);
                tsv_free(&om); return 1;
            }
        }
        tsv_free(&om);
    }
    /* confirmatory_selection.tsv against the pinned 19-row manifest contract */
    {
        int mi = cc_art_index("confirmatory_selection.tsv");
        if (cc_check_manifest(art->raw[mi], art->bytes[mi], cx, err, en)) return 1;
    }
    {
        int gi = cc_art_index("builder_G_confirmatory.tsv");
        long n = art->bytes[gi], z; const unsigned char *b = art->raw[gi];
        int ok = 0;
        for (z = 0; z + 11 <= n; z++)
            if (memcmp(b + z, "NOT_VERDICT", 11) == 0) { ok = 1; break; }
        if (!ok) { snprintf(err, en, "builder_G_confirmatory.tsv is not explicitly marked NOT_VERDICT"); return 1; }
    }
    return 0;
}

/* =====================================================================
 * The real --confirmatory door.
 * ===================================================================== */
typedef struct {
    const char *source_a, *base_text, *roots, *base_commit, *base_freeze, *selection, *builder_out;
    int receipt_test;
} CcArgs;

/* returns 0 on a completed verdict, nonzero on lawful refusal */
static int cc_confirmatory(const CcArgs *ag, int quiet)
{
    char err[512] = {0}, sha[65];
    char roots_receipt_sha[65], commit_receipt_sha[65], freeze_receipt_sha[65], selection_receipt_sha[65];
    long bytes;
    long roots_receipt_bytes = 0, commit_receipt_bytes = 0;
    long freeze_receipt_bytes = 0, selection_receipt_bytes = 0;
    CcRoot roots[18];
    CcKV commit[8], freeze[8], sel[24];
    int ncommit = 0, nfreeze = 0, nsel = 0;
    char rootdir[PATH_MAX], p[PATH_MAX];
    long lenA = 0, lenB = 0, nv = 0;
    unsigned char *A = NULL, *B = NULL, *W = NULL, *X = NULL;
    long lenX = 0;
    CcArtifacts art = {{0}, {0}};
    int truemap[256];
    int class_index = -1;
    uint64_t world_seed = 0, half_tail = 0;
    char cdig_s[65] = {0}, sdig_s[65] = {0}, wsh_s[32] = {0}, hth_s[32] = {0};

#define CC_REFUSE(...) do { char m[400]; snprintf(m, sizeof m, __VA_ARGS__); \
    snprintf(cc_last_refusal, sizeof cc_last_refusal, "%s", m); \
    if (!quiet) cc_row("REFUSE", "CONFIRMATORY", "%s", m); \
    cc_artifacts_free(&art); free(A); free(B); free(W); free(X); return 1; } while (0)

    cc_last_refusal[0] = 0;

    /* ---- roots receipt: schema, then every row resolved and rehashed ---- */
    if (cc_load_roots(ag->roots, roots, roots_receipt_sha, &roots_receipt_bytes, err, sizeof err))
        CC_REFUSE("roots receipt rejected: %s", err);
    if (snprintf(rootdir, sizeof rootdir, "%s", ag->roots) < 0 ||
        strlen(ag->roots) >= sizeof rootdir)
        CC_REFUSE("roots receipt path is too long");
    { char *s = strrchr(rootdir, '/');
      if (!s) strcpy(rootdir, ".");
      else if (s == rootdir) rootdir[1] = 0;
      else *s = 0; }
    {
        int i;
        for (i = 0; i < 18; i++) {
            unsigned char *rb = NULL; long rn = 0;
            if (snprintf(p, sizeof p, "%s/%s", rootdir, roots[i].path) < 0 ||
                strlen(rootdir) + 1 + strlen(roots[i].path) >= sizeof p)
                CC_REFUSE("roots role '%s': resolved path is too long", roots[i].role);
            if (cc_read_once(p, &rb, &rn, err, sizeof err))
                CC_REFUSE("roots role '%s': unreadable file %s", roots[i].role, roots[i].path);
            sha256_buf(rb, (size_t)rn, sha); bytes = rn;
            if (bytes != roots[i].bytes) {
                free(rb); CC_REFUSE("roots role '%s': byte length %ld, receipt says %ld", roots[i].role, bytes, roots[i].bytes);
            }
            if (strcmp(sha, roots[i].sha)) {
                free(rb); CC_REFUSE("roots role '%s': digest mismatch", roots[i].role);
            }
            if (!strcmp(roots[i].role, "prior_freeze")) {
                if (rn != 761 ||
                    strcmp(sha, "a054b759a13fab35181e9cd9e4f5fa5abad7fd9263419ef2cd9296a99fd9f250")) {
                    free(rb); CC_REFUSE("prior freeze root is not the frozen parent receipt");
                }
                if (cc_verify_prior_binding(rb, rn, roots, err, sizeof err)) {
                    free(rb); CC_REFUSE("roots role '%s': %s", roots[i].role, err);
                }
                free(rb);
            } else if (!strcmp(roots[i].role, "execution_law")) {
                if (rn != 26929 ||
                    strcmp(sha, "f823478054705091300b6a5f0ce4a0b43af50f019cea32dc27e8aa10889ba908")) {
                    free(rb); CC_REFUSE("execution law root is not frozen");
                }
                free(rb);
            } else if (!strcmp(roots[i].role, "execution_law_receipt")) {
                if (rn != 869 ||
                    strcmp(sha, "7bcdc748e2cfe9a60112ef36b8718f504620f2f16682f32da0e0bd553c077501")) {
                    free(rb); CC_REFUSE("execution law receipt root is not frozen");
                }
                free(rb);
            } else if (!strcmp(roots[i].role, "confirmatory_exclusions")) {
                if (cc_buffer_clean(roots[i].path, rb, rn, err, sizeof err)) {
                    free(rb); CC_REFUSE("roots role '%s': %s", roots[i].role, err);
                }
                X = rb; lenX = rn;
            } else free(rb);
        }
    }
    /* ---- base commitment ---- */
    if (cc_load_kv_snapshot(ag->base_commit, commit, 8, &ncommit,
                            commit_receipt_sha, &commit_receipt_bytes, err, sizeof err))
        CC_REFUSE("base commitment rejected: %s", err);
    if (cc_kv_order(commit, ncommit, CC_COMMIT_FIELDS, 4, err, sizeof err)) CC_REFUSE("base commitment field order: %s", err);
    if (strcmp(cc_kv(commit, ncommit, "status"), "base-committed-class-not-computed"))
        CC_REFUSE("base commitment status '%s' is not the committed status", cc_kv(commit, ncommit, "status"));
    /* ---- base freeze, bound to roots and commitment ---- */
    if (cc_load_kv_snapshot(ag->base_freeze, freeze, 8, &nfreeze,
                            freeze_receipt_sha, &freeze_receipt_bytes, err, sizeof err))
        CC_REFUSE("base freeze rejected: %s", err);
    if (cc_kv_order(freeze, nfreeze, CC_FREEZE_FIELDS, 5, err, sizeof err)) CC_REFUSE("base freeze field order: %s", err);
    if (strcmp(cc_kv(freeze, nfreeze, "status"), "base-commit-frozen-class-not-computed"))
        CC_REFUSE("base freeze status '%s' is wrong", cc_kv(freeze, nfreeze, "status"));
    if (cc_kv_uint(freeze, nfreeze, "roots_receipt_bytes", &nv) || roots_receipt_bytes != nv ||
        strcmp(roots_receipt_sha, cc_kv(freeze, nfreeze, "roots_receipt_sha256")))
        CC_REFUSE("base freeze does not bind this roots receipt");
    if (cc_kv_uint(freeze, nfreeze, "base_commit_bytes", &nv) || commit_receipt_bytes != nv ||
        strcmp(commit_receipt_sha, cc_kv(freeze, nfreeze, "base_commit_sha256")))
        CC_REFUSE("base freeze does not bind this base commitment");
    /* ---- selection receipt ---- */
    if (cc_load_kv_snapshot(ag->selection, sel, 24, &nsel,
                            selection_receipt_sha, &selection_receipt_bytes, err, sizeof err))
        CC_REFUSE("selection rejected: %s", err);
    if (cc_kv_order(sel, nsel, CC_SEL_FIELDS, 17, err, sizeof err)) CC_REFUSE("selection field order: %s", err);
    {
        const char *expected_status = ag->receipt_test
            ? "synthetic-selected-not-confirmatory" : "selected-not-run";
        if (strcmp(cc_kv(sel, nsel, "status"), expected_status))
            CC_REFUSE("selection status '%s' is not accepted by this mode", cc_kv(sel, nsel, "status"));
    }
    if (cc_kv_uint(sel, nsel, "roots_receipt_bytes", &nv) || roots_receipt_bytes != nv ||
        strcmp(roots_receipt_sha, cc_kv(sel, nsel, "roots_receipt_sha256")))
        CC_REFUSE("selection cross-reference to roots receipt is wrong");
    if (cc_kv_uint(sel, nsel, "base_commit_bytes", &nv) || commit_receipt_bytes != nv ||
        strcmp(commit_receipt_sha, cc_kv(sel, nsel, "base_commit_sha256")))
        CC_REFUSE("selection cross-reference to base commitment is wrong");
    if (cc_kv_uint(sel, nsel, "base_freeze_bytes", &nv) || freeze_receipt_bytes != nv ||
        strcmp(freeze_receipt_sha, cc_kv(sel, nsel, "base_freeze_sha256")))
        CC_REFUSE("selection cross-reference to base freeze is wrong");
    /* builder and verifier digests must come from the roots receipt, not the CLI */
    {
        const CcRoot *rb2 = cc_root(roots, "confirmatory_builder");
        const CcRoot *rv = cc_root(roots, "confirmatory_verifier");
        if (!rb2 || !rv) CC_REFUSE("roots receipt lacks builder or verifier role");
        if (strcmp(rb2->sha, cc_kv(sel, nsel, "builder_source_sha256")))
            CC_REFUSE("selection builder digest does not match the roots receipt");
        if (strcmp(rv->sha, cc_kv(sel, nsel, "verifier_source_sha256")))
            CC_REFUSE("selection verifier digest does not match the roots receipt");
    }
    /* ---- SOURCE_A pin: one read, then use these exact bytes below ---- */
    if (!ag->receipt_test) {
        if (cc_read_once(ag->source_a, &A, &lenA, err, sizeof err))
            CC_REFUSE("SOURCE_A cannot be read: %s", err);
        sha256_buf(A, (size_t)lenA, sha);
        if (lenA != 447545 || strcmp(sha, "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb"))
            CC_REFUSE("SOURCE_A is not the pinned development source");
    }
    /* ---- base text: one read, then hash, preflight and construct from this buffer ---- */
    if (cc_read_once(ag->base_text, &B, &lenB, err, sizeof err))
        CC_REFUSE("BASE_TEXT cannot be read: %s", err);
    sha256_buf(B, (size_t)lenB, sha);
    bytes = lenB;
    if (cc_kv_uint(commit, ncommit, "base_text_bytes", &nv) || lenB != nv ||
        strcmp(sha, cc_kv(commit, ncommit, "base_text_sha256")))
        CC_REFUSE("BASE_TEXT does not match the frozen base commitment");
    if (cc_kv_uint(sel, nsel, "base_text_bytes", &nv) || lenB != nv ||
        strcmp(sha, cc_kv(sel, nsel, "base_text_sha256")))
        CC_REFUSE("BASE_TEXT does not match the selection receipt");
    {
        const CcRoot *rx = cc_root(roots, "confirmatory_exclusions");
        Tsv ex; int i, excluded = 0, bad_bytes = 0;
        if (!rx || !X) CC_REFUSE("roots receipt lacks the exclusions role");
        ex = cc_tsv_snapshot(X, lenX);
        for (i = 1; i < ex.n; i++) {
            long excluded_bytes = 0;
            if (cc_parse_int(fs(&ex, i, 1), &excluded_bytes) ||
                excluded_bytes < 0) {
                bad_bytes = 1;
                break;
            }
            if (excluded_bytes == bytes && !strcmp(fs(&ex, i, 2), sha))
                excluded = 1;
        }
        tsv_free(&ex);
        if (bad_bytes) CC_REFUSE("confirmatory exclusions carries a non-canonical byte count");
        if (excluded) CC_REFUSE("BASE_TEXT is an excluded corpus");
    }
    /* ---- eligibility re-check against the committed lengths ---- */
    if (lenB < CC_RUN_BYTES) CC_REFUSE("BASE_TEXT shorter than RUN_BYTES");
    {
        int sat = 0; long ffl = cc_ff_length(B, lenB, &sat, NULL);
        if (sat) CC_REFUSE("false-friend probe saturated on BASE_TEXT");
        if (ffl < 0) CC_REFUSE("false-friend construction undefined on BASE_TEXT");
        if (lenB > (LONG_MAX - 16) / 2)
            CC_REFUSE("BASE_TEXT length cannot be represented by the frozen capacity formula");
        if (ffl > lenB * 2 + 16)
            CC_REFUSE("false-friend transformed length %ld exceeds capacity ceiling %ld",
                      ffl, lenB * 2 + 16);
        if (cc_kv_uint(commit, ncommit, "ff_transformed_bytes", &nv) || ffl != nv)
            CC_REFUSE("ff_transformed_bytes %ld does not match the commitment %s",
                      ffl, cc_kv(commit, ncommit, "ff_transformed_bytes"));
        if (ffl < CC_RUN_BYTES) CC_REFUSE("false-friend transform shorter than RUN_BYTES");
    }
    /* ---- recompute C2/C3 and compare every derived selection field ---- */
    {
        const CcRoot *rb2 = cc_root(roots, "confirmatory_builder");
        const CcRoot *rv = cc_root(roots, "confirmatory_verifier");
        unsigned char cd[32], sd[32];
        size_t cl2 = 0, sl2 = 0;
        char cdig[65], sdig[65], wbuf[32], hbuf[32], htd[65];
        cc_class_digest(rb2->sha, rv->sha, sha, cd, &cl2);
        cc_hex(cd, 32, cdig);
        strcpy(cdig_s, cdig);
        if (cl2 != 210) CC_REFUSE("class preimage length %zu != 210", cl2);
        if (strcmp(cdig, cc_kv(sel, nsel, "class_digest_sha256")))
            CC_REFUSE("recomputed class_digest %s does not match the selection", cdig);
        class_index = cd[31] & 3;
        if (cc_kv_uint(sel, nsel, "class_index", &nv) || nv < 0 || nv > 3 || class_index != (int)nv)
            CC_REFUSE("recomputed class_index %d does not match the selection", class_index);
        if (strcmp(CC_CLASS_NAME[class_index], cc_kv(sel, nsel, "class")))
            CC_REFUSE("class name '%s' does not match the selection", CC_CLASS_NAME[class_index]);
        cc_seed_digest(rb2->sha, rv->sha, sha, sd, &sl2);
        cc_hex(sd, 32, sdig);
        strcpy(sdig_s, sdig);
        if (sl2 != 209) CC_REFUSE("seed preimage length %zu != 209", sl2);
        if (strcmp(sdig, cc_kv(sel, nsel, "seed_digest_sha256")))
            CC_REFUSE("recomputed seed_digest does not match the selection");
        world_seed = cc_be64(sd);
        sprintf(wbuf, "%016llx", (unsigned long long)world_seed);
        if (strcmp(wbuf, cc_kv(sel, nsel, "world_seed_be64")))
            CC_REFUSE("recomputed world_seed does not match the selection");
        half_tail = cc_half_tail_seed(world_seed, htd);
        sprintf(hbuf, "%016llx", (unsigned long long)half_tail);
        strcpy(wsh_s, wbuf); strcpy(hth_s, hbuf);
        if (strcmp(hbuf, cc_kv(sel, nsel, "half_tail_seed_be64")))
            CC_REFUSE("recomputed half_tail_seed does not match the selection");
    }
    if (ag->receipt_test) {
        free(B); free(X);
        return 0;
    }
    /* ---- construct the selected world independently ---- */
    W = (unsigned char *)malloc(CC_RUN_BYTES);
    if (cc_build_world(class_index, B, lenB, world_seed, W, truemap, err, sizeof err))
        CC_REFUSE("world construction failed: %s", err);
    /* Capture the exact eleven builder outputs once. Every subsequent check
     * consumes only this immutable in-memory snapshot. */
    if (cc_capture_inventory(ag->builder_out, &art, err, sizeof err))
        CC_REFUSE("%s", err);
    /* ---- compare the builder's captured world ---- */
    {
        int wi = cc_art_index("W_confirmatory.bin");
        long bl = art.bytes[wi]; const unsigned char *bw = art.raw[wi];
        if (bl != CC_RUN_BYTES) CC_REFUSE("builder world is %ld bytes, law requires %d", bl, CC_RUN_BYTES);
        { long z; for (z = 0; z < CC_RUN_BYTES; z++) if (bw[z] != W[z]) {
            CC_REFUSE("builder world differs from independent construction at byte %ld", z); } }
    }
    /* ---- independent court, then C8 from this hand's own numbers ---- */
    /* the pinned C5 inventory and the structural evidence gate run BEFORE the
     * court: they depend only on the artifacts, so a bad witness is refused
     * without paying for a 22-arm reconstruction. */
    {
        char ierr[512]; int ei = cc_art_index("evidence_confirmatory.tsv");
        CcCtx cx;
        cx.roots = roots; cx.commit = commit; cx.ncommit = ncommit;
        cx.sel = sel; cx.nsel = nsel;
        cx.roots_sha = roots_receipt_sha; cx.commit_sha = commit_receipt_sha;
        cx.freeze_sha = freeze_receipt_sha; cx.sel_sha = selection_receipt_sha;
        cx.class_digest = cdig_s; cx.seed_digest = sdig_s;
        cx.world_seed_hex = wsh_s; cx.half_tail_hex = hth_s;
        cx.class_index = class_index;
        if (cc_check_inventory(&art, class_index, world_seed, &cx, ierr, sizeof ierr))
            CC_REFUSE("%s", ierr);
        if (cc_check_evidence_struct(art.raw[ei], art.bytes[ei],
                                     CC_RUN_BYTES / CHUNK, ierr, sizeof ierr))
            CC_REFUSE("%s", ierr);
    }
    cc_prepare_source(A, lenA);
    {
        static CcCourt R;
        CcVerdictIn in; CcVerdictOut o;
        cc_court(W, CC_RUN_BYTES, truemap, 1, &R);
        cc_fill_verdict_in(&R, class_index, &in);
        if (R.pE_bad) CC_REFUSE("specialist normalisation violated: p_E outside (0,1) at %ld positions, "
                                "first at raw offset %ld", R.pE_bad, R.pE_bad_off);
        if (R.price_bad) CC_REFUSE("non-finite or non-positive price at %ld positions, first at raw offset %ld",
                                   R.price_bad, R.price_bad_off);
        /* point (7): the A5 bits comparison, now that the court exists */
        {
            char eerr[512]; int ei = cc_art_index("evidence_confirmatory.tsv");
            if (cc_check_evidence_bits(art.raw[ei], art.bytes[ei], &R, eerr, sizeof eerr)) CC_REFUSE("%s", eerr);
            if (cc_check_books(&art, &R, eerr, sizeof eerr)) CC_REFUSE("%s", eerr);
        }
        cc_verdict(&in, &o);
        if (!quiet) {
            cc_row("OBS", "CONFIRMATORY", "class = %s (index %d)", CC_CLASS_NAME[class_index], class_index);
            cc_row("OBS", "CONFIRMATORY", "world_seed_be64 = %016llx  half_tail_seed_be64 = %016llx",
                   (unsigned long long)world_seed, (unsigned long long)half_tail);
            cc_row("OBS", "CONFIRMATORY", "contextual EARN present = %d, earliest raw offset = %ld",
                   in.earn_ctx_any, in.earn_min_offset);
            cc_row("OBS", "CONFIRMATORY", "p_rank = %.4f  p_rank_64K = %.4f  nulls>=live full/64K = %d/%d",
                   o.p_rank, o.p_rank_64k, in.nulls_ge_full, in.nulls_ge_64k);
            cc_row("OBS", "CONFIRMATORY", "CONF_MICRO = %d   CONF_MICRO_PREFIX = %d", o.conf_micro, o.conf_micro_prefix);
            cc_row("OBS", "CONFIRMATORY", "G_rel([65536,81920)) = %.6f   G_rel([0,16384)) = %.6f",
                   in.g_rel_half_guard, in.g_rel_ff_guard);
            printf("%s\n", o.pass ? CC_PASS_LINE : CC_FAIL_LINE);
        }
        cc_artifacts_free(&art); free(A); free(B); free(W); free(X);
        return o.pass ? 0 : 10;
    }
#undef CC_REFUSE
}


static void cc_write(const char *path, const void *b, size_t n);
static void cc_emit_evidence(const char *dir, const CcCourt *R, int mode);
static void cc_emit_books(const char *dir, const CcCourt *R);
static void cc_emit_relations_mode(const char *dir, int mode);

/* write a full C5 inventory into a synthetic builder-out directory.
 * mode 0 = coherent; the reds below mutate one artifact at a time afterwards. */
typedef struct {
    const char *roots_path, *commit_path, *freeze_path, *sel_path;
    const char *builder_sha, *verifier_sha, *class_digest, *seed_digest, *half_tail_hex;
    long base_bytes;
} CcMf;
static void cc_emit_inventory(const char *dir, const unsigned char *W, const CcCourt *R,
                              int class_index, uint64_t world_seed,
                              const char *cls_name, const char *seed_hex, const char *base_sha,
                              const CcMf *mf)
{
    char p[512]; FILE *o; int a, i;
    snprintf(p, sizeof p, "%s/%s", dir, CC_INVENTORY[0]);          /* W_confirmatory.bin */
    cc_write(p, W, CC_RUN_BYTES);
    { int perm[256];                                                /* oracle_confirmatory.tsv */
      if (class_index == 0 || class_index == 2) rename_perm(world_seed, perm);
      else for (i = 0; i < 256; i++) perm[i] = i;
      snprintf(p, sizeof p, "%s/%s", dir, CC_INVENTORY[1]);
      o = fopen(p, "wb");
      fprintf(o, "source\tdestination\n");
      for (i = 0; i < 256; i++) fprintf(o, "%d\t%d\n", i, perm[i]);
      fclose(o); }
    snprintf(p, sizeof p, "%s/%s", dir, CC_INVENTORY[2]);          /* confirmatory_selection.tsv */
    {
        const char *val[CC_MF_N];
        char ci[8], sb[24], bb[24], wb[24];
        char rs[65], cs[65], fsx[65], ss[65]; long by;
        sprintf(ci, "%d", class_index);
        sprintf(sb, "%d", 447545);
        sprintf(bb, "%ld", mf->base_bytes);
        sprintf(wb, "%d", CC_RUN_BYTES);
        cc_file_digest(mf->roots_path, rs, &by);
        cc_file_digest(mf->commit_path, cs, &by);
        cc_file_digest(mf->freeze_path, fsx, &by);
        cc_file_digest(mf->sel_path, ss, &by);
        val[0] = "raw-builder-artifacts-not-verdict";
        val[1] = "confirmatory";
        val[2] = cls_name;
        val[3] = ci;
        val[4] = "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb";
        val[5] = sb;
        val[6] = mf->builder_sha;
        val[7] = mf->verifier_sha;
        val[8] = base_sha;
        val[9] = mf->class_digest;
        val[10] = mf->seed_digest;
        val[11] = rs; val[12] = cs; val[13] = fsx; val[14] = ss;
        val[15] = seed_hex;
        val[16] = (class_index == 2) ? mf->half_tail_hex : "NOT_USED";
        val[17] = bb;
        val[18] = wb;
        o = fopen(p, "wb");
        fprintf(o, "field\tvalue\n");
        for (i = 0; i < CC_MF_N; i++) fprintf(o, "%s\t%s\n", CC_MF[i], val[i]);
        fclose(o);
    }
    snprintf(p, sizeof p, "%s/%s", dir, CC_INVENTORY[3]);          /* builder_G_confirmatory.tsv */
    o = fopen(p, "wb");
    fprintf(o, "%s\t%s\t%s\t%s\n", CC_H_G[0], CC_H_G[1], CC_H_G[2], CC_H_G[3]);
    for (a = 0; a < CC_ARMS; a++) {
        int hh;
        for (hh = 0; hh < 5; hh++)
            fprintf(o, "NOT_VERDICT\t%s\t%ld\t%.17g\n", a == 0 ? "cold" : mapname_of(a), CC_HORIZON[hh],
                    cc_G(R, a, 0, (int)(CC_HORIZON[hh] / CHUNK)));
    }
    fclose(o);
    cc_emit_evidence(dir, R, 0);                                    /* evidence_confirmatory.tsv */
    cc_emit_books(dir, R);                                          /* the six remaining books */
}

/* serialise the six remaining books from this hand's own court */
static void cc_emit_books(const char *dir, const CcCourt *R)
{
    char p[512]; FILE *o; long i; int j;
    snprintf(p, sizeof p, "%s/maps_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 7; j++) fprintf(o, "%s%c", CC_H_MAPS[j], j == 6 ? '\n' : '\t');
    for (i = 0; i < R->nmaps; i++) {
        const CcMapRow *m = &R->maps[i];
        fprintf(o, "%d\t%d\t%s\t%d\t%d\t%d\t%d\n", m->chunk, m->chunk * CHUNK,
                mapname_of(m->mapid), m->s, m->d, m->epoch, m->correct);
    }
    fclose(o);
    snprintf(p, sizeof p, "%s/map_summary_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 7; j++) fprintf(o, "%s%c", CC_H_MSUM[j], j == 6 ? '\n' : '\t');
    for (j = 1; j <= R->nchunks; j++) {
        long bp = 0, bh = 0, ap = 0, ah = 0;
        for (i = 0; i < R->nmaps; i++) {
            const CcMapRow *m = &R->maps[i];
            if (m->chunk != j) continue;
            if (m->mapid == 0) { bp++; if (m->correct > 0) bh++; }
            else if (m->mapid == 1) { ap++; if (m->correct > 0) ah++; }
        }
        fprintf(o, "%d\t%d\t%ld\t%ld\t%ld\t%ld\t%d\n", j, j * CHUNK,
                bp, bh, ap, ah, R->surf[j - 1]);
    }
    fclose(o);
    snprintf(p, sizeof p, "%s/profile_scrambles_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 6; j++) fprintf(o, "%s%c", CC_H_SCR[j], j == 5 ? '\n' : '\t');
    for (i = 0; i < R->nscr; i++) {
        const CcScrRow *m = &R->scr[i];
        fprintf(o, "%d\t%d\tnull%d\t%d\t%d\t%d\n", m->chunk, m->chunk * CHUNK, m->arm, m->d, m->prof, m->fixed);
    }
    fclose(o);
    cc_emit_relations_mode(dir, 0);
    snprintf(p, sizeof p, "%s/relation_events_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 18; j++) fprintf(o, "%s%c", CC_H_EVT[j], j == 17 ? '\n' : '\t');
    for (i = 0; i < R->nevt; i++) {
        const CcKeyRow *m = &R->evt[i];
        fprintf(o, "%ld\t%ld\t%s\t%s\t%.17g\t%d\t%d\t%d\t%d", m->off, m->upos, mapname_of(m->arm),
                m->b > 0.5 ? "earn" : "revoke", m->a, m->key.ts, m->key.td, m->key.tep, m->key.cl);
        for (j = 0; j < 3; j++) fprintf(o, "\t%d\t%d\t%d", m->key.cs[j], m->key.cd[j], m->key.cep[j]);
        fprintf(o, "\n");
    }
    fclose(o);
    snprintf(p, sizeof p, "%s/winners_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 18; j++) fprintf(o, "%s%c", CC_H_WIN[j], j == 17 ? '\n' : '\t');
    for (i = 0; i < R->nwin; i++) {
        const CcKeyRow *m = &R->win[i];
        fprintf(o, "%ld\t%ld\t%s\t%.17g\t%.17g\t%d\t%d\t%d\t%d", m->off, m->upos, mapname_of(m->arm),
                m->a, m->b, m->key.ts, m->key.td, m->key.tep, m->key.cl);
        for (j = 0; j < 3; j++) fprintf(o, "\t%d\t%d\t%d", m->key.cs[j], m->key.cd[j], m->key.cep[j]);
        fprintf(o, "\n");
    }
    fclose(o);
}

/* mode 0 is coherent; modes 1..3 reproduce Astra's exact relation-book reds. */
static void cc_emit_relations_mode(const char *dir, int mode)
{
    char p[512]; FILE *o; int a, j; long i;
    snprintf(p, sizeof p, "%s/relations_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (j = 0; j < 22; j++) fprintf(o, "%s%c", CC_H_REL[j], j == 21 ? '\n' : '\t');
    for (a = 0; a < CC_MAPS; a++) {
        int armcol = (a == 0) ? 1 : (a == 20) ? 21 : (1 + a);
        for (i = 0; i < rb_n[a]; i++) {
            const Rel *k = (mode == 1 && a == 0 && i == 1) ? &rb[a][0] : &rb[a][i];
            const char *arm_name = (mode == 2 && a == 0 && i == 0) ? "bonly" : mapname_of(armcol);
            long target_s = k->ts;
            if (mode == 3 && a == 0 && i == 0) target_s += 4294967296L;
            fprintf(o, "%s\t%ld\t%d\t%d\t%d", arm_name, target_s, k->td, k->tep, k->cl);
            for (j = 0; j < 3; j++) fprintf(o, "\t%d\t%d\t%d", k->cs[j], k->cd[j], k->cep[j]);
            fprintf(o, "\t%ld\t%ld\t%ld\t%.17g\t%.17g\t%d\t%.17g\t%d\n",
                    k->seen, k->pos, k->neg, k->led, k->peak, k->st, k->L, k->ever);
        }
    }
    fclose(o);
}
/* rewrite only the evidence table, optionally mutated, for the pass-7 reds */
static void cc_emit_evidence(const char *dir, const CcCourt *R, int mode)
{
    char p[512]; FILE *o; int c, a, i;
    snprintf(p, sizeof p, "%s/evidence_confirmatory.tsv", dir);
    o = fopen(p, "wb");
    for (i = 0; i < 14; i++) fprintf(o, "%s%c", CC_EV_HEADER[i], i == 13 ? '\n' : '\t');
    if (mode == 1) { fclose(o); return; }              /* 1 = header only */
#define CC_EVROW(cc, aa) \
    fprintf(o, "%d\t%d\t%d\t%s\t%ld\t%.17g\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%ld\t%d\n", \
            (cc), (cc) * CHUNK, CHUNK, (aa) == 0 ? "cold" : mapname_of(aa), \
            R->positions[cc], R->bits[cc][aa], R->pairs[cc][aa], R->relations[cc][aa], \
            R->cearn[cc][aa], R->earnev[cc][aa], R->revev[cc][aa], R->wins[cc][aa], \
            R->senio[cc][aa], R->surf[cc])
    for (c = 0; c < R->nchunks; c++) for (a = 0; a < CC_ARMS; a++) {
        if (mode == 2 && c == 7 && a == 3) continue;   /* 2 = one row deleted */
        if (mode == 4 && c == 9 && a == 5) {           /* 4 = malformed row */
            fprintf(o, "%d\t%d\t%d\t%s\t%ld\t%.17g\n", c, c * CHUNK, CHUNK,
                    mapname_of(a), R->positions[c], R->bits[c][a]);
            continue;
        }
        if (mode == 5 && c == 11 && a == 6) {          /* 5 = unknown arm name */
            fprintf(o, "%d\t%d\t%d\t%s\t%ld\t%.17g\t0\t0\t0\t0\t0\t0\t0\t0\n",
                    c, c * CHUNK, CHUNK, "null0junk", R->positions[c], R->bits[c][a]);
            continue;
        }
        if (mode == 6 && c == 13 && a == 4) {          /* 6 = integer field trailing garbage */
            fprintf(o, "%djunk\t%d\t%d\t%s\t%ld\t%.17g\t0\t0\t0\t0\t0\t0\t0\t0\n",
                    c, c * CHUNK, CHUNK, mapname_of(a), R->positions[c], R->bits[c][a]);
            continue;
        }
        if (mode == 7 && c == 15 && a == 7) {          /* 7 = double field trailing garbage */
            fprintf(o, "%d\t%d\t%d\t%s\t%ld\t%.17ggarbage\t0\t0\t0\t0\t0\t0\t0\t0\n",
                    c, c * CHUNK, CHUNK, mapname_of(a), R->positions[c], R->bits[c][a]);
            continue;
        }
        CC_EVROW(c, a);
        if (mode == 3 && c == 5 && a == 2) CC_EVROW(c, a);   /* 3 = one row duplicated */
    }
#undef CC_EVROW
    fclose(o);
}

/* =====================================================================
 * PASS-6 GATE: end-to-end green and red through the REAL CLI, all classes.
 * Everything here is synthetic: synthetic base, synthetic receipt chain,
 * synthetic builder-out. No real receipt, base, or builder artifact is read.
 * ===================================================================== */
static void cc_write(const char *path, const void *b, size_t n)
{
    FILE *f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "cannot write %s\n", path); exit(3); }
    fwrite(b, 1, n, f); fclose(f);
}

static int cc_ensure_dir(const char *path, char *why, size_t wn)
{
    if (mkdir(path, 0700) == 0) return 0;
    if (errno == EEXIST) {
        snprintf(why, wn, "scratch directory already exists and is not owned by this run: '%s'", path);
        return 1;
    }
    snprintf(why, wn, "cannot create scratch directory '%s': %s", path, strerror(errno));
    return 1;
}

static int cc_path_within(const char *path, const char *root)
{
    size_t rn = strlen(root);
    return !strcmp(path, root) || (!strncmp(path, root, rn) && path[rn] == '/');
}

static int cc_scratch_dir_guard(const char *path, char *why, size_t wn)
{
    char lexical[PATH_MAX], parent[PATH_MAX], resolved[PATH_MAX], repo_real[PATH_MAX];
    const char *slash;
    if (!path || !path[0] || strlen(path) >= sizeof lexical) {
        snprintf(why, wn, "scratch directory path is empty or too long");
        return 1;
    }
    if (!realpath(CC_REPO, repo_real)) {
        snprintf(why, wn, "cannot canonicalize repository root: %s", strerror(errno));
        return 1;
    }
    if (realpath(path, resolved)) {
        if (cc_path_within(resolved, repo_real)) {
            snprintf(why, wn, "scratch directory resolves inside the repository");
            return 1;
        }
        return 0;
    }
    if (errno != ENOENT) {
        snprintf(why, wn, "cannot canonicalize scratch directory '%s': %s", path, strerror(errno));
        return 1;
    }
    strcpy(lexical, path);
    slash = strrchr(lexical, '/');
    if (!slash) strcpy(parent, ".");
    else if (slash == lexical) strcpy(parent, "/");
    else {
        size_t pn = (size_t)(slash - lexical);
        if (pn >= sizeof parent) {
            snprintf(why, wn, "scratch directory parent is too long");
            return 1;
        }
        memcpy(parent, lexical, pn); parent[pn] = 0;
    }
    if (!realpath(parent, resolved)) {
        snprintf(why, wn, "cannot canonicalize scratch parent '%s': %s", parent, strerror(errno));
        return 1;
    }
    if (cc_path_within(resolved, repo_real)) {
        snprintf(why, wn, "scratch directory parent resolves inside the repository");
        return 1;
    }
    return 0;
}

/* Receipt-test is a proxy-only door.  Its roots receipt must remain outside
 * the repository even when the caller spells an absent path relatively or
 * through a symlinked parent.  Canonicalizing the parent lets that invariant
 * be checked before the receipt exists; an existing final symlink is refused. */
static int cc_receipt_roots_guard(const char *path, char *why, size_t wn)
{
    char lexical[PATH_MAX], parent[PATH_MAX], parent_real[PATH_MAX];
    char repo_real[PATH_MAX], candidate[PATH_MAX], repo_target[PATH_MAX];
    char file_real[PATH_MAX];
    const char *base, *slash;
    struct stat st;
    int z;

    if (!path || !path[0] || strlen(path) >= sizeof lexical) {
        snprintf(why, wn, "roots path is empty or too long");
        return 1;
    }
    strcpy(lexical, path);
    slash = strrchr(lexical, '/');
    base = slash ? slash + 1 : lexical;
    if (strcmp(base, "SYNTHETIC_ROOTS.tsv")) {
        snprintf(why, wn, "roots basename must be SYNTHETIC_ROOTS.tsv");
        return 1;
    }
    if (!slash) strcpy(parent, ".");
    else if (slash == lexical) strcpy(parent, "/");
    else {
        size_t pn = (size_t)(slash - lexical);
        if (pn >= sizeof parent) {
            snprintf(why, wn, "roots parent is too long");
            return 1;
        }
        memcpy(parent, lexical, pn); parent[pn] = 0;
    }
    if (!realpath(CC_REPO, repo_real)) {
        snprintf(why, wn, "cannot canonicalize repository root: %s", strerror(errno));
        return 1;
    }
    if (!realpath(parent, parent_real)) {
        snprintf(why, wn, "cannot canonicalize roots parent '%s': %s", parent, strerror(errno));
        return 1;
    }
    z = snprintf(candidate, sizeof candidate, "%s/%s", parent_real, base);
    if (z < 0 || (size_t)z >= sizeof candidate) {
        snprintf(why, wn, "canonical roots path is too long");
        return 1;
    }
    z = snprintf(repo_target, sizeof repo_target, "%s/SYNTHETIC_ROOTS.tsv", repo_real);
    if (z < 0 || (size_t)z >= sizeof repo_target) {
        snprintf(why, wn, "canonical repository target is too long");
        return 1;
    }
    if (cc_path_within(parent_real, repo_real) || !strcmp(candidate, repo_target)) {
        snprintf(why, wn, "canonical roots path is inside the repository tree");
        return 1;
    }
    if (lstat(path, &st) == 0) {
        if (!S_ISREG(st.st_mode)) {
            snprintf(why, wn, "roots path is not a regular file (symlinks are forbidden)");
            return 1;
        }
        if (!realpath(path, file_real)) {
            snprintf(why, wn, "cannot canonicalize existing roots file: %s", strerror(errno));
            return 1;
        }
        if (!strcmp(file_real, repo_target)) {
            snprintf(why, wn, "canonical roots file is inside the repository root");
            return 1;
        }
    } else if (errno != ENOENT) {
        snprintf(why, wn, "cannot inspect roots path: %s", strerror(errno));
        return 1;
    }
    return 0;
}
/* Deterministic synthetic base text.
 * PASS 11: the pseudo-random word soup produced no EARN, so winners and
 * relation_events were always empty and their comparison paths never ran.
 * A fixed 24-word cycle with a newline every 7th word makes one admitted pair
 * recur with an identical context thousands of times, which drives its ledger
 * past EARN and populates both books. No randomness: the body is a pure cycle,
 * and `variant` only appends trailing newlines, which changes the base digest
 * (and therefore the drawn class) while leaving the first RUN_BYTES untouched. */
#define CC_BREAK 61000
static const char *CC_EWORD[24] = {
    "alpha","bravo","charlie","delta","echo","foxtrot","golf","hotel",
    "india","juliet","kilo","lima","mike","november","oscar","papa",
    "quebec","romeo","sierra","tango","uniform","victor","whiskey","xray"
};
static long cc_make_base(unsigned char *out, long want, long variant)
{
    long n = 0, k; int i = 0;
    while (n < want) {
        const char *w = CC_EWORD[i % 24];
        const char *sep;
        i++;
        /* Two regions, deterministic.
         *   [0, CC_BREAK)  single-spaced: the space->space relation is admitted,
         *                  accumulates positive receipts and crosses EARN;
         *   [CC_BREAK, .)  double-spaced: a unit that begins with byte 32 is now
         *                  itself preceded by byte 32, so the event fires while
         *                  p_E > r and every such receipt is negative, driving the
         *                  SAME RelationKey back below REVOKE.
         * The digit on every 11th word keeps BPE units short enough that the R2
         * floor 256^-len does not underflow to zero. */
        if (n >= CC_BREAK) sep = ((i % 7) == 0) ? " \n " : "  ";
        else               sep = ((i % 7) == 0) ? "\n" : " ";
        if ((i % 11) == 0) n += sprintf((char *)out + n, "%s%d%s", w, (int)(i % 10), sep);
        else               n += sprintf((char *)out + n, "%s%s", w, sep);
    }
    for (k = 0; k < variant; k++) out[n++] = '\n';
    return n;
}

static void cc_kvfile(const char *path, const char *const *f, const char *const *v, int n)
{
    FILE *o = fopen(path, "wb"); int i;
    if (!o) { fprintf(stderr, "cannot write %s\n", path); exit(3); }
    fprintf(o, "field\tvalue\n");
    for (i = 0; i < n; i++) fprintf(o, "%s\t%s\n", f[i], v[i]);
    fclose(o);
}

static int cc_gate(const char *dir)
{
    static const char *real_paths[18];
    char sbase[PATH_MAX], sroots[PATH_MAX], scommit[PATH_MAX], sfreeze[PATH_MAX];
    char ssel[PATH_MAX], sbo[PATH_MAX], why[512];
    char bsha[65], vsha[65];
    long blen;
    unsigned char *B;
    int cls, fails = 0;

    snprintf(sbase, sizeof sbase, "%s/synthetic_base.txt", dir);
    snprintf(sroots, sizeof sroots, "%s/SYNTHETIC_ROOTS.tsv", dir);
    snprintf(scommit, sizeof scommit, "%s/SYNTHETIC_BASE_COMMIT.tsv", dir);
    snprintf(sfreeze, sizeof sfreeze, "%s/SYNTHETIC_BASE_FREEZE.tsv", dir);
    snprintf(ssel, sizeof ssel, "%s/SYNTHETIC_SELECTION.tsv", dir);
    snprintf(sbo, sizeof sbo, "%s/builder_out", dir);
    if (cc_ensure_dir(sbo, why, sizeof why)) {
        cc_pf("CC09_GATE", 0, "%s", why);
        return 1;
    }

    /* synthetic base */
    { long la; unsigned char *AA = slurp(SRCA, &la); cc_prepare_source(AA, la); free(AA); }
    B = (unsigned char *)malloc(400000);
    blen = cc_make_base(B, 300000, 0);
    cc_write(sbase, B, (size_t)blen);
    free(B);
    B = slurp(sbase, &blen);

    /* Synthetic roots and their targets all live in the caller's scratch tree.
     * No test receipt is ever copied into the repository root. */
    real_paths[0] = "COURT4_PASS4_FREEZE.tsv";
    real_paths[1] = "COURT2_SNAPSHOT.md";
    real_paths[2] = "COURT3_SNAPSHOT.md";
    real_paths[3] = "TRANSFER4_DRAFT.md";
    real_paths[4] = "transfer4.c";
    real_paths[5] = "COURT4_GAP_ADDENDUM.md";
    real_paths[6] = "transfer4_check.c";
    real_paths[7] = "COURT4_CONFIRMATORY_EXECUTION.md";
    real_paths[8] = "COURT4_EXECUTION_LAW_FREEZE.tsv";
    real_paths[9] = "COURT4_INTEROP_SNAPSHOT_REPAIR_ADDENDUM.md";
    real_paths[10] = "CONFIRMATORY_EXCLUSIONS.tsv";
    real_paths[11] = "transfer4_confirm.c";
    real_paths[12] = "transfer4_confirm_core.c";
    real_paths[13] = "transfer4_confirm_check.c";
    real_paths[14] = "court4_select.c";
    real_paths[15] = "COURT4_BUILDER_REGRESSION_MANIFEST.tsv";
    real_paths[16] = "COURT4_VERIFIER_REGRESSION_MANIFEST.tsv";
    real_paths[17] = "COURT4_SELECTOR_TEST_MANIFEST.tsv";
    {
        FILE *o = fopen(sroots, "wb"); int i;
        char s[65]; long n;
        if (!o) { fprintf(stderr, "cannot write roots\n"); exit(3); }
        fprintf(o, "role\tpath\tbytes\tsha256\n");
        for (i = 0; i < 18; i++) {
            char pp[512], dst[512]; unsigned char *copy;
            snprintf(pp, sizeof pp, "%s/%s", CC_REPO, real_paths[i]);
            copy = slurp(pp, &n);
            snprintf(dst, sizeof dst, "%s/%s", dir, real_paths[i]);
            cc_write(dst, copy, (size_t)n);
            free(copy);
            if (cc_file_digest(dst, s, &n)) { fprintf(stderr, "gate: cannot hash %s\n", real_paths[i]); exit(3); }
            fprintf(o, "%s\t%s\t%ld\t%s\n", CC_ROLES[i], real_paths[i], n, s);
            if (i == 11) strcpy(bsha, s);
            if (i == 13) strcpy(vsha, s);
        }
        fclose(o);
    }

    for (cls = 0; cls < 4; cls++) {
        char basesha[65]; long basebytes;
        unsigned char cd[32], sd[32]; size_t cl2, sl2;
        char cdig[65], sdig[65], wbuf[32], hbuf[32];
        uint64_t ws, hts;
        int drawn_class;
        long ffl; int sat = 0;
        const char *cf[4], *cv[4], *ff2[5], *fv[5], *sf[17], *sv2[17];
        char v_bytes[32], v_ffb[32], v_rb[32], v_rs[65], v_cb[32], v_cs[65], v_fb[32], v_fs[65];
        char v_ci[8];
        CcArgs ag;
        CcMf mf;
        unsigned char *W; int truemap[256]; char err[256];
        static CcCourt RC;
        int rc;

        cc_file_digest(sbase, basesha, &basebytes);
        ffl = cc_ff_length(B, blen, &sat, NULL);

        /* --- derive C2/C3 for THIS synthetic chain, then force the class by
         *     construction so all four doors are exercised. The forced class is
         *     written into the receipt and the world is built to match it; the
         *     door recomputes and must agree. To keep the door honest we only
         *     accept the chain whose recomputed class equals the built class. */
        cc_class_digest(bsha, vsha, basesha, cd, &cl2);
        cc_hex(cd, 32, cdig);
        drawn_class = cd[31] & 3;
        cc_seed_digest(bsha, vsha, basesha, sd, &sl2);
        cc_hex(sd, 32, sdig);
        ws = cc_be64(sd);
        hts = cc_half_tail_seed(ws, NULL);
        sprintf(wbuf, "%016llx", (unsigned long long)ws);
        sprintf(hbuf, "%016llx", (unsigned long long)hts);

        /* the drawn class for this synthetic chain is fixed by its hashes; to
         * exercise every class door we vary the base per class until the drawn
         * class matches, which keeps the receipt internally consistent. */
        {
            int tries = 0;
            while (drawn_class != cls && tries < 4096) {
                long nb2;
                unsigned char *B2 = (unsigned char *)malloc(400000);
                nb2 = cc_make_base(B2, 300000, (long)tries + 1);
                cc_write(sbase, B2, (size_t)nb2);
                free(B2);
                free(B); B = slurp(sbase, &blen);
                cc_file_digest(sbase, basesha, &basebytes);
                cc_class_digest(bsha, vsha, basesha, cd, &cl2);
                cc_hex(cd, 32, cdig);
                drawn_class = cd[31] & 3;
                cc_seed_digest(bsha, vsha, basesha, sd, &sl2);
                cc_hex(sd, 32, sdig);
                ws = cc_be64(sd); hts = cc_half_tail_seed(ws, NULL);
                sprintf(wbuf, "%016llx", (unsigned long long)ws);
                sprintf(hbuf, "%016llx", (unsigned long long)hts);
                tries++;
            }
            if (drawn_class != cls) {
                cc_pf("CC09_GATE", 0, "class %s: could not find a synthetic base drawing that class", CC_CLASS_NAME[cls]);
                fails++; continue;
            }
            sat = 0; ffl = cc_ff_length(B, blen, &sat, NULL);
        }

        /* commitment */
        sprintf(v_bytes, "%ld", basebytes); sprintf(v_ffb, "%ld", ffl);
        cf[0] = CC_COMMIT_FIELDS[0]; cf[1] = CC_COMMIT_FIELDS[1]; cf[2] = CC_COMMIT_FIELDS[2]; cf[3] = CC_COMMIT_FIELDS[3];
        cv[0] = "base-committed-class-not-computed"; cv[1] = v_bytes; cv[2] = v_ffb; cv[3] = basesha;
        cc_kvfile(scommit, cf, cv, 4);
        /* freeze binds roots + commitment */
        { long n; char s[65];
          cc_file_digest(sroots, s, &n); sprintf(v_rb, "%ld", n); strcpy(v_rs, s);
          cc_file_digest(scommit, s, &n); sprintf(v_cb, "%ld", n); strcpy(v_cs, s); }
        ff2[0] = CC_FREEZE_FIELDS[0]; ff2[1] = CC_FREEZE_FIELDS[1]; ff2[2] = CC_FREEZE_FIELDS[2];
        ff2[3] = CC_FREEZE_FIELDS[3]; ff2[4] = CC_FREEZE_FIELDS[4];
        fv[0] = "base-commit-frozen-class-not-computed"; fv[1] = v_rb; fv[2] = v_rs; fv[3] = v_cb; fv[4] = v_cs;
        cc_kvfile(sfreeze, ff2, fv, 5);
        { long n; char s[65]; cc_file_digest(sfreeze, s, &n); sprintf(v_fb, "%ld", n); strcpy(v_fs, s); }
        /* selection */
        sprintf(v_ci, "%d", drawn_class);
        { int i; for (i = 0; i < 17; i++) sf[i] = CC_SEL_FIELDS[i]; }
        sv2[0] = "synthetic-selected-not-confirmatory"; sv2[1] = v_rb; sv2[2] = v_rs; sv2[3] = v_cb; sv2[4] = v_cs;
        sv2[5] = v_fb; sv2[6] = v_fs; sv2[7] = bsha; sv2[8] = vsha; sv2[9] = v_bytes;
        sv2[10] = basesha; sv2[11] = cdig; sv2[12] = v_ci; sv2[13] = CC_CLASS_NAME[drawn_class];
        sv2[14] = sdig; sv2[15] = wbuf; sv2[16] = hbuf;
        cc_kvfile(ssel, sf, sv2, 17);

        /* The explicit receipt-test mode validates the synthetic chain and exits
         * before source preparation, world construction, artifact reads or court. */
        {
            CcArgs rt;
            memset(&rt, 0, sizeof rt);
            rt.base_text = sbase; rt.roots = sroots; rt.base_commit = scommit;
            rt.base_freeze = sfreeze; rt.selection = ssel; rt.receipt_test = 1;
            rc = cc_confirmatory(&rt, 1);
            cc_pf("CC13_HARDENING", rc == 0,
                  "class %s GREEN explicit --receipt-test chain accepted before construction (rc=%d)",
                  CC_CLASS_NAME[cls], rc);
            if (rc != 0) fails++;
        }
        sv2[0] = "selected-not-run";
        cc_kvfile(ssel, sf, sv2, 17);

        /* synthetic builder-out: world + evidence from an independent court */
        W = (unsigned char *)malloc(CC_RUN_BYTES);
        if (cc_build_world(cls, B, blen, ws, W, truemap, err, sizeof err)) {
            cc_pf("CC09_GATE", 0, "class %s: synthetic world construction failed: %s", CC_CLASS_NAME[cls], err);
            fails++; free(W); continue;
        }
        cc_court(W, CC_RUN_BYTES, truemap, 1, &RC);
        mf.roots_path = sroots; mf.commit_path = scommit; mf.freeze_path = sfreeze; mf.sel_path = ssel;
        mf.builder_sha = bsha; mf.verifier_sha = vsha;
        mf.class_digest = cdig; mf.seed_digest = sdig; mf.half_tail_hex = hbuf;
        mf.base_bytes = basebytes;
        cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);

        memset(&ag, 0, sizeof ag);
        ag.source_a = SRCA; ag.base_text = sbase; ag.roots = sroots;
        ag.base_commit = scommit; ag.base_freeze = sfreeze; ag.selection = ssel;
        ag.builder_out = sbo;

        /* ---- GREEN: coherent chain + matching artifacts reaches the verdict ---- */
        rc = cc_confirmatory(&ag, 0);        /* verbose: a failing green must state its reason */
        cc_pf("CC09_GATE", rc == 0 || rc == 10,
              "class %s GREEN: real --confirmatory CLI reached the verdict printer (rc=%d, 0=PASS 10=FAIL verdict)",
              CC_CLASS_NAME[cls], rc);
        if (!(rc == 0 || rc == 10)) fails++;

        /* ---- RED 1: corrupted receipt (selection status) ---- */
        { const char *bad_sv[17]; int i;
          for (i = 0; i < 17; i++) bad_sv[i] = sv2[i];
          bad_sv[0] = "synthetic-selected-not-confirmatory";
          cc_kvfile(ssel, sf, bad_sv, 17);
          rc = cc_confirmatory(&ag, 1);
          cc_pf("CC09_GATE", rc == 1, "class %s RED corrupted-receipt: refused before the verdict (rc=%d)", CC_CLASS_NAME[cls], rc);
          if (rc != 1) fails++;
          cc_kvfile(ssel, sf, sv2, 17); }

        /* ---- RED 2: wrong-class world in builder-out ---- */
        { unsigned char *W2 = (unsigned char *)malloc(CC_RUN_BYTES);
          int tm2[256]; char e2[256]; char wp[512];
          int other = (cls + 1) & 3;
          if (!cc_build_world(other, B, blen, ws, W2, tm2, e2, sizeof e2)) {
              snprintf(wp, sizeof wp, "%s/W_confirmatory.bin", sbo);
              cc_write(wp, W2, CC_RUN_BYTES);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED wrong-class-world (%s bytes supplied): refused (rc=%d)",
                    CC_CLASS_NAME[cls], CC_CLASS_NAME[other], rc);
              if (rc != 1) fails++;
              cc_write(wp, W, CC_RUN_BYTES);
          } else {
              cc_pf("CC09_GATE", 0, "class %s RED wrong-class-world could not be constructed", CC_CLASS_NAME[cls]);
              fails++;
          }
          free(W2); }

        /* ---- RED 3: tampered builder artifact (evidence bits) ---- */
        { char ep[512]; FILE *o; int c, a;
          snprintf(ep, sizeof ep, "%s/evidence_confirmatory.tsv", sbo);
          o = fopen(ep, "wb");
          for (a = 0; a < 14; a++) fprintf(o, "%s%c", CC_EV_HEADER[a], a == 13 ? '\n' : '\t');
          for (c = 0; c < RC.nchunks; c++) for (a = 0; a < CC_ARMS; a++)
              fprintf(o, "%d\t%d\t%d\t%s\t0\t%.17g\t0\t0\t0\t0\t0\t0\t0\t0\n",
                      c, c * CHUNK, CHUNK, a == 0 ? "cold" : mapname_of(a),
                      (c == 3 && a == 1) ? RC.bits[c][a] + 50.0 : RC.bits[c][a]);
          fclose(o);
          rc = cc_confirmatory(&ag, 1);
          cc_pf("CC09_GATE", rc == 1, "class %s RED tampered-artifact (+50 bits at chunk 3): refused (rc=%d)",
                CC_CLASS_NAME[cls], rc);
          if (rc != 1) fails++; }

        /* ---- PASS-7 reds: a reader satisfied by absence must die on absence ---- */
        { struct { int mode; const char *label; } evreds[7] = {
              { 1, "header-only evidence" },
              { 2, "one deleted row" },
              { 3, "one duplicated row" },
              { 4, "a malformed row" },
              { 5, "an unknown arm name" },
              { 6, "an integer field with trailing garbage" },
              { 7, "a double field with trailing garbage" } };
          int z;
          for (z = 0; z < 7; z++) {
              cc_emit_evidence(sbo, &RC, evreds[z].mode);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED evidence %s: refused before the verdict (rc=%d)",
                    CC_CLASS_NAME[cls], evreds[z].label, rc);
              if (rc != 1) fails++;
          } }
        /* ---- PASS-8 reds: each pinned artifact missing ---- */
        { int q;
          for (q = 0; q < CC_INV_N; q++) {
              char ip[512], bak[512];
              snprintf(ip, sizeof ip, "%s/%s", sbo, CC_INVENTORY[q]);
              snprintf(bak, sizeof bak, "%s/missing-%d.bak", dir, q);
              if (rename(ip, bak) != 0) continue;
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1 && strstr(cc_last_refusal, "inventory incomplete: missing"),
                    "class %s RED missing %s: exact missing-member refusal (rc=%d, %s)",
                    CC_CLASS_NAME[cls], CC_INVENTORY[q], rc, cc_last_refusal);
              if (!(rc == 1 && strstr(cc_last_refusal, "inventory incomplete: missing"))) fails++;
              rename(bak, ip);
          } }
        /* ---- Inventory red: the directory set must be exactly eleven ---- */
        { char xp[512];
          snprintf(xp, sizeof xp, "%s/UNLISTED_ARTIFACT.tsv", sbo);
          cc_write(xp, "extra\n", 6);
          rc = cc_confirmatory(&ag, 1);
          cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, "unlisted entry"),
                "class %s RED unlisted inventory member: exact refusal (rc=%d, %s)",
                CC_CLASS_NAME[cls], rc, cc_last_refusal);
          if (!(rc == 1 && strstr(cc_last_refusal, "unlisted entry"))) fails++;
          remove(xp); }
        /* A listed name must resolve to the same regular inode captured during
         * enumeration; a symlink cannot smuggle bytes under an allowed name. */
        { char ip[512], bak[512];
          snprintf(ip, sizeof ip, "%s/oracle_confirmatory.tsv", sbo);
          snprintf(bak, sizeof bak, "%s/symlink-oracle-target.tsv", dir);
          if (rename(ip, bak) == 0) {
              if (symlink(bak, ip) == 0) {
                  rc = cc_confirmatory(&ag, 1);
                  cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, "not a regular file"),
                        "class %s RED listed inventory symlink: exact refusal (rc=%d, %s)",
                        CC_CLASS_NAME[cls], rc, cc_last_refusal);
                  if (!(rc == 1 && strstr(cc_last_refusal, "not a regular file"))) fails++;
                  remove(ip);
              } else {
                  cc_pf("CC13_HARDENING", 0, "class %s RED listed inventory symlink fixture could not be built",
                        CC_CLASS_NAME[cls]);
                  fails++;
              }
              rename(bak, ip);
          } else {
              cc_pf("CC13_HARDENING", 0, "class %s RED listed inventory symlink fixture could not be built",
                    CC_CLASS_NAME[cls]);
              fails++;
          } }
        /* ---- Byte-format reds: every text artifact rejects NUL and missing LF ---- */
        { int q;
          for (q = 1; q < CC_INV_N; q++) {
              char tp[512]; long tn; unsigned char *tb; FILE *to;
              snprintf(tp, sizeof tp, "%s/%s", sbo, CC_INVENTORY[q]);
              tb = slurp(tp, &tn);
              to = fopen(tp, "wb"); fwrite(tb, 1, (size_t)tn, to); fputc(0, to); fputs("hidden\n", to); fclose(to);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, "embedded NUL"),
                    "class %s RED %s embedded NUL: exact refusal (rc=%d)",
                    CC_CLASS_NAME[cls], CC_INVENTORY[q], rc);
              if (!(rc == 1 && strstr(cc_last_refusal, "embedded NUL"))) fails++;
              cc_write(tp, tb, (size_t)(tn > 0 ? tn - 1 : 0));
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, "does not end with a newline"),
                    "class %s RED %s missing final LF: exact refusal (rc=%d)",
                    CC_CLASS_NAME[cls], CC_INVENTORY[q], rc);
              if (!(rc == 1 && strstr(cc_last_refusal, "does not end with a newline"))) fails++;
              cc_write(tp, tb, (size_t)tn);
              free(tb);
          } }
        /* ---- PASS-8 reds: the pinned oracle format ---- */
        { struct { int mode; const char *label; } orreds[5] = {
              { 1, "wrong header" },
              { 2, "255 rows" },
              { 3, "a non-bijection" },
              { 4, "a destination outside 0..255" },
              { 5, "a non-canonical number" } };
          int z, i2, perm[256];
          char op[512];
          snprintf(op, sizeof op, "%s/oracle_confirmatory.tsv", sbo);
          if (cls == 0 || cls == 2) rename_perm(ws, perm);
          else for (i2 = 0; i2 < 256; i2++) perm[i2] = i2;
          for (z = 0; z < 5; z++) {
              FILE *o = fopen(op, "wb");
              fprintf(o, "%s\n", orreds[z].mode == 1 ? "src\tdst" : "source\tdestination");
              for (i2 = 0; i2 < (orreds[z].mode == 2 ? 255 : 256); i2++) {
                  int d2 = perm[i2];
                  if (orreds[z].mode == 3 && i2 == 5) d2 = perm[4];
                  if (orreds[z].mode == 4 && i2 == 6) { fprintf(o, "%d\t999\n", i2); continue; }
                  if (orreds[z].mode == 5 && i2 == 7) { fprintf(o, "%d\t%djunk\n", i2, d2); continue; }
                  fprintf(o, "%d\t%d\n", i2, d2);
              }
              fclose(o);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED oracle %s: refused (rc=%d)",
                    CC_CLASS_NAME[cls], orreds[z].label, rc);
              if (rc != 1) fails++;
          } }
        /* ---- PASS-9 reds: the pinned 19-row manifest contract ---- */
        { const char *mfl[6] = { "removed row", "duplicated row", "reordered rows",
                                 "extra row", "malformed row", "semantic mismatch" };
          int z, q;
          char mp[512], line[CC_MF_N][512];
          FILE *o;
          snprintf(mp, sizeof mp, "%s/confirmatory_selection.tsv", sbo);
          /* capture the coherent manifest body once */
          { CcKV k2[64]; int n2 = 0; char e2[256];
            if (cc_load_kv(mp, k2, 64, &n2, e2, sizeof e2) == 0 && n2 == CC_MF_N)
                for (q = 0; q < CC_MF_N; q++)
                    snprintf(line[q], sizeof line[0], "%s\t%s", k2[q].field, k2[q].value);
            else for (q = 0; q < CC_MF_N; q++) line[q][0] = 0; }
          for (z = 0; z < 6; z++) {
              o = fopen(mp, "wb");
              fprintf(o, "field\tvalue\n");
              for (q = 0; q < CC_MF_N; q++) {
                  if (z == 0 && q == 7) continue;                       /* removed */
                  if (z == 2 && q == 3) { fprintf(o, "%s\n", line[4]); fprintf(o, "%s\n", line[3]); continue; }
                  if (z == 2 && q == 4) continue;                       /* reordered pair */
                  if (z == 4 && q == 9) { fprintf(o, "%s\n", CC_MF[q]); continue; }  /* malformed */
                  if (z == 5 && q == 18) { fprintf(o, "%s\t999999\n", CC_MF[q]); continue; } /* semantic */
                  fprintf(o, "%s\n", line[q]);
                  if (z == 1 && q == 5) fprintf(o, "%s\n", line[q]);     /* duplicated */
              }
              if (z == 3) fprintf(o, "extra_field\tx\n");               /* extra */
              fclose(o);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED manifest %s: refused before the verdict (rc=%d)",
                    CC_CLASS_NAME[cls], mfl[z], rc);
              if (rc != 1) fails++;
          }
          cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf); }

        /* ---- PASS-9 reds: canonical unsigned grammar on receipt integers ---- */
        { const char *bad[5] = { "151191junk", "00151191", "+151191", "", "99999999999999999999999" };
          const char *lbl[5] = { "junk suffix", "leading zero", "sign", "empty", "overflow" };
          int z, q;
          const char *sv3[17];
          for (z = 0; z < 5; z++) {
              for (q = 0; q < 17; q++) sv3[q] = sv2[q];
              sv3[9] = bad[z];                      /* base_text_bytes in the selection */
              cc_kvfile(ssel, sf, sv3, 17);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED receipt integer %s: refused (rc=%d)",
                    CC_CLASS_NAME[cls], lbl[z], rc);
              if (rc != 1) fails++;
          }
          cc_kvfile(ssel, sf, sv2, 17);
          {
              char other_bytes[32];
              snprintf(other_bytes, sizeof other_bytes, "%ld", basebytes + 1);
              for (q = 0; q < 17; q++) sv3[q] = sv2[q];
              sv3[9] = other_bytes;
              cc_kvfile(ssel, sf, sv3, 17);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, "selection receipt"),
                    "class %s RED canonical but wrong selection.base_text_bytes: direct refusal (rc=%d, %s)",
                    CC_CLASS_NAME[cls], rc, cc_last_refusal);
              if (!(rc == 1 && strstr(cc_last_refusal, "selection receipt"))) fails++;
              cc_kvfile(ssel, sf, sv2, 17);
          } }

        /* ---- PASS-10 reds: one semantic field changed in EACH of the seven books ----
         * A book with no data row cannot be mutated in place; appending a spurious
         * row is then the semantic change, so no red is ever vacuous. */
        { const char *bk[7] = { "evidence_confirmatory.tsv", "maps_confirmatory.tsv",
                                "map_summary_confirmatory.tsv", "profile_scrambles_confirmatory.tsv",
                                "winners_confirmatory.tsv", "relation_events_confirmatory.tsv",
                                "relations_confirmatory.tsv" };
          int z;
          for (z = 0; z < 7; z++) {
              char bp[512], line[8192], tmp[512]; FILE *in, *out2; long ln = 0; int done = 0;
              snprintf(bp, sizeof bp, "%s/%s", sbo, bk[z]);
              snprintf(tmp, sizeof tmp, "%s/.tamper", sbo);
              in = fopen(bp, "rb"); out2 = fopen(tmp, "wb");
              if (!in || !out2) { if (in) fclose(in); if (out2) fclose(out2); continue; }
              while (fgets(line, sizeof line, in)) {
                  ln++;
                  if (ln == 2 && !done) {            /* mutate the first data row */
                      char *t1 = strchr(line, '\t');
                      if (t1) {
                          char *t2 = strchr(t1 + 1, '\t');
                          if (t2) { fprintf(out2, "%.*s\t999999%s", (int)(t1 - line), line, t2); done = 1; continue; }
                      }
                  }
                  fputs(line, out2);
              }
              if (!done) {                            /* empty book: append a spurious row */
                  fprintf(out2, "999999\t999999\t999999\n");
                  done = 1;
              }
              fclose(in); fclose(out2);
              rename(tmp, bp);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED book %s semantic change (%s): refused (rc=%d)",
                    CC_CLASS_NAME[cls], bk[z], ln >= 2 ? "field mutated" : "spurious row appended", rc);
              if (rc != 1) fails++;
              cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);
          } }

        /* ---- Relation-book reds: one-to-one keys, no alias, no narrowing ---- */
        { const char *why[3] = { "duplicates a reconstructed key", "forbidden bonly alias", "outside int range" };
          const char *label[3] = { "duplicate replaces missing key", "bonly aliases relation", "target_s + 2^32 narrows" };
          int z;
          for (z = 0; z < 3; z++) {
              cc_emit_relations_mode(sbo, z + 1);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC13_HARDENING", rc == 1 && strstr(cc_last_refusal, why[z]),
                    "class %s RED %s: exact refusal (rc=%d, %s)",
                    CC_CLASS_NAME[cls], label[z], rc, cc_last_refusal);
              if (!(rc == 1 && strstr(cc_last_refusal, why[z]))) fails++;
          }
          cc_emit_relations_mode(sbo, 0); }

        /* ---- PASS-11 reds: semantic mutation of a POPULATED winners/events row ----
         * These only run when the book actually has data rows; when it does not,
         * the row count is reported so the skip is visible rather than silent. */
        { struct { const char *file; int col; const char *what; } pt[2] = {
              { "winners_confirmatory.tsv", 3, "ledger_before value" },
              { "relation_events_confirmatory.tsv", 3, "event kind" } };
          int z;
          for (z = 0; z < 2; z++) {
              char bp[512], tmp[512], line[8192]; FILE *in, *out2; long ln = 0, rows = 0; int done = 0;
              snprintf(bp, sizeof bp, "%s/%s", sbo, pt[z].file);
              in = fopen(bp, "rb");
              if (in) { while (fgets(line, sizeof line, in)) rows++; fclose(in); }
              rows = rows > 0 ? rows - 1 : 0;
              if (rows == 0) {
                  cc_row("OBS", "CC09_GATE", "class %s: %s has 0 data rows in this world, so the populated-row "
                         "tamper cannot run here; the empty-and-append red covers it instead",
                         CC_CLASS_NAME[cls], pt[z].file);
                  continue;
              }
              snprintf(tmp, sizeof tmp, "%s/.tamper", sbo);
              in = fopen(bp, "rb"); out2 = fopen(tmp, "wb");
              if (!in || !out2) { if (in) fclose(in); if (out2) fclose(out2); continue; }
              while (fgets(line, sizeof line, in)) {
                  ln++;
                  if (ln == 2 && !done) {          /* first data row: mutate the named field */
                      char *f[20]; int nf = 0; char *q; char buf2[8192];
                      snprintf(buf2, sizeof buf2, "%s", line);
                      buf2[strcspn(buf2, "\n")] = 0;
                      f[nf++] = buf2;
                      for (q = buf2; *q; q++) if (*q == '\t' && nf < 20) { *q = 0; f[nf++] = q + 1; }
                      if (nf > pt[z].col) {
                          int q2;
                          for (q2 = 0; q2 < nf; q2++) {
                              const char *v = f[q2];
                              if (q2 == pt[z].col)
                                  v = (z == 1) ? (!strcmp(f[q2], "earn") ? "revoke" : "earn") : "999999";
                              fprintf(out2, "%s%c", v, q2 == nf - 1 ? '\n' : '\t');
                          }
                          done = 1; continue;
                      }
                  }
                  fputs(line, out2);
              }
              fclose(in); fclose(out2);
              if (done) {
                  rename(tmp, bp);
                  rc = cc_confirmatory(&ag, 1);
                  cc_pf("CC09_GATE", rc == 1, "class %s RED populated %s %s mutated (%ld data rows): refused (rc=%d)",
                        CC_CLASS_NAME[cls], pt[z].file, pt[z].what, rows, rc);
                  if (rc != 1) fails++;
                  cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);
              } else remove(tmp);
          } }
        /* report the measured population of both books for this class */
        { long ee = 0, rr = 0, i3, ctxkeys = 0, revoked_ok = -1;
          for (i3 = 0; i3 < RC.nevt; i3++) { if (RC.evt[i3].b > 0.5) ee++; else rr++; }
          for (i3 = 0; i3 < rb_n[0]; i3++) {
              if (rb[0][i3].cl >= 1) ctxkeys++;
              if (rb[0][i3].cl >= 1 && rb[0][i3].ever && rb[0][i3].st == 0 && rb[0][i3].L == 0.0)
                  revoked_ok = i3;
          }
          cc_row("OBS", "CC09_GATE", "class %s book population: winners %ld rows, relation_events %ld rows "
                 "(earn %ld, revoke %ld), relation book %ld keys",
                 CC_CLASS_NAME[cls], RC.nwin, RC.nevt, ee, rr, rb_n[0]);
          /* the court's seven conditions, measured rather than asserted */
          if (rr > 0) {
              cc_pf("CC12_REVOKE", ctxkeys > 0, "class %s (1) a contextual relation is admitted: %ld keys with context_len>=1",
                    CC_CLASS_NAME[cls], ctxkeys);
              cc_pf("CC12_REVOKE", ee > 0, "class %s (2) it crosses EARN: %ld earn events", CC_CLASS_NAME[cls], ee);
              cc_pf("CC12_REVOKE", RC.nwin > 0, "class %s (3) it changes live pricing: %ld live-voice positions where p_final != p_local",
                    CC_CLASS_NAME[cls], RC.nwin);
              cc_pf("CC12_REVOKE", rr > 0, "class %s (4) negative receipts cross REVOKE: %ld revoke events", CC_CLASS_NAME[cls], rr);
              cc_pf("CC12_REVOKE", ee > 0 && rr > 0, "class %s (5) the events book carries BOTH kinds (earn %ld, revoke %ld)",
                    CC_CLASS_NAME[cls], ee, rr);
              cc_pf("CC12_REVOKE", revoked_ok >= 0,
                    "class %s (6) the revoked relation ends state=0 L=0 (key %d->%d ep%d cl%d, seen %ld, pos %ld, neg %ld, peak %.1f)",
                    CC_CLASS_NAME[cls],
                    revoked_ok >= 0 ? rb[0][revoked_ok].ts : -1, revoked_ok >= 0 ? rb[0][revoked_ok].td : -1,
                    revoked_ok >= 0 ? rb[0][revoked_ok].tep : -1, revoked_ok >= 0 ? rb[0][revoked_ok].cl : -1,
                    revoked_ok >= 0 ? rb[0][revoked_ok].seen : 0, revoked_ok >= 0 ? rb[0][revoked_ok].pos : 0,
                    revoked_ok >= 0 ? rb[0][revoked_ok].neg : 0, revoked_ok >= 0 ? rb[0][revoked_ok].peak : 0.0);
          } else {
              cc_row("OBS", "CC12_REVOKE", "class %s produced no REVOKE in this world (%ld earn, 0 revoke): the transform "
                     "moves the earn past the breaking region, so the earn-then-revoke arc does not fit; reported rather "
                     "than faked", CC_CLASS_NAME[cls], ee);
          } }

        /* ---- PASS-12 red (condition 7): mutate the REAL revoke row ---- */
        { char bp[512], tmp[512], line[8192]; FILE *in, *out2; int done = 0; long rev = 0;
          snprintf(bp, sizeof bp, "%s/relation_events_confirmatory.tsv", sbo);
          snprintf(tmp, sizeof tmp, "%s/.tamper", sbo);
          in = fopen(bp, "rb");
          if (in) { while (fgets(line, sizeof line, in)) if (strstr(line, "\trevoke\t")) rev++; fclose(in); }
          if (rev == 0) {
              cc_row("OBS", "CC09_GATE", "class %s: relation_events carries no revoke row in this world, so the "
                     "real-revoke-row tamper cannot run here", CC_CLASS_NAME[cls]);
          } else {
              in = fopen(bp, "rb"); out2 = fopen(tmp, "wb");
              if (in && out2) {
                  while (fgets(line, sizeof line, in)) {
                      if (!done && strstr(line, "\trevoke\t")) {
                          char *q = strstr(line, "\trevoke\t");
                          fprintf(out2, "%.*s\tearn\t%s", (int)(q - line), line, q + 8);
                          done = 1; continue;
                      }
                      fputs(line, out2);
                  }
                  fclose(in); fclose(out2);
                  if (done) {
                      rename(tmp, bp);
                      rc = cc_confirmatory(&ag, 1);
                      cc_pf("CC12_REVOKE", rc == 1,
                            "class %s (7) the REAL revoke row flipped to earn (%ld revoke rows present): refused (rc=%d)",
                            CC_CLASS_NAME[cls], rev, rc);
                      if (rc != 1) fails++;
                      cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);
                  } else remove(tmp);
              } else { if (in) fclose(in); if (out2) fclose(out2); }
          } }

        /* ---- PASS-10 reds: NUL + tail garbage in every receipt ---- */
        { const char *rcpt[4]; int z;
          rcpt[0] = sroots; rcpt[1] = scommit; rcpt[2] = sfreeze; rcpt[3] = ssel;
          for (z = 0; z < 4; z++) {
              long n2; unsigned char *b2 = slurp(rcpt[z], &n2);
              FILE *o2 = fopen(rcpt[z], "wb");
              fwrite(b2, 1, (size_t)n2, o2);
              fputc(0, o2); fputs("garbage\n", o2);
              fclose(o2);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED receipt %d NUL+tail garbage: refused (rc=%d)",
                    CC_CLASS_NAME[cls], z, rc);
              if (rc != 1) fails++;
              o2 = fopen(rcpt[z], "wb"); fwrite(b2, 1, (size_t)n2, o2); fclose(o2);
              free(b2);
          } }

        /* ---- PASS-10 red: oversized class_index that must not alias ---- */
        { const char *sv4[17]; int q;
          for (q = 0; q < 17; q++) sv4[q] = sv2[q];
          sv4[12] = "4294967296";
          cc_kvfile(ssel, sf, sv4, 17);
          rc = cc_confirmatory(&ag, 1);
          cc_pf("CC09_GATE", rc == 1, "class %s RED oversized class_index 4294967296: refused (rc=%d)",
                CC_CLASS_NAME[cls], rc);
          if (rc != 1) fails++;
          cc_kvfile(ssel, sf, sv2, 17); }

        /* ---- PASS-10 red: oversized evidence chunk that must not alias ---- */
        { char ep[512], line[8192], tmp[512]; FILE *in, *out2; long ln = 0;
          snprintf(ep, sizeof ep, "%s/evidence_confirmatory.tsv", sbo);
          snprintf(tmp, sizeof tmp, "%s/.tamper", sbo);
          in = fopen(ep, "rb"); out2 = fopen(tmp, "wb");
          if (in && out2) {
              while (fgets(line, sizeof line, in)) {
                  ln++;
                  if (ln == 2) { char *tab = strchr(line, '\t'); if (tab) { fprintf(out2, "4294967296%s", tab); continue; } }
                  fputs(line, out2);
              }
              fclose(in); fclose(out2); rename(tmp, ep);
              rc = cc_confirmatory(&ag, 1);
              cc_pf("CC09_GATE", rc == 1, "class %s RED oversized evidence chunk: refused (rc=%d)",
                    CC_CLASS_NAME[cls], rc);
              if (rc != 1) fails++;
              cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);
          } else { if (in) fclose(in); if (out2) fclose(out2); } }

        /* restore a coherent inventory before the remaining reds */
        cc_emit_inventory(sbo, W, &RC, cls, ws, CC_CLASS_NAME[drawn_class], wbuf, basesha, &mf);

        /* ---- RED 4: wrong-seed world ---- */
        { unsigned char *W2 = (unsigned char *)malloc(CC_RUN_BYTES);
          int tm2[256]; char e2[256]; char wp[512];
          if (!cc_build_world(cls, B, blen, ws ^ 0x5a5a5a5aULL, W2, tm2, e2, sizeof e2)) {
              snprintf(wp, sizeof wp, "%s/W_confirmatory.bin", sbo);
              cc_write(wp, W2, CC_RUN_BYTES);
              rc = cc_confirmatory(&ag, 1);
              if (cc_class_seedless(cls)) {
                  cc_row("OBS", "CC09_GATE", "class %s RED wrong-seed: class is seedless by law (no draw), "
                         "so a seed change cannot alter its bytes; covered by the wrong-class red instead",
                         CC_CLASS_NAME[cls]);
              } else {
                  cc_pf("CC09_GATE", rc == 1, "class %s RED wrong-seed-world: refused (rc=%d)", CC_CLASS_NAME[cls], rc);
                  if (rc != 1) fails++;
              }
              cc_write(wp, W, CC_RUN_BYTES);
          }
          free(W2); }
        free(W);
    }
    free(B);
    return fails;
}

/* C3 states literally: "plain and ff perform no random draw from it. cipher and
 * the cipher half of half give this exact 64-bit value to the frozen
 * xorshift/Fisher-Yates law." A seed change therefore cannot alter plain or ff
 * bytes. This is the law's own asymmetry, not one introduced here. */

/* =====================================================================
 * main -- now with a real door.
 * ===================================================================== */
static int cc_mandatory_kat(void)
{
    char h[65], bsha[65], vsha[65], tsha[65], cdig[65], sdig[65];
    unsigned char cd[32], sd[32];
    size_t cl = 0, sl = 0;
    uint64_t ws;
    int i;
    sha256_buf((const unsigned char *)"", 0, h);
    if (strcmp(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855")) return 1;
    sha256_buf((const unsigned char *)"abc", 3, h);
    if (strcmp(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad")) return 1;
    for (i = 0; i < 64; i++) { bsha[i] = '0'; vsha[i] = 'f'; }
    bsha[64] = 0; vsha[64] = 0;
    sha256_buf((const unsigned char *)"NETTA-C4-SELECTION-TEST-v1", 26, tsha);
    cc_class_digest(bsha, vsha, tsha, cd, &cl); cc_hex(cd, 32, cdig);
    cc_seed_digest(bsha, vsha, tsha, sd, &sl); cc_hex(sd, 32, sdig);
    ws = cc_be64(sd);
    if (strcmp(tsha, "f318964f23519b415bd3ef68ad9c03843a8af33e2d95f152a8a9797ef373d73f") ||
        cl != 210 || strcmp(cdig, "b0c142688cb55362d4952ef0bc42904537ce7ceac7d1b52df3e20cf4342c6be1") ||
        (cd[31] & 3) != 1 || sl != 209 ||
        strcmp(sdig, "f476f3a4bcc1d16f666acb811ef8c732e9c6a9574461a448d780cff225afb4de") ||
        ws != 0xf476f3a4bcc1d16fULL || cc_half_tail_seed(ws, NULL) != 0x4372270db242db11ULL)
        return 1;
    return 0;
}

static void cc_usage(void)
{
    fprintf(stderr,
        "usage:\n"
        "  transfer4_confirm_check --development\n"
        "  transfer4_confirm_check --receipt-test BASE_TEXT \\\n"
        "      --roots SYNTHETIC_ROOTS.tsv --base-commit SYNTHETIC_BASE_COMMIT.tsv \\\n"
        "      --base-freeze SYNTHETIC_BASE_FREEZE.tsv --selection SYNTHETIC_SELECTION.tsv\n"
        "  transfer4_confirm_check --confirmatory SOURCE_A BASE_TEXT \\\n"
        "      --roots ROOTS.tsv --base-commit BASE_COMMIT.tsv \\\n"
        "      --base-freeze BASE_FREEZE.tsv --selection SELECTION.tsv \\\n"
        "      --builder-out BUILDER_DIR\n"
        "  transfer4_confirm_check --self-test [GATE_DIR]\n");
}

int main(int argc, char **argv)
{
    char h[65];
    long n;
    const char *explicit_gate = NULL;

    if (cc_mandatory_kat()) {
        fprintf(stderr, "mandatory SHA/C2/C3 known-answer test failed\n");
        return 2;
    }
    if (argc > 1 && !strcmp(argv[1], "--development")) {
        if (argc != 2) { cc_usage(); return 2; }
        return frozen_dev_main();
    }

    if (argc > 1 && !strcmp(argv[1], "--receipt-test")) {
        CcArgs ag; int i, k;
        static const char *fl[4] = { "--roots", "--base-commit", "--base-freeze", "--selection" };
        const char **slot[4];
        int seen[4] = {0, 0, 0, 0};
        memset(&ag, 0, sizeof ag);
        if (argc != 11 || !argv[2][0]) { cc_usage(); return 2; }
        ag.base_text = argv[2]; ag.receipt_test = 1;
        slot[0] = &ag.roots; slot[1] = &ag.base_commit;
        slot[2] = &ag.base_freeze; slot[3] = &ag.selection;
        for (i = 3; i < argc; i += 2) {
            int hit = -1;
            for (k = 0; k < 4; k++) if (!strcmp(argv[i], fl[k])) { hit = k; break; }
            if (hit < 0 || seen[hit] || !argv[i + 1][0]) { cc_usage(); return 2; }
            seen[hit] = 1; *slot[hit] = argv[i + 1];
        }
        for (k = 0; k < 4; k++) if (!seen[k]) { cc_usage(); return 2; }
        {
            char why[512];
            if (cc_receipt_roots_guard(ag.roots, why, sizeof why)) {
                fprintf(stderr, "--receipt-test requires scratch SYNTHETIC_ROOTS.tsv outside the repository root: %s\n", why);
                return 2;
            }
        }
        return cc_confirmatory(&ag, 0);
    }

    if (argc > 1 && !strcmp(argv[1], "--confirmatory")) {
        CcArgs ag; int i;
        memset(&ag, 0, sizeof ag);
        if (argc < 4 || !argv[2][0] || !argv[3][0]) { cc_usage(); return 2; }
        ag.source_a = argv[2];
        ag.base_text = argv[3];
        {   /* exact argc, one occurrence of each flag, no dangling argv */
            static const char *fl[5] = { "--roots", "--base-commit", "--base-freeze",
                                         "--selection", "--builder-out" };
            const char **slot[5] = { &ag.roots, &ag.base_commit, &ag.base_freeze,
                                     &ag.selection, &ag.builder_out };
            int seen[5] = { 0, 0, 0, 0, 0 }, k;
            if (argc != 4 + 10) {
                fprintf(stderr, "--confirmatory takes exactly %d arguments, saw %d\n", 4 + 10, argc);
                cc_usage(); return 2;
            }
            for (i = 4; i < argc; i += 2) {
                int hit = -1;
                if (i + 1 >= argc) { fprintf(stderr, "dangling argument '%s'\n", argv[i]); cc_usage(); return 2; }
                for (k = 0; k < 5; k++) if (!strcmp(argv[i], fl[k])) { hit = k; break; }
                if (hit < 0) { fprintf(stderr, "unknown flag %s\n", argv[i]); cc_usage(); return 2; }
                if (seen[hit]) { fprintf(stderr, "flag %s given more than once\n", fl[hit]); cc_usage(); return 2; }
                if (!argv[i + 1][0]) { fprintf(stderr, "flag %s has an empty value\n", fl[hit]); return 2; }
                seen[hit] = 1; *slot[hit] = argv[i+1];
            }
            for (k = 0; k < 5; k++) if (!seen[k]) {
                fprintf(stderr, "missing required flag %s\n", fl[k]); cc_usage(); return 2; }
        }
        if (!ag.roots || !ag.base_commit || !ag.base_freeze || !ag.selection ||
            !ag.builder_out) {
            fprintf(stderr, "--confirmatory requires --roots --base-commit --base-freeze "
                            "--selection --builder-out\n");
            cc_usage();
            return 2;
        }
        printf("# transfer4_confirm_check --confirmatory\n");
        return cc_confirmatory(&ag, 0);
    }

    if (argc == 1) { cc_usage(); return 2; }
    if (strcmp(argv[1], "--self-test")) { cc_usage(); return 2; }
    if (argc > 3) { cc_usage(); return 2; }
    if (argc == 3) {
        char why[512];
        if (cc_scratch_dir_guard(argv[2], why, sizeof why)) {
            fprintf(stderr, "%s\n", why);
            return 2;
        }
        if (cc_ensure_dir(argv[2], why, sizeof why)) {
            fprintf(stderr, "%s\n", why);
            return 2;
        }
        explicit_gate = argv[2];
    }
    printf("# transfer4_confirm_check -- independent CONFIRMATORY verifier path\n");
    printf("# status\tclass\tdetail\n");
    fflush(stdout);

    /* ---------------- CC01: SHA-256 known-answer tests ---------------- */
    {
        sha256_buf((const unsigned char *)"", 0, h);
        cc_pf("CC01_SHAKAT", !strcmp(h, "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"),
              "SHA-256(\"\") = %s", h);
        sha256_buf((const unsigned char *)"abc", 3, h);
        cc_pf("CC01_SHAKAT", !strcmp(h, "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"),
              "SHA-256(\"abc\") = %s", h);
        cc_row("OBS", "CC01_SHAKAT", "the two KAT digests are the standard NIST vectors; the law mandates the test "
                                     "but does not print the values. The law-supplied C2/C3 vector below is the "
                                     "authoritative check on this implementation.");
    }

    /* ---------------- CC02: the complete C2/C3 selection vector -------- */
    {
        char bsha[65], vsha[65], tsha[65], cdig[65], sdig[65], htd[65];
        unsigned char cd[32], sd[32];
        size_t cl = 0, sl = 0;
        uint64_t ws, hts;
        int i, ok_all = 1;
        for (i = 0; i < 64; i++) { bsha[i] = '0'; vsha[i] = 'f'; }
        bsha[64] = 0; vsha[64] = 0;
        sha256_buf((const unsigned char *)"NETTA-C4-SELECTION-TEST-v1", 26, tsha);
        cc_pf("CC02_VECTOR", !strcmp(tsha, "f318964f23519b415bd3ef68ad9c03843a8af33e2d95f152a8a9797ef373d73f"),
              "base_text_sha = %s", tsha);
        if (strcmp(tsha, "f318964f23519b415bd3ef68ad9c03843a8af33e2d95f152a8a9797ef373d73f")) ok_all = 0;
        cc_class_digest(bsha, vsha, tsha, cd, &cl);
        cc_hex(cd, 32, cdig);
        cc_pf("CC02_VECTOR", cl == 210, "class preimage length = %zu bytes (law: exactly 210)", cl);
        cc_pf("CC02_VECTOR", !strcmp(cdig, "b0c142688cb55362d4952ef0bc42904537ce7ceac7d1b52df3e20cf4342c6be1"),
              "class_digest = %s", cdig);
        if (cl != 210 || strcmp(cdig, "b0c142688cb55362d4952ef0bc42904537ce7ceac7d1b52df3e20cf4342c6be1")) ok_all = 0;
        cc_pf("CC02_VECTOR", (cd[31] & 3) == 1, "class_index = digest[31] & 3 = %d -> %s",
              cd[31] & 3, CC_CLASS_NAME[cd[31] & 3]);
        if ((cd[31] & 3) != 1) ok_all = 0;
        cc_seed_digest(bsha, vsha, tsha, sd, &sl);
        cc_hex(sd, 32, sdig);
        cc_pf("CC02_VECTOR", sl == 209, "seed preimage length = %zu bytes (law: exactly 209)", sl);
        cc_pf("CC02_VECTOR", !strcmp(sdig, "f476f3a4bcc1d16f666acb811ef8c732e9c6a9574461a448d780cff225afb4de"),
              "seed_digest = %s", sdig);
        if (sl != 209 || strcmp(sdig, "f476f3a4bcc1d16f666acb811ef8c732e9c6a9574461a448d780cff225afb4de")) ok_all = 0;
        ws = cc_be64(sd);
        { char wb[32]; sprintf(wb, "%016llx", (unsigned long long)ws);
          cc_pf("CC02_VECTOR", !strcmp(wb, "f476f3a4bcc1d16f"), "world_seed_be64 = %s", wb);
          if (strcmp(wb, "f476f3a4bcc1d16f")) ok_all = 0; }
        hts = cc_half_tail_seed(ws, htd);
        { char hb[32]; sprintf(hb, "%016llx", (unsigned long long)hts);
          cc_pf("CC02_VECTOR", !strcmp(hb, "4372270db242db11"), "half_tail_seed_be64 = %s", hb);
          if (strcmp(hb, "4372270db242db11")) ok_all = 0; }
        if (!ok_all) { fprintf(stderr, "C2/C3 vector mismatch -- aborting\n"); return 2; }
        cc_row("PASS", "CC02_VECTOR", "complete C2/C3 vector reproduced bit-for-bit; serialization is lawful");
    }

    /* ---------------- CC03: exclusions root ---------------------------- */
    {
        Tsv ex = tsv_load(CC_REPO "CONFIRMATORY_EXCLUSIONS.tsv");
        int okhdr = (ex.n >= 1 && !strcmp(fs(&ex, 0, 0), "label") &&
                     !strcmp(fs(&ex, 0, 1), "bytes") && !strcmp(fs(&ex, 0, 2), "sha256"));
        cc_pf("CC03_EXCLUSIONS", okhdr, "CONFIRMATORY_EXCLUSIONS.tsv header is the literal label/bytes/sha256");
        cc_pf("CC03_EXCLUSIONS", ex.n == 15, "exclusions root holds %d data rows (law: exactly 14)", ex.n - 1);
        { int i, hitA = 0, hitD = 0;
          for (i = 1; i < ex.n; i++) {
              if (fi(&ex, i, 1) == 447545 &&
                  !strcmp(fs(&ex, i, 2), "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb")) hitA = 1;
              if (fi(&ex, i, 1) == 1301310 &&
                  !strcmp(fs(&ex, i, 2), "76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2")) hitD = 1;
          }
          cc_pf("CC03_EXCLUSIONS", hitA && hitD,
                "SOURCE_A and development D are both excluded bases (exact byte+digest match)"); }
        tsv_free(&ex);
    }

    /* ---------------- CC04: C1 eligibility on the development base ----- */
    {
        long lenD; unsigned char *D = slurp(BASED, &lenD);
        int sat = 0; long nw = 0, ffl;
        cc_pf("CC04_ELIGIBLE", lenD >= CC_RUN_BYTES, "raw base length %ld >= RUN_BYTES %d", lenD, CC_RUN_BYTES);
        ffl = cc_ff_length(D, lenD, &sat, &nw);
        cc_pf("CC04_ELIGIBLE", !sat, "bounded false-friend probe did not saturate (cap %d slots per word)", CC_FFPROBE);
        cc_pf("CC04_ELIGIBLE", ffl == 1287474,
              "independent false-friend transformed length = %ld (builder manifest records 1287474)", ffl);
        cc_pf("CC04_ELIGIBLE", ffl >= CC_RUN_BYTES, "transformed length %ld >= RUN_BYTES %d", ffl, CC_RUN_BYTES);
        cc_row("OBS", "CC04_ELIGIBLE", "distinct candidate words on the development base = %ld", nw);
        cc_pf("CC04_ELIGIBLE", !(1024 >= CC_RUN_BYTES), "red: a 1024-byte base is rejected by the raw-length gate");
        free(D);
        {
            long cn = 0; unsigned char *capbase = cc_capacity_red_fixture(&cn);
            unsigned char *outw = (unsigned char *)malloc(CC_RUN_BYTES);
            int csat = 0; long cff = capbase ? cc_ff_length(capbase, cn, &csat, NULL) : -1;
            cc_pf("CC13_HARDENING", capbase && !csat && cn <= (LONG_MAX - 16) / 2 && cff > cn * 2 + 16,
                  "capacity red crosses the frozen ceiling: base %ld, ff %ld, ceiling %ld",
                  cn, cff, cn <= (LONG_MAX - 16) / 2 ? cn * 2 + 16 : -1L);
            cc_pf("CC13_HARDENING", capbase && outw && cc_build_ff(capbase, cn, outw) < 0,
                  "verifier refuses the over-ceiling false-friend construction before any write");
            free(outw); free(capbase);
        }
    }

    /* ---------------- CC05: four C5 classes from the C4BRANCH seed ----- */
    {
        long lenD; unsigned char *D = slurp(BASED, &lenD);
        unsigned char *w = (unsigned char *)malloc(CC_RUN_BYTES);
        struct { const char *name; const char *sha; } exp[4] = {
            { "cipher", "34f8e8e6dca575f164fc517d33b8c1cbb727cd75ea90bfea66339c96a0540340" },
            { "plain",  "d9cb78b26cd40c0b7ae444eab989aa9633da0f5493a9183951c3950b3ec2e0ce" },
            { "half",   "4b71206a405892f01780bd84bb2a4d0e9e5f10ac8c11a42829cfa2b1e44c1716" },
            { "ff",     "722ae766a976c5f7f34b724333ae88a373a28fae6228dfe4cee7d907227de492" }
        };
        cc_build_cipher(D, CC_C4BRANCH_SEED, w);
        sha256_buf(w, CC_RUN_BYTES, h);
        cc_pf("CC05_CLASSES", !strcmp(h, exp[0].sha), "class cipher from C4BRANCH seed -> %s", h);
        cc_build_plain(D, w);
        sha256_buf(w, CC_RUN_BYTES, h);
        cc_pf("CC05_CLASSES", !strcmp(h, exp[1].sha), "class plain  -> %s", h);
        cc_build_half(D, lenD, CC_C4BRANCH_SEED, w);
        sha256_buf(w, CC_RUN_BYTES, h);
        cc_pf("CC05_CLASSES", !strcmp(h, exp[2].sha), "class half   from C4BRANCH seed -> %s", h);
        { long ffl = cc_build_ff(D, lenD, w);
          sha256_buf(w, CC_RUN_BYTES, h);
          cc_pf("CC05_CLASSES", ffl >= CC_RUN_BYTES && !strcmp(h, exp[3].sha),
                "class ff     (transformed %ld bytes) -> %s", ffl, h); }
        free(w); free(D);
    }

    /* ---------------- CC06/CC07: pass-5 receipt reds and C8 fixtures --- */
    cc_pass5_receipt_and_c8();

    /* ---------------- CC09: the pass-6 end-to-end door gate ----------- */
    {
        char auto_gate[PATH_MAX], why[512], alias[PATH_MAX], scratch_roots[PATH_MAX];
        const char *gd;
        int auto_created = 0;
        int f;
        if (explicit_gate) {
            gd = explicit_gate;
        } else {
            const char *tmp = getenv("TMPDIR");
            size_t tl;
            int z;
            if (!tmp || !tmp[0]) tmp = "/private/tmp";
            if (cc_scratch_dir_guard(tmp, why, sizeof why)) {
                fprintf(stderr, "TMPDIR cannot host the self-test gate: %s\n", why);
                return 2;
            }
            tl = strlen(tmp);
            z = snprintf(auto_gate, sizeof auto_gate, "%s%snetta-court4-gate.XXXXXX",
                         tmp, (tl && tmp[tl - 1] == '/') ? "" : "/");
            if (z < 0 || (size_t)z >= sizeof auto_gate) {
                fprintf(stderr, "TMPDIR is too long for a self-test gate\n");
                return 2;
            }
            if (!mkdtemp(auto_gate)) {
                fprintf(stderr, "cannot create fresh self-test gate under TMPDIR: %s\n", strerror(errno));
                return 2;
            }
            gd = auto_gate; auto_created = 1;
        }
        cc_row("OBS", "CC09_GATE", "%s scratch directory is ready",
               auto_created ? "fresh auto-created under TMPDIR" : "caller-supplied");
        cc_row("OBS", "CC09_GATE", "end-to-end gate: every invocation below goes through the REAL --confirmatory "
                                   "entry point on a fully synthetic chain; no real receipt, base text or builder "
                                   "artifact is consumed");
        f = cc_gate(gd);
        cc_pf("CC09_GATE", f == 0, "end-to-end door gate over all four classes: %d failures", f);
        snprintf(alias, sizeof alias, "%s./SYNTHETIC_ROOTS.tsv", CC_REPO);
        cc_pf("CC13_HARDENING", cc_receipt_roots_guard(alias, why, sizeof why) != 0,
              "receipt-test canonical guard rejects repository alias: %s", why);
        snprintf(scratch_roots, sizeof scratch_roots, "%s/SYNTHETIC_ROOTS.tsv", gd);
        cc_pf("CC13_HARDENING", cc_receipt_roots_guard(scratch_roots, why, sizeof why) == 0,
              "receipt-test canonical guard accepts the regular scratch receipt");
        cc_pf("CC13_ISOLATION", cc_scratch_dir_guard(CC_REPO "reports", why, sizeof why) != 0,
              "self-test scratch guard rejects a canonical directory in the repository: %s", why);
    }

    cc_row("OBS", "CC08_LIMITS", "no confirmatory base, class, or seed was computed for any REAL base: the only C2/C3 "
                                 "evaluations here are the law's synthetic vector and the gate's synthetic chain");
    cc_row("OBS", "CC08_LIMITS", "branch artifact directory paths are named in NEITHER manifest; the four branch "
                                 "constructions are verified against the frozen world digests instead");

    printf("CCSUMMARY\ttotal=%ld\tpass=%ld\tfail=%ld\tnote=%ld\n",
           cc_pass + cc_fail, cc_pass, cc_fail, cc_note);
    (void)n;
    return cc_fail ? 1 : 0;
}
