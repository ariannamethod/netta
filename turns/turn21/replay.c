#include "authority.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count, clip_count;
} Stats;

static char names[AR_MODES][16];

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void name_modes(void) {
    snprintf(names[AR_SLOW], sizeof(names[0]), "slow");
    snprintf(names[AR_FAST], sizeof(names[0]), "fast");
    snprintf(names[AR_WITNESS], sizeof(names[0]), "witness");
    for (int i = 0; i < AR_GRID_POINTS; i++)
        snprintf(names[AR_GRID + i], sizeof(names[0]), "h%dl%d",
                 ar_grid[i].high, ar_grid[i].low);
}

/* The capped modes can never carry more than `high` bits of odds, so their
   cold component never falls below 1/(2^high+1); the references keep the
   hazard bounds turn18 and turn20 used. */
static double drawdown_bound(ARMode mode) {
    if (mode == AR_FAST) return 10.0;
    if (mode < AR_GRID) return 16.0;
    return log2(exp2((double)ar_grid[mode - AR_GRID].high) + 1.0);
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
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 ||
        s->drawdown > drawdown_bound(mode) + 1e-7) fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) fail("usage: authority_replay < EPISODE_PRICE_TAPE");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    name_modes();
    ARState state[AR_MODES];
    Stats stats[AR_MODES] = {{0, 0, 0, 0, 0, 0, 0, 0}};
    double previous[AR_MODES] = {0};
    for (int m = 0; m < AR_MODES; m++)
        if (ar_init(&state[m], (ARMode)m)) fail("state initialization");
    fputs("t\tcold\tcandidate\tmatched\tshadow_before\tactive_before"
          "\tadmitted_after", stdout);
    for (int m = AR_SLOW; m <= AR_WITNESS; m++)
        printf("\t%s_live\t%s_odds_before\t%s_odds_after",
               names[m], names[m], names[m]);
    fputs("\twitness_slow_used\twitness_w_before\twitness_w_after", stdout);
    /* Every grid point carries the witness clock exactly, so w and the hazard
       choice are quoted once; each point's own quote, next odds and cap
       decision are quoted in full. odds_before is the previous odds_after,
       checked as such below rather than repeated 24 times per byte. */
    for (int i = 0; i < AR_GRID_POINTS; i++)
        printf("\t%s_live\t%s_odds_after\t%s_clipped",
               names[AR_GRID + i], names[AR_GRID + i], names[AR_GRID + i]);
    fputs("\n", stdout);
    size_t expected_t = 0, carry_checks = 0, clock_checks = 0, bound_checks = 0;
    char line[512];
    while (fgets(line, sizeof(line), stdin)) {
        size_t t;
        double cold, candidate;
        int matched;
        char surplus;
        int fields = sscanf(line, "%zu %lf %lf %d %c",
                            &t, &cold, &candidate, &matched, &surplus);
        if (fields != 4 || t != expected_t || matched < 0)
            fail("expected chronological t, two log prices and a match length");
        ARState before[AR_MODES];
        double live[AR_MODES];
        int slow_used[AR_MODES], admitted[AR_MODES], clipped[AR_MODES];
        for (int m = 0; m < AR_MODES; m++) {
            before[m] = state[m];
            if (before[m].odds != previous[m]) fail("carried odds continuity");
            carry_checks++;
            if (ar_quote(&state[m], cold, candidate, &live[m])) fail("quote");
            if (candidate == cold && live[m] != cold) fail("exact equal price");
            if (!before[m].active && live[m] != cold) fail("exact inactive price");
        }
        for (int m = 0; m < AR_MODES; m++) {
            if (ar_observe(&state[m], cold, candidate, matched,
                           &slow_used[m], &admitted[m], &clipped[m])) fail("observe");
            charge(&stats[m], (ARMode)m, cold, live[m], t,
                   slow_used[m], admitted[m], clipped[m]);
            if (state[m].shadow != state[0].shadow ||
                state[m].active != state[0].active ||
                admitted[m] != admitted[0] ||
                stats[m].admission != stats[0].admission)
                fail("shared first admission");
            previous[m] = state[m].odds;
        }
        if (slow_used[AR_SLOW] == 0 || slow_used[AR_FAST] == 1)
            fail("fixed control hazard");
        for (int m = AR_SLOW; m < AR_GRID; m++)
            if (clipped[m]) fail("reference mode clipped");
        for (int i = 0; i < AR_GRID_POINTS; i++) {
            int m = AR_GRID + i;
            if (before[m].evidence != before[AR_WITNESS].evidence ||
                state[m].evidence != state[AR_WITNESS].evidence ||
                slow_used[m] != slow_used[AR_WITNESS])
                fail("grid/witness clock identity");
            clock_checks++;
            double limit = state[m].evidence <= -1.0 ? (double)ar_grid[i].low
                                                     : (double)ar_grid[i].high;
            if (before[m].odds > (double)ar_grid[i].high || state[m].odds > limit)
                fail("grid odds bound");
            bound_checks++;
        }
        printf("%zu\t%.17g\t%.17g\t%d\t%.17g\t%u\t%d",
               t, cold, candidate, matched, before[0].shadow, before[0].active,
               admitted[0]);
        for (int m = AR_SLOW; m <= AR_WITNESS; m++)
            printf("\t%.17g\t%.17g\t%.17g", live[m], before[m].odds, state[m].odds);
        printf("\t%d\t%.17g\t%.17g", slow_used[AR_WITNESS],
               before[AR_WITNESS].evidence, state[AR_WITNESS].evidence);
        for (int i = 0; i < AR_GRID_POINTS; i++)
            printf("\t%.17g\t%.17g\t%d", live[AR_GRID + i], state[AR_GRID + i].odds,
                   clipped[AR_GRID + i]);
        fputs("\n", stdout);
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    fprintf(stderr, "events=%zu modes=%d state_bytes=%zu total_state_bytes=%zu"
            " carry_checks=%zu clock_identity_checks=%zu odds_bound_checks=%zu\n",
            expected_t, (int)AR_MODES, sizeof(ARState), sizeof(state),
            carry_checks, clock_checks, bound_checks);
    for (int m = 0; m < AR_MODES; m++)
        fprintf(stderr, "mode=%s gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu clipped=%zu\n",
                names[m], stats[m].gain, stats[m].minimum, stats[m].drawdown,
                stats[m].admission, stats[m].slow_count, stats[m].fast_count,
                stats[m].clip_count);
    return 0;
}
