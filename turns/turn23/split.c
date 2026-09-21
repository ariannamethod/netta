/* Turn23's sole new authority. Input: t cold_log2 candidate_log2 matchedL. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct {
    double shadow, cold, short_mass, long_mass, short_w, long_w;
    unsigned active;
} State;

static void die(const char *why) {
    fprintf(stderr, "split: %s\n", why);
    exit(1);
}

int main(int argc, char **argv) {
    (void)argv;
    if (argc != 1) die("usage: split < chronological price tape");
    State s = {0};
    char line[512];
    size_t expected = 0;
    puts("t\tcold\tcandidate\tmatched\tactive_before\tshadow_before\tshort_before\tlong_before\tshort_w_before\tlong_w_before\tlive\tadmitted_after\tshort_after\tlong_after\tshort_w_after\tlong_w_after\tshort_hazard\tlong_hazard");
    while (fgets(line, sizeof(line), stdin)) {
        size_t t;
        double cold, candidate;
        int matched;
        char surplus;
        if (sscanf(line, "%zu %lf %lf %d %c", &t, &cold, &candidate, &matched, &surplus) != 4 ||
            t != expected || matched < 0 || !isfinite(cold) || !isfinite(candidate) ||
            cold > 1e-9 || candidate > 1e-9) die("invalid price tape");
        State old = s;
        int lane = matched == 0 ? -1 : matched <= 2 ? 0 : 1;
        double a = lane == 0 ? s.short_mass : lane == 1 ? s.long_mass : 0.;
        double delta = candidate - cold;
        double ratio = exp2(delta);
        if (!isfinite(ratio) || ratio <= 0.) die("invalid candidate ratio");
        double mix = s.active && lane >= 0 ? 1. + a * (ratio - 1.) : 1.;
        double live = !s.active || candidate == cold || lane < 0 ? cold : cold + log2(mix);
        if (!isfinite(live) || live > 1e-8 || mix <= 0.) die("invalid quote");
        if ((!s.active || candidate == cold || lane < 0) && live != cold) die("cold quote not exact");
        s.shadow += delta;
        int admitted = 0;
        double sh = -1., lh = -1.;
        if (!s.active && s.shadow >= 32.) {
            s.active = 1;
            s.cold = .5;
            s.short_mass = .25;
            s.long_mass = .25;
            s.short_w = s.long_w = 0.;
            admitted = 1;
        } else if (s.active) {
            s.cold /= mix;
            s.short_mass *= lane == 0 ? ratio / mix : 1. / mix;
            s.long_mass *= lane == 1 ? ratio / mix : 1. / mix;
            sh = old.short_w <= -1. ? 0x1p-10 : 0x1p-16;
            lh = old.long_w <= -1. ? 0x1p-10 : 0x1p-16;
            s.cold += sh * s.short_mass + lh * s.long_mass;
            s.short_mass *= 1. - sh;
            s.long_mass *= 1. - lh;
            if (lane == 0) s.short_w = (31. / 32.) * old.short_w + delta;
            if (lane == 1) s.long_w = (31. / 32.) * old.long_w + delta;
        }
        if (s.active && (s.cold < 0. || s.short_mass < 0. || s.long_mass < 0. ||
            fabs(s.cold + s.short_mass + s.long_mass - 1.) > 1e-10 ||
            s.cold < 0x1p-16 - 1e-12)) die("capital invariant");
        if (!isfinite(s.shadow) || !isfinite(s.short_w) || !isfinite(s.long_w)) die("state invariant");
        printf("%zu\t%.17g\t%.17g\t%d\t%u\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\n",
               t, cold, candidate, matched, old.active, old.shadow,
               old.short_mass, old.long_mass, old.short_w, old.long_w, live,
               admitted, s.short_mass, s.long_mass, s.short_w, s.long_w, sh, lh);
        if (++expected == 0) die("counter overflow");
    }
    if (ferror(stdin) || ferror(stdout)) die("stream error");
    fprintf(stderr, "events=%zu prediction_state_bytes=%zu\n", expected, sizeof(s));
    return 0;
}
