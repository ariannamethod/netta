#ifndef NETTA_TURN25_LATCH_H
#define NETTA_TURN25_LATCH_H

#include "../turn22/authority.h"

/* The inherited core remains in AR_WITNESS mode between calls. The new bit
   selects the next active observation's hazard; it never changes a quote. */
typedef struct {
    ARState core;
    unsigned fast_latched;
} LRState;

int lr_init(LRState *state);
int lr_quote(const LRState *state, double cold, double candidate, double *live);
/* Quote first. Use the old flag, then update the inherited witness clock,
   then set at w <= -1 or clear at w >= +1. Admission excludes its crossing
   observation from the clock. Error preserves state and output arguments. */
int lr_observe(LRState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted);

#endif
