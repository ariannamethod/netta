/* BANK2-ROW: two immutable HEAD256 cases, one local recipient and outer HMM.
   Every distribution is completed before the current raw byte is read.
   All odds fields in the TSV are log2 odds. */
#include "../byte_recurrence/frontend.h"
#include "../portable_recurrence/recurrence.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BANK_BYTES (16 + 2 * PR_ARCHIVE_BYTES)

_Static_assert(BF_DEPTH == PR_DEPTH, "frontend and recurrence depths differ");
_Static_assert(BANK_BYTES == 14080, "BANK2 archive layout differs");

typedef enum { MODE_ROW, MODE_GLOBAL, MODE_POOL } Mode;
typedef struct { PRArchive cases[2]; PRArchive *pool; } Bank;
typedef struct {
    PRQuote quote;
    int groups[256];
    double cold[256], a[256], b[256], bank[256], live[256];
    double inner_before;
    int inner_index;
} CompleteQuote;

static void fail(const char *message) {
    fprintf(stderr, "bank: %s\n", message);
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

static int load_bank(Bank *bank, const char *path, Mode mode) {
    unsigned char bytes[BANK_BYTES];
    bank->pool = NULL;
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
    if (mode == MODE_POOL) {
        bank->pool = malloc(sizeof(*bank->pool));
        if (!bank->pool) return -1;
        for (int i = 0; i < PR_CELLS; i++) {
            uint64_t a = bank->cases[0].counts[i];
            uint64_t b = bank->cases[1].counts[i];
            if (UINT64_MAX - a < b) {
                free(bank->pool);
                bank->pool = NULL;
                return -1;
            }
            bank->pool->counts[i] = a + b;
        }
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

/* No truth or input stream is available to this function. Component quotes
   share the same immutable view of this event's recipient counts. */
static void complete_quote(const BFQuote *front, const PRLife *life,
                           const Bank *bank, Mode mode,
                           const double *inner, CompleteQuote *out) {
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
    out->inner_before = 0.0;
    out->inner_index = -1;
    if (mode == MODE_POOL) {
        if (pr_quote(life, bank->pool, pattern, log_mass, 32, &out->quote))
            fail("pooled quote");
    } else {
        out->quote = components[0];
        out->quote.selected = components[0].selected || components[1].selected;
        if (pattern) {
            int index = mode == MODE_ROW ? out->quote.row : 0;
            if (index < 0 || index >= PR_ROWS || !isfinite(inner[index]))
                fail("invalid inner posterior");
            out->inner_index = index;
            out->inner_before = inner[index];
            for (int g = 0; g < k; g++)
                out->quote.candidate_scale_log2[g] =
                    add_log2(components[0].candidate_scale_log2[g],
                             inner[index] + components[1].candidate_scale_log2[g]) -
                    add_log2(0.0, inner[index]);
            /* The protected NEW branch is exactly P0, including rounding. */
            out->quote.candidate_scale_log2[k] = out->quote.cold_scale_log2[k];
        }
    }
    for (int c = 0; c < 256; c++) {
        int g = out->groups[c] < 0 ? 0 : out->groups[c];
        double local = front->logp_base[c];
        if (components[0].cold_scale_log2[g] != components[1].cold_scale_log2[g] ||
            out->quote.cold_scale_log2[g] != components[0].cold_scale_log2[g])
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

int main(int argc, char **argv) {
    Mode mode = MODE_ROW;
    const char *mode_name = "row";
    if (argc == 3) {
        mode_name = argv[2];
        if (!strcmp(mode_name, "row")) mode = MODE_ROW;
        else if (!strcmp(mode_name, "global")) mode = MODE_GLOBAL;
        else if (!strcmp(mode_name, "pool")) mode = MODE_POOL;
        else argc = 0;
    }
    if (argc < 2 || argc > 3) {
        fputs("usage: bank BANK.bin [row|global|pool] < RAW > TSV\n", stderr);
        return 2;
    }
    Bank bank;
    if (load_bank(&bank, argv[1], mode))
        fail("invalid NETBANK1 archive, HEAD256 book, or pooled count overflow");
    BFState *front = bf_create();
    if (!front) fail("allocate frontend");
    PRLife life;
    pr_life_init_hmm(&life);
    size_t inner_cells = mode == MODE_ROW ? PR_ROWS : mode == MODE_GLOBAL ? 1 : 0;
    double *inner = inner_cells ? calloc(inner_cells, sizeof(*inner)) : NULL;
    if (inner_cells && !inner) fail("allocate inner posterior");
    double peak_gain = 0.0, max_drawdown = 0.0;
    uint64_t activation_end = UINT64_MAX;
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    fputs("t\tpattern\ttruth\tgroup\tlogbase\tlogcold\tlogA\tlogB\tlogbank\t"
          "loglive\tinner_odds_before\tshadow_before\touter_odds_before\t"
          "active_before\tactivated_after\tgain_after\n", stdout);
    for (;;) {
        BFQuote bf;
        CompleteQuote full;
        if (bf_quote(front, &bf)) fail("frontend quote");
        if (bf.t != life.events) fail("frontend/recurrence event mismatch");
        complete_quote(&bf, &life, &bank, mode, inner, &full);

        /* Base, both components, bank and outer live vectors already exist. */
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
        if (full.inner_index >= 0) {
            double delta = full.groups[truth] == full.quote.repeat_classes
                ? 0.0 : full.b[truth] - full.a[truth];
            inner[full.inner_index] += delta;
            if (!isfinite(inner[full.inner_index])) fail("inner posterior overflow");
        }
        if (bf_observe(front, truth)) fail("frontend observe");
        if (life.gain_bits > peak_gain) peak_gain = life.gain_bits;
        double drawdown = peak_gain - life.gain_bits;
        if (drawdown > max_drawdown) max_drawdown = drawdown;
        if (life.gain_bits < -1.0 - 1e-7) fail("one-bit lifetime bound");
        if (max_drawdown > 16.0 + 1e-7) fail("sixteen-bit interval bound");
        if (step.activated_after) activation_end = life.events;
        printf("%zu\t%s\t%u\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t"
               "%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g\n",
               bf.t, bf.pattern, (unsigned)truth, full.groups[truth],
               bf.logp_base[truth], full.cold[truth], full.a[truth],
               full.b[truth], full.bank[truth], full.live[truth],
               full.inner_before, step.shadow_before, full.quote.odds_before,
               step.active_before ? 1 : 0, step.activated_after ? 1 : 0,
               step.gain_after);
    }
    if (ferror(stdin) || ferror(stdout)) fail("stream I/O");
    if (fflush(stdout)) fail("flush output");
    bf_destroy(front);
    free(inner);
    free(bank.pool);
    fprintf(stderr, "bank: mode=%s bytes=%llu gain=%.17g min_gain=%.17g "
                    "max_drawdown=%.17g shadow=%.17g log2_outer_odds=%.17g "
                    "activation_end=", mode_name, (unsigned long long)life.events,
            life.gain_bits, life.minimum_gain_bits, max_drawdown,
            life.shadow_bits, life.log2_odds);
    if (activation_end == UINT64_MAX) fputs("never\n", stderr);
    else fprintf(stderr, "%llu\n", (unsigned long long)activation_end);
    return 0;
}
