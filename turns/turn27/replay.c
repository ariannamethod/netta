/* Turn27: six authority trajectories on one identical price tape and one
   shared admission.

   slow, fast, witness    turn22's frozen law, three fixed modes.
   hysteresis             turn25's latch.c, linked unchanged (the incumbent).
   cusum                  this turn's candidate (cusum.c).
   orate0                 turn24's O-rate-0: run slow, and on a switched-law
                          life use hazard 2^-10 from the seam onward. The whole
                          use of the generator's hidden label is the one guarded
                          test below; no other arm receives argv[1].

   Input:   t cold_log2 candidate_log2 matchedL, chronological, on stdin.
   argv[1]: seam byte, or -1 when this life has no law change. */
#include "cusum.h"

#include "../turn25/latch.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

enum { CONTROLS = 3, ARMS = 6, HYST = 3, CUSUM = 4, ORATE = 5 };
static const char *const names[ARMS] = {
    "slow", "fast", "witness", "hysteresis", "cusum", "orate0"};

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "authority_replay: %s\n", message);
    exit(1);
}

static void charge(Stats *s, int arm, double cold, double live,
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
    double bound = arm == AR_FAST ? 10.0 : 16.0;
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 || s->drawdown > bound + 1e-7)
        fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    if (argc != 2) fail("usage: authority_replay SEAM_OR_MINUS_ONE < EPISODE_PRICE_TAPE");
    char *end = argv[1];
    long seam = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end || (seam < 0 && seam != -1))
        fail("seam must be a non-negative byte index or -1");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");

    ARState control[CONTROLS], orate;
    LRState hyst;
    CSState cus;
    Stats stats[ARMS] = {{0, 0, 0, 0, 0, 0, 0}};
    long switch_byte = -1, latch_byte = -1, hyst_first_set = -1;
    for (int m = 0; m < CONTROLS; m++)
        if (ar_init(&control[m], (ARMode)m)) fail("control initialization");
    if (ar_init(&orate, AR_SLOW)) fail("oracle initialization");
    if (lr_init(&hyst)) fail("hysteresis initialization");
    if (cs_init(&cus)) fail("cusum initialization");

    fputs("t\tcold\tcandidate\tmatched\tshadow_before\tactive_before\tadmitted_after"
          "\tw_before\tw_after\thysteresis_latch_before\thysteresis_latch_after"
          "\ts_before\ts_after\tcusum_latch_before\tcusum_latch_after\torate0_armed",
          stdout);
    for (int m = 0; m < ARMS; m++)
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

        ARState before[ARMS], after[ARMS];
        LRState hyst_before = hyst;
        CSState cus_before = cus;
        double live[ARMS];
        int slow_used[ARMS], admitted[ARMS];
        for (int m = 0; m < CONTROLS; m++) before[m] = control[m];
        before[HYST] = hyst.core;
        before[CUSUM] = cus.core;
        before[ORATE] = orate;

        /* The oracle's whole use of the hidden label lives in this test. */
        if (seam >= 0 && (long)t == seam) {
            if (switch_byte != -1) fail("second switch on one life");
            switch_byte = (long)t;
            orate.mode = AR_FAST;
        }
        int armed = switch_byte != -1;
        if (!armed && orate.odds != control[AR_SLOW].odds)
            fail("pre-switch oracle odds left the slow reference");

        /* All six quotes precede every observation. */
        for (int m = 0; m < CONTROLS; m++)
            if (ar_quote(&control[m], cold, candidate, &live[m])) fail("control quote");
        if (lr_quote(&hyst, cold, candidate, &live[HYST])) fail("hysteresis quote");
        if (cs_quote(&cus, cold, candidate, &live[CUSUM])) fail("cusum quote");
        if (ar_quote(&orate, cold, candidate, &live[ORATE])) fail("oracle quote");
        for (int m = 0; m < ARMS; m++)
            if ((!before[m].active || candidate == cold) && live[m] != cold)
                fail("exact cold quote");
        if (!armed && live[ORATE] != live[AR_SLOW]) fail("pre-switch oracle quote left slow");

        for (int m = 0; m < CONTROLS; m++) {
            int clipped;
            if (ar_observe(&control[m], cold, candidate, matched, &slow_used[m], &admitted[m], &clipped))
                fail("control observe");
            if (clipped) fail("control clipped");
            after[m] = control[m];
        }
        if (lr_observe(&hyst, cold, candidate, matched, &slow_used[HYST], &admitted[HYST]))
            fail("hysteresis observe");
        after[HYST] = hyst.core;
        if (cs_observe(&cus, cold, candidate, matched, &slow_used[CUSUM], &admitted[CUSUM]))
            fail("cusum observe");
        after[CUSUM] = cus.core;
        {
            int clipped;
            if (ar_observe(&orate, cold, candidate, matched, &slow_used[ORATE], &admitted[ORATE], &clipped))
                fail("oracle observe");
            if (clipped) fail("oracle clipped");
            after[ORATE] = orate;
        }

        for (int m = 0; m < ARMS; m++) {
            charge(&stats[m], m, cold, live[m], t, slow_used[m], admitted[m]);
            if (before[m].shadow != before[0].shadow || before[m].active != before[0].active ||
                after[m].shadow != after[0].shadow || after[m].active != after[0].active ||
                admitted[m] != admitted[0] || stats[m].admission != stats[0].admission)
                fail("shared first admission");
        }
        if (slow_used[AR_SLOW] == 0 || slow_used[AR_FAST] == 1) fail("fixed hazard");

        /* Turn25's hysteresis chronology, replayed verbatim. */
        if (before[HYST].evidence != before[AR_WITNESS].evidence ||
            after[HYST].evidence != after[AR_WITNESS].evidence)
            fail("witness clock identity");
        unsigned wanted = hyst_before.fast_latched;
        if (before[HYST].active) {
            if (slow_used[HYST] != (hyst_before.fast_latched ? 0 : 1))
                fail("old latch hazard chronology");
            if (after[HYST].evidence <= -1.0) wanted = 1;
            else if (after[HYST].evidence >= 1.0) wanted = 0;
            if (candidate == cold && hyst_before.fast_latched && !hyst.fast_latched)
                fail("neutral observation released latch");
        } else if (admitted[HYST]) wanted = 0;
        if (hyst.fast_latched != wanted) fail("latch transition");
        if (!hyst_before.fast_latched && hyst.fast_latched && hyst_first_set == -1)
            hyst_first_set = (long)t;

        /* This turn's candidate: the sum, then the one-way latch. */
        double wanted_sum = cus_before.sum;
        unsigned wanted_latch = cus_before.latched;
        if (before[CUSUM].active) {
            if (slow_used[CUSUM] != (cus_before.latched ? 0 : 1))
                fail("old cusum latch hazard chronology");
            if (matched >= 1) {
                double raised = cus_before.sum + (-(candidate - cold) - CS_DRIFT);
                wanted_sum = raised > 0.0 ? raised : 0.0;
            }
            if (wanted_sum >= CS_THRESHOLD) wanted_latch = 1;
        } else if (admitted[CUSUM]) {
            wanted_sum = 0.0;
            wanted_latch = 0;
        }
        if (cus.sum != wanted_sum) fail("cusum statistic law");
        if (cus.latched != wanted_latch) fail("cusum latch law");
        if (cus.latched < cus_before.latched) fail("cusum latch released");
        if (cus.sum < 0.0) fail("cusum statistic left its floor");
        if (!cus_before.latched && cus.latched) {
            if (latch_byte != -1) fail("second cusum latch on one life");
            latch_byte = (long)t;
        }

        /* Turn24's O-rate-0 confinement, replayed verbatim. */
        if (!armed) {
            if (slow_used[ORATE] != slow_used[AR_SLOW] || after[ORATE].odds != after[AR_SLOW].odds)
                fail("pre-switch oracle state left the slow reference");
        } else if (slow_used[ORATE] == 1) fail("armed oracle used the slow hazard");

        printf("%zu\t%.17g\t%.17g\t%d\t%.17g\t%u\t%d\t%.17g\t%.17g\t%u\t%u"
               "\t%.17g\t%.17g\t%u\t%u\t%d",
               t, cold, candidate, matched, before[0].shadow, before[0].active, admitted[0],
               before[AR_WITNESS].evidence, after[AR_WITNESS].evidence,
               hyst_before.fast_latched, hyst.fast_latched,
               cus_before.sum, cus.sum, cus_before.latched, cus.latched, armed);
        for (int m = 0; m < ARMS; m++)
            printf("\t%.17g\t%.17g\t%d\t%.17g", live[m], before[m].odds, slow_used[m], after[m].odds);
        putchar('\n');
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");
    if (seam < 0 && switch_byte != -1) fail("switch on a life with no law change");
    if (seam >= 0 && seam < (long)expected_t && switch_byte != seam)
        fail("missing declared switch");

    fprintf(stderr, "events=%zu seam=%ld switch=%ld cusum_latch=%ld hysteresis_first_set=%ld\n",
            expected_t, seam, switch_byte, latch_byte, hyst_first_set);
    fprintf(stderr, "state_bytes ar=%zu lr=%zu cs=%zu total=%zu\n",
            sizeof(ARState), sizeof(LRState), sizeof(CSState),
            sizeof(control) + sizeof(orate) + sizeof(hyst) + sizeof(cus));
    fprintf(stderr, "cusum_sum_final=%.17g cusum_latched=%u\n", cus.sum, cus.latched);
    for (int m = 0; m < ARMS; m++)
        fprintf(stderr, "mode=%s gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
                names[m], stats[m].gain, stats[m].minimum, stats[m].drawdown,
                stats[m].admission, stats[m].slow_count, stats[m].fast_count);
    return 0;
}
