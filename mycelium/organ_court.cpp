/* organ_court.cpp -- the organ court: the threshold surface,
   preregistered in mycelium/ORGAN_COURT.md before this file existed.
   Read-only; one deterministic report; the verdict is printed by the
   machine from the sealed rule.

   Two arms only: L-byte and L-u8b. Three rooms from four corpora:
   the Hebrew two-source field, the mixed four-source field, and the
   English control. Metrics: uncapped attribution (the rescuing
   per-source cap removed), the ranking margin a threshold has to work
   with, and corpse composition under the unfold selection law. The
   att-cap column is the consistency row binding this instrument to the
   first court's numbers on the same field.

   usage: organ_court <en-a> <en-b> <he-a> <he-b> */

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

/* ---- the two arms ---- */

enum Law { L_BYTE = 0, L_U8B = 1 };
static const char *LAW_NAME[2] = {"L-byte", "L-u8b"};

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
        if (b0 == 0xed) hi = 0x9f;
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
        if (b0 == 0xf4) hi = 0x8f;
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
        if (b < 0x80) { atoms.push_back(b); i++; continue; }
        uint32_t cp;
        int len = utf8_seq(s, i, &cp);
        if (len) { atoms.push_back(cp); i += (size_t)len; }
        else { atoms.push_back(b); i++; }
    }
    return atoms;
}

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

/* ---- the rooms ---- */

struct RFrag {
    int source;
    std::vector<std::string> toks;
    std::string clause;
    std::vector<double> emb[2];
};

struct RoomLaw {
    double att_nocap, margin, corpse_src, corpse_cos, att_cap;
};

static RoomLaw judge_arm(std::vector<RFrag> &frags, Law law) {
    for (auto &f : frags) f.emb[law] = mean_embed_law(f.toks, law);
    size_t prompts = 0, hits_nocap = 0, hits_cap = 0;
    double margin_sum = 0.0, src_sum = 0.0, ccos_sum = 0.0;
    size_t corpse_rooms = 0;
    for (size_t i = 0; i < frags.size(); ++i) {
        auto core = tokenize(frags[i].clause);
        if (core.size() < 2) continue;
        prompts++;
        auto pe = mean_embed_law(core, law);
        std::vector<double> scores;
        scores.reserve(frags.size() - 1);
        double best = -2.0, worst = 2.0;
        size_t best_j = 0;
        std::vector<std::pair<double, size_t>> ranked;
        for (size_t j = 0; j < frags.size(); ++j) {
            if (j == i) continue;
            double c = cosine(pe, frags[j].emb[law]);
            scores.push_back(c);
            ranked.push_back({c, j});
            if (c > best) { best = c; best_j = j; }
            if (c < worst) worst = c;
        }
        /* 1: uncapped attribution -- earliest maximum wins on a tie */
        if (frags[best_j].source == frags[i].source) hits_nocap++;
        /* 2: ranking margin -- (top1 - median) / (top1 - min); the
           median is the upper middle of the ascending sort */
        std::sort(scores.begin(), scores.end());
        double med = scores[scores.size() / 2];
        margin_sum += (best > worst) ? (best - med) / (best - worst) : 0.0;
        /* 3: corpse composition under the unfold law, k=9 cap 3;
           the capped top-1 is the consistency row */
        std::stable_sort(ranked.begin(), ranked.end(),
                         [](const std::pair<double, size_t> &a,
                            const std::pair<double, size_t> &b) {
                             return a.first > b.first;
                         });
        std::vector<size_t> chosen;
        std::vector<int> seen(16, 0);
        bool first_taken = false;
        for (const auto &r : ranked) {
            int src = frags[r.second].source;
            if (seen[src] >= 3) continue;
            seen[src]++;
            if (!first_taken) {
                first_taken = true;
                if (src == frags[i].source) hits_cap++;
            }
            chosen.push_back(r.second);
            if (chosen.size() >= 9) break;
        }
        std::set<int> srcs;
        double cc = 0.0;
        size_t cp_pairs = 0;
        for (size_t a = 0; a < chosen.size(); ++a) {
            srcs.insert(frags[chosen[a]].source);
            for (size_t b = a + 1; b < chosen.size(); ++b) {
                cc += cosine(frags[chosen[a]].emb[law], frags[chosen[b]].emb[law]);
                cp_pairs++;
            }
        }
        if (cp_pairs) { ccos_sum += cc / (double)cp_pairs; corpse_rooms++; }
        src_sum += (double)srcs.size();
    }
    RoomLaw r;
    r.att_nocap = prompts ? (double)hits_nocap / (double)prompts : 0.0;
    r.att_cap = prompts ? (double)hits_cap / (double)prompts : 0.0;
    r.margin = prompts ? margin_sum / (double)prompts : 0.0;
    r.corpse_src = prompts ? src_sum / (double)prompts : 0.0;
    r.corpse_cos = corpse_rooms ? ccos_sum / (double)corpse_rooms : 0.0;
    return r;
}

static void build_room(const std::vector<const std::string *> &texts,
                       std::vector<RFrag> &frags) {
    frags.clear();
    for (size_t s = 0; s < texts.size(); ++s)
        for (const auto &entry : segment(*texts[s])) {
            auto toks = tokenize(entry);
            if (toks.size() < 2) continue;
            RFrag f;
            f.source = (int)s;
            f.clause = cut_clause(entry);
            f.toks = std::move(toks);
            frags.push_back(std::move(f));
        }
    if (frags.size() < 2) die("a room needs at least two fragments");
}

static RoomLaw sit_room(const char *tag,
                        const std::vector<const std::string *> &texts,
                        RoomLaw *out_u8b) {
    std::vector<RFrag> frags;
    build_room(texts, frags);
    printf("[%s] frags=%zu sources=%zu\n", tag, frags.size(), texts.size());
    RoomLaw rb = judge_arm(frags, L_BYTE);
    RoomLaw ru = judge_arm(frags, L_U8B);
    const RoomLaw *rl[2] = {&rb, &ru};
    for (int law = 0; law < 2; ++law)
        printf("[%s] %-6s att-nocap=%.4f margin=%.4f corpse-src=%.4f "
               "corpse-cos=%.4f att-cap=%.4f\n",
               tag, LAW_NAME[law], rl[law]->att_nocap, rl[law]->margin,
               rl[law]->corpse_src, rl[law]->corpse_cos, rl[law]->att_cap);
    *out_u8b = ru;
    return rb;
}

int main(int argc, char **argv) {
    if (argc != 5) {
        fputs("usage: organ_court <en-a> <en-b> <he-a> <he-b>\n", stderr);
        return 1;
    }
    std::string texts[4];
    printf("the organ court: the threshold surface (preregistered in "
           "ORGAN_COURT.md)\n");
    for (int i = 0; i < 4; ++i) {
        if (!read_file(argv[i + 1], texts[i]))
            die(std::string("cannot open ") + argv[i + 1]);
        printf("corpus %s: %zu bytes digest %s\n", argv[i + 1], texts[i].size(),
               hex16(fnv64(texts[i].data(), texts[i].size(), FNV_SEED)).c_str());
    }
    RoomLaw u;
    sit_room("en", {&texts[0], &texts[1]}, &u);
    RoomLaw hb = sit_room("he", {&texts[2], &texts[3]}, &u);
    RoomLaw hu = u;
    sit_room("mixed", {&texts[0], &texts[1], &texts[2], &texts[3]}, &u);

    double d_att = hu.att_nocap - hb.att_nocap;
    double ratio = hu.margin > 0.0 ? hb.margin / hu.margin : 1.0;
    double d_src = hu.corpse_src - hb.corpse_src;
    printf("hebrew rule: d-att-nocap=%+.4f margin-ratio(byte/u8b)=%.4f "
           "d-corpse-src=%+.4f  thresholds: 0.10 / 0.50 / 0.50\n",
           d_att, ratio, d_src);
    if (d_att >= 0.10 || ratio <= 0.5 || d_src >= 0.5)
        printf("VERDICT: the byte law LOSES the threshold surface; the field "
               "law shall bump to body1-utf8-or-byte-v2\n");
    else
        printf("VERDICT: the byte law holds the threshold surface; the "
               "parliament may sit on it with the cone as a named limit\n");
    return 0;
}
