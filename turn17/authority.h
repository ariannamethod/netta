#ifndef NETTA_TURN17_AUTHORITY_H
#define NETTA_TURN17_AUTHORITY_H

typedef enum { AR_SLOW = 0, AR_FAST, AR_ADAPTIVE, AR_MODES } ARMode;

/* Only prediction state, never world IDs, labels, outcomes or trace statistics. */
typedef struct {
    double shadow;
    double odds;
    double evidence;
    ARMode mode;
    unsigned active;
} ARState;

int ar_init(ARState *state, ARMode mode);
int ar_quote(const ARState *state, double cold, double candidate, double *live);
/* The observed byte is charged before this call. slow_used is 0 or 1 for an
   active update, -1 for an inactive observation. admitted is 1 only when
   this observation first crosses shadow 32. No output changes on error. */
int ar_observe(ARState *state, double cold, double candidate,
               int *slow_used, int *admitted);

#endif
