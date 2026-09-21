/* BANK2-REVISE-ROW. Two source cases and P0 share the same local recipient.
   All byte distributions precede the input read. Two log2 weight ratios per
   row represent its three masses; the fixed-share clock runs after truth. */
#include "../byte_recurrence/frontend.h"
#include "../portable_recurrence/recurrence.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BANK_BYTES (16 + 2 * PR_ARCHIVE_BYTES)
#define SHARE_RATE 0x1p-10

_Static_assert(BF_DEPTH == PR_DEPTH, "frontend and recurrence depths differ");
_Static_assert(BANK_BYTES == 14080, "BANK2 archive layout differs");

static const double prior[3] = {0.125, 0.4375, 0.4375};
typedef struct { PRArchive cases[2]; } Bank;
typedef struct {
    PRQuote quote;
    int groups[256];
    double cold[256], a[256], b[256], bank[256], live[256];
    double inner_before[2], log_weights[3], weights[3];
} CompleteQuote;

static void fail(const char *message) {
    fprintf(stderr, "revision: %s\n", message);
    exit(1);
}

static uint32_t little32(const unsigned char *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint64_t little64(const unsigned char *p) {
    uint64_t value = 0;
    for (int i = 7; i >= 0; i--) value = (value << 8) | p[i];
    return value;
}

static int load_bank(Bank *bank, const char *path) {
    unsigned char bytes[BANK_BYTES];
    FILE *file = fopen(path, "rb");
    if (!file) return -1;
    size_t n = fread(bytes, 1, sizeof(bytes), file);
    int extra = fgetc(file);
    int bad = ferror(file);
    if (fclose(file)) bad = 1;
    if (bad || n != sizeof(bytes) || extra != EOF ||
        memcmp(bytes, "NETBANK1", 8) || little32(bytes + 8) != 2 ||
        little32(bytes + 12) != 0) return -1;
    for (int j = 0; j < 2; j++) {
        const unsigned char *book = bytes + 16 + j * PR_ARCHIVE_BYTES;
        if (memcmp(book, "NETHD256", 8) ||
            little32(book + 8) != PR_DEPTH ||
            little32(book + 12) != PR_CELLS) return -1;
        for (int i = 0; i < PR_CELLS; i++)
            bank->cases[j].counts[i] = little64(book + 16 + 8 * i);
    }
    return 0;
}

static double add_log2(double a, double b) {
    if (a == -INFINITY) return b;
    if (b == -INFINITY) return a;
    double hi = a > b ? a : b;
    double lo = a > b ? b : a;
    return hi + log1p(exp2(lo - hi)) / log(2.0);
}

static double add_three(double a, double b, double c) {
    return add_log2(add_log2(a, b), c);
}

/* Ratios already include prior odds: initial values are log2(7/2).
   Keep finite log weights even if a diagnostic probability rounds to zero. */
static void normalize_ratios(const double ratios[2], double logs[3]) {
    if (!isfinite(ratios[0]) || !isfinite(ratios[1]))
        fail("nonfinite row log ratios");
    double denominator = add_three(0.0, ratios[0], ratios[1]);
    logs[0] = -denominator;
    logs[1] = ratios[0] - denominator;
    logs[2] = ratios[1] - denominator;
    for (int j = 0; j < 3; j++)
        if (!isfinite(logs[j])) fail("nonfinite row log weight");
}

static void validate_vector(const double vector[256]) {
    double total = 0.0;
    for (int c = 0; c < 256; c++) {
        if (!isfinite(vector[c]) || vector[c] > 1e-9)
            fail("invalid full-vector log probability");
        double probability = exp2(vector[c]);
        if (!(probability > 0.0) || !isfinite(probability))
            fail("nonpositive full-vector probability");
        total += probability;
    }
    if (fabs(total - 1.0) > 1e-8)
        fail("full-vector normalization");
}

/* This function cannot inspect the current truth or the input stream. */
static void complete_quote(const BFQuote *front, const PRLife *life,
                           const Bank *bank, const double inner[2 * PR_ROWS],
                           CompleteQuote *out) {
    double log_mass[PR_GROUPS] = {0};
    const char *pattern = NULL;
    int k = 0;
    if (front->context_len == PR_DEPTH) {
        k = front->repeat_classes;
        if (k < 1 || k > PR_DEPTH) fail("invalid repeat-class count");
        pattern = front->pattern;
        double named_mass = -INFINITY;
        for (int g = 0; g < k; g++) {
            for (int j = 0; j < g; j++)
                if (front->heads[g] == front->heads[j])
                    fail("duplicate repeat-class head");
            log_mass[g] = front->logp_base[front->heads[g]];
            named_mass = add_log2(named_mass, log_mass[g]);
        }
        if (!isfinite(named_mass) || !(named_mass < 0.0))
            fail("named classes leave no NEW mass");
        log_mass[k] = log2(-expm1(named_mass * log(2.0)));
        for (int c = 0; c < 256; c++) {
            int g = 0;
            while (g < k && front->heads[g] != (uint8_t)c) g++;
            out->groups[c] = g;
        }
    } else {
        if (front->context_len < 0 || front->context_len > PR_DEPTH)
            fail("invalid context length");
        for (int c = 0; c < 256; c++) out->groups[c] = -1;
    }
    PRQuote components[2];
    for (int j = 0; j < 2; j++) {
        if (pr_quote(life, &bank->cases[j], pattern, log_mass, 32,
                     &components[j])) fail("component quote");
        if (pattern && components[j].repeat_classes != k)
            fail("frontend and component pattern disagree");
    }
    out->quote = components[0];
    out->quote.selected = components[0].selected || components[1].selected;
    out->inner_before[0] = out->inner_before[1] = log2(3.5);
    if (pattern) {
        int row = out->quote.row;
        if (row < 0 || row >= PR_ROWS) fail("invalid row");
        out->inner_before[0] = inner[2 * row];
        out->inner_before[1] = inner[2 * row + 1];
    }
    normalize_ratios(out->inner_before, out->log_weights);
    double total_weight = 0.0;
    for (int j = 0; j < 3; j++) {
        out->weights[j] = exp2(out->log_weights[j]);
        if (!isfinite(out->weights[j]) || out->weights[j] < 0.0)
            fail("invalid diagnostic row weight");
        total_weight += out->weights[j];
    }
    if (fabs(total_weight - 1.0) > 1e-12)
        fail("row-weight normalization");
    if (pattern) {
        for (int g = 0; g < k; g++)
            out->quote.candidate_scale_log2[g] = add_three(
                out->log_weights[0] + out->quote.cold_scale_log2[g],
                out->log_weights[1] + components[0].candidate_scale_log2[g],
                out->log_weights[2] + components[1].candidate_scale_log2[g]);
        out->quote.candidate_scale_log2[k] = out->quote.cold_scale_log2[k];
    }
    for (int c = 0; c < 256; c++) {
        int g = out->groups[c] < 0 ? 0 : out->groups[c];
        double local = front->logp_base[c];
        if (components[0].cold_scale_log2[g] != components[1].cold_scale_log2[g])
            fail("component cold models differ");
        out->cold[c] = local + out->quote.cold_scale_log2[g];
        out->a[c] = local + components[0].candidate_scale_log2[g];
        out->b[c] = local + components[1].candidate_scale_log2[g];
        out->bank[c] = local + out->quote.candidate_scale_log2[g];
        out->live[c] = out->quote.active_before
            ? add_log2(out->cold[c], out->quote.odds_before + out->bank[c]) -
              add_log2(0.0, out->quote.odds_before)
            : out->cold[c];
        if (pattern && g == k &&
            (out->a[c] != out->cold[c] || out->b[c] != out->cold[c] ||
             out->bank[c] != out->cold[c] ||
             fabs(exp2(out->live[c]) - exp2(out->cold[c])) > 1e-10))
            fail("NEW probability differs from cold");
    }
    validate_vector(front->logp_base);
    validate_vector(out->cold);
    validate_vector(out->a);
    validate_vector(out->b);
    validate_vector(out->bank);
    validate_vector(out->live);
}

/* Observe only after charging the completed quote. Every complete row clocks,
   including NEW/k=1/unsupported components. Incomplete context does not. */
static void update_row(const CompleteQuote *full, uint8_t truth, int share,
                       double inner[2 * PR_ROWS], double after[2]) {
    after[0] = full->inner_before[0];
    after[1] = full->inner_before[1];
    int row = full->quote.row;
    if (row < 0) return;
    int is_new = full->groups[truth] == full->quote.repeat_classes;
    after[0] += is_new ? 0.0 : full->a[truth] - full->cold[truth];
    after[1] += is_new ? 0.0 : full->b[truth] - full->cold[truth];
    if (share) {
        double posterior[3], revised[3];
        normalize_ratios(after, posterior);
        double log_keep = log1p(-SHARE_RATE) / log(2.0);
        for (int j = 0; j < 3; j++)
            revised[j] = add_log2(log_keep + posterior[j],
                                  log2(SHARE_RATE) + log2(prior[j]));
        after[0] = revised[1] - revised[0];
        after[1] = revised[2] - revised[0];
    }
    if (!isfinite(after[0]) || !isfinite(after[1]))
        fail("row posterior overflow");
    inner[2 * row] = after[0];
    inner[2 * row + 1] = after[1];
}

int main(int argc, char **argv) {
    if (argc != 3 || (strcmp(argv[2], "static") && strcmp(argv[2], "share"))) {
        fputs("usage: revision BANK.bin static|share < RAW > TSV\n", stderr);
        return 2;
    }
    int share = !strcmp(argv[2], "share");
    Bank bank;
    if (load_bank(&bank, argv[1])) fail("invalid NETBANK1/HEAD256 archive");
    BFState *front = bf_create();
    if (!front) fail("allocate frontend");
    PRLife life;
    pr_life_init_hmm(&life);
    double inner[2 * PR_ROWS];
    for (int i = 0; i < 2 * PR_ROWS; i++) inner[i] = log2(3.5);
    double peak_gain = 0.0, max_drawdown = 0.0;
    uint64_t activation_end = UINT64_MAX;
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    fputs("t\tpattern\ttruth\tgroup\tlogbase\tlogcold\tlogA\tlogB\tlogbank\t"
          "loglive\tinner_logA_before\tinner_logB_before\tinner_logA_after\t"
          "inner_logB_after\tweight_cold\tweight_A\tweight_B\tshadow_before\t"
          "odds_before\tactive_before\tactivated_after\tgain_after\n", stdout);
    for (;;) {
        BFQuote bf;
        CompleteQuote full;
        if (bf_quote(front, &bf)) fail("frontend quote");
        if (bf.t != life.events) fail("frontend/recurrence event mismatch");
        complete_quote(&bf, &life, &bank, inner, &full);

        /* All six complete vectors already exist and passed normalization. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        PRStep step;
        if (pr_observe(&life, &full.quote, full.groups[truth],
                       bf.logp_base[truth], 32.0, &step))
            fail("recurrence observe");
        if (step.cold_log2 != full.cold[truth] ||
            step.candidate_log2 != full.bank[truth] ||
            step.live_log2 != full.live[truth])
            fail("observed price differs from completed pre-truth vector");
        double after[2];
        update_row(&full, truth, share, inner, after);
        if (bf_observe(front, truth)) fail("frontend observe");
        if (life.gain_bits > peak_gain) peak_gain = life.gain_bits;
        double drawdown = peak_gain - life.gain_bits;
        if (drawdown > max_drawdown) max_drawdown = drawdown;
        if (life.gain_bits < -1.0 - 1e-7) fail("one-bit lifetime bound");
        if (max_drawdown > 16.0 + 1e-7) fail("sixteen-bit interval bound");
        if (step.activated_after) activation_end = life.events;
        printf("%zu\t%s\t%u\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t"
               "%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t"
               "%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g\n",
               bf.t, bf.pattern, (unsigned)truth, full.groups[truth],
               bf.logp_base[truth], full.cold[truth], full.a[truth], full.b[truth],
               full.bank[truth], full.live[truth], full.inner_before[0],
               full.inner_before[1], after[0], after[1], full.weights[0],
               full.weights[1], full.weights[2], step.shadow_before,
               full.quote.odds_before, step.active_before ? 1 : 0,
               step.activated_after ? 1 : 0, step.gain_after);
    }
    if (ferror(stdin) || ferror(stdout)) fail("stream I/O");
    if (fflush(stdout)) fail("flush output");
    bf_destroy(front);
    fprintf(stderr, "revision: mode=%s bytes=%llu gain=%.17g min_gain=%.17g "
                    "max_drawdown=%.17g shadow=%.17g log2_outer_odds=%.17g "
                    "activation_end=", argv[2], (unsigned long long)life.events,
            life.gain_bits, life.minimum_gain_bits, max_drawdown,
            life.shadow_bits, life.log2_odds);
    if (activation_end == UINT64_MAX) fputs("never\n", stderr);
    else fprintf(stderr, "%llu\n", (unsigned long long)activation_end);
    return 0;
}
