/* unicode_court.cpp -- the court of three character laws, preregistered
   in mycelium/UNICODE_COURT.md before this file existed. A read-only
   instrument: it writes no state and prints one deterministic report.

   Three laws of what an ATOM is: L-byte (every byte), L-cp (strict
   UTF-8 code points, invalid bytes dropped as the prototype drops
   them), L-u8b (code points where valid, tagged byte atoms where not).
   Everything else -- segmentation, tokenisation, stops, strip set, the
   three-atom window framed by ^ and $, the DIM-96 expansion -- is held
   constant, copied verbatim from the organism's byte-law text organs,
   so the court varies exactly one thing.

   usage: unicode_court <en-a> <en-b> <he-a> <he-b>
          unicode_court --decode <hex-bytes>
          unicode_court --embed-cp <word> */

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <set>
#include <string>
#include <vector>

static const int DIM = 96;
static const uint64_t FNV_SEED = 0xcbf29ce484222325ULL;
static const uint64_t FNV_PRIME = 0x100000001b3ULL;

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
    fprintf(stderr, "unicode_court: %s\n", msg.c_str());
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

/* ---- constant text organs, copied from the organism's byte law ---- */

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
    size_t n = 6 + (words.size() % 11);
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

/* ---- the one variable: what an atom is ---- */

enum Law { L_BYTE = 0, L_CP = 1, L_U8B = 2 };
static const char *LAW_NAME[3] = {"L-byte", "L-cp", "L-u8b"};
static const uint32_t INVALID_BYTE_BASE = 0x110000u;

/* strict RFC-3629 decode of one sequence at s[i]; returns length 2..4
   and sets *cp on success, 0 on invalid lead/continuation. ASCII is
   handled by the caller. */
static int utf8_seq(const std::string &s, size_t i, uint32_t *cp) {
    unsigned char b0 = s[i];
    size_t left = s.size() - i;
    if (b0 >= 0xc2 && b0 <= 0xdf) {
        if (left < 2) return 0;
        unsigned char b1 = s[i + 1];
        if (b1 < 0x80 || b1 > 0xbf) return 0;
        *cp = ((uint32_t)(b0 & 0x1f) << 6) | (b1 & 0x3f);
        return 2;
    }
    if (b0 >= 0xe0 && b0 <= 0xef) {
        if (left < 3) return 0;
        unsigned char b1 = s[i + 1], b2 = s[i + 2];
        unsigned char lo = 0x80, hi = 0xbf;
        if (b0 == 0xe0) lo = 0xa0;
        if (b0 == 0xed) hi = 0x9f; /* no surrogates */
        if (b1 < lo || b1 > hi || b2 < 0x80 || b2 > 0xbf) return 0;
        *cp = ((uint32_t)(b0 & 0x0f) << 12) | ((uint32_t)(b1 & 0x3f) << 6) |
              (b2 & 0x3f);
        return 3;
    }
    if (b0 >= 0xf0 && b0 <= 0xf4) {
        if (left < 4) return 0;
        unsigned char b1 = s[i + 1], b2 = s[i + 2], b3 = s[i + 3];
        unsigned char lo = 0x80, hi = 0xbf;
        if (b0 == 0xf0) lo = 0x90;
        if (b0 == 0xf4) hi = 0x8f; /* max U+10FFFF */
        if (b1 < lo || b1 > hi || b2 < 0x80 || b2 > 0xbf || b3 < 0x80 || b3 > 0xbf)
            return 0;
        *cp = ((uint32_t)(b0 & 0x07) << 18) | ((uint32_t)(b1 & 0x3f) << 12) |
              ((uint32_t)(b2 & 0x3f) << 6) | (b3 & 0x3f);
        return 4;
    }
    return 0;
}

static std::vector<uint32_t> atomize(const std::string &s, Law law) {
    std::vector<uint32_t> atoms;
    if (law == L_BYTE) {
        for (unsigned char c : s) atoms.push_back(c);
        return atoms;
    }
    size_t i = 0;
    while (i < s.size()) {
        unsigned char b = s[i];
        if (b < 0x80) {
            atoms.push_back(b);
            i++;
            continue;
        }
        uint32_t cp;
        int len = utf8_seq(s, i, &cp);
        if (len) {
            atoms.push_back(cp);
            i += (size_t)len;
        } else if (law == L_U8B) {
            /* Keep fallback bytes outside the Unicode scalar domain. */
            atoms.push_back(INVALID_BYTE_BASE + b);
            i++;
        } else {
            i++; /* L-cp: the prototype drops it on the floor */
        }
    }
    return atoms;
}

/* the prototype's gram vector, folded over atom VALUES */
static std::vector<double> gram_vec(const std::vector<uint32_t> &gram) {
    uint32_t h = 2166136261u;
    for (uint32_t v : gram) { h ^= v; h *= 16777619u; }
    std::vector<double> out;
    out.reserve(DIM);
    for (int i = 0; i < DIM; ++i) {
        h ^= h >> 13; h *= 1597334677u; h ^= h >> 16;
        out.push_back((double)(h & 0xFFFF) / 32768.0 - 1.0);
    }
    return out;
}

static std::vector<double> embed_law(const std::string &word, Law law) {
    std::vector<uint32_t> framed;
    framed.push_back('^');
    for (uint32_t a : atomize(word, law)) framed.push_back(a);
    framed.push_back('$');
    std::vector<std::vector<uint32_t>> grams;
    if (framed.size() >= 3)
        for (size_t i = 0; i + 3 <= framed.size(); ++i)
            grams.push_back({framed[i], framed[i + 1], framed[i + 2]});
    else
        grams.push_back(framed);
    std::vector<double> vec(DIM, 0.0);
    for (const auto &g : grams) {
        auto gv = gram_vec(g);
        for (int i = 0; i < DIM; ++i) vec[i] += gv[i];
    }
    double norm = 0.0;
    for (double x : vec) norm += x * x;
    norm = std::sqrt(norm) + 1e-10;
    for (double &x : vec) x /= norm;
    return vec;
}

struct CFrag {
    std::string text;
    int source;
    std::vector<std::string> toks;
    std::vector<double> emb[3]; /* one per law */
};

static std::vector<double> mean_embed_law(const std::vector<std::string> &toks,
                                          Law law) {
    std::vector<double> vec(DIM, 0.0);
    if (toks.empty()) return vec;
    for (const auto &t : toks) {
        auto e = embed_law(t, law);
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

struct Verdicts {
    double att[3], self_r[3], gramdiv[3], simmean[3], simhi[3];
    size_t frags, tokens;
};

static Verdicts judge_language(const std::string &text_a, const std::string &text_b) {
    Verdicts v;
    std::vector<CFrag> frags;
    const std::string *texts[2] = {&text_a, &text_b};
    size_t tokens = 0;
    for (int s = 0; s < 2; ++s)
        for (const auto &entry : segment(*texts[s])) {
            auto toks = tokenize(entry);
            if (toks.size() < 2) continue;
            CFrag f;
            f.text = entry;
            f.source = s;
            tokens += toks.size();
            f.toks = std::move(toks);
            frags.push_back(std::move(f));
        }
    v.frags = frags.size();
    v.tokens = tokens;
    if (frags.size() < 2) die("a language field needs at least two fragments");

    for (int law = 0; law < 3; ++law) {
        for (auto &f : frags) f.emb[law] = mean_embed_law(f.toks, (Law)law);

        /* 1: attribution -- clause prompt, origin excluded, cap 3, k 9 */
        size_t hits = 0, prompts = 0;
        for (size_t i = 0; i < frags.size(); ++i) {
            auto core = tokenize(cut_clause(frags[i].text));
            if (core.size() < 2) continue;
            prompts++;
            auto pe = mean_embed_law(core, (Law)law);
            std::vector<std::pair<double, size_t>> scored;
            for (size_t j = 0; j < frags.size(); ++j) {
                if (j == i) continue;
                scored.push_back({cosine(pe, frags[j].emb[law]), j});
            }
            std::stable_sort(scored.begin(), scored.end(),
                             [](const std::pair<double, size_t> &a,
                                const std::pair<double, size_t> &b) {
                                 return a.first > b.first;
                             });
            int seen[2] = {0, 0};
            for (const auto &sc : scored) {
                int src = frags[sc.second].source;
                if (seen[src] >= 3) continue;
                seen[src]++;
                /* only top-1 matters for the metric */
                if (frags[sc.second].source == frags[i].source) hits++;
                break;
            }
        }
        v.att[law] = prompts ? (double)hits / (double)prompts : 0.0;

        /* 2: self-retrieval -- full text as prompt, nothing excluded */
        size_t self_hits = 0;
        for (size_t i = 0; i < frags.size(); ++i) {
            auto pe = mean_embed_law(frags[i].toks, (Law)law);
            double best = -2.0;
            size_t best_j = 0;
            for (size_t j = 0; j < frags.size(); ++j) {
                double c = cosine(pe, frags[j].emb[law]);
                if (c > best) { best = c; best_j = j; }
            }
            if (best_j == i) self_hits++;
        }
        v.self_r[law] = (double)self_hits / (double)frags.size();

        /* 3: gram diversity over the field's token instances */
        std::set<std::vector<uint32_t>> distinct;
        uint64_t total = 0;
        for (const auto &f : frags)
            for (const auto &t : f.toks) {
                std::vector<uint32_t> framed;
                framed.push_back('^');
                for (uint32_t a : atomize(t, (Law)law)) framed.push_back(a);
                framed.push_back('$');
                if (framed.size() >= 3)
                    for (size_t i = 0; i + 3 <= framed.size(); ++i) {
                        distinct.insert({framed[i], framed[i + 1], framed[i + 2]});
                        total++;
                    }
                else {
                    distinct.insert(framed);
                    total++;
                }
            }
        v.gramdiv[law] = total ? (double)distinct.size() / (double)total : 0.0;

        /* 4: similarity profile over all fragment pairs */
        double sum = 0.0;
        uint64_t pairs = 0, hi = 0;
        for (size_t i = 0; i < frags.size(); ++i)
            for (size_t j = i + 1; j < frags.size(); ++j) {
                double c = cosine(frags[i].emb[law], frags[j].emb[law]);
                sum += c;
                pairs++;
                if (c > 0.5) hi++;
            }
        v.simmean[law] = pairs ? sum / (double)pairs : 0.0;
        v.simhi[law] = pairs ? (double)hi / (double)pairs : 0.0;
    }
    return v;
}

static void print_language(const char *tag, const Verdicts &v) {
    printf("[%s] frags=%zu tokens=%zu\n", tag, v.frags, v.tokens);
    for (int law = 0; law < 3; ++law)
        printf("[%s] %-6s att=%.4f self=%.4f gramdiv=%.4f simmean=%.4f "
               "sim>0.5=%.4f\n",
               tag, LAW_NAME[law], v.att[law], v.self_r[law], v.gramdiv[law],
               v.simmean[law], v.simhi[law]);
}

int main(int argc, char **argv) {
    if (argc == 3 && !strcmp(argv[1], "--decode")) {
        const char *hx = argv[2];
        size_t n = strlen(hx);
        if (n % 2) die("--decode takes an even hex string");
        std::string bytes;
        for (size_t i = 0; i < n; i += 2) {
            auto nib = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            };
            int a = nib(hx[i]), b = nib(hx[i + 1]);
            if (a < 0 || b < 0) die("--decode takes lowercase hex");
            bytes.push_back((char)((a << 4) | b));
        }
        for (int law = 1; law < 3; ++law) {
            printf("%s:", law == L_CP ? "cp" : "u8b");
            for (uint32_t a : atomize(bytes, (Law)law)) printf(" %u", a);
            printf("\n");
        }
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--embed-cp")) {
        auto v = embed_law(argv[2], L_CP);
        for (int i = 0; i < DIM; ++i) printf("%s%.17g", i ? " " : "", v[i]);
        printf("\n");
        return 0;
    }
    if (argc != 5) {
        fputs("usage: unicode_court <en-a> <en-b> <he-a> <he-b>\n"
              "       unicode_court --decode <hex-bytes>\n"
              "       unicode_court --embed-cp <word>\n",
              stderr);
        return 1;
    }
    std::string texts[4];
    printf("the court of three character laws (preregistered in UNICODE_COURT.md)\n");
    for (int i = 0; i < 4; ++i) {
        if (!read_file(argv[i + 1], texts[i])) die(std::string("cannot open ") + argv[i + 1]);
        printf("corpus %s: %zu bytes digest %s\n", argv[i + 1], texts[i].size(),
               hex16(fnv64(texts[i].data(), texts[i].size(), FNV_SEED)).c_str());
    }
    Verdicts en = judge_language(texts[0], texts[1]);
    Verdicts he = judge_language(texts[2], texts[3]);
    print_language("en", en);
    print_language("he", he);

    /* the sealed decision rule; the machine speaks, nobody chooses */
    double d_att = he.att[L_CP] - he.att[L_BYTE];
    double d_self = he.self_r[L_CP] - he.self_r[L_BYTE];
    printf("hebrew deltas: att(cp-byte)=%+.4f self(cp-byte)=%+.4f margin=0.10\n",
           d_att, d_self);
    if (d_att >= 0.10 || d_self >= 0.10)
        printf("VERDICT: the byte law LOSES on Hebrew; the field law shall bump "
               "to body1-utf8-or-byte-v2\n");
    else
        printf("VERDICT: the byte law holds within margin; the boundary stays "
               "a named experimental difference\n");
    return 0;
}
