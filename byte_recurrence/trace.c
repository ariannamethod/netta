#include "frontend.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void fail(const char *message) {
    fprintf(stderr, "byte_trace: %s\n", message);
    exit(1);
}

static void hex(const uint8_t *bytes, size_t n) {
    static const char digits[] = "0123456789abcdef";
    for (size_t i = 0; i < n; i++) {
        putchar(digits[bytes[i] >> 4]);
        putchar(digits[bytes[i] & 15]);
    }
}

/* Disjoint finite implementation fixtures, not any evaluation world. */
static int selftest(void) {
    BFState *a = bf_create(), *b = bf_create();
    if (!a || !b) fail("selftest allocate");
    if (bf_observe(a, 0) == 0) fail("observe accepted without quote");
    double maximum = 0;
    size_t reference_checks = 0, learned_quotes = 0;
    static const uint8_t tape[] = "ababacabadabacaba0123012304560456xyz xyz zyxx ";
    for (size_t t = 0; t <= 2050; t++) {
        BFQuote qa, qb, repeat;
        if (bf_quote(a, &qa) || bf_quote(b, &qb) || bf_quote(a, &repeat)) fail("selftest quote");
        if (memcmp(&qa, &qb, sizeof(qa)) || memcmp(&qa, &repeat, sizeof(qa)))
            fail("common-prefix/idempotent quote differs");
        if (qa.t != t || qa.trained_bytes != (t / BF_REBUILD) * BF_REBUILD)
            fail("selftest training chronology");
        double sum = 0;
        for (int c = 0; c < 256; c++) sum += exp2(qa.logp_base[c]);
        if (fabs(sum - 1.0) > 1e-8) fail("selftest normalization");
        if (qa.nunits > 256) learned_quotes++;
        if (t == 0 || t == 6 || t == 255 || t == 256 || t == 257 ||
            t == 1023 || t == 1024 || t == 1025 || t == 2048 || t == 2050) {
            double error = bf_reference_error(a, &qa);
            if (error < 0 || error > 1e-10) fail("projected vector differs from core unit-price sum");
            if (error > maximum) maximum = error;
            reference_checks++;
        }
        if (t == 2050) {
            /* The current quote is complete before either future arrives. */
            if (bf_observe(a, 17) || bf_observe(b, 201)) fail("diverging futures observe");
            break;
        }
        uint8_t observed = tape[t % (sizeof(tape) - 1)];
        if (bf_observe(a, observed) || bf_observe(b, observed)) fail("selftest observe");
    }
    if (!learned_quotes) fail("fixtures never exercised learned units");
    bf_destroy(a);
    bf_destroy(b);
    printf("HEAD256 fixture PASS: 2051 common-prefix quotes; %zu learned-unit quotes; "
           "%zu explicit core-vector comparisons; max_log2_error=%.17g\n",
           learned_quotes, reference_checks, maximum);
    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--selftest") == 0) return selftest();
    if (argc != 1) {
        fprintf(stderr, "usage: byte_trace < INPUT.bin > TRACE.tsv\n"
                        "       byte_trace --selftest\n");
        return 2;
    }
    BFState *state = bf_create();
    if (!state) fail("allocate frontend");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    fputs("t\ttrained_bytes\tnunits\tcontext_len\tpattern\theads\tunit_ids\tunit_hex\t"
          "truth\tgroup\tlogp_base_truth", stdout);
    for (int c = 0; c < 256; c++) printf("\tlogp%d", c);
    putchar('\n');
    size_t total = 0;
    for (;;) {
        BFQuote quote;
        if (bf_quote(state, &quote)) fail("quote");
        /* The only read of the input stream occurs after the complete quote. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        int group = -1;
        if (quote.context_len == BF_DEPTH) {
            group = 0;
            while (group < quote.repeat_classes && quote.heads[group] != truth) group++;
        }
        printf("%zu\t%zu\t%u\t%d\t%s\t", quote.t, quote.trained_bytes,
               quote.nunits, quote.context_len, quote.pattern);
        if (quote.context_len < BF_DEPTH) putchar('-');
        else for (int i = 0; i < quote.repeat_classes; i++) {
            if (i) putchar(',');
            printf("%u", (unsigned)quote.heads[i]);
        }
        putchar('\t');
        if (!quote.context_len) putchar('-');
        for (int i = 0; i < quote.context_len; i++) {
            if (i) putchar(',');
            printf("%u", quote.unit_ids[i]);
        }
        putchar('\t');
        if (!quote.context_len) putchar('-');
        for (int i = 0; i < quote.context_len; i++) {
            if (i) putchar(',');
            hex(quote.unit_bytes[i], quote.unit_lengths[i]);
        }
        printf("\t%u\t%d\t%.17g", (unsigned)truth, group, quote.logp_base[truth]);
        for (int c = 0; c < 256; c++) printf("\t%.17g", quote.logp_base[c]);
        putchar('\n');
        if (bf_observe(state, truth)) fail("observe");
        total++;
    }
    if (ferror(stdin) || ferror(stdout)) fail("stream I/O");
    bf_destroy(state);
    if (fflush(stdout)) fail("flush output");
    fprintf(stderr, "byte_trace: %zu bytes, %zu sequential quotes\n", total, total);
    return 0;
}
