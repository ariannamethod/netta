#include "recurrence.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct { char name[PR_DEPTH + 1]; uint16_t offset; uint8_t k; } Row;
static Row rows[PR_ROWS];
static int nrows;
static int ncells;

static void enumerate(char name[PR_DEPTH + 1], int pos, int max_label) {
    if (pos == PR_DEPTH) {
        if (nrows >= PR_ROWS) return;
        Row *r = &rows[nrows++];
        memcpy(r->name, name, PR_DEPTH + 1);
        r->offset = (uint16_t)ncells;
        r->k = (uint8_t)(max_label + 1);
        ncells += r->k + 1;
        return;
    }
    for (int x = 0; x <= max_label + 1; x++) {
        name[pos] = (char)('0' + x);
        enumerate(name, pos + 1, x > max_label ? x : max_label);
    }
}

static int ensure_rows(void) {
    if (nrows) return nrows == PR_ROWS && ncells == PR_CELLS ? 0 : -1;
    char name[PR_DEPTH + 1] = "0";
    name[PR_DEPTH] = '\0';
    enumerate(name, 1, 0);
    return nrows == PR_ROWS && ncells == PR_CELLS ? 0 : -1;
}

int pr_row(const char pattern[PR_DEPTH + 1], int *repeat_classes) {
    if (!pattern || ensure_rows()) return -1;
    for (int i = 0; i < PR_ROWS; i++) {
        if (strcmp(rows[i].name, pattern) == 0) {
            if (repeat_classes) *repeat_classes = rows[i].k;
            return i;
        }
    }
    return -1;
}

void pr_pattern(const uint32_t context[PR_DEPTH], char out[PR_DEPTH + 1],
                uint32_t unique[PR_DEPTH], int *repeat_classes) {
    int k = 0;
    for (int i = 0; i < PR_DEPTH; i++) {
        int group = 0;
        while (group < k && unique[group] != context[i]) group++;
        if (group == k) unique[k++] = context[i];
        out[i] = (char)('0' + group);
    }
    out[PR_DEPTH] = '\0';
    if (repeat_classes) *repeat_classes = k;
}

static uint32_t read32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}
static uint64_t read64(const unsigned char *p) {
    uint64_t v = 0;
    for (int i = 7; i >= 0; i--) v = (v << 8) | p[i];
    return v;
}
static void write32(unsigned char *p, uint32_t v) {
    for (int i = 0; i < 4; i++) p[i] = (unsigned char)(v >> (8 * i));
}
static void write64(unsigned char *p, uint64_t v) {
    for (int i = 0; i < 8; i++) p[i] = (unsigned char)(v >> (8 * i));
}

int pr_archive_load(PRArchive *archive, const char *path) {
    if (!archive || !path || ensure_rows()) return -1;
    unsigned char buf[PR_ARCHIVE_BYTES];
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(buf, 1, sizeof(buf), f);
    int extra = fgetc(f);
    int error = ferror(f);
    int close_error = fclose(f);
    if (n != sizeof(buf) || extra != EOF || error || close_error ||
        memcmp(buf, "NETTARM1", 8) || read32(buf + 8) != PR_DEPTH ||
        read32(buf + 12) != PR_CELLS) return -1;
    for (int i = 0; i < PR_CELLS; i++)
        archive->counts[i] = read64(buf + 16 + 8 * i);
    return 0;
}

int pr_archive_save(const PRArchive *archive, const char *path) {
    if (!archive || !path || ensure_rows()) return -1;
    unsigned char buf[PR_ARCHIVE_BYTES];
    memcpy(buf, "NETTARM1", 8);
    write32(buf + 8, PR_DEPTH);
    write32(buf + 12, PR_CELLS);
    for (int i = 0; i < PR_CELLS; i++)
        write64(buf + 16 + 8 * i, archive->counts[i]);
    int fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0600);
    if (fd < 0) return -1;
    size_t done = 0;
    while (done < sizeof(buf)) {
        ssize_t n = write(fd, buf + done, sizeof(buf) - done);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) { close(fd); return -1; }
        done += (size_t)n;
    }
    return close(fd) == 0 ? 0 : -1;
}

int pr_archive_add(PRArchive *archive, const char *pattern, int outcome) {
    int k;
    int row = pr_row(pattern, &k);
    if (!archive || row < 0 || outcome < 0 || outcome > k) return -1;
    uint64_t *value = &archive->counts[rows[row].offset + outcome];
    if (*value == UINT64_MAX) return -1;
    (*value)++;
    return 0;
}

static int archive_support_checked(const PRArchive *archive, int row,
                                   uint64_t *out) {
    if (!archive || !out || ensure_rows() || row < 0 || row >= PR_ROWS)
        return -1;
    uint64_t support = 0;
    for (int g = 0; g <= rows[row].k; g++) {
        uint64_t value = archive->counts[rows[row].offset + g];
        if (UINT64_MAX - support < value) return -1;
        support += value;
    }
    *out = support;
    return 0;
}

uint64_t pr_archive_support(const PRArchive *archive, int row) {
    uint64_t support = 0;
    if (archive_support_checked(archive, row, &support)) return 0;
    return support;
}

void pr_life_init(PRLife *life) { if (life) memset(life, 0, sizeof(*life)); }
void pr_life_init_hmm(PRLife *life) {
    pr_life_init(life);
    if (life) life->hmm = true;
}

static double logadd2(double a, double b) {
    if (a == -INFINITY) return b;
    if (b == -INFINITY) return a;
    double hi = a > b ? a : b;
    double lo = a > b ? b : a;
    return hi + log1p(exp2(lo - hi)) / log(2.0);
}
static double log_complement(double logp) {
    return log2(-expm1(logp * log(2.0)));
}

int pr_quote(const PRLife *life, const PRArchive *archive,
             const char *pattern, const double log_mass[PR_GROUPS],
             uint64_t min_source_support, PRQuote *quote) {
    if (!life || !archive || !quote || ensure_rows()) return -1;
    memset(quote, 0, sizeof(*quote));
    quote->owner = life;
    quote->event = life->events;
    quote->row = -1;
    quote->active_before = life->active;
    quote->hmm_before = life->hmm;
    quote->odds_before = life->log2_odds;
    if (!pattern) return 0;
    int k;
    int row = pr_row(pattern, &k);
    if (row < 0 || !log_mass) return -1;
    quote->row = row;
    quote->repeat_classes = k;
    double total_mass = -INFINITY;
    for (int g = 0; g <= k; g++) {
        if (!isfinite(log_mass[g]) || log_mass[g] >= 0.0) return -1;
        quote->local_mass_log2[g] = log_mass[g];
        total_mass = logadd2(total_mass, log_mass[g]);
    }
    if (fabs(total_mass) > 1e-8) return -1;
    uint64_t total_b = 0;
    for (int g = 0; g <= k; g++) {
        uint64_t count = life->counts[rows[row].offset + g];
        if (UINT64_MAX - total_b < count) return -1;
        total_b += count;
    }
    uint64_t total_a = 0;
    if (archive_support_checked(archive, row, &total_a)) return -1;
    /* Beyond 2^53, converting integer counts to double loses unit precision. */
    if (total_a > (UINT64_C(1) << 53) ||
        total_b > (UINT64_C(1) << 53)) return -1;
    double loga0 = 0.0, loga1 = 0.0;
    bool selected = total_a >= min_source_support && total_b >= 1 && k > 1;
    quote->selected = selected;
    for (int g = 0; g <= k; g++) {
        double b = (double)life->counts[rows[row].offset + g];
        double r0 = (b + 0.5) / ((double)total_b + 0.5 * (k + 1));
        double cold = logadd2(0.0, log2(r0) - log_mass[g]) - 1.0;
        quote->cold_scale_log2[g] = cold;
        quote->candidate_scale_log2[g] = cold;
        if (selected) {
            double a = (double)archive->counts[rows[row].offset + g];
            double rA = (a + 0.5) / ((double)total_a + 0.5 * (k + 1));
            double r1 = (b + 32.0 * rA) / ((double)total_b + 32.0);
            quote->candidate_scale_log2[g] =
                logadd2(0.0, log2(r1) - log_mass[g]) - 1.0;
            if (g == k) {
                loga0 = logadd2(log_mass[g], log2(r0)) - 1.0;
                loga1 = logadd2(log_mass[g], log2(r1)) - 1.0;
            }
        }
    }
    if (selected) {
        if (!isfinite(loga0) || !isfinite(loga1) ||
            loga0 >= 0 || loga1 >= 0) return -1;
        double correction = log_complement(loga0) - log_complement(loga1);
        if (!isfinite(correction)) return -1;
        for (int g = 0; g < k; g++)
            quote->candidate_scale_log2[g] += correction;
        quote->candidate_scale_log2[k] = quote->cold_scale_log2[k];
    }
    return 0;
}

int pr_observe(PRLife *life, const PRQuote *quote, int outcome,
               double log_unit, double gate_bits, PRStep *step) {
    if (!life || !quote || !step || quote->owner != life ||
        quote->event != life->events ||
        quote->active_before != life->active ||
        quote->hmm_before != life->hmm ||
        quote->odds_before != life->log2_odds ||
        life->events == UINT64_MAX || !isfinite(log_unit) ||
        log_unit > 1e-10 || !isfinite(gate_bits) || gate_bits < 0)
        return -1;
    int g = outcome;
    if (quote->row < 0) { if (outcome != -1) return -1; g = 0; }
    else if (g < 0 || g > quote->repeat_classes) return -1;
    if (quote->row >= 0 &&
        log_unit > quote->local_mass_log2[g] + 1e-10) return -1;
    if (quote->row >= 0 &&
        life->counts[rows[quote->row].offset + g] == UINT64_MAX) return -1;
    double cold = log_unit + quote->cold_scale_log2[g];
    double candidate = log_unit + quote->candidate_scale_log2[g];
    double live = life->active
        ? logadd2(cold, life->log2_odds + candidate) -
          logadd2(0.0, life->log2_odds)
        : cold;
    if (!isfinite(cold) || !isfinite(candidate) || !isfinite(live) ||
        cold > 1e-9 || candidate > 1e-9 || live > 1e-9) return -1;
    step->cold_log2 = cold;
    step->candidate_log2 = candidate;
    step->live_log2 = live;
    step->shadow_before = life->shadow_bits;
    step->active_before = life->active;
    step->activated_after = false;
    double delta = candidate - cold;
    life->gain_bits += live - cold;
    if (life->gain_bits < life->minimum_gain_bits)
        life->minimum_gain_bits = life->gain_bits;
    life->shadow_bits += delta;
    if (life->active) {
        double z = life->log2_odds + delta;
        life->log2_odds = life->hmm
            ? log1p(-PR_HMM_HAZARD) / log(2.0) -
              logadd2(-z, log2(PR_HMM_HAZARD))
            : z;
    }
    else if (life->shadow_bits >= gate_bits) {
        life->active = true;
        life->log2_odds = 0.0;
        step->activated_after = true;
    }
    if (quote->row >= 0) {
        uint64_t *count = &life->counts[rows[quote->row].offset + g];
        (*count)++;
    }
    life->events++;
    step->gain_after = life->gain_bits;
    return 0;
}
