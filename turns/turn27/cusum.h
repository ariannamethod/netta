#ifndef NETTA_TURN27_CUSUM_H
#define NETTA_TURN27_CUSUM_H

#include "../turn22/authority.h"

/* Preregistered constants (turn27 PROTOCOL.md). Never scanned, never tuned. */
#define CS_DRIFT 0.5
#define CS_THRESHOLD 8.0

/* One non-negative cumulative sum of net received harm, and one one-way latch.
   The inherited core rests in AR_SLOW; the flag selects the next active
   observation's hazard and never changes a quote. */
typedef struct {
    ARState core;
    double sum;
    unsigned latched;
} CSState;

int cs_init(CSState *state);
int cs_quote(const CSState *state, double cold, double candidate, double *live);
/* Quote first. The old flag picks this byte's hazard; then the charged byte's
   delta enters the sum on a voting byte; then the latch may set, never clear.
   The admission-crossing observation enters neither. Error preserves state and
   output arguments. */
int cs_observe(CSState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted);

#endif
