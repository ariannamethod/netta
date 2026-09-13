/* transfer4_check.c -- independent verifier for NETTA transfer court 4 (development run).
 *
 * Written from exactly two frozen files:
 *   TRANSFER4_DRAFT.md  sha256 b17abc1e546134374977dc1ecf501cb0af8b6a4db62570c70cc3e068e499f0fe
 *   COURT3_SNAPSHOT.md  sha256 20df368e719f275b4eb655c8c34b4f197904e5873aa77e5e1bf767604e883423
 * plus the raw world inputs and the builder's OUTPUT artifacts. No builder source was read.
 *
 * Status vocabulary on stdout:
 *   PASS  law-determined check reproduced and agreed
 *   FAIL  law-determined check reproduced and disagreed
 *   GAP   the two frozen files do not determine the object; not reproducible by this hand
 *   OBS   observation about artifacts, not a law check
 *
 * cc -O2 -Wall -Wextra -Wpedantic -o transfer4_check transfer4_check.c -lm
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

/* ------------------------------------------------------------------ paths */
#define REPO "/Users/ataeff/arianna/netta/"
#define ART  REPO "transfer4dev_closure2/"
#define SRCA REPO "netta.txt"
#define BASED "/Users/ataeff/arianna-datasets/miller/combined.clean.txt"

/* -------------------------------------------------- inherited constants */
/* COURT3_SNAPSHOT.md: run length 131072, chunk 1024, horizons {1024,4096,16384,65536},
 * M = MARGIN(16384) = 163.84 bits, ghost line 520 bits.
 * TRANSFER4_DRAFT.md: MARGIN(N)=0.01*N, EARN 32, REVOKE 16, enter L=0.05,
 * clamp [0.01,0.5], eta 0.05.                                                */
#define RUN_BYTES 131072
#define CHUNK 1024
#define DECIDE_N 16384
#define NHOR (DECIDE_N / CHUNK)
#define MARGIN_M (0.01 * (double)DECIDE_N)
#define GHOST_LINE 520.0
#define EARN_BITS 32.0
#define REVOKE_BITS 16.0

/* ------------------------------------------------------------- reporting */
static long n_pass = 0, n_fail = 0, n_gap = 0, n_obs = 0;

static void P(const char *cls, const char *fmt, ...)
{ va_list ap; printf("PASS\t%s\t", cls); va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); n_pass++; }
static void Fl(const char *cls, const char *fmt, ...)
{ va_list ap; printf("FAIL\t%s\t", cls); va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); n_fail++; }
static void Gp(const char *cls, const char *fmt, ...)
{ va_list ap; printf("GAP\t%s\t", cls); va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); n_gap++; }
static void Ob(const char *cls, const char *fmt, ...)
{ va_list ap; printf("OBS\t%s\t", cls); va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n"); n_obs++; }
static void PF(const char *cls, int ok, const char *fmt, ...)
{
    va_list ap; printf("%s\t%s\t", ok ? "PASS" : "FAIL", cls);
    va_start(ap, fmt); vprintf(fmt, ap); va_end(ap); printf("\n");
    if (ok) n_pass++; else n_fail++;
}

/* ---- A5 comparator law (COURT4_GAP_ADDENDUM.md) ------------------------
 * Both values must be finite; a NaN or infinity is an immediate mismatch.
 * Structural and integer fields compare exactly and never route through here. */
static long a5_nonfinite = 0;
static int a5_finite2(double x, double y)
{
    if (isfinite(x) && isfinite(y)) return 1;
    a5_nonfinite++; return 0;
}
/* fixture probability/price: abs(x-y) <= 1e-12 * max(abs x, abs y); two exact zeros agree */
static int a5_fix_prob(double x, double y)
{
    if (!a5_finite2(x, y)) return 0;
    if (x == y) return 1;
    return fabs(x - y) <= 1e-12 * fmax(fabs(x), fabs(y));
}
/* fixture live weight: absolute 1e-12 ; fixture ledger: absolute 1e-9 */
static int a5_fix_abs(double x, double y, double tol)
{
    if (!a5_finite2(x, y)) return 0;
    return fabs(x - y) <= tol;
}
/* main-court accumulated / ledger-like / nonzero live weight:
 * abs(x-y) <= 1e-6 * max(1, abs x, abs y) */
static int a5_main(double x, double y)
{
    if (!a5_finite2(x, y)) return 0;
    return fabs(x - y) <= 1e-6 * fmax(1.0, fmax(fabs(x), fabs(y)));
}
/* %.17g must round-trip a serialized value to its binary64 original */
static int a5_roundtrip(const char *s)
{
    char buf[64]; double v = strtod(s, NULL), w;
    if (!isfinite(v)) return 0;
    snprintf(buf, sizeof buf, "%.17g", v);
    w = strtod(buf, NULL);
    return memcmp(&v, &w, sizeof v) == 0;
}

/* ------------------------------------------------------------------- io */
static unsigned char *slurp(const char *path, long *len)
{
    FILE *f = fopen(path, "rb");
    long n;
    unsigned char *b;
    if (!f) { fprintf(stderr, "cannot open %s\n", path); exit(2); }
    fseek(f, 0, SEEK_END); n = ftell(f); fseek(f, 0, SEEK_SET);
    b = (unsigned char *)malloc((size_t)n + 1);
    if (!b) { fprintf(stderr, "oom\n"); exit(2); }
    if (n > 0 && fread(b, 1, (size_t)n, f) != (size_t)n) { fprintf(stderr, "short read %s\n", path); exit(2); }
    b[n] = 0; fclose(f); *len = n; return b;
}

/* ---------------------------------------------------------------- sha256 */
typedef struct { uint32_t h[8]; } SHA;
static const uint32_t K256[64] = {
0x428a2f98u,0x71374491u,0xb5c0fbcfu,0xe9b5dba5u,0x3956c25bu,0x59f111f1u,0x923f82a4u,0xab1c5ed5u,
0xd807aa98u,0x12835b01u,0x243185beu,0x550c7dc3u,0x72be5d74u,0x80deb1feu,0x9bdc06a7u,0xc19bf174u,
0xe49b69c1u,0xefbe4786u,0x0fc19dc6u,0x240ca1ccu,0x2de92c6fu,0x4a7484aau,0x5cb0a9dcu,0x76f988dau,
0x983e5152u,0xa831c66du,0xb00327c8u,0xbf597fc7u,0xc6e00bf3u,0xd5a79147u,0x06ca6351u,0x14292967u,
0x27b70a85u,0x2e1b2138u,0x4d2c6dfcu,0x53380d13u,0x650a7354u,0x766a0abbu,0x81c2c92eu,0x92722c85u,
0xa2bfe8a1u,0xa81a664bu,0xc24b8b70u,0xc76c51a3u,0xd192e819u,0xd6990624u,0xf40e3585u,0x106aa070u,
0x19a4c116u,0x1e376c08u,0x2748774cu,0x34b0bcb5u,0x391c0cb3u,0x4ed8aa4au,0x5b9cca4fu,0x682e6ff3u,
0x748f82eeu,0x78a5636fu,0x84c87814u,0x8cc70208u,0x90befffau,0xa4506cebu,0xbef9a3f7u,0xc67178f2u};
#define ROR(x,n) (((x)>>(n))|((x)<<(32-(n))))
static void sha_block(SHA *s, const unsigned char *p)
{
    uint32_t w[64], a,b,c,d,e,f,g,hh,t1,t2,S0,S1,ch,mj; int i;
    for (i = 0; i < 16; i++)
        w[i] = ((uint32_t)p[4*i]<<24)|((uint32_t)p[4*i+1]<<16)|((uint32_t)p[4*i+2]<<8)|p[4*i+3];
    for (i = 16; i < 64; i++) {
        uint32_t x0 = ROR(w[i-15],7)^ROR(w[i-15],18)^(w[i-15]>>3);
        uint32_t x1 = ROR(w[i-2],17)^ROR(w[i-2],19)^(w[i-2]>>10);
        w[i] = w[i-16]+x0+w[i-7]+x1;
    }
    a=s->h[0];b=s->h[1];c=s->h[2];d=s->h[3];e=s->h[4];f=s->h[5];g=s->h[6];hh=s->h[7];
    for (i = 0; i < 64; i++) {
        S1 = ROR(e,6)^ROR(e,11)^ROR(e,25);
        ch = (e&f)^((~e)&g);
        t1 = hh+S1+ch+K256[i]+w[i];
        S0 = ROR(a,2)^ROR(a,13)^ROR(a,22);
        mj = (a&b)^(a&c)^(b&c);
        t2 = S0+mj;
        hh=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
    }
    s->h[0]+=a;s->h[1]+=b;s->h[2]+=c;s->h[3]+=d;s->h[4]+=e;s->h[5]+=f;s->h[6]+=g;s->h[7]+=hh;
}
static void sha256_buf(const unsigned char *data, size_t n, char *out65)
{
    SHA s; size_t i, full, rem, total;
    unsigned char last[128];
    uint64_t bits = (uint64_t)n * 8u;
    s.h[0]=0x6a09e667u;s.h[1]=0xbb67ae85u;s.h[2]=0x3c6ef372u;s.h[3]=0xa54ff53au;
    s.h[4]=0x510e527fu;s.h[5]=0x9b05688cu;s.h[6]=0x1f83d9abu;s.h[7]=0x5be0cd19u;
    full = n / 64; rem = n % 64;
    for (i = 0; i < full; i++) sha_block(&s, data + 64*i);
    memset(last, 0, sizeof last);
    memcpy(last, data + 64*full, rem);
    last[rem] = 0x80;
    total = (rem < 56) ? 64 : 128;
    for (i = 0; i < 8; i++) last[total - 1 - i] = (unsigned char)(bits >> (8*i));
    sha_block(&s, last);
    if (total == 128) sha_block(&s, last + 64);
    for (i = 0; i < 8; i++) sprintf(out65 + 8*i, "%08x", s.h[i]);
    out65[64] = 0;
}
static void sha256_file(const char *path, char *out65, long *len)
{
    long n; unsigned char *b = slurp(path, &n);
    sha256_buf(b, (size_t)n, out65);
    *len = n; free(b);
}

/* ---------------------------------------------------------- tsv reading */
typedef struct { char **f; int nf; } Row;
typedef struct { Row *r; int n; char *raw; } Tsv;

static Tsv tsv_load(const char *path)
{
    Tsv t; long n; char *b = (char *)slurp(path, &n);
    char *p, *end;
    int cap = 1024;
    t.raw = b; t.r = (Row *)malloc(sizeof(Row) * (size_t)cap); t.n = 0;
    p = b; end = b + n;
    while (p < end) {
        char *nl = (char *)memchr(p, '\n', (size_t)(end - p));
        char *line_end = nl ? nl : end;
        *line_end = 0;
        if (line_end > p) {
            int nf = 1, i = 0; char *q; char **fl;
            if (t.n == cap) { cap *= 2; t.r = (Row *)realloc(t.r, sizeof(Row) * (size_t)cap); }
            for (q = p; *q; q++) if (*q == '\t') nf++;
            fl = (char **)malloc(sizeof(char *) * (size_t)nf);
            fl[i++] = p;
            for (q = p; *q; q++) if (*q == '\t') { *q = 0; fl[i++] = q + 1; }
            t.r[t.n].f = fl; t.r[t.n].nf = nf; t.n++;
        }
        p = line_end + 1;
    }
    return t;
}
static void tsv_free(Tsv *t) { int i; for (i = 0; i < t->n; i++) free(t->r[i].f); free(t->r); free(t->raw); }
static long fi(Tsv *t, int r, int c) { return strtol(t->r[r].f[c], NULL, 10); }
static double fd(Tsv *t, int r, int c) { return strtod(t->r[r].f[c], NULL); }
static const char *fs(Tsv *t, int r, int c) { return t->r[r].f[c]; }

/* --------------------------------------------------------- arm indexing */
/* map ids: 0 bonly, 1 relation, 2..20 null0..null18, 21 oracle */
static int mapid_of(const char *s)
{
    if (!strcmp(s, "bonly")) return 0;
    if (!strcmp(s, "relation")) return 1;
    if (!strcmp(s, "oracle")) return 21;
    if (!strncmp(s, "null", 4)) { int k = atoi(s + 4); if (k >= 0 && k <= 18) return 2 + k; }
    return -1;
}
static const char *mapname_of(int id)
{
    static char buf[16];
    if (id == 0) return "bonly";
    if (id == 1) return "relation";
    if (id == 21) return "oracle";
    sprintf(buf, "null%d", id - 2); return buf;
}
/* pricing arms: 0 cold, 1 relation, 2..20 null0..18, 21 oracle */
static int armid_of(const char *s)
{
    int m;
    if (!strcmp(s, "cold")) return 0;
    m = mapid_of(s);
    return (m <= 0) ? -1 : m;
}

/* ------------------------------------------------------------- xorshift */
static uint64_t xs64(uint64_t *x)
{
    uint64_t v = *x;
    v ^= v << 13; v ^= v >> 7; v ^= v << 17;
    *x = v; return v;
}

static const uint64_t NULL_SEED[19] = {
0x243f6a8885a308d3ULL,0x13198a2e03707344ULL,0xa4093822299f31d0ULL,
0x082efa98ec4e6c89ULL,0x452821e638d01377ULL,0xbe5466cf34e90c6cULL,
0xc0ac29b7c97c50ddULL,0x3f84d5b5b5470917ULL,0x9216d5d98979fb1bULL,
0xd1310ba698dfb5acULL,0x2ffd72dbd01adfb7ULL,0xb8e1afed6a267e96ULL,
0xba7c9045f12c7f99ULL,0x24a19947b3916cf7ULL,0x0801f2e2858efc16ULL,
0x636920d871574e69ULL,0xa458fea3f4933d7eULL,0x0d95748f728eb658ULL,
0x718bcd5882154aeeULL};

/* frozen null-scramble construction fixture, transcribed from TRANSFER4_DRAFT.md */
static const int FIX_ARRIVE[8] = {11,29,47,83,131,173,211,251};
static const int FIX_TABLE[19][8] = {
{211,131,11,173,251,83,29,47},
{83,11,29,211,47,131,173,251},
{211,29,173,251,11,47,131,83},
{29,47,173,251,83,211,11,131},
{131,173,11,83,47,29,211,251},
{131,47,173,83,211,11,251,29},
{173,11,131,251,83,29,47,211},
{211,173,251,83,11,29,47,131},
{11,131,47,173,251,211,29,83},
{11,47,131,251,211,83,29,173},
{211,131,173,83,251,29,47,11},
{11,251,47,131,211,83,173,29},
{29,131,83,47,211,11,251,173},
{29,11,211,251,173,83,47,131},
{131,83,11,173,29,251,211,47},
{11,29,83,47,251,173,131,211},
{211,47,173,131,251,11,83,29},
{131,11,173,211,47,251,29,83},
{47,173,83,211,251,29,11,131}};

/* online Fisher-Yates over arrival indices; private state reset to the seed per boundary */
static void null_perm(uint64_t seed, int n, int *p)
{
    uint64_t x = seed; int i;
    for (i = 0; i < n; i++) p[i] = i;
    for (i = 1; i < n; i++) {
        uint64_t b = (uint64_t)(i + 1);
        uint64_t threshold = (0ULL - b) % b;
        uint64_t draw; int j, tmp;
        do { draw = xs64(&x); } while (draw < threshold);
        j = (int)(draw % b);
        tmp = p[i]; p[i] = p[j]; p[j] = tmp;
    }
}

/* machine-law renaming permutation (literal modulo, no rejection sampler) */
static void rename_perm(uint64_t seed, int *p)
{
    uint64_t x = seed; int i;
    for (i = 0; i < 256; i++) p[i] = i;
    for (i = 255; i >= 1; i--) {
        uint64_t dd = xs64(&x);
        int j = (int)(dd % (uint64_t)(i + 1));
        int t = p[i]; p[i] = p[j]; p[j] = t;
    }
}

/* ================= court-2 world generators (COURT2_SNAPSHOT.md) ========
 * W-iso   : byte-substitution cipher of D, Fisher-Yates over 0..255, seed 0xB170C5
 * W-ghost : i.i.d. unigram sample from D's byte distribution, seed 0x5EED02
 * W-half  : [0,65536) cipher of D's first 65536 under the W-iso permutation,
 *           [65536,131072) i.i.d. unigram continuation, seed 0x5EED03   (A9)
 * aux1    : first 8192 bytes of W-iso + W-iso's own continuation, 16384 total (A2/A9)
 * aux2    : same 8192-byte prefix + ghost-law bytes, seed 0x5EED04
 * The only Fisher-Yates / xorshift64 spelling anywhere in the frozen corpus is the
 * one TRANSFER4_DRAFT.md writes out for its label renamings; it is used verbatim
 * here with court-2's seeds. Court 2 pins the seeds, not the loop.              */
#define SEED_CIPHER   0xB170C5ULL
#define SEED_GHOST    0x5EED02ULL
#define SEED_HALFTAIL 0x5EED03ULL
#define SEED_AUX2     0x5EED04ULL

static void ghost_stream(uint64_t seed, const long *cnt, long total, unsigned char *out, long n)
{
    uint64_t x = seed; long i;
    for (i = 0; i < n; i++) {
        uint64_t v;
        long r, acc = 0; int b;
        v = x; v ^= v << 13; v ^= v >> 7; v ^= v << 17; x = v;
        r = (long)(v % (uint64_t)total);
        for (b = 0; b < 256; b++) { acc += cnt[b]; if (r < acc) break; }
        out[i] = (unsigned char)(b < 256 ? b : 255);
    }
}

/* =============== court-2 local model: R5 BPE + Body-0 unit chain ========
 * R5 total order: highest pair count, then earliest first-appearance position,
 * then smaller (left_id,right_id).  merges 2048, MIN_PAIR 4.
 * R2 floor: U(u) = 256^-len(u) / Z over the inventory; with only the 256 base
 * units Z = 1 and an empty model prices a byte at exactly 8 bits.
 * epsilon 0.1 per level; the court-4 seam clause ("the last one or two local
 * unit-context ids needed to price the first two new units") fixes the chain at
 * unit orders 0..2.                                                            */
#define MAX_MERGES 2048
#define MIN_PAIR 4

typedef struct { int a, b; } Merge;
static Merge mrg[MAX_MERGES];
static int n_mrg;
static int unit_len[256 + MAX_MERGES];
static int unit_fb[256 + MAX_MERGES];   /* first raw byte of the unit's expansion */
/* admitted maps saved by the per-world engine: [world][boundary][arm][source]
 * arm 0 = relation, arms 1..19 = null0..null18, arm 20 = oracle              */
#define SVW 8
#define SVB 129
#define SVA 21
static int *sv_adm, *sv_ep;
#define SVIX(w,b,a) (((((long)(w) * SVB + (b)) * SVA + (a))) * 256)

/* linked-list BPE learner over the lived raw prefix */
static int  *bp_tok; static long *bp_prev, *bp_next; static long bp_cap = 0;
static long *bp_slot; static long bp_slot_cap = 0;

typedef struct { int a, b; long cnt, first; } PairRec;

static void bpe_learn(const unsigned char *W, long len)
{
    long i, alive;
    static PairRec *pr = NULL; static long pr_cap = 0;
    static long *hh = NULL; static long hcap = 0;
    long npr;
    n_mrg = 0;
    for (i = 0; i < 256; i++) { unit_len[i] = 1; unit_fb[i] = (int)i; }
    if (len < 2) return;
    if (bp_cap < len) {
        bp_cap = len; bp_tok = (int *)realloc(bp_tok, sizeof(int) * (size_t)bp_cap);
        bp_prev = (long *)realloc(bp_prev, sizeof(long) * (size_t)bp_cap);
        bp_next = (long *)realloc(bp_next, sizeof(long) * (size_t)bp_cap);
    }
    for (i = 0; i < len; i++) { bp_tok[i] = W[i]; bp_prev[i] = i - 1; bp_next[i] = (i + 1 < len) ? i + 1 : -1; }
    alive = len;
    if (hcap == 0) {
        hcap = 1 << 20; hh = (long *)malloc(sizeof(long) * (size_t)hcap);
        for (i = 0; i < hcap; i++) hh[i] = -1;
    }
    if (pr_cap == 0) { pr_cap = 1 << 19; pr = (PairRec *)malloc(sizeof(PairRec) * (size_t)pr_cap); }
    if (!bp_slot_cap) { bp_slot_cap = pr_cap; bp_slot = (long *)malloc(sizeof(long) * (size_t)bp_slot_cap); }

    while (n_mrg < MAX_MERGES && alive >= 2) {
        long best = -1, p;
        int ba = 0, bb = 0;
        long bcnt = 0, bfirst = 0;
        npr = 0;
        for (p = 0; p != -1 && npr < pr_cap; p = bp_next[p]) {
            long q = bp_next[p];
            uint64_t key, slot;
            if (q == -1) break;
            key = ((uint64_t)(unsigned)bp_tok[p] << 32) | (uint64_t)(unsigned)bp_tok[q];
            slot = (key * 1146111111111111111ULL) >> 44;
            slot &= (uint64_t)(hcap - 1);
            while (hh[slot] != -1) {
                long ix = hh[slot];
                if (pr[ix].a == bp_tok[p] && pr[ix].b == bp_tok[q]) break;
                slot = (slot + 1) & (uint64_t)(hcap - 1);
            }
            if (hh[slot] == -1) {
                hh[slot] = npr; bp_slot[npr] = (long)slot;
                pr[npr].a = bp_tok[p]; pr[npr].b = bp_tok[q]; pr[npr].cnt = 1; pr[npr].first = p; npr++;
            } else pr[hh[slot]].cnt++;
        }
        for (i = 0; i < npr; i++) {
            if (best < 0 || pr[i].cnt > bcnt ||
                (pr[i].cnt == bcnt && pr[i].first < bfirst) ||
                (pr[i].cnt == bcnt && pr[i].first == bfirst &&
                 (pr[i].a < ba || (pr[i].a == ba && pr[i].b < bb)))) {
                best = i; bcnt = pr[i].cnt; bfirst = pr[i].first; ba = pr[i].a; bb = pr[i].b;
            }
        }
        for (i = 0; i < npr; i++) hh[bp_slot[i]] = -1;   /* clear only touched slots */
        if (best < 0 || bcnt < MIN_PAIR) break;
        {
            int nid = 256 + n_mrg;
            mrg[n_mrg].a = ba; mrg[n_mrg].b = bb;
            unit_len[nid] = unit_len[ba] + unit_len[bb];
            unit_fb[nid] = unit_fb[ba];        /* expansion begins with the left component's first byte */
            n_mrg++;
            for (p = 0; p != -1; ) {
                long q = bp_next[p];
                if (q != -1 && bp_tok[p] == ba && bp_tok[q] == bb) {
                    long r = bp_next[q];
                    bp_tok[p] = nid; bp_next[p] = r;
                    if (r != -1) bp_prev[r] = p;
                    alive--;
                    p = r;
                } else p = q;
            }
        }
    }
}

/* segment a raw span with the frozen merge list; returns unit count, fills ids */
static long bpe_segment(const unsigned char *W, long from, long to, int *ids, long cap)
{
    static int *tk = NULL; static long tcap = 0;
    long n = to - from, i, k, m;
    if (n <= 0) return 0;
    if (tcap < n) { tcap = n; tk = (int *)realloc(tk, sizeof(int) * (size_t)tcap); }
    for (i = 0; i < n; i++) tk[i] = W[from + i];
    for (m = 0; m < n_mrg; m++) {
        int a = mrg[m].a, b = mrg[m].b;
        long w = 0;
        for (i = 0; i < n; ) {
            if (i + 1 < n && tk[i] == a && tk[i+1] == b) { tk[w++] = 256 + (int)m; i += 2; }
            else tk[w++] = tk[i++];
        }
        n = w;
    }
    for (k = 0; k < n && k < cap; k++) ids[k] = tk[k];
    return n;
}

/* ---- Body-0 unit chain: orders 0..2 over units, epsilon 0.1, R2 floor ----
 * Court 2 fixes epsilon 0.1 per level and the R2 length-aware escape U(u).
 * The context depth is forced by the court-4 seam clause: old_units supplies
 * "the last one or two local unit-context ids needed to price the first two
 * new units", i.e. the deepest context is two units.                        */
#define HBITS 20
#define HSIZE (1L << HBITS)
typedef struct { uint64_t *key; long *val; long *touched; long ntouch; } Hmap;
static Hmap hm1, hm1t, hm2, hm2t;

static void hm_init(Hmap *h)
{
    long i;
    h->key = (uint64_t *)malloc(sizeof(uint64_t) * (size_t)HSIZE);
    h->val = (long *)malloc(sizeof(long) * (size_t)HSIZE);
    h->touched = (long *)malloc(sizeof(long) * (size_t)HSIZE);
    for (i = 0; i < HSIZE; i++) h->key[i] = 0;
    h->ntouch = 0;
}
static void hm_clear(Hmap *h)
{
    long i;
    for (i = 0; i < h->ntouch; i++) h->key[h->touched[i]] = 0;
    h->ntouch = 0;
}
static void hm_add(Hmap *h, uint64_t k, long d)
{
    uint64_t kk = k + 1, slot = (kk * 11400714819323198485ULL) >> (64 - HBITS);
    while (h->key[slot] && h->key[slot] != kk) slot = (slot + 1) & (uint64_t)(HSIZE - 1);
    if (!h->key[slot]) { h->key[slot] = kk; h->val[slot] = 0; h->touched[h->ntouch++] = (long)slot; }
    h->val[slot] += d;
}
static void hm_put(Hmap *h, uint64_t k, long v)
{
    uint64_t kk = k + 1, slot = (kk * 11400714819323198485ULL) >> (64 - HBITS);
    while (h->key[slot] && h->key[slot] != kk) slot = (slot + 1) & (uint64_t)(HSIZE - 1);
    if (!h->key[slot]) { h->key[slot] = kk; h->touched[h->ntouch++] = (long)slot; }
    h->val[slot] = v;
}
static long hm_get(Hmap *h, uint64_t k)
{
    uint64_t kk = k + 1, slot = (kk * 11400714819323198485ULL) >> (64 - HBITS);
    while (h->key[slot]) {
        if (h->key[slot] == kk) return h->val[slot];
        slot = (slot + 1) & (uint64_t)(HSIZE - 1);
    }
    return 0;
}

static long ch_c0[256 + MAX_MERGES];
static long ch_N0;
static double ch_U[256 + MAX_MERGES];
static int ch_inv;
static long ch_S0[256]; static double ch_SU[256];  /* order-0 aggregates per first byte */

/* successor lists per context, for aggregating p_E over the event "expansion starts with d" */
static Hmap hm1h, hm2h;
static int *sc_u = NULL; static long *sc_nx = NULL, sc_n = 0, sc_cap = 0;
static void sc_push(Hmap *head, uint64_t ctx, int unit)
{
    long h = hm_get(head, ctx);           /* stored as index+1; 0 means empty */
    if (sc_n == sc_cap) {
        sc_cap = sc_cap ? sc_cap * 2 : (RUN_BYTES + 8);
        sc_u = (int *)realloc(sc_u, sizeof(int) * (size_t)sc_cap);
        sc_nx = (long *)realloc(sc_nx, sizeof(long) * (size_t)sc_cap);
    }
    sc_u[sc_n] = unit; sc_nx[sc_n] = h - 1;
    hm_put(head, ctx, sc_n + 1);
    sc_n++;
}

static void chain_build(const int *u, long n)
{
    long i; double Z = 0.0;
    ch_inv = 256 + n_mrg;
    for (i = 0; i < ch_inv; i++) ch_c0[i] = 0;
    ch_N0 = 0;
    hm_clear(&hm1); hm_clear(&hm1t); hm_clear(&hm2); hm_clear(&hm2t);
    hm_clear(&hm1h); hm_clear(&hm2h); sc_n = 0;
    for (i = 0; i < ch_inv; i++) Z += exp2(-8.0 * (double)unit_len[i]);
    for (i = 0; i < ch_inv; i++) ch_U[i] = exp2(-8.0 * (double)unit_len[i]) / Z;
    for (i = 0; i < 256; i++) { ch_S0[i] = 0; ch_SU[i] = 0.0; }
    for (i = 0; i < n; i++) { ch_c0[u[i]]++; ch_N0++; }
    for (i = 0; i < ch_inv; i++) { ch_S0[unit_fb[i]] += ch_c0[i]; ch_SU[unit_fb[i]] += ch_U[i]; }
    for (i = 1; i < n; i++) {
        uint64_t k = ((uint64_t)u[i-1] << 12) | (uint64_t)u[i];
        hm_add(&hm1t, (uint64_t)u[i-1], 1);
        if (hm_get(&hm1, k) == 0) sc_push(&hm1h, (uint64_t)u[i-1], u[i]);
        hm_add(&hm1, k, 1);
    }
    for (i = 2; i < n; i++) {
        uint64_t cx = ((uint64_t)u[i-2] << 12) | (uint64_t)u[i-1];
        uint64_t k = (cx << 12) | (uint64_t)u[i];
        hm_add(&hm2t, cx, 1);
        if (hm_get(&hm2, k) == 0) sc_push(&hm2h, cx, u[i]);
        hm_add(&hm2, k, 1);
    }
}

/* p(u) = A2*c2[cx][u] + A1*c1[p1][u] + A0*c0[u] + AU*U(u); the same coefficients let
 * p_E(d) = sum over units whose expansion starts with d be aggregated in one pass. */
static double cf_A2, cf_A1, cf_A0, cf_AU;
static void chain_coeffs(int p1, int p2)
{
    long t1 = (p1 >= 0) ? hm_get(&hm1t, (uint64_t)p1) : 0;
    long t2 = 0; double w1, w0;
    if (p1 >= 0 && p2 >= 0) t2 = hm_get(&hm2t, ((uint64_t)p2 << 12) | (uint64_t)p1);
    cf_A2 = (t2 > 0) ? 0.9 / (double)t2 : 0.0;
    w1 = (t2 > 0) ? 0.1 : 1.0;
    cf_A1 = (t1 > 0) ? w1 * 0.9 / (double)t1 : 0.0;
    w0 = (t1 > 0) ? w1 * 0.1 : w1;
    cf_A0 = (ch_N0 > 0) ? w0 * 0.9 / (double)ch_N0 : 0.0;
    cf_AU = (ch_N0 > 0) ? w0 * 0.1 : w0;
}
/* p_E for every destination byte at the current position */
static double pE_buf[256];
static void chain_pE(int p1, int p2)
{
    int b; long s;
    for (b = 0; b < 256; b++) pE_buf[b] = cf_A0 * (double)ch_S0[b] + cf_AU * ch_SU[b];
    if (cf_A1 != 0.0) {
        for (s = hm_get(&hm1h, (uint64_t)p1) - 1; s >= 0; s = sc_nx[s]) {
            int u2 = sc_u[s];
            pE_buf[unit_fb[u2]] += cf_A1 * (double)hm_get(&hm1, ((uint64_t)p1 << 12) | (uint64_t)u2);
        }
    }
    if (cf_A2 != 0.0) {
        uint64_t cx = ((uint64_t)p2 << 12) | (uint64_t)p1;
        for (s = hm_get(&hm2h, cx) - 1; s >= 0; s = sc_nx[s]) {
            int u2 = sc_u[s];
            pE_buf[unit_fb[u2]] += cf_A2 * (double)hm_get(&hm2, (cx << 12) | (uint64_t)u2);
        }
    }
}
static double chain_p(int u, int p1, int p2)
{
    double p0, pp1, pp2;
    p0 = (ch_N0 == 0) ? ch_U[u]
       : 0.9 * ((double)ch_c0[u] / (double)ch_N0) + 0.1 * ch_U[u];
    if (p1 < 0) return p0;
    {
        long t1 = hm_get(&hm1t, (uint64_t)p1);
        if (t1 == 0) pp1 = p0;
        else pp1 = 0.9 * ((double)hm_get(&hm1, ((uint64_t)p1 << 12) | (uint64_t)u) / (double)t1) + 0.1 * p0;
    }
    if (p2 < 0) return pp1;
    {
        uint64_t cx = ((uint64_t)p2 << 12) | (uint64_t)p1;
        long t2 = hm_get(&hm2t, cx);
        if (t2 == 0) pp2 = pp1;
        else pp2 = 0.9 * ((double)hm_get(&hm2, (cx << 12) | (uint64_t)u) / (double)t2) + 0.1 * pp1;
    }
    return pp2;
}

/* ---- source cargo: unit-start events on A-train (TRANSFER4_DRAFT.md) ---- */
static Hmap cg, cgt;
static long cg_c0[256], cg_starts;
static uint64_t cg_key(const int *c, int k)
{
    uint64_t v = (uint64_t)k << 24;
    if (k > 0) v |= (uint64_t)c[0] << 16;
    if (k > 1) v |= (uint64_t)c[1] << 8;
    if (k > 2) v |= (uint64_t)c[2];
    return v;
}
static void cargo_build(const unsigned char *A, long na)
{
    static int *au = NULL;
    long nu, i, ro = 0;
    au = (int *)malloc(sizeof(int) * (size_t)(na + 8));
    bpe_learn(A, na);
    nu = bpe_segment(A, 0, na, au, na + 8);
    hm_init(&cg); hm_init(&cgt);
    for (i = 0; i < 256; i++) cg_c0[i] = 0;
    cg_starts = 0;
    for (i = 0; i < nu; i++) {
        int s = unit_fb[au[i]], ctx[3], k;
        cg_c0[s]++; cg_starts++;
        for (k = 1; k <= 3 && ro >= k; k++) {
            int z; uint64_t kc;
            for (z = 0; z < k; z++) ctx[z] = A[ro - k + z];   /* oldest to newest */
            kc = cg_key(ctx, k);
            hm_add(&cgt, kc, 1);
            hm_add(&cg, (kc << 8) | (uint64_t)s, 1);
        }
        ro += unit_len[au[i]];
    }
    free(au);
}
/* P_start(s | c), c held oldest-to-newest, length k.
 * P_k is defined recursively ON P_(k-1), so the unrolled evaluation starts at the
 * base P_0 and builds upward: the longest context is applied LAST and therefore
 * carries the outermost 0.9 weight, each shorter context entering through 0.1. */
static double cargo_p(int s, const int *c, int k)
{
    double p = 0.9 * ((double)cg_c0[s] / (double)cg_starts) + 0.1 / 256.0;
    int j;
    for (j = 1; j <= k; j++) {
        const int *cc = c + (k - j);            /* the j newest bytes = suffix of length j */
        uint64_t kc = cg_key(cc, j);
        long tot = hm_get(&cgt, kc);
        if (tot > 0) p = 0.9 * ((double)hm_get(&cg, (kc << 8) | (uint64_t)s) / (double)tot) + 0.1 * p;
    }
    return p;
}

/* ------------------------- relation book (court-4 RelationKey) ---------- */
typedef struct {
    int ts, td, tep, cl, cs[3], cd[3], cep[3];
    long seen, pos, neg; double led, peak; int st; double L; int ever;
} Rel;
static Rel *rb[SVA]; static long rb_n[SVA], rb_cap[SVA];
#define RBBITS 15
#define RBSIZE (1 << RBBITS)
static int rb_tab[SVA][RBSIZE];
static uint64_t rel_hash(const Rel *r)
{
    uint64_t v = 1469598103934665603ULL; int i2;
    v = v * 1099511628211ULL + (uint64_t)r->ts;
    v = v * 1099511628211ULL + (uint64_t)r->td;
    v = v * 1099511628211ULL + (uint64_t)r->tep;
    v = v * 1099511628211ULL + (uint64_t)r->cl;
    for (i2 = 0; i2 < 3; i2++) {
        v = v * 1099511628211ULL + (uint64_t)r->cs[i2];
        v = v * 1099511628211ULL + (uint64_t)r->cd[i2];
        v = v * 1099511628211ULL + (uint64_t)r->cep[i2];
    }
    return v;
}
static int rel_same(const Rel *a, const Rel *b)
{
    int i2;
    if (a->ts != b->ts || a->td != b->td || a->tep != b->tep || a->cl != b->cl) return 0;
    for (i2 = 0; i2 < 3; i2++)
        if (a->cs[i2] != b->cs[i2] || a->cd[i2] != b->cd[i2] || a->cep[i2] != b->cep[i2]) return 0;
    return 1;
}
static long rel_find(int arm, const Rel *k)
{
    uint64_t h = rel_hash(k), slot = h >> (64 - RBBITS);
    while (rb_tab[arm][slot]) {
        long ix = rb_tab[arm][slot] - 1;
        if (rel_same(&rb[arm][ix], k)) return ix;
        slot = (slot + 1) & (uint64_t)(RBSIZE - 1);
    }
    if (rb_n[arm] + 1 >= (long)(RBSIZE / 2)) { fprintf(stderr, "relation table overflow\n"); exit(3); }
    if (rb_n[arm] == rb_cap[arm]) {
        rb_cap[arm] = rb_cap[arm] ? rb_cap[arm] * 2 : 4096;
        rb[arm] = (Rel *)realloc(rb[arm], sizeof(Rel) * (size_t)rb_cap[arm]);
    }
    rb[arm][rb_n[arm]] = *k;
    rb[arm][rb_n[arm]].seen = 0; rb[arm][rb_n[arm]].pos = 0; rb[arm][rb_n[arm]].neg = 0;
    rb[arm][rb_n[arm]].led = 0; rb[arm][rb_n[arm]].peak = 0;
    rb[arm][rb_n[arm]].st = 0; rb[arm][rb_n[arm]].L = 0; rb[arm][rb_n[arm]].ever = 0;
    rb_tab[arm][slot] = (int)rb_n[arm] + 1;
    return rb_n[arm]++;
}
/* frozen RelationKey lexicographic order, used only to break an exact ledger tie */
static int rel_cmp(const Rel *a, const Rel *b)
{
    int i2;
    if (a->ts != b->ts) return a->ts - b->ts;
    if (a->td != b->td) return a->td - b->td;
    if (a->tep != b->tep) return a->tep - b->tep;
    if (a->cl != b->cl) return a->cl - b->cl;
    for (i2 = 0; i2 < 3; i2++) {
        if (a->cs[i2] != b->cs[i2]) return a->cs[i2] - b->cs[i2];
        if (a->cd[i2] != b->cd[i2]) return a->cd[i2] - b->cd[i2];
        if (a->cep[i2] != b->cep[i2]) return a->cep[i2] - b->cep[i2];
    }
    return 0;
}

/* --------------------------------------------------------------- models */
static double prof_A_R[256][256], prof_A_L[256][256];
static double H_A_R[256], H_A_L[256];
static long cntA[256], firstA[256], NA;

static double prof_D_R[256][256], prof_D_L[256][256];
static double H_D_R[256], H_D_L[256];

static int cmp_desc(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x < y) - (x > y);
}
static double ent(const double *p)
{
    int i; double h = 0.0;
    for (i = 0; i < 256; i++) if (p[i] > 0.0) h -= p[i] * log2(p[i]);
    return h;
}
static double js_sorted(const double *p, double hp, const double *q, double hq)
{
    int i; double hm = 0.0;
    for (i = 0; i < 256; i++) {
        double m = 0.5 * (p[i] + q[i]);
        if (m > 0.0) hm -= m * log2(m);
    }
    return hm - 0.5 * hp - 0.5 * hq;
}
static void kt_row(const long *cnt, double *out)
{
    int i; long s = 0;
    for (i = 0; i < 256; i++) s += cnt[i];
    for (i = 0; i < 256; i++) out[i] = ((double)cnt[i] + 0.5) / ((double)s + 128.0);
}

/* ------------------------------------------------- shared working state */
static double Bm[256][256], Fm[256][256];
static int alive_s[256], n_as, alive_d[256], n_ad;
static long firstD[256], cntD[256];
static long RD[256][256], LD[256][256];

static int argmax_row(const double (*M)[256], int s, const int *cand, int ncand, const long *firstc)
{
    int i, best = -1; double bv = 0; long bf = 0;
    for (i = 0; i < ncand; i++) {
        int d = cand[i]; double v = M[s][d];
        if (best < 0 || v > bv || (v == bv && firstc[d] < bf)) { best = d; bv = v; bf = firstc[d]; }
    }
    return best;
}
static int argmax_col(const double (*M)[256], int d, const int *cand, int ncand, const long *firstc)
{
    int i, best = -1; double bv = 0; long bf = 0;
    for (i = 0; i < ncand; i++) {
        int s = cand[i]; double v = M[s][d];
        if (best < 0 || v > bv || (v == bv && firstc[s] < bf)) { best = s; bv = v; bf = firstc[s]; }
    }
    return best;
}
/* mutual-best admission: bonly = B mutual best, adm = B and F mutual best */
static void admit(const double (*Bx)[256], const double (*Fx)[256], int *bonly, int *adm)
{
    int rowB[256], colB[256], rowF[256], colF[256];
    int i, s, d;
    for (i = 0; i < 256; i++) { rowB[i] = colB[i] = rowF[i] = colF[i] = -1; bonly[i] = adm[i] = -1; }
    for (i = 0; i < n_as; i++) {
        s = alive_s[i];
        rowB[s] = argmax_row(Bx, s, alive_d, n_ad, firstD);
        rowF[s] = argmax_row(Fx, s, alive_d, n_ad, firstD);
    }
    for (i = 0; i < n_ad; i++) {
        d = alive_d[i];
        colB[d] = argmax_col(Bx, d, alive_s, n_as, firstA);
        colF[d] = argmax_col(Fx, d, alive_s, n_as, firstA);
    }
    for (i = 0; i < n_as; i++) {
        s = alive_s[i]; d = rowB[s];
        if (d >= 0 && colB[d] == s) {
            bonly[s] = d;
            if (rowF[s] == d && colF[d] == s) adm[s] = d;
        }
    }
}

/* ------------------------------------------------------ sortable records */
typedef struct { int chunk, mapid, s, d, epoch, correct; } MRow;
static int mrow_cmp(const void *a, const void *b)
{
    const MRow *x = (const MRow *)a, *y = (const MRow *)b;
    if (x->chunk != y->chunk) return x->chunk - y->chunk;
    if (x->mapid != y->mapid) return x->mapid - y->mapid;
    return x->s - y->s;
}
typedef struct { int chunk, arm, d, prof, fixed; } SRow;
static int srow_cmp(const void *a, const void *b)
{
    const SRow *x = (const SRow *)a, *y = (const SRow *)b;
    if (x->chunk != y->chunk) return x->chunk - y->chunk;
    if (x->arm != y->arm) return x->arm - y->arm;
    return x->d - y->d;
}
typedef struct { const unsigned char *p; int l; long ti; } KV;
static int kvcmp(const void *a, const void *b)
{
    const KV *x = (const KV *)a, *y = (const KV *)b;
    int n = x->l < y->l ? x->l : y->l;
    int r = memcmp(x->p, y->p, (size_t)n);
    if (r) return r;
    return x->l - y->l;
}

/* per-chunk evidence tables (chunk x arm) */
static double ev_bits[300][22];
static long ev_pos[300][22], ev_pairs[300][22], ev_rels[300][22], ev_cearn[300][22];
static long ev_ee[300][22], ev_re[300][22], ev_wu[300][22], ev_sa[300][22], ev_byt[300];
static long wcnt_[300][22], ecnt_[300][22], rcnt_[300][22];
/* A5 Boolean-gate clause: G accumulated independently and from the artifacts */
static double myGF[8][22], myGN2[8][22], aGF2[8][22], aGN2[8][22];

/* ================================================================= main */
int main(void)
{
    char hx[65]; long n;
    int i, j, k, c, s, d;
    long lenA, lenD;
    unsigned char *A, *D;
    int cipher[256], seenb[256];
    unsigned char *w_plain, *w_iso, *w_swap, *w_ff = NULL;
    long lenff = 0;
    int perm_src[256], perm_dst[256];

    printf("# transfer4_check -- independent verifier, DEVELOPMENT run\n");
    printf("# status\tclass\tdetail\n");
    fflush(stdout);
    {
        long sn = (long)SVW * SVB * SVA * 256, z;
        sv_adm = (int *)malloc(sizeof(int) * (size_t)sn);
        sv_ep  = (int *)malloc(sizeof(int) * (size_t)sn);
        for (z = 0; z < sn; z++) { sv_adm[z] = -1; sv_ep[z] = 0; }
    }

    /* ----------------------------------------------------- C01 freeze */
    {
        struct { const char *path; const char *sha; long bytes; } rec[3] = {
            { REPO "COURT3_SNAPSHOT.md", "20df368e719f275b4eb655c8c34b4f197904e5873aa77e5e1bf767604e883423", 8103 },
            { REPO "TRANSFER4_DRAFT.md", "b17abc1e546134374977dc1ecf501cb0af8b6a4db62570c70cc3e068e499f0fe", 27895 },
            { REPO "transfer4.c",        "d3b10b8c74ef3285926f7ff9c99253c11531f933e743b31654b0abcc1fd282c1", 79965 }
        };
        Tsv fz = tsv_load(REPO "COURT4_FREEZE.tsv");
        PF("C01_FREEZE", fz.n == 4, "COURT4_FREEZE.tsv rows=%d (header + 3 stages)", fz.n);
        for (i = 0; i < 3; i++) {
            sha256_file(rec[i].path, hx, &n);
            PF("C01_FREEZE", !strcmp(hx, rec[i].sha) && n == rec[i].bytes,
               "%s sha256=%s bytes=%ld", rec[i].path, hx, n);
            if (i + 1 < fz.n)
                PF("C01_FREEZE", !strcmp(fs(&fz, i + 1, 3), rec[i].sha) && fi(&fz, i + 1, 2) == rec[i].bytes,
                   "receipt stage %s agrees with the recomputed digest and length", fs(&fz, i + 1, 0));
        }
        tsv_free(&fz);
    }

    /* --------------------------------------------------- raw inputs */
    A = slurp(SRCA, &lenA);
    D = slurp(BASED, &lenD);
    NA = (9 * lenA) / 10;          /* A1: integer arithmetic, never binary64 0.9 */
    Ob("C02_INPUT", "A=%ld bytes ; A-train (first 90%%) = %ld ; base D = %ld bytes", lenA, NA, lenD);
    PF("C02_INPUT", lenD >= RUN_BYTES, "base D length %ld >= frozen RUN_BYTES %d", lenD, RUN_BYTES);

    {
        static long RA[256][256], LA[256][256];
        long tmp[256];
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
        Ob("C02_INPUT", "alive source bytes in A-train = %d ; A-train[0] = %u", n_as, A[0]);
    }

    /* ------------------------------------------- C03 cipher / worlds */
    {
        Tsv oc = tsv_load(ART "oracle_cipher.tsv");
        int okperm = 1;
        PF("C03_CIPHER", oc.n == 256, "oracle_cipher.tsv rows=%d (expected 256)", oc.n);
        for (i = 0; i < 256; i++) { seenb[i] = 0; cipher[i] = -1; }
        for (i = 0; i < oc.n && i < 256; i++) {
            long a = fi(&oc, i, 0), b = fi(&oc, i, 1);
            if (a != i || b < 0 || b > 255 || seenb[b]) okperm = 0; else seenb[b] = 1;
            cipher[i] = (int)b;
        }
        PF("C03_CIPHER", okperm, "oracle cipher is a bijection of 0..255 in index order");
        tsv_free(&oc);
        Ob("C03_CIPHER", "CLOSED in pass 2: the permutation is derived from court-2 seed 0xB170C5 and reproduces "
                         "oracle_cipher.tsv exactly - see C23_WORLDGEN");
    }

    w_plain = (unsigned char *)malloc(RUN_BYTES);
    w_iso   = (unsigned char *)malloc(RUN_BYTES);
    w_swap  = (unsigned char *)malloc(RUN_BYTES);
    memcpy(w_plain, D, RUN_BYTES);
    for (i = 0; i < RUN_BYTES; i++) w_iso[i] = (unsigned char)cipher[D[i]];
    for (i = 0; i < RUN_BYTES; i++) {
        unsigned char b = D[i];
        w_swap[i] = (b == 'a') ? (unsigned char)'e' : (b == 'e') ? (unsigned char)'a' : b;
    }

    /* W_ff reconstruction (complete law, TRANSFER4_DRAFT.md "False friends") */
    {
        typedef struct { long off; int len; long cnt; long first; int pair; } Wrd;
        long ntok = 0, capt = 1 << 18, ncand = 0, p2 = 0, gstart;
        long *toff = (long *)malloc(sizeof(long) * (size_t)capt);
        int  *tlen = (int  *)malloc(sizeof(int)  * (size_t)capt);
        KV *kv; Wrd top[16]; int ntop = 0;

        while (p2 < lenD) {
            unsigned char b = D[p2];
            long st;
            if (b == 32 || b == 10 || b == 13 || b == 9) { p2++; continue; }
            st = p2;
            while (p2 < lenD) { unsigned char q = D[p2]; if (q==32||q==10||q==13||q==9) break; p2++; }
            if (ntok == capt) { capt *= 2; toff = (long *)realloc(toff, sizeof(long)*(size_t)capt); tlen = (int *)realloc(tlen, sizeof(int)*(size_t)capt); }
            toff[ntok] = st; tlen[ntok] = (int)(p2 - st); ntok++;
        }
        kv = (KV *)malloc(sizeof(KV) * (size_t)(ntok ? ntok : 1));
        for (i = 0; i < ntok; i++)
            if (tlen[i] >= 1 && tlen[i] <= 63) { kv[ncand].p = D + toff[i]; kv[ncand].l = tlen[i]; kv[ncand].ti = i; ncand++; }
        qsort(kv, (size_t)ncand, sizeof(KV), kvcmp);
        gstart = 0;
        while (gstart < ncand) {
            long gend = gstart + 1, cnt, first = -1;
            Wrd cur;
            while (gend < ncand && kv[gend].l == kv[gstart].l &&
                   memcmp(kv[gend].p, kv[gstart].p, (size_t)kv[gstart].l) == 0) gend++;
            cnt = gend - gstart;
            for (j = 0; j < cnt; j++) { long o = toff[kv[gstart+j].ti]; if (first < 0 || o < first) first = o; }
            cur.off = first; cur.len = kv[gstart].l; cur.cnt = cnt; cur.first = first; cur.pair = -1;
            if (ntop < 16) top[ntop++] = cur;
            else {
                int worst = 0;
                for (j = 1; j < ntop; j++)
                    if (top[j].cnt < top[worst].cnt || (top[j].cnt == top[worst].cnt && top[j].first > top[worst].first)) worst = j;
                if (cur.cnt > top[worst].cnt || (cur.cnt == top[worst].cnt && cur.first < top[worst].first)) top[worst] = cur;
            }
            gstart = gend;
        }
        PF("C02_WORLD_FF", ntop == 16, "base D yields %d distinct 1..63-byte candidate words (>=16 required or construction aborts)", ntop);
        for (i = 0; i < ntop; i++)
            for (j = i + 1; j < ntop; j++)
                if (top[j].cnt > top[i].cnt || (top[j].cnt == top[i].cnt && top[j].first < top[i].first))
                    { Wrd t = top[i]; top[i] = top[j]; top[j] = t; }
        for (i = 0; i + 1 < ntop; i += 2) { top[i].pair = i + 1; top[i+1].pair = i; }
        {
            char buf[80];
            for (i = 0; i < ntop; i++) {
                int L2 = top[i].len > 60 ? 60 : top[i].len;
                memcpy(buf, D + top[i].off, (size_t)L2); buf[L2] = 0;
                for (j = 0; j < L2; j++) if ((unsigned char)buf[j] < 32) buf[j] = '.';
                Ob("C02_WORLD_FF", "rank %2d count=%-7ld first=%-9ld word=\"%s\"", i + 1, top[i].cnt, top[i].first, buf);
            }
        }
        {
            long cap = lenD * 2 + 16, out = 0, pos = 0;
            w_ff = (unsigned char *)malloc((size_t)cap);
            while (pos < lenD) {
                unsigned char b = D[pos];
                long st; int L2, hit = -1;
                if (b == 32 || b == 10 || b == 13 || b == 9) { w_ff[out++] = b; pos++; continue; }
                st = pos;
                while (pos < lenD) { unsigned char q = D[pos]; if (q==32||q==10||q==13||q==9) break; pos++; }
                L2 = (int)(pos - st);
                if (L2 >= 1 && L2 <= 63)
                    for (i = 0; i < ntop; i++)
                        if (top[i].len == L2 && memcmp(D + st, D + top[i].off, (size_t)L2) == 0) { hit = i; break; }
                if (hit >= 0) { int q2 = top[hit].pair; memcpy(w_ff + out, D + top[q2].off, (size_t)top[q2].len); out += top[q2].len; }
                else { memcpy(w_ff + out, D + st, (size_t)L2); out += L2; }
            }
            lenff = out;
        }
        free(kv); free(toff); free(tlen);
        PF("C02_WORLD_FF", lenff >= RUN_BYTES, "ff transformed stream length %ld >= RUN_BYTES %d", lenff, RUN_BYTES);
    }

    /* compare reconstructed worlds against artifacts */
    {
        struct { const char *file; unsigned char *mine; const char *lbl; } wl[4] = {
            { ART "W_plain.bin",   NULL, "plain = base D prefix" },
            { ART "W_iso.bin",     NULL, "iso = oracle cipher applied to base D prefix" },
            { ART "W_swap_ea.bin", NULL, "swap_ea = base D with every raw a and e transposed" },
            { ART "W_ff.bin",      NULL, "ff = frozen 16-word swap over the whole base D" }
        };
        wl[0].mine = w_plain; wl[1].mine = w_iso; wl[2].mine = w_swap; wl[3].mine = w_ff;
        for (i = 0; i < 4; i++) {
            long ln; unsigned char *art = slurp(wl[i].file, &ln);
            long bad = -1;
            if (ln != RUN_BYTES) bad = -2;
            else for (j = 0; j < RUN_BYTES; j++) if (art[j] != wl[i].mine[j]) { bad = j; break; }
            if (bad == -1) P("C02_WORLD", "%s : %s reproduced byte-for-byte over %d bytes", wl[i].file, wl[i].lbl, RUN_BYTES);
            else if (bad == -2) Fl("C02_WORLD", "%s length %ld != %d", wl[i].file, ln, RUN_BYTES);
            else Fl("C02_WORLD", "%s : first divergence at byte %ld (artifact %u, reconstruction %u) [%s]",
                    wl[i].file, bad, art[bad], wl[i].mine[bad], wl[i].lbl);
            free(art);
        }
            Ob("C02_WORLD", "CLOSED in pass 2: W_half.bin is reconstructed byte-for-byte under court-2 A9 - see C23_WORLDGEN");
        Ob("C02_WORLD", "CLOSED in pass 2: the A2 causality pair is reconstructed byte-for-byte - see C23_WORLDGEN");
        {
            long ln; unsigned char *hf = slurp(ART "W_half.bin", &ln);
            Ob("C02_WORLD", "W_half.bin[0..65536) equals reconstructed iso[0..65536) : %s",
               (ln == RUN_BYTES && memcmp(hf, w_iso, 65536) == 0) ? "yes" : "no");
            free(hf);
        }
        {
            long ln; unsigned char *g1 = slurp(ART "W_ghost.bin", &ln);
            long ug[256], up[256]; double l1 = 0;
            for (i = 0; i < 256; i++) { ug[i] = 0; up[i] = 0; }
            for (i = 0; i < RUN_BYTES; i++) { ug[g1[i]]++; up[w_plain[i]]++; }
            for (i = 0; i < 256; i++) l1 += fabs((double)ug[i] - (double)up[i]) / (double)RUN_BYTES;
            Ob("C02_WORLD", "ghost vs plain raw unigram L1 = %.6f", l1);
            free(g1);
            Ob("C08_GHOSTINV", "CLOSED in pass 2: court-2 A3 supplies both thresholds (L1 <= 0.01 vs D, bigram MI <= 0.01 bits) "
                               "and both are recomputed and graded - see C24_A3GHOST");
        }
    }

    /* ------------------------------------------ C04 null scramble fixture */
    {
        int perm[8]; long nbad = 0, abad = 0;
        Tsv fx;
        for (k = 0; k < 19; k++) {
            null_perm(NULL_SEED[k], 8, perm);
            for (i = 0; i < 8; i++) {
                int got = FIX_ARRIVE[perm[i]];
                if (got != FIX_TABLE[k][i]) {
                    if (nbad < 5) Fl("C04_NULLFIX", "null%d arrival index %d: frozen protocol table %d, reconstruction %d",
                                     k, i, FIX_TABLE[k][i], got);
                    nbad++;
                }
            }
        }
        PF("C04_NULLFIX", nbad == 0, "19 null permutations rebuilt from the frozen seeds reproduce the protocol's own "
                                     "152-cell construction table (%ld mismatches)", nbad);
        fx = tsv_load(ART "null_scramble_fixture.tsv");
        for (i = 1; i < fx.n; i++) {
            int arm = mapid_of(fs(&fx, i, 0)) - 2;
            int ai = (int)fi(&fx, i, 2), dst = (int)fi(&fx, i, 3), pr = (int)fi(&fx, i, 4);
            int exp_ = (int)fi(&fx, i, 5), ex = (int)fi(&fx, i, 6), minev;
            unsigned long long sd = strtoull(fs(&fx, i, 1), NULL, 16);
            if (arm < 0 || arm > 18 || ai < 0 || ai > 7) { abad++; continue; }
            null_perm(NULL_SEED[arm], 8, perm);
            minev = FIX_ARRIVE[perm[ai]];
            if (sd != NULL_SEED[arm] || dst != FIX_ARRIVE[ai] || pr != minev ||
                exp_ != FIX_TABLE[arm][ai] || ex != 1) {
                if (abad < 5) Fl("C04_NULLFIX", "null_scramble_fixture.tsv line %d: arm=%s dest=%d profile=%d expected=%d exact=%d ; reconstruction profile=%d",
                                 i + 1, fs(&fx, i, 0), dst, pr, exp_, ex, minev);
                abad++;
            }
        }
        PF("C04_NULLFIX", abad == 0 && fx.n == 153,
           "null_scramble_fixture.tsv %d data rows agree with the reconstruction (%ld mismatches)", fx.n - 1, abad);
        tsv_free(&fx);
    }

    /* ------------------------------------------------ C05 chunk seam fixture */
    {
        Tsv sf = tsv_load(ART "chunk_seam_fixture.tsv");
        int ok = sf.n == 2 && !strcmp(fs(&sf, 1, 0), "PASS") &&
                 fi(&sf, 1, 1) == 17 && fi(&sf, 1, 2) == 9 && fi(&sf, 1, 3) == 17 &&
                 fi(&sf, 1, 4) == 18 && fi(&sf, 1, 5) == 1 && fi(&sf, 1, 6) == 1 && fi(&sf, 1, 7) == 1;
        PF("C05_SEAMFIX", ok, "chunk_seam_fixture.tsv equals the frozen seam numbers: 17-byte prefix -> 9 units covering 17; "
                              "illegal 18-byte resegmentation covers 18; seam-clamped new chunk = 1 unit covering 1 byte");
        tsv_free(&sf);
    }

    /* ------------------------------------------- C18 downstream fixture */
    {
        Tsv su = tsv_load(ART "downstream_fixture_summary.tsv");
        Tsv tr = tsv_load(ART "downstream_fixture.tsv");
        Tsv ev = tsv_load(ART "downstream_fixture_events.tsv");
        Tsv wn = tsv_load(ART "downstream_fixture_winners.tsv");
        int src = (int)fi(&su, 1, 2), dst = (int)fi(&su, 1, 3);
        double r = fd(&su, 1, 5), pE = fd(&su, 1, 6);
        double dw = fd(&su, 1, 8), dl = fd(&su, 1, 9);
        long earn_step = fi(&su, 1, 10), rev_step = fi(&su, 1, 11);
        long ret_ep = fi(&su, 1, 12), fr_ep = fi(&su, 1, 13);
        double cl0_led = fd(&su, 1, 25); long cl0_state = fi(&su, 1, 26);
        double cl0_L = fd(&su, 1, 27); long cl0_ever = fi(&su, 1, 28);
        long exact_arms = fi(&su, 1, 29);
        double my_dw = log2(r / pE), my_dl = log2((1.0 - r) / (1.0 - pE));

        PF("C18_DOWNFIX", dst == 255, "fixture admitted destination is byte 255 (exercises the map alphabet law)");
        PF("C18_DOWNFIX", ret_ep == 1 && fr_ep == 3,
           "epoch law: retired key epoch %ld, fresh key epoch %ld after remove and re-admit", ret_ep, fr_ep);
        PF("C18_DOWNFIX", cl0_led >= EARN_BITS && cl0_state == 0 && cl0_L == 0.0 && cl0_ever == 0,
           "context_len=0 key ledger %.6f crosses EARN yet keeps state=%ld ever_earned=%ld L=%g",
           cl0_led, cl0_state, cl0_ever, cl0_L);
        PF("C18_DOWNFIX", a5_fix_prob(my_dw, dw),
           "delta(win) = log2(r/p_E) = %.17g vs artifact %.17g", my_dw, dw);
        PF("C18_DOWNFIX", a5_fix_prob(my_dl, dl),
           "delta(loss) = log2((1-r)/(1-p_E)) = %.17g vs artifact %.17g", my_dl, dl);
        {
            double led = 0.0, L = 0.0; int st = 0;
            long pos = 0, neg = 0, earns = 0, revs = 0, wins = 0;
            long nbad = 0, firstbad = -1, my_earn = -1, my_rev = -1;
            for (i = 1; i < tr.n; i++) {
                int truth = (int)fi(&tr, i, 2);
                double p_local = fd(&tr, i, 3), p_final = fd(&tr, i, 4);
                double a_led = fd(&tr, i, 5), a_L = fd(&tr, i, 7);
                long a_st = fi(&tr, i, 6), a_seen = fi(&tr, i, 8), a_pos = fi(&tr, i, 9);
                long a_neg = fi(&tr, i, 10), a_e = fi(&tr, i, 11), a_r = fi(&tr, i, 12), a_w = fi(&tr, i, 13);
                double q = (truth == 255) ? p_local * r / pE : p_local * (1.0 - r) / (1.0 - pE);
                double my_pf = (1.0 - L) * p_local + L * q;
                double delta = log2(q / p_local);
                int bad = 0;
                if (L > 0.0) wins++;
                led += delta;
                if (truth == 255) pos++; else neg++;
                if (st == 1) { double nl = L * exp(0.05 * delta); if (nl < 0.01) nl = 0.01; if (nl > 0.5) nl = 0.5; L = nl; }
                if (st == 0 && led >= EARN_BITS) { st = 1; L = 0.05; earns++; if (my_earn < 0) my_earn = fi(&tr, i, 0); }
                else if (st == 1 && led < REVOKE_BITS) { st = 0; L = 0.0; revs++; if (my_rev < 0) my_rev = fi(&tr, i, 0); }
                if (!a5_fix_prob(my_pf, p_final)) bad = 1;
                if (!a5_fix_abs(led, a_led, 1e-9)) bad = 1;
                if (a_st != st) bad = 1;
                if (!a5_fix_abs(a_L, L, 1e-12)) bad = 1;
                if (a_seen != pos + neg || a_pos != pos || a_neg != neg) bad = 1;
                if (a_e != earns || a_r != revs || a_w != wins) bad = 1;
                if (fi(&tr, i, 0) != i - 1) bad = 1;
                if (bad) { if (firstbad < 0) firstbad = i; nbad++; }
            }
            PF("C18_DOWNFIX", nbad == 0,
               "downstream_fixture.tsv %d-step trace replayed from the frozen specialist and authority law (%ld divergent rows)",
               tr.n - 1, nbad);
            if (firstbad >= 0) Fl("C18_DOWNFIX", "first divergent trace row is file line %ld", firstbad + 1);
            PF("C18_DOWNFIX", earns == 1 && revs == 1 && pos >= 1 && neg >= 1,
               "replay: %ld EARN, %ld REVOKE, %ld positive receipts, %ld negative receipts", earns, revs, pos, neg);
            PF("C18_DOWNFIX", my_earn == earn_step && my_rev == rev_step,
               "replayed earn_step=%ld revoke_step=%ld match the summary row (%ld / %ld)", my_earn, my_rev, earn_step, rev_step);
            PF("C18_DOWNFIX", wins * 20 == wn.n - 1,
               "%ld live-voice positions x 20 arms = %ld rows ; downstream_fixture_winners.tsv holds %d", wins, wins * 20, wn.n - 1);
        }
        {
            int narm = 0, armseen[22]; long badlock = 0;
            for (i = 0; i < 22; i++) armseen[i] = 0;
            for (i = 1; i < ev.n; i++) { int a = mapid_of(fs(&ev, i, 2)); if (a >= 0 && !armseen[a]) { armseen[a] = 1; narm++; } }
            PF("C18_DOWNFIX", narm == 20 && exact_arms == 20,
               "downstream_fixture_events.tsv carries %d distinct arms (summary claims exact_arms=%ld)", narm, exact_arms);
            for (i = 1; i < ev.n; i++) {
                int base = -1, jj;
                for (jj = 1; jj < ev.n; jj++)
                    if (fi(&ev, jj, 1) == fi(&ev, i, 1) && !strcmp(fs(&ev, jj, 3), fs(&ev, i, 3))) { base = jj; break; }
                if (base < 0) continue;
                for (jj = 4; jj < ev.r[i].nf; jj++) if (strcmp(fs(&ev, i, jj), fs(&ev, base, jj))) badlock++;
            }
            PF("C18_DOWNFIX", badlock == 0, "all fixture EARN/REVOKE events are bit-identical across the 20 arms (%ld divergences)", badlock);
            badlock = 0;
            for (i = 1; i < wn.n; i++) {
                int base = -1, jj;
                for (jj = 1; jj < wn.n; jj++) if (fi(&wn, jj, 1) == fi(&wn, i, 1)) { base = jj; break; }
                if (base < 0) continue;
                for (jj = 3; jj < wn.r[i].nf; jj++) if (strcmp(fs(&wn, i, jj), fs(&wn, base, jj))) badlock++;
            }
            PF("C18_DOWNFIX", badlock == 0, "all fixture live-voice rows are bit-identical across the 20 arms (%ld divergences)", badlock);
        }
        Ob("C18_DOWNFIX", "CLOSED in pass 2: the A-train unit learner is now available, and argmax_s P_start(s|s,s,s) is "
                          "recomputed and agrees with the declared s=%d", src);
        Gp("C18_DOWNFIX", "UNVERIFIED NON-VERDICT TELEMETRY (A6.6): p_local = %.17g and r for the fixture-not-court "
                          "downstream fixture are taken as artifact-declared inputs to the replay rather than rebuilt from "
                          "the 4096-zero-byte model. The fixture contributes no scientific result and enters no primary "
                          "verdict or label; everything downstream of those two inputs is reconstructed and graded.", pE);
        tsv_free(&su); tsv_free(&tr); tsv_free(&ev); tsv_free(&wn);
    }

    /* ================================================== per-world engine */
    rename_perm(0xB06726C4ULL, perm_src);
    rename_perm(0x62FD57F1ULL, perm_dst);
    {
        static const char *wname[9] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2","neutralB" };
        static const char *wfile[9] = { "W_iso.bin","W_plain.bin","W_ghost.bin","W_half.bin","W_ff.bin",
                                        "W_swap_ea.bin","W_aux1.bin","W_aux2.bin","W_iso.bin" };
        int wx;
        for (wx = 0; wx < 9; wx++) {
            char path[512];
            long wlen;
            unsigned char *W;
            int neutral = !strcmp(wname[wx], "neutralB");
            int nb, truemap[256], true_known = 0, no_true;
            Tsv T_maps, T_msum, T_scr;
            MRow *amap, *mine; SRow *ascr, *smine;
            int namap = 0, nascr = 0, nmine = 0, nsmine = 0, mcap, scap;
            int prev_live[256], ep_live[256], prev_or[256], ep_or[256];
            static int prev_null[19][256], ep_null[19][256], prev_prof[19][256];
            static int surf_bit[300];
            int surf_any = 0, have_prev_prof = 0;
            long rename_fail = 0, scr_bij_fail = 0, scr_growth_fail = 0, t_prev = 0;

            sprintf(path, ART "%s", wfile[wx]);
            W = slurp(path, &wlen);
            nb = (int)(wlen / CHUNK);
            for (i = 0; i < 256; i++) truemap[i] = -1;
            if (!strcmp(wname[wx], "iso") || neutral) { for (i = 0; i < 256; i++) truemap[i] = cipher[i]; true_known = 1; }
            else if (!strcmp(wname[wx], "plain") || !strcmp(wname[wx], "ff")) { for (i = 0; i < 256; i++) truemap[i] = i; true_known = 1; }
            else if (!strcmp(wname[wx], "swap_ea")) { for (i = 0; i < 256; i++) truemap[i] = i; truemap['a'] = 'e'; truemap['e'] = 'a'; true_known = 1; }
            /* court 2: W-half's lived half and the A2 causality pair are cipher of D under the W-iso permutation */
            else if (!strcmp(wname[wx], "half") || !strcmp(wname[wx], "aux1") || !strcmp(wname[wx], "aux2"))
                { for (i = 0; i < 256; i++) truemap[i] = cipher[i]; true_known = 1; }

            no_true = !true_known;   /* A4: ghost is the only world with no true byte correspondence */
            sprintf(path, ART "maps_%s.tsv", wname[wx]);              T_maps = tsv_load(path);
            sprintf(path, ART "map_summary_%s.tsv", wname[wx]);       T_msum = tsv_load(path);
            sprintf(path, ART "profile_scrambles_%s.tsv", wname[wx]); T_scr  = tsv_load(path);

            amap = (MRow *)malloc(sizeof(MRow) * (size_t)(T_maps.n + 1));
            for (i = 1; i < T_maps.n; i++) {
                amap[namap].chunk = (int)fi(&T_maps, i, 0);
                amap[namap].mapid = mapid_of(fs(&T_maps, i, 2));
                amap[namap].s = (int)fi(&T_maps, i, 3);
                amap[namap].d = (int)fi(&T_maps, i, 4);
                amap[namap].epoch = (int)fi(&T_maps, i, 5);
                amap[namap].correct = (int)fi(&T_maps, i, 6);
                namap++;
            }
            qsort(amap, (size_t)namap, sizeof(MRow), mrow_cmp);
            ascr = (SRow *)malloc(sizeof(SRow) * (size_t)(T_scr.n + 1));
            for (i = 1; i < T_scr.n; i++) {
                ascr[nascr].chunk = (int)fi(&T_scr, i, 0);
                ascr[nascr].arm = mapid_of(fs(&T_scr, i, 2)) - 2;
                ascr[nascr].d = (int)fi(&T_scr, i, 3);
                ascr[nascr].prof = (int)fi(&T_scr, i, 4);
                ascr[nascr].fixed = (int)fi(&T_scr, i, 5);
                nascr++;
            }
            qsort(ascr, (size_t)nascr, sizeof(SRow), srow_cmp);

            mcap = 65536; mine = (MRow *)malloc(sizeof(MRow) * (size_t)mcap);
            scap = 65536; smine = (SRow *)malloc(sizeof(SRow) * (size_t)scap);

            for (i = 0; i < 256; i++) { prev_live[i] = -1; ep_live[i] = 0; prev_or[i] = -1; ep_or[i] = 0; }
            for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) { prev_null[k][i] = -1; ep_null[k][i] = 0; prev_prof[k][i] = -1; }
            for (i = 0; i < 256; i++) { cntD[i] = 0; firstD[i] = -1; }
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
                for (i = 0; i < n_ad; i++)
                    for (j = i + 1; j < n_ad; j++)
                        if (firstD[arr[j]] < firstD[arr[i]]) { int tt = arr[i]; arr[i] = arr[j]; arr[j] = tt; }

                { long tmp[256];
                  for (i = 0; i < n_ad; i++) {
                      d = alive_d[i];
                      for (j = 0; j < 256; j++) tmp[j] = RD[d][j];
                      kt_row(tmp, prof_D_R[d]); qsort(prof_D_R[d], 256, sizeof(double), cmp_desc); H_D_R[d] = ent(prof_D_R[d]);
                      for (j = 0; j < 256; j++) tmp[j] = LD[d][j];
                      kt_row(tmp, prof_D_L[d]); qsort(prof_D_L[d], 256, sizeof(double), cmp_desc); H_D_L[d] = ent(prof_D_L[d]);
                  } }

                for (i = 0; i < n_as; i++) {
                    double fa;
                    s = alive_s[i];
                    fa = log2(((double)cntA[s] + 0.5) / ((double)NA + 128.0));
                    for (j = 0; j < n_ad; j++) {
                        double fdv;
                        d = alive_d[j];
                        Bm[s][d] = neutral ? 0.0
                                 : -(js_sorted(prof_A_R[s], H_A_R[s], prof_D_R[d], H_D_R[d]) +
                                     js_sorted(prof_A_L[s], H_A_L[s], prof_D_L[d], H_D_L[d]));
                        fdv = log2(((double)cntD[d] + 0.5) / ((double)t + 128.0));
                        Fm[s][d] = -fabs(fa - fdv);
                    }
                }

                admit((const double (*)[256])Bm, (const double (*)[256])Fm, bonly, adm);
                for (i = 0; i < 256; i++) if (prev_live[i] != adm[i]) ep_live[i]++;

                {   /* A3: the gate is evaluated for W_swap_ea only; elsewhere the field is exactly zero */
                    int bit = 0;
                    if (!strcmp(wname[wx], "swap_ea") &&
                        cntA['a'] > 0 && cntA['e'] > 0 && cntD['a'] > 0 && cntD['e'] > 0)
                        if (adm['a'] == 'a' || adm['e'] == 'e') bit = 1;
                    surf_bit[c] = bit; if (bit) surf_any = 1;
                }

                {   /* rename-commutation machine law */
                    static double B2[256][256], F2[256][256];
                    int as2[256], ad2[256], save_as[256], save_ad[256], save_nas, save_nad;
                    long fA2[256], fD2[256], save_fA[256], save_fD[256];
                    int b2[256], a2[256];
                    memcpy(save_as, alive_s, sizeof save_as); memcpy(save_ad, alive_d, sizeof save_ad);
                    save_nas = n_as; save_nad = n_ad;
                    memcpy(save_fA, firstA, sizeof save_fA); memcpy(save_fD, firstD, sizeof save_fD);
                    for (i = 0; i < n_as; i++) as2[i] = perm_src[alive_s[i]];
                    for (i = 0; i < n_ad; i++) ad2[i] = perm_dst[alive_d[i]];
                    for (i = 0; i < 256; i++) { fA2[perm_src[i]] = firstA[i]; fD2[perm_dst[i]] = firstD[i]; }
                    for (i = 0; i < n_as; i++) for (j = 0; j < n_ad; j++) {
                        B2[perm_src[alive_s[i]]][perm_dst[alive_d[j]]] = Bm[alive_s[i]][alive_d[j]];
                        F2[perm_src[alive_s[i]]][perm_dst[alive_d[j]]] = Fm[alive_s[i]][alive_d[j]];
                    }
                    memcpy(alive_s, as2, sizeof as2); memcpy(alive_d, ad2, sizeof ad2);
                    memcpy(firstA, fA2, sizeof fA2); memcpy(firstD, fD2, sizeof fD2);
                    admit((const double (*)[256])B2, (const double (*)[256])F2, b2, a2);
                    memcpy(alive_s, save_as, sizeof save_as); memcpy(alive_d, save_ad, sizeof save_ad);
                    n_as = save_nas; n_ad = save_nad;
                    memcpy(firstA, save_fA, sizeof save_fA); memcpy(firstD, save_fD, sizeof save_fD);
                    for (i = 0; i < 256; i++) {
                        int e1 = (adm[i] < 0) ? -1 : perm_dst[adm[i]];
                        int e2 = (bonly[i] < 0) ? -1 : perm_dst[bonly[i]];
                        if (a2[perm_src[i]] != e1) rename_fail++;
                        if (b2[perm_src[i]] != e2) rename_fail++;
                    }
                }

                for (k = 0; k < 19; k++) {
                    int p[256], rho[256], cnt2[256], b2[256];
                    static double Bk[256][256];
                    null_perm(NULL_SEED[k], n_ad, p);
                    for (i = 0; i < 256; i++) { rho[i] = -1; cnt2[i] = 0; }
                    for (i = 0; i < n_ad; i++) rho[arr[i]] = arr[p[i]];
                    for (i = 0; i < n_ad; i++) {
                        int v = rho[alive_d[i]];
                        if (v < 0 || cntD[v] == 0) scr_bij_fail++; else cnt2[v]++;
                    }
                    for (i = 0; i < n_ad; i++) if (cnt2[alive_d[i]] != 1) scr_bij_fail++;
                    if (have_prev_prof) {
                        int changed = 0, newly = 0;
                        for (i = 0; i < 256; i++) {
                            if (prev_prof[k][i] >= 0 && rho[i] != prev_prof[k][i]) changed++;
                            if (prev_prof[k][i] < 0 && rho[i] >= 0) newly++;
                        }
                        if (changed > newly) scr_growth_fail++;
                    }
                    for (i = 0; i < n_ad; i++) {
                        if (nsmine == scap) { scap *= 2; smine = (SRow *)realloc(smine, sizeof(SRow) * (size_t)scap); }
                        smine[nsmine].chunk = c; smine[nsmine].arm = k;
                        smine[nsmine].d = alive_d[i]; smine[nsmine].prof = rho[alive_d[i]];
                        smine[nsmine].fixed = (rho[alive_d[i]] == alive_d[i]);
                        nsmine++;
                    }
                    for (i = 0; i < n_as; i++) { s = alive_s[i];
                        for (j = 0; j < n_ad; j++) { d = alive_d[j]; Bk[s][d] = Bm[s][rho[d]]; } }
                    admit((const double (*)[256])Bk, (const double (*)[256])Fm, b2, adm_null[k]);
                    for (i = 0; i < 256; i++) if (prev_null[k][i] != adm_null[k][i]) ep_null[k][i]++;
                    for (i = 0; i < 256; i++) prev_prof[k][i] = rho[i];
                }
                have_prev_prof = 1;

                for (i = 0; i < 256; i++) adm_or[i] = -1;
                for (i = 0; i < 256; i++) if (adm[i] >= 0) {
                    int td = true_known ? truemap[i] : -1;
                    adm_or[i] = (td >= 0 && cntD[td] > 0) ? td : adm[i];
                }
                for (i = 0; i < 256; i++) if (prev_or[i] != adm_or[i]) ep_or[i]++;

#define PUSH(mid, ss, dd, ee) do { \
    if (nmine == mcap) { mcap *= 2; mine = (MRow *)realloc(mine, sizeof(MRow) * (size_t)mcap); } \
    mine[nmine].chunk = c; mine[nmine].mapid = (mid); mine[nmine].s = (ss); mine[nmine].d = (dd); \
    mine[nmine].epoch = (ee); \
    mine[nmine].correct = no_true ? -1 : ((truemap[ss] == (dd)) ? 1 : 0); nmine++; } while (0)
                for (i = 0; i < 256; i++) if (bonly[i] >= 0) PUSH(0, i, bonly[i], 0);
                for (i = 0; i < 256; i++) if (adm[i] >= 0) PUSH(1, i, adm[i], ep_live[i]);
                for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) if (adm_null[k][i] >= 0) PUSH(2 + k, i, adm_null[k][i], ep_null[k][i]);
                /* A4: a world with no true byte correspondence has no oracle arm and no oracle rows */
                if (!no_true) for (i = 0; i < 256; i++) if (adm_or[i] >= 0) PUSH(21, i, adm_or[i], ep_or[i]);
#undef PUSH
                if (wx < SVW && c < SVB) {   /* stash every arm's map for the price layer */
                    for (i = 0; i < 256; i++) {
                        sv_adm[SVIX(wx, c, 0) + i] = adm[i];  sv_ep[SVIX(wx, c, 0) + i] = ep_live[i];
                        sv_adm[SVIX(wx, c, 20) + i] = adm_or[i]; sv_ep[SVIX(wx, c, 20) + i] = ep_or[i];
                    }
                    for (k = 0; k < 19; k++) for (i = 0; i < 256; i++) {
                        sv_adm[SVIX(wx, c, 1 + k) + i] = adm_null[k][i];
                        sv_ep[SVIX(wx, c, 1 + k) + i] = ep_null[k][i];
                    }
                }
                memcpy(prev_live, adm, sizeof prev_live);
                memcpy(prev_or, adm_or, sizeof prev_or);
                for (k = 0; k < 19; k++) memcpy(prev_null[k], adm_null[k], sizeof prev_null[k]);
            }
            qsort(mine, (size_t)nmine, sizeof(MRow), mrow_cmp);

            {   /* maps */
                long bad = 0, ia = 0, im = 0; int shown = 0;
                while (ia < namap || im < nmine) {
                    int cres;
                    if (ia >= namap) cres = 1;
                    else if (im >= nmine) cres = -1;
                    else cres = mrow_cmp(&amap[ia], &mine[im]);
                    if (cres == 0) {
                        int fb = 0;
                        if (amap[ia].d != mine[im].d || amap[ia].epoch != mine[im].epoch) fb = 1;
                        if (amap[ia].correct != mine[im].correct) fb = 1;
                        if (fb) {
                            bad++;
                            if (shown < 3) { Fl("C06_ADMIT", "maps_%s.tsv chunk=%d map=%s source=%d : artifact dest=%d epoch=%d correct=%d ; reconstruction dest=%d epoch=%d correct=%d",
                                wname[wx], amap[ia].chunk, mapname_of(amap[ia].mapid), amap[ia].s,
                                amap[ia].d, amap[ia].epoch, amap[ia].correct, mine[im].d, mine[im].epoch, mine[im].correct); shown++; }
                        }
                        ia++; im++;
                    } else if (cres < 0) {
                        bad++;
                        if (shown < 3) { Fl("C06_ADMIT", "maps_%s.tsv chunk=%d map=%s source=%d dest=%d in artifact only",
                            wname[wx], amap[ia].chunk, mapname_of(amap[ia].mapid), amap[ia].s, amap[ia].d); shown++; }
                        ia++;
                    } else {
                        bad++;
                        if (shown < 3) { Fl("C06_ADMIT", "maps_%s.tsv chunk=%d map=%s source=%d dest=%d in reconstruction only",
                            wname[wx], mine[im].chunk, mapname_of(mine[im].mapid), mine[im].s, mine[im].d); shown++; }
                        im++;
                    }
                }
                PF("C06_ADMIT", bad == 0, "maps_%s.tsv : %d artifact rows vs %d reconstructed rows (bonly + live + 19 nulls + oracle over %d boundaries) - %ld divergences",
                   wname[wx], namap, nmine, nb, bad);
            }
            {   /* scrambles */
                long bad = 0, ia = 0, im = 0; int shown = 0;
                while (ia < nascr || im < nsmine) {
                    int cres;
                    if (ia >= nascr) cres = 1; else if (im >= nsmine) cres = -1;
                    else cres = srow_cmp(&ascr[ia], &smine[im]);
                    if (cres == 0) {
                        if (ascr[ia].prof != smine[im].prof || ascr[ia].fixed != smine[im].fixed) {
                            bad++;
                            if (shown < 3) { Fl("C08_SCRAMBLE", "profile_scrambles_%s.tsv chunk=%d null%d dest=%d : artifact profile=%d fixed=%d ; reconstruction profile=%d fixed=%d",
                                wname[wx], ascr[ia].chunk, ascr[ia].arm, ascr[ia].d, ascr[ia].prof, ascr[ia].fixed, smine[im].prof, smine[im].fixed); shown++; }
                        }
                        ia++; im++;
                    } else if (cres < 0) { bad++;
                        if (shown < 3) { Fl("C08_SCRAMBLE", "profile_scrambles_%s.tsv chunk=%d null%d dest=%d in artifact only", wname[wx], ascr[ia].chunk, ascr[ia].arm, ascr[ia].d); shown++; }
                        ia++;
                    } else { bad++;
                        if (shown < 3) { Fl("C08_SCRAMBLE", "profile_scrambles_%s.tsv chunk=%d null%d dest=%d in reconstruction only", wname[wx], smine[im].chunk, smine[im].arm, smine[im].d); shown++; }
                        im++; }
                }
                PF("C08_SCRAMBLE", bad == 0, "profile_scrambles_%s.tsv : %d artifact rows vs %d reconstructed rows - %ld divergences",
                   wname[wx], nascr, nsmine, bad);
                PF("C08_SCRAMBLE", scr_bij_fail == 0, "%s : rho_k is a bijection of exactly the alive destination profiles at every boundary (%ld violations)", wname[wx], scr_bij_fail);
                PF("C08_SCRAMBLE", scr_growth_fail == 0, "%s : changed assignments among previously alive destinations never exceed newly alive count (%ld violations)", wname[wx], scr_growth_fail);
            }
            PF("C12_RENAME", rename_fail == 0, "%s : admission map commutes exactly with the two frozen label renamings at all %d boundaries (%ld violations)", wname[wx], nb, rename_fail);
            {   /* map_summary */
                long bad = 0; int shown = 0;
                for (i = 1; i < T_msum.n; i++) {
                    int ch = (int)fi(&T_msum, i, 0), sb = (int)fi(&T_msum, i, 6), rowbad = 0;
                    long lv = fi(&T_msum, i, 1), bp = fi(&T_msum, i, 2), bh = fi(&T_msum, i, 3);
                    long ap = fi(&T_msum, i, 4), ah = fi(&T_msum, i, 5);
                    long mbp = 0, mbh = 0, mapn = 0, mah = 0;
                    if (no_true) { mbh = -1; mah = -1; }
                    for (j = 0; j < nmine; j++) if (mine[j].chunk == ch) {
                        if (mine[j].mapid == 0) { mbp++; if (!no_true) mbh += mine[j].correct; }
                        else if (mine[j].mapid == 1) { mapn++; if (!no_true) mah += mine[j].correct; }
                    }
                    if (lv != (long)ch * CHUNK) rowbad = 1;
                    if (mbp != bp || mapn != ap) rowbad = 1;
                    if (mbh != bh || mah != ah) rowbad = 1;   /* A4: -1 means undefined */
                    if (ch >= 0 && ch < 300 && sb != surf_bit[ch]) rowbad = 1;
                    if (rowbad) { bad++;
                        if (shown < 3) { Fl("C11_MAPSUM", "map_summary_%s.tsv chunk=%d : artifact bonly=%ld/%ld admitted=%ld/%ld surface=%d ; reconstruction bonly=%ld/%ld admitted=%ld/%ld surface=%d",
                            wname[wx], ch, bp, bh, ap, ah, sb, mbp, mbh, mapn, mah, surf_bit[ch]); shown++; } }
                }
                PF("C11_MAPSUM", bad == 0, "map_summary_%s.tsv : %d rows agree with the reconstruction (%ld divergences)", wname[wx], T_msum.n - 1, bad);
            }
            Ob("C13_SURFACE", "%s : SURFACE_ADMITTED = %d (A3: the gate is evaluated for swap_ea only; elsewhere exactly zero)", wname[wx], surf_any);
            if (!strcmp(wname[wx], "swap_ea"))
                PF("C13_SURFACE", surf_any == 0, "swap_ea blocking gate: no boundary admits a->a or e->e while a and e are alive on both sides");

            free(mine); free(smine); free(amap); free(ascr); free(W);
            tsv_free(&T_maps); tsv_free(&T_msum); tsv_free(&T_scr);
            fflush(stdout);
        }
    }

    /* ------------------------------------------- neutral-B construction law */
    {
        Tsv nl = tsv_load(ART "neutral_B_law.tsv");
        Tsv ev, rl;
        long bad = 0; int shown = 0;
        PF("C14_NEUTRAL", nl.n == 2 && !strcmp(fs(&nl, 1, 0), "iso") && fi(&nl, 1, 1) == RUN_BYTES &&
                          fi(&nl, 1, 2) == 19 && fi(&nl, 1, 3) == 1,
           "neutral_B_law.tsv: world=%s bytes=%ld arms_compared=%ld exact=%ld",
           fs(&nl, 1, 0), fi(&nl, 1, 1), fi(&nl, 1, 2), fi(&nl, 1, 3));
        tsv_free(&nl);
        ev = tsv_load(ART "evidence_neutralB.tsv");
        for (c = 0; c < 128; c++) {
            int base = -1;
            for (i = 1; i < ev.n; i++) {
                int a;
                if (fi(&ev, i, 0) != c) continue;
                a = armid_of(fs(&ev, i, 3));
                if (a < 1 || a > 20) continue;
                if (base < 0) { base = i; continue; }
                for (j = 4; j < ev.r[i].nf; j++)
                    if (strcmp(fs(&ev, i, j), fs(&ev, base, j))) {
                        bad++;
                        if (shown < 3) { Fl("C14_NEUTRAL", "evidence_neutralB.tsv chunk=%d field %d: arm %s = %s but arm %s = %s",
                            c, j, fs(&ev, i, 3), fs(&ev, i, j), fs(&ev, base, 3), fs(&ev, base, j)); shown++; }
                        break;
                    }
            }
        }
        PF("C14_NEUTRAL", bad == 0, "evidence_neutralB.tsv: live arm and all 19 structural nulls bit-identical in every field at every chunk (%ld divergences)", bad);
        tsv_free(&ev);
        rl = tsv_load(ART "relations_neutralB.tsv");
        Ob("C14_NEUTRAL", "relations_neutralB.tsv holds %d relation rows; with constant B the strict mutual-best law admits "
                          "no pair, so the neutral court is vacuous downstream exactly as the protocol warns", rl.n - 1);
        tsv_free(&rl);
    }

    /* ------------------------------- forced surface-shadow books (swap_ea) */
    {
        Tsv sh = tsv_load(ART "surface_shadows_swap_ea.tsv");
        long bpair = 0, bepoch = 0, bctx = 0, bauth = 0, bseen = 0;
        int shown = 0;
        for (i = 1; i < sh.n; i++) {
            const char *pr = fs(&sh, i, 0);
            long ts = fi(&sh, i, 1), td = fi(&sh, i, 2), te = fi(&sh, i, 3), cl = fi(&sh, i, 4);
            long sn = fi(&sh, i, 14), po = fi(&sh, i, 15), ng = fi(&sh, i, 16);
            long st = fi(&sh, i, 19), evd = fi(&sh, i, 21);
            double L = fd(&sh, i, 20);
            int want = !strcmp(pr, "a_to_a") ? 'a' : !strcmp(pr, "e_to_e") ? 'e' : -1;
            if (want < 0 || ts != want || td != want) bpair++;
            if (te != 1) bepoch++;
            for (j = 0; j < 3; j++) {
                long cs = fi(&sh, i, 5 + 3*j), cd = fi(&sh, i, 6 + 3*j), ce = fi(&sh, i, 7 + 3*j);
                if (j < cl) { if (cs != cd || ce != 1) bctx++; }
                else if (cs != 0 || cd != 0 || ce != 0) bctx++;
            }
            if (cl < 0 || cl > 3) bctx++;
            if (st != 0 || L != 0.0 || evd != 0) {
                bauth++;
                if (shown < 3) { Fl("C21_SURFSHADOW", "surface_shadows_swap_ea.tsv line %d carries a nonzero authority field: state=%ld L=%.17g ever_earned=%ld",
                                    i + 1, st, L, evd); shown++; }
            }
            if (sn != po + ng) bseen++;
        }
        PF("C21_SURFSHADOW", bpair == 0, "surface_shadows_swap_ea.tsv: all %d rows hold the forced pairs a->a / e->e with target_s == target_d (%ld violations)", sh.n - 1, bpair);
        PF("C21_SURFSHADOW", bepoch == 0, "surface_shadows_swap_ea.tsv: every book keeps target epoch 1 (%ld violations)", bepoch);
        PF("C21_SURFSHADOW", bctx == 0, "surface_shadows_swap_ea.tsv: identity context map (c_s == c_d), every context epoch fixed to 1, every unused field zero (%ld violations)", bctx);
        PF("C21_SURFSHADOW", bauth == 0, "surface_shadows_swap_ea.tsv: permanent shadow authority - state, L and ever_earned are zero in every row (%ld violations)", bauth);
        PF("C21_SURFSHADOW", bseen == 0, "surface_shadows_swap_ea.tsv: seen == positive + negative in every row (%ld violations)", bseen);
        tsv_free(&sh);
        Gp("C21_SURFSHADOW", "UNVERIFIED NON-VERDICT TELEMETRY (A6.6): the forced surface-shadow ledger VALUES are not "
                             "independently repriced; only the frozen structural and authority laws are graded. By "
                             "TRANSFER4_DRAFT.md this telemetry is not an arm and never enters the blocking "
                             "SURFACE_ADMITTED gate, any primary verdict, or any label.");
    }

    /* ============ PASS 2: objects the court-2 snapshot now defines ======= */
    {
        /* ---- C22 source pins ---- */
        sha256_file(SRCA, hx, &n);
        PF("C22_SOURCE", !strcmp(hx, "02c08152e281d28e48e17a2b6813bb693dfa255c94f30e033137409d0e8b5cfb") && n == 447545,
           "A-train source netta.txt sha256=%s bytes=%ld (court-2 pin)", hx, n);
        sha256_file(BASED, hx, &n);
        PF("C22_SOURCE", !strcmp(hx, "76e8246b462c9d697fff5f14e5d25c131620397bb85112c9cc10132d03ef61a2") && n == 1301310,
           "base text D combined.clean.txt sha256=%s bytes=%ld (court-2 pin)", hx, n);
        PF("C22_SOURCE", NA == 402790, "A1 exact A-train = floor((9*%ld)/10) = %ld bytes", lenA, NA);
    }
    {
        /* ---- C23 world generation from the court-2 seeds ---- */
        int cip[256];
        long cntD_full[256], totD = 0;
        unsigned char *gh, *hf, *a1, *a2, *art;
        long ln, bad, i2;
        rename_perm(SEED_CIPHER, cip);
        {
            long diff = -1;
            for (i = 0; i < 256; i++) if (cip[i] != cipher[i]) { diff = i; break; }
            if (diff < 0) P("C23_WORLDGEN", "iso cipher derived from Fisher-Yates seed 0xB170C5 reproduces all 256 rows of oracle_cipher.tsv");
            else Fl("C23_WORLDGEN", "iso cipher from seed 0xB170C5 diverges at byte %ld: artifact %d, derived %d", diff, cipher[diff], cip[diff]);
        }
        for (i = 0; i < 256; i++) cntD_full[i] = 0;
        for (i2 = 0; i2 < lenD; i2++) cntD_full[D[i2]]++;
        totD = lenD;
        gh = (unsigned char *)malloc(RUN_BYTES);
        ghost_stream(SEED_GHOST, cntD_full, totD, gh, RUN_BYTES);
        art = slurp(ART "W_ghost.bin", &ln);
        bad = -1; for (i2 = 0; i2 < RUN_BYTES; i2++) if (art[i2] != gh[i2]) { bad = i2; break; }
        if (bad < 0) P("C23_WORLDGEN", "W_ghost.bin reproduced byte-for-byte from the i.i.d. unigram law, seed 0x5EED02");
        else Fl("C23_WORLDGEN", "W_ghost.bin first divergence at byte %ld (artifact %u, derived %u)", bad, art[bad], gh[bad]);
        free(art);
        hf = (unsigned char *)malloc(RUN_BYTES);
        for (i2 = 0; i2 < 65536; i2++) hf[i2] = (unsigned char)cipher[D[i2]];
        ghost_stream(SEED_HALFTAIL, cntD_full, totD, hf + 65536, 65536);
        art = slurp(ART "W_half.bin", &ln);
        bad = -1; for (i2 = 0; i2 < RUN_BYTES; i2++) if (art[i2] != hf[i2]) { bad = i2; break; }
        if (bad < 0) P("C23_WORLDGEN", "W_half.bin reproduced byte-for-byte (A9: cipher of D[0..65536) then unigram continuation, seed 0x5EED03)");
        else Fl("C23_WORLDGEN", "W_half.bin first divergence at byte %ld (artifact %u, derived %u)", bad, art[bad], hf[bad]);
        free(art);
        a1 = (unsigned char *)malloc(16384);
        for (i2 = 0; i2 < 16384; i2++) a1[i2] = w_iso[i2];
        art = slurp(ART "W_aux1.bin", &ln);
        bad = (ln != 16384) ? -2 : -1;
        if (bad == -1) for (i2 = 0; i2 < 16384; i2++) if (art[i2] != a1[i2]) { bad = i2; break; }
        if (bad == -1) P("C23_WORLDGEN", "W_aux1.bin reproduced byte-for-byte (A2/A9: 8192-byte shared prefix + W-iso's own continuation, 16384 total)");
        else if (bad == -2) Fl("C23_WORLDGEN", "W_aux1.bin length %ld != 16384", ln);
        else Fl("C23_WORLDGEN", "W_aux1.bin first divergence at byte %ld (artifact %u, derived %u)", bad, art[bad], a1[bad]);
        free(art);
        a2 = (unsigned char *)malloc(16384);
        for (i2 = 0; i2 < 8192; i2++) a2[i2] = w_iso[i2];
        ghost_stream(SEED_AUX2, cntD_full, totD, a2 + 8192, 8192);
        art = slurp(ART "W_aux2.bin", &ln);
        bad = (ln != 16384) ? -2 : -1;
        if (bad == -1) for (i2 = 0; i2 < 16384; i2++) if (art[i2] != a2[i2]) { bad = i2; break; }
        if (bad == -1) P("C23_WORLDGEN", "W_aux2.bin reproduced byte-for-byte (A2: shared prefix + ghost-law continuation B, seed 0x5EED04)");
        else if (bad == -2) Fl("C23_WORLDGEN", "W_aux2.bin length %ld != 16384", ln);
        else Fl("C23_WORLDGEN", "W_aux2.bin first divergence at byte %ld (artifact %u, derived %u)", bad, art[bad], a2[bad]);
        free(art);

        /* ---- C24 A3 ghost invariants: preserved unigram, destroyed relations ----
         * The ghost world is "length of D" (court 2); the A3 invariants are properties
         * of that constructed world and are measured on it, not on the 131072-byte run
         * window the court later lives.                                              */
        {
            unsigned char *gfull = (unsigned char *)malloc((size_t)lenD);
            long gc[256]; double l1 = 0, mi = 0;
            static long bg[256][256];
            long rowm[256], colm[256], tot2 = 0;
            Tsv gi = tsv_load(ART "ghost_invariants.tsv");
            ghost_stream(SEED_GHOST, cntD_full, totD, gfull, lenD);
            {
                unsigned char *g1 = slurp(ART "W_ghost.bin", &ln);
                long bad2 = -1;
                for (i2 = 0; i2 < RUN_BYTES; i2++) if (g1[i2] != gfull[i2]) { bad2 = i2; break; }
                PF("C24_A3GHOST", bad2 < 0, "W_ghost.bin is the first %d bytes of the full-length ghost world", RUN_BYTES);
                free(g1);
            }
            for (i = 0; i < 256; i++) { gc[i] = 0; rowm[i] = 0; colm[i] = 0; }
            memset(bg, 0, sizeof bg);
            for (i2 = 0; i2 < lenD; i2++) gc[gfull[i2]]++;
            for (i2 = 0; i2 + 1 < lenD; i2++) { bg[gfull[i2]][gfull[i2+1]]++; tot2++; }
            for (i = 0; i < 256; i++) l1 += fabs((double)gc[i] - (double)cntD_full[i]) / (double)lenD;
            for (i = 0; i < 256; i++) for (j = 0; j < 256; j++) { rowm[i] += bg[i][j]; colm[j] += bg[i][j]; }
            for (i = 0; i < 256; i++) for (j = 0; j < 256; j++) if (bg[i][j]) {
                double pxy = (double)bg[i][j] / (double)tot2;
                double px = (double)rowm[i] / (double)tot2, py = (double)colm[j] / (double)tot2;
                mi += pxy * log2(pxy / (px * py));
            }
            PF("C24_A3GHOST", fabs(l1 - fd(&gi, 0, 1)) < 5e-7,
               "ghost raw unigram L1 over the full-length world = %.6f vs ghost_invariants.tsv %.6f", l1, fd(&gi, 0, 1));
            PF("C24_A3GHOST", fabs(mi - fd(&gi, 1, 1)) < 5e-7,
               "ghost raw bigram MI over the full-length world = %.6f bits vs ghost_invariants.tsv %.6f", mi, fd(&gi, 1, 1));
            PF("C24_A3GHOST", l1 <= 0.01, "A3 preserved-unigram gate: L1 %.6f <= 0.01 vs D", l1);
            PF("C24_A3GHOST", mi <= 0.01, "A3 destroyed-relations gate: bigram MI %.6f <= 0.01 bits", mi);
            Ob("C24_A3GHOST", "measured on the 131072-byte run window instead, the same statistics are L1 0.061890 and "
                              "MI 0.018542 - sampling noise of the shorter window, not a property of the world");
            tsv_free(&gi); free(gfull);
        }
        /* ---- C25 A1 empty-model floor ---- */
        {
            Tsv fx = tsv_load(ART "fixtures.tsv");
            double got = fd(&fx, 0, 1), want = 1.0 / 256.0;
            PF("C25_A1FIXTURE", fx.n == 1 && !strcmp(fs(&fx, 0, 0), "F1_empty_floor") &&
                                got == want && fd(&fx, 0, 3) == want && fi(&fx, 0, 5) == 1,
               "fixtures.tsv F1_empty_floor = %.17g == 2^-8 exactly (R2 length-aware floor, Z=1 over the 256 base units => 8.000000 bits/byte)", got);
            tsv_free(&fx);
        }
        free(gh); free(hf); free(a1); free(a2);
    }
    {
        /* ---- C26 A8 first-chunk causality: exactly 8 bits/byte, every arm, every world ---- */
        static const char *ew8[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        int wx2;
        for (wx2 = 0; wx2 < 8; wx2++) {
            char path[512]; Tsv ev; long bad = 0; int narm = 0;
            sprintf(path, ART "evidence_%s.tsv", ew8[wx2]); ev = tsv_load(path);
            for (i = 1; i < ev.n; i++) {
                if (fi(&ev, i, 0) != 0) continue;
                narm++;
                if (fd(&ev, i, 5) != 8192.0 || fi(&ev, i, 4) != 1024) bad++;
            }
            PF("C26_A8FIRSTCHUNK", bad == 0,
               "evidence_%s.tsv chunk 0: all %d arms present price exactly 8192.000000 bits over 1024 positions (A8 empty-state law, exact)", ew8[wx2], narm);
            if (narm != 22)
                Ob("C26_A8FIRSTCHUNK", "evidence_%s.tsv carries %d arms: A4 gives a world with no true byte "
                   "correspondence no oracle arm", ew8[wx2], narm);
            tsv_free(&ev);
        }
    }
    {
        /* ---- C27 unit learner: R5 BPE + seam-clamped segmentation vs `positions` ---- */
        static const char *ew8[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        static const char *wf8[8] = { "W_iso.bin","W_plain.bin","W_ghost.bin","W_half.bin","W_ff.bin",
                                      "W_swap_ea.bin","W_aux1.bin","W_aux2.bin" };
        static int seg[CHUNK + 8];
        static int *oldu = NULL;
        int wx2;
        if (!oldu) oldu = (int *)malloc(sizeof(int) * (RUN_BYTES + 8));
        hm_init(&hm1); hm_init(&hm1t); hm_init(&hm2); hm_init(&hm2t); hm_init(&hm1h); hm_init(&hm2h);
        for (wx2 = 0; wx2 < 8; wx2++) {
            char path[512]; Tsv ev; long ln, bad = 0, firstbad = -1, mypos, artpos;
            long badcold = 0, firstcold = -1; double mycold = 0.0;
            unsigned char *W; int nb2, c2;
            static long apos[300]; static double acold[300];
            sprintf(path, ART "%s", wf8[wx2]); W = slurp(path, &ln);
            nb2 = (int)(ln / CHUNK);
            sprintf(path, ART "evidence_%s.tsv", ew8[wx2]); ev = tsv_load(path);
            for (i = 0; i < 300; i++) apos[i] = -1;
            for (i = 1; i < ev.n; i++) {
                int ch = (int)fi(&ev, i, 0);
                if (ch >= 0 && ch < 300 && !strcmp(fs(&ev, i, 3), "cold")) apos[ch] = fi(&ev, i, 4);
            }
            for (i = 0; i < 300; i++) acold[i] = -1.0;
            for (i = 1; i < ev.n; i++) {
                int ch = (int)fi(&ev, i, 0);
                if (ch >= 0 && ch < 300 && !strcmp(fs(&ev, i, 3), "cold")) acold[ch] = fd(&ev, i, 5);
            }
            for (c2 = 0; c2 < nb2; c2++) {
                long lived = (long)c2 * CHUNK, nold, q2;
                double bits = 0.0;
                bpe_learn(W, lived);
                mypos = bpe_segment(W, lived, (long)(c2 + 1) * CHUNK, seg, CHUNK + 8);
                artpos = apos[c2];
                if (mypos != artpos) { bad++; if (firstbad < 0) firstbad = c2; }
                nold = bpe_segment(W, 0, lived, oldu, RUN_BYTES + 8);
                chain_build(oldu, nold);
                for (q2 = 0; q2 < mypos; q2++) {
                    int p1 = (q2 >= 1) ? seg[q2-1] : (nold >= 1 ? oldu[nold-1] : -1);
                    int p2 = (q2 >= 2) ? seg[q2-2] : ((nold >= 2 - q2) ? oldu[nold - (2 - q2)] : -1);
                    bits += -log2(chain_p(seg[q2], p1, p2));
                }
                if (acold[c2] >= 0 && !a5_main(bits, acold[c2])) {
                    badcold++;
                    if (firstcold < 0) { firstcold = c2; mycold = bits; }
                }
            }
            PF("C27_UNITS", bad == 0,
               "evidence_%s.tsv priced-unit counts reproduced by the R5 BPE learner (merges<=2048, MIN_PAIR 4) with seam-clamped segmentation over %d chunks (%ld divergences)",
               ew8[wx2], nb2, bad);
            PF("C28_PRICE", badcold == 0,
               "evidence_%s.tsv cold-arm bits reproduced from the Body-0 unit chain (R2 length-aware floor, epsilon 0.1 per level, unit orders 0..2) over %d chunks (%ld divergences)",
               ew8[wx2], nb2, badcold);
            if (firstcold >= 0)
                Fl("C28_PRICE", "evidence_%s.tsv first divergent cold chunk=%ld : artifact bits=%.17g, reconstruction bits=%.17g",
                   ew8[wx2], firstcold, acold[firstcold], mycold);
            if (firstbad >= 0) {
                bpe_learn(W, firstbad * CHUNK);
                mypos = bpe_segment(W, firstbad * CHUNK, (firstbad + 1) * CHUNK, seg, CHUNK + 8);
                Fl("C27_UNITS", "evidence_%s.tsv first divergent chunk=%ld : artifact positions=%ld, reconstruction positions=%ld (merges learned from the lived prefix: %d)",
                   ew8[wx2], firstbad, apos[firstbad], mypos, n_mrg);
            }
            tsv_free(&ev); free(W);
            fflush(stdout);
        }
    }

    {   /* ==== C29: every arm's bits, and the relation books, from my own prices ==== */
        static const char *ew8[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        static const char *wf8[8] = { "W_iso.bin","W_plain.bin","W_ghost.bin","W_half.bin","W_ff.bin",
                                      "W_swap_ea.bin","W_aux1.bin","W_aux2.bin" };
        static int seg[CHUNK + 8];
        static int *oldu2 = NULL;
        static double abits[300][22];
        static long asen[300][22], awu[300][22];
        int wx2, a;
        if (!oldu2) oldu2 = (int *)malloc(sizeof(int) * (RUN_BYTES + 8));
        for (a = 0; a < SVA; a++) { memset(rb_tab[a], 0, sizeof rb_tab[a]); rb[a] = NULL; rb_n[a] = 0; rb_cap[a] = 0; }
        cargo_build(A, NA);
        Ob("C29_ARMS", "source cargo built on A-train: %ld learned-unit-start events over %d units of inventory", cg_starts, 256 + n_mrg);

        for (wx2 = 0; wx2 < 8; wx2++) {
            char path[512]; Tsv ev; long ln; unsigned char *W; int nb2, c2;
            long badbits[22]; long firstbad2[22]; double mybits_first[22]; long pE_bad = 0;
            long badsen = 0, firstsen = -1, sen_mine = 0, badwu = 0, firstwu = -1, wu_mine = 0;
            int sen_arm = 0, wu_arm = 0;
            sprintf(path, ART "%s", wf8[wx2]); W = slurp(path, &ln);
            nb2 = (int)(ln / CHUNK);
            sprintf(path, ART "evidence_%s.tsv", ew8[wx2]); ev = tsv_load(path);
            for (i = 0; i < 300; i++) for (j = 0; j < 22; j++) { abits[i][j] = -1.0; asen[i][j] = -1; awu[i][j] = -1; }
            for (i = 1; i < ev.n; i++) {
                int ch = (int)fi(&ev, i, 0), ai = armid_of(fs(&ev, i, 3));
                if (ai >= 0 && ch >= 0 && ch < 300) {
                    abits[ch][ai] = fd(&ev, i, 5);
                    awu[ch][ai] = fi(&ev, i, 11);
                    asen[ch][ai] = fi(&ev, i, 12);
                }
            }
            for (a = 0; a < 22; a++) { badbits[a] = 0; firstbad2[a] = -1; mybits_first[a] = 0; }
            for (a = 0; a < SVA; a++) { memset(rb_tab[a], 0, sizeof rb_tab[a]); rb_n[a] = 0; }

            for (c2 = 0; c2 < nb2; c2++) {
                long lived = (long)c2 * CHUNK, nold, npos, q2, ro;
                double bits[22]; long senio[22], wuse[22];
                int minv[SVA][256], hasmap[SVA], admlist[SVA][256], nadm[SVA];
                const int *adm_a[SVA], *ep_a[SVA];
                bpe_learn(W, lived);
                npos = bpe_segment(W, lived, (long)(c2 + 1) * CHUNK, seg, CHUNK + 8);
                nold = bpe_segment(W, 0, lived, oldu2, RUN_BYTES + 8);
                chain_build(oldu2, nold);
                for (a = 0; a < 22; a++) { bits[a] = 0.0; senio[a] = 0; wuse[a] = 0; }
                for (a = 0; a < SVA; a++) {
                    hasmap[a] = 0; nadm[a] = 0;
                    for (i = 0; i < 256; i++) minv[a][i] = -1;
                    if (c2 >= 1 && c2 < SVB) {
                        adm_a[a] = sv_adm + SVIX(wx2, c2, a);
                        ep_a[a] = sv_ep + SVIX(wx2, c2, a);
                        for (i = 0; i < 256; i++) if (adm_a[a][i] >= 0) {
                            minv[a][adm_a[a][i]] = (int)i;
                            admlist[a][nadm[a]++] = (int)i;
                            hasmap[a] = 1;
                        }
                    } else { adm_a[a] = NULL; ep_a[a] = NULL; }
                }
                ro = lived;
                for (q2 = 0; q2 < npos; q2++) {
                    int p1 = (q2 >= 1) ? seg[q2-1] : (nold >= 1 ? oldu2[nold-1] : -1);
                    int p2 = (q2 >= 2) ? seg[q2-2] : ((nold >= 2 - q2) ? oldu2[nold - (2 - q2)] : -1);
                    int u = seg[q2], fb = unit_fb[u];
                    double p;
                    chain_coeffs(p1, p2);
                    chain_pE(p1, p2);
                    p = chain_p(u, p1, p2);
                    { double sE = 0; int z2; for (z2 = 0; z2 < 256; z2++) sE += pE_buf[z2];
                      if (fabs(sE - 1.0) > 1e-6) pE_bad++; }   /* A5: inherited pinned 1e-6 */
                    bits[0] += -log2(p);
                    for (a = 0; a < SVA; a++) {
                        int armcol = (a == 0) ? 1 : (a == 20) ? 21 : (1 + a);
                        int cl = 0, cs[3], cd[3], cep[3], nb3 = 0, tb[3];
                        int sIdx, best = -1; double bestled = 0; Rel bestk;
                        double pf; long cand[260]; int ncand = 0;
                        if (!hasmap[a]) { bits[armcol] += -log2(p); continue; }
                        for (j = 1; j <= 3; j++) {
                            long rr = ro - j; int bb, ss;
                            if (rr < 0) break;
                            bb = W[rr]; ss = minv[a][bb];
                            if (ss < 0) break;
                            tb[nb3++] = bb;
                        }
                        cl = nb3;
                        for (j = 0; j < 3; j++) { cs[j] = 0; cd[j] = 0; cep[j] = 0; }
                        for (j = 0; j < cl; j++) {           /* oldest to newest */
                            int bb = tb[cl - 1 - j], ss = minv[a][bb];
                            cs[j] = ss; cd[j] = bb; cep[j] = ep_a[a][ss];
                        }
                        /* winner among eligible contextual relations of the current map */
                        for (sIdx = 0; sIdx < nadm[a]; sIdx++) {
                            Rel key; long ix; int z; int src = admlist[a][sIdx];
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
                        /* seniority telemetry: a losing eligible relation with fewer observations
                         * than the winner and a larger ledger-per-observation rate */
                        if (best >= 0) {
                            int z3;
                            wuse[armcol]++;
                            for (z3 = 0; z3 < ncand; z3++) {
                                Rel *Lz = &rb[a][cand[z3]], *Wz = &rb[a][best];
                                if (cand[z3] == best) continue;
                                if (Lz->seen > 0 && Wz->seen > 0 && Lz->seen < Wz->seen &&
                                    (Lz->led / (double)Lz->seen) > (Wz->led / (double)Wz->seen))
                                    senio[armcol]++;
                            }
                        }
                        pf = p;
                        if (best >= 0) {
                            int bs = rb[a][best].ts, bd = rb[a][best].td;
                            double pE = pE_buf[bd], r = cargo_p(bs, cs, cl), qq;
                            qq = (fb == bd) ? p * r / pE : p * (1.0 - r) / (1.0 - pE);
                            pf = (1.0 - rb[a][best].L) * p + rb[a][best].L * qq;
                        }
                        bits[armcol] += -log2(pf);
                        /* every current relation is priced in shadow and updated */
                        for (sIdx = 0; sIdx < nadm[a]; sIdx++) {
                            Rel key; long ix; int z, src = admlist[a][sIdx], dd = adm_a[a][src];
                            double pE, r, qq, delta; Rel *R;
                            key.ts = src; key.td = dd; key.tep = ep_a[a][src]; key.cl = cl;
                            for (z = 0; z < 3; z++) { key.cs[z] = cs[z]; key.cd[z] = cd[z]; key.cep[z] = cep[z]; }
                            ix = rel_find(a, &key);
                            R = &rb[a][ix];
                            pE = pE_buf[dd]; r = cargo_p(src, cs, cl);
                            qq = (fb == dd) ? p * r / pE : p * (1.0 - r) / (1.0 - pE);
                            delta = log2(qq / p);
                            R->seen++; if (delta > 0.0) R->pos++; else if (delta < 0.0) R->neg++;
                            R->led += delta;
                            if (R->led > R->peak) R->peak = R->led;
                            if (R->cl >= 1) {
                                if (R->st == 1) { double nl = R->L * exp(0.05 * delta); if (nl < 0.01) nl = 0.01; if (nl > 0.5) nl = 0.5; R->L = nl; }
                                if (R->st == 0 && R->led >= EARN_BITS) { R->st = 1; R->L = 0.05; R->ever = 1; }
                                else if (R->st == 1 && R->led < REVOKE_BITS) { R->st = 0; R->L = 0.0; }
                            }
                        }
                    }
                    ro += unit_len[u];
                }
                for (a = 0; a < 22; a++) {
                    if (abits[c2][a] < 0) continue;
                    myGF[wx2][a] += bits[0] - bits[a];
                    aGF2[wx2][a] += abits[c2][0] - abits[c2][a];
                    if (c2 < NHOR) { myGN2[wx2][a] += bits[0] - bits[a]; aGN2[wx2][a] += abits[c2][0] - abits[c2][a]; }
                    if (!a5_main(bits[a], abits[c2][a])) {
                        badbits[a]++;
                        if (firstbad2[a] < 0) { firstbad2[a] = c2; mybits_first[a] = bits[a]; }
                    }
                    if (asen[c2][a] >= 0 && senio[a] != asen[c2][a]) {
                        badsen++;
                        if (firstsen < 0) { firstsen = c2; sen_arm = a; sen_mine = senio[a]; }
                    }
                    if (awu[c2][a] >= 0 && wuse[a] != awu[c2][a]) {
                        badwu++;
                        if (firstwu < 0) { firstwu = c2; wu_arm = a; wu_mine = wuse[a]; }
                    }
                }
            }
            {
                long tot = 0; int shown = 0;
                for (a = 0; a < 22; a++) tot += badbits[a];
                PF("C29_ARMS", tot == 0,
                   "evidence_%s.tsv: all %d present arms' per-chunk bits reproduced from my own prices over %d chunks (%ld divergent arm-chunks)",
                   ew8[wx2], 22, nb2, tot);
                for (a = 0; a < 22 && shown < 3; a++) if (firstbad2[a] >= 0) {
                    Fl("C29_ARMS", "evidence_%s.tsv arm=%s first divergent chunk=%ld : artifact bits=%.17g, reconstruction bits=%.17g",
                       ew8[wx2], a == 0 ? "cold" : mapname_of(a), firstbad2[a], abits[firstbad2[a]][a], mybits_first[a]);
                    shown++;
                }
            }
            PF("C32_SENIORITY", badsen == 0,
               "evidence_%s.tsv seniority_suppressions reproduced from my own candidate set (%ld divergent arm-chunks)",
               ew8[wx2], badsen);
            if (firstsen >= 0)
                Fl("C32_SENIORITY", "evidence_%s.tsv arm=%s first divergent chunk=%ld : artifact=%ld, reconstruction=%ld",
                   ew8[wx2], sen_arm == 0 ? "cold" : mapname_of(sen_arm), firstsen, asen[firstsen][sen_arm], sen_mine);
            PF("C32_SENIORITY", badwu == 0,
               "evidence_%s.tsv winner_uses reproduced from my own winner selection (%ld divergent arm-chunks)",
               ew8[wx2], badwu);
            if (firstwu >= 0)
                Fl("C32_SENIORITY", "evidence_%s.tsv arm=%s first divergent chunk=%ld : artifact winner_uses=%ld, reconstruction=%ld",
                   ew8[wx2], wu_arm == 0 ? "cold" : mapname_of(wu_arm), firstwu, awu[firstwu][wu_arm], wu_mine);
            PF("C31_PENORM", pE_bad == 0,
               "%s : the specialist normalisation sum_d p_E(d) = 1 holds at every priced position (%ld violations)",
               ew8[wx2], pE_bad);
            /* relation book of the live arm vs relations_<world>.tsv, key by key */
            {
                Tsv rl; long mine_n = 0, ever_n = 0, art_n = 0, art_ever = 0;
                long unmatched = 0, badseen = 0, badled = 0; int shown = 0;
                sprintf(path, ART "relations_%s.tsv", ew8[wx2]); rl = tsv_load(path);
                for (i = 0; i < rb_n[0]; i++) { mine_n++; if (rb[0][i].cl >= 1 && rb[0][i].ever) ever_n++; }
                for (i = 1; i < rl.n; i++) if (!strcmp(fs(&rl, i, 0), "relation")) {
                    art_n++;
                    if (fi(&rl, i, 4) >= 1 && fi(&rl, i, 21) == 1) art_ever++;
                }
                PF("C30_BOOKS", mine_n == art_n && ever_n == art_ever,
                   "relations_%s.tsv relation arm: %ld keys / %ld contextual ever-earned in the artifact vs %ld / %ld reconstructed",
                   ew8[wx2], art_n, art_ever, mine_n, ever_n);
                /* field-by-field: does the update cadence match while the magnitude does not? */
                for (i = 1; i < rl.n; i++) {
                    Rel k; long m; int z, found = -1;
                    if (strcmp(fs(&rl, i, 0), "relation")) continue;
                    k.ts = (int)fi(&rl, i, 1); k.td = (int)fi(&rl, i, 2);
                    k.tep = (int)fi(&rl, i, 3); k.cl = (int)fi(&rl, i, 4);
                    for (z = 0; z < 3; z++) {
                        k.cs[z] = (int)fi(&rl, i, 5 + 3*z);
                        k.cd[z] = (int)fi(&rl, i, 6 + 3*z);
                        k.cep[z] = (int)fi(&rl, i, 7 + 3*z);
                    }
                    for (m = 0; m < rb_n[0]; m++) if (rel_same(&rb[0][m], &k)) { found = (int)m; break; }
                    if (found < 0) { unmatched++; continue; }
                    if (rb[0][found].seen != fi(&rl, i, 14) || rb[0][found].pos != fi(&rl, i, 15) ||
                        rb[0][found].neg != fi(&rl, i, 16)) {
                        badseen++;
                        if (shown < 2) { Fl("C30_BOOKS", "relations_%s.tsv key (%d->%d ep%d cl%d): artifact seen/pos/neg = %ld/%ld/%ld ; reconstruction = %ld/%ld/%ld",
                            ew8[wx2], k.ts, k.td, k.tep, k.cl, fi(&rl, i, 14), fi(&rl, i, 15), fi(&rl, i, 16),
                            rb[0][found].seen, rb[0][found].pos, rb[0][found].neg); shown++; }
                    }
                    if (!a5_main(rb[0][found].led, fd(&rl, i, 17))) badled++;
                }
                PF("C30_BOOKS", unmatched == 0, "relations_%s.tsv: every artifact key is reproduced by the reconstruction (%ld unmatched)", ew8[wx2], unmatched);
                PF("C30_BOOKS", badseen == 0, "relations_%s.tsv: seen/positive/negative receipt counts agree key by key (%ld divergent keys)", ew8[wx2], badseen);
                PF("C30_BOOKS", badled == 0, "relations_%s.tsv: ledger_bits agrees key by key (%ld divergent keys)", ew8[wx2], badled);
                tsv_free(&rl);
            }
            tsv_free(&ev); free(W);
            fflush(stdout);
        }
        {   /* the downstream fixture's source byte is now derivable from the cargo model */
            Tsv su = tsv_load(ART "downstream_fixture_summary.tsv");
            int bestS = -1, ctx3[3]; double bv = 0;
            for (i = 0; i < 256; i++) if (cg_c0[i] > 0) {
                double v;
                ctx3[0] = (int)i; ctx3[1] = (int)i; ctx3[2] = (int)i;
                v = cargo_p((int)i, ctx3, 3);
                if (bestS < 0 || v > bv || (v == bv && firstA[i] < firstA[bestS])) { bestS = (int)i; bv = v; }
            }
            PF("C18_DOWNFIX", bestS == (int)fi(&su, 1, 2),
               "fixture source byte: argmax_s P_start(s|s,s,s) over A-train unit starts = %d (P=%.17g) ; artifact declares %ld",
               bestS, bv, fi(&su, 1, 2));
            tsv_free(&su);
        }
    }

    {   /* ==== C34: A5 Boolean-gate clause ====
         * Independently reconstructed and artifact-derived values must produce the same
         * Boolean for every gate; a numeric tolerance can never excuse a different result. */
        static const char *ew8[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        double M = MARGIN_M;
        long bad = 0; int wx3, a3, shown = 0;
        for (wx3 = 0; wx3 < 8; wx3++) for (a3 = 0; a3 < 22; a3++) {
            int g1 = (myGN2[wx3][a3] >= M), g2 = (aGN2[wx3][a3] >= M);
            int h1 = (fabs(myGN2[wx3][a3]) <= GHOST_LINE), h2 = (fabs(aGN2[wx3][a3]) <= GHOST_LINE);
            int i1 = (myGN2[wx3][a3] <= -M), i2 = (aGN2[wx3][a3] <= -M);
            int r1 = (myGF[wx3][a3] >= myGF[wx3][1]), r2 = (aGF2[wx3][a3] >= aGF2[wx3][1]);
            if (g1 != g2 || h1 != h2 || i1 != i2 || r1 != r2) {
                bad++;
                if (shown < 3) { Fl("C34_GATEAGREE", "%s arm=%s: margin %d/%d ghostline %d/%d interference %d/%d rank %d/%d (mine G_N=%.6f G_FULL=%.6f ; artifact G_N=%.6f G_FULL=%.6f)",
                    ew8[wx3], a3 == 0 ? "cold" : mapname_of(a3), g1, g2, h1, h2, i1, i2, r1, r2,
                    myGN2[wx3][a3], myGF[wx3][a3], aGN2[wx3][a3], aGF2[wx3][a3]); shown++; }
            }
        }
        PF("C34_GATEAGREE", bad == 0,
           "margin, ghost-line, interference and rank gates agree between reconstruction and artifact for all 8 worlds x 22 arms (%ld disagreements)", bad);
    }
    {   /* ==== C33: A5 %.17g round-trip on every serialized artifact value ==== */
        static const char *ew8[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        struct { const char *pat; int col[4]; int nc; } spec[5] = {
            { ART "evidence_%s.tsv",        { 5, -1, -1, -1 }, 1 },
            { ART "relations_%s.tsv",       { 17, 18, 20, -1 }, 3 },
            { ART "winners_%s.tsv",         { 3, 4, -1, -1 }, 2 },
            { ART "relation_events_%s.tsv", { 4, -1, -1, -1 }, 1 },
            { ART "map_summary_%s.tsv",     { -1, -1, -1, -1 }, 0 }
        };
        long checked = 0, bad = 0; int wx3, si, shown = 0;
        char path[512];
        for (wx3 = 0; wx3 < 8; wx3++) for (si = 0; si < 5; si++) {
            Tsv t;
            if (spec[si].nc == 0) continue;
            sprintf(path, spec[si].pat, ew8[wx3]); t = tsv_load(path);
            for (i = 1; i < t.n; i++) for (j = 0; j < spec[si].nc; j++) {
                int cc = spec[si].col[j];
                if (cc < 0 || cc >= t.r[i].nf) continue;
                checked++;
                if (!a5_roundtrip(fs(&t, i, cc))) {
                    bad++;
                    if (shown < 3) { Fl("C33_ROUNDTRIP", "%s line %d col %d value %s does not round-trip through %%.17g",
                                        path, i + 1, cc, fs(&t, i, cc)); shown++; }
                }
            }
            tsv_free(&t);
        }
        {   /* the fixture files too */
            const char *fx[2] = { ART "downstream_fixture.tsv", ART "surface_shadows_swap_ea.tsv" };
            int fc[2][4] = { { 3, 4, 5, 7 }, { 17, 18, 20, -1 } };
            int fn[2] = { 4, 3 }, z;
            for (z = 0; z < 2; z++) {
                Tsv t = tsv_load(fx[z]);
                for (i = 1; i < t.n; i++) for (j = 0; j < fn[z]; j++) {
                    int cc = fc[z][j];
                    if (cc < 0 || cc >= t.r[i].nf) continue;
                    checked++;
                    if (!a5_roundtrip(fs(&t, i, cc))) {
                        bad++;
                        if (shown < 3) { Fl("C33_ROUNDTRIP", "%s line %d col %d value %s does not round-trip", fx[z], i + 1, cc, fs(&t, i, cc)); shown++; }
                    }
                }
                tsv_free(&t);
            }
        }
        PF("C33_ROUNDTRIP", bad == 0, "%ld serialized artifact values round-trip through %%.17g to their binary64 originals (%ld failures)", checked, bad);
        PF("C33_ROUNDTRIP", a5_nonfinite == 0, "A5 finite-guard: no NaN or infinity encountered in any graded comparison (%ld non-finite)", a5_nonfinite);
    }

    /* --------------------------- evidence laws, cross-refs, and verdict */
    {
        static const char *ew[8] = { "iso","plain","ghost","half","ff","swap_ea","aux1","aux2" };
        static double GFULL[8][22], GN[8][22];
        int has_earn[8], wx;
        double h1 = 0, h2 = 0, oh1 = 0, oh2 = 0;

        for (wx = 0; wx < 8; wx++) {
            char path[512];
            Tsv ev, wn, re, rl, ms;
            int nch = 0;
            sprintf(path, ART "evidence_%s.tsv", ew[wx]);        ev = tsv_load(path);
            sprintf(path, ART "winners_%s.tsv", ew[wx]);         wn = tsv_load(path);
            sprintf(path, ART "relation_events_%s.tsv", ew[wx]); re = tsv_load(path);
            sprintf(path, ART "relations_%s.tsv", ew[wx]);       rl = tsv_load(path);
            sprintf(path, ART "map_summary_%s.tsv", ew[wx]);     ms = tsv_load(path);

            for (i = 0; i < 300; i++) { ev_byt[i] = -1; for (j = 0; j < 22; j++) { ev_bits[i][j] = 0; ev_pos[i][j] = -1; } }
            for (i = 1; i < ev.n; i++) {
                int ch = (int)fi(&ev, i, 0), a = armid_of(fs(&ev, i, 3));
                if (a < 0 || ch < 0 || ch >= 300) continue;
                if (ch + 1 > nch) nch = ch + 1;
                ev_byt[ch] = fi(&ev, i, 2);
                ev_pos[ch][a] = fi(&ev, i, 4); ev_bits[ch][a] = fd(&ev, i, 5);
                ev_pairs[ch][a] = fi(&ev, i, 6); ev_rels[ch][a] = fi(&ev, i, 7);
                ev_cearn[ch][a] = fi(&ev, i, 8); ev_ee[ch][a] = fi(&ev, i, 9);
                ev_re[ch][a] = fi(&ev, i, 10); ev_wu[ch][a] = fi(&ev, i, 11);
                ev_sa[ch][a] = fi(&ev, i, 13);
            }
            { long tot = 0, badb = 0;
              for (c = 0; c < nch; c++) { if (ev_byt[c] != CHUNK) badb++; tot += ev_byt[c]; }
              PF("C19_SEAMRUN", badb == 0, "evidence_%s.tsv: every chunk consumes exactly %d raw bytes (%ld violations)", ew[wx], CHUNK, badb);
              PF("C19_SEAMRUN", tot == (long)nch * CHUNK, "evidence_%s.tsv: whole run consumes %ld raw bytes over %d chunks", ew[wx], tot, nch); }
            { long badp = 0;
              for (c = 0; c < nch; c++) for (j = 1; j < 22; j++) if (ev_pos[c][j] >= 0 && ev_pos[c][j] != ev_pos[c][0]) badp++;
              PF("C15_EVIDENCE", badp == 0, "evidence_%s.tsv: priced-unit count identical across all 22 arms at every chunk (%ld violations)", ew[wx], badp); }
            { long badq = 0; int shown = 0;
              for (c = 0; c < nch; c++) for (j = 1; j < 22; j++)
                  if (ev_pos[c][j] >= 0 && ev_wu[c][j] == 0 && ev_bits[c][j] != ev_bits[c][0]) {
                      badq++;
                      if (shown < 2) { Fl("C15_EVIDENCE", "evidence_%s.tsv chunk=%d arm=%s: winner_uses=0 but bits %.17g != cold %.17g",
                          ew[wx], c, mapname_of(j), ev_bits[c][j], ev_bits[c][0]); shown++; }
                  }
              PF("C15_EVIDENCE", badq == 0, "evidence_%s.tsv: every arm with no live voice in a chunk prices bit-identically to cold (%ld violations)", ew[wx], badq); }
            { long badw = 0;
              memset(wcnt_, 0, sizeof wcnt_);
              for (i = 1; i < wn.n; i++) {
                  int ch = (int)(fi(&wn, i, 0) / CHUNK), a = armid_of(fs(&wn, i, 2));
                  if (a >= 0 && ch >= 0 && ch < 300) wcnt_[ch][a]++;
              }
              for (c = 0; c < nch; c++) for (j = 1; j < 22; j++) if (ev_pos[c][j] >= 0 && wcnt_[c][j] != ev_wu[c][j]) badw++;
              PF("C16_XREF", badw == 0, "evidence_%s.tsv winner_uses equals the row count of winners_%s.tsv per chunk and arm (%ld divergences over %d rows)", ew[wx], ew[wx], badw, wn.n - 1); }
            { long bade = 0;
              memset(ecnt_, 0, sizeof ecnt_); memset(rcnt_, 0, sizeof rcnt_);
              for (i = 1; i < re.n; i++) {
                  int ch = (int)(fi(&re, i, 0) / CHUNK), a = armid_of(fs(&re, i, 2));
                  if (a < 0 || ch < 0 || ch >= 300) continue;
                  if (!strcmp(fs(&re, i, 3), "earn")) ecnt_[ch][a]++; else rcnt_[ch][a]++;
              }
              for (c = 0; c < nch; c++) for (j = 1; j < 22; j++)
                  if (ev_pos[c][j] >= 0 && (ecnt_[c][j] != ev_ee[c][j] || rcnt_[c][j] != ev_re[c][j])) bade++;
              PF("C16_XREF", bade == 0, "evidence_%s.tsv EARN/REVOKE counters equal relation_events_%s.tsv rows per chunk and arm (%ld divergences over %d rows)", ew[wx], ew[wx], bade, re.n - 1); }
            { long rc[22], ec2[22], badr = 0;
              for (j = 0; j < 22; j++) { rc[j] = 0; ec2[j] = 0; }
              for (i = 1; i < rl.n; i++) {
                  int a = armid_of(fs(&rl, i, 0));
                  if (a < 0) continue;
                  rc[a]++;
                  if (fi(&rl, i, 4) >= 1 && fi(&rl, i, 21) == 1) ec2[a]++;
              }
              for (j = 1; j < 22; j++) if (ev_pos[nch-1][j] >= 0)
                  if (ev_rels[nch-1][j] != rc[j] || ev_cearn[nch-1][j] != ec2[j]) {
                      badr++;
                      Fl("C16_XREF", "evidence_%s.tsv last chunk arm=%s: relations=%ld ever_earned=%ld ; relations_%s.tsv holds %ld rows and %ld contextual ever-earned",
                         ew[wx], mapname_of(j), ev_rels[nch-1][j], ev_cearn[nch-1][j], ew[wx], rc[j], ec2[j]);
                  }
              PF("C16_XREF", badr == 0, "evidence_%s.tsv final relation-book counters equal relations_%s.tsv contents for every arm", ew[wx], ew[wx]); }
            { long badp = 0; int shown = 0;
              for (c = 0; c < nch; c++) {
                  long ap = -1, sb = -1;
                  for (i = 1; i < ms.n; i++) if (fi(&ms, i, 0) == c) { ap = fi(&ms, i, 4); sb = fi(&ms, i, 6); break; }
                  if (c == 0) { if (ev_pairs[0][1] != 0) badp++; continue; }
                  if (ap < 0) continue;
                  if (ev_pairs[c][1] != ap || ev_sa[c][1] != sb) {
                      badp++;
                      if (shown < 2) { Fl("C16_XREF", "evidence_%s.tsv chunk=%d relation pairs=%ld surface=%ld vs map_summary admitted=%ld surface=%ld",
                          ew[wx], c, ev_pairs[c][1], ev_sa[c][1], ap, sb); shown++; }
                  }
              }
              PF("C16_XREF", badp == 0, "evidence_%s.tsv relation pair count and surface bit equal map_summary_%s.tsv at every chunk (%ld divergences)", ew[wx], ew[wx], badp); }
            { long b1 = 0, b2 = 0, b3 = 0, b4 = 0, b5 = 0;
              for (i = 1; i < rl.n; i++) {
                  long cl = fi(&rl, i, 4), sn = fi(&rl, i, 14), po = fi(&rl, i, 15), ng = fi(&rl, i, 16);
                  double pk = fd(&rl, i, 18), L = fd(&rl, i, 20);
                  long st = fi(&rl, i, 19), ev2 = fi(&rl, i, 21);
                  if (sn < po + ng) b1++;   /* A2: delta==0 increments seen only */
                  if (cl < 0 || cl > 3) b2++;
                  for (j = (int)cl; j < 3; j++)
                      if (fi(&rl, i, 5 + 3*j) != 0 || fi(&rl, i, 6 + 3*j) != 0 || fi(&rl, i, 7 + 3*j) != 0) b3++;
                  if (cl == 0 && (st != 0 || ev2 != 0 || L != 0.0)) b4++;
                  if (st == 1 && !(L >= 0.01 - 1e-15 && L <= 0.5 + 1e-15)) b5++;
                  if (st == 0 && L != 0.0) b5++;
                  if (ev2 == 1 && pk < EARN_BITS - 1e-9) b5++;
              }
              PF("C17_RELBOOK", b1 == 0, "relations_%s.tsv: seen >= positive + negative for all %d rows, A2 sign law (%ld violations)", ew[wx], rl.n - 1, b1);
              PF("C17_RELBOOK", b2 == 0 && b3 == 0, "relations_%s.tsv: context_len in 0..3 and every unused context field zero (%ld/%ld violations)", ew[wx], b2, b3);
              PF("C17_RELBOOK", b4 == 0, "relations_%s.tsv: every context_len=0 key keeps state=0, ever_earned=0, L=0 (%ld violations)", ew[wx], b4);
              PF("C17_RELBOOK", b5 == 0, "relations_%s.tsv: authority-state invariants hold (L in [0.01,0.5] when eligible, L=0 in shadow, peak>=EARN when ever-earned) (%ld violations)", ew[wx], b5); }
            { long b1 = 0, b2 = 0;
              for (i = 1; i < re.n; i++) {
                  double la = fd(&re, i, 4); long cl = fi(&re, i, 8);
                  if (cl < 1) b1++;
                  if (!strcmp(fs(&re, i, 3), "earn")) { if (la < EARN_BITS) b2++; }
                  else if (la >= REVOKE_BITS) b2++;
              }
              PF("C17_RELBOOK", b1 == 0, "relation_events_%s.tsv: every EARN/REVOKE carries context_len>=1 (%ld violations)", ew[wx], b1);
              PF("C17_RELBOOK", b2 == 0, "relation_events_%s.tsv: EARN ledger>=32 and REVOKE ledger<16 (%ld violations)", ew[wx], b2); }
            { long b1 = 0, b2 = 0, b3 = 0, lastpos[22]; int shown = 0;
              for (j = 0; j < 22; j++) lastpos[j] = -1;
              for (i = 1; i < wn.n; i++) {
                  long cl = fi(&wn, i, 8), up = fi(&wn, i, 1);
                  double lb = fd(&wn, i, 3), Lb = fd(&wn, i, 4);
                  int a = armid_of(fs(&wn, i, 2));
                  if (cl < 1) b1++;
                  if (lb < REVOKE_BITS || Lb < 0.01 - 1e-15 || Lb > 0.5 + 1e-15) {
                      b2++;
                      if (shown < 2) { Fl("C17_RELBOOK", "winners_%s.tsv line %d: ledger_before=%.17g L_before=%.17g outside the frozen eligibility band", ew[wx], i + 1, lb, Lb); shown++; }
                  }
                  if (a >= 0) { if (up == lastpos[a]) b3++; lastpos[a] = up; }
              }
              PF("C17_RELBOOK", b1 == 0, "winners_%s.tsv: every live voice has context_len>=1 (%ld violations)", ew[wx], b1);
              PF("C17_RELBOOK", b2 == 0, "winners_%s.tsv: ledger_before>=16 and L_before in [0.01,0.5] (%ld violations)", ew[wx], b2);
              PF("C17_RELBOOK", b3 == 0, "winners_%s.tsv: at most one live voice per arm per unit position (%ld violations)", ew[wx], b3); }

            for (j = 0; j < 22; j++) { GFULL[wx][j] = 0; GN[wx][j] = 0; }
            for (c = 0; c < nch; c++) for (j = 0; j < 22; j++) if (ev_pos[c][j] >= 0) {
                GFULL[wx][j] += ev_bits[c][0] - ev_bits[c][j];
                if (c < NHOR) GN[wx][j] += ev_bits[c][0] - ev_bits[c][j];
            }
            has_earn[wx] = 0;
            for (i = 1; i < re.n; i++)
                if (!strcmp(fs(&re, i, 2), "relation") && !strcmp(fs(&re, i, 3), "earn") && fi(&re, i, 8) >= 1) has_earn[wx] = 1;
            if (!strcmp(ew[wx], "half")) {
                for (c = 0; c < NHOR; c++) { h1 += ev_bits[c][0] - ev_bits[c][1]; oh1 += ev_bits[c][0] - ev_bits[c][21]; }
                for (c = 64; c < 64 + NHOR; c++) { h2 += ev_bits[c][0] - ev_bits[c][1]; oh2 += ev_bits[c][0] - ev_bits[c][21]; }
            }
            tsv_free(&ev); tsv_free(&wn); tsv_free(&re); tsv_free(&rl); tsv_free(&ms);
            fflush(stdout);
        }

        {
            const int IW = 0, PW = 1, GW = 2, FW = 4;
            double prank[8], M = MARGIN_M;
            int nullfloor[8], micro[8], surface_admitted = 0;
            int organ, macro, oracle_pass, blocking = 0, surface_leak, null_beat = 1, ob = 0;
            const char *verdict;

            for (wx = 0; wx < 8; wx++) {
                double mx = GFULL[wx][2];
                int ge = 0;
                for (k = 3; k <= 20; k++) if (GFULL[wx][k] > mx) mx = GFULL[wx][k];
                for (k = 2; k <= 20; k++) if (GFULL[wx][k] >= GFULL[wx][1]) ge++;
                prank[wx] = (1.0 + ge) / 20.0;
                nullfloor[wx] = (prank[wx] == 0.05);
                micro[wx] = has_earn[wx] && nullfloor[wx];
                Ob("C20_VERDICT", "%-8s G_FULL relation=%.4f oracle=%.4f max_null=%.4f | nulls>=live=%d p_rank=%.4f NULL_FLOOR=%d EARN=%d MICRO=%d",
                   ew[wx], GFULL[wx][1], GFULL[wx][21], mx, ge, prank[wx], nullfloor[wx], has_earn[wx], micro[wx]);
            }
            {
                Tsv ms = tsv_load(ART "map_summary_swap_ea.tsv");
                for (i = 1; i < ms.n; i++) if (fi(&ms, i, 6)) surface_admitted = 1;
                tsv_free(&ms);
            }
            organ = micro[IW] && micro[PW] && !micro[GW] && !surface_admitted;
            Ob("C20_VERDICT", "ORGAN = MICRO(iso)=%d AND MICRO(plain)=%d AND NOT MICRO(ghost)=%d AND NOT SURFACE_ADMITTED=%d -> %d",
               micro[IW], micro[PW], micro[GW], surface_admitted, organ);
            for (wx = 0; wx < 8; wx++) if (GN[wx][1] <= -M) blocking = 1;
            surface_leak = (GN[FW][1] <= -M);
            for (k = 2; k <= 20; k++) {
                if (GN[IW][1] - GN[IW][k] < M) null_beat = 0;
                if (GN[PW][1] - GN[PW][k] < M) null_beat = 0;
            }
            macro = (GN[IW][1] >= M) && (GN[PW][1] >= M) && null_beat &&
                    (fabs(GN[GW][1]) <= GHOST_LINE) && (h1 >= M) && (fabs(h2) < M) &&
                    (GN[FW][1] > -M) && !blocking;
            Ob("C20_VERDICT", "deciding horizon N=%d M=%.2f: G_relation iso=%.4f plain=%.4f ghost=%.4f ff=%.4f half1=%.4f half2=%.4f",
               DECIDE_N, M, GN[IW][1], GN[PW][1], GN[GW][1], GN[FW][1], h1, h2);
            Ob("C20_VERDICT", "MACRO_PASS=%d (beats every one of the 19 nulls by M on iso and plain: %d ; blocking label: %d ; surface leak: %d)",
               macro, null_beat, blocking, surface_leak);
            GN[GW][21] = 0.0;   /* A4: absent ghost oracle, formula convention only */
            for (wx = 0; wx < 8; wx++) if (GN[wx][21] <= -M) ob = 1;
            oracle_pass = (GN[IW][21] >= M) && (GN[PW][21] >= M) && (fabs(GN[GW][21]) <= GHOST_LINE) &&
                          (oh1 >= M) && (fabs(oh2) < M) && (GN[FW][21] > -M) && !ob;
            Ob("C20_VERDICT", "ORACLE_PASS=%d (G_oracle iso=%.4f plain=%.4f ghost=%.4f ff=%.4f half1=%.4f half2=%.4f blocking=%d)",
               oracle_pass, GN[IW][21], GN[PW][21], GN[GW][21], GN[FW][21], oh1, oh2, ob);

            if (surface_admitted) verdict = "organ failed: surface shadow admitted";
            else if (organ && macro) verdict = "learned transfer earned (development) - final claim still requires untouched confirmatory evidence";
            else if (organ && !macro) verdict = "microscopic relation earned, transfer-at-scale not reached";
            else if (!organ && oracle_pass && !micro[GW])   /* A4: guard, no ghost oracle exists */
                verdict = "learned correspondence remains the bottleneck conditional on live-admitted source rows";
            else verdict = "transfer not detected in this form";
            printf("VERDICT\tC20_VERDICT\tDEVELOPMENT\tprimary=%s\n", verdict);
            printf("VERDICT\tC20_VERDICT\tDEVELOPMENT\tlabels: interference=%d surface_leak=%d surface_admitted=%d\n",
                   blocking, surface_leak, surface_admitted);
        }
        Ob("C20_VERDICT", "half1/half2 CLOSED in pass 2: court-2 amendment A9 fixes the boundary inside the run and states "
                          "\"the half gates measure G over [0,16384) (half 1) and over the first 16384 bytes after the "
                          "boundary (half 2)\". Boundary = 65536, so half2 = [65536,81920) - exactly the reading this hand "
                          "chose blind in pass 1.");
    }


    /* --------------------------------------------------- standing gaps */
    Ob("G01_LOCALMODEL", "CLOSED in pass 2: COURT2_SNAPSHOT.md supplied the inherited local model (R5 total-order BPE, "
                         "merges 2048, MIN_PAIR 4, R2 length-aware newborn floor, epsilon 0.1 per level). The learner and "
                         "the unit chain are now reconstructed and verified - see C27_UNITS and C28_PRICE.");
    Ob("G02_COURT2", "CLOSED in pass 2: the court-3 mandate names COURT3_SNAPSHOT.md + COURT2_SNAPSHOT.md, and the "
                     "court-2 snapshot was supplied and hash-verified (c5a60f66..., 14218 bytes) before being read.");
    Ob("G03_SENIORITY", "CLOSED in pass 3: the seniority-suppression counter is recomputed from the reconstructed "
                        "candidate set and graded against the artifacts - see C32_SENIORITY.");
    Ob("G04_FPTOL", "CLOSED by addendum A5: the comparison-tolerance law is now stated in full (finite-guard, exact "
                    "structural/integer fields, fixture 1e-12/1e-12/1e-9, main-court mixed relative 1e-6 with "
                    "max(1,|x|,|y|), structural-zero L exactly zero, %%.17g round-trip, Boolean-gate agreement) and this "
                    "verifier implements exactly those - see C33_ROUNDTRIP and C34_GATEAGREE.");
    Ob("G05_ATRAIN", "CLOSED by addendum A1: A_train_bytes = floor((9*|A|)/10) in integer arithmetic = 402790.");

    printf("SUMMARY\ttotal_checks=%ld\tpass=%ld\tfail=%ld\tgap=%ld\tobs=%ld\n",
           n_pass + n_fail, n_pass, n_fail, n_gap, n_obs);
    free(A); free(D); free(w_plain); free(w_iso); free(w_swap); free(w_ff);
    return n_fail ? 1 : 0;
}
