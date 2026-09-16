#ifndef NETTA_BYTE_RECURRENCE_FRONTEND_H
#define NETTA_BYTE_RECURRENCE_FRONTEND_H

#include <stddef.h>
#include <stdint.h>

#define BF_SUFFIX 256
#define BF_DEPTH 6
#define BF_REBUILD 1024

typedef struct BFState BFState;

/* HEAD256-v1 quote. Every member is determined by already observed bytes.
   Expansions are copied, so a quote owns its display data. */
typedef struct {
    size_t t;
    size_t trained_bytes;
    uint32_t nunits;
    int context_len;
    uint32_t unit_ids[BF_DEPTH];
    uint32_t unit_lengths[BF_DEPTH];
    uint8_t unit_bytes[BF_DEPTH][BF_SUFFIX];
    char pattern[BF_DEPTH + 1];
    int repeat_classes;
    uint8_t heads[BF_DEPTH];
    double logp_base[256];
} BFQuote;

BFState *bf_create(void);
void bf_destroy(BFState *state);

/* Quote accepts no truth, input buffer, target length, or future handle.
   Repeated quotes before observing a byte are byte-identical. Model rebuilding
   may occur here, using state-owned past bytes only. Returns 0 or -1. */
int bf_quote(BFState *state, BFQuote *quote);

/* Observe one byte after a completed quote. Calling without a pending quote
   refuses. Recurrence counts and authority belong to the caller, not here. */
int bf_observe(BFState *state, uint8_t byte);

/* Diagnostic: compare the projected vector against an explicit sum of every
   unchanged core price_local_log2 unit probability. Returns largest absolute
   log2 error, or -1 for an invalid current quote. Does not change the state. */
double bf_reference_error(const BFState *state, const BFQuote *quote);

#endif
