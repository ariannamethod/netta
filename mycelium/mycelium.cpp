/* mycelium.cpp -- body 1: the graft. The organ that grows between lives.
   A C++17 port of microkarpathy's mycelium.py resonance mechanics under
   the byte law of mycelium/MYCELIUM.md, plus the two organs the prototype
   never had: the event-sourced ledger and the attestation gate. The
   prototype's token edges are not ported: it grows and decays them but
   resonate/unfold never read them -- a dead organ stays buried.

   State is the grave plus the ledger; the field is derived by replay on
   every run. Speech enters only when its FNV-1a-64 digest matches an s
   event in the biography supplied beside it; that attests the pair, not
   authorship. The dedup key and the pinned biography digest cover the
   ATTESTATION PREFIX -- the biography's bytes through the matched s line
   -- so the same event refuses forever while the life keeps living.

   Ledger record grammar (tab-separated payload, then '\t' hex16 chain):
     V \t 1 \t body1-byte-v1
     G \t label \t priordigest \t ordinal \t speechdigest \t bytes \t slen \t s-line
     U \t promptdigest \t corpsedigest \t k \t touched \t sources-csv
   The chain folds each payload's raw bytes into the running FNV-1a-64
   (seed cbf29ce484222325); the hex16 after the payload is the chain
   value after that fold. The s-line field is read by slen, not by tabs.
   ablate is an instrument: it reads the field and writes nothing. */

#include <algorithm>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

/* body 5's forge: linked from the installation path, never vendored */
extern "C" {
#include <ariannamethod/notorch.h>
}

static const int DIM = 96;
static const uint64_t FNV_SEED = 0xcbf29ce484222325ULL;
static const uint64_t FNV_PRIME = 0x100000001b3ULL;
static const char *LEDGER_PATH = ".mycelium.ledger";
static const char *GRAVE_DIR = ".mycelium.grave";
static const char *LEDGER_LAW = "body1-byte-v1";
static const size_t LEDGER_RECORD_MAX = 4096;
static const char *PROPS_PATH = ".mycelium.proposals";
static const char *PROPS_LAW = "body2-props-v1";
static const uint64_t RENT_WINDOW = 16;
static const char *SCHOOL_PATH = ".mycelium.school";
static const char *SCHOOL_LAW = "body3-school-v2";
static const char *BASELINE_LAW = "laplace-unigram-lmf-u6-libm-v1";
static const uint64_t W_EXAM = 8;
static const uint64_t GAIN_MIN = 8000000; /* microbits: one full byte */
static const char *PARL_PATH = ".mycelium.parliament";
static const char *PARL_LAW = "body4-parl-v2";
static const char *RECOGNIZER_V1 = "pair-triple-v1";
static const uint64_t SCAR_WINDOW = 16;  /* meals from the scar's closing meal */
static const uint64_t FREEZE_WINDOW = 8; /* meals from the frozen petition    */
static const char *NOTES_PATH = ".mycelium.notes";
static const char *NOTES_LAW = "body5-notes-v1";
static const char *NOTES_DIR = ".mycelium.notes.d";
static const char *FORGE_HEADER_PATH =
    "/opt/homebrew/include/ariannamethod/notorch.h";
static const char *FORGE_LIB_PATH = "/opt/homebrew/lib/libnotorch.a";
static const uint64_t NOTE_STEPS_PASS = 512;
static const uint64_t NOTE_STEPS_WEAKEN = 256;
static const uint64_t NOTE_COOLDOWN = 8;   /* meals between trainings */
static const size_t NOTE_BLOB_BYTES = 97 * 4; /* 96 f32 weights + f32 bias */
static const float NOTE_LR = 0.05f;

static uint64_t fnv64(const void *p, size_t n, uint64_t h) {
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= FNV_PRIME; }
    return h;
}

static void f32_to_le(char *out, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    out[0] = (char)(bits & 0xffu);
    out[1] = (char)((bits >> 8) & 0xffu);
    out[2] = (char)((bits >> 16) & 0xffu);
    out[3] = (char)((bits >> 24) & 0xffu);
}

static float f32_from_le(const char *in) {
    uint32_t bits = (uint32_t)(uint8_t)in[0] |
                    ((uint32_t)(uint8_t)in[1] << 8) |
                    ((uint32_t)(uint8_t)in[2] << 16) |
                    ((uint32_t)(uint8_t)in[3] << 24);
    float value;
    memcpy(&value, &bits, sizeof value);
    return value;
}

static std::string hex16(uint64_t v) {
    char b[17];
    snprintf(b, sizeof b, "%016llx", (unsigned long long)v);
    return std::string(b);
}

[[noreturn]] static void die(const std::string &msg) {
    fprintf(stderr, "mycelium: %s\n", msg.c_str());
    exit(1);
}

static bool read_file(const std::string &path, std::string &out) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) return false;
    char buf[65536];
    out.clear();
    for (;;) {
        size_t n = fread(buf, 1, sizeof buf, f);
        if (n) out.append(buf, n);
        if (n < sizeof buf) {
            bool ok = !ferror(f);
            fclose(f);
            return ok;
        }
    }
}

/* ---- the byte law: hashing, lowering, cuts over bytes, ASCII semantics */

static std::unordered_map<std::string, std::vector<double>> gram_cache;
static std::unordered_map<std::string, std::vector<double>> word_cache;

static const std::vector<double> &fnv_vec(const std::string &s) {
    auto it = gram_cache.find(s);
    if (it != gram_cache.end()) return it->second;
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    std::vector<double> v;
    v.reserve(DIM);
    for (int i = 0; i < DIM; ++i) {
        h ^= h >> 13; h *= 1597334677u; h ^= h >> 16;
        v.push_back((double)(h & 0xFFFF) / 32768.0 - 1.0);
    }
    return gram_cache.emplace(s, std::move(v)).first->second;
}

static const std::vector<double> &embed(const std::string &word) {
    auto it = word_cache.find(word);
    if (it != word_cache.end()) return it->second;
    std::string w = "^" + word + "$";
    std::vector<std::string> grams;
    if (w.size() >= 3)
        for (size_t i = 0; i + 3 <= w.size(); ++i) grams.push_back(w.substr(i, 3));
    else
        grams.push_back(w);
    std::vector<double> vec(DIM, 0.0);
    for (const auto &g : grams) {
        const auto &gv = fnv_vec(g);
        for (int i = 0; i < DIM; ++i) vec[i] += gv[i];
    }
    double norm = 0.0;
    for (double x : vec) norm += x * x;
    norm = std::sqrt(norm) + 1e-10;
    for (double &x : vec) x /= norm;
    return word_cache.emplace(word, std::move(vec)).first->second;
}

static std::vector<double> mean_embed(const std::vector<std::string> &toks) {
    std::vector<double> vec(DIM, 0.0);
    if (toks.empty()) return vec;
    for (const auto &t : toks) {
        const auto &e = embed(t);
        for (int i = 0; i < DIM; ++i) vec[i] += e[i];
    }
    double norm = 0.0;
    for (double x : vec) norm += x * x;
    norm = std::sqrt(norm) + 1e-10;
    for (double &x : vec) x /= norm;
    return vec;
}

static double cosine(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0.0;
    for (int i = 0; i < DIM; ++i) s += a[i] * b[i];
    return s;
}

static bool byte_space(unsigned char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f' ||
           (c >= 0x1c && c <= 0x1f);
}

static std::vector<std::string> split_ws(const std::string &s) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < s.size()) {
        while (i < s.size() && byte_space(s[i])) ++i;
        size_t j = i;
        while (j < s.size() && !byte_space(s[j])) ++j;
        if (j > i) out.push_back(s.substr(i, j - i));
        i = j;
    }
    return out;
}

static const std::set<std::string> &stops() {
    static std::set<std::string> s;
    if (s.empty()) {
        const char *raw =
            "the a an and or but of to in on at is are was were be been it its this that these "
            "those i you he she we they me him her us them my your his our their as by for with "
            "from into over under so not no do does did have has had will would can could he's "
            "it's there here then than when where what which who whom whose all any some more most";
        for (const auto &t : split_ws(raw)) s.insert(t);
    }
    return s;
}

static std::vector<std::string> segment(const std::string &text) {
    std::vector<std::string> entries;
    /* blocks split on a newline, optional blank bytes, newline */
    std::vector<std::string> blocks;
    size_t start = 0, i = 0;
    while (i < text.size()) {
        if (text[i] == '\n') {
            size_t j = i + 1;
            while (j < text.size() && text[j] != '\n' && byte_space(text[j])) ++j;
            if (j < text.size() && text[j] == '\n') {
                blocks.push_back(text.substr(start, i - start));
                i = j + 1;
                start = i;
                continue;
            }
        }
        ++i;
    }
    blocks.push_back(text.substr(start));
    for (const auto &block : blocks) {
        auto words = split_ws(block);
        if (words.empty()) continue;
        std::string line;
        for (size_t w = 0; w < words.size(); ++w) {
            if (w) line += ' ';
            line += words[w];
        }
        /* sentence cut after . ! ? followed by the single spaces above */
        size_t p = 0;
        for (size_t q = 0; q < line.size(); ++q) {
            char c = line[q];
            if ((c == '.' || c == '!' || c == '?') &&
                (q + 1 == line.size() || line[q + 1] == ' ')) {
                std::string sent = line.substr(p, q - p + 1);
                size_t a = sent.find_first_not_of(' ');
                size_t b = sent.find_last_not_of(' ');
                if (a != std::string::npos) sent = sent.substr(a, b - a + 1);
                if (sent.size() > 12) entries.push_back(sent);
                p = q + 1;
                while (p < line.size() && line[p] == ' ') ++p;
                q = p ? p - 1 : 0;
            }
        }
        if (p < line.size()) {
            std::string sent = line.substr(p);
            size_t a = sent.find_first_not_of(' ');
            size_t b = sent.find_last_not_of(' ');
            if (a != std::string::npos) sent = sent.substr(a, b - a + 1);
            if (sent.size() > 12) entries.push_back(sent);
        }
    }
    return entries;
}

static std::vector<std::string> tokenize(const std::string &entry) {
    static const std::string strip = ".,!?;:\"'()[]{}-*_";
    std::string low = entry;
    for (char &c : low)
        if (c >= 'A' && c <= 'Z') c = (char)(c - 'A' + 'a');
    std::vector<std::string> out;
    for (auto &t : split_ws(low)) {
        size_t a = 0, b = t.size();
        while (a < b && strip.find(t[a]) != std::string::npos) ++a;
        while (b > a && strip.find(t[b - 1]) != std::string::npos) --b;
        std::string w = t.substr(a, b - a);
        if (w.size() > 2 && !stops().count(w)) out.push_back(w);
    }
    return out;
}

static std::string cut_clause(const std::string &text) {
    auto words = split_ws(text);
    size_t n = 6 + (words.size() % 11); /* lo=6 hi=16 */
    if (n > words.size()) n = words.size();
    std::string frag;
    for (size_t i = 0; i < n; ++i) {
        if (i) frag += ' ';
        frag += words[i];
    }
    while (!frag.empty()) {
        char c = frag.back();
        if (c == '.' || c == ',' || c == ';' || c == ':') frag.pop_back();
        else break;
    }
    return frag;
}

/* ---- canonical field parsers (the s line's own laws) */

static bool canon_u64(const std::string &s, uint64_t *out) {
    if (s.empty() || s.size() > 20) return false;
    if (s.size() > 1 && s[0] == '0') return false;
    uint64_t v = 0;
    for (char c : s) {
        if (c < '0' || c > '9') return false;
        if (v > (UINT64_MAX - (uint64_t)(c - '0')) / 10) return false;
        v = v * 10 + (uint64_t)(c - '0');
    }
    *out = v;
    return true;
}

static bool canon_hex16(const std::string &s, uint64_t *out) {
    if (s.size() != 16) return false;
    uint64_t v = 0;
    for (char c : s) {
        int d;
        if (c >= '0' && c <= '9') d = c - '0';
        else if (c >= 'a' && c <= 'f') d = c - 'a' + 10;
        else return false;
        v = (v << 4) | (uint64_t)d;
    }
    *out = v;
    return true;
}

static bool canon_hex4(const std::string &s) {
    if (s.size() != 4) return false;
    for (char c : s)
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) return false;
    return true;
}

static std::vector<std::string> split_tabs(const std::string &s) {
    std::vector<std::string> f;
    size_t p = 0;
    while (true) {
        size_t q = s.find('\t', p);
        if (q == std::string::npos) { f.push_back(s.substr(p)); break; }
        f.push_back(s.substr(p, q - p));
        p = q + 1;
    }
    return f;
}

static bool label_ok(const std::string &s) {
    if (s.empty() || s.size() > 32) return false;
    for (char c : s)
        if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-'))
            return false;
    return true;
}

static bool parse_s_event(const std::string &line, uint64_t *candidate,
                          uint64_t *bytes) {
    auto f = split_tabs(line);
    uint64_t episode, seed, lived;
    if (f.size() != 9 || f[0] != "s" || !canon_u64(f[1], &episode) ||
        !canon_hex16(f[2], candidate) || !canon_u64(f[3], bytes) || *bytes == 0 ||
        !canon_u64(f[4], &seed) ||
        (f[5] != "uni" && f[5] != "bi" && f[5] != "tri") ||
        (f[6] != "supported-backoff" && f[6] != "laplace-red") ||
        !canon_hex4(f[7]) || !canon_u64(f[8], &lived) || lived == 0)
        return false;
    return true;
}

static void write_bytes(FILE *f, const std::string &s, size_t limit = SIZE_MAX) {
    size_t n = std::min(s.size(), limit);
    if (n && fwrite(s.data(), 1, n, f) != n) die("stdout write failed");
}

/* ---- the ledger: append-only chain, replayed into the field */

struct GraveEvent {
    std::string label;
    uint64_t prior_digest;  /* attestation prefix: bio bytes through the s line */
    uint64_t ordinal;       /* the s line's 1-based line number */
    uint64_t speech_digest;
    uint64_t speech_bytes;
    std::string s_line;
};

struct Ledger {
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;

    void append(const std::string &payload) {
        if (payload.size() + 18 > LEDGER_RECORD_MAX)
            die("ledger record exceeds 4096-byte law");
        chain = fnv64(payload.data(), payload.size(), chain);
        std::string line = payload + "\t" + hex16(chain) + "\n";
        int fd = open(LEDGER_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) die("cannot open ledger for append");
        ssize_t w = write(fd, line.data(), line.size());
        if (w < 0 || (size_t)w != line.size() || close(fd) != 0)
            die("ledger append failed");
        records++;
    }
};

struct Frag {
    std::string text;
    std::string source; /* operator-supplied source label */
    std::string sref;   /* "label s#N" */
    uint64_t meal = 0;  /* which G event fed it, 1-based */
    std::vector<std::string> toks;
    std::vector<double> emb;
};

struct Mycelium {
    Ledger led;
    std::vector<Frag> frags;
    std::vector<std::string> corpora;   /* labels in ingest order */
    std::set<std::string> keys;         /* exact attestation witnesses eaten */
    std::set<std::string> blobs_seen;   /* speech digests referenced by G */
    std::set<std::pair<uint64_t, uint64_t>> prefixes; /* (G meals, chain) */
    bool version_seen = false;
    uint64_t meals = 0;                 /* G events replayed or eaten */

    static std::string make_key(uint64_t prior, uint64_t ord,
                                const std::string &sline) {
        return hex16(prior) + ":" + std::to_string(ord) + ":" + sline;
    }

    void add_fragments(const GraveEvent &g, const std::string &speech,
                       uint64_t *out_frags, uint64_t *out_toks) {
        uint64_t nf = 0, nt = 0;
        meals++;
        std::string sref = g.label + "@" + hex16(g.prior_digest) +
                           " s#" + std::to_string(g.ordinal);
        for (const auto &entry : segment(speech)) {
            auto toks = tokenize(entry);
            if (toks.size() < 2) continue;
            Frag f;
            f.text = entry;
            f.source = g.label;
            f.sref = sref;
            f.meal = meals;
            f.emb = mean_embed(toks);
            nt += toks.size();
            f.toks = std::move(toks);
            frags.push_back(std::move(f));
            nf++;
        }
        if (std::find(corpora.begin(), corpora.end(), g.label) == corpora.end())
            corpora.push_back(g.label);
        *out_frags = nf;
        *out_toks = nt;
    }

    /* replay the ledger against the grave; refuse anything that no longer
       reproduces. Line numbers are 1-based for every refusal. */
    void load() {
        std::string raw;
        if (!read_file(LEDGER_PATH, raw)) {
            struct stat st;
            if (stat(LEDGER_PATH, &st) == 0) die("cannot read ledger");
            return; /* no ledger yet: empty field */
        }
        size_t pos = 0;
        uint64_t lineno = 0;
        while (pos < raw.size()) {
            size_t nl = raw.find('\n', pos);
            if (nl == std::string::npos)
                die("ledger line " + std::to_string(lineno + 1) + " is unsealed");
            std::string line = raw.substr(pos, nl - pos);
            if (line.size() + 1 > LEDGER_RECORD_MAX)
                die("ledger line " + std::to_string(lineno + 1) +
                    " exceeds 4096-byte law");
            pos = nl + 1;
            lineno++;
            size_t tab = line.rfind('\t');
            if (tab == std::string::npos || line.size() - tab - 1 != 16)
                die("ledger line " + std::to_string(lineno) + " has no chain field");
            uint64_t claimed;
            if (!canon_hex16(line.substr(tab + 1), &claimed))
                die("ledger line " + std::to_string(lineno) + " chain is not hex16");
            std::string payload = line.substr(0, tab);
            uint64_t next = fnv64(payload.data(), payload.size(), led.chain);
            if (next != claimed)
                die("ledger chain broken at line " + std::to_string(lineno));
            led.chain = next;
            led.records++;
            replay(payload, lineno);
            prefixes.insert({meals, led.chain});
        }
        if (!version_seen) die("ledger has no version record");
    }

    void replay(const std::string &payload, uint64_t lineno) {
        auto f = split_tabs(payload);
        const std::string where = "ledger line " + std::to_string(lineno);
        if (f.empty()) die(where + " is empty");
        if (f[0] == "V") {
            if (lineno != 1 || version_seen || f.size() != 3 || f[1] != "1" ||
                f[2] != LEDGER_LAW)
                die(where + ": version record");
            version_seen = true;
        } else if (f[0] == "G") {
            if (!version_seen) die(where + ": G before version record");
            /* G label prior ord sdig sbytes slen s-line(by length, may hold tabs) */
            if (f.size() < 8) die(where + ": G arity");
            GraveEvent g;
            g.label = f[1];
            uint64_t slen;
            if (!label_ok(g.label) || !canon_hex16(f[2], &g.prior_digest) ||
                !canon_u64(f[3], &g.ordinal) || g.ordinal == 0 ||
                !canon_hex16(f[4], &g.speech_digest) || !canon_u64(f[5], &g.speech_bytes) ||
                g.speech_bytes == 0 || !canon_u64(f[6], &slen) || slen == 0)
                die(where + ": G field grammar");
            /* the s line is everything after the 7th tab, by declared length */
            size_t head = f[0].size() + f[1].size() + f[2].size() + f[3].size() +
                          f[4].size() + f[5].size() + f[6].size() + 7;
            if (payload.size() < head || payload.size() - head != slen)
                die(where + ": s line length does not match slen");
            g.s_line = payload.substr(head);
            uint64_t candidate, bytes;
            if (!parse_s_event(g.s_line, &candidate, &bytes) ||
                candidate != g.speech_digest || bytes != g.speech_bytes)
                die(where + ": attested s line is not canonical for its grave blob");
            std::string key = make_key(g.prior_digest, g.ordinal, g.s_line);
            if (!keys.insert(key).second) die(where + ": pair eaten twice");
            std::string blob_path = std::string(GRAVE_DIR) + "/" + hex16(g.speech_digest);
            std::string speech;
            if (!read_file(blob_path, speech))
                die(where + ": grave blob " + hex16(g.speech_digest) + " is missing");
            if (speech.size() != g.speech_bytes ||
                fnv64(speech.data(), speech.size(), FNV_SEED) != g.speech_digest)
                die(where + ": grave blob " + hex16(g.speech_digest) + " digest mismatch");
            blobs_seen.insert(hex16(g.speech_digest));
            uint64_t nf, nt;
            add_fragments(g, speech, &nf, &nt);
            (void)nf;
            (void)nt;
        } else if (f[0] == "U") {
            if (!version_seen) die(where + ": U before version record");
            if (f.size() != 6) die(where + ": U arity");
            uint64_t d, k, touched;
            if (!canon_hex16(f[1], &d) || !canon_hex16(f[2], &d) ||
                !canon_u64(f[3], &k) || k == 0 ||
                !canon_u64(f[4], &touched) || touched == 0 || touched > k)
                die(where + ": U field grammar");
            std::set<std::string> named;
            std::string prev;
            size_t p = 0;
            while (p <= f[5].size()) {
                size_t q = f[5].find(',', p);
                std::string label = f[5].substr(p, q == std::string::npos ? q : q - p);
                if (!label_ok(label) || !named.insert(label).second ||
                    std::find(corpora.begin(), corpora.end(), label) == corpora.end())
                    die(where + ": U sources");
                if (!prev.empty() && !(prev < label))
                    die(where + ": U sources are not in canonical order");
                prev = label;
                if (q == std::string::npos) break;
                p = q + 1;
            }
            if (named.size() != touched) die(where + ": U touched/source count");
            /* a receipt; nothing to rebuild */
        } else {
            die(where + ": unknown record type");
        }
    }

    void ensure_version() {
        if (version_seen) return;
        if (led.records != 0) die("cannot version a nonempty ledger");
        led.append(std::string("V\t1\t") + LEDGER_LAW);
        version_seen = true;
    }

    void report_orphans() const {
        struct stat st;
        if (stat(GRAVE_DIR, &st) != 0) return;
        if (!S_ISDIR(st.st_mode)) die("the grave is not a directory");
        DIR *dir = opendir(GRAVE_DIR);
        if (!dir) die("cannot read the grave");
        std::vector<std::string> names;
        while (struct dirent *de = readdir(dir)) {
            std::string name = de->d_name;
            if (name != "." && name != ".." && !blobs_seen.count(name))
                names.push_back(name);
        }
        closedir(dir);
        std::sort(names.begin(), names.end());
        for (const auto &name : names)
            fprintf(stderr, "mycelium: orphan grave blob %s (ignored)\n", name.c_str());
    }
};

struct ProposalState {
    std::string shape;
    std::set<uint64_t> meals;
    bool alive = false;
};

struct ProposalSnapshot {
    std::vector<ProposalState> states;
    std::string block;
    uint64_t alive = 0, dead = 0, digest = FNV_SEED;
};

/* One derivation serves both body 2's public receipt and body 3's
   historical enrollment check. This keeps "currently alive proposal"
   from becoming a second, drifting implementation inside the school. */
static ProposalSnapshot proposal_snapshot(const Mycelium &m, uint64_t after,
                                            uint64_t mainchain) {
    std::map<std::string, std::set<uint64_t>> shapes;
    for (const auto &f : m.frags) {
        if (f.meal > after) continue;
        for (size_t i = 0; i + 1 < f.toks.size(); ++i) {
            shapes[f.toks[i] + " " + f.toks[i + 1]].insert(f.meal);
            if (i + 2 < f.toks.size())
                shapes[f.toks[i] + " " + f.toks[i + 1] + " " + f.toks[i + 2]]
                    .insert(f.meal);
        }
    }
    ProposalSnapshot snap;
    for (const auto &kv : shapes) {
        if (kv.second.size() < 2) continue;
        ProposalState state;
        state.shape = kv.first;
        state.meals = kv.second;
        state.alive = after - *state.meals.rbegin() < RENT_WINDOW;
        snap.states.push_back(std::move(state));
    }
    std::stable_sort(snap.states.begin(), snap.states.end(),
                     [](const ProposalState &a, const ProposalState &b) {
        if (a.meals.size() != b.meals.size()) return a.meals.size() > b.meals.size();
        return a.shape < b.shape;
    });
    snap.block = "proposals after meal " + std::to_string(after) +
                 " (main chain " + hex16(mainchain) + "):\n";
    for (const auto &state : snap.states) {
        std::string csv;
        for (uint64_t meal : state.meals) {
            if (!csv.empty()) csv += ",";
            csv += std::to_string(meal);
        }
        if (state.alive) {
            snap.alive++;
            snap.block += "  alive " + state.shape + " support=" +
                          std::to_string(state.meals.size()) + " meals=" + csv + "\n";
        } else {
            snap.dead++;
            snap.block += "  dead " + state.shape + " support=" +
                          std::to_string(state.meals.size()) + " meals=" + csv +
                          " last=" + std::to_string(*state.meals.rbegin()) + "\n";
        }
    }
    if (snap.states.empty()) snap.block += "  (nothing recurs across meals yet)\n";
    snap.digest = fnv64(snap.block.data(), snap.block.size(), FNV_SEED);
    return snap;
}

static bool proposal_alive_at(const Mycelium &m, uint64_t after,
                              uint64_t mainchain, const std::string &shape) {
    ProposalSnapshot snap = proposal_snapshot(m, after, mainchain);
    for (const auto &state : snap.states)
        if (state.alive && state.shape == shape) return true;
    return false;
}

/* body 2: the proposer's receipts. Their own chain under their own law;
   the main ledger and its closed body1-byte-v1 law are never touched. */
struct Props {
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;
    uint64_t last_after = 0;
    bool law_seen = false;
    std::set<std::pair<uint64_t, uint64_t>> prefixes; /* (records, chain) */

    void load(const Mycelium &m) {
        std::string raw;
        if (!read_file(PROPS_PATH, raw)) {
            struct stat st;
            if (stat(PROPS_PATH, &st) == 0) die("cannot read proposals");
            return; /* no proposals yet */
        }
        if (raw.empty()) die("proposals has no law record");
        size_t pos = 0;
        uint64_t lineno = 0;
        while (pos < raw.size()) {
            size_t nl = raw.find('\n', pos);
            if (nl == std::string::npos)
                die("proposals line " + std::to_string(lineno + 1) + " is unsealed");
            std::string line = raw.substr(pos, nl - pos);
            if (line.size() + 1 > LEDGER_RECORD_MAX)
                die("proposals line " + std::to_string(lineno + 1) +
                    " exceeds 4096-byte law");
            pos = nl + 1;
            lineno++;
            size_t tab = line.rfind('\t');
            if (tab == std::string::npos || line.size() - tab - 1 != 16)
                die("proposals line " + std::to_string(lineno) + " has no chain field");
            uint64_t claimed;
            if (!canon_hex16(line.substr(tab + 1), &claimed))
                die("proposals line " + std::to_string(lineno) + " chain is not hex16");
            std::string payload = line.substr(0, tab);
            uint64_t next = fnv64(payload.data(), payload.size(), chain);
            if (next != claimed)
                die("proposals chain broken at line " + std::to_string(lineno));
            chain = next;
            records++;
            prefixes.insert({records, chain});
            auto f = split_tabs(payload);
            const std::string where = "proposals line " + std::to_string(lineno);
            if (f.empty()) die(where + " is empty");
            if (f[0] == "W") {
                if (lineno != 1 || f.size() != 3 || f[1] != "1" || f[2] != PROPS_LAW)
                    die(where + ": props law record");
                law_seen = true;
            } else if (f[0] == "P") {
                if (!law_seen) die(where + ": P before props law record");
                if (f.size() != 6) die(where + ": P arity");
                uint64_t after, mainchain, alive, dead, dig;
                if (!canon_u64(f[1], &after) || after == 0 ||
                    !canon_hex16(f[2], &mainchain) ||
                    !canon_u64(f[3], &alive) || !canon_u64(f[4], &dead) ||
                    !canon_hex16(f[5], &dig))
                    die(where + ": P field grammar");
                if (!m.prefixes.count({after, mainchain}))
                    die(where + ": P main prefix");
                if (after < last_after)
                    die(where + ": P after-meals is not monotonic");
                ProposalSnapshot snap = proposal_snapshot(m, after, mainchain);
                if (alive != snap.alive || dead != snap.dead || dig != snap.digest)
                    die(where + ": P snapshot drifted from the main prefix");
                last_after = after;
            } else {
                die(where + ": unknown proposals record type");
            }
        }
        if (records && !law_seen) die("proposals has no law record");
    }

    void append(const std::string &payload) {
        if (payload.size() + 18 > LEDGER_RECORD_MAX)
            die("proposals record exceeds 4096-byte law");
        chain = fnv64(payload.data(), payload.size(), chain);
        std::string line = payload + "\t" + hex16(chain) + "\n";
        int fd = open(PROPS_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) die("cannot open proposals for append");
        ssize_t w = write(fd, line.data(), line.size());
        if (w < 0 || (size_t)w != line.size() || close(fd) != 0)
            die("proposals append failed");
        records++;
    }
};

/* body 3: the school. A hypothesis enrolls, is priced only on the stream
   that arrives AFTER enrollment over a window fixed in the law, and is
   judged against a baseline that differs from the candidate by exactly
   one shape -- gain is marginal by construction. Verdicts pass, weak and
   fail are public forever; a pass mints a glyph whose only power is over
   LATER enrollments' baselines. The field is untouched. */

/* longest-match-first, leftmost, non-overlapping merge of unit shapes
   into a token stream; units hold their split token forms, tried arity 3
   before 2, in list order within one arity. */
static std::vector<std::string> merge_symbols(
        const std::vector<std::string> &toks,
        const std::vector<std::vector<std::string>> &units) {
    std::vector<std::string> out;
    size_t i = 0;
    while (i < toks.size()) {
        const std::vector<std::string> *hit = nullptr;
        for (size_t arity = 3; arity >= 2 && !hit; --arity) {
            for (const auto &u : units) {
                if (u.size() != arity || i + arity > toks.size()) continue;
                bool eq = true;
                for (size_t k = 0; k < arity && eq; ++k)
                    if (toks[i + k] != u[k]) eq = false;
                if (eq) { hit = &u; break; }
            }
        }
        if (hit) {
            std::string sym;
            for (size_t k = 0; k < hit->size(); ++k) {
                if (k) sym += ' ';
                sym += (*hit)[k];
            }
            out.push_back(sym);
            i += hit->size();
        } else {
            out.push_back(toks[i]);
            i++;
        }
    }
    return out;
}

/* prequential Laplace unigram over an evolving symbol alphabet:
   P(sym) = (count+1)/(N+A+1), where count, N and A (distinct symbols)
   are the state BEFORE this symbol is priced. */
struct Arm {
    std::unordered_map<std::string, uint64_t> counts;
    uint64_t n = 0;
    double bits = 0.0;

    void price(const std::string &sym) {
        auto it = counts.find(sym);
        uint64_t c = it == counts.end() ? 0 : it->second;
        double p = (double)(c + 1) /
                   (double)(n + (uint64_t)counts.size() + 1);
        bits += -std::log2(p);
        counts[sym] = c + 1;
        n++;
    }
};

struct Hyp {
    uint64_t id = 0, arity = 0;
    std::string shape;
    uint64_t after = 0;      /* enrolled after this meal */
    uint64_t last_meal = 0;  /* last observed meal, 0 = none */
    uint64_t base_total = 0, cand_total = 0; /* sums of sealed microbits */
    bool verdicted = false;
    bool passed = false;
    std::vector<std::vector<std::string>> base_units; /* legalised at enrollment */
    Arm base, cand;
};

static bool shape_canonical(const std::string &shape, uint64_t arity,
                            std::vector<std::string> *toks_out) {
    auto toks = split_ws(shape);
    if (toks.size() != arity || (arity != 2 && arity != 3)) return false;
    std::string joined;
    for (size_t i = 0; i < toks.size(); ++i) {
        if (i) joined += ' ';
        joined += toks[i];
    }
    if (joined != shape) return false;
    *toks_out = toks;
    return true;
}

struct School {
    const Mycelium &m;
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;
    bool law_seen = false;
    std::vector<Hyp> hyps;
    std::vector<std::string> legalised;  /* shapes in L order */
    uint64_t r_count = 0, o_count = 0, v_count = 0;
    uint64_t pending_glyph = 0;
    std::set<std::pair<uint64_t, uint64_t>> prefixes; /* (records, chain) */

    explicit School(const Mycelium &myc) : m(myc) {}

    Hyp *open_by_shape(const std::string &shape) {
        for (auto &h : hyps)
            if (!h.verdicted && h.shape == shape) return &h;
        return nullptr;
    }

    bool is_legalised(const std::string &shape) const {
        return std::find(legalised.begin(), legalised.end(), shape) !=
               legalised.end();
    }

    std::vector<std::vector<std::string>> unit_forms(const Hyp &h,
                                                     bool with_self) const {
        std::vector<std::vector<std::string>> units;
        for (const auto &u : h.base_units) units.push_back(u);
        if (with_self) units.push_back(split_ws(h.shape));
        return units;
    }

    /* price one meal for one hypothesis; returns sealed-scale microbits */
    void price_meal(Hyp &h, uint64_t meal, uint64_t *frags_out,
                    uint64_t *base_out, uint64_t *cand_out) {
        auto base_units = unit_forms(h, false);
        auto cand_units = unit_forms(h, true);
        double b0 = h.base.bits, c0 = h.cand.bits;
        uint64_t nf = 0;
        for (const auto &f : m.frags) {
            if (f.meal != meal) continue;
            nf++;
            for (const auto &sym : merge_symbols(f.toks, base_units))
                h.base.price(sym);
            for (const auto &sym : merge_symbols(f.toks, cand_units))
                h.cand.price(sym);
        }
        *frags_out = nf;
        double bd = h.base.bits - b0, cd = h.cand.bits - c0;
        double limit = (double)LLONG_MAX / 1e6;
        if (!std::isfinite(bd) || !std::isfinite(cd) || bd < 0.0 || cd < 0.0 ||
            bd > limit || cd > limit)
            die("school observation exceeds the microbit range");
        *base_out = (uint64_t)llround(bd * 1e6);
        *cand_out = (uint64_t)llround(cd * 1e6);
    }

    /* limit = 0 loads the whole chain; a nonzero limit stops after that
       many records — the historical view a pinned parliament ballot reads */
    void load(uint64_t limit = 0) {
        std::string raw;
        if (!read_file(SCHOOL_PATH, raw)) {
            struct stat st;
            if (stat(SCHOOL_PATH, &st) == 0) die("cannot read school");
            return;
        }
        if (raw.empty()) die("school has no law record");
        size_t pos = 0;
        uint64_t lineno = 0;
        while (pos < raw.size()) {
            size_t nl = raw.find('\n', pos);
            if (nl == std::string::npos)
                die("school line " + std::to_string(lineno + 1) + " is unsealed");
            std::string line = raw.substr(pos, nl - pos);
            if (line.size() + 1 > LEDGER_RECORD_MAX)
                die("school line " + std::to_string(lineno + 1) +
                    " exceeds 4096-byte law");
            pos = nl + 1;
            lineno++;
            size_t tab = line.rfind('\t');
            if (tab == std::string::npos || line.size() - tab - 1 != 16)
                die("school line " + std::to_string(lineno) + " has no chain field");
            uint64_t claimed;
            if (!canon_hex16(line.substr(tab + 1), &claimed))
                die("school line " + std::to_string(lineno) + " chain is not hex16");
            std::string payload = line.substr(0, tab);
            uint64_t next = fnv64(payload.data(), payload.size(), chain);
            if (next != claimed)
                die("school chain broken at line " + std::to_string(lineno));
            chain = next;
            records++;
            replay(payload, lineno);
            prefixes.insert({records, chain});
            if (limit && records == limit) return;
        }
        if (limit && records != limit)
            die("school prefix ends before requested record count");
        if (!law_seen) die("school has no law record");
    }

    void replay(const std::string &payload, uint64_t lineno) {
        auto f = split_tabs(payload);
        const std::string where = "school line " + std::to_string(lineno);
        if (f.empty()) die(where + " is empty");
        if (f[0] == "S") {
            if (lineno != 1 || f.size() != 3 || f[1] != "1" || f[2] != SCHOOL_LAW)
                die(where + ": school law record");
            law_seen = true;
            return;
        }
        if (!law_seen) die(where + ": record before school law");
        if (pending_glyph && f[0] != "L")
            die(where + ": passed hypothesis must be legalised immediately");
        if (f[0] == "H") {
            if (f.size() != 9) die(where + ": H arity");
            Hyp h;
            uint64_t slen;
            if (!canon_u64(f[1], &h.id) || h.id != hyps.size() + 1 ||
                !canon_u64(f[2], &h.arity) || !canon_u64(f[3], &slen) ||
                f[4].size() != slen || !canon_u64(f[5], &h.after))
                die(where + ": H field grammar");
            uint64_t mainchain;
            if (!canon_hex16(f[6], &mainchain) ||
                !m.prefixes.count({h.after, mainchain}))
                die(where + ": H main prefix");
            if (h.after > UINT64_MAX - W_EXAM)
                die(where + ": H window overflows");
            if (f[7] != std::to_string(W_EXAM) || f[8] != BASELINE_LAW)
                die(where + ": H window or baseline law");
            std::vector<std::string> toks;
            if (!shape_canonical(f[4], h.arity, &toks))
                die(where + ": H shape is not canonical for its arity");
            h.shape = f[4];
            if (open_by_shape(h.shape)) die(where + ": H shape already open");
            if (is_legalised(h.shape)) die(where + ": H shape already legalised");
            if (!proposal_alive_at(m, h.after, mainchain, h.shape))
                die(where + ": H shape was not an alive proposal at its prefix");
            for (const auto &u : legalised) h.base_units.push_back(split_ws(u));
            hyps.push_back(std::move(h));
        } else if (f[0] == "R") {
            uint64_t slen, after, mainchain;
            if (f.size() != 6 ||
                (f[1] != "not-proposed" && f[1] != "already-enrolled" &&
                 f[1] != "already-legalised") ||
                !canon_u64(f[2], &slen) || f[3].size() != slen ||
                !canon_u64(f[4], &after) || !canon_hex16(f[5], &mainchain))
                die(where + ": R field grammar");
            if (!m.prefixes.count({after, mainchain}))
                die(where + ": R main prefix");
            std::vector<std::string> toks;
            if (!shape_canonical(f[3], 2, &toks) && !shape_canonical(f[3], 3, &toks))
                die(where + ": R shape is not canonical");
            bool open = open_by_shape(f[3]) != nullptr;
            bool legal = is_legalised(f[3]);
            bool alive = proposal_alive_at(m, after, mainchain, f[3]);
            if ((f[1] == "already-legalised" && !legal) ||
                (f[1] == "already-enrolled" && (legal || !open)) ||
                (f[1] == "not-proposed" && (legal || open || alive)))
                die(where + ": R reason does not match its pinned state");
            r_count++;
        } else if (f[0] == "O") {
            uint64_t id, meal, nf, base_mb, cand_mb;
            if (f.size() != 6 || !canon_u64(f[1], &id) || !canon_u64(f[2], &meal) ||
                !canon_u64(f[3], &nf) || !canon_u64(f[4], &base_mb) ||
                !canon_u64(f[5], &cand_mb))
                die(where + ": O field grammar");
            if (id == 0 || id > hyps.size()) die(where + ": O names no hypothesis");
            Hyp &h = hyps[id - 1];
            uint64_t expect = h.last_meal ? h.last_meal + 1 : h.after + 1;
            if (h.verdicted || meal != expect || meal > h.after + W_EXAM)
                die(where + ": O outside the window's order");
            if (meal > m.meals) die(where + ": O prices an unarrived meal");
            uint64_t rf, rb, rc;
            price_meal(h, meal, &rf, &rb, &rc);
            if (rf != nf || rb != base_mb || rc != cand_mb)
                die(where + ": observation drifted from the field");
            h.last_meal = meal;
            if (UINT64_MAX - h.base_total < base_mb ||
                UINT64_MAX - h.cand_total < cand_mb)
                die(where + ": O totals overflow");
            h.base_total += base_mb;
            h.cand_total += cand_mb;
            o_count++;
        } else if (f[0] == "V") {
            uint64_t id, bt, ct;
            if (f.size() != 5 || !canon_u64(f[1], &id) || !canon_u64(f[3], &bt) ||
                !canon_u64(f[4], &ct))
                die(where + ": V field grammar");
            if (id == 0 || id > hyps.size()) die(where + ": V names no hypothesis");
            Hyp &h = hyps[id - 1];
            if (h.verdicted || h.last_meal != h.after + W_EXAM)
                die(where + ": V before the window closed");
            if (bt != h.base_total || ct != h.cand_total)
                die(where + ": V totals do not match the observations");
            if (f[2] != verdict_word(bt, ct)) die(where + ": V verdict class");
            h.verdicted = true;
            h.passed = f[2] == "pass";
            if (h.passed) pending_glyph = h.id;
            v_count++;
        } else if (f[0] == "L") {
            uint64_t id, glyph;
            if (f.size() != 3 || !canon_u64(f[1], &id) || !canon_u64(f[2], &glyph))
                die(where + ": L field grammar");
            if (id == 0 || id > hyps.size()) die(where + ": L names no hypothesis");
            Hyp &h = hyps[id - 1];
            if (!h.verdicted || !h.passed || pending_glyph != id ||
                is_legalised(h.shape) ||
                glyph != legalised.size() + 1)
                die(where + ": L without a pass or out of order");
            legalised.push_back(h.shape);
            pending_glyph = 0;
        } else {
            die(where + ": unknown school record type");
        }
    }

    static const char *verdict_word(uint64_t base_total, uint64_t cand_total) {
        if (base_total >= cand_total && base_total - cand_total >= GAIN_MIN)
            return "pass";
        if (base_total > cand_total) return "weak";
        return "fail";
    }

    void ensure_law() {
        if (law_seen) return;
        if (records != 0) die("cannot start a nonempty school");
        append(std::string("S\t1\t") + SCHOOL_LAW);
        law_seen = true;
    }

    bool recover_pending_glyph() {
        if (!pending_glyph) return false;
        Hyp &h = hyps[pending_glyph - 1];
        uint64_t glyph = legalised.size() + 1;
        append("L\t" + std::to_string(h.id) + "\t" + std::to_string(glyph));
        legalised.push_back(h.shape);
        pending_glyph = 0;
        printf("  recovered glyph %llu for hyp %llu after a V-boundary prefix\n",
               (unsigned long long)glyph, (unsigned long long)h.id);
        return true;
    }

    void append(const std::string &payload) {
        if (payload.size() + 18 > LEDGER_RECORD_MAX)
            die("school record exceeds 4096-byte law");
        chain = fnv64(payload.data(), payload.size(), chain);
        std::string line = payload + "\t" + hex16(chain) + "\n";
        int fd = open(SCHOOL_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) die("cannot open school for append");
        ssize_t w = write(fd, line.data(), line.size());
        if (w < 0 || (size_t)w != line.size() || close(fd) != 0)
            die("school append failed");
        records++;
    }
};

/* ---- resonate: kk's X.Wr under a per-source cap, ported */

struct Scored { double score; const Frag *frag; };

static std::vector<Scored> resonate(const std::vector<Frag> &frags,
                                    const std::vector<double> &field,
                                    int k, int per_source_cap,
                                    const Frag *exclude = nullptr,
                                    const std::string *exclude_clause = nullptr) {
    std::vector<Scored> scored;
    scored.reserve(frags.size());
    for (const auto &f : frags) {
        if (&f == exclude) continue;
        if (exclude_clause && cut_clause(f.text) == *exclude_clause) continue;
        scored.push_back({cosine(field, f.emb), &f});
    }
    std::stable_sort(scored.begin(), scored.end(),
                     [](const Scored &a, const Scored &b) { return a.score > b.score; });
    std::vector<Scored> out;
    std::unordered_map<std::string, int> seen;
    for (const auto &s : scored) {
        if (seen[s.frag->source] >= per_source_cap) continue;
        seen[s.frag->source]++;
        out.push_back(s);
        if ((int)out.size() >= k) break;
    }
    return out;
}

/* ---- commands */

static void cmd_ingest(Mycelium &m, const std::string &label,
                       const std::string &speech_path, const std::string &bio_path) {
    if (!label_ok(label))
        die("label must be 1..32 of [a-z0-9_-]");
    std::string speech;
    if (!read_file(speech_path, speech)) die("cannot open speech " + speech_path);
    if (speech.empty()) die("speech " + speech_path + " is empty");
    uint64_t sdig = fnv64(speech.data(), speech.size(), FNV_SEED);
    std::string bio;
    if (!read_file(bio_path, bio)) die("cannot open biography " + bio_path);

    /* Find the first UNEATEN canonical s event whose candidate digest and
       byte count match. The attestation prefix runs through its newline;
       an unsealed final row is not a biography event. */
    uint64_t ordinal = 0, prior = 0;
    std::string s_line;
    bool found = false, matched_eaten = false;
    uint64_t eaten_ordinal = 0, eaten_prior = 0;
    size_t pos = 0;
    uint64_t lineno = 0;
    while (pos < bio.size() && !found) {
        size_t nl = bio.find('\n', pos);
        size_t end = (nl == std::string::npos) ? bio.size() : nl;
        std::string line = bio.substr(pos, end - pos);
        lineno++;
        if (nl != std::string::npos && line.size() <= 1023) {
            uint64_t cand, bytes;
            if (parse_s_event(line, &cand, &bytes) && cand == sdig &&
                bytes == speech.size()) {
                size_t prefix_end = nl + 1;
                uint64_t candidate_prior = fnv64(bio.data(), prefix_end, FNV_SEED);
                std::string candidate_key =
                    Mycelium::make_key(candidate_prior, lineno, line);
                if (!m.keys.count(candidate_key)) {
                    found = true;
                    ordinal = lineno;
                    s_line = line;
                    prior = candidate_prior;
                } else if (!matched_eaten) {
                    matched_eaten = true;
                    eaten_ordinal = lineno;
                    eaten_prior = candidate_prior;
                }
            }
        }
        if (nl == std::string::npos) break;
        pos = nl + 1;
    }
    if (!found && matched_eaten)
        die("already eaten witness: s#" + std::to_string(eaten_ordinal) +
            " of biography prefix " + hex16(eaten_prior));
    if (!found)
        die("speech " + speech_path + " (digest " + hex16(sdig) +
            ") matches no canonical newline-sealed s event in " + bio_path +
            "; only attested speech is food");

    std::string key = Mycelium::make_key(prior, ordinal, s_line);

    /* blob first (temp then rename), ledger second */
    struct stat st;
    if (stat(GRAVE_DIR, &st) != 0) {
        if (mkdir(GRAVE_DIR, 0755) != 0) die("cannot open the grave");
    } else if (!S_ISDIR(st.st_mode))
        die("the grave is not a directory");
    std::string blob_path = std::string(GRAVE_DIR) + "/" + hex16(sdig);
    std::string existing;
    if (read_file(blob_path, existing)) {
        if (existing != speech) die("grave blob " + hex16(sdig) + " digest mismatch");
    } else {
        std::string tmp = blob_path + ".tmp";
        FILE *f = fopen(tmp.c_str(), "wb");
        if (!f) die("cannot write grave blob");
        if (fwrite(speech.data(), 1, speech.size(), f) != speech.size() ||
            fflush(f) != 0 || fclose(f) != 0)
            die("cannot write grave blob");
        if (rename(tmp.c_str(), blob_path.c_str()) != 0) die("cannot seal grave blob");
    }

    GraveEvent g;
    g.label = label;
    g.prior_digest = prior;
    g.ordinal = ordinal;
    g.speech_digest = sdig;
    g.speech_bytes = speech.size();
    g.s_line = s_line;
    m.ensure_version();
    std::string gp = "G\t" + label + "\t" + hex16(prior) + "\t" + std::to_string(ordinal) +
                     "\t" + hex16(sdig) + "\t" + std::to_string(speech.size()) + "\t" +
                     std::to_string(s_line.size()) + "\t" + s_line;
    m.led.append(gp);
    m.keys.insert(key);
    m.blobs_seen.insert(hex16(sdig));

    uint64_t nf = 0, nt = 0;
    m.add_fragments(g, speech, &nf, &nt);

    printf("ate %s s#%llu: %llu fragments, %llu tokens (grave %s)\n",
           label.c_str(), (unsigned long long)ordinal, (unsigned long long)nf,
           (unsigned long long)nt, hex16(sdig).c_str());
    printf("field: %zu fragments from %zu sources | ledger %llu records chain %s\n",
           m.frags.size(), m.corpora.size(),
           (unsigned long long)m.led.records, hex16(m.led.chain).c_str());
}

static void cmd_unfold(Mycelium &m, const std::string &prompt) {
    if (m.frags.empty()) die("the mycelium is empty; ingest something first");
    auto core = tokenize(prompt);
    auto field = mean_embed(core);
    printf("  prompt : \"");
    write_bytes(stdout, prompt);
    printf("\"\n");
    printf("  field  :");
    if (core.empty()) printf(" (none)");
    for (const auto &t : core) { putchar(' '); write_bytes(stdout, t); }
    printf("\n");
    auto res = resonate(m.frags, field, 9, 3);
    printf("\n  -- CORPSE (spliced from %zu sources by resonance) --\n", m.corpora.size());
    std::string corpse;
    std::set<std::string> touched;
    std::vector<std::pair<std::string, const Scored *>> cuts;
    for (const auto &s : res) {
        touched.insert(s.frag->source);
        cuts.push_back({cut_clause(s.frag->text), &s});
    }
    for (size_t i = 0; i < cuts.size(); ++i) {
        if (i) corpse += ".  ";
        corpse += cuts[i].first;
    }
    corpse += ".";
    printf("  ");
    write_bytes(stdout, corpse);
    printf("\n");
    printf("\n  -- lineage (which source each clause came from) --\n");
    for (const auto &c : cuts) {
        printf("    [");
        write_bytes(stdout, c.second->frag->sref);
        printf(" r=%.3f]  ", c.second->score);
        write_bytes(stdout, c.first, 60);
        putchar('\n');
    }
    printf("\n  -> the corpse drew on %zu/%zu sources (", touched.size(), m.corpora.size());
    size_t i = 0;
    for (const auto &t : touched) printf("%s%s", i++ ? ", " : "", t.c_str());
    printf(")\n");

    uint64_t pdig = fnv64(prompt.data(), prompt.size(), FNV_SEED);
    uint64_t cdig = fnv64(corpse.data(), corpse.size(), FNV_SEED);
    std::string csv;
    i = 0;
    for (const auto &t : touched) { if (i++) csv += ","; csv += t; }
    std::string up = "U\t" + hex16(pdig) + "\t" + hex16(cdig) + "\t9\t" +
                     std::to_string(touched.size()) + "\t" + csv;
    m.led.append(up);
    printf("  receipt: prompt %s corpse %s | ledger %llu records chain %s\n",
           hex16(pdig).c_str(), hex16(cdig).c_str(),
           (unsigned long long)m.led.records, hex16(m.led.chain).c_str());
}

static void cmd_field(Mycelium &m) {
    printf("sources: %zu (", m.corpora.size());
    for (size_t i = 0; i < m.corpora.size(); ++i)
        printf("%s%s", i ? ", " : "", m.corpora[i].c_str());
    printf(")\n");
    printf("frags  : %zu\n", m.frags.size());
    printf("eaten  : %zu attested witnesses\n", m.keys.size());
    printf("ledger : %llu records chain %s\n",
           (unsigned long long)m.led.records, hex16(m.led.chain).c_str());
}

/* ablate: the mechanism's own trial, read-only. For every fragment, its
   clause becomes a prompt and every prompt-equivalent fragment is excluded;
   real embeddings answer, then a control where every embedding -- prompt
   and fragment alike -- is the whole-string hash vector. Metric A: top-1
   comes from the origin's source. Metric B: mean top-k score. The sealed
   prediction: real beats control on both, or resonance failed vivisection. */

static std::vector<double> hash_unit_vec(const std::string &s) {
    uint32_t h = 2166136261u;
    for (unsigned char c : s) { h ^= c; h *= 16777619u; }
    std::vector<double> v(DIM);
    double norm = 0.0;
    for (int i = 0; i < DIM; ++i) {
        h ^= h >> 13; h *= 1597334677u; h ^= h >> 16;
        v[i] = (double)(h & 0xFFFF) / 32768.0 - 1.0;
        norm += v[i] * v[i];
    }
    norm = std::sqrt(norm) + 1e-10;
    for (double &x : v) x /= norm;
    return v;
}

static void cmd_ablate(Mycelium &m) {
    if (m.frags.size() < 2) die("the ablation needs a field of at least two fragments");
    size_t hitsA_real = 0, hitsA_ctrl = 0, prompts = 0;
    double sumB_real = 0.0, sumB_ctrl = 0.0;
    std::vector<Frag> ctrl = m.frags;
    for (auto &f : ctrl) f.emb = hash_unit_vec(f.text);
    for (size_t i = 0; i < m.frags.size(); ++i) {
        std::string prompt = cut_clause(m.frags[i].text);
        auto core = tokenize(prompt);
        if (core.size() < 2) continue;
        prompts++;
        auto res = resonate(m.frags, mean_embed(core), 9, 3, &m.frags[i], &prompt);
        if (!res.empty() && res[0].frag->source == m.frags[i].source) hitsA_real++;
        double b = 0.0;
        for (const auto &s : res) b += s.score;
        if (!res.empty()) sumB_real += b / (double)res.size();
        auto cres = resonate(ctrl, hash_unit_vec(prompt), 9, 3, &ctrl[i], &prompt);
        if (!cres.empty() && cres[0].frag->source == m.frags[i].source) hitsA_ctrl++;
        b = 0.0;
        for (const auto &s : cres) b += s.score;
        if (!cres.empty()) sumB_ctrl += b / (double)cres.size();
    }
    if (!prompts) die("the ablation found no usable prompts");
    double A_real = (double)hitsA_real / (double)prompts;
    double A_ctrl = (double)hitsA_ctrl / (double)prompts;
    double B_real = sumB_real / (double)prompts;
    double B_ctrl = sumB_ctrl / (double)prompts;
    int pass = A_real > A_ctrl && B_real > B_ctrl;
    printf("ablation over %zu prompts:\n", prompts);
    printf("  real    A=%zu/%zu (%.3f)  B=%.4f\n", hitsA_real, prompts, A_real, B_real);
    printf("  control A=%zu/%zu (%.3f)  B=%.4f\n", hitsA_ctrl, prompts, A_ctrl, B_ctrl);
    printf("  verdict: %s\n", pass ? "resonance beats the hash control on A and B"
                                   : "ABLATION FAILED: the organ does not beat its absence");
    if (!pass) exit(1);
}

/* body 2: the proposer. It NOTICES recurring shapes -- adjacent token
   pairs and triples inside one fragment -- supported by at least two
   distinct meals, prices their rent over a sixteen-meal window, and
   seals one receipt per run. It grants nothing: the field, the mouthly
   organs and the main ledger are untouched by its existence. */
static void cmd_propose(Mycelium &m) {
    if (m.meals == 0) die("the mycelium is empty; ingest something first");
    Props pr;
    pr.load(m);
    ProposalSnapshot snap = proposal_snapshot(m, m.meals, m.led.chain);
    write_bytes(stdout, snap.block);
    if (!pr.law_seen) {
        pr.append(std::string("W\t1\t") + PROPS_LAW);
        pr.law_seen = true;
    }
    std::string payload = "P\t" + std::to_string(m.meals) + "\t" +
                          hex16(m.led.chain) + "\t" + std::to_string(snap.alive) +
                          "\t" + std::to_string(snap.dead) + "\t" +
                          hex16(snap.digest);
    pr.append(payload);
    printf("  receipt: snapshot %s | proposals %llu records chain %s\n",
           hex16(snap.digest).c_str(), (unsigned long long)pr.records,
           hex16(pr.chain).c_str());
}

[[noreturn]] static void enroll_refuse(School &sc, const char *reason,
                                       const std::string &shape) {
    sc.ensure_law();
    sc.append(std::string("R\t") + reason + "\t" +
              std::to_string(shape.size()) + "\t" + shape + "\t" +
              std::to_string(sc.m.meals) + "\t" + hex16(sc.m.led.chain));
    die(std::string("enrollment refused (") + reason + "): " + shape);
}

static void cmd_enroll(Mycelium &m, const std::string &arity_str,
                       const std::vector<std::string> &tokens) {
    uint64_t arity;
    if (!canon_u64(arity_str, &arity) || (arity != 2 && arity != 3) ||
        tokens.size() != arity)
        die("enroll takes an arity of 2 or 3 and exactly that many tokens");
    std::string shape;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i) shape += ' ';
        shape += tokens[i];
    }
    std::vector<std::string> toks;
    if (!shape_canonical(shape, arity, &toks))
        die("enroll tokens are not canonical");
    Props pr;
    pr.load(m); /* body 3 is where historical P snapshots are re-derived */
    School sc(m);
    sc.load();
    sc.recover_pending_glyph();
    if (sc.is_legalised(shape)) enroll_refuse(sc, "already-legalised", shape);
    if (sc.open_by_shape(shape)) enroll_refuse(sc, "already-enrolled", shape);
    if (!proposal_alive_at(m, m.meals, m.led.chain, shape))
        enroll_refuse(sc, "not-proposed", shape);
    sc.ensure_law();
    uint64_t id = sc.hyps.size() + 1;
    sc.append("H\t" + std::to_string(id) + "\t" + std::to_string(arity) + "\t" +
              std::to_string(shape.size()) + "\t" + shape + "\t" +
              std::to_string(m.meals) + "\t" + hex16(m.led.chain) + "\t" +
              std::to_string(W_EXAM) + "\t" + BASELINE_LAW);
    printf("enrolled hyp %llu arity %llu \"", (unsigned long long)id,
           (unsigned long long)arity);
    write_bytes(stdout, shape);
    printf("\" after meal %llu (window %llu)\n", (unsigned long long)m.meals,
           (unsigned long long)W_EXAM);
    printf("  receipt: school %llu records chain %s\n",
           (unsigned long long)sc.records, hex16(sc.chain).c_str());
}

static bool decode_hex_token(const std::string &hex, std::string *out) {
    if (hex.empty() || hex.size() % 2) return false;
    out->clear();
    out->reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = hex[i] >= '0' && hex[i] <= '9' ? hex[i] - '0' :
                 hex[i] >= 'a' && hex[i] <= 'f' ? hex[i] - 'a' + 10 : -1;
        int lo = hex[i + 1] >= '0' && hex[i + 1] <= '9' ? hex[i + 1] - '0' :
                 hex[i + 1] >= 'a' && hex[i + 1] <= 'f' ? hex[i + 1] - 'a' + 10 : -1;
        if (hi < 0 || lo < 0) return false;
        unsigned char byte = (unsigned char)((hi << 4) | lo);
        if (byte_space(byte)) return false;
        out->push_back((char)byte);
    }
    return true;
}

static void cmd_enroll_hex(Mycelium &m, const std::string &arity_str,
                           const std::vector<std::string> &encoded) {
    std::vector<std::string> tokens;
    tokens.reserve(encoded.size());
    for (const auto &hex : encoded) {
        std::string token;
        if (!decode_hex_token(hex, &token))
            die("enroll-hex tokens must be nonempty lowercase even hex without byte-space");
        tokens.push_back(std::move(token));
    }
    cmd_enroll(m, arity_str, tokens);
}

static void cmd_examine(Mycelium &m) {
    School sc(m);
    sc.load();
    if (!sc.law_seen) die("the school is empty; enroll something first");
    uint64_t sealed = sc.recover_pending_glyph() ? 1 : 0;
    for (auto &h : sc.hyps) {
        if (h.verdicted) continue;
        uint64_t meal = h.last_meal ? h.last_meal + 1 : h.after + 1;
        uint64_t stop = h.after + W_EXAM;
        if (stop > m.meals) stop = m.meals;
        for (; meal <= stop; ++meal) {
            uint64_t nf, base_mb, cand_mb;
            sc.price_meal(h, meal, &nf, &base_mb, &cand_mb);
            if (UINT64_MAX - h.base_total < base_mb ||
                UINT64_MAX - h.cand_total < cand_mb)
                die("school observation totals overflow");
            sc.append("O\t" + std::to_string(h.id) + "\t" + std::to_string(meal) +
                      "\t" + std::to_string(nf) + "\t" + std::to_string(base_mb) +
                      "\t" + std::to_string(cand_mb));
            h.last_meal = meal;
            h.base_total += base_mb;
            h.cand_total += cand_mb;
            sealed++;
            printf("  O hyp %llu meal %llu frags %llu base %llu cand %llu\n",
                   (unsigned long long)h.id, (unsigned long long)meal,
                   (unsigned long long)nf, (unsigned long long)base_mb,
                   (unsigned long long)cand_mb);
        }
        if (h.last_meal == h.after + W_EXAM) {
            const char *verdict = School::verdict_word(h.base_total, h.cand_total);
            sc.append("V\t" + std::to_string(h.id) + "\t" + verdict + "\t" +
                      std::to_string(h.base_total) + "\t" +
                      std::to_string(h.cand_total));
            h.verdicted = true;
            h.passed = verdict == std::string("pass");
            sealed++;
            printf("  verdict: hyp %llu %s (base %llu cand %llu)\n",
                   (unsigned long long)h.id, verdict,
                   (unsigned long long)h.base_total,
                   (unsigned long long)h.cand_total);
            if (h.passed) {
                uint64_t glyph = sc.legalised.size() + 1;
                sc.append("L\t" + std::to_string(h.id) + "\t" +
                          std::to_string(glyph));
                sc.legalised.push_back(h.shape);
                sealed++;
                printf("  glyph %llu minted for hyp %llu\n",
                       (unsigned long long)glyph, (unsigned long long)h.id);
            }
        }
    }
    if (!sealed) printf("nothing to examine\n");
    printf("  receipt: school %llu records chain %s\n",
           (unsigned long long)sc.records, hex16(sc.chain).c_str());
}

/* body 4: the parliament. It admits proven citizens toward a power it
   does not itself hold: one new chain, three old ones read-only, no
   field authority, no weight. Law: MYCELIUM.md body 4 contract as
   repaired by the second hand (body4-parl-v2). */

static uint64_t unit_id_v1(uint64_t arity, const std::string &shape) {
    uint8_t head[2] = {(uint8_t)arity, 0x00};
    uint64_t h = fnv64(head, 2, FNV_SEED);
    return fnv64(shape.data(), shape.size(), h);
}

struct Ballot {
    uint64_t pid = 0;
    uint64_t glyph = 0;
    bool unheard = false;          /* '-' identity: missing glyph lane */
    std::string version;           /* '-' when unheard */
    std::string unit_hex;          /* '-' when unheard */
    uint64_t school_records = 0, school_chain = 0;
    uint64_t after_meals = 0, main_chain = 0;
    uint64_t prop_records = 0, prop_chain = 0;
    std::vector<std::pair<std::string, std::string>> jrows;
    bool has_v = false;
    std::string verdict;
};

struct Findings {
    std::string exam, record, rent;
    uint64_t closing = 0; /* latest adverse closing meal when scarred */
};

struct Parliament {
    const Mycelium &m;
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;
    bool law_seen = false;
    std::vector<Ballot> ballots;
    std::set<std::pair<uint64_t, uint64_t>> prefixes; /* (records, chain) */
    struct Admission {
        uint64_t at_records;
        std::string version, unit_hex, verdict;
    };
    std::vector<Admission> admissions;

    /* is this identity admitted (PASS/WEAKEN) by the pinned prefix? */
    const Admission *citizen_at(uint64_t records, const std::string &version,
                                const std::string &unit_hex) const {
        for (const auto &a : admissions)
            if (a.at_records <= records && a.version == version &&
                a.unit_hex == unit_hex)
                return &a;
        return nullptr;
    }

    explicit Parliament(const Mycelium &myc) : m(myc) {}

    void append(const std::string &payload) {
        if (payload.size() + 18 > LEDGER_RECORD_MAX)
            die("parliament record exceeds 4096-byte law");
        chain = fnv64(payload.data(), payload.size(), chain);
        std::string line = payload + "\t" + hex16(chain) + "\n";
        int fd = open(PARL_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) die("cannot open parliament for append");
        ssize_t w = write(fd, line.data(), line.size());
        if (w < 0 || (size_t)w != line.size() || close(fd) != 0)
            die("parliament append failed");
        records++;
    }

    void ensure_law() {
        if (law_seen) return;
        if (records != 0) die("cannot start a nonempty parliament");
        append(std::string("Q\t1\t") + PARL_LAW);
        law_seen = true;
    }

    /* the three known-lane jurors, computed from the PINNED prefixes:
       school truth at school_records, rent at after_meals. */
    Findings compute_findings(const Ballot &b) const {
        Findings f;
        School sc(m);
        sc.load(b.school_records);
        if (sc.chain != b.school_chain)
            die("ballot " + std::to_string(b.pid) +
                " pins a school prefix that does not reproduce");
        if (b.glyph == 0 || b.glyph > sc.legalised.size())
            die("ballot " + std::to_string(b.pid) +
                " names a glyph absent from its pinned school prefix");
        const std::string &shape = sc.legalised[b.glyph - 1];
        uint64_t arity = split_ws(shape).size();
        if (unit_id_v1(arity, shape) != strtoull(b.unit_hex.c_str(), nullptr, 16))
            die("ballot " + std::to_string(b.pid) + " unit id does not reproduce");
        const Hyp *passed = nullptr;
        uint64_t uid = unit_id_v1(arity, shape);
        bool adverse = false;
        uint64_t closing = 0;
        for (const auto &h : sc.hyps) {
            uint64_t ha = split_ws(h.shape).size();
            if (unit_id_v1(ha, h.shape) != uid) continue;
            if (h.verdicted && h.passed && h.shape == shape) passed = &h;
            if (h.verdicted && !h.passed) {
                adverse = true;
                uint64_t c = h.after + W_EXAM;
                if (c > closing) closing = c;
            }
        }
        if (!passed)
            die("ballot " + std::to_string(b.pid) +
                " glyph has no passed hypothesis in its pinned prefix");
        uint64_t gain = passed->base_total - passed->cand_total;
        f.exam = gain >= 2 * GAIN_MIN ? "strong" : "adequate";
        if (adverse) {
            f.record = "scarred:" + std::to_string(closing);
            f.closing = closing;
        } else {
            f.record = "clean";
        }
        ProposalSnapshot snap = proposal_snapshot(m, b.after_meals, b.main_chain);
        bool alive = false;
        for (const auto &st : snap.states)
            if (st.shape == shape && st.alive) alive = true;
        f.rent = alive ? "alive" : "starved";
        return f;
    }

    std::string verdict_of(const Ballot &b, const Findings &f) const {
        if (b.unheard) return "SILENCE";
        if (b.version != RECOGNIZER_V1) return "DARK";
        if (f.closing && b.after_meals < f.closing + SCAR_WINDOW) return "SCAR";
        if (f.rent == "starved") return "FREEZE";
        if (f.exam == "strong" && f.record == "clean" && f.rent == "alive")
            return "PASS";
        return "WEAKEN";
    }

    /* verify a complete ballot against its own pinned truth */
    void verify_ballot(const Ballot &b) const {
        static const char *jur[3] = {"exam", "record", "rent"};
        if (b.jrows.size() != 3) die("ballot has a wrong juror count");
        for (int i = 0; i < 3; ++i)
            if (b.jrows[i].first != jur[i])
                die("ballot " + std::to_string(b.pid) + " jurors out of order");
        if (b.unheard) {
            School sc(m);
            sc.load(b.school_records);
            if (sc.chain != b.school_chain)
                die("ballot " + std::to_string(b.pid) +
                    " pins a school prefix that does not reproduce");
            if (b.glyph != 0 && b.glyph <= sc.legalised.size())
                die("ballot " + std::to_string(b.pid) +
                    " claims silence for a glyph its prefix holds");
            for (const auto &j : b.jrows)
                if (j.second != "unheard")
                    die("ballot " + std::to_string(b.pid) + " false finding");
            if (b.verdict != "SILENCE")
                die("ballot " + std::to_string(b.pid) + " false verdict");
            return;
        }
        if (b.version != RECOGNIZER_V1) {
            School sc(m);
            sc.load(b.school_records);
            if (sc.chain != b.school_chain)
                die("ballot " + std::to_string(b.pid) +
                    " pins a school prefix that does not reproduce");
            if (b.glyph == 0 || b.glyph > sc.legalised.size())
                die("ballot " + std::to_string(b.pid) +
                    " names a glyph absent from its pinned school prefix");
            for (const auto &j : b.jrows)
                if (j.second != "opaque")
                    die("ballot " + std::to_string(b.pid) + " false finding");
            if (b.verdict != "DARK")
                die("ballot " + std::to_string(b.pid) + " false verdict");
            return;
        }
        Findings f = compute_findings(b);
        if (b.jrows[0].second != f.exam || b.jrows[1].second != f.record ||
            b.jrows[2].second != f.rent)
            die("ballot " + std::to_string(b.pid) + " false finding");
        if (b.verdict != verdict_of(b, f))
            die("ballot " + std::to_string(b.pid) + " false verdict");
    }

    void load(const Props &pr) {
        std::string raw;
        if (!read_file(PARL_PATH, raw)) {
            struct stat st;
            if (stat(PARL_PATH, &st) == 0) die("cannot read parliament");
            return;
        }
        if (raw.empty()) die("parliament has no law record");
        size_t pos = 0;
        uint64_t lineno = 0;
        while (pos < raw.size()) {
            size_t nl = raw.find('\n', pos);
            if (nl == std::string::npos)
                die("parliament line " + std::to_string(lineno + 1) +
                    " is unsealed");
            std::string line = raw.substr(pos, nl - pos);
            if (line.size() + 1 > LEDGER_RECORD_MAX)
                die("parliament line " + std::to_string(lineno + 1) +
                    " exceeds 4096-byte law");
            pos = nl + 1;
            lineno++;
            size_t tab = line.rfind('\t');
            if (tab == std::string::npos || line.size() - tab - 1 != 16)
                die("parliament line " + std::to_string(lineno) +
                    " has no chain field");
            uint64_t claimed;
            if (!canon_hex16(line.substr(tab + 1), &claimed))
                die("parliament line " + std::to_string(lineno) +
                    " chain is not hex16");
            std::string payload = line.substr(0, tab);
            uint64_t next = fnv64(payload.data(), payload.size(), chain);
            if (next != claimed)
                die("parliament chain broken at line " + std::to_string(lineno));
            chain = next;
            records++;
            replay(payload, lineno, pr);
            prefixes.insert({records, chain});
        }
        if (!law_seen) die("parliament has no law record");
        /* every ballot but a trailing interrupted one must be complete
           and must reproduce from its pinned truth */
        for (size_t i = 0; i < ballots.size(); ++i) {
            const Ballot &b = ballots[i];
            bool complete = b.has_v && b.jrows.size() == 3;
            if (!complete && i + 1 != ballots.size())
                die("parliament holds an abandoned ballot before its tip");
            if (complete) verify_ballot(b);
        }
    }

    void replay(const std::string &payload, uint64_t lineno, const Props &pr) {
        auto f = split_tabs(payload);
        const std::string where = "parliament line " + std::to_string(lineno);
        if (f.empty()) die(where + " is empty");
        if (f[0] == "Q") {
            if (lineno != 1 || f.size() != 3 || f[1] != "1" || f[2] != PARL_LAW)
                die(where + ": parliament law record");
            law_seen = true;
            return;
        }
        if (!law_seen) die(where + ": record before parliament law");
        if (f[0] == "P") {
            if (!ballots.empty()) {
                const Ballot &prev = ballots.back();
                if (!prev.has_v || prev.jrows.size() != 3)
                    die(where + ": a new petition before the last ballot closed");
            }
            if (f.size() != 11) die(where + ": P arity");
            Ballot b;
            if (!canon_u64(f[1], &b.pid) || b.pid != ballots.size() + 1)
                die(where + ": P petition id is not sequential");
            if (!canon_u64(f[2], &b.glyph) || b.glyph == 0)
                die(where + ": P glyph");
            if (f[3] == "-" && f[4] == "-") {
                b.unheard = true;
                b.unit_hex = "-";
                b.version = "-";
            } else {
                uint64_t uid;
                if (!canon_hex16(f[3], &uid) || !label_ok(f[4]))
                    die(where + ": P identity grammar");
                b.unit_hex = f[3];
                b.version = f[4];
            }
            if (!canon_u64(f[5], &b.school_records) || b.school_records == 0 ||
                !canon_hex16(f[6], &b.school_chain) ||
                !canon_u64(f[7], &b.after_meals) ||
                !canon_hex16(f[8], &b.main_chain) ||
                !canon_u64(f[9], &b.prop_records) ||
                !canon_hex16(f[10], &b.prop_chain))
                die(where + ": P field grammar");
            if (!m.prefixes.count({b.after_meals, b.main_chain}) &&
                !(b.after_meals == 0 && b.main_chain == FNV_SEED))
                die(where + ": P main prefix");
            if (b.prop_records == 0) {
                if (b.prop_chain != FNV_SEED) die(where + ": P proposals prefix");
            } else if (!pr.prefixes.count({b.prop_records, b.prop_chain}))
                die(where + ": P proposals prefix");
            ballots.push_back(std::move(b));
        } else if (f[0] == "J") {
            uint64_t pid;
            if (f.size() != 4 || !canon_u64(f[1], &pid))
                die(where + ": J field grammar");
            if (ballots.empty() || pid != ballots.back().pid)
                die(where + ": J outside its ballot");
            Ballot &b = ballots.back();
            if (b.has_v || b.jrows.size() >= 3)
                die(where + ": J after the ballot closed");
            static const char *jur[3] = {"exam", "record", "rent"};
            if (f[2] != jur[b.jrows.size()])
                die(where + ": J juror out of canonical order");
            if (f[3].empty()) die(where + ": J empty finding");
            b.jrows.push_back({f[2], f[3]});
        } else if (f[0] == "V") {
            uint64_t pid;
            if (f.size() != 3 || !canon_u64(f[1], &pid))
                die(where + ": V field grammar");
            if (ballots.empty() || pid != ballots.back().pid)
                die(where + ": V outside its ballot");
            Ballot &b = ballots.back();
            if (b.has_v || b.jrows.size() != 3)
                die(where + ": V before all three jurors");
            if (f[2] != "PASS" && f[2] != "WEAKEN" && f[2] != "FREEZE" &&
                f[2] != "SCAR" && f[2] != "DARK" && f[2] != "SILENCE")
                die(where + ": V verdict class");
            b.verdict = f[2];
            b.has_v = true;
            if ((b.verdict == "PASS" || b.verdict == "WEAKEN") && !b.unheard)
                admissions.push_back({records, b.version, b.unit_hex, b.verdict});
        } else {
            die(where + ": unknown parliament record type");
        }
    }

    /* complete a trailing interrupted ballot exactly once, from its
       pinned artifacts; growth after those tips cannot alter it */
    void recover() {
        if (ballots.empty()) return;
        Ballot &b = ballots.back();
        if (b.has_v && b.jrows.size() == 3) return;
        static const char *jur[3] = {"exam", "record", "rent"};
        std::string findings[3];
        if (b.unheard) {
            findings[0] = findings[1] = findings[2] = "unheard";
        } else if (b.version != RECOGNIZER_V1) {
            findings[0] = findings[1] = findings[2] = "opaque";
        } else {
            Findings f = compute_findings(b);
            findings[0] = f.exam;
            findings[1] = f.record;
            findings[2] = f.rent;
        }
        for (size_t i = b.jrows.size(); i < 3; ++i) {
            append("J\t" + std::to_string(b.pid) + "\t" + jur[i] + "\t" +
                   findings[i]);
            b.jrows.push_back({jur[i], findings[i]});
        }
        Findings f2;
        if (!b.unheard && b.version == RECOGNIZER_V1) f2 = compute_findings(b);
        b.verdict = verdict_of(b, f2);
        if (b.unheard) b.verdict = "SILENCE";
        else if (b.version != RECOGNIZER_V1) b.verdict = "DARK";
        append("V\t" + std::to_string(b.pid) + "\t" + b.verdict);
        b.has_v = true;
        fprintf(stderr, "parliament: recovered ballot %llu as %s\n",
                (unsigned long long)b.pid, b.verdict.c_str());
    }

    /* re-petition boundaries against the completed record */
    void refuse_by_history(bool opaque_lane, uint64_t glyph,
                           const std::string &version,
                           const std::string &unit_hex,
                           uint64_t glyphs_now) const {
        for (const auto &b : ballots) {
            if (!b.has_v) continue;
            if (b.unheard) {
                if (b.glyph == glyph && glyph > glyphs_now)
                    die("petition refused: glyph " + std::to_string(glyph) +
                        " is still silent");
                continue;
            }
            if (b.version != version || b.unit_hex != unit_hex) continue;
            if (b.verdict == "PASS" || b.verdict == "WEAKEN")
                die("petition refused: identity already admitted as " + b.verdict);
            if (b.verdict == "DARK" && opaque_lane)
                die("petition refused: recognizer " + version +
                    " has no new preregistered law");
            if (b.verdict == "SCAR") {
                uint64_t closing = 0;
                for (const auto &j : b.jrows)
                    if (j.first == "record" &&
                        j.second.rfind("scarred:", 0) == 0)
                        closing = strtoull(j.second.c_str() + 8, nullptr, 10);
                if (m.meals < closing + SCAR_WINDOW)
                    die("petition refused: scar holds until meal " +
                        std::to_string(closing + SCAR_WINDOW));
            }
            if (b.verdict == "FREEZE" &&
                m.meals < b.after_meals + FREEZE_WINDOW)
                die("petition refused: frozen until meal " +
                    std::to_string(b.after_meals + FREEZE_WINDOW));
        }
    }
};

static void seal_ballot(Parliament &pl, Ballot &b, const Findings &f) {
    static const char *jur[3] = {"exam", "record", "rent"};
    std::string findings[3];
    if (b.unheard)
        findings[0] = findings[1] = findings[2] = "unheard";
    else if (b.version != RECOGNIZER_V1)
        findings[0] = findings[1] = findings[2] = "opaque";
    else {
        findings[0] = f.exam;
        findings[1] = f.record;
        findings[2] = f.rent;
    }
    pl.ensure_law();
    pl.append("P\t" + std::to_string(b.pid) + "\t" + std::to_string(b.glyph) +
              "\t" + b.unit_hex + "\t" + b.version + "\t" +
              std::to_string(b.school_records) + "\t" + hex16(b.school_chain) +
              "\t" + std::to_string(b.after_meals) + "\t" + hex16(b.main_chain) +
              "\t" + std::to_string(b.prop_records) + "\t" +
              hex16(b.prop_chain));
    for (int i = 0; i < 3; ++i) {
        pl.append("J\t" + std::to_string(b.pid) + "\t" + jur[i] + "\t" +
                  findings[i]);
        b.jrows.push_back({jur[i], findings[i]});
        printf("  J %s %s\n", jur[i], findings[i].c_str());
    }
    b.verdict = pl.verdict_of(b, f);
    pl.append("V\t" + std::to_string(b.pid) + "\t" + b.verdict);
    b.has_v = true;
    printf("  verdict: %s\n", b.verdict.c_str());
    printf("  receipt: parliament %llu records chain %s\n",
           (unsigned long long)pl.records, hex16(pl.chain).c_str());
}

static void cmd_petition(Mycelium &m, const std::string &glyph_str) {
    uint64_t glyph;
    if (!canon_u64(glyph_str, &glyph) || glyph == 0)
        die("petition takes a glyph ordinal");
    School sc(m);
    sc.load();
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    pl.recover();
    Ballot b;
    b.pid = pl.ballots.size() + 1;
    b.glyph = glyph;
    b.school_records = sc.records;
    b.school_chain = sc.chain;
    b.after_meals = m.meals;
    b.main_chain = m.led.chain;
    b.prop_records = pr.records;
    b.prop_chain = pr.chain;
    Findings f;
    if (glyph > sc.legalised.size()) {
        pl.refuse_by_history(false, glyph, "-", "-", sc.legalised.size());
        b.unheard = true;
        b.unit_hex = "-";
        b.version = "-";
        printf("petition %llu: glyph %llu is not in the school\n",
               (unsigned long long)b.pid, (unsigned long long)glyph);
    } else {
        const std::string &shape = sc.legalised[glyph - 1];
        uint64_t arity = split_ws(shape).size();
        b.unit_hex = hex16(unit_id_v1(arity, shape));
        b.version = RECOGNIZER_V1;
        pl.refuse_by_history(false, glyph, b.version, b.unit_hex,
                             sc.legalised.size());
        pl.ballots.push_back(b);
        f = pl.compute_findings(pl.ballots.back());
        pl.ballots.pop_back();
        printf("petition %llu: glyph %llu unit %s version %s\n",
               (unsigned long long)b.pid, (unsigned long long)glyph,
               b.unit_hex.c_str(), b.version.c_str());
    }
    seal_ballot(pl, b, f);
}

static void cmd_petition_opaque(Mycelium &m, const std::string &glyph_str,
                                const std::string &version,
                                const std::string &unit_hex) {
    uint64_t glyph, uid;
    if (!canon_u64(glyph_str, &glyph) || glyph == 0)
        die("petition-opaque takes a glyph ordinal");
    if (!label_ok(version) || version == RECOGNIZER_V1)
        die("petition-opaque takes a foreign recognizer version");
    if (!canon_hex16(unit_hex, &uid))
        die("petition-opaque takes a hex16 unit id");
    School sc(m);
    sc.load();
    if (glyph > sc.legalised.size())
        die("petition-opaque needs an existing glyph");
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    pl.recover();
    pl.refuse_by_history(true, glyph, version, unit_hex, sc.legalised.size());
    Ballot b;
    b.pid = pl.ballots.size() + 1;
    b.glyph = glyph;
    b.version = version;
    b.unit_hex = unit_hex;
    b.school_records = sc.records;
    b.school_chain = sc.chain;
    b.after_meals = m.meals;
    b.main_chain = m.led.chain;
    b.prop_records = pr.records;
    b.prop_chain = pr.chain;
    printf("petition %llu (opaque): glyph %llu claimed unit %s version %s\n",
           (unsigned long long)b.pid, (unsigned long long)glyph,
           unit_hex.c_str(), version.c_str());
    Findings f;
    seal_ballot(pl, b, f);
}

static void cmd_franchise(Mycelium &m) {
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    pl.recover();
    uint64_t n = 0;
    for (const auto &b : pl.ballots)
        if (b.has_v && (b.verdict == "PASS" || b.verdict == "WEAKEN")) {
            n++;
            printf("citizen %llu: glyph %llu unit %s version %s %s\n",
                   (unsigned long long)n, (unsigned long long)b.glyph,
                   b.unit_hex.c_str(), b.version.c_str(), b.verdict.c_str());
        }
    if (!n) printf("the franchise is empty\n");
    printf("  receipt: parliament %llu records chain %s\n",
           (unsigned long long)pl.records, hex16(pl.chain).c_str());
}

/* body 5: the mint. Notes are tiny mortal nets trained by the linked
   forge on a citizen's lived contexts, masked so the net learns the
   company a shape keeps, never the shape itself. Every gram of weight
   is powerless: bodies 1-4 answer byte-identically with and without
   this chain. Law: MYCELIUM.md body 5 contract as repaired by the
   second hand (body5-notes-v1). */

struct NoteExample {
    std::vector<float> x; /* DIM features, count-normalised bag */
    int label;
    uint64_t key; /* fnv64 over tokens joined by 0x1f */
};

struct NoteDataset {
    std::vector<NoteExample> train, holdout;
    uint64_t positives = 0, negatives = 0;
    uint64_t holdout_pos = 0, holdout_neg = 0;
    bool ok = false;
    std::string refusal;
};

static uint64_t note_frag_key(const std::vector<std::string> &toks) {
    uint64_t h = FNV_SEED;
    for (size_t i = 0; i < toks.size(); ++i) {
        if (i) { uint8_t sep = 0x1f; h = fnv64(&sep, 1, h); }
        h = fnv64(toks[i].data(), toks[i].size(), h);
    }
    return h;
}

/* mark every token position covered by a complete shape occurrence */
static void note_mark_occurrences(const std::vector<std::string> &toks,
                                  const std::vector<std::string> &shape,
                                  std::vector<char> *covered, bool *any) {
    covered->assign(toks.size(), 0);
    *any = false;
    if (shape.empty() || toks.size() < shape.size()) return;
    for (size_t i = 0; i + shape.size() <= toks.size(); ++i) {
        bool hit = true;
        for (size_t k = 0; k < shape.size() && hit; ++k)
            if (toks[i + k] != shape[k]) hit = false;
        if (hit) {
            *any = true;
            for (size_t k = 0; k < shape.size(); ++k) (*covered)[i + k] = 1;
        }
    }
}

/* DIM-96 trigram bag of the SURVIVING tokens, normalised by surviving
   window count; masking = the occurrence's tokens are omitted entirely,
   no sentinel, no splice. Returns window count (0 = no features). */
static uint64_t note_features(const std::vector<std::string> &toks,
                              const std::vector<char> &covered,
                              std::vector<float> *out) {
    std::vector<double> bag(DIM, 0.0);
    uint64_t windows = 0;
    for (size_t t = 0; t < toks.size(); ++t) {
        if (covered[t]) continue;
        std::string w = "^" + toks[t] + "$";
        if (w.size() >= 3) {
            for (size_t i = 0; i + 3 <= w.size(); ++i) {
                const auto &gv = fnv_vec(w.substr(i, 3));
                for (int d = 0; d < DIM; ++d) bag[d] += gv[d];
                windows++;
            }
        } else {
            const auto &gv = fnv_vec(w);
            for (int d = 0; d < DIM; ++d) bag[d] += gv[d];
            windows++;
        }
    }
    out->assign(DIM, 0.0f);
    if (!windows) return 0;
    for (int d = 0; d < DIM; ++d)
        (*out)[d] = (float)(bag[d] / (double)windows);
    return windows;
}

/* the sealed dataset law: positives masked, equal-count negatives by
   hash order, stratified 1-in-4 holdout after hash sorting */
static NoteDataset note_dataset(const Mycelium &m,
                                const std::vector<std::string> &shape,
                                uint64_t after_meals) {
    NoteDataset ds;
    struct Cand { uint64_t key; std::vector<float> x; };
    std::vector<Cand> pos, neg;
    for (const auto &f : m.frags) {
        if (f.meal > after_meals) continue;
        std::vector<char> covered;
        bool any;
        note_mark_occurrences(f.toks, shape, &covered, &any);
        std::vector<float> x;
        if (any) {
            if (!note_features(f.toks, covered, &x)) continue; /* nothing survives */
            pos.push_back({note_frag_key(f.toks), std::move(x)});
        } else {
            note_features(f.toks, covered, &x);
            neg.push_back({note_frag_key(f.toks), std::move(x)});
        }
    }
    auto by_key = [](const Cand &a, const Cand &b) { return a.key < b.key; };
    std::stable_sort(pos.begin(), pos.end(), by_key);
    std::stable_sort(neg.begin(), neg.end(), by_key);
    if (neg.size() < pos.size()) {
        ds.refusal = "insufficient data: " + std::to_string(neg.size()) +
                     " shape-free fragments against " +
                     std::to_string(pos.size()) + " positives";
        return ds;
    }
    neg.resize(pos.size());
    ds.positives = pos.size();
    ds.negatives = neg.size();
    for (int label = 0; label < 2; ++label) {
        const std::vector<Cand> &side = label ? pos : neg;
        for (size_t i = 0; i < side.size(); ++i) {
            NoteExample ex;
            ex.x = side[i].x;
            ex.label = label;
            ex.key = side[i].key;
            if (i % 4 == 0) {
                ds.holdout.push_back(std::move(ex));
                if (label) ds.holdout_pos++; else ds.holdout_neg++;
            } else {
                ds.train.push_back(std::move(ex));
            }
        }
    }
    uint64_t train_pos = ds.positives - ds.holdout_pos;
    uint64_t train_neg = ds.negatives - ds.holdout_neg;
    if (!ds.holdout_pos || !ds.holdout_neg || !train_pos || !train_neg) {
        ds.refusal = "insufficient data: a split lost a whole label";
        return ds;
    }
    ds.ok = true;
    return ds;
}

/* Chuck trains the 97-parameter probe; the gradient of a linear
   sigmoid is exact closed form, the step is the forge's own. */
struct NoteWeights {
    float w[DIM];
    float bias;
    uint64_t loss_microbits;
};

static NoteWeights note_train(const NoteDataset &ds, uint64_t steps,
                              uint64_t seed) {
    nt_seed(seed);
    nt_tape_start();
    nt_tensor *W = nt_tensor_new(DIM);
    nt_tensor *B = nt_tensor_new(1);
    int wi = nt_tape_param(W);
    int bi = nt_tape_param(B);
    nt_tape *tape = nt_tape_get();
    if (!tape->entries[wi].grad) tape->entries[wi].grad = nt_tensor_new(DIM);
    if (!tape->entries[bi].grad) tape->entries[bi].grad = nt_tensor_new(1);
    float *gw = tape->entries[wi].grad->data;
    float *gb = tape->entries[bi].grad->data;
    double loss = 0.0;
    size_t n = ds.train.size();
    for (uint64_t step = 0; step < steps; ++step) {
        for (int d = 0; d < DIM; ++d) gw[d] = 0.0f;
        gb[0] = 0.0f;
        loss = 0.0;
        for (const auto &ex : ds.train) {
            double z = (double)B->data[0];
            for (int d = 0; d < DIM; ++d)
                z += (double)W->data[d] * (double)ex.x[d];
            double p = 1.0 / (1.0 + std::exp(-z));
            if (p < 1e-7) p = 1e-7;
            if (p > 1.0 - 1e-7) p = 1.0 - 1e-7;
            loss += ex.label ? -std::log(p) : -std::log(1.0 - p);
            float g = (float)((p - (double)ex.label) / (double)n);
            for (int d = 0; d < DIM; ++d) gw[d] += g * ex.x[d];
            gb[0] += g;
        }
        loss /= (double)n;
        nt_tape_chuck_step(NOTE_LR, (float)loss);
    }
    NoteWeights out;
    for (int d = 0; d < DIM; ++d) out.w[d] = W->data[d];
    out.bias = B->data[0];
    double final_bits = 0.0;
    for (const auto &ex : ds.train) {
        double z = (double)out.bias;
        for (int d = 0; d < DIM; ++d)
            z += (double)out.w[d] * (double)ex.x[d];
        double p = 1.0 / (1.0 + std::exp(-z));
        if (p < 1e-7) p = 1e-7;
        if (p > 1.0 - 1e-7) p = 1.0 - 1e-7;
        final_bits += ex.label ? -std::log2(p) : -std::log2(1.0 - p);
    }
    final_bits /= (double)n;
    double scaled = final_bits * 1e6;
    out.loss_microbits =
        scaled > 0.0 && scaled < 9e18 ? (uint64_t)llround(scaled) : 0;
    nt_tape_destroy();
    nt_tensor_free(W);
    nt_tensor_free(B);
    return out;
}

static uint64_t note_holdout_hits(const NoteWeights &nw,
                                  const std::vector<NoteExample> &holdout) {
    uint64_t hits = 0;
    for (const auto &ex : holdout) {
        double z = (double)nw.bias;
        for (int d = 0; d < DIM; ++d)
            z += (double)nw.w[d] * (double)ex.x[d];
        int pred = z >= 0.0 ? 1 : 0;
        if (pred == ex.label) hits++;
    }
    return hits;
}

static std::string note_blob_bytes(const NoteWeights &nw) {
    std::string blob(NOTE_BLOB_BYTES, '\0');
    for (int d = 0; d < DIM; ++d) f32_to_le(&blob[(size_t)d * 4], nw.w[d]);
    f32_to_le(&blob[(size_t)DIM * 4], nw.bias);
    return blob;
}

static uint64_t forge_digest(const char *path, bool *present) {
    std::string raw;
    if (!read_file(path, raw)) { *present = false; return 0; }
    *present = true;
    return fnv64(raw.data(), raw.size(), FNV_SEED);
}

static uint64_t note_seed_for(const std::string &version,
                              const std::string &unit_hex) {
    std::string src = std::string("note:") + version + ":" + unit_hex;
    return fnv64(src.data(), src.size(), FNV_SEED);
}

static bool note_shape_alive_at(const Mycelium &m,
                                const std::vector<std::string> &shape,
                                uint64_t after_meals, uint64_t main_chain) {
    std::string joined;
    for (size_t i = 0; i < shape.size(); ++i) {
        if (i) joined += ' ';
        joined += shape[i];
    }
    ProposalSnapshot snap = proposal_snapshot(m, after_meals, main_chain);
    for (const auto &st : snap.states)
        if (st.shape == joined && st.alive) return true;
    return false;
}

struct NoteRec {
    uint64_t id = 0;
    std::string unit_hex, version;
    uint64_t parl_records = 0, parl_chain = 0;
    uint64_t after_meals = 0, main_chain = 0;
    uint64_t forge_h = 0, forge_l = 0, seed = 0;
    struct Training {
        uint64_t after_meals, main_chain, forge_h, forge_l, seed, steps,
            loss_mb, positives, negatives, hpos, hneg, hits, baseline;
        std::string weight_hex;
        bool has_e = false;
        std::string verdict;
    };
    std::vector<Training> ts;
    /* rent state: 1 = alive, 0 = in the morgue */
    int alive = 1;
    uint64_t last_event_meal = 0;
};

struct Notes {
    const Mycelium &m;
    const Parliament &pl;
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;
    bool law_seen = false;
    std::vector<NoteRec> notes;
    uint64_t open_training = 0;

    Notes(const Mycelium &myc, const Parliament &parl) : m(myc), pl(parl) {}

    void append(const std::string &payload) {
        if (payload.size() + 18 > LEDGER_RECORD_MAX)
            die("notes record exceeds 4096-byte law");
        chain = fnv64(payload.data(), payload.size(), chain);
        std::string line = payload + "\t" + hex16(chain) + "\n";
        int fd = open(NOTES_PATH, O_WRONLY | O_APPEND | O_CREAT, 0644);
        if (fd < 0) die("cannot open notes for append");
        ssize_t w = write(fd, line.data(), line.size());
        if (w < 0 || (size_t)w != line.size() || close(fd) != 0)
            die("notes append failed");
        records++;
    }

    void ensure_law() {
        if (law_seen) return;
        if (records != 0) die("cannot start a nonempty notes chain");
        append(std::string("N\t1\t") + NOTES_LAW);
        law_seen = true;
    }

    NoteRec *by_identity(const std::string &version, const std::string &unit) {
        for (auto &n : notes)
            if (n.version == version && n.unit_hex == unit) return &n;
        return nullptr;
    }

    /* the citizen's shape, recovered from the pinned school glyphs */
    static std::vector<std::string> shape_of_unit(const School &sc,
                                                  const std::string &unit_hex) {
        for (const auto &shape : sc.legalised) {
            uint64_t arity = split_ws(shape).size();
            if (hex16(unit_id_v1(arity, shape)) == unit_hex)
                return split_ws(shape);
        }
        return {};
    }

    void write_blob(const std::string &blob, uint64_t digest) {
        struct stat st;
        if (stat(NOTES_DIR, &st) != 0) {
            if (mkdir(NOTES_DIR, 0755) != 0) die("cannot open the weight morgue");
        } else if (!S_ISDIR(st.st_mode))
            die("the weight morgue is not a directory");
        std::string path = std::string(NOTES_DIR) + "/" + hex16(digest);
        std::string existing;
        if (read_file(path, existing)) {
            if (existing != blob) die("weight blob digest collision");
            return;
        }
        std::string tmp = path + ".tmp";
        FILE *f = fopen(tmp.c_str(), "wb");
        if (!f) die("cannot write weight blob");
        if (fwrite(blob.data(), 1, blob.size(), f) != blob.size() ||
            fflush(f) != 0 || fclose(f) != 0)
            die("cannot write weight blob");
        if (rename(tmp.c_str(), path.c_str()) != 0)
            die("cannot seal weight blob");
    }

    NoteWeights verify_blob(const std::string &weight_hex,
                            uint64_t lineno) const {
        std::string blob;
        std::string path = std::string(NOTES_DIR) + "/" + weight_hex;
        if (!read_file(path, blob))
            die("notes line " + std::to_string(lineno) + ": weight blob " +
                weight_hex + " is missing");
        if (blob.size() != NOTE_BLOB_BYTES)
            die("notes line " + std::to_string(lineno) +
                ": weight blob is not canonical body5-note-weight-v1");
        if (hex16(fnv64(blob.data(), blob.size(), FNV_SEED)) != weight_hex)
            die("notes line " + std::to_string(lineno) +
                ": weight blob digest mismatch");
        NoteWeights nw{};
        for (int d = 0; d <= DIM; ++d) {
            float value = f32_from_le(blob.data() + (size_t)d * 4);
            if (!std::isfinite(value))
                die("notes line " + std::to_string(lineno) +
                    ": weight blob holds a non-finite float");
            if (d == DIM) nw.bias = value;
            else nw.w[d] = value;
        }
        return nw;
    }

    /* verify a training receipt's integer side against its own pins */
    void verify_training(const NoteRec &n, const NoteRec::Training &t,
                         const School &sc_now, uint64_t lineno) const {
        auto shape = shape_of_unit(sc_now, n.unit_hex);
        if (shape.empty())
            die("notes line " + std::to_string(lineno) +
                ": citizen unit has no glyph shape");
        NoteDataset ds = note_dataset(m, shape, t.after_meals);
        if (!ds.ok)
            die("notes line " + std::to_string(lineno) +
                ": pinned dataset no longer derives (" + ds.refusal + ")");
        if (ds.positives != t.positives || ds.negatives != t.negatives ||
            ds.holdout_pos != t.hpos || ds.holdout_neg != t.hneg)
            die("notes line " + std::to_string(lineno) +
                ": dataset drifted from its pin");
        uint64_t baseline = ds.holdout_pos > ds.holdout_neg ? ds.holdout_pos
                                                            : ds.holdout_neg;
        if (t.baseline != baseline)
            die("notes line " + std::to_string(lineno) + ": false baseline");
        if (t.hits > ds.holdout_pos + ds.holdout_neg)
            die("notes line " + std::to_string(lineno) +
                ": holdout hits exceed the holdout");
        if (t.has_e) {
            const char *want = t.hits > baseline ? "LIT" : "DIM";
            if (t.verdict != want)
                die("notes line " + std::to_string(lineno) + ": false verdict");
        }
        NoteWeights nw = verify_blob(t.weight_hex, lineno);
        uint64_t hits = note_holdout_hits(nw, ds.holdout);
        if (hits != t.hits)
            die("notes line " + std::to_string(lineno) +
                ": false holdout hits");
    }

    void load(const School &sc_now) {
        std::string raw;
        if (!read_file(NOTES_PATH, raw)) {
            struct stat st;
            if (stat(NOTES_PATH, &st) == 0) die("cannot read notes");
            return;
        }
        if (raw.empty()) die("notes has no law record");
        size_t pos = 0;
        uint64_t lineno = 0;
        while (pos < raw.size()) {
            size_t nl = raw.find('\n', pos);
            if (nl == std::string::npos)
                die("notes line " + std::to_string(lineno + 1) + " is unsealed");
            std::string line = raw.substr(pos, nl - pos);
            if (line.size() + 1 > LEDGER_RECORD_MAX)
                die("notes line " + std::to_string(lineno + 1) +
                    " exceeds 4096-byte law");
            pos = nl + 1;
            lineno++;
            size_t tab = line.rfind('\t');
            if (tab == std::string::npos || line.size() - tab - 1 != 16)
                die("notes line " + std::to_string(lineno) + " has no chain field");
            uint64_t claimed;
            if (!canon_hex16(line.substr(tab + 1), &claimed))
                die("notes line " + std::to_string(lineno) + " chain is not hex16");
            std::string payload = line.substr(0, tab);
            uint64_t next = fnv64(payload.data(), payload.size(), chain);
            if (next != claimed)
                die("notes chain broken at line " + std::to_string(lineno));
            chain = next;
            records++;
            replay(payload, lineno, sc_now);
        }
        if (!law_seen) die("notes has no law record");
        /* verify every complete training against its pins; the trailing
           record may be an honest interruption, recovered by recover() */
        size_t unfinished = 0;
        for (size_t i = 0; i < notes.size(); ++i) {
            const NoteRec &n = notes[i];
            for (size_t t = 0; t < n.ts.size(); ++t) {
                if (!n.ts[t].has_e) unfinished++;
                verify_training(n, n.ts[t], sc_now, records);
            }
            if (n.ts.empty() && i + 1 != notes.size())
                die("notes holds a mint without training before its tip");
        }
        if (unfinished > 1)
            die("notes holds more than one unfinished training");
    }

    void replay(const std::string &payload, uint64_t lineno,
                const School &sc_now) {
        auto f = split_tabs(payload);
        const std::string where = "notes line " + std::to_string(lineno);
        if (f.empty()) die(where + " is empty");
        if (f[0] == "N") {
            if (lineno != 1 || f.size() != 3 || f[1] != "1" || f[2] != NOTES_LAW)
                die(where + ": notes law record");
            law_seen = true;
            return;
        }
        if (!law_seen) die(where + ": record before notes law");
        if (open_training && f[0] != "E")
            die(where + ": record between T and its E");
        if (!notes.empty() && notes.back().ts.empty() && f[0] != "T")
            die(where + ": record between M and its first T");
        if (f[0] == "M") {
            if (!notes.empty()) {
                const NoteRec &prev = notes.back();
                if (prev.ts.empty() || !prev.ts.back().has_e)
                    die(where + ": a new mint before the last note closed");
            }
            if (f.size() != 11) die(where + ": M arity");
            NoteRec n;
            if (!canon_u64(f[1], &n.id) || n.id != notes.size() + 1)
                die(where + ": M note id is not sequential");
            uint64_t uid;
            if (!canon_hex16(f[2], &uid) || !label_ok(f[3]))
                die(where + ": M identity grammar");
            n.unit_hex = f[2];
            n.version = f[3];
            if (!canon_u64(f[4], &n.parl_records) || n.parl_records == 0 ||
                !canon_hex16(f[5], &n.parl_chain) ||
                !canon_u64(f[6], &n.after_meals) ||
                !canon_hex16(f[7], &n.main_chain) ||
                !canon_hex16(f[8], &n.forge_h) || !canon_hex16(f[9], &n.forge_l) ||
                !canon_hex16(f[10], &n.seed))
                die(where + ": M field grammar");
            if (!pl.prefixes.count({n.parl_records, n.parl_chain}))
                die(where + ": M parliament prefix");
            if (!m.prefixes.count({n.after_meals, n.main_chain}))
                die(where + ": M main prefix");
            if (!pl.citizen_at(n.parl_records, n.version, n.unit_hex))
                die(where + ": M names no citizen at its pinned hour");
            if (by_identity(n.version, n.unit_hex))
                die(where + ": identity already minted");
            if (n.seed != note_seed_for(n.version, n.unit_hex))
                die(where + ": M seed is not derived from its identity");
            auto shape = shape_of_unit(sc_now, n.unit_hex);
            if (shape.empty()) die(where + ": citizen unit has no glyph shape");
            if (!note_shape_alive_at(m, shape, n.after_meals, n.main_chain))
                die(where + ": mint pinned at a starved hour");
            n.last_event_meal = n.after_meals;
            notes.push_back(std::move(n));
        } else if (f[0] == "T") {
            if (f.size() != 16) die(where + ": T arity");
            uint64_t id;
            if (!canon_u64(f[1], &id) || id == 0 || id > notes.size())
                die(where + ": T names no note");
            NoteRec &n = notes[id - 1];
            if (notes.back().ts.empty() && id != notes.size())
                die(where + ": first T does not close the open mint");
            if (!n.ts.empty() && !n.ts.back().has_e)
                die(where + ": T before the last training closed");
            if (!n.alive) die(where + ": T for a note in the morgue");
            NoteRec::Training t;
            if (!canon_u64(f[2], &t.after_meals) ||
                !canon_hex16(f[3], &t.main_chain) ||
                !canon_hex16(f[4], &t.forge_h) || !canon_hex16(f[5], &t.forge_l) ||
                !canon_hex16(f[6], &t.seed) || !canon_u64(f[7], &t.steps) ||
                !canon_u64(f[8], &t.loss_mb) || !canon_u64(f[9], &t.positives) ||
                !canon_u64(f[10], &t.negatives) || !canon_u64(f[11], &t.hpos) ||
                !canon_u64(f[12], &t.hneg) || !canon_u64(f[13], &t.hits) ||
                !canon_u64(f[14], &t.baseline) || f[15].size() != 16)
                die(where + ": T field grammar");
            uint64_t wd;
            if (!canon_hex16(f[15], &wd)) die(where + ": T weight digest");
            t.weight_hex = f[15];
            if (!m.prefixes.count({t.after_meals, t.main_chain}))
                die(where + ": T main prefix");
            if (n.ts.empty()) {
                if (t.after_meals != n.after_meals ||
                    t.main_chain != n.main_chain || t.forge_h != n.forge_h ||
                    t.forge_l != n.forge_l || t.seed != n.seed)
                    die(where + ": first training does not repeat the mint pin");
            } else {
                if (t.after_meals < n.ts.back().after_meals + NOTE_COOLDOWN)
                    die(where + ": training inside the cooldown");
            }
            if (t.seed != n.seed)
                die(where + ": training seed forked from the mint");
            if (t.after_meals < n.last_event_meal)
                die(where + ": training pin travels backward");
            if (t.steps == 0 ||
                (t.steps != NOTE_STEPS_PASS && t.steps != NOTE_STEPS_WEAKEN))
                die(where + ": T steps outside the governed budgets");
            const Parliament::Admission *a =
                pl.citizen_at(n.parl_records, n.version, n.unit_hex);
            uint64_t budget =
                a && a->verdict == "PASS" ? NOTE_STEPS_PASS : NOTE_STEPS_WEAKEN;
            if (t.steps != budget)
                die(where + ": T budget does not match the citizen's verdict");
            auto shape = shape_of_unit(sc_now, n.unit_hex);
            if (shape.empty()) die(where + ": citizen unit has no glyph shape");
            if (!note_shape_alive_at(m, shape, t.after_meals, t.main_chain))
                die(where + ": training pinned at a starved hour");
            n.ts.push_back(std::move(t));
            n.last_event_meal = n.ts.back().after_meals;
            open_training = id;
        } else if (f[0] == "E") {
            if (f.size() != 3) die(where + ": E arity");
            uint64_t id;
            if (!canon_u64(f[1], &id) || id == 0 || id > notes.size())
                die(where + ": E names no note");
            NoteRec &n = notes[id - 1];
            if (id != open_training || n.ts.empty() || n.ts.back().has_e)
                die(where + ": E without an open training");
            if (f[2] != "LIT" && f[2] != "DIM") die(where + ": E verdict class");
            n.ts.back().has_e = true;
            n.ts.back().verdict = f[2];
            open_training = 0;
        } else if (f[0] == "R" || f[0] == "Z") {
            if (f.size() != 4) die(where + ": rent arity");
            uint64_t id, after, mc;
            if (!canon_u64(f[1], &id) || id == 0 || id > notes.size() ||
                !canon_u64(f[2], &after) || !canon_hex16(f[3], &mc))
                die(where + ": rent field grammar");
            NoteRec &n = notes[id - 1];
            if (!m.prefixes.count({after, mc})) die(where + ": rent main prefix");
            if (after < n.last_event_meal)
                die(where + ": rent pin travels backward");
            auto shape = shape_of_unit(sc_now, n.unit_hex);
            if (shape.empty()) die(where + ": citizen unit has no glyph shape");
            bool truth = note_shape_alive_at(m, shape, after, mc);
            if (f[0] == "R") {
                if (!n.alive) die(where + ": R for a note already in the morgue");
                if (truth) die(where + ": R pinned at a well-fed hour");
                n.alive = 0;
            } else {
                if (n.alive) die(where + ": Z for a note not in the morgue");
                if (!truth) die(where + ": Z pinned at a starved hour");
                n.alive = 1;
            }
            n.last_event_meal = after;
        } else {
            die(where + ": unknown notes record type");
        }
    }

    /* seal T (+blob) and its integer E from a finished training */
    void seal_training(NoteRec &n, uint64_t after_meals, uint64_t main_chain,
                       uint64_t fh, uint64_t fl, uint64_t seed, uint64_t steps,
                       const NoteDataset &ds, const NoteWeights &nw) {
        for (int d = 0; d < DIM; ++d)
            if (!std::isfinite(nw.w[d]))
                die("training produced a non-finite weight");
        if (!std::isfinite(nw.bias))
            die("training produced a non-finite bias");
        std::string blob = note_blob_bytes(nw);
        uint64_t wd = fnv64(blob.data(), blob.size(), FNV_SEED);
        write_blob(blob, wd);
        uint64_t hits = note_holdout_hits(nw, ds.holdout);
        uint64_t baseline = ds.holdout_pos > ds.holdout_neg ? ds.holdout_pos
                                                            : ds.holdout_neg;
        NoteRec::Training t;
        t.after_meals = after_meals;
        t.main_chain = main_chain;
        t.forge_h = fh;
        t.forge_l = fl;
        t.seed = seed;
        t.steps = steps;
        t.loss_mb = nw.loss_microbits;
        t.positives = ds.positives;
        t.negatives = ds.negatives;
        t.hpos = ds.holdout_pos;
        t.hneg = ds.holdout_neg;
        t.hits = hits;
        t.baseline = baseline;
        t.weight_hex = hex16(wd);
        append("T\t" + std::to_string(n.id) + "\t" + std::to_string(after_meals) +
               "\t" + hex16(main_chain) + "\t" + hex16(fh) + "\t" + hex16(fl) +
               "\t" + hex16(seed) + "\t" + std::to_string(steps) + "\t" +
               std::to_string(nw.loss_microbits) + "\t" +
               std::to_string(ds.positives) + "\t" + std::to_string(ds.negatives) +
               "\t" + std::to_string(ds.holdout_pos) + "\t" +
               std::to_string(ds.holdout_neg) + "\t" + std::to_string(hits) +
               "\t" + std::to_string(baseline) + "\t" + t.weight_hex);
        const char *verdict = hits > baseline ? "LIT" : "DIM";
        append("E\t" + std::to_string(n.id) + "\t" + verdict);
        t.has_e = true;
        t.verdict = verdict;
        printf("  T steps %llu loss %llu holdout %llu/%llu baseline %llu "
               "weight %s\n",
               (unsigned long long)steps,
               (unsigned long long)nw.loss_microbits, (unsigned long long)hits,
               (unsigned long long)(ds.holdout_pos + ds.holdout_neg),
               (unsigned long long)baseline, t.weight_hex.c_str());
        printf("  E %s\n", verdict);
        n.ts.push_back(std::move(t));
        n.last_event_meal = after_meals;
    }

    /* recover a trailing interrupted mint or training exactly once */
    void recover(const School &sc_now) {
        if (notes.empty()) return;
        if (open_training) {
            NoteRec &n = notes[open_training - 1];
            NoteRec::Training &t = n.ts.back();
            const char *verdict = t.hits > t.baseline ? "LIT" : "DIM";
            append("E\t" + std::to_string(n.id) + "\t" + verdict);
            t.has_e = true;
            t.verdict = verdict;
            open_training = 0;
            fprintf(stderr, "notes: recovered training of note %llu as %s\n",
                    (unsigned long long)n.id, verdict);
            return;
        }
        NoteRec &n = notes.back();
        if (n.ts.empty()) {
            auto shape = shape_of_unit(sc_now, n.unit_hex);
            if (shape.empty()) die("recovery: citizen unit has no glyph shape");
            if (!note_shape_alive_at(m, shape, n.after_meals, n.main_chain))
                die("recovery: mint pinned at a starved hour");
            NoteDataset ds = note_dataset(m, shape, n.after_meals);
            if (!ds.ok) die("recovery: " + ds.refusal);
            const Parliament::Admission *a =
                pl.citizen_at(n.parl_records, n.version, n.unit_hex);
            if (!a) die("recovery: mint names no citizen");
            bool hp, lp;
            uint64_t fh = forge_digest(FORGE_HEADER_PATH, &hp);
            uint64_t fl = forge_digest(FORGE_LIB_PATH, &lp);
            if (!hp || !lp || fh != n.forge_h || fl != n.forge_l)
                die("recovery: installed forge no longer matches the mint pin");
            uint64_t steps = a->verdict == "PASS" ? NOTE_STEPS_PASS
                                                  : NOTE_STEPS_WEAKEN;
            NoteWeights nw = note_train(ds, steps, n.seed);
            seal_training(n, n.after_meals, n.main_chain, n.forge_h, n.forge_l,
                          n.seed, steps, ds, nw);
            fprintf(stderr, "notes: recovered mint of note %llu from its pins\n",
                    (unsigned long long)n.id);
        }
    }

    /* reconcile rent for every note against the live prefix */
    void reconcile_rent(const School &sc_now) {
        for (auto &n : notes) {
            auto shape = shape_of_unit(sc_now, n.unit_hex);
            if (shape.empty()) continue;
            bool alive_now = note_shape_alive_at(m, shape, m.meals, m.led.chain);
            if (n.alive && !alive_now) {
                append("R\t" + std::to_string(n.id) + "\t" +
                       std::to_string(m.meals) + "\t" + hex16(m.led.chain));
                n.alive = 0;
                n.last_event_meal = m.meals;
                printf("  R note %llu at meal %llu (the morgue keeps the "
                       "weight)\n",
                       (unsigned long long)n.id, (unsigned long long)m.meals);
            } else if (!n.alive && alive_now) {
                append("Z\t" + std::to_string(n.id) + "\t" +
                       std::to_string(m.meals) + "\t" + hex16(m.led.chain));
                n.alive = 1;
                n.last_event_meal = m.meals;
                printf("  Z note %llu at meal %llu (the same identity "
                       "returns)\n",
                       (unsigned long long)n.id, (unsigned long long)m.meals);
            }
        }
    }
};

/* refuse when any earlier body holds an open record boundary the mint
   would have to repair; notes may not write the old chains */
static void refuse_open_boundaries(const School &sc, const Parliament &pl) {
    if (sc.pending_glyph)
        die("an earlier body has an open record boundary (school pass "
            "without its glyph)");
    if (!pl.ballots.empty()) {
        const Ballot &b = pl.ballots.back();
        if (!b.has_v || b.jrows.size() != 3)
            die("an earlier body has an open record boundary (parliament "
                "ballot unfinished)");
    }
}

static void cmd_mint(Mycelium &m, const std::string &glyph_str) {
    uint64_t glyph;
    if (!canon_u64(glyph_str, &glyph) || glyph == 0)
        die("mint takes a glyph ordinal");
    bool hp, lp;
    uint64_t fh = forge_digest(FORGE_HEADER_PATH, &hp);
    uint64_t fl = forge_digest(FORGE_LIB_PATH, &lp);
    if (!hp || !lp)
        die("the forge is not installed; the mint refuses instead of "
            "falling back to a checkout");
    School sc(m);
    sc.load();
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    refuse_open_boundaries(sc, pl);
    if (glyph > sc.legalised.size()) die("mint needs an existing glyph");
    const std::string &shape_str = sc.legalised[glyph - 1];
    uint64_t arity = split_ws(shape_str).size();
    std::string unit = hex16(unit_id_v1(arity, shape_str));
    const Parliament::Admission *a =
        pl.citizen_at(pl.records, RECOGNIZER_V1, unit);
    if (!a) die("mint refused: glyph " + glyph_str + " is not a citizen");
    Notes no(m, pl);
    no.load(sc);
    no.recover(sc);
    no.reconcile_rent(sc);
    if (no.by_identity(RECOGNIZER_V1, unit))
        die("mint refused: identity already minted");
    ProposalSnapshot snap = proposal_snapshot(m, m.meals, m.led.chain);
    bool alive = false;
    for (const auto &st : snap.states)
        if (st.shape == shape_str && st.alive) alive = true;
    if (!alive)
        die("mint refused: a starved citizen cannot birth a fresh note");
    auto shape = split_ws(shape_str);
    NoteDataset ds = note_dataset(m, shape, m.meals);
    if (!ds.ok) die("mint refused: " + ds.refusal);
    uint64_t steps = a->verdict == "PASS" ? NOTE_STEPS_PASS : NOTE_STEPS_WEAKEN;
    uint64_t seed = note_seed_for(RECOGNIZER_V1, unit);
    NoteRec n;
    n.id = no.notes.size() + 1;
    n.unit_hex = unit;
    n.version = RECOGNIZER_V1;
    n.parl_records = pl.records;
    n.parl_chain = pl.chain;
    n.after_meals = m.meals;
    n.main_chain = m.led.chain;
    n.forge_h = fh;
    n.forge_l = fl;
    n.seed = seed;
    n.last_event_meal = m.meals;
    no.ensure_law();
    no.append("M\t" + std::to_string(n.id) + "\t" + unit + "\t" + RECOGNIZER_V1 +
              "\t" + std::to_string(pl.records) + "\t" + hex16(pl.chain) + "\t" +
              std::to_string(m.meals) + "\t" + hex16(m.led.chain) + "\t" +
              hex16(fh) + "\t" + hex16(fl) + "\t" + hex16(seed));
    printf("mint %llu: glyph %llu unit %s %s budget %llu\n",
           (unsigned long long)n.id, (unsigned long long)glyph, unit.c_str(),
           a->verdict.c_str(), (unsigned long long)steps);
    NoteWeights nw = note_train(ds, steps, seed);
    no.notes.push_back(n);
    no.seal_training(no.notes.back(), m.meals, m.led.chain, fh, fl, seed, steps,
                     ds, nw);
    printf("  receipt: notes %llu records chain %s\n",
           (unsigned long long)no.records, hex16(no.chain).c_str());
}

static void cmd_retrain(Mycelium &m, const std::string &id_str) {
    uint64_t id;
    if (!canon_u64(id_str, &id) || id == 0) die("retrain takes a note id");
    bool hp, lp;
    uint64_t fh = forge_digest(FORGE_HEADER_PATH, &hp);
    uint64_t fl = forge_digest(FORGE_LIB_PATH, &lp);
    if (!hp || !lp) die("the forge is not installed");
    School sc(m);
    sc.load();
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    refuse_open_boundaries(sc, pl);
    Notes no(m, pl);
    no.load(sc);
    no.recover(sc);
    no.reconcile_rent(sc);
    if (id > no.notes.size()) die("retrain names no note");
    NoteRec &n = no.notes[id - 1];
    if (!n.alive) die("retrain refused: the note is in the morgue");
    if (!n.ts.empty() &&
        m.meals < n.ts.back().after_meals + NOTE_COOLDOWN)
        die("retrain refused: cooldown holds until meal " +
            std::to_string(n.ts.back().after_meals + NOTE_COOLDOWN));
    const Parliament::Admission *a =
        pl.citizen_at(n.parl_records, n.version, n.unit_hex);
    if (!a) die("retrain: mint names no citizen");
    auto shape = Notes::shape_of_unit(sc, n.unit_hex);
    if (shape.empty()) die("retrain: citizen unit has no glyph shape");
    NoteDataset ds = note_dataset(m, shape, m.meals);
    if (!ds.ok) die("retrain refused: " + ds.refusal);
    uint64_t steps = a->verdict == "PASS" ? NOTE_STEPS_PASS : NOTE_STEPS_WEAKEN;
    printf("retrain %llu: %s budget %llu\n", (unsigned long long)id,
           a->verdict.c_str(), (unsigned long long)steps);
    NoteWeights nw = note_train(ds, steps, n.seed);
    no.seal_training(n, m.meals, m.led.chain, fh, fl, n.seed, steps, ds, nw);
    printf("  receipt: notes %llu records chain %s\n",
           (unsigned long long)no.records, hex16(no.chain).c_str());
}

static void cmd_notes(Mycelium &m) {
    School sc(m);
    sc.load();
    Props pr;
    pr.load(m);
    Parliament pl(m);
    pl.load(pr);
    refuse_open_boundaries(sc, pl);
    Notes no(m, pl);
    no.load(sc);
    no.recover(sc);
    no.reconcile_rent(sc);
    for (const auto &n : no.notes) {
        const char *verdict = "unjudged";
        if (!n.ts.empty() && n.ts.back().has_e)
            verdict = n.ts.back().verdict.c_str();
        printf("note %llu: unit %s %s trainings %zu %s %s\n",
               (unsigned long long)n.id, n.unit_hex.c_str(), n.version.c_str(),
               n.ts.size(), verdict, n.alive ? "alive" : "morgue");
    }
    if (no.notes.empty()) printf("the mint has struck nothing yet\n");
    printf("  receipt: notes %llu records chain %s\n",
           (unsigned long long)no.records, hex16(no.chain).c_str());
}

int main(int argc, char **argv) {
    const char *usage =
        "usage: mycelium ingest <label> <speech> <biography>\n"
        "       mycelium unfold <prompt words...>\n"
        "       mycelium field\n"
        "       mycelium ablate\n"
        "       mycelium propose\n"
        "       mycelium petition <glyph>\n"
        "       mycelium petition-opaque <glyph> <version> <unit-id>\n"
        "       mycelium franchise\n"
        "       mycelium mint <glyph>\n"
        "       mycelium retrain <note-id>\n"
        "       mycelium notes\n"
        "       mycelium enroll <arity> <token...>\n"
        "       mycelium enroll-hex <arity> <lower-hex-token...>\n"
        "       mycelium examine\n";
    if (argc < 2) { fputs(usage, stderr); return 1; }
    Mycelium m;
    m.load();
    m.report_orphans();
    std::string cmd = argv[1];
    if (cmd == "ingest") {
        if (argc != 5) { fputs(usage, stderr); return 1; }
        cmd_ingest(m, argv[2], argv[3], argv[4]);
    } else if (cmd == "unfold") {
        if (argc < 3) { fputs(usage, stderr); return 1; }
        std::string prompt;
        for (int i = 2; i < argc; ++i) {
            if (i > 2) prompt += ' ';
            prompt += argv[i];
        }
        cmd_unfold(m, prompt);
    } else if (cmd == "field") {
        cmd_field(m);
    } else if (cmd == "ablate") {
        cmd_ablate(m);
    } else if (cmd == "propose") {
        cmd_propose(m);
    } else if (cmd == "enroll") {
        if (argc < 4) { fputs(usage, stderr); return 1; }
        std::vector<std::string> tokens;
        for (int i = 3; i < argc; ++i) tokens.push_back(argv[i]);
        cmd_enroll(m, argv[2], tokens);
    } else if (cmd == "enroll-hex") {
        if (argc < 4) { fputs(usage, stderr); return 1; }
        std::vector<std::string> tokens;
        for (int i = 3; i < argc; ++i) tokens.push_back(argv[i]);
        cmd_enroll_hex(m, argv[2], tokens);
    } else if (cmd == "petition") {
        if (argc != 3) { fputs(usage, stderr); return 1; }
        cmd_petition(m, argv[2]);
    } else if (cmd == "petition-opaque") {
        if (argc != 5) { fputs(usage, stderr); return 1; }
        cmd_petition_opaque(m, argv[2], argv[3], argv[4]);
    } else if (cmd == "mint") {
        if (argc != 3) { fputs(usage, stderr); return 1; }
        cmd_mint(m, argv[2]);
    } else if (cmd == "retrain") {
        if (argc != 3) { fputs(usage, stderr); return 1; }
        cmd_retrain(m, argv[2]);
    } else if (cmd == "notes") {
        cmd_notes(m);
    } else if (cmd == "franchise") {
        cmd_franchise(m);
    } else if (cmd == "examine") {
        cmd_examine(m);
    } else {
        fputs(usage, stderr);
        return 1;
    }
    return 0;
}
