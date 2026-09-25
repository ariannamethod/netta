/* NETEB001: two source cases at one learned episode prefix. Reuse the frozen
   frontend, record matcher, component law and slow outer law verbatim. */
#define main inherited_episode_main
#include ".build/turn13/episode.c"
#undef main

#define EB_ARMS 5
#define EB_SHARE 0x1p-10

enum { EB_LOCAL, EB_GLOBAL, EB_FULL, EB_SMALL, EB_PERMUTED };
static const char *const eb_names[EB_ARMS] = {
    "local", "global", "pooled_full", "pooled_small", "permuted"};
static const double eb_prior[3] = {0.125, 0.4375, 0.4375};

typedef struct { double weight[3]; } EBRouter;
typedef struct {
    /* Rules/expansions are shared by the two books; branch arrays are owned
       separately. Only book[0] owns the shared allocations. */
    Grammar book[2];
    size_t bytes;
} EBank;

static void eb_init(EBRouter *router) {
    memcpy(router->weight, eb_prior, sizeof(eb_prior));
}

static void eb_check_router(const EBRouter *router) {
    double sum = 0.0;
    for (int i = 0; i < 3; i++) {
        if (!isfinite(router->weight[i]) || router->weight[i] <= 0.0 ||
            router->weight[i] > 1.0) fail("invalid router weight");
        sum += router->weight[i];
    }
    if (fabs(sum - 1.0) > 1e-10) fail("router normalization");
}

static EBank eb_load(const char *path) {
    EBank bank = {0};
    unsigned char *bytes = read_bytes(path, MAX_ARCHIVE_BYTES, &bank.bytes);
    if (bank.bytes < 16 || memcmp(bytes, "NETEB001", 8)) fail("invalid NETEB001 header");
    size_t nr = le32(bytes + 8), nb = le32(bytes + 12);
    if (nr > MAX_RULES || nb > MAX_RECORDS || bank.bytes != 16 + 4 * nr + 32 * nb)
        fail("invalid NETEB001 count/size");
    Grammar *g = &bank.book[0];
    g->count = nr;
    g->records = nb;
    g->bytes = bank.bytes;
    if (nr) {
        g->rule = calloc(nr, sizeof(*g->rule));
        g->forward = calloc(nr, sizeof(*g->forward));
        if (!g->rule || !g->forward) fail("allocate bank grammar");
    }
    for (size_t i = 0; i < nr; i++) {
        const unsigned char *p = bytes + 16 + 4 * i;
        g->rule[i] = (Rule){le16(p), le16(p + 2)};
        if (g->rule[i].left >= 7 + i || g->rule[i].right >= 7 + i)
            fail("invalid bank topology");
        append_child(&g->forward[i], g->rule[i].left, g->forward);
        append_child(&g->forward[i], g->rule[i].right, g->forward);
    }
    bank.book[1] = *g;
    for (int c = 0; c < 2; c++) {
        if (nb) {
            bank.book[c].branch = calloc(nb, sizeof(Branch));
            if (!bank.book[c].branch) fail("allocate bank records");
        }
    }
    for (size_t i = 0; i < nb; i++) {
        const unsigned char *p = bytes + 16 + 4 * nr + 32 * i;
        if (le16(p + 2)) fail("nonzero NETEB001 reserved bytes");
        for (int c = 0; c < 2; c++) {
            Branch *b = &bank.book[c].branch[i];
            b->rule_id = p[0];
            b->prefix_len = p[1];
            for (int j = 0; j < 7; j++) b->counts[j] = le16(p + 4 + 14 * c + 2 * j);
            if (b->rule_id >= nr || !b->prefix_len ||
                b->prefix_len >= g->forward[b->rule_id].length)
                fail("invalid bank record address");
        }
        const Branch *b = &g->branch[i];
        for (size_t j = 0; j < i; j++)
            if (g->branch[j].prefix_len == b->prefix_len &&
                !memcmp(g->forward[g->branch[j].rule_id].event,
                        g->forward[b->rule_id].event, b->prefix_len))
                fail("duplicate bank prefix content");
    }
    free(bytes);
    return bank;
}

static void eb_same_rules(const Grammar *a, const Grammar *b) {
    if (a->count != b->count) fail("bank/control rule count differs");
    for (size_t i = 0; i < a->count; i++)
        if (a->rule[i].left != b->rule[i].left || a->rule[i].right != b->rule[i].right)
            fail("bank/control grammar differs");
}

static void eb_same_address(const Branch *a, const Branch *b) {
    if (a->rule_id != b->rule_id || a->prefix_len != b->prefix_len)
        fail("bank/control selected address differs");
}

static void eb_check_projection(const EBank *bank, const Grammar *full,
                                const Grammar *small, const EBank *permuted) {
    const Grammar *a = &bank->book[0], *b = &bank->book[1];
    eb_same_rules(a, full);
    eb_same_rules(a, small);
    eb_same_rules(a, &permuted->book[0]);
    size_t capacity = (MAX_ARCHIVE_BYTES - 16 - 4 * a->count) / 32;
    size_t expected = full->records < capacity ? full->records : capacity;
    if (a->records != expected || small->records != a->records ||
        permuted->book[0].records != a->records || permuted->bytes != bank->bytes)
        fail("bank/control record projection differs");
    for (size_t i = 0; i < a->records; i++) {
        eb_same_address(&a->branch[i], &full->branch[i]);
        eb_same_address(&a->branch[i], &small->branch[i]);
        for (int c = 0; c < 2; c++)
            eb_same_address(&a->branch[i], &permuted->book[c].branch[i]);
        for (int r = 0; r < 7; r++) {
            unsigned total = (unsigned)a->branch[i].counts[r] + b->branch[i].counts[r];
            if (total > UINT16_MAX || total != small->branch[i].counts[r] ||
                total != full->branch[i].counts[r]) fail("source-case pooled count mismatch");
            int old = r ? r % 6 + 1 : 0;
            for (int c = 0; c < 2; c++)
                if (permuted->book[c].branch[i].counts[r] != bank->book[c].branch[i].counts[old])
                    fail("permuted source count mismatch");
        }
    }
}

static Match eb_record_match(const Grammar *g, int record) {
    Match match = {0};
    if (record < 0) return match;
    if ((size_t)record >= g->records) fail("record index outside bank");
    const Branch *b = &g->branch[record];
    match.length = b->prefix_len;
    match.nmatches = 1;
    match.matches[0] = record;
    for (int j = 0; j < 7; j++) match.votes[j] = b->counts[j];
    return match;
}

static void eb_quote(const EBRouter *router, const double cold[256],
                      const double a[256], const double b[256], double out[256]) {
    eb_check_router(router);
    double lw[3];
    for (int i = 0; i < 3; i++) lw[i] = log2(router->weight[i]);
    for (int byte = 0; byte < 256; byte++) {
        if (a[byte] == cold[byte] && b[byte] == cold[byte]) out[byte] = cold[byte];
        else out[byte] = la(la(lw[0] + cold[byte], lw[1] + a[byte]), lw[2] + b[byte]);
    }
}

static void eb_observe(EBRouter *router, double cold, double a, double b, double quoted) {
    double prices[3] = {cold, a, b};
    EBRouter next;
    for (int i = 0; i < 3; i++) {
        double posterior = router->weight[i] * exp2(prices[i] - quoted);
        next.weight[i] = (1.0 - EB_SHARE) * posterior + EB_SHARE * eb_prior[i];
    }
    eb_check_router(&next);
    *router = next;
}

static void eb_print_weights(const EBRouter *router) {
    printf("%.17g,%.17g,%.17g", router->weight[0], router->weight[1], router->weight[2]);
}

static void eb_free_bank(EBank *bank) {
    free(bank->book[0].rule);
    free(bank->book[0].forward);
    free(bank->book[0].branch);
    free(bank->book[1].branch);
}

static void eb_free_grammar(Grammar *g) {
    free(g->rule);
    free(g->forward);
    free(g->branch);
}

static void eb_predict(const char *bank_path, const char *full_path,
                        const char *small_path, const char *permuted_path) {
    EBank bank = eb_load(bank_path), permuted = eb_load(permuted_path);
    Grammar full = load_grammar(full_path), small = load_grammar(small_path);
    eb_check_projection(&bank, &full, &small, &permuted);
    size_t records = bank.book[0].records;
    EBRouter *local = records ? calloc(records, sizeof(*local)) : NULL;
    EBRouter *perm = records ? calloc(records, sizeof(*perm)) : NULL;
    if (records && (!local || !perm)) fail("allocate recipient routers");
    for (size_t i = 0; i < records; i++) {
        eb_init(&local[i]);
        eb_init(&perm[i]);
    }
    EBRouter global, prior;
    eb_init(&global);
    eb_init(&prior);
    BFState *front = bf_create();
    PRLife life;
    pr_life_init(&life);
    if (!front) fail("allocate bank frontend");
    History history = {{0}, 0};
    Outer outer[EB_ARMS] = {{0}};
    double overall_norm = 0.0;
    fputs("t\tk\theads\tcold_heads\ttruth\trank\thistory\tlogcold\tmatched\trecord"
          "\tfull_matched\tfull_record\tbank_a\tbank_b\tperm_a\tperm_b\tmax_norm_error\tnew_exact", stdout);
    fputs("\tlocal_w_before\tlocal_w_after\tglobal_w_before\tglobal_w_after\tpermuted_w_before\tpermuted_w_after", stdout);
    for (int arm = 0; arm < EB_ARMS; arm++)
        printf("\t%s_candidate\t%s_live\t%s_shadow_before\t%s_odds_before\t%s_active_before\t%s_activated_after\t%s_gain_after",
               eb_names[arm], eb_names[arm], eb_names[arm], eb_names[arm], eb_names[arm], eb_names[arm], eb_names[arm]);
    putchar('\n');
    for (;;) {
        Quote q;
        quote_cold(front, &life, &q);
        Match match = branch_match(&bank.book[0], &history);
        Match full_match = branch_match(&full, &history);
        int record = match.nmatches ? match.matches[0] : -1;
        int full_record = full_match.nmatches ? full_match.matches[0] : -1;
        Match book_b = eb_record_match(&bank.book[1], record);
        Match perm_a = eb_record_match(&permuted.book[0], record);
        Match perm_b = eb_record_match(&permuted.book[1], record);
        Match small_match = eb_record_match(&small, record);
        double components[4][256], candidates[EB_ARMS][256], live[EB_ARMS][256];
        candidate(&q, &match, components[0]);
        candidate(&q, &book_b, components[1]);
        candidate(&q, &perm_a, components[2]);
        candidate(&q, &perm_b, components[3]);
        EBRouter before[3] = {record < 0 ? prior : local[record], global,
                              record < 0 ? prior : perm[record]};
        eb_quote(&before[0], q.cold, components[0], components[1], candidates[EB_LOCAL]);
        eb_quote(&before[1], q.cold, components[0], components[1], candidates[EB_GLOBAL]);
        candidate(&q, &full_match, candidates[EB_FULL]);
        candidate(&q, &small_match, candidates[EB_SMALL]);
        eb_quote(&before[2], q.cold, components[2], components[3], candidates[EB_PERMUTED]);
        double norm = fmax(validate(q.bf.logp_base), validate(q.cold));
        for (int j = 0; j < 4; j++) norm = fmax(norm, validate(components[j]));
        for (int arm = 0; arm < EB_ARMS; arm++) {
            for (int byte = 0; byte < 256; byte++) {
                live[arm][byte] = !outer[arm].active || candidates[arm][byte] == q.cold[byte]
                    ? q.cold[byte] : la(q.cold[byte], outer[arm].odds + candidates[arm][byte]) - la(0, outer[arm].odds);
                if (!q.rank[byte] && (candidates[arm][byte] != q.cold[byte] || live[arm][byte] != q.cold[byte]))
                    fail("protected NEW changed in bank arm");
            }
            norm = fmax(norm, validate(candidates[arm]));
            norm = fmax(norm, validate(live[arm]));
        }
        /* Complete source, router and live vectors are fixed before truth. */
        int next = fgetc(stdin);
        if (next == EOF) break;
        uint8_t truth = (uint8_t)next;
        overall_norm = fmax(overall_norm, norm);
        Outer outer_before[EB_ARMS];
        int activated[EB_ARMS];
        for (int arm = 0; arm < EB_ARMS; arm++) {
            outer_before[arm] = outer[arm];
            activated[arm] = observe_outer(&outer[arm], q.bf.t, q.cold[truth], candidates[arm][truth], live[arm][truth]);
        }
        if (record >= 0) {
            /* All matched visits clock, including NEW, equal prices, k=0 and
               visits before the relevant arm's external admission. */
            eb_observe(&local[record], q.cold[truth], components[0][truth], components[1][truth], candidates[EB_LOCAL][truth]);
            eb_observe(&global, q.cold[truth], components[0][truth], components[1][truth], candidates[EB_GLOBAL][truth]);
            eb_observe(&perm[record], q.cold[truth], components[2][truth], components[3][truth], candidates[EB_PERMUTED][truth]);
        }
        EBRouter after[3] = {record < 0 ? prior : local[record], global,
                             record < 0 ? prior : perm[record]};
        printf("%zu\t%d\t", q.bf.t, q.k);
        print_heads(&q, 0);
        putchar('\t');
        print_heads(&q, 1);
        printf("\t%u\t%u\t", (unsigned)truth, (unsigned)q.rank[truth]);
        if (!history.length) putchar('-');
        for (size_t i = 0; i < history.length; i++) putchar('0' + history.event[i]);
        printf("\t%.17g\t%d\t%d\t%d\t%d\t%.17g\t%.17g\t%.17g\t%.17g\t%.17g\t1",
               q.cold[truth], match.length, record, full_match.length, full_record,
               components[0][truth], components[1][truth], components[2][truth], components[3][truth], norm);
        for (int j = 0; j < 3; j++) {
            putchar('\t'); eb_print_weights(&before[j]);
            putchar('\t'); eb_print_weights(&after[j]);
        }
        for (int arm = 0; arm < EB_ARMS; arm++)
            printf("\t%.17g\t%.17g\t%.17g\t%.17g\t%d\t%d\t%.17g",
                   candidates[arm][truth], live[arm][truth], outer_before[arm].shadow,
                   outer_before[arm].odds, outer_before[arm].active, activated[arm], outer[arm].gain);
        putchar('\n');
        observe_cold(front, &life, &q, truth);
        if (history.length == HORIZON) {
            memmove(history.event, history.event + 1, HORIZON - 1);
            history.length--;
        }
        history.event[history.length++] = q.rank[truth];
    }
    stream_end();
    fprintf(stderr, "bank predict: bytes=%" PRIu64 " bank_bytes=%zu pooled_full_bytes=%zu pooled_small_bytes=%zu permuted_bytes=%zu "
                    "rules=%zu bank_records=%zu full_records=%zu router_struct_bytes=%zu "
                    "local_router_bytes=%zu global_router_bytes=%zu permuted_router_bytes=%zu "
                    "router_total_bytes=%zu outer_state_bytes=%zu history_struct_bytes=%zu "
                    "forecast_vector_bytes=%zu max_norm_error=%.17g\n",
            life.events, bank.bytes, full.bytes, small.bytes, permuted.bytes,
            bank.book[0].count, records, full.records, sizeof(EBRouter),
            records * sizeof(EBRouter), sizeof(EBRouter), records * sizeof(EBRouter),
            (2 * records + 1) * sizeof(EBRouter), sizeof(outer), sizeof(history),
            (4 + 2 * EB_ARMS) * 256 * sizeof(double), overall_norm);
    for (int arm = 0; arm < EB_ARMS; arm++)
        fprintf(stderr, "arm=%s gain=%.17g minimum=%.17g drawdown=%.17g active=%d activation=%zu\n",
                eb_names[arm], outer[arm].gain, outer[arm].minimum, outer[arm].drawdown,
                outer[arm].active, outer[arm].activation);
    free(local);
    free(perm);
    bf_destroy(front);
    eb_free_bank(&bank);
    eb_free_bank(&permuted);
    eb_free_grammar(&full);
    eb_free_grammar(&small);
}

static void eb_json_weights(const EBRouter *router) {
    putchar('['); eb_print_weights(router); putchar(']');
}

static void eb_fixture(void) {
    /* Known normalized full-byte distributions, two independent record
       states, and no archive/frontend/world access. */
    Quote q = {0};
    q.k = 3;
    q.heads[0] = 3; q.heads[1] = 7; q.heads[2] = 11;
    for (int byte = 0; byte < 256; byte++) q.cold[byte] = -8.0;
    for (int j = 0; j < 3; j++) q.rank[q.heads[j]] = (uint8_t)(j + 1);
    Match ma = {0}, mb = {0};
    ma.length = mb.length = 1;
    ma.votes[1] = 40; ma.votes[2] = 2; ma.votes[3] = 1;
    mb.votes[1] = 1; mb.votes[2] = 2; mb.votes[3] = 40;
    const int records[8] = {0, 1, 0, 0, -1, 1, 0, 1};
    const uint8_t truths[8] = {3, 11, 23, 7, 3, 11, 3, 7};
    EBRouter local[2], global, prior;
    eb_init(&local[0]); eb_init(&local[1]); eb_init(&global); eb_init(&prior);
    for (int t = 0; t < 8; t++) {
        int record = records[t];
        double a[256], b[256], lq[256], gq[256];
        if (record < 0 || t == 3) {
            memcpy(a, q.cold, sizeof(a)); memcpy(b, q.cold, sizeof(b));
        } else {
            candidate(&q, &ma, a); candidate(&q, &mb, b);
        }
        EBRouter before = record < 0 ? prior : local[record], gb = global;
        eb_quote(&before, q.cold, a, b, lq);
        eb_quote(&gb, q.cold, a, b, gq);
        double norm = fmax(fmax(validate(a), validate(b)), fmax(validate(lq), validate(gq)));
        for (int byte = 0; byte < 256; byte++)
            if ((!q.rank[byte] || (a[byte] == q.cold[byte] && b[byte] == q.cold[byte])) &&
                (lq[byte] != q.cold[byte] || gq[byte] != q.cold[byte])) fail("fixture exact equality");
        uint8_t truth = truths[t];
        if (record >= 0) {
            eb_observe(&local[record], q.cold[truth], a[truth], b[truth], lq[truth]);
            eb_observe(&global, q.cold[truth], a[truth], b[truth], gq[truth]);
        }
        EBRouter after = record < 0 ? prior : local[record];
        printf("{\"t\":%d,\"record\":%d,\"matched\":%d,\"truth\":%u,\"rank\":%u,\"cold\":%.17g,"
               "\"a\":%.17g,\"b\":%.17g,\"local_candidate\":%.17g,\"global_candidate\":%.17g,"
               "\"local_w_before\":", t, record, record >= 0, (unsigned)truth, (unsigned)q.rank[truth],
               q.cold[truth], a[truth], b[truth], lq[truth], gq[truth]);
        eb_json_weights(&before);
        fputs(",\"local_w_after\":", stdout); eb_json_weights(&after);
        fputs(",\"global_w_before\":", stdout); eb_json_weights(&gb);
        fputs(",\"global_w_after\":", stdout); eb_json_weights(&global);
        printf(",\"max_norm_error\":%.17g,\"router_struct_bytes\":%zu}\n", norm, sizeof(EBRouter));
    }
    stream_end();
}

int main(int argc, char **argv) {
    if (argc >= 2 && (!strcmp(argv[1], "emit") || !strcmp(argv[1], "trace")))
        return inherited_episode_main(argc, argv);
    if (setvbuf(stdout, NULL, _IOFBF, 1u << 20)) fail("stdout buffering");
    if (argc == 6 && !strcmp(argv[1], "predict")) eb_predict(argv[2], argv[3], argv[4], argv[5]);
    else if (argc == 2 && !strcmp(argv[1], "--fixture")) eb_fixture();
    else {
        fputs("usage: bank predict BANK POOLED_FULL POOLED_SMALL PERMUTED < RAW\n"
              "       bank emit ALPHABET NONZERO_SEED < COMMANDS\n"
              "       bank trace [compact] < RAW\n"
              "       bank --fixture\n", stderr);
        return 2;
    }
    return 0;
}
