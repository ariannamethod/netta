#ifndef NETTA_TURN22_AUTHORITY_H
#define NETTA_TURN22_AUTHORITY_H

typedef enum { AR_SLOW = 0, AR_FAST, AR_WITNESS, AR_H8L4, AR_SELFNORM, AR_MODES } ARMode;

/* Prediction state only. absolute is used only by SELF NORM; other modes
   retain the inherited signed witness clock and keep absolute at zero. */
typedef struct {
    double shadow;
    double odds;
    double evidence;
    double absolute;
    ARMode mode;
    unsigned active;
} ARState;

int ar_init(ARState *state, ARMode mode);
int ar_quote(const ARState *state, double cold, double candidate, double *live);
/* Descriptive prospective cap; NAN for uncapped modes. Inactive capped
   modes still expose their zero-state limit, as the trace contract allows. */
double ar_cap(const ARState *state);
/* Charge the quoted observation before this call. Old signed evidence
   chooses hazard; updated statistics choose the next cap. The crossing
   observation does not enter either clock. clipped measures this state's
   own hazard-updated odds. Error leaves state and output arguments intact. */
int ar_observe(ARState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted, int *clipped);

#endif
