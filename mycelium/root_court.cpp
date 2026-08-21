/* root_court.cpp -- the root court: subword inheritance, preregistered
   in mycelium/ROOT_COURT.md before this file existed and only after
   that contract became a merged historical fact. Read-only; the verdict
   is printed by the machine from the single sealed rule.

   The gold is the eight explicitly declared root families of the pinned
   shoresh corpus: 37 surface forms embedded directly, no tokenizer, no
   root extractor, no Pitomadom logic. Two arms: L-byte (every byte an
   atom) and L-u8b (strict RFC-3629 scalars; an invalid byte is the
   tagged atom 0x110000+byte, above the Unicode ceiling, so no fallback
   can impersonate a lawful character). Everything after atomisation is
   body 1's embedding law verbatim.

   Every run performs the contract's own gates: corpus digest, whole-file
   strict UTF-8, each declaration line exactly once, 37 distinct forms,
   68 within-family and 598 between-family pairs. Any mismatch is
   refusal, not a partial court.

   usage: root_court <shoresh.txt>
          root_court --ascii
          root_court --atoms <hex-bytes> */

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
static const uint64_t GOLD_DIGEST = 0x05d2840d282a7f14ULL;
static const uint32_t INVALID_BYTE_BASE = 0x110000u;

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
    fprintf(stderr, "root_court: %s\n", msg.c_str());
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

/* ---- the sealed gold: eight families, 37 forms ---- */

struct Family { const char *root; std::vector<const char *> forms; };

static const std::vector<Family> &gold() {
    static const std::vector<Family> g = {
        {"שלח", {"שלח", "שליח", "שליחות", "משלוח"}},
        {"כתב", {"כתב", "כתיבה", "מכתב", "כתובת"}},
        {"ספר", {"ספר", "סיפור", "ספרות", "מספר", "ספריה"}},
        {"למד", {"למד", "לימוד", "תלמיד", "תלמוד", "מלמד"}},
        {"בנה", {"בנה", "בניין", "מבנה", "בנאי", "תבנית"}},
        {"שמר", {"שמר", "שמירה", "משמר", "משמרת", "שמורה"}},
        {"פתח", {"פתח", "פתיחה", "מפתח", "פתיחות"}},
        {"חבר", {"חבר", "חיבור", "מחברת", "חברה", "חבורה"}},
    };
    return g;
}

/* ---- atoms ---- */

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
        else { atoms.push_back(INVALID_BYTE_BASE + b); i++; }
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

static double cosine(const std::vector<double> &a, const std::vector<double> &b) {
    double s = 0.0;
    for (int i = 0; i < DIM; ++i) s += a[i] * b[i];
    return s;
}

/* ---- the contract's own gates, run on every sitting ---- */

static void verify_corpus(const std::string &raw) {
    uint64_t d = fnv64(raw.data(), raw.size(), FNV_SEED);
    if (d != GOLD_DIGEST)
        die("corpus digest mismatch: " + hex16(d) + " is not " +
            hex16(GOLD_DIGEST) + "; refusal, not a partial court");
    size_t i = 0;
    while (i < raw.size()) {
        unsigned char b = raw[i];
        if (b < 0x80) { i++; continue; }
        uint32_t cp;
        int len = utf8_seq(raw, i, &cp);
        if (!len)
            die("corpus byte " + std::to_string(i) + " breaks strict UTF-8");
        i += (size_t)len;
    }
    /* each declaration line, rebuilt from the sealed table, occurs once */
    for (const auto &fam : gold()) {
        std::string line = fam.forms[0];
        for (size_t f = 1; f < fam.forms.size(); ++f)
            line += std::string(" ו") + fam.forms[f];
        line += std::string(" כולם מן השורש ") + fam.root;
        std::string sealed = "\n" + line + "\n";
        size_t count = 0, pos = 0;
        std::string hay = "\n" + raw;
        while ((pos = hay.find(sealed, pos)) != std::string::npos) {
            count++;
            pos += 1;
        }
        if (count != 1)
            die(std::string("declaration for root ") + fam.root + " occurs " +
                std::to_string(count) + " times, not once");
    }
    /* 37 distinct forms; 68 within and 598 between pairs */
    std::set<std::string> distinct;
    size_t total = 0, within = 0;
    for (const auto &fam : gold()) {
        size_t n = fam.forms.size();
        total += n;
        within += n * (n - 1) / 2;
        for (const auto *f : fam.forms) distinct.insert(f);
    }
    size_t between = total * (total - 1) / 2 - within;
    if (total != 37 || distinct.size() != 37 || within != 68 || between != 598)
        die("gold arithmetic drifted: forms " + std::to_string(total) +
            " distinct " + std::to_string(distinct.size()) + " within " +
            std::to_string(within) + " between " + std::to_string(between));
    printf("declarations: 8 lines once each; forms 37 distinct; "
           "pairs within=68 between=598\n");
}

/* ---- the sitting ---- */

struct Form { std::string text; int family; };

int main(int argc, char **argv) {
    if (argc == 2 && !strcmp(argv[1], "--ascii")) {
        static const char *fix[6] = {"raven", "shalom", "cafe",
                                     "root",  "rooted", "roots"};
        for (int law = 0; law < 2; ++law)
            for (int w = 0; w < 6; ++w) {
                auto v = embed_law(fix[w], (Law)law);
                printf("%s %s", LAW_NAME[law], fix[w]);
                for (int i = 0; i < DIM; ++i) printf(" %.17g", v[i]);
                printf("\n");
            }
        return 0;
    }
    if (argc == 3 && !strcmp(argv[1], "--atoms")) {
        const char *hx = argv[2];
        size_t n = strlen(hx);
        if (n % 2) die("--atoms takes an even hex string");
        std::string bytes;
        for (size_t i = 0; i < n; i += 2) {
            auto nib = [](char c) -> int {
                if (c >= '0' && c <= '9') return c - '0';
                if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                return -1;
            };
            int a = nib(hx[i]), b = nib(hx[i + 1]);
            if (a < 0 || b < 0) die("--atoms takes lowercase hex");
            bytes.push_back((char)((a << 4) | b));
        }
        for (int law = 0; law < 2; ++law) {
            printf("%s:", LAW_NAME[law]);
            for (uint32_t a : atomize(bytes, (Law)law)) printf(" %u", a);
            printf("\n");
        }
        return 0;
    }
    if (argc != 2) {
        fputs("usage: root_court <shoresh.txt>\n"
              "       root_court --ascii\n"
              "       root_court --atoms <hex-bytes>\n",
              stderr);
        return 1;
    }

    std::string raw;
    if (!read_file(argv[1], raw)) die(std::string("cannot open ") + argv[1]);
    printf("the root court: subword inheritance (preregistered in "
           "ROOT_COURT.md)\n");
    printf("corpus %s: %zu bytes digest %s\n", argv[1], raw.size(),
           hex16(fnv64(raw.data(), raw.size(), FNV_SEED)).c_str());
    verify_corpus(raw);

    std::vector<Form> forms;
    for (size_t fam = 0; fam < gold().size(); ++fam)
        for (const auto *f : gold()[fam].forms)
            forms.push_back({f, (int)fam});

    double map_law[2] = {0.0, 0.0};
    std::string rows;
    for (int law = 0; law < 2; ++law) {
        std::vector<std::vector<double>> emb;
        for (const auto &f : forms) emb.push_back(embed_law(f.text, (Law)law));

        size_t top1_hits = 0;
        double ap_sum = 0.0;
        double win_sum = 0.0, btw_sum = 0.0;
        size_t win_n = 0, btw_n = 0;
        size_t tp = 0, fn_ = 0, fp = 0, tn = 0;

        for (size_t i = 0; i < forms.size(); ++i)
            for (size_t j = i + 1; j < forms.size(); ++j) {
                double c = cosine(emb[i], emb[j]);
                bool same = forms[i].family == forms[j].family;
                if (same) { win_sum += c; win_n++; }
                else { btw_sum += c; btw_n++; }
                if (same) { if (c >= 0.5) tp++; else fn_++; }
                else { if (c >= 0.5) fp++; else tn++; }
            }

        for (size_t q = 0; q < forms.size(); ++q) {
            struct Cand { double score; const Form *f; };
            std::vector<Cand> cands;
            for (size_t j = 0; j < forms.size(); ++j) {
                if (j == q) continue;
                cands.push_back({cosine(emb[q], emb[j]), &forms[j]});
            }
            std::stable_sort(cands.begin(), cands.end(),
                             [](const Cand &a, const Cand &b) {
                                 if (a.score != b.score) return a.score > b.score;
                                 return a.f->text < b.f->text;
                             });
            size_t r = gold()[forms[q].family].forms.size() - 1;
            size_t rel_seen = 0;
            double ap = 0.0;
            for (size_t k = 0; k < cands.size(); ++k) {
                bool same = cands[k].f->family == forms[q].family;
                if (k == 0 && same) top1_hits++;
                if (same) {
                    rel_seen++;
                    ap += (double)rel_seen / (double)(k + 1);
                }
                char row[256];
                snprintf(row, sizeof row, "%s\t%s\t%zu\t%s\t%.17g\t%d\n",
                         LAW_NAME[law], forms[q].text.c_str(), k + 1,
                         cands[k].f->text.c_str(), cands[k].score, same ? 1 : 0);
                rows += row;
            }
            ap_sum += ap / (double)r;
        }
        map_law[law] = ap_sum / (double)forms.size();
        double tpr = (tp + fn_) ? (double)tp / (double)(tp + fn_) : 0.0;
        double fpr = (fp + tn) ? (double)fp / (double)(fp + tn) : 0.0;
        printf("[%s] top1=%.17g MAP=%.17g within-cos=%.17g between-cos=%.17g "
               "TPR=%.17g FPR=%.17g balanced=%.17g\n",
               LAW_NAME[law], (double)top1_hits / (double)forms.size(),
               map_law[law], win_sum / (double)win_n, btw_sum / (double)btw_n,
               tpr, fpr, 0.5 * (tpr + 1.0 - fpr));
    }

    fwrite(rows.data(), 1, rows.size(), stdout);
    printf("rows: %zu sealed, digest %s\n", (size_t)(37 * 36 * 2),
           hex16(fnv64(rows.data(), rows.size(), FNV_SEED)).c_str());
    double delta = map_law[L_U8B] - map_law[L_BYTE];
    printf("rule: MAP(u8b)-MAP(byte) = %.17g threshold 0.10\n", delta);
    if (delta >= 0.10)
        printf("VERDICT: the byte law LOSES subword inheritance; body 1 bumps "
               "to body1-utf8-or-byte-v2 and every prior body is remeasured\n");
    else
        printf("VERDICT: the byte law holds subword inheritance within the "
               "sealed margin; the parliament may open on L-byte with both "
               "cone limits recorded\n");
    return 0;
}
