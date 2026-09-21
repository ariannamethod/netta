#include "authority.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void charge(Stats *s, ARMode mode, double cold, double live,
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
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 ||
        s->drawdown > bound + 1e-7) fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) fail("usage: authority_replay < EPISODE_PRICE_TAPE");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    ARState state[AR_MODES];
    Stats stats[AR_MODES] = {{0}};
    for (int m = 0; m < AR_MODES; m++)
        if (ar_init(&state[m], (ARMode)m)) fail("state initialization");
    fputs("t\tcold\tcandidate\tshadow_before\tactive_before\tadmitted_after"
          "\tslow_live\tslow_odds_before\tfast_live\tfast_odds_before"
          "\tadaptive_live\tadaptive_odds_before\tadaptive_e_before"
          "\tadaptive_slow_used\tadaptive_e_after\tadaptive_odds_after\n", stdout);
    size_t expected_t = 0;
    char line[512];
    while (fgets(line, sizeof(line), stdin)) {
        size_t t;
        double cold, candidate;
        char surplus;
        int fields = sscanf(line, "%zu %lf %lf %c", &t, &cold, &candidate, &surplus);
        if (fields != 3 || t != expected_t)
            fail("expected chronological t and two log prices");
        ARState before[AR_MODES];
        double live[AR_MODES];
        int slow_used[AR_MODES], admitted[AR_MODES];
        for (int m = 0; m < AR_MODES; m++) {
            before[m] = state[m];
            if (ar_quote(&state[m], cold, candidate, &live[m])) fail("quote");
            if (candidate == cold && live[m] != cold) fail("exact equal price");
        }
        for (int m = 0; m < AR_MODES; m++) {
            if (ar_observe(&state[m], cold, candidate,
                           &slow_used[m], &admitted[m])) fail("observe");
            charge(&stats[m], (ARMode)m, cold, live[m], t,
                   slow_used[m], admitted[m]);
            if (state[m].shadow != state[0].shadow ||
                state[m].active != state[0].active ||
                admitted[m] != admitted[0] ||
                stats[m].admission != stats[0].admission)
                fail("shared first admission");
        }
        printf("%zu\t%.17g\t%.17g\t%.17g\t%u\t%d",
               t, cold, candidate, before[0].shadow, before[0].active,
               admitted[0]);
        for (int m = 0; m < AR_MODES; m++)
            printf("\t%.17g\t%.17g", live[m], before[m].odds);
        printf("\t%.17g\t%d\t%.17g\t%.17g\n",
               before[AR_ADAPTIVE].evidence, slow_used[AR_ADAPTIVE],
               state[AR_ADAPTIVE].evidence, state[AR_ADAPTIVE].odds);
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    fprintf(stderr, "events=%zu state_bytes=%zu\n", expected_t, sizeof(ARState));
    for (int m = 0; m < AR_MODES; m++)
        fprintf(stderr, "mode=%d gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
                m, stats[m].gain, stats[m].minimum, stats[m].drawdown,
                stats[m].admission, stats[m].slow_count, stats[m].fast_count);
    return 0;
}
