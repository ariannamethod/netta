/* adaptation_witness.cpp -- body A1: a powerless reader of Court 4.

   The old mycelium bodies grow from their own grave.  A1 does not feed that
   grave, rank a fragment, vote, mint, or spend.  It reads the compact public
   Court-4 biography capsule, checks its byte identities and relation laws,
   and writes one append-only witness plus one content-addressed blob under a
   new namespace.  The blob carries the exact manifest, event book, and final
   relation book, so a later diet court can read citizens without reopening
   the spent experiment.

   usage: adaptation_witness <capsule-dir> [witness-ledger]

   Default ledger: .mycelium.court4-witness
   Blob directory: <witness-ledger>.d                                      */

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

static const uint64_t FNV_SEED = 0xcbf29ce484222325ULL;
static const uint64_t FNV_PRIME = 0x100000001b3ULL;
static const char *LAW = "body-a1-court4-witness-v1";
static const char *BIO_DOMAIN =
    "arianna-method.netta.mycelium.court4-biography/v1";
static const char *SOURCE_COMMIT = "e02c645";
static const char *VERDICT =
    "CONFIRMATORY PASS: microscopic relation replicated in selected class";
static const size_t FILE_MAX = 8u * 1024u * 1024u;

[[noreturn]] static void die(const std::string &s) {
  fprintf(stderr, "adaptation_witness: %s\n", s.c_str());
  exit(1);
}

static uint64_t fnv64(const std::string &s, uint64_t h = FNV_SEED) {
  for (unsigned char c : s) {
    h ^= c;
    h *= FNV_PRIME;
  }
  return h;
}

static std::string hex16(uint64_t v) {
  char b[17];
  snprintf(b, sizeof b, "%016llx", (unsigned long long)v);
  return b;
}

/* Small local SHA-256.  Artifact identity uses Court 4's SHA law; the
   witness chain itself keeps the historical mycelium FNV fold. */
struct Sha256 {
  uint32_t h[8] = {0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                   0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
  uint64_t bits = 0;
  uint8_t buf[64] = {0};
  size_t used = 0;

  static uint32_t rotr(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
  }
  void block(const uint8_t *p) {
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
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
      w[i] = (uint32_t)p[4 * i] << 24 | (uint32_t)p[4 * i + 1] << 16 |
             (uint32_t)p[4 * i + 2] << 8 | p[4 * i + 3];
    for (int i = 16; i < 64; ++i) {
      uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
      uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5],
             g = h[6], z = h[7];
    for (int i = 0; i < 64; ++i) {
      uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
      uint32_t ch = (e & f) ^ (~e & g);
      uint32_t t1 = z + s1 + ch + k[i] + w[i];
      uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
      uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
      uint32_t t2 = s0 + maj;
      z = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += z;
  }
  void add(const void *vp, size_t n) {
    const uint8_t *p = static_cast<const uint8_t *>(vp);
    bits += (uint64_t)n * 8;
    while (n) {
      size_t take = 64 - used;
      if (take > n)
        take = n;
      memcpy(buf + used, p, take);
      used += take;
      p += take;
      n -= take;
      if (used == 64) {
        block(buf);
        used = 0;
      }
    }
  }
  std::string finish() {
    uint64_t total = bits;
    uint8_t one = 0x80, zero = 0;
    add(&one, 1);
    while (used != 56)
      add(&zero, 1);
    uint8_t tail[8];
    for (int i = 0; i < 8; ++i)
      tail[7 - i] = (uint8_t)(total >> (8 * i));
    add(tail, 8);
    static const char x[] = "0123456789abcdef";
    std::string out(64, '0');
    for (int i = 0; i < 8; ++i)
      for (int j = 0; j < 4; ++j) {
        uint8_t q = (uint8_t)(h[i] >> (24 - 8 * j));
        out[8 * i + 2 * j] = x[q >> 4];
        out[8 * i + 2 * j + 1] = x[q & 15];
      }
    return out;
  }
};

static std::string sha256(const std::string &s) {
  Sha256 h;
  h.add(s.data(), s.size());
  return h.finish();
}

static std::string join_path(const std::string &a, const std::string &b) {
  return a.empty() || a.back() == '/' ? a + b : a + "/" + b;
}

static std::string read_regular(const std::string &path) {
  struct stat st;
  if (lstat(path.c_str(), &st) != 0)
    die("cannot stat " + path);
  if (!S_ISREG(st.st_mode))
    die("input is not a regular file: " + path);
  if ((uint64_t)st.st_size > FILE_MAX)
    die("input exceeds 8 MiB: " + path);
  FILE *f = fopen(path.c_str(), "rb");
  if (!f)
    die("cannot open " + path);
  std::string out((size_t)st.st_size, '\0');
  if (!out.empty() && fread(&out[0], 1, out.size(), f) != out.size()) {
    fclose(f);
    die("cannot read " + path);
  }
  if (fgetc(f) != EOF || ferror(f) || fclose(f) != 0)
    die("input changed while reading: " + path);
  return out;
}

static std::vector<std::string> lines(const std::string &raw,
                                      const std::string &name) {
  if (raw.empty() || raw.back() != '\n')
    die(name + " is not newline sealed");
  if (raw.find('\r') != std::string::npos ||
      raw.find('\0') != std::string::npos)
    die(name + " contains CR or NUL");
  std::vector<std::string> out;
  size_t p = 0;
  while (p < raw.size()) {
    size_t q = raw.find('\n', p);
    out.push_back(raw.substr(p, q - p));
    p = q + 1;
  }
  return out;
}

static std::vector<std::string> tabs(const std::string &s) {
  std::vector<std::string> out;
  size_t p = 0;
  for (;;) {
    size_t q = s.find('\t', p);
    if (q == std::string::npos) {
      out.push_back(s.substr(p));
      return out;
    }
    out.push_back(s.substr(p, q - p));
    p = q + 1;
  }
}

static uint64_t u64(const std::string &s, const std::string &where) {
  if (s.empty() || s.size() > 20 || (s.size() > 1 && s[0] == '0'))
    die(where + ": non-canonical integer");
  uint64_t v = 0;
  for (char c : s) {
    if (c < '0' || c > '9' || v > (UINT64_MAX - (uint64_t)(c - '0')) / 10)
      die(where + ": non-canonical integer");
    v = v * 10 + (uint64_t)(c - '0');
  }
  return v;
}

static double number(const std::string &s, const std::string &where) {
  if (s.empty() || s[0] == '+' || s.find(' ') != std::string::npos)
    die(where + ": non-canonical number");
  char *end = nullptr;
  errno = 0;
  double v = strtod(s.c_str(), &end);
  if (errno || end != s.c_str() + s.size() || !std::isfinite(v))
    die(where + ": non-finite or malformed number");
  return v;
}

struct ExpectedPin {
  const char *role, *path;
  uint64_t bytes;
  const char *sha;
};

static const ExpectedPin STAGES[] = {
    {"roots2", "COURT4_CONFIRMATORY_ROOTS2.tsv", 2123,
     "9b21de267935713d4d4b84d7281aca6968e1071b5878c558b99b58668b3bc3bc"},
    {"base_commit2", "BASE_COMMIT2.tsv", 186,
     "acf118b62a9505e5c4f3066239b97710ea5dce8d15cf3faf9d950abd49b06f31"},
    {"freeze2", "COURT4_BASE_COMMIT_FREEZE2.tsv", 274,
     "5de9b2f356353e3a22500dc3196f5f35f2f6d4bb32eb6d36f6d71c235e1287da"},
    {"selection2", "SELECTION2.tsv", 901,
     "69c3ee3d0e8e4d0b68f3f975edbba1245c95cb0b19599227a7f526996264dacc"},
    {"builder_receipt", "COURT4_DRAW2_BUILDER_OUTPUT_RECEIPT.tsv", 4379,
     "5281e130d35ef677fad7893fd96bee626a51e3fae9ad4aff5b70ea19299a5678"},
    {"c8_verdict", "COURT4_DRAW2_C8_VERDICT_RECORD.tsv", 5398,
     "b528c3e2efe1a2dd38b892e18f67e2db98338b61e617c1b581f119686fc76b1e"}};
#ifndef A1_EVENTS_BYTES
#define A1_EVENTS_BYTES 903
#endif
#ifndef A1_EVENTS_SHA
#define A1_EVENTS_SHA                                                          \
  "e8ac386fb11c6ba6199edea505ff0ad8c0b4c08284366823ea92a6b64f314b4c"
#endif
#ifndef A1_RELATIONS_BYTES
#define A1_RELATIONS_BYTES 36101
#endif
#ifndef A1_RELATIONS_SHA
#define A1_RELATIONS_SHA                                                       \
  "d3e5e514ea4cccc4a044ad224d19170e637beb79a810fb0875816f798211b438"
#endif
static const ExpectedPin ARTIFACTS[] = {
    {"events", "relation_events.tsv", A1_EVENTS_BYTES, A1_EVENTS_SHA},
    {"relations", "relations.tsv", A1_RELATIONS_BYTES, A1_RELATIONS_SHA}};

struct Summary {
  uint64_t events = 0, earn = 0, revoke = 0, candidates = 0, live = 0, ever = 0,
           controls = 0;
};

struct Manifest {
  std::map<std::string, uint64_t> summary;
  std::string manifest, events, relations;

  explicit Manifest(const std::string &dir) {
    manifest = read_regular(join_path(dir, "MANIFEST.tsv"));
    auto l = lines(manifest, "MANIFEST.tsv");
    if (l.size() != 18)
      die("MANIFEST.tsv must hold exactly 18 rows");
    if (l[0] != std::string("domain\t") + BIO_DOMAIN ||
        l[1] != std::string("source_commit\t") + SOURCE_COMMIT ||
        l[2] != std::string("verdict\t") + VERDICT)
      die("manifest identity, source commit, or verdict drifted");
    size_t at = 3;
    for (const auto &e : STAGES) {
      auto f = tabs(l[at++]);
      if (f.size() != 5 || f[0] != "stage" || f[1] != e.role ||
          f[2] != e.path || u64(f[3], "stage bytes") != e.bytes ||
          f[4] != e.sha)
        die("manifest stage order or pin drifted");
    }
    for (const auto &e : ARTIFACTS) {
      auto f = tabs(l[at++]);
      if (f.size() != 5 || f[0] != "artifact" || f[1] != e.role ||
          f[2] != e.path || u64(f[3], "artifact bytes") != e.bytes ||
          f[4] != e.sha)
        die("manifest artifact order or pin drifted");
      std::string raw = read_regular(join_path(dir, e.path));
      if (raw.size() != e.bytes || sha256(raw) != e.sha)
        die(std::string("artifact identity mismatch: ") + e.path);
      if (f[1] == "events")
        events = std::move(raw);
      else
        relations = std::move(raw);
    }
    static const char *names[] = {"events",        "earn",
                                  "revoke",        "relation_candidates",
                                  "relation_live", "relation_ever_earned",
                                  "control_rows"};
    for (const char *name : names) {
      auto f = tabs(l[at++]);
      if (f.size() != 3 || f[0] != "summary" || f[1] != name)
        die("manifest summary order drifted");
      summary[name] = u64(f[2], "manifest summary");
    }
  }
};

using Key = std::array<uint64_t, 13>;

static Key key_at(const std::vector<std::string> &f, size_t start,
                  const std::string &where) {
  Key k;
  for (size_t i = 0; i < k.size(); ++i)
    k[i] = u64(f[start + i], where);
  if (k[0] > 255 || k[1] > 255 || k[2] == 0 || k[3] > 3)
    die(where + ": invalid RelationKey head");
  for (size_t i = 0; i < 3; ++i) {
    size_t p = 4 + 3 * i;
    if (i < k[3]) {
      if (k[p] > 255 || k[p + 1] > 255 || k[p + 2] == 0)
        die(where + ": invalid live context tuple");
    } else if (k[p] || k[p + 1] || k[p + 2])
      die(where + ": unused context tuple is not zero");
  }
  return k;
}

static bool arm_ok(const std::string &arm) {
  if (arm == "relation" || arm == "oracle")
    return true;
  if (arm.rfind("null", 0) != 0)
    return false;
  std::string n = arm.substr(4);
  if (n.empty() || (n.size() > 1 && n[0] == '0'))
    return false;
  for (char c : n)
    if (c < '0' || c > '9')
      return false;
  return strtoul(n.c_str(), nullptr, 10) <= 18;
}

static std::string identity(const std::string &arm, const Key &k) {
  std::string s = arm;
  for (uint64_t v : k)
    s += "\t" + std::to_string(v);
  return s;
}

static Summary verify_books(const Manifest &m) {
  static const char *EH =
      "byte_offset\tunit_position\tarm\tevent\tledger_after\t"
      "target_s\ttarget_d\ttarget_epoch\tcontext_len\tc1_s\tc1_d\tc1_epoch\t"
      "c2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch";
  static const char *RH =
      "arm\ttarget_s\ttarget_d\ttarget_epoch\tcontext_len\t"
      "c1_s\tc1_d\tc1_epoch\tc2_s\tc2_d\tc2_epoch\tc3_s\tc3_d\tc3_epoch\t"
      "seen\tpositive\tnegative\tledger_bits\tpeak_bits\tstate\tL\tever_earned";
  Summary s;
  std::map<std::string, int> last_event;
  auto el = lines(m.events, "relation_events.tsv");
  if (el[0] != EH)
    die("relation_events.tsv header drifted");
  for (size_t i = 1; i < el.size(); ++i) {
    auto f = tabs(el[i]);
    std::string w = "event row " + std::to_string(i + 1);
    if (f.size() != 18 || !arm_ok(f[2]) || (f[3] != "earn" && f[3] != "revoke"))
      die(w + ": grammar");
    (void)u64(f[0], w);
    (void)u64(f[1], w);
    double balance = number(f[4], w);
    Key k = key_at(f, 5, w);
    if (k[3] == 0)
      die(w + ": context-zero key crossed authority");
    std::string id = identity(f[2], k);
    int &state = last_event[id];
    if (f[3] == "earn") {
      if (state || balance < 32.0)
        die(w + ": false or duplicate EARN");
      state = 1;
      s.earn++;
    } else {
      if (!state || balance >= 16.0)
        die(w + ": false or duplicate REVOKE");
      state = 0;
      s.revoke++;
    }
    s.events++;
  }

  std::set<std::string> seen;
  auto rl = lines(m.relations, "relations.tsv");
  if (rl[0] != RH)
    die("relations.tsv header drifted");
  for (size_t i = 1; i < rl.size(); ++i) {
    auto f = tabs(rl[i]);
    std::string w = "relation row " + std::to_string(i + 1);
    if (f.size() != 22 || !arm_ok(f[0]))
      die(w + ": grammar");
    Key k = key_at(f, 1, w);
    std::string id = identity(f[0], k);
    if (!seen.insert(id).second)
      die(w + ": duplicate arm/RelationKey");
    uint64_t n = u64(f[14], w), pos = u64(f[15], w), neg = u64(f[16], w);
    double ledger = number(f[17], w), peak = number(f[18], w),
           L = number(f[20], w);
    uint64_t state = u64(f[19], w), ever = u64(f[21], w);
    if (pos > n || neg > n - pos || state > 1 || ever > 1 || (state && !ever))
      die(w + ": count or state invariant");
    if (k[3] == 0 && (state || ever || L != 0.0))
      die(w + ": context-zero relation acquired authority");
    if (state && (ledger < 16.0 || L < 0.01 || L > 0.5))
      die(w + ": live relation has unlawful balance or L");
    if (!state && L != 0.0)
      die(w + ": shadow relation has nonzero L");
    if (ever && peak < 32.0)
      die(w + ": earned relation never reached EARN");
    auto e = last_event.find(id);
    if (ever && e == last_event.end())
      die(w + ": ever-earned relation has no event");
    if (!ever && e != last_event.end())
      die(w + ": event belongs to never-earned relation");
    if (e != last_event.end() && (uint64_t)e->second != state)
      die(w + ": final state disagrees with last EARN/REVOKE");
    if (f[0] == "relation") {
      s.candidates++;
      s.live += state;
      s.ever += ever;
    } else
      s.controls++;
  }
  for (const auto &e : last_event)
    if (!seen.count(e.first))
      die("event names no final relation row");
  return s;
}

static void check_summary(const Manifest &m, const Summary &s) {
  const std::pair<const char *, uint64_t> v[] = {
      {"events", s.events},        {"earn", s.earn},
      {"revoke", s.revoke},        {"relation_candidates", s.candidates},
      {"relation_live", s.live},   {"relation_ever_earned", s.ever},
      {"control_rows", s.controls}};
  for (const auto &x : v) {
    auto it = m.summary.find(x.first);
    if (it == m.summary.end() || it->second != x.second)
      die(std::string("manifest summary drifted: ") + x.first);
  }
}

static void ensure_dir(const std::string &path) {
  struct stat st;
  if (lstat(path.c_str(), &st) == 0) {
    if (!S_ISDIR(st.st_mode))
      die("blob path is not a directory");
    return;
  }
  if (errno != ENOENT || mkdir(path.c_str(), 0755) != 0)
    die("cannot create blob directory");
}

static void write_atomic(const std::string &path, const std::string &raw,
                         mode_t mode) {
  struct stat st;
  if (lstat(path.c_str(), &st) == 0) {
    if (!S_ISREG(st.st_mode) || read_regular(path) != raw)
      die("existing output disagrees: " + path);
    return;
  }
  if (errno != ENOENT)
    die("cannot inspect output " + path);
  std::string tmp =
      path + ".tmp." + std::to_string((unsigned long long)getpid());
  int fd = open(tmp.c_str(), O_WRONLY | O_CREAT | O_EXCL, mode);
  if (fd < 0)
    die("cannot create temporary output");
  size_t off = 0;
  while (off < raw.size()) {
    ssize_t n = write(fd, raw.data() + off, raw.size() - off);
    if (n <= 0) {
      close(fd);
      unlink(tmp.c_str());
      die("output write failed");
    }
    off += (size_t)n;
  }
  if (fsync(fd) != 0 || close(fd) != 0 ||
      rename(tmp.c_str(), path.c_str()) != 0) {
    unlink(tmp.c_str());
    die("cannot seal output");
  }
}

static std::string sealed(const std::string &payload, uint64_t &chain) {
  chain = fnv64(payload, chain);
  return payload + "\t" + hex16(chain) + "\n";
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    fputs("usage: adaptation_witness <capsule-dir> [witness-ledger]\n", stderr);
    return 1;
  }
  std::string ledger = argc == 3 ? argv[2] : ".mycelium.court4-witness";
  Manifest m(argv[1]);
  Summary s = verify_books(m);
  check_summary(m, s);
  std::string mh = sha256(m.manifest), eh = sha256(m.events),
              rh = sha256(m.relations);
  std::string head = std::string("A1B\t1\tcourt4-relation-biography-v1\n") +
                     "manifest\t" + std::to_string(m.manifest.size()) + "\t" +
                     mh + "\n" + "events\t" + std::to_string(m.events.size()) +
                     "\t" + eh + "\n" + "relations\t" +
                     std::to_string(m.relations.size()) + "\t" + rh + "\n";
  std::string blob = head + m.manifest + m.events + m.relations;
  std::string bh = sha256(blob), blobdir = ledger + ".d";
  ensure_dir(blobdir);
  write_atomic(join_path(blobdir, bh), blob, 0444);

  uint64_t chain = FNV_SEED;
  std::string out = sealed(std::string("A\t1\t") + LAW, chain);
  std::string w = std::string("W\t1\t") + STAGES[5].sha + "\t" + mh + "\t" +
                  eh + "\t" + rh + "\t" + bh + "\t" + std::to_string(s.events) +
                  "\t" + std::to_string(s.earn) + "\t" +
                  std::to_string(s.revoke) + "\t" +
                  std::to_string(s.candidates) + "\t" + std::to_string(s.live) +
                  "\t" + std::to_string(s.ever) + "\t" +
                  std::to_string(s.controls) + "\tzero-authority";
  out += sealed(w, chain);
  struct stat prior;
  bool existed = lstat(ledger.c_str(), &prior) == 0; /* only for wording */
  write_atomic(ledger, out, 0644);
  printf("%s Court 4: %llu candidate relations, %llu live, %llu ever-earned; "
         "%llu controls; witness %s chain %s\n",
         existed ? "already witnessed" : "witnessed",
         (unsigned long long)s.candidates, (unsigned long long)s.live,
         (unsigned long long)s.ever, (unsigned long long)s.controls, bh.c_str(),
         hex16(chain).c_str());
  return 0;
}
