#include "recurrence.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>

/* Disjoint API fixture: gate=0 exposes transition timing without constructing
   measured data. Four distinct zero-evidence branches must still consume one
   active raw event each. Source probability uses a probability-space oracle. */
int main(void) {
    PRArchive archive = {0};
    PRLife life;
    PRQuote quote;
    PRStep step;
    const double two[PR_GROUPS] = {-2.0, -2.0, -1.0};
    const double one[PR_GROUPS] = {-1.0, -1.0};
    const double three[PR_GROUPS] = {-2.0, -2.0, -2.0, -2.0};
    const char *names[] = {"incomplete", "one_class", "unsupported", "selected_NEW"};
    const char *patterns[] = {NULL, "000000", "012012", "010101"};
    const double *masses[] = {NULL, one, three, two};
    const int outcomes[] = {-1, 0, 3, 2};
    const double log_units[] = {-8.0, -2.0, -3.0, -1.0};
    for (int i = 0; i < 64; ++i)
        assert(pr_archive_add(&archive, "010101", 0) == 0);
    pr_life_init_hmm(&life);
    assert(pr_quote(&life, &archive, "010101", two, 32, &quote) == 0);
    assert(!quote.selected);
    assert(pr_observe(&life, &quote, 0, -2.0, 0.0, &step) == 0);
    assert(step.activated_after && !step.active_before);
    assert(life.log2_odds == 0.0 && step.live_log2 == step.cold_log2);
    puts("gate event: cold, admitted next odds=1, no hazard transition");
    double source_probability = 0.5;
    double maximum_error = 0.0;
    for (int i = 0; i < 4; ++i) {
        assert(pr_quote(&life, &archive, patterns[i], masses[i], 32, &quote) == 0);
        assert(quote.active_before && quote.hmm_before);
        assert(quote.selected == (i == 3));
        assert(pr_observe(&life, &quote, outcomes[i], log_units[i], 32.0, &step) == 0);
        assert(step.candidate_log2 == step.cold_log2);
        assert(fabs(step.live_log2 - step.cold_log2) < 1e-14);
        source_probability *= 1.0 - PR_HMM_HAZARD;
        double expected = log2(source_probability / (1.0 - source_probability));
        double error = fabs(life.log2_odds - expected);
        assert(error < 1e-14);
        if (error > maximum_error) maximum_error = error;
        printf("%s selected=%d delta=%.17g log2_odds=%.17g oracle=%.17g error=%.17g\n",
               names[i], quote.selected ? 1 : 0,
               step.candidate_log2 - step.cold_log2,
               life.log2_odds, expected, error);
    }
    printf("PASS active_events=4 all_zero_evidence maximum_log2_error=%.17g\n", maximum_error);
    return 0;
}
