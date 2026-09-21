#include "authority.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

static const char *const names[AR_MODES] = {"slow", "fast", "witness", "h8l4", "selfnorm"};

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count, clip_count;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void charge(Stats *s, ARMode mode, double cold, double live,
                   size_t t, int slow_used, int admitted, int clipped) {
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
    if (clipped) s->clip_count++;
    double bound = mode == AR_FAST ? 10.0 : 16.0;
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 || s->drawdown > bound + 1e-7)
        fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) fail("usage: authority_replay < EPISODE_PRICE_TAPE");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    ARState state[AR_MODES];
    Stats stats[AR_MODES] = {{0}};
    for (int m = 0; m < AR_MODES; m++)
        if (ar_init(&state[m], (ARMode)m)) fail("state initialization");
    fputs("t\tcold\tcandidate\tmatched\tshadow_before\tactive_before\tadmitted_after", stdout);
    for (int m = 0; m < AR_MODES; m++)
        printf("\t%s_live\t%s_odds_before\t%s_slow_used\t%s_odds_after", names[m], names[m], names[m], names[m]);
    for (int m = AR_WITNESS; m < AR_MODES; m++)
        printf("\t%s_w_before\t%s_w_after", names[m], names[m]);
    fputs("\tselfnorm_a_before\tselfnorm_a_after", stdout);
    for (int m = AR_H8L4; m < AR_MODES; m++)
        printf("\t%s_cap_before\t%s_cap_after\t%s_clipped", names[m], names[m], names[m]);
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
        ARState before[AR_MODES];
        double live[AR_MODES];
        int slow_used[AR_MODES], admitted[AR_MODES], clipped[AR_MODES];
        /* All five quotes precede every observation. */
        for (int m = 0; m < AR_MODES; m++) {
            before[m] = state[m];
            if (ar_quote(&state[m], cold, candidate, &live[m])) fail("quote");
            if ((!before[m].active || candidate == cold) && live[m] != cold) fail("exact cold quote");
        }
        for (int m = 0; m < AR_MODES; m++) {
            if (ar_observe(&state[m], cold, candidate, matched, &slow_used[m], &admitted[m], &clipped[m]))
                fail("observe");
            charge(&stats[m], (ARMode)m, cold, live[m], t, slow_used[m], admitted[m], clipped[m]);
            if (state[m].shadow != state[0].shadow || state[m].active != state[0].active ||
                admitted[m] != admitted[0] || stats[m].admission != stats[0].admission)
                fail("shared first admission");
            if (m < AR_H8L4 && clipped[m]) fail("reference clipped");
        }
        if (slow_used[AR_SLOW] == 0 || slow_used[AR_FAST] == 1) fail("fixed hazard");
        for (int m = AR_H8L4; m < AR_MODES; m++) {
            if (before[m].evidence != before[AR_WITNESS].evidence ||
                state[m].evidence != state[AR_WITNESS].evidence ||
                slow_used[m] != slow_used[AR_WITNESS]) fail("witness clock identity");
            if (before[m].odds > ar_cap(&before[m]) + 1e-10 || state[m].odds > ar_cap(&state[m]) + 1e-10)
                fail("prospective cap bound");
        }
        printf("%zu\t%.17g\t%.17g\t%d\t%.17g\t%u\t%d", t, cold, candidate, matched,
               before[0].shadow, before[0].active, admitted[0]);
        for (int m = 0; m < AR_MODES; m++)
            printf("\t%.17g\t%.17g\t%d\t%.17g", live[m], before[m].odds, slow_used[m], state[m].odds);
        for (int m = AR_WITNESS; m < AR_MODES; m++)
            printf("\t%.17g\t%.17g", before[m].evidence, state[m].evidence);
        printf("\t%.17g\t%.17g", before[AR_SELFNORM].absolute, state[AR_SELFNORM].absolute);
        for (int m = AR_H8L4; m < AR_MODES; m++)
            printf("\t%.17g\t%.17g\t%d", ar_cap(&before[m]), ar_cap(&state[m]), clipped[m]);
        putchar('\n');
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    fprintf(stderr, "events=%zu state_bytes=%zu total_state_bytes=%zu\n", expected_t, sizeof(ARState), sizeof(state));
    for (int m = 0; m < AR_MODES; m++)
        fprintf(stderr, "mode=%s gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu clipped=%zu\n",
                names[m], stats[m].gain, stats[m].minimum, stats[m].drawdown,
                stats[m].admission, stats[m].slow_count, stats[m].fast_count, stats[m].clip_count);
    return 0;
}
