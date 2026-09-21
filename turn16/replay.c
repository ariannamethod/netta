#include "authority.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define NARMS 7

static const char *const arms[NARMS] = {
    "episode", "isolated", "frequency", "reverse", "permuted", "flat", "row"
};
static const char *const modes[AR_MODES] = {"slow", "fast", "budget", "return"};

/* Measurement state is deliberately outside ARState. */
typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission;
    unsigned returns;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void charge(Stats *s, ARMode mode, double cold, double live,
                   size_t t, AREvent event) {
    s->gain += live - cold;
    s->minimum = fmin(s->minimum, s->gain);
    s->peak = fmax(s->peak, s->gain);
    s->drawdown = fmax(s->drawdown, s->peak - s->gain);
    if (event == AR_ADMITTED) {
        if (s->admission) fail("more than one initial admission");
        s->admission = t + 1;
    }
    if (event == AR_RETURNED && ++s->returns > 1) fail("more than one return");
    double bound = mode == AR_SLOW ? 16.0 : mode == AR_RETURN ? 10.5 : 10.0;
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 || s->drawdown > bound + 1e-7)
        fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) fail("usage: authority_replay < PRICE_TAPE");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    ARState state[NARMS][AR_MODES];
    Stats stats[NARMS][AR_MODES] = {{{0}}};
    for (int a = 0; a < NARMS; a++)
        for (int m = 0; m < AR_MODES; m++)
            if (ar_init(&state[a][m], (ARMode)m)) fail("state initialization");
    fputs("t\tarm\tcold\tcandidate\tshadow_before\tactive_before\tadmitted_after"
          "\tslow_live\tslow_odds_before\tfast_live\tfast_odds_before"
          "\tbudget_live\tbudget_odds_before\treturn_live\treturn_odds_before"
          "\treturn_armed_before\treturn_support_before\treturn_used_before"
          "\treturn_event\treturn_odds_after\treturn_support_after"
          "\treturn_armed_after\treturn_used_after\n", stdout);
    size_t expected_t = 0;
    char line[4096];
    while (fgets(line, sizeof(line), stdin)) {
        size_t t;
        double cold, candidate[NARMS];
        char surplus;
        int fields = sscanf(line, "%zu %lf %lf %lf %lf %lf %lf %lf %lf %c",
                            &t, &cold, &candidate[0], &candidate[1], &candidate[2],
                            &candidate[3], &candidate[4], &candidate[5], &candidate[6], &surplus);
        if (fields != 9 || t != expected_t) fail("expected chronological t and eight log prices");
        ARState before[NARMS][AR_MODES];
        double live[NARMS][AR_MODES];
        /* Every arm/mode quote is complete before any state observes. */
        for (int a = 0; a < NARMS; a++) {
            for (int m = 0; m < AR_MODES; m++) {
                before[a][m] = state[a][m];
                if (ar_quote(&state[a][m], cold, candidate[a], &live[a][m])) fail("quote");
                if (candidate[a] == cold && live[a][m] != cold) fail("exact equal-price identity");
            }
        }
        for (int a = 0; a < NARMS; a++) {
            AREvent event[AR_MODES];
            for (int m = 0; m < AR_MODES; m++) {
                if (before[a][m].shadow != before[a][0].shadow ||
                    before[a][m].active != before[a][0].active) fail("shared prequote admission/shadow");
                if (ar_observe(&state[a][m], cold, candidate[a], &event[m])) fail("observe");
                charge(&stats[a][m], (ARMode)m, cold, live[a][m], t, event[m]);
                if (state[a][m].shadow != state[a][0].shadow ||
                    state[a][m].active != state[a][0].active ||
                    (event[m] == AR_ADMITTED) != (event[0] == AR_ADMITTED) ||
                    stats[a][m].admission != stats[a][0].admission)
                    fail("shared postobservation admission/shadow");
            }
            const ARState *old = &before[a][AR_RETURN], *now = &state[a][AR_RETURN];
            printf("%zu\t%s\t%.17g\t%.17g\t%.17g\t%u\t%d",
                   t, arms[a], cold, candidate[a], old->shadow, old->active,
                   event[0] == AR_ADMITTED);
            for (int m = 0; m < AR_MODES; m++)
                printf("\t%.17g\t%.17g", live[a][m], before[a][m].odds);
            printf("\t%u\t%.17g\t%u\t%d\t%.17g\t%.17g\t%u\t%u\n",
                   old->armed, old->support, old->used, event[AR_RETURN] == AR_RETURNED,
                   now->odds, now->support, now->armed, now->used);
        }
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    fprintf(stderr, "authority_replay: events=%zu arms=%d modes=%d sizeof_ARState=%zu persistent_state_bytes=%zu\n",
            expected_t, NARMS, AR_MODES, sizeof(ARState), sizeof(state));
    for (int a = 0; a < NARMS; a++) for (int m = 0; m < AR_MODES; m++) {
        const Stats *s = &stats[a][m];
        fprintf(stderr, "%s/%s gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu returns=%u\n",
                arms[a], modes[m], s->gain, s->minimum, s->drawdown, s->admission, s->returns);
    }
    return 0;
}
