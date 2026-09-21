#include "authority.h"

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* One synthetic price journey, unrelated to any protocol world. It makes
   the simultaneous natural-recovery/support crossing observable, then
   reaches and spends the single artificial return. */
static AREvent see(ARState *s, double delta) {
    double cold = delta > 0 ? -1.0 - delta : -1.0;
    double candidate = cold + delta;
    double live;
    ARState before = *s;
    assert(ar_quote(s, cold, candidate, &live) == 0);
    assert(memcmp(s, &before, sizeof(before)) == 0);
    assert(isfinite(live));
    if (!before.active || delta == 0) assert(live == cold);
    AREvent event;
    assert(ar_observe(s, cold, candidate, &event) == 0);
    return event;
}

int main(void) {
    for (int mode = 0; mode < AR_MODES; mode++) {
        ARState s;
        assert(ar_init(&s, (ARMode)mode) == 0);
        assert(see(&s, 32.0) == AR_ADMITTED);
        assert(s.active && !s.armed && !s.used && s.support == 0);
        assert(s.odds == ar_initial_odds((ARMode)mode));
        /* Concrete historical fast-NEW precision value, now exact. */
        double cold = -8.2806284658485314, live;
        assert(ar_quote(&s, cold, cold, &live) == 0 && live == cold);
    }
    ARState s;
    assert(ar_init(&s, AR_RETURN) == 0);
    assert(see(&s, 32.0) == AR_ADMITTED);
    assert(see(&s, -32.0) == AR_ARMED);
    assert(s.odds <= -32.0 && s.support == 0);
    assert(see(&s, 31.5) == AR_NONE);
    assert(s.armed && s.support == 31.5 && s.odds < ar_initial_odds(AR_RETURN));
    assert(see(&s, 1.0) == AR_NATURAL_RECOVERY);
    assert(!s.armed && !s.used && s.support == 0);
    assert(see(&s, -64.0) == AR_ARMED);
    assert(s.support == 0); /* The arming observation pays no support. */
    double old_odds = s.odds;
    assert(see(&s, 0.0) == AR_NONE);
    assert(s.armed && s.support == 0 && s.odds < old_odds);
    assert(see(&s, 16.0) == AR_NONE);
    assert(s.support == 16.0 && !s.used);
    double old_quote;
    assert(ar_quote(&s, -17.0, -1.0, &old_quote) == 0);
    assert(see(&s, 16.0) == AR_RETURNED);
    assert(s.used && !s.armed && s.support == 0);
    assert(s.odds == ar_initial_odds(AR_RETURN));
    double next_quote;
    assert(ar_quote(&s, -17.0, -1.0, &next_quote) == 0 && next_quote > old_quote);
    assert(see(&s, -64.0) == AR_NONE);
    assert(see(&s, 64.0) == AR_NONE);
    assert(s.used && !s.armed && s.support == 0);
    printf("PASS: exact equality, cold crossing, natural priority, excluded arming event, neutral hazard, prospective return, one-use; sizeof_ARState=%zu\n",
           sizeof(ARState));
    return 0;
}
