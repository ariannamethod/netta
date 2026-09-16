#include "frontend.h"

/* The published core remains byte-identical. Its unrelated entry points and
   fixture helpers are included but never called by this frontend. */
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "../court4/transfer4_confirm_core.c"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

struct BFState {
    Model model;
    uint8_t *past;
    size_t n, capacity, trained;
    int built, pending;
    uint8_t unit_heads[MAX_UNITS];
    double unigram[256];
    BFQuote cached;
};

BFState *bf_create(void) { return calloc(1, sizeof(BFState)); }

void bf_destroy(BFState *state) {
    if (!state) return;
    free(state->model.stream);
    free(state->model.tri);
    free(state->model.bi);
    free(state->model.pool);
    free(state->past);
    free(state);
}

/* Projection commutes with the core's fixed-EPSILON interpolation. Cache
   the projected unigram once per model rebuild; project only the sparse
   context counts at each quote instead of rescanning them for every unit. */
static void bf_build_unigram(BFState *state) {
    const Model *m = &state->model;
    memset(state->unigram, 0, sizeof(state->unigram));
    for (uint32_t u = 0; u < m->nunits; u++) {
        uint8_t head = m->pool[m->exp_off[u]];
        state->unit_heads[u] = head;
        double p = floorU(m, u);
        if (m->n1_total)
            p = EPSILON * p + (1.0 - EPSILON) *
                ((double)m->n1[u] / (double)m->n1_total);
        state->unigram[head] += p;
    }
}

static void bf_apply_context(const BFState *state, const uint64_t *keys,
                              size_t nkeys, uint64_t prefix, unsigned shift,
                              double p[256]) {
    size_t lo, hi;
    krange(keys, nkeys, prefix, shift, &lo, &hi);
    if (hi == lo) return;
    uint64_t count[256] = {0};
    for (size_t i = lo; i < hi; i++) {
        uint32_t unit = (uint32_t)(keys[i] & PACK_MASK);
        count[state->unit_heads[unit]]++;
    }
    double n = (double)(hi - lo);
    for (int b = 0; b < 256; b++)
        p[b] = EPSILON * p[b] + (1.0 - EPSILON) * ((double)count[b] / n);
}

int bf_quote(BFState *state, BFQuote *quote) {
    if (!state || !quote) return -1;
    if (state->pending) {
        memcpy(quote, &state->cached, sizeof(*quote));
        return 0;
    }
    if (!state->built || (state->n % BF_REBUILD == 0 && state->trained != state->n)) {
        model_build(&state->model, state->past, state->n);
        state->trained = state->n;
        state->built = 1;
        bf_build_unigram(state);
    }
    memset(quote, 0, sizeof(*quote));
    quote->t = state->n;
    quote->trained_bytes = state->trained;
    quote->nunits = state->model.nunits;
    quote->pattern[0] = '-';
    size_t suffix = state->n < BF_SUFFIX ? state->n : BF_SUFFIX;
    size_t nt = 0;
    const uint8_t *bytes = suffix ? state->past + state->n - suffix : NULL;
    uint32_t *tokens = segment(&state->model, bytes, suffix, &nt);
    if (token_coverage(&state->model, tokens, nt) != suffix) {
        free(tokens);
        return -1;
    }
    quote->context_len = nt < BF_DEPTH ? (int)nt : BF_DEPTH;
    for (int i = 0; i < quote->context_len; i++) {
        uint32_t unit = tokens[nt - (size_t)quote->context_len + (size_t)i];
        uint32_t len = state->model.exp_len[unit];
        if (!len || len > BF_SUFFIX) { free(tokens); return -1; }
        quote->unit_ids[i] = unit;
        quote->unit_lengths[i] = len;
        memcpy(quote->unit_bytes[i], state->model.pool + state->model.exp_off[unit], len);
    }
    free(tokens);
    if (quote->context_len == BF_DEPTH) {
        for (int i = 0; i < BF_DEPTH; i++) {
            uint8_t head = quote->unit_bytes[i][0];
            int group = 0;
            while (group < quote->repeat_classes && quote->heads[group] != head) group++;
            if (group == quote->repeat_classes) quote->heads[quote->repeat_classes++] = head;
            quote->pattern[i] = (char)('0' + group);
        }
        quote->pattern[BF_DEPTH] = '\0';
    }
    double p[256];
    memcpy(p, state->unigram, sizeof(p));
    if (quote->context_len >= 1) {
        uint32_t c2 = quote->unit_ids[quote->context_len - 1];
        bf_apply_context(state, state->model.bi, state->model.nbi, c2, PACK, p);
        if (quote->context_len >= 2) {
            uint32_t c1 = quote->unit_ids[quote->context_len - 2];
            uint64_t pair = ((uint64_t)c1 << PACK) | c2;
            bf_apply_context(state, state->model.tri, state->model.ntri, pair, PACK, p);
        }
    }
    double total = 0;
    for (int b = 0; b < 256; b++) {
        if (!isfinite(p[b]) || p[b] <= 0 || p[b] > 1.0 + 1e-12) return -1;
        total += p[b];
        quote->logp_base[b] = log2(p[b]);
    }
    if (fabs(total - 1.0) > 1e-8) return -1;
    memcpy(&state->cached, quote, sizeof(*quote));
    state->pending = 1;
    return 0;
}

int bf_observe(BFState *state, uint8_t byte) {
    if (!state || !state->pending || state->n == SIZE_MAX) return -1;
    if (state->n == state->capacity) {
        size_t capacity = state->capacity ? state->capacity * 2 : 2048;
        if (capacity < state->capacity || capacity <= state->n) return -1;
        uint8_t *past = realloc(state->past, capacity);
        if (!past) return -1;
        state->past = past;
        state->capacity = capacity;
    }
    state->past[state->n++] = byte;
    state->pending = 0;
    return 0;
}

double bf_reference_error(const BFState *state, const BFQuote *quote) {
    if (!state || !quote || !state->pending || quote->t != state->n ||
        memcmp(quote, &state->cached, sizeof(*quote)) != 0) return -1;
    uint32_t c2 = quote->context_len >= 1 ? quote->unit_ids[quote->context_len - 1] : UINT32_MAX;
    uint32_t c1 = quote->context_len >= 2 ? quote->unit_ids[quote->context_len - 2] : UINT32_MAX;
    double reference[256];
    for (int b = 0; b < 256; b++) reference[b] = -INFINITY;
    for (uint32_t unit = 0; unit < state->model.nunits; unit++) {
        double logp = price_local_log2(&state->model, c1, c2, unit);
        uint8_t head = state->unit_heads[unit];
        reference[head] = logadd2(reference[head], logp);
    }
    double maximum = 0;
    for (int b = 0; b < 256; b++) {
        double delta = fabs(reference[b] - quote->logp_base[b]);
        if (!isfinite(delta)) return -1;
        if (delta > maximum) maximum = delta;
    }
    return maximum;
}
