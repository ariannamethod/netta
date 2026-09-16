#ifndef NETTA_PORTABLE_RECURRENCE_H
#define NETTA_PORTABLE_RECURRENCE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Anonymous equality patterns of the six most recent local units. No unit
   names, dictionary, source gate, or source posterior cross a life boundary. */
#define PR_DEPTH 6
#define PR_ROWS 203
#define PR_CELLS 877
#define PR_GROUPS (PR_DEPTH + 1)
#define PR_ARCHIVE_BYTES (16 + 8 * PR_CELLS)
#define PR_HMM_HAZARD 0x1p-16

typedef struct { uint64_t counts[PR_CELLS]; } PRArchive;

typedef struct {
    uint64_t counts[PR_CELLS];
    uint64_t events;
    double shadow_bits;
    double log2_odds;
    double gain_bits;
    double minimum_gain_bits;
    bool active;
    bool hmm;
} PRLife;

/* A quote is produced from the previous state, before looking at the truth.
   Its group scales transform any local unit price in that group into P0/P2.
   It belongs to the exact PRLife object that issued it. It is valid only until
   that object's next successful observe or reinitialization. The caller must
   discard outstanding quotes before pr_life_init; reuse of the same address
   after reinitialization is not a new runtime identity. */
typedef struct {
    const PRLife *owner;
    uint64_t event;
    int row;
    int repeat_classes;
    bool selected;
    bool active_before;
    bool hmm_before;
    double odds_before;
    double local_mass_log2[PR_GROUPS];
    double cold_scale_log2[PR_GROUPS];
    double candidate_scale_log2[PR_GROUPS];
} PRQuote;

typedef struct {
    double cold_log2;
    double candidate_log2;
    double live_log2;
    double shadow_before;
    double gain_after;
    bool active_before;
    bool activated_after;
} PRStep;

/* Return a row index and its number of repeat classes for a six-digit
   restricted-growth pattern (for example 010212). Invalid patterns return -1. */
int pr_row(const char pattern[PR_DEPTH + 1], int *repeat_classes);

/* The context IDs are meaningful only in this life and this inventory. A
   caller must reconstruct them when its learned-unit dictionary changes. */
void pr_pattern(const uint32_t context[PR_DEPTH], char out[PR_DEPTH + 1],
                uint32_t unique[PR_DEPTH], int *repeat_classes);

/* Binary format: NETTARM1, u32 LE depth=6, u32 LE cells=877, then 877 u64 LE.
   Existing paths are never overwritten by save. Both operations return 0 on
   success and -1 on error. */
int pr_archive_load(PRArchive *archive, const char *path);
int pr_archive_save(const PRArchive *archive, const char *path);
int pr_archive_add(PRArchive *archive, const char *pattern, int outcome);
/* Compatibility accessor: 0 also denotes invalid input or sum overflow.
   pr_quote uses a checked sum and rejects overflow rather than treating it
   as an empty source row. */
uint64_t pr_archive_support(const PRArchive *archive, int row);

void pr_life_init(PRLife *life);
/* One-way source-to-cold transition, fixed per raw observation at 2^-16.
   The first gate crossing has no transition; it admits odds=1 next event. */
void pr_life_init_hmm(PRLife *life);

/* log_mass[g] is the local model's probability mass for each repeat class and
   NEW last. It must be computed solely from the previous local state. If
   pattern is NULL, the context is incomplete and local-only pricing applies.
   min_source_support is 32 in the measured construction. Returns 0 or -1. */
int pr_quote(const PRLife *life, const PRArchive *archive,
             const char *pattern, const double log_mass[PR_GROUPS],
             uint64_t min_source_support, PRQuote *quote);

/* Price one observed unit, then update shadow, posterior and local counts.
   Owner identity rejects another life's quote; the event number prevents
   accidentally replaying a quote twice in the same initialized life.
   log_unit is the local model's price of the truth, supplied after quoting.
   outcome is the truth's repeat class, or NEW (= repeat_classes); use -1 for
   an incomplete context. Returns 0 or -1. */
int pr_observe(PRLife *life, const PRQuote *quote, int outcome,
               double log_unit, double gate_bits, PRStep *step);

#endif
