#define _POSIX_C_SOURCE 200809L

/* adaptation_check.c -- independent plain-C reader for body A1.

   It shares no parsing or hash code with adaptation_witness.cpp.  It reads
   the two-record witness, opens the content-addressed biography blob, checks
   all SHA-256 and FNV seals, reparses both Court-4 books, and independently
   derives the candidate/control and EARN/REVOKE census.  It never writes.

   usage: adaptation_check [witness-ledger]                              */

#include <errno.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LIMIT (8u * 1024u * 1024u)
#define MAX_ROWS 1024
#define MAX_FIELDS 32
#define LAW "body-a1-court4-witness-v1"
#ifndef C8_SHA
#define C8_SHA                                                                 \
  "b528c3e2efe1a2dd38b892e18f67e2db98338b61e617c1b581f119686fc76b1e"
#endif
#ifndef MANIFEST_SHA
#define MANIFEST_SHA                                                           \
  "a8208e14bf20f8b1e573394abf5fd63142dc7a1227e7a55420c62bb7e059b647"
#endif
#ifndef A1_EVENTS_SHA
#define A1_EVENTS_SHA                                                          \
  "e8ac386fb11c6ba6199edea505ff0ad8c0b4c08284366823ea92a6b64f314b4c"
#endif
#ifndef A1_RELATIONS_SHA
#define A1_RELATIONS_SHA                                                       \
  "d3e5e514ea4cccc4a044ad224d19170e637beb79a810fb0875816f798211b438"
#endif

_Noreturn static void fail(const char *s) {
  fprintf(stderr, "adaptation_check: %s\n", s);
  exit(1);
}

static uint64_t fold(const unsigned char *p, size_t n, uint64_t h) {
  size_t i;
  for (i = 0; i < n; i++) {
    h ^= p[i];
    h *= 0x100000001b3ULL;
  }
  return h;
}

typedef struct {
  uint32_t h[8];
  uint64_t bits;
  unsigned char b[64];
  size_t n;
} Sha;
static uint32_t rr(uint32_t x, unsigned n) {
  return (x >> n) | (x << (32 - n));
}
static void sha_block(Sha *s, const unsigned char *p) {
  static const uint32_t k[64] = {
      0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu,
      0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u, 0xd807aa98u, 0x12835b01u,
      0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u,
      0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu,
      0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u,
      0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u,
      0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
      0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
      0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u,
      0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u, 0x1e376c08u,
      0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu,
      0x682e6ff3u, 0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u,
      0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};
  uint32_t w[64], a, b, c, d, e, f, g, z, t1, t2, s0, s1, ch, maj;
  int i;
  for (i = 0; i < 16; i++)
    w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 |
           (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
  for (i = 16; i < 64; i++) {
    s0 = rr(w[i - 15], 7) ^ rr(w[i - 15], 18) ^ (w[i - 15] >> 3);
    s1 = rr(w[i - 2], 17) ^ rr(w[i - 2], 19) ^ (w[i - 2] >> 10);
    w[i] = w[i - 16] + s0 + w[i - 7] + s1;
  }
  a = s->h[0];
  b = s->h[1];
  c = s->h[2];
  d = s->h[3];
  e = s->h[4];
  f = s->h[5];
  g = s->h[6];
  z = s->h[7];
  for (i = 0; i < 64; i++) {
    s1 = rr(e, 6) ^ rr(e, 11) ^ rr(e, 25);
    ch = (e & f) ^ (~e & g);
    t1 = z + s1 + ch + k[i] + w[i];
    s0 = rr(a, 2) ^ rr(a, 13) ^ rr(a, 22);
    maj = (a & b) ^ (a & c) ^ (b & c);
    t2 = s0 + maj;
    z = g;
    g = f;
    f = e;
    e = d + t1;
    d = c;
    c = b;
    b = a;
    a = t1 + t2;
  }
  s->h[0] += a;
  s->h[1] += b;
  s->h[2] += c;
  s->h[3] += d;
  s->h[4] += e;
  s->h[5] += f;
  s->h[6] += g;
  s->h[7] += z;
}
static void sha_init(Sha *s) {
  static const uint32_t q[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u,
                                0xa54ff53au, 0x510e527fu, 0x9b05688cu,
                                0x1f83d9abu, 0x5be0cd19u};
  memcpy(s->h, q, sizeof q);
  s->bits = 0;
  s->n = 0;
}
static void sha_add(Sha *s, const void *vp, size_t n) {
  const unsigned char *p = vp;
  s->bits += (uint64_t)n * 8;
  while (n) {
    size_t q = 64 - s->n;
    if (q > n)
      q = n;
    memcpy(s->b + s->n, p, q);
    s->n += q;
    p += q;
    n -= q;
    if (s->n == 64) {
      sha_block(s, s->b);
      s->n = 0;
    }
  }
}
static void sha_hex(const void *p, size_t n, char out[65]) {
  static const char x[] = "0123456789abcdef";
  Sha s;
  uint64_t bits;
  unsigned char one = 0x80, zero = 0, tail[8];
  int i, j;
  sha_init(&s);
  sha_add(&s, p, n);
  bits = s.bits;
  sha_add(&s, &one, 1);
  while (s.n != 56)
    sha_add(&s, &zero, 1);
  for (i = 0; i < 8; i++)
    tail[7 - i] = (unsigned char)(bits >> (8 * i));
  sha_add(&s, tail, 8);
  for (i = 0; i < 8; i++)
    for (j = 0; j < 4; j++) {
      unsigned char q = (unsigned char)(s.h[i] >> (24 - 8 * j));
      out[8 * i + 2 * j] = x[q >> 4];
      out[8 * i + 2 * j + 1] = x[q & 15];
    }
  out[64] = 0;
}

static unsigned char *read_reg(const char *path, size_t *n) {
  struct stat st;
  FILE *f;
  unsigned char *p;
  if (lstat(path, &st) || !S_ISREG(st.st_mode) || (uint64_t)st.st_size > LIMIT)
    fail("missing, non-regular, or oversized input");
  f = fopen(path, "rb");
  if (!f)
    fail("cannot open input");
  p = malloc((size_t)st.st_size + 1);
  if (!p)
    fail("memory");
  if (st.st_size && fread(p, 1, (size_t)st.st_size, f) != (size_t)st.st_size) {
    fclose(f);
    free(p);
    fail("short read");
  }
  if (fgetc(f) != EOF || ferror(f)) {
    fclose(f);
    free(p);
    fail("input changed while reading");
  }
  fclose(f);
  p[st.st_size] = 0;
  *n = (size_t)st.st_size;
  return p;
}

static int lowerhex(const char *p, size_t n) {
  size_t i;
  for (i = 0; i < n; i++)
    if (!((p[i] >= '0' && p[i] <= '9') || (p[i] >= 'a' && p[i] <= 'f')))
      return 0;
  return 1;
}
static int pu64(const char *s, uint64_t *v) {
  uint64_t x = 0;
  size_t i, n = strlen(s);
  if (!n || n > 20 || (n > 1 && s[0] == '0'))
    return 0;
  for (i = 0; i < n; i++) {
    unsigned d;
    if (s[i] < '0' || s[i] > '9')
      return 0;
    d = (unsigned)(s[i] - '0');
    if (x > (UINT64_MAX - d) / 10)
      return 0;
    x = x * 10 + d;
  }
  *v = x;
  return 1;
}
static double pdbl(const char *s) {
  char *e;
  double v;
  errno = 0;
  if (!*s || *s == '+' || strchr(s, ' '))
    fail("malformed number");
  v = strtod(s, &e);
  if (errno || *e || !isfinite(v))
    fail("malformed number");
  return v;
}
static int split(char *s, char **f, int max) {
  int n = 0;
  char *p = s;
  for (;;) {
    char *q = strchr(p, '\t');
    if (n < max)
      f[n] = p;
    n++;
    if (!q)
      return n;
    *q = 0;
    p = q + 1;
  }
}
static int arm_ok(const char *s) {
  char *e;
  long n;
  if (!strcmp(s, "relation") || !strcmp(s, "oracle"))
    return 1;
  if (strncmp(s, "null", 4) || !s[4] || (s[4] == '0' && s[5]))
    return 0;
  n = strtol(s + 4, &e, 10);
  return !*e && n >= 0 && n <= 18;
}

typedef struct {
  char arm[16];
  uint64_t k[13];
  int state, used;
} Event;
static Event ev[MAX_ROWS];
static size_t nev;

static void getkey(char **f, int at, uint64_t k[13]) {
  int i, j;
  for (i = 0; i < 13; i++)
    if (!pu64(f[at + i], &k[i]))
      fail("non-canonical RelationKey field");
  if (k[0] > 255 || k[1] > 255 || !k[2] || k[3] > 3)
    fail("invalid RelationKey head");
  for (i = 0; i < 3; i++) {
    j = 4 + 3 * i;
    if ((uint64_t)i < k[3]) {
      if (k[j] > 255 || k[j + 1] > 255 || !k[j + 2])
        fail("invalid context tuple");
    } else if (k[j] || k[j + 1] || k[j + 2])
      fail("unused context tuple is nonzero");
  }
}
static int sameid(const Event *e, const char *arm, const uint64_t k[13]) {
  return !strcmp(e->arm, arm) && !memcmp(e->k, k, 13 * sizeof(uint64_t));
}
static Event *event_for(const char *arm, const uint64_t k[13], int create) {
  size_t i;
  for (i = 0; i < nev; i++)
    if (sameid(&ev[i], arm, k))
      return &ev[i];
  if (!create)
    return NULL;
  if (nev == MAX_ROWS)
    fail("too many event identities");
  memset(&ev[nev], 0, sizeof ev[nev]);
  snprintf(ev[nev].arm, sizeof ev[nev].arm, "%s", arm);
  memcpy(ev[nev].k, k, 13 * sizeof(uint64_t));
  return &ev[nev++];
}

typedef struct {
  uint64_t events, earn, revoke, candidates, live, ever, controls;
} Census;

static void verify_events(const unsigned char *raw, size_t n, Census *c) {
  static const char H[] =
      "byte_offset\tunit_position\tarm\tevent\tledger_after\ttarget_s\ttarget_"
      "d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_"
      "epoch\tc3_s\tc3_d\tc3_epoch";
  char *copy, *p, *nl;
  int row = 0;
  if (!n || raw[n - 1] != '\n' || memchr(raw, '\r', n) || memchr(raw, 0, n))
    fail("events are not cleanly sealed");
  copy = malloc(n + 1);
  if (!copy)
    fail("memory");
  memcpy(copy, raw, n);
  copy[n] = 0;
  p = copy;
  while (*p) {
    char *f[MAX_FIELDS];
    int nf;
    uint64_t k[13], q;
    double bal;
    Event *e;
    nl = strchr(p, '\n');
    *nl = 0;
    row++;
    if (row == 1) {
      if (strcmp(p, H))
        fail("event header drifted");
      p = nl + 1;
      continue;
    }
    nf = split(p, f, MAX_FIELDS);
    if (nf != 18 || !arm_ok(f[2]) ||
        (strcmp(f[3], "earn") && strcmp(f[3], "revoke")))
      fail("event grammar");
    if (!pu64(f[0], &q) || !pu64(f[1], &q))
      fail("event position");
    bal = pdbl(f[4]);
    getkey(f, 5, k);
    if (!k[3])
      fail("context-zero event");
    e = event_for(f[2], k, 1);
    if (!strcmp(f[3], "earn")) {
      if (e->state || bal < 32.0)
        fail("false EARN");
      e->state = 1;
      c->earn++;
    } else {
      if (!e->state || bal >= 16.0)
        fail("false REVOKE");
      e->state = 0;
      c->revoke++;
    }
    c->events++;
    p = nl + 1;
  }
  free(copy);
}

static void verify_relations(const unsigned char *raw, size_t n, Census *c) {
  static const char H[] = "arm\ttarget_s\ttarget_d\ttarget_epoch\tcontext_"
                          "len\tc1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_"
                          "s\tc3_d\tc3_epoch\tseen\tpositive\tnegative\tledger_"
                          "bits\tpeak_bits\tstate\tL\tever_earned";
  char *copy, *p, *nl;
  int row = 0;
  Event rel[MAX_ROWS];
  size_t nr = 0, i;
  if (!n || raw[n - 1] != '\n' || memchr(raw, '\r', n) || memchr(raw, 0, n))
    fail("relations are not cleanly sealed");
  copy = malloc(n + 1);
  if (!copy)
    fail("memory");
  memcpy(copy, raw, n);
  copy[n] = 0;
  p = copy;
  while (*p) {
    char *f[MAX_FIELDS];
    int nf;
    uint64_t k[13], seen, pos, neg, state, ever;
    double led, peak, L;
    Event *e;
    nl = strchr(p, '\n');
    *nl = 0;
    row++;
    if (row == 1) {
      if (strcmp(p, H))
        fail("relation header drifted");
      p = nl + 1;
      continue;
    }
    nf = split(p, f, MAX_FIELDS);
    if (nf != 22 || !arm_ok(f[0]))
      fail("relation grammar");
    getkey(f, 1, k);
    for (i = 0; i < nr; i++)
      if (sameid(&rel[i], f[0], k))
        fail("duplicate relation identity");
    if (nr == MAX_ROWS)
      fail("too many relations");
    memset(&rel[nr], 0, sizeof rel[nr]);
    snprintf(rel[nr].arm, sizeof rel[nr].arm, "%s", f[0]);
    memcpy(rel[nr].k, k, sizeof k);
    nr++;
    if (!pu64(f[14], &seen) || !pu64(f[15], &pos) || !pu64(f[16], &neg) ||
        !pu64(f[19], &state) || !pu64(f[21], &ever))
      fail("relation integer grammar");
    led = pdbl(f[17]);
    peak = pdbl(f[18]);
    L = pdbl(f[20]);
    if (pos > seen || neg > seen - pos || state > 1 || ever > 1 ||
        (state && !ever))
      fail("relation count/state invariant");
    if (!k[3] && (state || ever || L != 0.0))
      fail("context-zero authority");
    if (state && (led < 16.0 || L < 0.01 || L > 0.5))
      fail("live relation invariant");
    if (!state && L != 0.0)
      fail("shadow L invariant");
    if (ever && peak < 32.0)
      fail("peak below EARN");
    e = event_for(f[0], k, 0);
    if (ever && !e)
      fail("earned relation lacks event");
    if (!ever && e)
      fail("event on never-earned relation");
    if (e) {
      e->used = 1;
      if ((uint64_t)e->state != state)
        fail("last event/final state mismatch");
    }
    if (!strcmp(f[0], "relation")) {
      c->candidates++;
      c->live += state;
      c->ever += ever;
    } else
      c->controls++;
    p = nl + 1;
  }
  for (i = 0; i < nev; i++)
    if (!ev[i].used)
      fail("event has no final relation");
  free(copy);
}

static char *line_take(unsigned char **p, unsigned char *end) {
  unsigned char *q = memchr(*p, '\n', (size_t)(end - *p));
  char *s;
  if (!q)
    fail("unsealed header");
  s = malloc((size_t)(q - *p) + 1);
  if (!s)
    fail("memory");
  memcpy(s, *p, (size_t)(q - *p));
  s[q - *p] = 0;
  *p = q + 1;
  return s;
}

int main(int argc, char **argv) {
  const char *ledger = argc == 2 ? argv[1] : ".mycelium.court4-witness";
  size_t ln, bn;
  unsigned char *lraw, *braw, *p, *end;
  char *line, *f[MAX_FIELDS], bh[65], got[65], path[4096];
  uint64_t chain = 0xcbf29ce484222325ULL, claim, v[7], sizes[3];
  char hashes[4][65];
  int row = 0, nf, i;
  Census c = {0};
  if (argc > 2) {
    fputs("usage: adaptation_check [witness-ledger]\n", stderr);
    return 1;
  }
  lraw = read_reg(ledger, &ln);
  if (!ln || lraw[ln - 1] != '\n' || memchr(lraw, '\r', ln) ||
      memchr(lraw, 0, ln))
    fail("witness is not cleanly sealed");
  p = lraw;
  end = lraw + ln;
  while (p < end) {
    unsigned char *q = memchr(p, '\n', (size_t)(end - p));
    size_t n, pay;
    if (!q)
      fail("unsealed witness row");
    n = (size_t)(q - p);
    if (n < 18 || p[n - 17] != '\t' || !lowerhex((char *)p + n - 16, 16))
      fail("witness chain field");
    pay = n - 17;
    chain = fold(p, pay, chain);
    {
      char tmp[17];
      memcpy(tmp, p + n - 16, 16);
      tmp[16] = 0;
      claim = strtoull(tmp, NULL, 16);
    }
    if (chain != claim)
      fail("witness chain broken");
    line = malloc(pay + 1);
    if (!line)
      fail("memory");
    memcpy(line, p, pay);
    line[pay] = 0;
    nf = split(line, f, MAX_FIELDS);
    row++;
    if (row == 1) {
      if (nf != 3 || strcmp(f[0], "A") || strcmp(f[1], "1") ||
          strcmp(f[2], LAW))
        fail("witness law row");
    } else if (row == 2) {
      if (nf != 15 || strcmp(f[0], "W") || strcmp(f[1], "1") ||
          strcmp(f[14], "zero-authority"))
        fail("witness record grammar");
      for (i = 0; i < 5; i++)
        if (strlen(f[2 + i]) != 64 || !lowerhex(f[2 + i], 64))
          fail("witness SHA field");
      if (strcmp(f[2], C8_SHA) || strcmp(f[3], MANIFEST_SHA) ||
          strcmp(f[4], A1_EVENTS_SHA) || strcmp(f[5], A1_RELATIONS_SHA))
        fail("witness Court or manifest identity drifted");
      strcpy(hashes[0], f[3]);
      strcpy(hashes[1], f[4]);
      strcpy(hashes[2], f[5]);
      strcpy(bh, f[6]);
      for (i = 0; i < 7; i++)
        if (!pu64(f[7 + i], &v[i]))
          fail("witness census field");
    } else
      fail("witness has more than two rows");
    free(line);
    p = q + 1;
  }
  free(lraw);
  if (row != 2)
    fail("witness does not have two rows");
  if (snprintf(path, sizeof path, "%s.d/%s", ledger, bh) >= (int)sizeof path)
    fail("blob path too long");
  braw = read_reg(path, &bn);
  sha_hex(braw, bn, got);
  if (strcmp(got, bh))
    fail("biography blob digest mismatch");
  p = braw;
  end = braw + bn;
  line = line_take(&p, end);
  if (strcmp(line, "A1B\t1\tcourt4-relation-biography-v1"))
    fail("blob law header");
  free(line);
  for (i = 0; i < 3; i++) {
    line = line_take(&p, end);
    nf = split(line, f, MAX_FIELDS);
    if (nf != 3 ||
        strcmp(f[0], i == 0   ? "manifest"
                     : i == 1 ? "events"
                              : "relations") ||
        !pu64(f[1], &sizes[i]) || strlen(f[2]) != 64 || !lowerhex(f[2], 64) ||
        strcmp(f[2], hashes[i]))
      fail("blob section header");
    free(line);
  }
  if (sizes[0] > SIZE_MAX - sizes[1] ||
      sizes[0] + sizes[1] > SIZE_MAX - sizes[2] ||
      (uint64_t)(end - p) != sizes[0] + sizes[1] + sizes[2])
    fail("blob section lengths");
  for (i = 0; i < 3; i++) {
    sha_hex(p, (size_t)sizes[i], got);
    if (strcmp(got, hashes[i]))
      fail("blob section digest mismatch");
    if (i == 1)
      verify_events(p, (size_t)sizes[i], &c);
    if (i == 2)
      verify_relations(p, (size_t)sizes[i], &c);
    p += (size_t)sizes[i];
  }
  if (c.events != v[0] || c.earn != v[1] || c.revoke != v[2] ||
      c.candidates != v[3] || c.live != v[4] || c.ever != v[5] ||
      c.controls != v[6])
    fail("witness census disagrees with independent replay");
  printf("adaptation_check: 2 records, %llu events (%llu EARN, %llu REVOKE), "
         "%llu candidates / %llu live / %llu ever-earned, %llu controls, chain "
         "%016llx\n",
         (unsigned long long)c.events, (unsigned long long)c.earn,
         (unsigned long long)c.revoke, (unsigned long long)c.candidates,
         (unsigned long long)c.live, (unsigned long long)c.ever,
         (unsigned long long)c.controls, (unsigned long long)chain);
  printf("scope: A1 witnesses Court-4 citizenship history; it grants no diet, "
         "vote, mint, ranking, or circulation authority\n");
  free(braw);
  return 0;
}
