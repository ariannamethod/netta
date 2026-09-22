/* One handcrafted 48-observation tape, unrelated to protocol worlds.
   --tape emits its prices for exact comparison with the frozen turn22 replay.
   Default output is JSON evidence. A long-double probability-space reader
   checks all four live prices/odds independently of the log-odds helper. */
#include "latch.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { EVENTS = 48, MODES = 4 };
static const char *const names[MODES] = {"slow", "fast", "witness", "latch"};

typedef struct {
    double cold, candidate;
    int matched;
} Event;

typedef struct {
    long double source, shadow, evidence, gain, peak;
    unsigned active, latched;
} Mass;

static void check(int ok, const char *message) {
    if (!ok) {
        fprintf(stderr, "fixture: %s\n", message);
        exit(1);
    }
}

static Event event_at(int t) {
    if (t < 8) return (Event){-5.0, -1.0, 1};
    if (t == 8) return (Event){-1.0, -3.0, 1};
    if (t == 9 || t == 38) return (Event){-2.0, -2.0, 0};
    if ((t >= 10 && t <= 33) || t == 40) return (Event){-2.0, -2.0, 1};
    if (t == 35) return (Event){-3.0, -1.0, 1};
    if (t == 37) return (Event){-1.0, -5.0, 1};
    if (t == 39) return (Event){-5.0, -1.0, 1};
    if (t == 41) return (Event){-1.0, -9.0, 1};
    return (Event){-2.0, -1.0, 1};
}

static double mass_quote(const Mass *s, Event e) {
    if (!s->active || e.cold == e.candidate) return e.cold;
    return (double)log2l((1.0L - s->source) * exp2l(e.cold) + s->source * exp2l(e.candidate));
}

static void mass_observe(Mass *s, int mode, Event e, int *slow, int *admitted) {
    long double delta = (long double)e.candidate - e.cold;
    s->shadow += delta;
    *slow = -1;
    *admitted = 0;
    if (!s->active) {
        if (s->shadow >= 32.0L) {
            s->active = 1;
            s->source = 0.5L;
            s->evidence = 0.0L;
            s->latched = 0;
            *admitted = 1;
        }
        return;
    }
    *slow = mode == AR_SLOW || (mode == AR_WITNESS && s->evidence > -1.0L) ||
            (mode == 3 && !s->latched);
    long double hazard = *slow ? 0x1p-16L : 0x1p-10L;
    long double a = s->source * exp2l(delta), b = 1.0L - s->source;
    s->source = (a / (a + b)) * (1.0L - hazard);
    if (mode >= AR_WITNESS && e.matched >= 1)
        s->evidence = (31.0L / 32.0L) * s->evidence + delta;
    if (mode == 3) {
        if (s->evidence <= -1.0L) s->latched = 1;
        else if (s->evidence >= 1.0L) s->latched = 0;
    }
}

int main(int argc, char **argv) {
    if (argc == 2 && strcmp(argv[1], "--tape") == 0) {
        for (int t = 0; t < EVENTS; t++) {
            Event e = event_at(t);
            printf("%d %.17g %.17g %d\n", t, e.cold, e.candidate, e.matched);
        }
        return ferror(stdout) ? 1 : 0;
    }
    check(argc == 1, "usage: fixture [--tape]");
    ARState controls[3];
    LRState latch;
    Mass mass[MODES] = {{0}};
    double max_error = 0.0;
    unsigned sets = 0, returns = 0, neutral_holds = 0, rebound_holds = 0;
    for (int m = 0; m < 3; m++) check(!ar_init(&controls[m], (ARMode)m), "control init");
    check(!lr_init(&latch), "latch init");
    fputs("{\"events\":[\n", stdout);
    for (int t = 0; t < EVENTS; t++) {
        Event e = event_at(t);
        ARState before[MODES], after[MODES];
        double live[MODES], expected[MODES];
        int slow[MODES], admitted[MODES];
        unsigned flag_before = latch.fast_latched;
        for (int m = 0; m < 3; m++) {
            before[m] = controls[m];
            check(!ar_quote(&controls[m], e.cold, e.candidate, &live[m]), "control quote");
        }
        before[3] = latch.core;
        check(!lr_quote(&latch, e.cold, e.candidate, &live[3]), "latch quote");
        double quoted_again;
        check(!lr_quote(&latch, e.cold, e.candidate, &quoted_again) && quoted_again == live[3], "quote idempotence");
        for (int m = 0; m < MODES; m++) {
            expected[m] = mass_quote(&mass[m], e);
            double error = fabs(expected[m] - live[m]);
            max_error = fmax(max_error, error);
            check(error <= 1e-11, "probability-space quote");
            if (!before[m].active || e.cold == e.candidate) check(live[m] == e.cold, "exact cold quote");
        }
        for (int m = 0; m < 3; m++) {
            int clipped;
            check(!ar_observe(&controls[m], e.cold, e.candidate, e.matched, &slow[m], &admitted[m], &clipped), "control observe");
            check(!clipped, "control cap");
            after[m] = controls[m];
        }
        check(!lr_observe(&latch, e.cold, e.candidate, e.matched, &slow[3], &admitted[3]), "latch observe");
        after[3] = latch.core;
        for (int m = 0; m < MODES; m++) {
            int oracle_slow, oracle_admitted;
            mass_observe(&mass[m], m, e, &oracle_slow, &oracle_admitted);
            check(slow[m] == oracle_slow && admitted[m] == oracle_admitted, "hazard/admission chronology");
            check(admitted[m] == (t == 7), "fixed first admission");
            if (mass[m].active) {
                double z = (double)log2l(mass[m].source / (1.0L - mass[m].source));
                double error = fabs(z - after[m].odds);
                max_error = fmax(max_error, error);
                check(error <= 1e-11, "probability-space odds");
            }
            mass[m].gain += (long double)live[m] - e.cold;
            mass[m].peak = fmaxl(mass[m].peak, mass[m].gain);
            check(mass[m].gain >= -1.0L - 1e-10L, "whole-prefix loss bound");
            check(mass[m].peak - mass[m].gain <= (m == AR_FAST ? 10.0L : 16.0L) + 1e-10L, "interval loss bound");
            check(after[m].shadow == after[0].shadow && after[m].active == after[0].active, "shared admission");
        }
        check(after[3].evidence == after[2].evidence, "exact inherited witness clock");
        check(latch.fast_latched == mass[3].latched, "probability-space latch");
        if (!flag_before && latch.fast_latched) sets++;
        if (flag_before && !latch.fast_latched) returns++;
        if (e.candidate == e.cold && flag_before) {
            check(latch.fast_latched, "neutral latch hold");
            neutral_holds++;
            if (after[3].evidence > -1.0) rebound_holds++;
        }
        if (t == 8) check(!flag_before && latch.fast_latched && slow[3] == 1, "first set remains prospective");
        if (t == 34) check(latch.fast_latched && after[3].evidence > -1.0 && after[3].evidence < 1.0, "deadband holds");
        if (t == 35) check(flag_before && !latch.fast_latched && slow[3] == 0, "return remains prospective");
        if (t == 36) check(slow[3] == 1, "slow after return");
        printf("%s{\"t\":%d,\"cold\":%.17g,\"candidate\":%.17g,\"matched\":%d,"
               "\"shadow_before\":%.17g,\"active_before\":%u,\"admitted_after\":%d,"
               "\"w_before\":%.17g,\"w_after\":%.17g,\"latch_before\":%u,\"latch_after\":%u",
               t ? ",\n" : "", t, e.cold, e.candidate, e.matched,
               before[0].shadow, before[0].active, admitted[0], before[2].evidence,
               after[2].evidence, flag_before, latch.fast_latched);
        for (int m = 0; m < MODES; m++)
            printf(",\"%s_live\":%.17g,\"%s_odds_before\":%.17g,\"%s_slow_used\":%d,\"%s_odds_after\":%.17g",
                   names[m], live[m], names[m], before[m].odds, names[m], slow[m], names[m], after[m].odds);
        putchar('}');
    }
    check(sets >= 2 && returns >= 2 && neutral_holds && rebound_holds, "fixture covers transitions and neutral rebound");
    /* The wrapper promises atomic failure even after inherited work on its
       local copy. This single invalid input exercises the public contract. */
    LRState preserved = latch;
    int slow_sentinel = 23, admitted_sentinel = 29;
    check(lr_observe(&latch, -2.0, NAN, 1, &slow_sentinel, &admitted_sentinel) == -1 &&
          latch.core.shadow == preserved.core.shadow && latch.core.odds == preserved.core.odds &&
          latch.core.evidence == preserved.core.evidence && latch.core.mode == preserved.core.mode &&
          latch.core.active == preserved.core.active && latch.fast_latched == preserved.fast_latched &&
          slow_sentinel == 23 && admitted_sentinel == 29, "atomic rejection");
    printf("\n],\"summary\":{\"passed\":true,\"event_count\":%d,\"sets\":%u,\"returns\":%u,"
           "\"neutral_holds\":%u,\"neutral_rebound_holds\":%u,\"max_mass_error\":%.17g,"
           "\"control_state_bytes\":%zu,\"latch_state_bytes\":%zu}}\n",
           EVENTS, sets, returns, neutral_holds, rebound_holds, max_error, sizeof(ARState), sizeof(LRState));
    return ferror(stdout) ? 1 : 0;
}
