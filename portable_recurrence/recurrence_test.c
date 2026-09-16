#define _DARWIN_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#include "recurrence.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void close_enough(double a, double b) { assert(fabs(a - b) < 1e-10); }

int main(void) {
    int k = 0;
    assert(pr_row("010212", &k) >= 0 && k == 3);
    assert(pr_row("000000", &k) >= 0 && k == 1);
    assert(pr_row("012345", &k) >= 0 && k == 6);
    assert(pr_row("010213", &k) >= 0 && k == 4);
    assert(pr_row("011111", &k) >= 0 && k == 2);
    assert(pr_row("123456", &k) < 0);
    uint32_t context[6] = {17, 4, 17, 9, 4, 9};
    uint32_t unique[6] = {0};
    char pattern[7];
    pr_pattern(context, pattern, unique, &k);
    assert(strcmp(pattern, "010212") == 0 && k == 3);
    assert(unique[0] == 17 && unique[1] == 4 && unique[2] == 9);

    PRArchive archive = {0};
    for (int j = 0; j < 60; j++) assert(pr_archive_add(&archive, pattern, 0) == 0);
    for (int j = 0; j < 30; j++) assert(pr_archive_add(&archive, pattern, 1) == 0);
    for (int j = 0; j < 20; j++) assert(pr_archive_add(&archive, pattern, 2) == 0);
    for (int j = 0; j < 10; j++) assert(pr_archive_add(&archive, pattern, 3) == 0);
    assert(pr_archive_support(&archive, pr_row(pattern, NULL)) == 120);
    char dir[] = "/private/tmp/netta-pr-test-XXXXXX";
    assert(mkdtemp(dir));
    char path[256];
    assert(snprintf(path, sizeof(path), "%s/archive.bin", dir) < (int)sizeof(path));
    assert(pr_archive_save(&archive, path) == 0);
    assert(pr_archive_save(&archive, path) == -1);
    PRArchive loaded;
    assert(pr_archive_load(&loaded, path) == 0);
    assert(memcmp(&archive, &loaded, sizeof(archive)) == 0);
    assert(unlink(path) == 0);
    assert(rmdir(dir) == 0);

    PRLife life;
    pr_life_init(&life);
    double logmass[PR_GROUPS] = {log2(.15), log2(.20), log2(.25), log2(.40)};
    PRQuote first;
    PRStep step;
    assert(pr_quote(&life, &archive, pattern, logmass, 32, &first) == 0);
    assert(!first.selected && !first.active_before);
    assert(pr_observe(&life, &first, 0, logmass[0], 0.0, &step) == 0);
    assert(step.activated_after);
    assert(pr_observe(&life, &first, 0, logmass[0], 0.0, &step) == -1);
    PRQuote second;
    assert(pr_quote(&life, &archive, pattern, logmass, 32, &second) == 0);
    assert(second.selected && second.active_before);
    double cold_sum = 0, candidate_sum = 0;
    for (int g = 0; g <= k; g++) {
        double cold = exp2(logmass[g] + second.cold_scale_log2[g]);
        double candidate = exp2(logmass[g] + second.candidate_scale_log2[g]);
        cold_sum += cold;
        candidate_sum += candidate;
        if (g == k) close_enough(cold, candidate);
    }
    close_enough(cold_sum, 1.0);
    close_enough(candidate_sum, 1.0);
    for (int j = 0; j < 200; j++) {
        PRQuote q;
        assert(pr_quote(&life, &archive, pattern, logmass, 32, &q) == 0);
        assert(pr_observe(&life, &q, 2, logmass[2], 32.0, &step) == 0);
        assert(life.gain_bits >= -1.0 - 1e-8);
    }

    /* A crossing caused by the current truth can only change the next quote. */
    PRLife delayed;
    pr_life_init(&delayed);
    PRQuote q;
    assert(pr_quote(&delayed, &archive, pattern, logmass, 32, &q) == 0);
    assert(pr_observe(&delayed, &q, 1, logmass[1], .001, &step) == 0);
    assert(!step.activated_after);
    assert(pr_quote(&delayed, &archive, pattern, logmass, 32, &q) == 0);
    assert(q.selected && !q.active_before);
    assert(pr_observe(&delayed, &q, 0, 0.0, .001, &step) == -1);
    assert(pr_observe(&delayed, &q, 0, logmass[0], .001, &step) == 0);
    assert(!step.active_before && step.activated_after);
    assert(pr_quote(&delayed, &archive, pattern, logmass, 32, &q) == 0);
    assert(q.active_before);

    /* With one repeat class there is no conditional repeat choice to import. */
    for (int j = 0; j < 50; j++)
        assert(pr_archive_add(&archive, "000000", 0) == 0);
    PRLife one_class;
    pr_life_init(&one_class);
    double one_mass[PR_GROUPS] = {log2(.4), log2(.6)};
    assert(pr_quote(&one_class, &archive, "000000", one_mass, 32, &q) == 0);
    assert(pr_observe(&one_class, &q, 0, one_mass[0], 32, &step) == 0);
    assert(pr_quote(&one_class, &archive, "000000", one_mass, 32, &q) == 0);
    assert(!q.selected);
    close_enough(q.cold_scale_log2[0], q.candidate_scale_log2[0]);
    close_enough(q.cold_scale_log2[1], q.candidate_scale_log2[1]);

    /* Same event/active/odds do not make two recipient histories identical.
       A quote from another actual life must fail without changing recipient. */
    PRArchive empty = {0};
    PRLife issuer, recipient, recipient_before;
    pr_life_init(&issuer);
    pr_life_init(&recipient);
    PRQuote foreign, own;
    assert(pr_quote(&issuer, &empty, pattern, logmass, 32, &q) == 0);
    assert(pr_observe(&issuer, &q, 0, logmass[0], 32, &step) == 0);
    assert(pr_quote(&recipient, &empty, pattern, logmass, 32, &q) == 0);
    assert(pr_observe(&recipient, &q, 1, logmass[1], 32, &step) == 0);
    assert(pr_quote(&issuer, &empty, pattern, logmass, 32, &foreign) == 0);
    assert(pr_quote(&recipient, &empty, pattern, logmass, 32, &own) == 0);
    assert(foreign.event == own.event);
    assert(foreign.active_before == own.active_before);
    assert(foreign.odds_before == own.odds_before);
    assert(fabs(foreign.cold_scale_log2[0] - own.cold_scale_log2[0]) > 0.5);
    memcpy(&recipient_before, &recipient, sizeof(recipient));
    assert(pr_observe(&recipient, &foreign, 0, logmass[0], 32, &step) == -1);
    assert(memcmp(&recipient, &recipient_before, sizeof(recipient)) == 0);
    assert(pr_observe(&recipient, &own, 0, logmass[0], 32, &step) == 0);
    close_enough(step.cold_log2, logmass[0] + own.cold_scale_log2[0]);

    /* Header-valid u64 cells can have an unrepresentable row sum. The
       compatibility support accessor returns zero; quoting must reject it. */
    PRArchive overflowing = {0};
    overflowing.counts[0] = UINT64_MAX;
    overflowing.counts[1] = 1;
    assert(pr_archive_support(&overflowing, pr_row("000000", NULL)) == 0);
    memcpy(&recipient_before, &recipient, sizeof(recipient));
    assert(pr_quote(&recipient, &overflowing, "000000", one_mass, 32, &q) == -1);
    assert(memcmp(&recipient, &recipient_before, sizeof(recipient)) == 0);

    /* The gate byte is cold; only later active observations transition.
       Validate the next predictive odds against an independent expression. */
    PRLife hmm;
    pr_life_init_hmm(&hmm);
    assert(pr_quote(&hmm, &archive, pattern, logmass, 32, &q) == 0);
    assert(pr_observe(&hmm, &q, 0, logmass[0], 0.0, &step) == 0);
    assert(step.activated_after && hmm.log2_odds == 0.0);
    assert(pr_quote(&hmm, &archive, pattern, logmass, 32, &q) == 0);
    assert(q.hmm_before && q.active_before && q.odds_before == 0.0);
    assert(pr_observe(&hmm, &q, 2, logmass[2], 0.0, &step) == 0);
    double z = step.candidate_log2 - step.cold_log2;
    double posterior = exp2(z) / (1.0 + exp2(z));
    double next_source = (1.0 - PR_HMM_HAZARD) * posterior;
    close_enough(hmm.log2_odds, log2(next_source / (1.0 - next_source)));
    assert(hmm.log2_odds <= log2((1.0 - PR_HMM_HAZARD) / PR_HMM_HAZARD));
    /* The quote must not silently change policy between pricing and observe. */
    assert(pr_quote(&hmm, &archive, pattern, logmass, 32, &q) == 0);
    hmm.hmm = false;
    assert(pr_observe(&hmm, &q, 2, logmass[2], 0.0, &step) == -1);
    hmm.hmm = true;
    assert(pr_observe(&hmm, &q, 2, logmass[2], 0.0, &step) == 0);
    puts("API regressions: foreign quote and source-sum overflow rejected; recipient unchanged");
    puts("HMM-16: gate timing, transition and quote policy PASS");
    puts("portable recurrence: PASS");
    return 0;
}
