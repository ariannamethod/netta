#include "latch.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum { CONTROLS = 3, MODES = 4 };
static const char *const names[MODES] = {"slow", "fast", "witness", "latch"};

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void charge(Stats *s, int mode, double cold, double live,
                   size_t t, int slow_used, int admitted) {
    s->gain += live - cold;
    s->minimum = fmin(s->minimum, s->gain);
    s->peak = fmax(s->peak, s->gain);
    s->drawdown = fmax(s->drawdown, s->peak - s->gain);
    if (admitted) {
        if (s->admission) fail("more than one first admission");
        s->admission = t + 1;
    }
    if (slow_used == 1) s->slow_count++;
    if (slow_used == 0) s->fast_count++;
    double bound = mode == AR_FAST ? 10.0 : 16.0;
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 || s->drawdown > bound + 1e-7)
        fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) fail("usage: authority_replay < EPISODE_PRICE_TAPE");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    ARState control[CONTROLS];
    LRState latch;
    Stats stats[MODES] = {{0}};
    for (int m = 0; m < CONTROLS; m++)
        if (ar_init(&control[m], (ARMode)m)) fail("control initialization");
    if (lr_init(&latch)) fail("latch initialization");
    fputs("t\tcold\tcandidate\tmatched\tshadow_before\tactive_before\tadmitted_after"
          "\tw_before\tw_after\tlatch_before\tlatch_after", stdout);
    for (int m = 0; m < MODES; m++)
        printf("\t%s_live\t%s_odds_before\t%s_slow_used\t%s_odds_after",
               names[m], names[m], names[m], names[m]);
    putchar('\n');
    char line[512];
    size_t expected_t = 0;
    while (fgets(line, sizeof(line), stdin)) {
        size_t t;
        double cold, candidate;
        int matched;
        char surplus;
        int fields = sscanf(line, "%zu %lf %lf %d %c", &t, &cold, &candidate, &matched, &surplus);
        if (fields != 4 || t != expected_t || matched < 0)
            fail("expected chronological t, two log prices and a match length");
        ARState before[MODES], after[MODES];
        LRState latch_before = latch;
        double live[MODES];
        int slow_used[MODES], admitted[MODES];
        /* All four quotes precede every observation. */
        for (int m = 0; m < CONTROLS; m++) {
            before[m] = control[m];
            if (ar_quote(&control[m], cold, candidate, &live[m])) fail("control quote");
        }
        before[CONTROLS] = latch.core;
        if (lr_quote(&latch, cold, candidate, &live[CONTROLS])) fail("latch quote");
        for (int m = 0; m < MODES; m++)
            if ((!before[m].active || candidate == cold) && live[m] != cold)
                fail("exact cold quote");
        for (int m = 0; m < CONTROLS; m++) {
            int clipped;
            if (ar_observe(&control[m], cold, candidate, matched, &slow_used[m], &admitted[m], &clipped))
                fail("control observe");
            if (clipped) fail("control clipped");
            after[m] = control[m];
        }
        if (lr_observe(&latch, cold, candidate, matched, &slow_used[CONTROLS], &admitted[CONTROLS]))
            fail("latch observe");
        after[CONTROLS] = latch.core;
        for (int m = 0; m < MODES; m++) {
            charge(&stats[m], m, cold, live[m], t, slow_used[m], admitted[m]);
            if (before[m].shadow != before[0].shadow || before[m].active != before[0].active ||
                after[m].shadow != after[0].shadow || after[m].active != after[0].active ||
                admitted[m] != admitted[0] || stats[m].admission != stats[0].admission)
                fail("shared first admission");
        }
        if (slow_used[AR_SLOW] == 0 || slow_used[AR_FAST] == 1) fail("fixed hazard");
        if (before[CONTROLS].evidence != before[AR_WITNESS].evidence ||
            after[CONTROLS].evidence != after[AR_WITNESS].evidence)
            fail("witness clock identity");
        unsigned wanted = latch_before.fast_latched;
        if (before[CONTROLS].active) {
            if (slow_used[CONTROLS] != (latch_before.fast_latched ? 0 : 1))
                fail("old latch hazard chronology");
            if (after[CONTROLS].evidence <= -1.0) wanted = 1;
            else if (after[CONTROLS].evidence >= 1.0) wanted = 0;
            if (candidate == cold && latch_before.fast_latched && !latch.fast_latched)
                fail("neutral observation released latch");
        } else if (admitted[CONTROLS]) wanted = 0;
        if (latch.fast_latched != wanted) fail("latch transition");
        printf("%zu\t%.17g\t%.17g\t%d\t%.17g\t%u\t%d\t%.17g\t%.17g\t%u\t%u",
               t, cold, candidate, matched, before[0].shadow, before[0].active, admitted[0],
               before[AR_WITNESS].evidence, after[AR_WITNESS].evidence,
               latch_before.fast_latched, latch.fast_latched);
        for (int m = 0; m < MODES; m++)
            printf("\t%.17g\t%.17g\t%d\t%.17g", live[m], before[m].odds, slow_used[m], after[m].odds);
        putchar('\n');
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    fprintf(stderr, "events=%zu control_state_bytes=%zu latch_state_bytes=%zu total_state_bytes=%zu\n",
            expected_t, sizeof(ARState), sizeof(LRState), sizeof(control) + sizeof(latch));
    for (int m = 0; m < MODES; m++)
        fprintf(stderr, "mode=%s gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
                names[m], stats[m].gain, stats[m].minimum, stats[m].drawdown,
                stats[m].admission, stats[m].slow_count, stats[m].fast_count);
    return 0;
}
