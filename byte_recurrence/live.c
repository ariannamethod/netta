/* HEAD256-v1 live composition. Quote all 256 outcomes before reading stdin.
   odds_before in the TSV is log2 posterior odds, matching PRQuote's field. */
#include "frontend.h"
#include "../portable_recurrence/recurrence.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(BF_DEPTH == PR_DEPTH, "frontend and recurrence depths differ");

static void fail(const char *message) {
    fprintf(stderr, "byte_live: %s\n", message);
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

/* Shared cell layout does not make the old unit-event archive a byte archive.
   The observation-law magic is checked before any recipient byte is read. */
static int load_head_archive(PRArchive *archive, const char *path) {
    unsigned char bytes[PR_ARCHIVE_BYTES];
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    size_t n = fread(bytes, 1, sizeof(bytes), f);
    int extra = fgetc(f);
    int bad = ferror(f);
    if (fclose(f)) bad = 1;
    if (bad || n != sizeof(bytes) || extra != EOF ||
        memcmp(bytes, "NETHD256", 8) ||
        little32(bytes + 8) != PR_DEPTH ||
        little32(bytes + 12) != PR_CELLS) return -1;
    for (int i = 0; i < PR_CELLS; i++)
        archive->counts[i] = little64(bytes + 16 + 8 * i);
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
    double total = 0;
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

/* No truth/input-stream argument: maps all possible bytes and prices all of
   them under the current local history before the first read of this event. */
static void complete_quote(const BFQuote *front, const PRLife *life,
                            const PRArchive *archive, PRQuote *quote,
                            int groups[256], double cold[256],
                            double candidate[256], double live[256]) {
    double log_mass[PR_GROUPS] = {0};
    const char *pattern = NULL;
    if (front->context_len == PR_DEPTH) {
        int k = front->repeat_classes;
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
        if (!isfinite(named_mass) || !(named_mass < 0))
            fail("named classes leave no NEW mass");
        log_mass[k] = log2(-expm1(named_mass * log(2.0)));
        for (int c = 0; c < 256; c++) {
            int g = 0;
            while (g < k && front->heads[g] != (uint8_t)c) g++;
            groups[c] = g;
        }
    } else {
        if (front->context_len < 0 || front->context_len > PR_DEPTH)
            fail("invalid context length");
        for (int c = 0; c < 256; c++) groups[c] = -1;
    }
    if (pr_quote(life, archive, pattern, log_mass, 32, quote))
        fail("recurrence quote");
    if (pattern && quote->repeat_classes != front->repeat_classes)
        fail("frontend and recurrence pattern disagree");

    for (int c = 0; c < 256; c++) {
        int g = groups[c] < 0 ? 0 : groups[c];
        cold[c] = front->logp_base[c] + quote->cold_scale_log2[g];
        candidate[c] = front->logp_base[c] + quote->candidate_scale_log2[g];
        live[c] = quote->active_before
            ? add_log2(cold[c], quote->odds_before + candidate[c]) -
              add_log2(0.0, quote->odds_before)
            : cold[c];
        if (pattern && g == front->repeat_classes &&
            fabs(exp2(candidate[c]) - exp2(cold[c])) > 1e-10)
            fail("NEW probability differs from cold");
    }
    validate_vector(front->logp_base);
    validate_vector(cold);
    validate_vector(candidate);
    validate_vector(live);
}

int main(int argc, char **argv) {
    if (argc < 2 || argc > 3 || (argc == 3 && strcmp(argv[2], "--hmm") != 0)) {
        fprintf(stderr, "usage: byte_live HEAD256_ARCHIVE.bin [--hmm] < INPUT.bin > LIVE.tsv\n"
                        "       archive magic must be NETHD256; NETTARM1 is refused\n"
                        "       TSV odds_before is log2 posterior odds\n");
        return 2;
    }
    PRArchive archive;
    if (load_head_archive(&archive, argv[1]))
        fail("invalid HEAD256-v1 archive (requires NETHD256, depth 6, 877 cells)");
    BFState *front = bf_create();
    if (!front) fail("allocate frontend");
    PRLife life;
    if (argc == 3) pr_life_init_hmm(&life);
    else pr_life_init(&life);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    fputs("t\tpattern\ttruth\tgroup\tlogbase\tlogcold\tlogcandidate\tloglive\t"
          "shadow_before\todds_before\tactive_before\tactivated_after\tgain_after\n",
          stdout);
    uint64_t activation_end = UINT64_MAX;
    for (;;) {
        BFQuote bf;
        PRQuote quote;
        int groups[256];
        double cold[256], candidate[256], live[256];
        if (bf_quote(front, &bf)) fail("frontend quote");
        if (bf.t != life.events) fail("frontend/recurrence event mismatch");
        complete_quote(&bf, &life, &archive, &quote, groups,
                       cold, candidate, live);

        /* Entire base/P0/P2/live vectors now exist and have passed their
           normalization checks. This is the only recipient input read. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        PRStep step;
        if (pr_observe(&life, &quote, groups[truth],
                       bf.logp_base[truth], 32.0, &step))
            fail("recurrence observe");
        if (step.cold_log2 != cold[truth] ||
            step.candidate_log2 != candidate[truth] ||
            step.live_log2 != live[truth])
            fail("observed price differs from completed pre-truth vector");
        if (bf_observe(front, truth)) fail("frontend observe");
        if (life.gain_bits < -1.0 - 1e-7) fail("one-bit lifetime bound");
        if (step.activated_after) activation_end = life.events;
        printf("%zu\t%s\t%u\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t"
               "%.17g\t%.17g\t%d\t%d\t%.17g\n",
               bf.t, bf.pattern, (unsigned)truth, groups[truth],
               bf.logp_base[truth], cold[truth], candidate[truth], live[truth],
               step.shadow_before, quote.odds_before,
               step.active_before ? 1 : 0, step.activated_after ? 1 : 0,
               step.gain_after);
    }
    if (ferror(stdin) || ferror(stdout)) fail("stream I/O");
    if (fflush(stdout)) fail("flush output");
    bf_destroy(front);
    fprintf(stderr, "byte_live: bytes=%llu gain=%.17g min_gain=%.17g "
                    "shadow=%.17g log2_odds=%.17g activation_end=",
            (unsigned long long)life.events, life.gain_bits,
            life.minimum_gain_bits, life.shadow_bits, life.log2_odds);
    if (activation_end == UINT64_MAX) fputs("never\n", stderr);
    else fprintf(stderr, "%llu\n", (unsigned long long)activation_end);
    return 0;
}
