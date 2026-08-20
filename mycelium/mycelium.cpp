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

static uint64_t fnv64(const void *p, size_t n, uint64_t h) {
    const uint8_t *b = (const uint8_t *)p;
    for (size_t i = 0; i < n; ++i) { h ^= b[i]; h *= FNV_PRIME; }
    return h;
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

/* body 2: the proposer's receipts. Their own chain under their own law;
   the main ledger and its closed body1-byte-v1 law are never touched. */
struct Props {
    uint64_t chain = FNV_SEED;
    uint64_t records = 0;
    uint64_t last_after = 0;
    bool law_seen = false;

    void load(const std::set<std::pair<uint64_t, uint64_t>> &main_prefixes) {
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
                if (!main_prefixes.count({after, mainchain}))
                    die(where + ": P main prefix");
                if (after < last_after)
                    die(where + ": P after-meals is not monotonic");
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
    pr.load(m.prefixes);
    std::map<std::string, std::set<uint64_t>> shapes;
    for (const auto &f : m.frags)
        for (size_t i = 0; i + 1 < f.toks.size(); ++i) {
            shapes[f.toks[i] + " " + f.toks[i + 1]].insert(f.meal);
            if (i + 2 < f.toks.size())
                shapes[f.toks[i] + " " + f.toks[i + 1] + " " + f.toks[i + 2]]
                    .insert(f.meal);
        }
    struct Prop { const std::string *shape; const std::set<uint64_t> *meals; };
    std::vector<Prop> props;
    for (const auto &kv : shapes)
        if (kv.second.size() >= 2) props.push_back({&kv.first, &kv.second});
    std::stable_sort(props.begin(), props.end(), [](const Prop &a, const Prop &b) {
        if (a.meals->size() != b.meals->size())
            return a.meals->size() > b.meals->size();
        return *a.shape < *b.shape;
    });
    std::string block = "proposals after meal " + std::to_string(m.meals) +
                        " (main chain " + hex16(m.led.chain) + "):\n";
    uint64_t alive = 0, dead = 0;
    for (const auto &p : props) {
        uint64_t last = *p.meals->rbegin();
        bool live = m.meals - last < RENT_WINDOW;
        std::string csv;
        for (uint64_t meal : *p.meals) {
            if (!csv.empty()) csv += ",";
            csv += std::to_string(meal);
        }
        if (live) {
            alive++;
            block += "  alive " + *p.shape + " support=" +
                     std::to_string(p.meals->size()) + " meals=" + csv + "\n";
        } else {
            dead++;
            block += "  dead " + *p.shape + " support=" +
                     std::to_string(p.meals->size()) + " meals=" + csv +
                     " last=" + std::to_string(last) + "\n";
        }
    }
    if (props.empty()) block += "  (nothing recurs across meals yet)\n";
    write_bytes(stdout, block);
    uint64_t dig = fnv64(block.data(), block.size(), FNV_SEED);
    if (!pr.law_seen) {
        pr.append(std::string("W\t1\t") + PROPS_LAW);
        pr.law_seen = true;
    }
    std::string payload = "P\t" + std::to_string(m.meals) + "\t" +
                          hex16(m.led.chain) + "\t" + std::to_string(alive) +
                          "\t" + std::to_string(dead) + "\t" + hex16(dig);
    pr.append(payload);
    printf("  receipt: snapshot %s | proposals %llu records chain %s\n",
           hex16(dig).c_str(), (unsigned long long)pr.records,
           hex16(pr.chain).c_str());
}

int main(int argc, char **argv) {
    const char *usage =
        "usage: mycelium ingest <label> <speech> <biography>\n"
        "       mycelium unfold <prompt words...>\n"
        "       mycelium field\n"
        "       mycelium ablate\n"
        "       mycelium propose\n";
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
    } else {
        fputs(usage, stderr);
        return 1;
    }
    return 0;
}
