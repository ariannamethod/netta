/* Turn29: per-record permission for the full pooled episode archive. */
#define main inherited_episode_main
#include ".build/turn13/episode.c"
#undef main

#define CAL_ARMS 4
#define CAL_SHARE 0x1p-10
#define CAL_PRIOR 0.875

enum { CAL_LOCAL, CAL_GLOBAL, CAL_POOLED, CAL_PERMUTED };
static const char *const cal_names[CAL_ARMS] = {
    "local_full", "global_full", "pooled_full", "permuted_full"};

typedef struct { double memory; } CalRouter;

static void cal_init(CalRouter *router) { router->memory = CAL_PRIOR; }

static void cal_check(const CalRouter *router) {
    if (!isfinite(router->memory) || router->memory <= 0.0 || router->memory >= 1.0)
        fail("invalid binary router weight");
}

static void cal_same_archives(const Grammar *full, const Grammar *permuted) {
    if (full->bytes > MAX_ARCHIVE_BYTES || permuted->bytes > MAX_ARCHIVE_BYTES ||
        full->count != permuted->count || full->records != permuted->records ||
        full->bytes != permuted->bytes)
        fail("full/permuted archive shape differs");
    for (size_t i = 0; i < full->count; i++)
        if (full->rule[i].left != permuted->rule[i].left ||
            full->rule[i].right != permuted->rule[i].right)
            fail("full/permuted grammar differs");
    for (size_t i = 0; i < full->records; i++) {
        const Branch *a = &full->branch[i], *b = &permuted->branch[i];
        if (a->rule_id != b->rule_id || a->prefix_len != b->prefix_len)
            fail("full/permuted address differs");
        for (int r = 0; r < 7; r++) {
            int old = r ? r % 6 + 1 : 0;
            if (b->counts[r] != a->counts[old]) fail("full permutation differs");
        }
    }
}

static void cal_quote(const CalRouter *router, const double cold[256],
                      const double source[256], double out[256]) {
    cal_check(router);
    double local = log2(1.0 - router->memory), memory = log2(router->memory);
    for (int byte = 0; byte < 256; byte++)
        out[byte] = source[byte] == cold[byte]
            ? cold[byte] : la(local + cold[byte], memory + source[byte]);
}

static void cal_observe(CalRouter *router, double source, double quoted) {
    double posterior = router->memory * exp2(source - quoted);
    router->memory = (1.0 - CAL_SHARE) * posterior + CAL_SHARE * CAL_PRIOR;
    cal_check(router);
}

static void cal_free(Grammar *g) {
    free(g->rule);
    free(g->forward);
    free(g->branch);
}

static void cal_predict(const char *full_path, const char *permuted_path) {
    Grammar full = load_grammar(full_path), permuted = load_grammar(permuted_path);
    cal_same_archives(&full, &permuted);
    CalRouter *local = full.records ? calloc(full.records, sizeof(*local)) : NULL;
    CalRouter *perm = full.records ? calloc(full.records, sizeof(*perm)) : NULL;
    if (full.records && (!local || !perm)) fail("allocate binary routers");
    for (size_t i = 0; i < full.records; i++) {
        cal_init(&local[i]);
        cal_init(&perm[i]);
    }
    CalRouter global, prior;
    cal_init(&global);
    cal_init(&prior);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate calibrated frontend");
    History history = {{0}, 0};
    Outer outer[CAL_ARMS] = {{0}};
    double overall_norm = 0.0;

    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\tsource\tperm_source\tmax_norm_error\tnew_exact"
          "\tlocal_w_before\tlocal_w_after\tglobal_w_before\tglobal_w_after"
          "\tpermuted_w_before\tpermuted_w_after", stdout);
    for (int arm = 0; arm < CAL_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before"
               "\t%s_active_before\t%s_activated_after\t%s_gain_after",
               cal_names[arm], cal_names[arm], cal_names[arm], cal_names[arm],
               cal_names[arm], cal_names[arm], cal_names[arm]);
    putchar('\n');

    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match match = branch_match(&full, &history);
        Match perm_match = branch_match(&permuted, &history);
        if (match.length != perm_match.length || match.nmatches != perm_match.nmatches ||
            (match.nmatches && match.matches[0] != perm_match.matches[0]))
            fail("full/permuted match differs");
        int record = match.nmatches ? match.matches[0] : -1;
        double source[256], perm_source[256], candidates[CAL_ARMS][256], live[CAL_ARMS][256];
        candidate(&q, &match, source);
        candidate(&q, &perm_match, perm_source);
        CalRouter before[3] = {record < 0 ? prior : local[record], global,
                               record < 0 ? prior : perm[record]};
        cal_quote(&before[0], q.cold, source, candidates[CAL_LOCAL]);
        cal_quote(&before[1], q.cold, source, candidates[CAL_GLOBAL]);
        memcpy(candidates[CAL_POOLED], source, sizeof(source));
        cal_quote(&before[2], q.cold, perm_source, candidates[CAL_PERMUTED]);
        double norm = fmax(fmax(validate(q.bf.logp_base), validate(q.cold)),
                           fmax(validate(source), validate(perm_source)));
        for (int arm = 0; arm < CAL_ARMS; arm++) {
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte]
                    : la(q.cold[byte], outer[arm].odds + candidates[arm][byte])
                      - la(0, outer[arm].odds);
                if (!q.rank[byte] &&
                    (candidates[arm][byte] != q.cold[byte] || live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in calibrated arm");
            }
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }

        /* All source, router, candidate and live vectors precede truth. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        overall_norm = fmax(overall_norm, norm);
        Outer outer_before[CAL_ARMS];
        int activated[CAL_ARMS];
        for (int arm = 0; arm < CAL_ARMS; arm++) {
            outer_before[arm] = outer[arm];
            activated[arm] = observe_outer(&outer[arm], q.bf.t, q.cold[truth],
                                            candidates[arm][truth], live[arm][truth]);
        }
        if (record >= 0) {
            cal_observe(&local[record], source[truth], candidates[CAL_LOCAL][truth]);
            cal_observe(&global, source[truth], candidates[CAL_GLOBAL][truth]);
            cal_observe(&perm[record], perm_source[truth], candidates[CAL_PERMUTED][truth]);
        }
        CalRouter after[3] = {record < 0 ? prior : local[record], global,
                              record < 0 ? prior : perm[record]};

        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0);
        putchar('\t');
        print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        printf("\t%.17g\t%d\t%d\t%.17g\t%.17g\t%.17g\t1",
               q.cold[truth], match.length, record, source[truth], perm_source[truth], norm);
        for (int i = 0; i < 3; i++)
            printf("\t%.17g\t%.17g", before[i].memory, after[i].memory);
        for (int arm = 0; arm < CAL_ARMS; arm++)
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[arm][truth], live[arm][truth], outer_before[arm].shadow,
                   outer_before[arm].odds, outer_before[arm].active,
                   activated[arm], outer[arm].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) {
            memmove(history.event, history.event + 1, HORIZON - 1);
            history.length--;
        }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    fprintf(stderr, "calibrate predict: bytes=%" PRIu64 " full_bytes=%zu permuted_bytes=%zu "
                    "rules=%zu records=%zu router_struct_bytes=%zu local_router_bytes=%zu "
                    "global_router_bytes=%zu permuted_router_bytes=%zu router_total_bytes=%zu "
                    "outer_state_bytes=%zu history_struct_bytes=%zu forecast_vector_bytes=%zu "
                    "max_norm_error=%.17g\n",
            life.events, full.bytes, permuted.bytes, full.count, full.records,
            sizeof(CalRouter), full.records * sizeof(CalRouter), sizeof(CalRouter),
            full.records * sizeof(CalRouter), (2 * full.records + 1) * sizeof(CalRouter),
            sizeof(outer), sizeof(history), (2 + 2 * CAL_ARMS) * 256 * sizeof(double),
            overall_norm);
    for (int arm = 0; arm < CAL_ARMS; arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                cal_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(local);
    free(perm);
    bf_destroy(front);
    cal_free(&full);
    cal_free(&permuted);
}

static void cal_fixture(void) {
    Quote q = {0};
    q.k = 2;
    q.heads[0] = 3;
    q.heads[1] = 7;
    for (int byte = 0; byte < 256; byte++) q.cold[byte] = -8.0;
    q.rank[3] = 1;
    q.rank[7] = 2;
    Match source_match = {0};
    source_match.length = source_match.nmatches = 1;
    source_match.matches[0] = 0;
    source_match.votes[1] = 40;
    source_match.votes[2] = 2;
    CalRouter local[2], global;
    cal_init(&local[0]); cal_init(&local[1]); cal_init(&global);
    const int records[] = {0, 1, 0, -1, 1, 0};
    const uint8_t truths[] = {3, 7, 3, 23, 7, 3};
    for (size_t t = 0; t < sizeof(records)/sizeof(records[0]); t++) {
        int record = records[t];
        double source[256], lq[256], gq[256];
        if (record < 0 || t == 2) memcpy(source, q.cold, sizeof(source));
        else candidate(&q, &source_match, source);
        CalRouter before = record < 0 ? (CalRouter){CAL_PRIOR} : local[record];
        CalRouter gbefore = global;
        cal_quote(&before, q.cold, source, lq);
        cal_quote(&gbefore, q.cold, source, gq);
        double norm = fmax(fmax(validate(source), validate(lq)), validate(gq));
        uint8_t truth = truths[t];
        if (record >= 0) {
            cal_observe(&local[record], source[truth], lq[truth]);
            cal_observe(&global, source[truth], gq[truth]);
        }
        CalRouter after = record < 0 ? (CalRouter){CAL_PRIOR} : local[record];
        printf("{\"t\":%zu,\"record\":%d,\"truth\":%u,\"cold\":%.17g,"
               "\"source\":%.17g,\"local_candidate\":%.17g,"
               "\"global_candidate\":%.17g,\"local_before\":%.17g,"
               "\"local_after\":%.17g,\"global_before\":%.17g,"
               "\"global_after\":%.17g,\"max_norm_error\":%.17g}\n",
               t, record, (unsigned)truth, q.cold[truth], source[truth], lq[truth],
               gq[truth], before.memory, after.memory, gbefore.memory, global.memory, norm);
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 4 && !strcmp(argv[1], "predict")) cal_predict(argv[2], argv[3]);
    else if (argc == 2 && !strcmp(argv[1], "--fixture")) cal_fixture();
    else {
        fputs("usage: calibrate predict POOLED_FULL PERMUTED_FULL < RAW\n"
              "       calibrate emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       calibrate trace [compact] < RAW\n"
              "       calibrate --fixture\n", stderr);
        return 2;
    }
    return 0;
}
