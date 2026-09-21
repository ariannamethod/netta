#ifndef NETTA_TURN16_AUTHORITY_H
#define NETTA_TURN16_AUTHORITY_H

typedef enum {
    AR_SLOW = 0,
    AR_FAST,
    AR_BUDGET,
    AR_RETURN,
    AR_MODES
} ARMode;

typedef enum {
    AR_NONE = 0,
    AR_ADMITTED,
    AR_ARMED,
    AR_NATURAL_RECOVERY,
    AR_RETURNED
} AREvent;

/* Persistent prediction state only: no traces, gains, world IDs or seam. */
typedef struct {
    double shadow;
    double odds;
    double support;
    ARMode mode;
    unsigned active;
    unsigned armed;
    unsigned used;
} ARState;

int ar_init(ARState *state, ARMode mode);
double ar_initial_odds(ARMode mode);

/* Quote is pure and can be called for every outcome before observation.
   Caller must finish all desired quotes before ar_observe for that event. */
int ar_quote(const ARState *state, double cold, double candidate, double *live);

/* Observe the already priced outcome, then change only future authority.
   On error, state and event remain unchanged. Initial admission excludes
   its crossing observation from the return clock. */
int ar_observe(ARState *state, double cold, double candidate, AREvent *event);

#endif
