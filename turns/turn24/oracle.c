/* Turn24 oracle arms. Non-causal by declaration: the only input beyond the
   shared price tape is the seam byte of a switched-law life, read from the
   generator's hidden labels. The hazard substrate is turn22's frozen
   authority, linked unchanged; this file adds the latency law and nothing
   else.

   O-rate-d  : run slow; on a switched-law life use hazard 2^-10 from seam+d.
   O-level-d : as O-rate-d, and at seam+d clamp the odds once to the fast
               reference's carried odds at that same byte of the same life.

   Input:  t cold_log2 candidate_log2 matchedL, chronological, on stdin.
   argv[1]: seam byte, or -1 when this life has no law change. */
#include "../turn22/authority.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define ARMS 8

static const long DELAY[ARMS] = {0, 32, 128, 512, 0, 32, 128, 512};
static const int LEVEL[ARMS] = {0, 0, 0, 0, 1, 1, 1, 1};
static const char *const NAMES[ARMS] = {
    "orate0", "orate32", "orate128", "orate512",
    "olevel0", "olevel32", "olevel128", "olevel512"};

typedef struct {
    double gain, minimum, peak, drawdown;
    size_t admission, slow_count, fast_count;
} Stats;

static void fail(const char *message) {
    fprintf(stderr, "oracle: %s\n", message);
    exit(1);
}

static void charge(Stats *s, double bound, double cold, double live,
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
    if (!isfinite(s->gain) || s->minimum < -1.0 - 1e-7 || s->drawdown > bound + 1e-7)
        fail("prefix/interval loss bound");
}

int main(int argc, char **argv) {
    if (argc != 2) fail("usage: oracle SEAM_OR_MINUS_ONE < EPISODE_PRICE_TAPE");
    char *end = argv[1];
    long seam = strtol(argv[1], &end, 10);
    if (end == argv[1] || *end || (seam < 0 && seam != -1))
        fail("seam must be a non-negative byte index or -1");
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");

    ARState slow, fast, arm[ARMS];
    Stats slow_stats = {0.0, 0.0, 0.0, 0.0, 0, 0, 0};
    Stats fast_stats = {0.0, 0.0, 0.0, 0.0, 0, 0, 0};
    Stats arm_stats[ARMS];
    double clamp_value[ARMS];
    long switch_byte[ARMS];
    int clamp_seen[ARMS];
    if (ar_init(&slow, AR_SLOW) || ar_init(&fast, AR_FAST)) fail("state initialization");
    for (int i = 0; i < ARMS; i++) {
        if (ar_init(&arm[i], AR_SLOW)) fail("state initialization");
        Stats blank = {0.0, 0.0, 0.0, 0.0, 0, 0, 0};
        arm_stats[i] = blank;
        clamp_value[i] = 0.0;
        switch_byte[i] = -1;
        clamp_seen[i] = 0;
    }

    fputs("t\tcold\tcandidate\tmatched\tshadow_before\tactive_before\tadmitted_after", stdout);
    fputs("\tslow_live\tslow_odds_before\tslow_slow_used\tslow_odds_after", stdout);
    fputs("\tfast_live\tfast_odds_before\tfast_slow_used\tfast_odds_after", stdout);
    for (int i = 0; i < ARMS; i++)
        printf("\t%s_live\t%s_odds_carried\t%s_odds_before\t%s_slow_used\t%s_odds_after\t%s_clamped",
               NAMES[i], NAMES[i], NAMES[i], NAMES[i], NAMES[i], NAMES[i]);
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

        ARState slow_before = slow, fast_before = fast;
        double carried[ARMS];
        int clamped[ARMS], armed[ARMS];
        for (int i = 0; i < ARMS; i++) {
            carried[i] = arm[i].odds;
            clamped[i] = 0;
            /* The oracle's whole use of the hidden label lives in this test. */
            if (seam >= 0 && (long)t == seam + DELAY[i]) {
                if (switch_byte[i] != -1) fail("second switch on one life");
                switch_byte[i] = (long)t;
                if (LEVEL[i]) {
                    arm[i].odds = fast_before.odds;
                    clamp_value[i] = arm[i].odds;
                    clamp_seen[i] = 1;
                    clamped[i] = 1;
                }
                arm[i].mode = AR_FAST;
            }
            armed[i] = switch_byte[i] != -1;
            if (!armed[i] && arm[i].odds != slow_before.odds)
                fail("pre-switch oracle odds left the slow reference");
        }

        double slow_live, fast_live, live[ARMS];
        if (ar_quote(&slow, cold, candidate, &slow_live)) fail("quote");
        if (ar_quote(&fast, cold, candidate, &fast_live)) fail("quote");
        for (int i = 0; i < ARMS; i++)
            if (ar_quote(&arm[i], cold, candidate, &live[i])) fail("quote");
        if ((!slow_before.active || candidate == cold) &&
            (slow_live != cold || fast_live != cold)) fail("exact cold quote");
        for (int i = 0; i < ARMS; i++) {
            if ((!slow_before.active || candidate == cold) && live[i] != cold)
                fail("exact cold quote");
            if (!armed[i] && live[i] != slow_live) fail("pre-switch oracle quote left slow");
            if (armed[i] && LEVEL[i] && live[i] != fast_live)
                fail("level oracle quote left the fast envelope");
        }

        int slow_used_slow, slow_used_fast, slow_used[ARMS];
        int admitted_slow, admitted_fast, admitted[ARMS], clipped;
        if (ar_observe(&slow, cold, candidate, matched, &slow_used_slow, &admitted_slow, &clipped))
            fail("observe");
        if (clipped) fail("reference clipped");
        if (ar_observe(&fast, cold, candidate, matched, &slow_used_fast, &admitted_fast, &clipped))
            fail("observe");
        if (clipped) fail("reference clipped");
        if (slow_used_slow == 0 || slow_used_fast == 1) fail("fixed reference hazard");
        for (int i = 0; i < ARMS; i++) {
            if (ar_observe(&arm[i], cold, candidate, matched, &slow_used[i], &admitted[i], &clipped))
                fail("observe");
            if (clipped) fail("oracle clipped");
            if (admitted[i] != admitted_slow || arm[i].shadow != slow.shadow ||
                arm[i].active != slow.active) fail("shared first admission");
            if (armed[i] && slow_used[i] == 1) fail("armed oracle used the slow hazard");
            if (!armed[i] && (slow_used[i] != slow_used_slow || arm[i].odds != slow.odds))
                fail("pre-switch oracle state left the slow reference");
            if (armed[i] && LEVEL[i] && arm[i].odds != fast.odds)
                fail("level oracle state left the fast envelope");
        }
        if (admitted_fast != admitted_slow || fast.shadow != slow.shadow ||
            fast.active != slow.active) fail("shared first admission");

        charge(&slow_stats, 16.0, cold, slow_live, t, slow_used_slow, admitted_slow);
        charge(&fast_stats, 10.0, cold, fast_live, t, slow_used_fast, admitted_fast);
        for (int i = 0; i < ARMS; i++)
            charge(&arm_stats[i], 16.0, cold, live[i], t, slow_used[i], admitted[i]);

        printf("%zu\t%.17g\t%.17g\t%d\t%.17g\t%u\t%d", t, cold, candidate, matched,
               slow_before.shadow, slow_before.active, admitted_slow);
        printf("\t%.17g\t%.17g\t%d\t%.17g", slow_live, slow_before.odds, slow_used_slow, slow.odds);
        printf("\t%.17g\t%.17g\t%d\t%.17g", fast_live, fast_before.odds, slow_used_fast, fast.odds);
        for (int i = 0; i < ARMS; i++)
            printf("\t%.17g\t%.17g\t%.17g\t%d\t%.17g\t%d", live[i], carried[i],
                   clamped[i] ? clamp_value[i] : carried[i], slow_used[i], arm[i].odds, clamped[i]);
        putchar('\n');
        if (++expected_t == 0) fail("event counter overflow");
    }
    if (ferror(stdin) || ferror(stdout) || fflush(stdout)) fail("stream I/O");

    for (int i = 0; i < ARMS; i++) {
        if (seam < 0 && switch_byte[i] != -1) fail("switch on a life with no law change");
        if (seam >= 0 && seam + DELAY[i] < (long)expected_t && switch_byte[i] != seam + DELAY[i])
            fail("missing declared switch");
        if (LEVEL[i] && switch_byte[i] != -1 && !clamp_seen[i]) fail("missing declared clamp");
    }
    fprintf(stderr, "events=%zu arms=%d seam=%ld state_bytes=%zu\n",
            expected_t, ARMS, seam, sizeof(ARState));
    fprintf(stderr, "mode=slow gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
            slow_stats.gain, slow_stats.minimum, slow_stats.drawdown,
            slow_stats.admission, slow_stats.slow_count, slow_stats.fast_count);
    fprintf(stderr, "mode=fast gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
            fast_stats.gain, fast_stats.minimum, fast_stats.drawdown,
            fast_stats.admission, fast_stats.slow_count, fast_stats.fast_count);
    for (int i = 0; i < ARMS; i++)
        fprintf(stderr, "mode=%s delay=%ld level=%d switch=%ld clamp=%.17g clamped=%d "
                "gain=%.17g minimum=%.17g drawdown=%.17g admission=%zu slow=%zu fast=%zu\n",
                NAMES[i], DELAY[i], LEVEL[i], switch_byte[i], clamp_value[i], clamp_seen[i],
                arm_stats[i].gain, arm_stats[i].minimum, arm_stats[i].drawdown,
                arm_stats[i].admission, arm_stats[i].slow_count, arm_stats[i].fast_count);
    return 0;
}
