#ifndef NETTA_TURN21_AUTHORITY_H
#define NETTA_TURN21_AUTHORITY_H

/* Turn21 measures turn20's witnessed ceiling over a preregistered (high, low)
   grid against three references. Mode AR_GRID+i is grid point i of ar_grid;
   (5,5) is turn19's fixed ceiling and (10,5) is turn20's witnessed ceiling,
   grid points rather than separate laws. */
#define AR_GRID_POINTS 24

typedef enum { AR_SLOW = 0, AR_FAST, AR_WITNESS, AR_GRID,
               AR_MODES = AR_GRID + AR_GRID_POINTS } ARMode;

typedef struct { int high, low; } ARCeiling;

/* Frozen before any byte of worlds 216..223 existed; never extended after data. */
extern const ARCeiling ar_grid[AR_GRID_POINTS];

/* Only prediction state, never world IDs, labels, outcomes or trace statistics.
   One scalar clock serves the modes that own it: turn18's signed wrongness w
   under AR_WITNESS and every grid point. The record stays 32 bytes, as the
   protocol declares. */
typedef struct {
    double shadow;
    double odds;
    double evidence;
    ARMode mode;
    unsigned active;
} ARState;

int ar_init(ARState *state, ARMode mode);
int ar_quote(const ARState *state, double cold, double candidate, double *live);
/* The observed byte is charged before this call. matched is the episode
   candidate's longest stored match length at this byte; witness modes read
   it, and only matched >= 1 moves its clock. slow_used is 0 or 1 for an active
   update, -1 for an inactive observation. admitted is 1 only when this
   observation first crosses shadow 32. clipped is 1 only if this state's own
   hazard-updated odds exceeded this grid point's next-quote cap; it is not
   persistent state. No output changes on error. */
int ar_observe(ARState *state, double cold, double candidate, int matched,
               int *slow_used, int *admitted, int *clipped);

#endif
